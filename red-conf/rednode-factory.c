/*
 Copyright 2021 IoT.bzh

 author: José Bollo <jose.bollo@iot.bzh>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#define _GNU_SOURCE

#include "rednode-factory.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "redconf-defaults.h"
#include "redconf-expand.h"
#include "redconf-node.h"
#include "redconf-utils.h"
#include "redconf-valid.h"

/*********************************************************************************/
/*********************************************************************************/
/** PRIVATE PART                                                                **/
/*********************************************************************************/
/*********************************************************************************/

#define REDNODE_TEMPLATE_DIR         NODE_PREFIX""redpak_TMPL
#define REDNODE_NORMAL_CONF_SUBPATH  REDNODE_CONF
#define REDNODE_ADMIN_CONF_SUBPATH   REDNODE_ADMIN
#define REDNODE_STATUS_SUBPATH       REDNODE_STATUS

/* default template names */
const char rednode_factory_deftmpl_full[] = "full";
const char rednode_factory_deftmpl_root[] = "root";
const char rednode_factory_deftmpl_leaf[] = "leaf";

/* automatic admin aliasing */
static const char ADMIN_ALIAS_PREFIX[] = "admin-of-";

/* name deduction from model */
static const char YAMLEXT[] = ".yaml";
static const char STDEXT[] = "-normal.yaml";
static const char ADMEXT[] = "-admin.yaml";

/* error code */
static const char *text_of_errors[] =
{
    [RednodeFactory_OK]                         = "No error",
    [RednodeFactory_Error_Cleared]              = "Factory cleared",
    [RednodeFactory_Error_Allocation]           = "Out of memory",
    [RednodeFactory_Error_Root_Too_Long]        = "Root path too long",
    [RednodeFactory_Error_Root_Not_Absolute]    = "Root path isn't absolute",
    [RednodeFactory_Error_Root_Not_Exist]       = "Root path isn't existing",
    [RednodeFactory_Error_Node_Too_Long]        = "Node path too long",
    [RednodeFactory_Error_Default_Alias_Empty]  = "Default alias is empty",
    [RednodeFactory_Error_MkDir]                = "Can't create directories",
    [RednodeFactory_Error_FmtDate]              = "Can't create current date",
    [RednodeFactory_Error_TemplateDir_Too_Long] = "Template directory path too long",
    [RednodeFactory_Error_Template_Too_Long]    = "Config path too long",
    [RednodeFactory_Error_No_Template]          = "No configuration file",
    [RednodeFactory_Error_Loading_Template]     = "Can't load configuration file",
    [RednodeFactory_Error_Config_Exist]         = "Configuration file already existing",
    [RednodeFactory_Error_Storing_Config]       = "Can't store configuration file",
    [RednodeFactory_Error_Path_Too_Long]        = "Path is too long",
    [RednodeFactory_Error_Storing_Status]       = "Can't store status file",
    [RednodeFactory_Error_At_Root]              = "Can't inspect below root path",
    [RednodeFactory_Error_Config_Not_Exist]     = "Can't update not existing config",
    [RednodeFactory_Error_Invalid_Alias]        = "Invalid alias",
};

static const char *get_default_item(const char *defvalue, const char *varname)
{
    const char *value = varname ? secure_getenv(varname) : NULL;
    return value ?: defvalue;
}

static const char *get_template_dir()
{
    return get_default_item(REDNODE_TEMPLATE_DIR, "REDNODE_TEMPLATE_DIR");
}

static const char *get_normal_conf_subpath()
{
    return get_default_item(REDNODE_NORMAL_CONF_SUBPATH, "REDNODE_NORMAL_CONF_SUBPATH");
}

static const char *get_admin_conf_subpath()
{
    return get_default_item(REDNODE_ADMIN_CONF_SUBPATH, "REDNODE_ADMIN_CONF_SUBPATH");
}

static const char *get_status_subpath()
{
    return get_default_item(REDNODE_STATUS_SUBPATH, "REDNODE_STATUS_SUBPATH");
}

/* create the directories */
static int create_directories(char path[REDNODE_FACTORY_PATH_LEN], size_t root_length, size_t length, size_t *existing_length)
{
    int rc;
    mode_t mode = 00755;

    /* check that root exists */
    if (root_length > 1) {
        path[root_length - 1] = 0;
        rc = access(path, F_OK);
        path[root_length - 1] = '/';
        if (rc != 0)
            return -RednodeFactory_Error_Root_Not_Exist;
    }

    rc = make_directories(path, root_length, length, mode, existing_length);
    return rc < 0 ? RednodeFactory_Error_MkDir : RednodeFactory_OK;
}

/* create the directory caontaining the given path */
static int create_parent_directories(char path[REDNODE_FACTORY_PATH_LEN], size_t root_length, size_t length, size_t *existing_length)
{
    while (length > root_length && path[length -1] != '/')
        length--;
    return create_directories(path, root_length, length, existing_length);
}

/* load the template configuration in redconfig */
static int load_template_config(const rednode_factory_param_t *params, bool admin, redConfigT **redconfig)
{
    char path[REDNODE_FACTORY_PATH_LEN];
    size_t sztmplname, sz = 0;
    const char *tmplname;

    /* get the name */
    tmplname = admin ? params->admin : params->normal;

    /* prepend default directory if needed */
    if (strchr(tmplname, '/') == NULL) {
        const char *tmpldir = params->templatedir;
        sz = strlen(tmpldir);
        while (sz > 0 && tmpldir[sz - 1] == '/')
            sz--;
        if (sz > 0 && sz < sizeof path)
            memcpy(path, tmpldir, sz);
        if (sz < sizeof path)
            path[sz++] = '/';
        if (sz >= sizeof path)
            return -RednodeFactory_Error_TemplateDir_Too_Long;
    }

    /* copy the template name */
    sztmplname = strlen(tmplname);
    if (sztmplname + sz >= sizeof path)
        return -RednodeFactory_Error_Template_Too_Long;
    memcpy(&path[sz], tmplname, sztmplname + 1); /*also copy the zero*/
    sz += sztmplname;

    /* check existing file */
    if (access(path, F_OK) != 0) {
        /* check if path suffixed by .yaml */
        if (sz < (sizeof YAMLEXT - 1) || 0 == strcmp(YAMLEXT, &path[sz - (sizeof YAMLEXT - 1)]))
            /* yes suffixed by .yaml, stop here */
            return -RednodeFactory_Error_No_Template;

        /* try to add the suffix .yaml and check existing */
        if (sz + sizeof YAMLEXT >= sizeof path)
            return -RednodeFactory_Error_Template_Too_Long;
        memcpy(&path[sz], YAMLEXT, sizeof YAMLEXT); /*also copy the zero*/
        if (access(path, F_OK) == 0)
            /* okay, existing */
            sz += sizeof YAMLEXT - 1;
        else if (admin) {
            /* not existing, try with the suffix -admin.yaml */
            if (sz + sizeof ADMEXT >= sizeof path)
                return -RednodeFactory_Error_Template_Too_Long;
            memcpy(&path[sz], ADMEXT, sizeof ADMEXT); /*also copy the zero*/
            if (access(path, F_OK) != 0)
                return -RednodeFactory_Error_No_Template;
            sz += sizeof ADMEXT - 1;
        }
        else {
            /* not existing, try with the suffix -normal.yaml */
            if (sz + sizeof STDEXT >= sizeof path)
                return -RednodeFactory_Error_Template_Too_Long;
            memcpy(&path[sz], STDEXT, sizeof STDEXT); /*also copy the zero*/
            if (access(path, F_OK) != 0)
                return -RednodeFactory_Error_No_Template;
            sz += sizeof STDEXT - 1;
        }
    }

    /* load rednode configuration */
    *redconfig = RedLoadConfig(path, 0);
    if (*redconfig == NULL)
        return -RednodeFactory_Error_Loading_Template;

    return RednodeFactory_OK;
}

/* compute the configuration of redconfig */
static int set_subpath_path(rednode_factory_t *rfab, const char *subpath, bool createdirs)
{
    size_t sz, esz;

    /* check config length */
    sz = strlen(subpath);
    if (sz + rfab->node_length >= sizeof rfab->path)
        return -RednodeFactory_Error_Path_Too_Long;

    /* make the path */
    memcpy(&rfab->path[rfab->node_length], subpath, sz + 1); /* include last zero */

    sz += rfab->node_length;

    /* ensure existing directories */
    return createdirs ? create_parent_directories(rfab->path, rfab->root_length, sz, &esz) : RednodeFactory_OK;
}

/* store the given redconfig */
static int store_config(rednode_factory_t *rfab, const char *subpath, redConfigT *redconfig, bool update)
{
    int status;

    /* get the path */
    status = set_subpath_path(rfab, subpath, true);
    if (status == RednodeFactory_OK) {

        /* check no unexpected overwrite */
        if (!update && access(rfab->path, F_OK) == 0)
            status = -RednodeFactory_Error_Config_Exist;

        /* save the config now */
        if (status == RednodeFactory_OK && RedSaveConfig(rfab->path, redconfig, 0) != 0)
            status = -RednodeFactory_Error_Storing_Config;
    }
    return status;
}

/* store the given redstatus */
static int store_status(rednode_factory_t *rfab, const char *subpath, redStatusT *redstatus)
{
    int status;

    /* get the path */
    status = set_subpath_path(rfab, subpath, true);
    if (status == RednodeFactory_OK) {

        /* save the status now */
        if (RedSaveStatus(rfab->path, redstatus, 0) != 0)
            status = -RednodeFactory_Error_Storing_Status;
    }
    return status;
}

/* make freash header data */
static int make_config_headers(redConfigT *config, const char *alias, bool admin)
{
    char *h_info, *h_name, *h_alias;

    /* create/copy specific values */
    h_info = (char*)RedNodeStringExpand(NULL, "Node created by $LOGNAME($HOSTNAME) the $TODAY");
    h_name = malloc(RED_UUID_STR_LEN);
    h_alias = admin ? malloc(strlen(alias) + sizeof(ADMIN_ALIAS_PREFIX)) : strdup(alias);

    /* check creation status */
    if (h_info == NULL || h_name == NULL || h_alias == NULL) {
        free(h_info);
        free(h_name);
        free(h_alias);
        return -RednodeFactory_Error_Allocation;
    }

    /* get new uuid */
    getFreshUUID(h_name, RED_UUID_STR_LEN);

    /* make the alias */
    if (admin) {
        strcpy(h_alias, ADMIN_ALIAS_PREFIX);
        strcpy(&h_alias[sizeof(ADMIN_ALIAS_PREFIX) - 1], alias);
    }

    /* assign to the config */
    free((char *)config->headers.info);
    free((char *)config->headers.name);
    free((char *)config->headers.alias);
    config->headers.info = h_info;
    config->headers.name = h_name;
    config->headers.alias = h_alias;

    return RednodeFactory_OK;
}

/* reload header data from existing */
static int update_config_headers(redConfigT *config, rednode_factory_t *rfab, const char *subpath)
{
    char *h_info, *h_name, *h_alias;
    redConfigT *prvcfg;
    int status;

    /* get the path */
    status = set_subpath_path(rfab, subpath, true);
    if (status != RednodeFactory_OK)
        return status;

    /* load the config */
    prvcfg = RedLoadConfig(rfab->path, 0);
    if (prvcfg == NULL)
        return -RednodeFactory_Error_Config_Not_Exist;

    /* check validity of updated alias */
    if (!is_valid_alias(prvcfg->headers.alias)) {
        RedFreeConfig(prvcfg, 0);
        return -RednodeFactory_Error_Invalid_Alias;
    }

    /* copy header values */
    h_info = strdup(prvcfg->headers.info);
    h_name = strdup(prvcfg->headers.name);
    h_alias = strdup(prvcfg->headers.alias);
    RedFreeConfig(prvcfg, 0);

    /* check creation status */
    if (h_info == NULL || h_name == NULL || h_alias == NULL) {
        free(h_info);
        free(h_name);
        free(h_alias);
        return -RednodeFactory_Error_Allocation;
    }

    /* assign to the config */
    free((char *)config->headers.info);
    free((char *)config->headers.name);
    free((char *)config->headers.alias);
    config->headers.info = h_info;
    config->headers.name = h_name;
    config->headers.alias = h_alias;

    return RednodeFactory_OK;
}

/* create the given node */
static int create_node(rednode_factory_t *rfab, const rednode_factory_param_t *params, bool update)
{
    char today[100];
    char info[150];
    int rc, status;
    redStatusT redstatus;
    redConfigT *normal_config = NULL,  *admin_config = NULL;
    size_t existing_length;
    unsigned long timestamp;

    /* check alias name */
    if (!is_valid_alias(params->alias))
        return -RednodeFactory_Error_Invalid_Alias;

    /* get date */
    timestamp = RedUtcGetTimeMs();
    rc = getDateOfToday(today, sizeof today);
    if (rc == 0)
        return -RednodeFactory_Error_FmtDate;

    /* load the config */
    status = load_template_config(params, false, &normal_config);
    if (status != RednodeFactory_OK)
        return status;

    /* creation of specific headers */
    status = update ? update_config_headers(normal_config, rfab, get_normal_conf_subpath())
                    : make_config_headers(normal_config, params->alias, false);

    /* load the administrative config */
    if (status == RednodeFactory_OK) {
        status = load_template_config(params, true, &admin_config);
        if (status == RednodeFactory_OK) {

            /* creation of specific headers */
            status = update ? update_config_headers(admin_config, rfab, get_admin_conf_subpath())
                            : make_config_headers(admin_config, params->alias, true);

            /* create the directories */
            if (status == RednodeFactory_OK)
                status = create_directories(rfab->path, rfab->root_length, rfab->node_length, &existing_length);

            /* store the configs */
            if (status == RednodeFactory_OK)
                status = store_config(rfab, get_normal_conf_subpath(), normal_config, update);
            if (status == RednodeFactory_OK)
                status = store_config(rfab, get_admin_conf_subpath(), admin_config, update);

            /* create and store the status */
            if (status == RednodeFactory_OK) {
                char realpath[rfab->node_length];
                memcpy(realpath, rfab->path, rfab->node_length - 1);
                realpath[rfab->node_length - 1] = 0;
                redstatus.realpath = realpath;
                redstatus.state = RED_STATUS_ENABLE;
                redstatus.timestamp = timestamp;
                redstatus.info = info;
                strcpy(stpcpy(info, "created by red-manager the "), today);
                status = store_status(rfab, get_status_subpath(), &redstatus);
            }
        }
        /* cleanup */
        RedFreeConfig(admin_config, 0);
    }
    RedFreeConfig(normal_config, 0);
    return status;
}

/*********************************************************************************/
/*********************************************************************************/
/** PUBLIC PART                                                                 **/
/*********************************************************************************/
/*********************************************************************************/

/* see rednode-factory.h */
const char *rednode_factory_error_text(rednode_factory_error_t error)
{
    const char *result = NULL;
    if (error >= 0 && error < (sizeof text_of_errors / sizeof *text_of_errors))
        result = text_of_errors[error];
    return result != NULL ? result : "Unexpected error";
}

/* see rednode-factory.h */
void rednode_factory_clear(rednode_factory_t *rfab)
{
    memset(rfab, 0, sizeof *rfab);
}

/* see rednode-factory.h */
int rednode_factory_set_root_len(rednode_factory_t *rfab, const char *path, size_t length)
{
    while (length > 0 && path[length - 1] == '/')
        length--;
    if (length > 0) {
        if (length + 1 >= sizeof rfab->path)
            return -RednodeFactory_Error_Root_Too_Long;
        if (path[0] != '/')
            return -RednodeFactory_Error_Root_Not_Absolute;
        memcpy(rfab->path, path, length);
    }
    rfab->path[length++] = '/';
    rfab->root_length = length;
    rfab->node_length = length; /* empty node length */
    return RednodeFactory_OK;
}

/* see rednode-factory.h */
int rednode_factory_set_root(rednode_factory_t *rfab, const char *path)
{
    return rednode_factory_set_root_len(rfab, path, strlen(path));
}

/* see rednode-factory.h */
int rednode_factory_set_node_len(rednode_factory_t *rfab, const char *path, size_t length)
{
    /* check if existing root */
    if (rfab->root_length == 0)
        return -RednodeFactory_Error_Cleared;

    while (length > 0 && path[length - 1] == '/')
        length--;
    while (length > 0 && path[0] == '/')
        { path++; length--; }
    if (length == 0)
        rfab->node_length = rfab->root_length;
    else {
        if (length + 1 + rfab->root_length >= sizeof rfab->path)
            return -RednodeFactory_Error_Node_Too_Long;
        memcpy(rfab->path + rfab->root_length, path, length);
        length += rfab->root_length;
        rfab->path[length++] = '/';
        rfab->node_length = length;
    }
    return RednodeFactory_OK;
}

/* see rednode-factory.h */
int rednode_factory_set_node(rednode_factory_t *rfab, const char *path)
{
    return rednode_factory_set_node_len(rfab, path, strlen(path));
}

/**
* Check if the file DIRPATH/STATUS exists
*
* @param dirpath the path of the directory to be checked
* @param length the length of path in bytes
*
* @return true if existing or else false
*/
static bool is_rednode(const char *dirpath, size_t length)
{
    const char *stssubpath = get_status_subpath();
    size_t sz = strlen(stssubpath);
    char buffer[length + sz + 2];
    memcpy(buffer, dirpath, length);
    buffer[length] = '/';
    memcpy(&buffer[length + 1], stssubpath, sz);
    buffer[length + sz + 1] = 0;
    return access(buffer, F_OK) == 0;
}

/**
* Create a system node based on settings of the factory.
*
* @param rfab the factory context
* @param node_length the length of the node to create
* @param templatedir path of templates' directory
*
* @return @ref RednodeFactory_OK on success or a negative value on error.
* When error is returned, the absolute value returned is one of these:
*   - @ref RednodeFactory_Error_Cleared when root wasn't already set
*   - @ref RednodeFactory_Error_Allocation when memory got exhausted
*   - @ref RednodeFactory_Error_Default_Alias_Empty when params->alias == NULL
*     and when the node path is the same that the root path
*   - @ref RednodeFactory_Error_FmtDate abnormal error when computing date
*/
static int create_root_node(rednode_factory_t *rfab, size_t node_length, const char *templatedir)
{
        rednode_factory_t sysfab;
        rednode_factory_param_t syspar;
        sysfab.root_length = rfab->root_length;
        sysfab.node_length = node_length;
        memcpy(sysfab.path, rfab->path, node_length);
        syspar.alias = "system";
        syspar.normal = rednode_factory_deftmpl_root;
        syspar.admin = rednode_factory_deftmpl_root;
        syspar.templatedir = templatedir;
        return create_node(&sysfab, &syspar, false);
}

/* see rednode-factory.h */
int rednode_factory_create_node(
            rednode_factory_t *rfab,
            const rednode_factory_param_t *params,
            bool update,
            rednode_factory_mode_t mode
) {
    int status;
    size_t off, len;
    rednode_factory_param_t locparam;
    char *localias = NULL;

    /* check if existing root */
    if (rfab->root_length == 0)
        return -RednodeFactory_Error_Cleared;

    /*
    * get offset of the parent
    * example:
    *     root_length   off   node_length
    *               V    V    V
    *    /some/root/with/node/
    *                    <-->
    *                    len
    */
    len = rfab->node_length;
    while (len && rfab->path[len - 1] == '/')
        len--;
    for (off = len ; off && rfab->path[off - 1] != '/' ; off--);
    len -= off;

    /* setup default params if none was given */
    if (params == NULL) {
        locparam.alias = locparam.normal = locparam.admin = locparam.templatedir = NULL;
        params = &locparam;
    }

    /* template directory */
    if (params->templatedir != NULL)
        locparam.templatedir = params->templatedir;
    else
        locparam.templatedir = get_template_dir();

    /* get alias */
    if (params->alias && params->alias[0])
        locparam.alias = params->alias; /* alias as set in params */
    else {
        /* alias isn't set, compute it from directory name */
        if (rfab->node_length == rfab->root_length || len == 0)
            return -RednodeFactory_Error_Default_Alias_Empty;
        localias = strndup(&rfab->path[off], len);
        if (localias == NULL)
            return -RednodeFactory_Error_Allocation;
        locparam.alias = localias;
    }

    /* get template */
    if (params->normal && params->normal[0])
        locparam.normal = params->normal;
    else if (mode == RednodeFactory_Mode_Legacy_NoSys)
        locparam.normal = rednode_factory_deftmpl_full;
    else
        locparam.normal = rednode_factory_deftmpl_leaf;

    /* get admin template */
    if (params->admin && params->admin[0])
        locparam.admin = params->admin;
    else if (mode == RednodeFactory_Mode_Legacy_NoSys)
        locparam.admin = rednode_factory_deftmpl_full;
    else
        locparam.admin = rednode_factory_deftmpl_leaf;

    /* check if required parent system exists */
    status = RednodeFactory_OK;
    if(mode == RednodeFactory_Mode_Legacy) {
        if (off < rfab->root_length)
            status = -RednodeFactory_Error_At_Root;
        else if (!is_rednode(rfab->path, off - 1))
            status = create_root_node(rfab, off, locparam.templatedir);
    }

    /* create the node now */
    if (status == RednodeFactory_OK)
        status = create_node(rfab, &locparam, update);

    /* end */
    free(localias);
    return status;
}

