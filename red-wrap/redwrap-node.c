/*
 * Copyright (C) 2020-2025 - 2022 IoT.bzh Company
 *
 * Author: fulup Ar Foll <fulup@iot.bzh>
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

#include "redwrap-node.h"

#include "redconf-log.h"
#include "redconf-utils.h"

int RwrapValidateNode (redNodeT *node, int unsafe) {

    redConfigT *configN = node->config;
    redStatusT *statusN = node->status;

    // make sure node is not disabled
    if (statusN->state !=  RED_STATUS_ENABLE) {
        RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] is DISABLED [check/update node] nodepath=%s", configN->headers.alias, node->redpath);
        return 1;
    }

    // if not in force mode do further sanity check
    if (!(unsafe || configN->conftag.unsafe)) {

        // check node was not moved from one family to an other
        if (!RedConfIsSameFile(node->redpath, statusN->realpath)) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] was moved [require 'dnf red-update' or --force] nodepath=%s", configN->headers.alias, statusN->realpath);
            return 1;
        }
    }

    return 0;
}
