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


#ifndef _REDWRAP_INCLUDE_
#define _REDWRAP_INCLUDE_

#include "redconf.h"
#include <sys/types.h>
#include <sys/stat.h>


#ifndef BWRAP_CMD_PATH
#define BWRAP_CMD_PATH "/usr/bin/bwrap"
#endif

#ifndef BWRAP_MAXVAR_LEN
#define BWRAP_MAXVAR_LEN 1024
#endif

typedef struct {
    const char*redpath;
    const char*cnfpath;
    const char*bwrap;
    const char*adminpath;
    int index;
    int verbose;
    int forcemod;
    int unsafe;
    int verboseopts;
} rWrapConfigT;

rWrapConfigT *RwrapParseArgs(int argc, char *argv[], const char *usage);
int RwrapParseNode (redNodeT *node, rWrapConfigT *cliargs,  int lastleaf, const char *argval[], int *argcount);
int RwrapParseConfig (redNodeT *node, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount);
int RedSetCapabilities(const redNodeT *rootnode, redConfTagT *mergedConfTags, const char *argval[], int *argcount);

#endif
