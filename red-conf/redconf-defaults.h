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

#ifndef _RED_DEFAULTS_INCLUDE_
#define _RED_DEFAULTS_INCLUDE_


// set some reasonnable defaults
#ifndef RED_MAXPATHLEN
#define RED_MAXPATHLEN 512
#endif

#ifndef RED_CONFIG_WARNING_DEFAULT
#define RED_CONFIG_WARNING_DEFAULT 1
#endif

#ifndef redpak_MAIN
#define redpak_MAIN "/etc/redpak/main.conf"
#endif

// Following file/dir are automatically prefixed by $REDPATH at runtime
#ifndef REDNODE_CONF
#define REDNODE_CONF "etc/redpak.conf"
#endif

#ifndef REDNODE_STATUS
#define REDNODE_STATUS ".rednode.status"
#endif

#ifndef REDNODE_VARDIR
#define REDNODE_VARDIR "var/rpm/lib"
#endif

#ifndef REDNODE_LOCK
#define REDNODE_LOCK ".rpm.lock"
#endif

#ifndef REDSTATUS_INFO
#define REDSTATUS_INFO "Last update by $LOGNAME($HOSTNAME) the $TODAY"
#endif


typedef enum  {
    REDLOG_EMERGENCY = 0,
    REDLOG_ALERT = 1,
    REDLOG_CRITICAL = 2,
    REDLOG_ERROR = 3,
    REDLOG_WARNING = 4,
    REDLOG_NOTICE = 5,
    REDLOG_INFO = 6,
    REDLOG_DEBUG = 7
} RedLogLevelE;


typedef enum {
    REDDEFLT_STR,
    REDDEFLT_CB
} RedConfDefaultE;

typedef char*(*RedGetDefaultCbT)(const char *label, void *ctx, void*handle);

typedef struct {
    const char *label;
    RedGetDefaultCbT callback;
    void *handle;
} RedConfDefaultsT;

extern RedConfDefaultsT nodeConfigDefaults[];

#endif