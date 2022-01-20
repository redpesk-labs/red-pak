/*
 * Copyright (C) 2020 "IoT.bzh"
 * Author fulup Ar Foll <fulup@iot.bzh>
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "redwrap.h"

static int RwrapParseSubConfig (redNodeT *node, redConfigT *configN, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount) {
    int err;

    // scan export directory
    //printf("\n\n\n--------- Scan export for %s\n", node->redpath);
    for (int idx=0; idx <configN->exports_count; idx++) {
        redExportFlagE mode= configN->exports[idx].mode;
        const char* mount= configN->exports[idx].mount;
        const char* path=configN->exports[idx].path;
        struct stat status;
        const char * expandpath = NULL;

        // if mouting path is not privide let's duplicate mount
        if (!path) path=mount;
	//printf("path=%s expandpath%s mount=%s \n", path, expandpath, mount);
        switch (mode) {
        case RED_EXPORT_PRIVATE:
            // on export if we are in terminal lead node
            if (lastleaf) {
                expandpath = RedNodeStringExpand (node, NULL, path, NULL, NULL);
                argval[(*argcount)++]="--bind";
                argval[(*argcount)++]=expandpath;
                argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            }
            break;

        case  RED_EXPORT_PUBLIC:
            expandpath = RedNodeStringExpand (node, NULL, path, NULL, NULL);
            argval[(*argcount)++]="--bind";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_RESTRICTED:
            expandpath = RedNodeStringExpand (node, NULL, path, NULL, NULL);
            argval[(*argcount)++]="--ro-bind";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_SYMLINK:
            argval[(*argcount)++]="--symlink";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, path, NULL, NULL);
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_EXECFD:
            argval[(*argcount)++]="--file";
            argval[(*argcount)++]=MemFdExecCmd (mount, path);
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_DEFLT:
            argval[(*argcount)++]="--file";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, path, NULL, NULL);
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_ANONYMOUS:
            argval[(*argcount)++]="--dir";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_TMPFS:
            argval[(*argcount)++]="--tmpfs";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_DEVFS:
            argval[(*argcount)++]="--dev";
            if (mount) argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_PROCFS:
            argval[(*argcount)++]="--proc";
            if (mount) argval[(*argcount)++]= RedNodeStringExpand (node, NULL, mount, NULL, NULL);
            break;

        case RED_EXPORT_MQUEFS:
            argval[(*argcount)++]="--mqueue";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, path, NULL, NULL);
            break;

        case RED_EXPORT_LOCK:
            argval[(*argcount)++]="--lock-file";
            argval[(*argcount)++]= RedNodeStringExpand (node, NULL, path, NULL, NULL);
            break;

        default:
            break;
        }

        if (!expandpath) continue;
        //check path after expanding
        err = stat(expandpath, &status);
        switch (mode) {
        case RED_EXPORT_PRIVATE:
        case RED_EXPORT_PUBLIC:
        case RED_EXPORT_RESTRICTED:
            if (err < 0) {
                if (cliargs->forcemod) {
                    // mkdir ACL do not work as documented, need chmod to install proper acl
                    mode_t mask=RedSetUmask(configN->conftag);
               		mask= 07777 & ~mask;
                    mkdir(expandpath, 0);
                    chmod(expandpath, mask);
                    err = stat(expandpath, &status);
                }
            }
            if (err < 0) {
                RedLog(REDLOG_ERROR, "*** Node [%s] export path=%s(=%s) does not exist [use --force]", configN->headers->alias, path, expandpath);
                goto OnErrorExit;
            }
                break;
        default:
            break;
        }
    }

    // scan export environment variables
    for (int idx=0; idx < configN->confvar_count; idx++) {
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
            argval[(*argcount)++]=MemFdExecCmd (key, value);
            break;

        case RED_CONFVAR_DEFLT:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, value, NULL, NULL);
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

OnErrorExit:
    return 1;
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

int RwrapParseNode (redNodeT *node, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount) {
    int error;

    redConfigT *configN = node->config;
    redStatusT *statusN = node->status;
    unsigned long epocms =RedUtcGetTimeMs();

    // make sure node is not disabled
    if (statusN->state !=  RED_STATUS_ENABLE) {
        RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] is DISABLED [check/update node] nodepath=%s", configN->headers->alias, node->redpath);
        goto OnErrorExit;
    }

    // if not in force mode do further sanity check
    if (!(configN->conftag->unsafe || cliargs->unsafe)) {
        // check it was updated in the future
        if (epocms < statusN->timestamp) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] is older that it's parent [require 'dnf red-update' or --force] nodepath=%s", configN->headers->alias, node->redpath);
            goto OnErrorExit;
        }

        // check node was not moved from one family to an other
        if (RedConfGetInod(node->redpath) != RedConfGetInod(statusN->realpath)) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] was moved [require 'dnf red-update' or --force] nodepath=%s", configN->headers->alias, statusN->realpath);
            goto OnErrorExit;
        }

        // update epoc for parent time check
        epocms=statusN->timestamp;
    }

    // Finaly add environment from node config
    error=  RwrapParseConfig (node, cliargs, lastleaf, argval, argcount);
    if (error) goto OnErrorExit;

    return 0;

OnErrorExit:
    return 1;
}
