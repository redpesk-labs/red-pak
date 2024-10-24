/*
 * Copyright (C) 2020 - 2022 "IoT.bzh"
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "redconf-log.h"
#include "redconf-expand.h"
#include "redconf-utils.h"

/*
 * @brief Check if path exists, and if not and if required, create a directory for the path
 *
 * @param path      Path into the node
 * @param configN   Config of the Node
 * @param create    Create the directory if doesn't exist
 * @return 0 in success negative otherwise
 */
static int RwrapCheckDir(const char *path, redConfigT *configN, int create) {
    struct stat status;
    int err = stat(path, &status);
    if (err && create) {
        err = make_directories(path, 0, strlen(path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH, NULL) != 0;
    }
    if (err) {
        RedLog(REDLOG_WARNING, "*** Node [%s] export expanded path=%s does not exist (error=%s) [use --force]", configN->headers.alias, path, strerror(errno));
        return err;
    }
    return 0;
}

static int RwrapParseSubConfig (redNodeT *node, redConfigT *configN, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount) {

    unsigned idx;
    char buffer[2049];

    // scan export directory
    for (idx=0; idx <configN->exports_count; idx++) {
        redExportFlagE mode= configN->exports[idx].mode;
        const char* mount= configN->exports[idx].mount;
        const char* path=configN->exports[idx].path;
        struct stat status;
        const char * expandpath = NULL;
        char *exp_mount;

        // if mouting path is not privide let's duplicate mount
        if (!path) path=mount;
        // if private and not last leaf: ignore
        if (mode & RED_EXPORT_PRIVATES && !lastleaf)
            continue;

        expandpath = RedNodeStringExpand (node, path);

        // if directory: check/create it
        if (mode & RED_EXPORT_DIRS) {
            if (RwrapCheckDir(expandpath, configN, !cliargs->strict))
                continue;
        }
        // if file: check file exists
        else if (mode & RED_EXPORT_FILES) {
            if (stat(expandpath, &status) >= 0) {
                RedLog(REDLOG_WARNING,
                       "*** Node [%s] export path=%s Missing file, not mount for now(error=%s)",
                       configN->headers.alias, expandpath, strerror(errno));
                continue;
            }
        }

        exp_mount = RedNodeStringExpand (node, mount);

        switch (mode) {

        case RED_EXPORT_PRIVATE:
        case RED_EXPORT_PUBLIC:
        case RED_EXPORT_PRIVATE_FILE:
        case RED_EXPORT_PUBLIC_FILE:
            argval[(*argcount)++]="--bind";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_RESTRICTED_FILE:
        case RED_EXPORT_RESTRICTED:
        case RED_EXPORT_PRIVATE_RESTRICTED:
            argval[(*argcount)++]="--ro-bind";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_SYMLINK:
            argval[(*argcount)++]="--symlink";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_EXECFD:
            argval[(*argcount)++]="--file";
            snprintf(buffer, sizeof(buffer), "%d", MemFdExecCmd(mount, path, 1));
            argval[(*argcount)++]=strdup(buffer);
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_INTERNAL:
            argval[(*argcount)++]="--file";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_ANONYMOUS:
            argval[(*argcount)++]="--dir";
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_TMPFS:
            argval[(*argcount)++]="--tmpfs";
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_DEVFS:
            argval[(*argcount)++]="--dev";
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_PROCFS:
            argval[(*argcount)++]="--proc";
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_MQUEFS:
            argval[(*argcount)++]="--mqueue";
            argval[(*argcount)++]= exp_mount;
            break;

        case RED_EXPORT_LOCK:
            argval[(*argcount)++]="--lock-file";
            argval[(*argcount)++]= exp_mount;
            break;

        default:
            free(exp_mount);
            break;
        }
    }

    // scan export environment variables
    for (idx=0; idx < configN->confvar_count; idx++) {
        redVarEnvFlagE mode= configN->confvar[idx].mode;
        const char* key= configN->confvar[idx].key;
        const char* value=configN->confvar[idx].value;

        switch (mode) {
        case RED_CONFVAR_STATIC:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            argval[(*argcount)++]=value;
            break;

        case RED_CONFVAR_EXECFD:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            ExecCmd(key, value, buffer, sizeof(buffer), 1);
            argval[(*argcount)++]=strdup(buffer);
            break;

        case RED_CONFVAR_DEFLT:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            argval[(*argcount)++]=RedNodeStringExpand (node, value);
            break;

        case RED_CONFVAR_REMOVE:
            argval[(*argcount)++]="--unsetenv";
            argval[(*argcount)++]=key;
            break;

        default:
            break;
        }

    }
    return 0;
}

int RwrapParseConfig (redNodeT *node, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount) {
    int error = 0;

    error = RwrapParseSubConfig(node, node->config, cliargs, lastleaf, argval, argcount);
    if (error) goto OnExit;

    //load admin config if needed
    if(!node->confadmin) goto OnExit;

    error = RwrapParseSubConfig(node, node->confadmin, cliargs, lastleaf, argval, argcount);
    if(error) goto OnExit;

OnExit:
    return error;
}

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

        unsigned long epocms =RedUtcGetTimeMs();

        // check it was not updated in the future
        if (epocms < statusN->timestamp) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] is older that it's parent [require 'dnf red-update' or --force] nodepath=%s", configN->headers.alias, node->redpath);
            return 1;
        }

        // check node was not moved from one family to an other
        if (!RedConfIsSameFile(node->redpath, statusN->realpath)) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] was moved [require 'dnf red-update' or --force] nodepath=%s", configN->headers.alias, statusN->realpath);
            return 1;
        }
    }

    return 0;
}
