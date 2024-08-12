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

#ifndef _RED_CONF_LOG_INCLUDE_
#define _RED_CONF_LOG_INCLUDE_

typedef enum  {
    REDLOG_EMERGENCY = 0,
    REDLOG_ALERT = 1,
    REDLOG_CRITICAL = 2,
    REDLOG_ERROR = 3,
    REDLOG_WARNING = 4,
    REDLOG_NOTICE = 5,
    REDLOG_INFO = 6,
    REDLOG_DEBUG = 7,
    REDLOG_TRACE = 8
} RedLogLevelE;

// callback type definition
typedef void(*RedLogCbT) (RedLogLevelE level, const char *format, ...);
extern void RedLogRegister (RedLogCbT *redlogcb);

/* set the global log level */
extern void SetLogLevel(RedLogLevelE level);

/* checks if log is currently required for the given log level */
extern RedLogLevelE GetLogLevel();

/* for the file and the line, emit a formatted comment if required */
extern void redlog (RedLogLevelE level, const char *file, int line, const char *format, ...);

/* emit a formatted comment if required: RedLog(level, format, ...) */
#define RedLog(REDLOGLEVEL,REDLOGFORMAT,...) \
            do { \
                if (GetLogLevel()>=(REDLOGLEVEL)) \
                    redlog(REDLOGLEVEL,__FILE__,__LINE__,REDLOGFORMAT,##__VA_ARGS__); \
            } while(0)

#endif