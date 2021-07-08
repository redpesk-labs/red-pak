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

/*
 Log level is defined by syslog standard:
       KERN_EMERG             0        System is unusable
       KERN_ALERT             1        Action must be taken immediately
       KERN_CRIT              2        Critical conditions
       KERN_ERR               3        Error conditions
       KERN_WARNING           4        Warning conditions
       KERN_NOTICE            5        Normal but significant condition
       KERN_INFO              6        Informational
       KERN_DEBUG             7        Debug-level messages
*/

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
} rWrapConfigT;

rWrapConfigT *RwrapParseArgs(int argc, char *argv[]);
const char* MemFdExecCmd (const char* mount, const char* command);
int RwrapParseNode (redNodeT *node, rWrapConfigT *cliargs,  int lastleaf, const char *argval[], int *argcount);
int RwrapParseConfig (redNodeT *node, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount);
void RedPutEnv (const char*key, const char*value);
mode_t RedSetUmask (redConfTagT *conftag);

#endif
