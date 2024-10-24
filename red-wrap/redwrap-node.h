/*
 Copyright (C) 2016, 2017 "IoT.bzh"

 author: Fulup Ar Foll <fulup@iot.bzh>

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


#ifndef _REDWRAP_NODE_INCLUDE_
#define _REDWRAP_NODE_INCLUDE_

#include "redwrap-conf.h"
#include "redconf-node.h"

extern int RwrapValidateNode (redNodeT *node, int unsafe);
extern int RwrapParseNode (redNodeT *node, rWrapConfigT *cliargs,  int lastleaf, const char *argval[], int *argcount);
extern int RwrapParseConfig (redNodeT *node, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount);
extern int RedSetCapabilities(const redNodeT *rootnode, redConfTagT *mergedConfTags, const char *argval[], int *argcount);

#endif
