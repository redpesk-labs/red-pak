/*
 Copyright (C) 2016-2025 IoT.bzh Company

 Author: Fulup Ar Foll <fulup@iot.bzh>

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


#ifndef _REDWRAP_CONF_INCLUDE_
#define _REDWRAP_CONF_INCLUDE_

typedef struct {
    const char *redpath;
    const char *bwrap;
    const char *adminpath;
    const char *setuser;
    const char *setgroup;
    int index;
    int verbose;
    int strict;
    int unsafe;
    int verboseopts;
    int dump;
    int isadmin;
} rWrapConfigT;

extern rWrapConfigT *RwrapParseArgs(int argc, char *argv[], const char *usage);

#endif
