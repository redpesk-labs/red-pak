/*
 * Copyright (C) 2022-2025 IoT.bzh Company
 *
 * Author: Clément Bénier <clement.benier@iot.bzh>
 * Author: Valentin Lefebvre <valentin.lefebvre@iot.bzh>
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
 */

#define _GNU_SOURCE

#include <stdlib.h>

#include "redwrap-conf.h"
#include "redwrap-exec.h"

static const char * redwrap_usage = "usage: redwrap --redpath=... [--verbose] [--strict] [--admin[=.../main-admin.yaml]] -- program args\n";

/**
 * @brief execute redwrap command according arguments
 * 
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @return 0 in success negative otherwise
 */
int main (int argc, char *argv[]) {
    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, redwrap_usage);
    if (!cliarg)
        return EXIT_FAILURE;
    if (redwrapExecBwrap(argv[0], cliarg, argc - cliarg->index, argv + cliarg->index) < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
