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

#include "options.h"

#include <string.h>
#include <stdio.h>

void usageOptions(const rOption *options) {
    printf("\nOptions:\n");
    for (const rOption *opt = options; opt->option.name; opt++) {
        printf("\t-%c, --%s\t\t%s\n", opt->option.val, opt->option.name, opt->desc);
    }
}

/* getLongOpt: copy struct option into struct option array for getopt_long*/
void setLongOptions(const rOption opts[], struct option *longOpts) {
    struct option *tmpLongOpt = longOpts;
    for (rOption *opt = (rOption *)opts; opt->option.name; opt++) {
        memcpy(tmpLongOpt, opt, sizeof(struct option));
        tmpLongOpt++;
    }
    memset(tmpLongOpt, 0, sizeof(struct option));
}
