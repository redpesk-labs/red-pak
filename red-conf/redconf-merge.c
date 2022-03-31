/*
* Copyright (C) 2022 "IoT.bzh"
* Author Clément Bénier <clement.benier@iot.bzh>
* Author Fulup Ar Foll <fulup@iot.bzh>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <search.h>


#include "lookup3.h"
#include "redconf-merge.h"
#include "redconf-utils.h"
#include "redconf-hash.h"

static const char *expandAlloc(const redNodeT *node, const char *input, int expand) {
    if (!input)
        return NULL;

    if(expand)
        return RedNodeStringExpand(node, NULL, input, NULL, NULL);
    else
        return strdup(input);
}

// Only merge tags that should overloaded
static int RedConfCopyConfTags(redConfTagT *source, redConfTagT *destination) {
    if(!source || !destination) {
        RedLog(REDLOG_ERROR, "source=%p or destination=%p is NULL", source, destination);
        goto OnError;
    }

    if (source->cachedir) destination->cachedir = source->cachedir;
    if (source->hostname) destination->hostname = source->hostname;
    if (source->chdir) destination->chdir = source->chdir;
    if (source->umask) destination->umask = source->umask;
    if (source->verbose) destination->verbose = source->verbose;
    if (source->diewithparent) destination->diewithparent = source->diewithparent;
    if (source->newsession) destination->newsession = source->newsession;
    if (source->umask)  destination->umask = source->umask;

    if(destination->share_all != RED_CONF_OPT_DISABLED) destination->share_all = source->share_all;
    if(destination->share_user != RED_CONF_OPT_DISABLED) destination->share_user = source->share_user;
    if(destination->share_ipc != RED_CONF_OPT_DISABLED) destination->share_ipc = source->share_ipc;
    if(destination->share_cgroup != RED_CONF_OPT_DISABLED) destination->share_cgroup =source->share_cgroup;
    if(destination->share_pid != RED_CONF_OPT_DISABLED) destination->share_pid = source->share_pid;
    if(destination->share_net != RED_CONF_OPT_DISABLED) destination->share_net = source->share_net;

    return 0;
OnError:
    return 1;
}

int mergeSpecialConfVar(const redNodeT *node, dataNodeT *dataNode) {
    int error;
    // node looks good extract path/ldpath before adding red-wrap cli program+arguments
    error= RedConfAppendEnvKey(dataNode->ldpathString, (int *)&dataNode->ldpathIdx, RED_MAXPATHLEN, node->config->conftag->ldpath, NULL, ":", NULL);
    if (error) goto OnErrorExit;

    error= RedConfAppendEnvKey(dataNode->pathString, (int *)&dataNode->pathIdx, RED_MAXPATHLEN, node->config->conftag->path, NULL, ":",NULL);
    if (error) goto OnErrorExit;

OnErrorExit:
    return error;
}

//get mergedConftags from hierarchy
redConfTagT *mergedConftags(const redNodeT *rootnode, int admin) {
    redConfTagT *mergedConfTags = calloc(1, sizeof(redConfTagT));
    memset(mergedConfTags, 0, sizeof(redConfTagT));

    for (const redNodeT *node=rootnode; node; node=node->childs->child) {
        (void) RedConfCopyConfTags(node->config->conftag, mergedConfTags);
        if(!node->ancestor) { //system_node
            // update process default umask
            RedSetUmask (mergedConfTags);
        }
    }

    //assume admin overload everything exepted node specific
    if (admin) {
        for (const redNodeT *node=rootnode; node != NULL; node=node->ancestor) {
	        if (!node->confadmin->conftag) {
                RedLog(REDLOG_INFO, "no admin conftag for %s", node->redpath);
		        continue;
            }
            (void) RedConfCopyConfTags(node->confadmin->conftag, mergedConfTags);
        }
    }
    return mergedConfTags;
}

/*************************************
 * MERGE EXPORTS *
 * **********************************/
static const void *getDataExports(const redNodeT *node, int *count) {
    *count = node->config->exports_count;
    return (const void *)node->config->exports;
}

static int setDataExports(const redNodeT *node, const void *destdata, const void *srcdata, const char *hashkey, const char *warn, int expand) {
    redConfExportPathT *export_expand = (redConfExportPathT*)destdata;
    redConfExportPathT *export = (redConfExportPathT*)srcdata;

    export_expand->mode = export->mode;
    export_expand->mount = expandAlloc(node, export->mount, expand);
    export_expand->info = strdup(node->redpath);
    if(warn) export_expand->warn = strdup(warn);

    //if not expand: free hashkey
    if(!expand) {
        free((char *)hashkey);
        export_expand->path = strdup(export->path);
    } else {
        export_expand->path = hashkey;
    }

    return 0;
}

static const char* getKeyExports(const redNodeT *node, const void *srcdata, int *ignore) {
    redConfExportPathT *export = (redConfExportPathT*)srcdata;

    /* if not leaf: ignore mode: */
    if(node->childs->child && export->mode == RED_EXPORT_PRIVATE) {
        *ignore = 1;
        goto OnExit;
    }

    if(!export->path)
        goto OnExit;

    return RedNodeStringExpand (node, NULL, export->path, NULL, NULL);

OnExit:
    return NULL;
}

static int mergeExports(const redNodeT *rootnode, redNodeT *expandNode, int expand) {
    int mergecount = -1;

    redHashCbsT hashcbs = {
        .getdata = getDataExports,
        .getkey = getKeyExports,
        .setdata = setDataExports,
    };

    expandNode->config->exports = (redConfExportPathT*)mergeData(rootnode, sizeof(redConfExportPathT), &mergecount, &hashcbs, 1, expand);
    expandNode->config->exports_count = mergecount;

    return 0;
}

/*************************************
 * MERGE CONFVARS *
 * **********************************/

static const void *getDataConfVars(const redNodeT *node, int *count) {
    *count = node->config->confvar_count;
    return (const void *)node->config->confvar;
}

static int setDataConfVars(const redNodeT *node, const void *destdata, const void *srcdata, const char *hashkey, const char *warn, int expand) {
    redConfVarT *confvar_expand = (redConfVarT*)destdata;
    redConfVarT *confvar = (redConfVarT*)srcdata;

    confvar_expand->key = hashkey;
    confvar_expand->mode = confvar->mode;
    if(confvar->value) confvar_expand->value = expandAlloc(node, confvar->value, expand);
    confvar_expand->info = strdup(node->redpath);
    if(warn) confvar_expand->warn = strdup(warn);

    return 0;
}

static const char* getKeyConfVars(const redNodeT *node, const void *srcdata, int *ignore) {
    redConfVarT *confvar = (redConfVarT*)srcdata;

    return confvar->key ? strdup(confvar->key) : NULL;
}

static int mergeConfVars(const redNodeT *rootnode, redNodeT *expandNode, int expand) {
    int mergecount = -1;

    redHashCbsT hashcbs = {
        .getdata = getDataConfVars,
        .getkey = getKeyConfVars,
        .setdata = setDataConfVars,
    };

    expandNode->config->confvar = (redConfVarT*)mergeData(rootnode, sizeof(redConfVarT), &mergecount, &hashcbs, 1, expand);
    expandNode->config->confvar_count = mergecount;

    return 0;
}

/*************************************
 * MAIN MERGE *
 * **********************************/
redNodeT *mergeNode(const redNodeT *leaf, const redNodeT* rootNode, int expand) {

    if (!rootNode) {
        for(rootNode = leaf; rootNode->ancestor; rootNode = rootNode->ancestor);
    }

    redNodeT *mergedNode = malloc(sizeof(redNodeT));
    memset(mergedNode, 0, sizeof(redNodeT));
    mergedNode->redpath = strdup(leaf->redpath);

    //config
    mergedNode->config =  malloc(sizeof(redConfigT));
    memset(mergedNode->config, 0, sizeof(redConfigT));

    //acl
    if(leaf->config->acl) {
        mergedNode->config->acl = malloc(sizeof(redConfAclT));
        mergedNode->config->acl->gid_t = leaf->config->acl->gid_t;
        mergedNode->config->acl->uid = leaf->config->acl->uid;
        mergedNode->config->acl->umask = leaf->config->acl->umask;
    }

    //headers
    mergedNode->config->headers = malloc(sizeof(redConfHeaderT));
    mergedNode->config->headers->alias = strdup(leaf->config->headers->alias);
    mergedNode->config->headers->name = strdup(leaf->config->headers->name);
    mergedNode->config->headers->info = strdup(leaf->config->headers->info);

    //conftar
    mergedNode->config->conftag = mergedConftags(rootNode, 0);
    mergedNode->config->conftag->cachedir = expandAlloc(mergedNode, mergedNode->config->conftag->cachedir, expand);
    mergedNode->config->conftag->hostname = expandAlloc(mergedNode, mergedNode->config->conftag->hostname, expand);
    mergedNode->config->conftag->chdir = expandAlloc(mergedNode, mergedNode->config->conftag->chdir, expand);
    mergedNode->config->conftag->umask = expandAlloc(mergedNode, mergedNode->config->conftag->umask, expand);

    //confvars
    if(mergeConfVars(rootNode, mergedNode, expand)) {
        RedLog(REDLOG_ERROR, "Issue mergeConfVars for %s", mergedNode->redpath);
    }


    //exports
    if(mergeExports(rootNode, mergedNode, expand)) {
        RedLog(REDLOG_ERROR, "Issue mergeExports for %s", mergedNode->redpath);
    }

    //relocations ignored

    return mergedNode;
}

const char *getMergeConfig(const char *redpath, size_t *len, int expand) {
    char *output = NULL;
    redNodeT *redleaf = RedNodesScan(redpath, 0);
    redNodeT *mergedNode = mergeNode(redleaf, NULL, expand);
    if(RedGetConfig(&output, len, mergedNode->config)) {
        RedLog(REDLOG_ERROR, "Issue getting yaml string merged config");
    }

    freeRedLeaf(mergedNode);
    freeRedLeaf(redleaf);
    return output;
}