/*
* Copyright (C) 2020-2025 IoT.bzh Company
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

#include <stdio.h>
#include <unistd.h>

#define COLOR_REDPRINT "\033[0;31m"
#define COLOR_RESET "\033[0m"

static RedLogLevelE RedLogLevel = REDLOG_WARNING;

void SetLogLevel(RedLogLevelE level) {
    RedLogLevel = level < REDLOG_ERROR ? REDLOG_ERROR
                : level > REDLOG_TRACE ? REDLOG_TRACE : level;
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

static const char *debugPrefixFormat[] = {
    "[EMERGENCY]: ",
    "[ALERT]: ",
    "[CRITICAL]: ",
    "[ERROR]: ",
    "[WARNING]: ",
    "[NOTICE]: ",
    "[INFO]: ",
    "[DEBUG]: ",
    "[TRACE]: "
};

RedLogLevelE GetLogLevel() {
    return RedLogLevel;
}

// redlib and redconf use RedLog to display error messages
void redlog(RedLogLevelE level, const char *file, int line, const char *format, ...) {
    if (level < REDLOG_EMERGENCY)
        level = REDLOG_EMERGENCY;
    else if (level > REDLOG_TRACE)
        level = REDLOG_TRACE;
    if (level > RedLogLevel)
        return;
    va_list args;
    va_start(args, format);

    if (redLogRegisteredCb) {
        (*redLogRegisteredCb) (level, format, args);
    } else {
        const char *setcolo = "", *unsetcolo = "";
        if (isatty(fileno(stderr)) && level <= REDLOG_WARNING) {
            setcolo = COLOR_REDPRINT;
            unsetcolo = COLOR_RESET;
        }
        fprintf(stderr, "%s%s ", setcolo, debugPrefixFormat[level]);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\t[%s:%d]", file, line);
        fprintf(stderr, "%s\n", unsetcolo);
    }
    va_end(args);
}

