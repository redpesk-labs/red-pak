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

/* merge the source string to the destination
 * returns 0 on success or 1 on error
 */
static inline int merge_string(const char *source, const char **destination, int duplicate) {
    if (source != NULL) {
        if (!duplicate)
            *destination = source;
        else {
            free((void*)*destination);
            *destination = strdup(source);
            if (*destination == NULL)
                return 1;
        }
    }
    return 0;
}

/* merge the source sharing opt flag to the destination */
static inline void merge_optflag_sharing(redConfOptFlagE source, redConfOptFlagE *destination) {
    if (*destination != RED_CONF_OPT_DISABLED)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void merge_optflag_overwrite(redConfOptFlagE source, redConfOptFlagE *destination) {
    if (source != RED_CONF_OPT_UNSET)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void merge_int_not_null(int source, int *destination) {
    if (source)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void merge_uint_not_null(unsigned source, unsigned *destination) {
    if (source)
        *destination = source;
}

// Only merge tags that should overloaded
static int RedConfCopyConfTags(const redConfTagT *source, redConfTagT *destination, int duplicate) {
    if(destination == NULL) {
        RedLog(REDLOG_ERROR, "destination is NULL", source, destination);
        return 1;
    }

    if(source == NULL)
        return 0;

    merge_optflag_sharing(source->share.all, &destination->share.all);
    merge_optflag_sharing(source->share.user, &destination->share.user);
    merge_optflag_sharing(source->share.ipc, &destination->share.ipc);
    merge_optflag_sharing(source->share.cgroup, &destination->share.cgroup);
    merge_optflag_sharing(source->share.pid, &destination->share.pid);
    merge_optflag_sharing(source->share.net, &destination->share.net);
    merge_optflag_sharing(source->share.time, &destination->share.time);

    merge_optflag_overwrite(source->diewithparent, &destination->diewithparent);
    merge_optflag_overwrite(source->newsession, &destination->newsession);

    merge_int_not_null(source->verbose, &destination->verbose);
    merge_uint_not_null(source->maprootuser, &destination->maprootuser);

    return merge_string(source->cachedir, &destination->cachedir, duplicate)
        || merge_string(source->hostname, &destination->hostname, duplicate)
        || merge_string(source->chdir, &destination->chdir, duplicate)
        || merge_string(source->umask, &destination->umask, duplicate)
        || merge_string(source->cgrouproot, &destination->cgrouproot, duplicate);
}

int mergeSpecialConfVar(const redNodeT *node, dataNodeT *dataNode) {
    int error = 0;

    if (node->config->conftag.ldpath)
        // node looks good extract path/ldpath before adding red-wrap cli program+arguments
        error= RedConfAppendExpandedPath(node, dataNode->ldpathString, &dataNode->ldpathIdx, RED_MAXPATHLEN, node->config->conftag.ldpath);

    if (!error && node->config->conftag.path)
        error= RedConfAppendExpandedPath(node, dataNode->pathString, &dataNode->pathIdx, RED_MAXPATHLEN, node->config->conftag.path);

    return error;
}

// put in conftag the merge of node hierarchy
int mergeConfTag(const redNodeT *node, redConfTagT *conftag, int duplicate) {
    const redNodeT *iter;

    for (iter=node; iter != NULL; iter=iter->first_child) {
        (void) RedConfCopyConfTags(&iter->config->conftag, conftag, duplicate);
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

        (void) RedConfCopyConfTags(&iter->confadmin->conftag, conftag, duplicate);
    }
    return 0;
}

//get mergedConftags from hierarchy
redConfTagT *mergedConftags(const redNodeT *node) {
    redConfTagT *mergedConfTags = calloc(1, sizeof(redConfTagT));
    if (mergedConfTags != NULL) {
        int rc = mergeConfTag(node, mergedConfTags, 0 /* not duplicating */);
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

    //headers
    mergedNode->config->headers.alias = strdup(leaf->config->headers.alias);
    mergedNode->config->headers.name = strdup(leaf->config->headers.name);
    mergedNode->config->headers.info = strdup(leaf->config->headers.info);

    //conftar
    mergeConfTag(rootNode, &mergedNode->config->conftag, 0);
    mergedNode->config->conftag.cachedir = expandAlloc(mergedNode, mergedNode->config->conftag.cachedir, expand);
    mergedNode->config->conftag.hostname = expandAlloc(mergedNode, mergedNode->config->conftag.hostname, expand);
    mergedNode->config->conftag.chdir = expandAlloc(mergedNode, mergedNode->config->conftag.chdir, expand);
    mergedNode->config->conftag.umask = expandAlloc(mergedNode, mergedNode->config->conftag.umask, expand);
    mergedNode->config->conftag.cgrouproot = expandAlloc(mergedNode, mergedNode->config->conftag.cgrouproot, expand);

    //capabilities
    if(mergeCapabilities(rootNode, &mergedNode->config->conftag, duplicate)) {
        RedLog(REDLOG_ERROR, "Issue mergeCapabilities for %s", mergedNode->redpath);
    }
    mergedNode->config->conftag.umask = expandAlloc(mergedNode, mergedNode->config->conftag.umask, expand);

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
