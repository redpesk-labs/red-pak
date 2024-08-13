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


#ifndef _RED_CONF_MERGEHASH_INCLUDE_
#define _RED_CONF_MERGEHASH_INCLUDE_

#include "redconf-node.h"

int mergeExports(const redNodeT *rootnode, redNodeT *expandNode, int expand, int duplicate);
int mergeConfVars(const redNodeT *rootnode, redNodeT *expandNode, int expand, int duplicate);
int mergeCapabilities(const redNodeT *rootnode, redConfTagT *mergedconftag, int duplicate);

#endif //_RED_CONF_MERGEHASH_INCLUDE_