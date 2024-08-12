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

#define _GNU_SOURCE

#include "redconf-log.h"

#include <stdarg.h>
#include <stdio.h>

#define COLOR_REDPRINT "\033[0;31m"
#define COLOR_RESET "\033[0m"

static RedLogLevelE RedLogLevel = REDLOG_INFO;

void SetLogLevel(RedLogLevelE level) {
    RedLogLevel = level < REDLOG_ERROR ? REDLOG_ERROR : level;
}

// Allow log function to be mapped on RedLog, syslog, ...
static RedLogCbT *redLogRegisteredCb;

void RedLogRegister (RedLogCbT *redlogcb) {

    redLogRegisteredCb= redlogcb;
}

typedef struct {
    const char *str;
    int val;

} debugStrValT;

static const debugStrValT debugPrefixFormat[] = {
    {COLOR_REDPRINT"[EMERGENCY]: ", REDLOG_EMERGENCY},
    {COLOR_REDPRINT"[ALERT]: ", REDLOG_ALERT},
    {COLOR_REDPRINT"[CRITICAL]: ", REDLOG_CRITICAL},
    {COLOR_REDPRINT"[ERROR]: ", REDLOG_ERROR},
    {COLOR_REDPRINT"[WARNING]: ", REDLOG_WARNING},
    {              "[NOTICE]: ", REDLOG_NOTICE},
    {              "[INFO]: ", REDLOG_INFO},
    {              "[DEBUG]: ", REDLOG_DEBUG},
    {              "[TRACE]: ", REDLOG_TRACE},
};

RedLogLevelE GetLogLevel() {
    return RedLogLevel;
}

// redlib and redconf use RedLog to display error messages
void redlog(RedLogLevelE level, const char *file, int line, const char *format, ...) {
    if (level > RedLogLevel)
        return;
    va_list args;
    va_start(args, format);

    if (redLogRegisteredCb) {
        (*redLogRegisteredCb) (level, format, args);
    } else {
        fprintf(stderr, "%s ", debugPrefixFormat[level].str);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\t[%s:%d] ", file, line);
        fprintf(stderr,"\n"COLOR_RESET);
    }
    va_end(args);
}

