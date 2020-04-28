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

#ifndef _REDLIB_RPMGLUE_INCLUDES_
#define _REDLIB_RPMGLUE_INCLUDES_

#include <rpm/rpmts.h>
#include "redconf.h"

int RedRegisterFamilyDb (rpmts ts, redConfigT *redconf, redNodeT *redFamily);
const char *RedGetRpmVarDir (redConfigT *config, redNodeT *node, const char* prefix, const char* trailler);

#endif
