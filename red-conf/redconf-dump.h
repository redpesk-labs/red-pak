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


#ifndef _RED_CONF_DUMP_INCLUDE_
#define _RED_CONF_DUMP_INCLUDE_

#include "redconf-schema.h"

void RedDumpConftag(redConfTagT *conftag);

void RedDumpStatusHandle (redStatusT *status);
void RedDumpConfigHandle(redConfigT *config);

int RedDumpConfigPath (const char *filepath, int warning);
int RedDumpStatusPath (const char *filepath, int warning);

void RedDumpFamilyNodeHandle(redNodeT *familyTree, int yaml);
int RedDumpFamilyNodePath (const char* redpath, int yaml, int verbose);

int RedDumpNodePathMerge(const char* redpath, int expand);

int RedDumpStatusPath (const char *filepath, int warning);
int RedDumpConfigPath (const char *filepath, int warning);

int RedDumpTree(const char *redrootpath, int verbose);

#endif