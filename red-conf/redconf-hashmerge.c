/*
* Copyright (C) 2022 "IoT.bzh"
* Author Clément Bénier <clement.benier@iot.bzh>
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

#include "redconf-hashmerge.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "redconf-log.h"
#include "redconf-hash.h"
#include "redconf-expand.h"


static char *hashwarn(int alloc, const char *format, ...) {
    char *res = NULL;
    const size_t warnlen = 256;
    char warn[warnlen];

    va_list args;
    va_start(args, format);
    vsnprintf(warn, warnlen, format, args);
    va_end(args);

    RedLog(REDLOG_WARNING, "%s", warn);

    if(alloc)
        res = strdup(warn);
    return res;
}

// command_warn: alloc return strdup else just print warning
static char *common_hashwarn(int alloc, const char *key, const char *redpath) {
    return hashwarn(alloc, "key=%s is overload by %s", key, redpath);
}


/*************************************
 * MERGE EXPORTS *
 * **********************************/
static const void *getDataExports(const redNodeT *node, int *count) {
    *count = node->config->exports_count;
    if (*count == 0)
        RedLog(REDLOG_DEBUG, "no export values for %s", node->redpath);

    return (const void *)node->config->exports;
}

static int setDataExports(const void *destdata, redHashT *srchash, redHashT *overload, int expand, int duplicate) {
    char *warn = NULL;

    redConfExportPathT *export_expand = (redConfExportPathT*)destdata;
    redConfExportPathT *export = (redConfExportPathT*)srchash->value;

    if(overload) {
        warn = common_hashwarn(duplicate, srchash->key, srchash->node->redpath);
        if (!duplicate)
            goto ExitNoDuplicate;
    }

    export_expand->mode = export->mode;
    export_expand->mount = expandAlloc(srchash->node, export->mount, expand);
    export_expand->info = strdup(srchash->node->redpath);
    export_expand->warn = warn;

    //if not expand: free hashkey
    if(!expand) {
        free((char *)srchash->key);
        if(export->path)
            export_expand->path = strdup(export->path);
    } else {
        export_expand->path = srchash->key;
    }

    return 0;
ExitNoDuplicate:
    return 1;
}

static const char* getKeyExports(const redNodeT *node, const void *srcdata, int *ignore) {
    redConfExportPathT *export = (redConfExportPathT*)srcdata;

    /* if not leaf: ignore mode: */
    if(node->first_child && export->mode == RED_EXPORT_PRIVATE) {
        *ignore = 1;
        goto OnExit;
    }

    if(!export->path)
        goto OnExit;

    return RedNodeStringExpand (node, export->path);

OnExit:
    return NULL;
}

int mergeExports(const redNodeT *rootnode, redNodeT *expandNode, int expand, int duplicate) {
    int mergecount = -1;

    redHashCbsT hashcbs = {
        .getdata = getDataExports,
        .getkey = getKeyExports,
        .setdata = setDataExports,
    };

    expandNode->config->exports = (redConfExportPathT*)mergeData(rootnode, sizeof(redConfExportPathT), &mergecount, &hashcbs, duplicate, expand);
    expandNode->config->exports_count = mergecount;

    return 0;
}

/*************************************
 * MERGE CONFVARS *
 * **********************************/

static const void *getDataConfVars(const redNodeT *node, int *count) {
    *count = node->config->confvar_count;
    if (*count == 0)
        RedLog(REDLOG_DEBUG, "no confvars values for %s", node->redpath);

    return (const void *)node->config->confvar;
}

static int setDataConfVars(const void *destdata, redHashT *srchash, redHashT *overload, int expand, int duplicate) {
    char *warn = NULL;

    redConfVarT *confvar_expand = (redConfVarT*)destdata;
    redConfVarT *confvar = (redConfVarT*)srchash->value;

    if(overload) {
        warn = common_hashwarn(duplicate, srchash->key, srchash->node->redpath);
        if (!duplicate)
            goto ExitNoDuplicate;
    }

    confvar_expand->key = srchash->key;
    confvar_expand->mode = confvar->mode;
    if(confvar->value) confvar_expand->value = expandAlloc(srchash->node, confvar->value, expand);
    confvar_expand->info = strdup(srchash->node->redpath);
    confvar_expand->warn = warn;

    return 0;
ExitNoDuplicate:
    return 1;
}

static const char* getKeyConfVars(const redNodeT *node, const void *srcdata, int *ignore) {
    (void)node;
    (void)ignore;

    redConfVarT *confvar = (redConfVarT*)srcdata;

    return confvar->key ? strdup(confvar->key) : NULL;
}

int mergeConfVars(const redNodeT *rootnode, redNodeT *expandNode, int expand, int duplicate) {
    int mergecount = -1;

    redHashCbsT hashcbs = {
        .getdata = getDataConfVars,
        .getkey = getKeyConfVars,
        .setdata = setDataConfVars,
    };

    expandNode->config->confvar = (redConfVarT*)mergeData(rootnode, sizeof(redConfVarT), &mergecount, &hashcbs, duplicate, expand);
    expandNode->config->confvar_count = mergecount;

    return 0;
}

/*************************************
 * MERGE CAPABILITIES *
 * **********************************/

static const void *getDataCapabilities(const redNodeT *node, int *count) {
    if (!node->config->conftag || !node->config->conftag->capabilities_count) {
        RedLog(REDLOG_DEBUG, "no confvars values for %s", node->redpath);
        return NULL;
    }
    *count = node->config->conftag->capabilities_count;
    return (const void *)node->config->conftag->capabilities;
}

static int setDataCapabilities(const void *destdata, redHashT *srchash, redHashT *overload, int expand, int duplicate) {

    (void)expand;

    char *warn = NULL;

    redConfCapT *capdest = (redConfCapT*)destdata;
    redConfCapT *capsrc = (redConfCapT*)srchash->value;

    if(overload) {
        redConfCapT *capoverload = (redConfCapT*)overload->value;
        if (capoverload->add == capsrc->add) {
            warn = common_hashwarn(duplicate, srchash->key, overload->node->redpath);
        } else if(!capoverload->add) {
                warn = hashwarn(duplicate, "capability=%s is dropped by parent=%s", capsrc->cap, overload->node->redpath);
        }
        if(!duplicate)
            goto ExitNoDuplicate;

    }

    capdest->cap = strdup(capsrc->cap);
    capdest->add = capsrc->add;
    capdest->info = strdup(srchash->node->redpath);
    capdest->warn = warn;

    return 0;
ExitNoDuplicate:
    return 1;
}

static const char* getKeyCapabilities(const redNodeT *node, const void *srcdata, int *ignore) {
    (void)node;
    (void)ignore;
    redConfCapT *cap = (redConfCapT*)srcdata;
    return cap->cap;
}

int mergeCapabilities(const redNodeT *rootnode, redConfTagT *mergedconftag, int duplicate) {
    int count;

    redHashCbsT hashcbs = {
        .getdata = getDataCapabilities,
        .getkey = getKeyCapabilities,
        .setdata = setDataCapabilities,
    };

    mergedconftag->capabilities = (redConfCapT*)mergeData(rootnode, sizeof(redConfCapT), &count, &hashcbs, duplicate, 0);
    mergedconftag->capabilities_count = count;

    return 0;
}