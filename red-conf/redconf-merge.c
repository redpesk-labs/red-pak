/*
* Copyright (C) 2022-2025 IoT.bzh Company
* Author: Clément Bénier <clement.benier@iot.bzh>
* Author: Fulup Ar Foll <fulup@iot.bzh>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#define _GNU_SOURCE

#include "redconf-merge.h"

#include <stdlib.h>
#include <string.h>

#include "redconf-log.h"
#include "redconf-schema.h"
#include "redconf-mix.h"
#include "redconf-expand.h"
#include "redconf-defaults.h"
#include "redconf-utils.h"
#include "redconf-sharing.h"

static int cpstrinn(const char **dst, const char *src)
{
    return -(src != NULL && (*dst = strdup(src)) == NULL);
}

/* set bwrap arguments for given conftag */
static int mixconftag_cb(void *closure, const redConfTagT *conftag)
{
    redNodeT   *merge = (redNodeT*)closure;
    redConfTagT *ctag = &merge->config->conftag;

    /* scalars */
    ctag->gpgcheck = conftag->gpgcheck;
    ctag->unsafe = conftag->unsafe;
    ctag->inheritenv = conftag->inheritenv;
    ctag->maprootuser = conftag->maprootuser;
    ctag->verbose = conftag->verbose;
    ctag->diewithparent = conftag->diewithparent;
    ctag->newsession = conftag->newsession;

    /* capabilities */
    ctag->capabilities_count = conftag->capabilities_count;
    if (ctag->capabilities_count == 0)
        ctag->capabilities = NULL;
    else {
        size_t size = ctag->capabilities_count * sizeof *conftag->capabilities;
        ctag->capabilities = malloc(size);
        if (ctag->capabilities == NULL)
            return -1;
        memcpy(ctag->capabilities, conftag->capabilities, size);
    }
    return cpstrinn(&ctag->rpmdir, conftag->rpmdir)
    || cpstrinn(&ctag->persistdir, conftag->persistdir)
    || cpstrinn(&ctag->cachedir, conftag->cachedir)
    || cpstrinn(&ctag->path, conftag->path)
    || cpstrinn(&ctag->ldpath, conftag->ldpath)
    || cpstrinn(&ctag->hostname, conftag->hostname)
    || cpstrinn(&ctag->umask, conftag->umask)
    || cpstrinn(&ctag->chdir, conftag->chdir)
    || cpstrinn(&ctag->cgrouproot, conftag->cgrouproot)
    || cpstrinn(&ctag->setuser, conftag->setuser)
    || cpstrinn(&ctag->setgroup, conftag->setgroup)
    || cpstrinn(&ctag->smack, conftag->smack)
    || cpstrinn(&ctag->share.all, conftag->share.all)
    || cpstrinn(&ctag->share.user, conftag->share.user)
    || cpstrinn(&ctag->share.cgroup, conftag->share.cgroup)
    || cpstrinn(&ctag->share.ipc, conftag->share.ipc)
    || cpstrinn(&ctag->share.pid, conftag->share.pid)
    || cpstrinn(&ctag->share.net, conftag->share.net)
    || cpstrinn(&ctag->share.time, conftag->share.time);
}



/* get the PATH */
static const char *get_config_path(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.path;
}

/* set the PATH */
static int set_env_path(void *closure, const char *value, size_t length)
{
    redNodeT   *merge = (redNodeT*)closure;
    redConfigT *config = merge->config;
    (void)length;
    return cpstrinn(&config->conftag.path, value);
}

/* get the LD_LIBRARY_PATH */
static const char *get_config_ldpath(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.ldpath;
}

/* set the LD_LIBRARY_PATH */
static int set_env_ldpath(void *closure, const char *value, size_t length)
{
    redNodeT   *merge = (redNodeT*)closure;
    redConfigT *config = merge->config;
    (void)length;
    return cpstrinn(&config->conftag.ldpath, value);
}

/* callback for setting environment variables */
static int mixvar_cb(void *closure, const redConfVarT *var, const redNodeT *node, unsigned index, unsigned count)
{
    redNodeT   *merge = (redNodeT*)closure;
    redConfigT *config = merge->config;
    redConfVarT *vars = config->confvar;
    (void)node;

    if (index == 0) {
        vars = calloc(count, sizeof *vars);
        if (vars == NULL)
            return -1;
        config->confvar = vars;
        config->confvar_count = count;
    }
    vars = &vars[index];
    vars->mode = var->mode;
    return cpstrinn(&vars->key, var->key)
    || cpstrinn(&vars->value, var->value)
    || cpstrinn(&vars->info, var->info)
    || cpstrinn(&vars->warn, var->warn);
}

/* callback for setting exports */
static int mixexp_cb(void *closure, const redConfExportPathT *exp, const redNodeT *node, unsigned index, unsigned count)
{
    redNodeT   *merge = (redNodeT*)closure;
    redConfigT *config = merge->config;
    redConfExportPathT *exports = config->exports;
    (void)node;

    if (index == 0) {
        exports = calloc(count, sizeof *exports);
        if (exports == NULL)
            return -1;
        config->exports = exports;
        config->exports_count = count;
    }
    exports = &exports[index];
    exports->mode = exp->mode;
    return cpstrinn(&exports->mount, exp->mount)
    || cpstrinn(&exports->path, exp->path)
    || cpstrinn(&exports->info, exp->info)
    || cpstrinn(&exports->warn, exp->warn);
}

int get_merged_node(redNodeT **result, const redNodeT *leaf)
{
    int rc;
    redNodeT *merge;

    /* allocate structures */
    merge = calloc(1, sizeof *merge);
    if (merge == NULL)
        goto out_of_memory;

    merge->config =  calloc(1, sizeof *merge->config);
    if (merge->config == NULL)
        goto out_of_memory;

    /* init basic data */
    merge->redpath = strdup(leaf->redpath);
    merge->config->headers.alias = strdup(leaf->config->headers.alias);
    merge->config->headers.name = strdup(leaf->config->headers.name);
    merge->config->headers.info = strdup(leaf->config->headers.info);
    if (merge->redpath == NULL
     || merge->config->headers.alias == NULL
     || merge->config->headers.name == NULL
     || merge->config->headers.info == NULL)
        goto out_of_memory;

    /* set config */
    rc = mixConfTags(leaf, mixconftag_cb, merge);
    if (rc)
        goto error;

    /* set exports */
    rc = mixExports(leaf, mixexp_cb, merge);
    if (rc)
        goto error;

    /* set vars */
    rc = mixVariables(leaf, mixvar_cb, merge);
    if (rc)
        goto error;

    /* set PATH */
    rc = mixPaths(leaf, get_config_path, set_env_path, merge);
    if (rc)
        goto error;

    /* set LD_PATH_LIBRARY */
    rc = mixPaths(leaf, get_config_ldpath, set_env_ldpath, merge);
    if (rc)
        goto error;

    *result = merge;
    return 0;

out_of_memory:
    RedLog(REDLOG_ERROR, "Memory allocation failed");
    rc = -1;
error:
    freeRedLeaf(merge);
    *result = NULL;
    return rc;
}

redNodeT *mergeNode(const redNodeT *leaf, const redNodeT* root, int expand, int duplicate)
{
    redNodeT *result;
    int rc = get_merged_node(&result, leaf);
    (void)root;
    (void)expand;
    (void)duplicate;
    return rc ? NULL : result;
}
