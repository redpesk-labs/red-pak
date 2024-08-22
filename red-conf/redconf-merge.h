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

#ifndef _RED_CONF_MERGE_INCLUDE_
#define _RED_CONF_MERGE_INCLUDE_

#include "redconf-node.h"

// ---- Special confvar
typedef struct {
    char *ldpathString;
    unsigned int ldpathIdx;
    char *pathString;
    unsigned int pathIdx;

} dataNodeT;

// mergeSpecfialConfVar
int mergeSpecialConfVar(const redNodeT *node, dataNodeT *dataNode);
//merge conftags from hierarchy
redConfTagT *mergedConftags(const redNodeT *rootnode);
//merge node from hierarchy: if rootNode=NULL rootNode is found
redNodeT *mergeNode(const redNodeT *leaf, const redNodeT* rootNode, int expand, int duplicate);

// yaml string get config
const char *getMergeConfig(const char *redpath, size_t *len, int expand);
const char *getConfig(const char *redpath, size_t *len);

#endif
