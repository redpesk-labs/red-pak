/*
* Copyright (C) 2022-2025 IoT.bzh Company
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

#ifndef _REDCONFCMD_GLOBCONF_INCLUDE_
#define _REDCONFCMD_GLOBCONF_INCLUDE_

typedef struct {
    const char* cmd;
    int sub_argc;
    char **sub_argv;
    int yaml;
    int logyaml;
} rGlobalConfigT;

#endif