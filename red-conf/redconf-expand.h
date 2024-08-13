/*
* Copyright (C) 2020 "IoT.bzh"
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

#ifndef _RED_CONF_EXPAND_INCLUDE_
#define _RED_CONF_EXPAND_INCLUDE_

#include "redconf-defaults.h"
#include "redconf-node.h"

const char * RedNodeStringExpand (const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, const char*prefix, const char*trailler);
const char *RedGetDefaultExpand(redNodeT *node, RedConfDefaultsT *defaults, const char* inputS);
int RedConfGetEnvKey (const redNodeT *node, RedConfDefaultsT *defaults, int *idxIn, const char *inputS, int *idxOut, char *outputS, int maxlen);
int RedConfAppendEnvKey (const redNodeT *node, char *outputS, int *idxOut, int maxlen, const char *inputS,  RedConfDefaultsT *defaults, const char* prefix, const char *trailler);
const char *expandAlloc(const redNodeT *node, const char *input, int expand);

#endif