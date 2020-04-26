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
* Documentation: https://pyyaml.org/wiki/LibYAML http://codepad.org/W7StVSkV
*/

#include <fcntl.h>
#include <rpm/rpmmacro.h>

#include "redrpm.h"

// compute RPM vardir from node/config/default
const char *RedGetRpmVarDir (redConfigT *config, redNodeT *node, const char*prefix, const char*trailler) {
    const char *rpmdir;

    // get rpmdb path from node config or use default
    rpmdir = node->config->conftag->rpmdir;
    if (!rpmdir) rpmdir = config->conftag->rpmdir;
    if (!rpmdir) rpmdir = REDNODE_VARDIR;

    // expand dir with system environement variables
    rpmdir= RedNodeStringExpand (node, nodeConfigDefaults, rpmdir, prefix, trailler);
    return rpmdir;
}

int RedRegisterFamilyDb (rpmts ts, redConfigT *config, redNodeT *redLeafNode) { 
    const char *fullpath;
    const char *rpmdir;
    int error;

    // Make sure last node was created in the future !!!
    unsigned long epocms = RedUtcGetTimeMs ();

    // Scan redpath family nodes from terminal leaf to root node
    for (redNodeT *node=redLeafNode; node != NULL; node=node->ancestor) {
        redConfigT *configN = node->config;
        redStatusT *statusN = node->status;

        // make sure node is not disabled
        if (statusN->state !=  RED_STATUS_ENABLE) {
            rpmlog(REDLOG_ERROR, "*** Node [%s] is DISABLED [check/update node] nodepath=%s", configN->headers->alias, node->redpath);
            goto OnErrorExit;
        }

        // Expand node config to fit within redpath
        fullpath = RedNodeStringExpand (node, nodeConfigDefaults, configN->conftag->rpmdir, NULL, NULL);

        //printf ("---- RedRegisterFamilyDb dbpath=%s ---\n", fullpath);
        rpmPushMacro(NULL, "_dbpath", NULL, fullpath, 0);
        error = rpmtsOpenDB(ts, O_RDONLY);
        if (error) {
            rpmlog(REDLOG_ERROR, "*** Fail to open RPMdb node='%s' path='%s'", configN->headers->alias, statusN->realpath);
            goto OnErrorExit;
        }
    }

    return 0;

OnErrorExit:
    return 1;    
}
