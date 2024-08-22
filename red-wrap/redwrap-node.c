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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "redconf-hashmerge.h"
#include "redwrap.h"

static int mkdir_p(const char *path, mode_t mode) {
    size_t len = strlen(path);
    if(len + 1 > RED_MAXPATHLEN) {
        RedLog(REDLOG_ERROR, "size=%d of path %s is bigger the RED_MAXPATHLEN=%d", len, path, RED_MAXPATHLEN);
        goto OnError;
    }

    int err;
    char tmp[RED_MAXPATHLEN];
    struct stat status;

    for (int i = 0; i < len + 1; i++) {
        tmp[i] = path[i];

        /* if not / or not full path */
        if(tmp[i] != '/' && i < len)
            continue;

        tmp[i+1] = '\0';
        /* is directory exist */
        err = stat(tmp, &status);
        if(!err)
            continue;

        err = mkdir((const char *)tmp, mode);
        if(err) {
            RedLog(REDLOG_ERROR, "Cannot create %s (error=%s)", tmp, strerror(errno));
            goto OnError;
        }
    }

    return 0;
OnError:
    return 1;
}

/**
 * @brief Check if folder to mount exists, and try to create it if not
 *
 * @param path      Path into the node
 * @param configN   Config of the Node
 * @param forcemod  force command
 * @return 0 in success negative otherwise
 */
static int RwrapCreateDir(const char *path, redConfigT *configN, int forcemod) {
    struct stat status;
    int err = stat(path, &status);
    if (err) {
        if (forcemod) {
            //err = mkdir(path, 07777);
            err = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            // try to recursively create directories
            if(err)
                err = mkdir_p(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
    }
    if (err) {
        RedLog(REDLOG_WARNING, "*** Node [%s] export expanded path=%s does not exist (error=%s) [use --force]", configN->headers->alias, path, strerror(errno));
        return err;
    }
    return 0;
}

/**
 * @brief Adding arguments for bwrap to mount path from system to node
 *
 * @param node          Node data
 * @param mount         System path to mount
 * @param bindMode      Mode of the bind "--bind" or "--ro-bind"
 * @param expandpath    Node path to mount
 * @param argval        List of arguments
 * @param argcount      Index and count args
 */
static void RwrapMountModeArgval(redNodeT *node, const char *mount, const char *bindMode, const char *expandpath, const char *argval[], int *argcount) {
    argval[(*argcount)++]=bindMode;
    argval[(*argcount)++]=expandpath;
    argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
}

static int RwrapParseSubConfig (redNodeT *node, redConfigT *configN, rWrapConfigT *cliargs, int lastleaf, const char *argval[], int *argcount) {

    // scan export directory
    for (int idx=0; idx <configN->exports_count; idx++) {
        redExportFlagE mode= configN->exports[idx].mode;
        const char* mount= configN->exports[idx].mount;
        const char* path=configN->exports[idx].path;
        struct stat status;
        const char * expandpath = NULL;
        char fdstr[32];

        // if mouting path is not privide let's duplicate mount
        if (!path) path=mount;
        // if private and not last leaf: ignore
        if (mode & RED_EXPORT_PRIVATES && !lastleaf)
            continue;

        expandpath = RedNodeStringExpand (node, NULL, path);

        // if directory: check/try to create it in forcemod else ignore
        if (mode & RED_EXPORT_DIRS) {
            if (RwrapCreateDir(expandpath, configN, cliargs->forcemod))
                continue;
        }
        // if file: check file exists
        else if (mode & RED_EXPORT_FILES) {
            if (stat(expandpath, &status) >= 0) {
                RedLog(REDLOG_WARNING,
                       "*** Node [%s] export path=%s Missing file, not mount for now(error=%s)",
                       configN->headers->alias, expandpath, strerror(errno));
                continue;
            }
        }

        switch (mode) {

        case RED_EXPORT_PRIVATE:
        case RED_EXPORT_PUBLIC:
        case RED_EXPORT_PRIVATE_FILE:
        case RED_EXPORT_PUBLIC_FILE:
            RwrapMountModeArgval(node, mount, "--bind", expandpath, argval, argcount);
            break;

        case RED_EXPORT_RESTRICTED_FILE:
        case RED_EXPORT_RESTRICTED:
        case RED_EXPORT_PRIVATE_RESTRICTED:
            RwrapMountModeArgval(node, mount, "--ro-bind", expandpath, argval, argcount);
            break;

        case RED_EXPORT_SYMLINK:
            argval[(*argcount)++]="--symlink";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_EXECFD:
            argval[(*argcount)++]="--file";
            snprintf(fdstr, sizeof(fdstr), "%d", MemFdExecCmd(mount, path, 1));
            argval[(*argcount)++]=strdup(fdstr);
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_DEFLT:
            argval[(*argcount)++]="--file";
            argval[(*argcount)++]=expandpath;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_ANONYMOUS:
            argval[(*argcount)++]="--dir";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_TMPFS:
            argval[(*argcount)++]="--tmpfs";
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_DEVFS:
            argval[(*argcount)++]="--dev";
            if (mount) argval[(*argcount)++]=RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_PROCFS:
            argval[(*argcount)++]="--proc";
            if (mount) argval[(*argcount)++]= RedNodeStringExpand (node, NULL, mount);
            break;

        case RED_EXPORT_MQUEFS:
            argval[(*argcount)++]="--mqueue";
            argval[(*argcount)++]=expandpath;
            break;

        case RED_EXPORT_LOCK:
            argval[(*argcount)++]="--lock-file";
            argval[(*argcount)++]= expandpath;
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
        char fdstr[32];

        switch (mode) {
        case RED_CONFVAR_STATIC:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            argval[(*argcount)++]=value;
            break;

        case RED_CONFVAR_EXECFD:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            snprintf(fdstr, sizeof(fdstr), "%d", MemFdExecCmd(key, value, 1));
            argval[(*argcount)++]=strdup(fdstr);
            break;

        case RED_CONFVAR_DEFLT:
            argval[(*argcount)++]="--setenv";
            argval[(*argcount)++]=key;
            argval[(*argcount)++]=RedNodeStringExpand (node, NULL, value);
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
    if (!((configN->conftag && configN->conftag->unsafe) || cliargs->unsafe)) {
        // check it was updated in the future
        if (epocms < statusN->timestamp) {
            RedLog(REDLOG_ERROR, "*** ERROR: Node [%s] is older that it's parent [require 'dnf red-update' or --force] nodepath=%s", configN->headers->alias, node->redpath);
            goto OnErrorExit;
        }

        // check node was not moved from one family to an other
        if (!RedConfIsSameFile(node->redpath, statusN->realpath)) {
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

int RedSetCapabilities(const redNodeT *rootnode, redConfTagT *mergedConfTags, const char *argval[], int *argcount) {
    redConfCapT *cap;

    if(mergeCapabilities(rootnode, mergedConfTags, 0)) {
        RedLog(REDLOG_ERROR, "ISsue to merge capabilities");
        goto Error;
    }

    for(int i = 0; i < mergedConfTags->capabilities_count; i++) {
        cap = mergedConfTags->capabilities+i;
        argval[(*argcount)++] = cap->add ? "--cap-add" : "--cap-drop";
        argval[(*argcount)++] = cap->cap;
    }

    return 0;
Error:
    return 1;
}