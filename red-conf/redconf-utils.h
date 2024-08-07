/*
* Copyright (C) 2020 "IoT.bzh"
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

/*
 ** Redpath hierachie

    -'root' path='/' -> alias='coreos'
    -'node' path=${redroot}/xxxxxx -> alias=(template, plateform, ....)
    -'leaf' same as node but without decendants (typically a projet)
    -'family' every nodes in direct line from a leaf to root node.

 ** export visibility

    -'private' visible only with one node/leaf
    -'restricted' visible from all descendants in readonly
    -'public' visible from all descendants in writemode

    Note: when sibling node need to share data, then need to use ancestor (public/restricted directories)
*/

#ifndef _RED_CONF_UTILS_INCLUDE_
#define _RED_CONF_UTILS_INCLUDE_

#define STATIC_STR_CONCAT(S1,S2) S1 S2

#include <stdio.h>

#include "redconf-schema.h"
#include "redconf-defaults.h"

int remove_directories(const char *path);
unsigned long RedUtcGetTimeMs ();

redNodeT *RedNodesScan(const char* redpath, int verbose);
redNodeT *RedNodesDownScan(const char* redroot, int verbose);
int RedUpdateStatus(redNodeT *node, int verbose);

void freeRedLeaf(redNodeT *redleaf);
void freeRedRoot(redNodeT *redroot);

const char * RedNodeStringExpand (const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, const char*prefix, const char*trailler);
const char *RedGetDefaultExpand(redNodeT *node, RedConfDefaultsT *defaults, const char* inputS);
int RedConfGetEnvKey (const redNodeT *node, RedConfDefaultsT *defaults, int *idxIn, const char *inputS, int *idxOut, char *outputS, int maxlen);
int RedConfAppendEnvKey (const redNodeT *node, char *outputS, int *idxOut, int maxlen, const char *inputS,  RedConfDefaultsT *defaults, const char* prefix, const char *trailler);
const char *expandAlloc(const redNodeT *node, const char *input, int expand);

mode_t RedSetUmask (redConfTagT *conftag);

// callback type definition
typedef void(*RedLogCbT) (RedLogLevelE level, const char *format, ...);

//void redlog (RedLogLevelE level, const char *file, const char *line, const char *format, ...);
void redlog (RedLogLevelE level, const char *file, int line, const char *format, ...);

//#define RedLog(REDLOGLEVEL, REDLOGFORMAT, ...) _redlog((REDLOGLEVEL), __FILE__, __LINE__, (REDLOGFORMAT), __VA_ARGS__)
#define RedLog(REDLOGLEVEL,REDLOGFORMAT,...) redlog((REDLOGLEVEL),__FILE__,__LINE__,(REDLOGFORMAT),##__VA_ARGS__)

void SetLogLevel(RedLogLevelE level);
int RedConfGetInod (const char* path);

/* Exec Cmd */
int MemFdExecCmd (const char* mount, const char* command);
int ExecCmd (const char* mount, const char* command, char *res, size_t size);

#endif