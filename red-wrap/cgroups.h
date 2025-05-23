/*
* Copyright (C) 2021-2025 IoT.bzh Company
* Author: Fulup Ar Foll <fulup@iot.bzh>
* Author: Clément Bénier <clement.benier@iot.bzh>
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

#ifndef _CGROUPS_H_
#define _CGROUPS_H_

#include "redconf-node.h"

extern int set_cgroups(const redNodeT *node, const char *root);

#endif //_CGROUPS_H_
