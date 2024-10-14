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

#include "redconf-merge.h"

#include <stdlib.h>
#include <string.h>

#include "lookup3.h"
#include "redconf-log.h"
#include "redconf-defaults.h"
#include "redconf-expand.h"
#include "redconf-node.h"
#include "redconf-utils.h"
#include "redconf-hashmerge.h"

// Only merge tags that should overloaded
static int RedConfCopyConfTags(redConfTagT *source, redConfTagT *destination) {
    if(!destination) {
        RedLog(REDLOG_ERROR, "destination is NULL", source, destination);
        return 1;
    }

    if(source) {
        if (source->cachedir)
            destination->cachedir = source->cachedir;
        if (source->hostname)
            destination->hostname = source->hostname;
        if (source->chdir)
            destination->chdir = source->chdir;
        if (source->umask)
            destination->umask = source->umask;
        if (source->verbose)
            destination->verbose = source->verbose;
        if (source->diewithparent)
            destination->diewithparent = source->diewithparent;
        if (source->newsession)
            destination->newsession = source->newsession;
        if (source->umask)
            destination->umask = source->umask;
        if (source->maprootuser)
            destination->maprootuser = source->maprootuser;
        if (source->cgrouproot)
            destination->cgrouproot = source->cgrouproot;

        if(destination->share_all != RED_CONF_OPT_DISABLED)
            destination->share_all = source->share_all;
        if(destination->share_user != RED_CONF_OPT_DISABLED)
            destination->share_user = source->share_user;
        if(destination->share_ipc != RED_CONF_OPT_DISABLED)
            destination->share_ipc = source->share_ipc;
        if(destination->share_cgroup != RED_CONF_OPT_DISABLED)
            destination->share_cgroup =source->share_cgroup;
        if(destination->share_pid != RED_CONF_OPT_DISABLED)
            destination->share_pid = source->share_pid;
        if(destination->share_net != RED_CONF_OPT_DISABLED)
            destination->share_net = source->share_net;
        if(destination->share_time != RED_CONF_OPT_DISABLED)
            destination->share_time = source->share_time;
    }
    return 0;
}

int mergeSpecialConfVar(const redNodeT *node, dataNodeT *dataNode) {
    int error = 0;

    if (node->config->conftag && node->config->conftag->ldpath)
        // node looks good extract path/ldpath before adding red-wrap cli program+arguments
        error= RedConfAppendPath(node, dataNode->ldpathString, &dataNode->ldpathIdx, RED_MAXPATHLEN, node->config->conftag->ldpath);

    if (!error && node->config->conftag && node->config->conftag->path)
        error= RedConfAppendPath(node, dataNode->pathString, &dataNode->pathIdx, RED_MAXPATHLEN, node->config->conftag->path);

    return error;
}

// put in conftag the merge of node hierarchy
static int mergeConfTag(const redNodeT *node, redConfTagT *conftag) {
    const redNodeT *iter;

    for (iter=node; iter != NULL; iter=iter->first_child) {
        (void) RedConfCopyConfTags(iter->config->conftag, conftag);
        if(!iter->parent) { //system_node
            // update process default umask
            RedSetUmask (conftag ? conftag->umask : NULL);
        }
    }

    // assume admin overload everything exepted node specific
    for (iter=node; iter != NULL; iter=iter->parent) {
        if (!iter->confadmin) {
            RedLog(REDLOG_DEBUG, "no admin config for %s", iter->redpath);
            continue;
        }

        if (!iter->confadmin->conftag) {
            RedLog(REDLOG_INFO, "no admin conftag for %s", iter->redpath);
            continue;
        }
        (void) RedConfCopyConfTags(iter->confadmin->conftag, conftag);
    }
    return 0;
}

//get mergedConftags from hierarchy
redConfTagT *mergedConftags(const redNodeT *node) {
    redConfTagT *mergedConfTags = calloc(1, sizeof(redConfTagT));
    if (mergedConfTags != NULL) {
        int rc = mergeConfTag(node, mergedConfTags);
        if (rc < 0) {
            free(mergedConfTags);
            mergedConfTags = NULL;
        }
    }
    return mergedConfTags;
}

/*************************************
 * MAIN MERGE *
 * **********************************/
redNodeT *mergeNode(const redNodeT *leaf, const redNodeT* rootNode, int expand, int duplicate) {

    if (!rootNode) {
        for(rootNode = leaf; rootNode->parent; rootNode = rootNode->parent);
    }

    redNodeT *mergedNode = calloc(1, sizeof(redNodeT));
    mergedNode->redpath = strdup(leaf->redpath);

    //config
    mergedNode->config =  calloc(1, sizeof(redConfigT));
    mergedNode->config->headers = calloc(1, sizeof(redConfHeaderT));

    //headers
    mergedNode->config->headers->alias = strdup(leaf->config->headers->alias);
    mergedNode->config->headers->name = strdup(leaf->config->headers->name);
    mergedNode->config->headers->info = strdup(leaf->config->headers->info);

    //conftar
    mergedNode->config->conftag = mergedConftags(rootNode);
    mergedNode->config->conftag->cachedir = expandAlloc(mergedNode, mergedNode->config->conftag->cachedir, expand);
    mergedNode->config->conftag->hostname = expandAlloc(mergedNode, mergedNode->config->conftag->hostname, expand);
    mergedNode->config->conftag->chdir = expandAlloc(mergedNode, mergedNode->config->conftag->chdir, expand);
    mergedNode->config->conftag->umask = expandAlloc(mergedNode, mergedNode->config->conftag->umask, expand);
    mergedNode->config->conftag->cgrouproot = expandAlloc(mergedNode, mergedNode->config->conftag->cgrouproot, expand);

    //capabilities
    if(mergeCapabilities(rootNode, mergedNode->config->conftag, duplicate)) {
        RedLog(REDLOG_ERROR, "Issue mergeCapabilities for %s", mergedNode->redpath);
    }
    mergedNode->config->conftag->umask = expandAlloc(mergedNode, mergedNode->config->conftag->umask, expand);

    //confvars
    if(mergeConfVars(rootNode, mergedNode, expand, duplicate)) {
        RedLog(REDLOG_ERROR, "Issue mergeConfVars for %s", mergedNode->redpath);
    }

    //exports
    if(mergeExports(rootNode, mergedNode, expand, duplicate)) {
        RedLog(REDLOG_ERROR, "Issue mergeExports for %s", mergedNode->redpath);
    }

    return mergedNode;
}

const char *getMergeConfig(const char *redpath, size_t *len, int expand) {
    char *output = NULL;
    redNodeT *redleaf = RedNodesScan(redpath, 0, 0);
    if (!redleaf)
        goto OnExit;

    redNodeT *mergedNode = mergeNode(redleaf, NULL, expand, 1);
    if (!mergedNode)
        goto OnExitFreeLeaf;

    if(RedGetConfigYAML(&output, len, mergedNode->config)) {
        RedLog(REDLOG_ERROR, "Issue getting yaml string merged config");
    }

    freeRedLeaf(mergedNode);
OnExitFreeLeaf:
    freeRedLeaf(redleaf);
OnExit:
    return output;
}

const char *getConfig(const char *redpath, size_t *len) {
    char *output = NULL;
    redNodeT *redleaf = RedNodesScan(redpath, 0, 0);
    if (!redleaf)
        goto OnExit;

    if(RedGetConfigYAML(&output, len, redleaf->config)) {
        RedLog(REDLOG_ERROR, "Issue getting yaml string config");
    }

    freeRedLeaf(redleaf);

OnExit:
    return output;
}
