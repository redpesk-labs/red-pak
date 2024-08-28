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
static const char deftemplate_Default[]             = "default";
static const char deftemplate_Admin[]               = "admin";
static const char deftemplate_DefaultNoSystemNode[] = "default-no-system-node";
static const char deftemplate_AdminNoSystemNode[]   = "admin-no-system-node";
static const char deftemplate_System[]              = "main-system";
static const char deftemplate_SystemAdmin[]         = "main-admin-system";

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
    [RednodeFactory_Error_TemplateDir_Too_Long] = "Template path too long",
    [RednodeFactory_Error_Config_Too_Long]      = "Config path too long",
    [RednodeFactory_Error_No_Config]            = "No configuration file",
    [RednodeFactory_Error_Loading_Config]       = "Can't load configuration file",
    [RednodeFactory_Error_Config_Exist]         = "Configuration file already existing",
    [RednodeFactory_Error_Storing_Config]       = "Can't store configuration file",
    [RednodeFactory_Error_Path_Too_Long]        = "Path is too long",
    [RednodeFactory_Error_Storing_Status]       = "Can't store status file"
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
static int load_template_config(const char *tmplname, redConfigT **redconfig)
{
    static const char YAMLEXT[] = ".yaml";

    char path[REDNODE_FACTORY_PATH_LEN];
    size_t sztmplname, sz = 0;

    /* prepend default directory if needed */
    if (strchr(tmplname, '/') == NULL) {
        const char *tmpldir = get_template_dir();
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
        return -RednodeFactory_Error_Config_Too_Long;
    memcpy(&path[sz], tmplname, sztmplname + 1); /*also copy the zero*/
    sz += sztmplname;

    /* append '.yaml' suffix when needed */
    if (access(path, F_OK) != 0
     && sz >= (sizeof YAMLEXT - 1)
     && strcmp(YAMLEXT, &path[sz - (sizeof YAMLEXT - 1)])) {
        if (sz + sizeof YAMLEXT >= sizeof path)
            return -RednodeFactory_Error_Config_Too_Long;
        memcpy(&path[sz], YAMLEXT, sizeof YAMLEXT); /*also copy the zero*/
        sz += sizeof YAMLEXT - 1;
    }

    /* check existing file */
    if (access(path, F_OK))
        return -RednodeFactory_Error_No_Config;

    *redconfig = RedLoadConfig(path, 0);
    if (*redconfig == NULL)
        return -RednodeFactory_Error_Loading_Config;

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

/* create the given node */
static int create_node(rednode_factory_t *rfab, const rednode_factory_param_t *params, bool update)
{
    char today[100];
    char uuid[RED_UUID_STR_LEN];
    char info[150];
    int rc, status;
    redStatusT redstatus;
    redConfigT *normal_config = NULL,  *admin_config = NULL;
    size_t existing_length;
    unsigned long timestamp;

    /* get date and new uuid */
    timestamp = RedUtcGetTimeMs();
    rc = getDateOfToday(today, sizeof today);
    if (rc == 0)
        return -RednodeFactory_Error_FmtDate;
    getFreshUUID(uuid, sizeof uuid);

    /* load the config */
    status = load_template_config(params->normal, &normal_config);
    if (status != RednodeFactory_OK)
        return status;

    /* creation of specific headers */
    if(!update) {
        /* create/copy specific values */
        char *info = (char*)RedNodeStringExpand(NULL, "Node created by $LOGNAME($HOSTNAME) the $TODAY");
        char *name = strdup(uuid);
        char *alias = strdup(params->alias);

        /* check creation status */
        if (info == NULL || name == NULL || alias == NULL) {
            free(info);
            free(name);
            free(alias);
            RedFreeConfig(normal_config, 0);
            return -RednodeFactory_Error_Allocation;
        }

        /* assign to the config */
        free((char *)normal_config->headers->info);
        free((char *)normal_config->headers->name);
        free((char *)normal_config->headers->alias);
        normal_config->headers->info = info;
        normal_config->headers->name = name;
        normal_config->headers->alias = alias;
    }

    /* load the administrative config */
    status = load_template_config(params->admin, &admin_config);
    if (status == RednodeFactory_OK) {

        /* create the directories */
        status = create_directories(rfab->path, rfab->root_length, rfab->node_length, &existing_length);
        if (status == RednodeFactory_OK)
            status = store_config(rfab, get_normal_conf_subpath(), normal_config, update);
        if (status == RednodeFactory_OK)
            status = store_config(rfab, get_admin_conf_subpath(), admin_config, update);
        if (status == RednodeFactory_OK) {
            redstatus.state = RED_STATUS_ENABLE;
            redstatus.realpath = rfab->path;
            redstatus.timestamp = timestamp;
            redstatus.info = info;
            strcpy(stpcpy(info, "created by red-manager the "), today);
            status = store_status(rfab, get_status_subpath(), &redstatus);
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

/* see rednode-factory.h */
int rednode_factory_create_node(
            rednode_factory_t *rfab,
            const rednode_factory_param_t *params,
            bool update,
            bool is_system
) {
    int status, rc;
    size_t off;
    rednode_factory_param_t locparam;
    char *localias = NULL;

    /* check if existing root */
    if (rfab->root_length == 0)
        return -RednodeFactory_Error_Cleared;

    /* get offset of the parent */
    for (off = rfab->node_length ; off && rfab->path[--off] != '/' ;);

    /* setup default params if none was given */
    if (params == NULL) {
        locparam.alias = locparam.normal = locparam.admin = NULL;
        params = &locparam;
    }

    /* get alias */
    if (params->alias && params->alias[0])
        locparam.alias = params->alias; /* alias as set in params */
    else {
        /* alias isn't set, compute it from directory name */
        if (rfab->node_length == rfab->root_length || rfab->node_length < off + 1)
            return -RednodeFactory_Error_Default_Alias_Empty;
        localias = strndup(&rfab->path[off + 1], rfab->node_length - off - 1);
        if (localias == NULL)
            return -RednodeFactory_Error_Allocation;
        locparam.alias = localias;
    }

    /* get template */
    if (params->normal && params->normal[0])
        locparam.normal = params->normal;
    else if (is_system)
        locparam.normal = deftemplate_Default;
    else
        locparam.normal = deftemplate_DefaultNoSystemNode;

    /* get admin template */
    if (params->admin && params->admin[0])
        locparam.admin = params->admin;
    else if (is_system)
        locparam.admin = deftemplate_Admin;
    else
        locparam.admin = deftemplate_AdminNoSystemNode;

    /* check parent exists */
    status = RednodeFactory_OK;
    if(is_system && off + 1 >= rfab->root_length) {
        rfab->path[off] = 0;
        rc = access(rfab->path, F_OK);
        rfab->path[off] = '/';
        if (rc != 0) {
            rednode_factory_t sysfab;
            rednode_factory_param_t syspar;
            sysfab.root_length = rfab->root_length;
            sysfab.node_length = off + 1;
            memcpy(sysfab.path, rfab->path, off + 1);
            syspar.alias = "system";
            syspar.normal = deftemplate_System;
            syspar.admin = deftemplate_SystemAdmin;
            status = rednode_factory_create_node(&sysfab, &syspar, false, true);
        }
    }

    /* create the node now */
    if (status == RednodeFactory_OK)
        status = create_node(rfab, &locparam, update);

    /* end */
    free(localias);
    return status;
}

