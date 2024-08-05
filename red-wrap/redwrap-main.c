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

// Small wrapper on top of bwrap. Read everynone node config from redpath
// generated corresponding environment variables LD_PATh, PATH, ...
// generate all sharing and mouting point
// exec bwrap
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>

#include "redwrap-main.h"
#include "redconf-defaults.h"
#include "cgroups.h"

static int loadNode(redNodeT *node, rWrapConfigT *cliarg, int lastleaf, dataNodeT *dataNode, int *argcount, const char *argval[]) {
    int error = RwrapParseNode (node, cliarg, lastleaf, argval, argcount);
    if (error) goto OnErrorExit;

    // node looks good extract path/ldpath before adding red-wrap cli program+arguments
    error = mergeSpecialConfVar(node, dataNode);
    if (error) goto OnErrorExit;

OnErrorExit:
    return error;
}

static int setmap(const char *map_path, const char *map_buf) {
    int err = 0;
    int fd;

    fd = open(map_path, O_RDWR);
    if (fd < 0) {
        RedLog(REDLOG_ERROR, "Issue open map_path=%s error=%s", map_path, strerror(errno));
        err = 1;
        goto Error;
    }

    if (write(fd, map_buf, strlen(map_buf)) != strlen(map_buf)) {
        fprintf(stderr, "ERROR: write %s: %s\n", map_buf, strerror(errno));
        err = 2;
        goto ErrorOpen;
    }

ErrorOpen:
    close(fd);
Error:
    return err;
}

static int setuidgidmap(int pid, int maprootuser) {
    const int MAP_BUF_SIZE = 100;
    char map_path[PATH_MAX];
    char map_buf[MAP_BUF_SIZE];
    int uid = getuid();
    int gid = getgid();
    int uid_to_map = maprootuser ? 0 : uid;
    int gid_to_map = maprootuser ? 0 : gid;

    /* set uid map */
    snprintf(map_path, PATH_MAX, "/proc/%d/uid_map", pid);
    snprintf(map_buf, MAP_BUF_SIZE, "%d %d 1", uid_to_map, uid);
    if(setmap(map_path, map_buf))
        goto Error;

    /* setgroups to deny */
    snprintf(map_path, PATH_MAX, "/proc/%d/setgroups", pid);
    snprintf(map_buf, MAP_BUF_SIZE, "deny");
    if(setmap(map_path, map_buf))
        goto Error;

    /* set gid map */
    snprintf(map_path, PATH_MAX, "/proc/%d/gid_map", pid);
    snprintf(map_buf, MAP_BUF_SIZE, "%d %d 1", gid_to_map, gid);
    if(setmap(map_path, map_buf))
        goto Error;

    return 0;
Error:
    return -1;
}

static int setcgroups(redConfTagT* mergedConfTags, redNodeT *rootNode) {
    char cgroupParent[PATH_MAX] = {0};

    if (mergedConfTags->cgrouproot) {
        mergedConfTags->cgrouproot = RedNodeStringExpand (rootNode, NULL, mergedConfTags->cgrouproot, NULL, NULL);
        strncpy(cgroupParent, mergedConfTags->cgrouproot, PATH_MAX);
    }

    RedLog(REDLOG_DEBUG, "[redwrap-main]: set cgroup");
    for (redNodeT *node=rootNode; node != NULL; node=node->childs->child) {
        //remove / from cgroup name
        char *cgroup_name = (char *)alloca(strlen(node->status->realpath));
        replaceSlashDash(node->status->realpath, cgroup_name);

        if (cgroups(node->config->conftag->cgroups, cgroup_name, cgroupParent))
            break;

        //set next cgroup parent
        strncat(cgroupParent, "/", PATH_MAX - 1 - strlen("/") - strlen(cgroupParent));
        strncat(cgroupParent, cgroup_name, PATH_MAX - 1 - strlen(cgroup_name) - strlen(cgroupParent));
    }
    return 0;
}

int redwrapMain (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {
    if (cliarg->verbose)
        SetLogLevel(REDLOG_DEBUG);


    redConfTagT *mergedConfTags = NULL;
    int argcount=0;
    int error;
    const char *argval[MAX_BWRAP_ARGS];
    char pathString[BWRAP_MAXVAR_LEN];
    char ldpathString[BWRAP_MAXVAR_LEN];
    dataNodeT dataNode = {
        .ldpathString = ldpathString,
        .ldpathIdx = 0,
        .pathString = pathString,
        .pathIdx = 0
    };

    // start argument list with red-wrap command name
    argval[argcount++] = command_name;

    // update verbose/redpath from cliarg
    const char *redpath = cliarg->redpath;

    redNodeT *redtree = RedNodesScan(redpath, cliarg->verbose);
    if (!redtree) {
        RedLog(REDLOG_ERROR, "Fail to scan rednodes family tree redpath=%s", redpath);
        goto OnErrorExit;
    }

    // build arguments from nodes family tree
    // Scan redpath family nodes from terminal leaf to root node without ancestor
    int isCgroups = 0;

    redNodeT *rootNode = NULL;
    for (redNodeT *node=redtree; node != NULL; node=node->ancestor) {
        error = loadNode(node, cliarg, (node == redtree), &dataNode, &argcount, argval);
        if (error) goto OnErrorExit;
        rootNode = node;
        if (node->config->conftag->cgroups)
            isCgroups = 1;
    }

    mergedConfTags = mergedConftags(rootNode, cliarg->adminpath ? 1 : 0);

    if (RedSetCapabilities(rootNode, mergedConfTags, argval, &argcount)) {
        RedLog(REDLOG_ERROR, "Cannot set capabilities");
        goto OnErrorExit;
    }

    if (isCgroups) setcgroups(mergedConfTags, rootNode);

    // add commulated LD_PATH_LIBRARY & PATH
    argval[argcount++]="--setenv";
    argval[argcount++]="PATH";
    argval[argcount++]=strdup(pathString);
    argval[argcount++]="--setenv";
    argval[argcount++]="LD_LIBRARY_PATH";
    argval[argcount++]=strdup(ldpathString);


    // set global merged config tags
    if (mergedConfTags->hostname) {
        argval[argcount++]="--unshare-uts";
        argval[argcount++]="--hostname";
        argval[argcount++]= RedNodeStringExpand (redtree, NULL, mergedConfTags->hostname, NULL, NULL);
    }

    if (mergedConfTags->chdir) {
        argval[argcount++]="--chdir";
        argval[argcount++]= RedNodeStringExpand (redtree, NULL, mergedConfTags->chdir, NULL, NULL);
    }

    if (!(mergedConfTags->share_all & RED_CONF_OPT_ENABLED)) {
        argval[argcount++]="--unshare-all";
    }

    if (!(mergedConfTags->share_user & RED_CONF_OPT_ENABLED)) {
        argval[argcount++]="--unshare-user";
    }

    if (!(mergedConfTags->share_cgroup & RED_CONF_OPT_ENABLED)) {
        argval[argcount++]="--unshare-cgroup";
    }

    if (!(mergedConfTags->share_ipc & RED_CONF_OPT_ENABLED)) {
        argval[argcount++]="--unshare-ipc";
    }

    if (!(mergedConfTags->share_pid & RED_CONF_OPT_ENABLED)) {
        argval[argcount++]="--unshare-pid";
    }

    if (mergedConfTags->share_net & RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-net";
    } else {
        argval[argcount++]="--unshare-net";
    }

    if (mergedConfTags->diewithparent & RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--die-with-parent";
    }

    if (mergedConfTags->newsession & RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--new-session";
    }

    // add remaining program to execute arguments
    for (int idx=0; idx < subargc; idx++ ) {
        if (idx == MAX_BWRAP_ARGS) {
            RedLog(REDLOG_ERROR,"red-wrap too many arguments limit=[%d]", MAX_BWRAP_ARGS);
        }
        argval[argcount++]=subargv[idx];
    }

    //if (1) {
    //    printf("\n#### OPTIONS ####\n");
    //    for (int idx=1; idx < argcount; idx++) {
    //        printf (" %s", argval[idx]);
    //    }
    //    printf ("\n###################\n");
    //}


    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        RedLog(REDLOG_ERROR, "Cannot pipe error=%s", strerror(errno));
        goto OnErrorExit;
    }

    int pid = (int) syscall (__NR_clone, CLONE_NEWUSER|SIGCHLD, NULL);
    if (pid < 0) {
        RedLog(REDLOG_ERROR, "Cannot clone with NEWUSER error=%s", strerror(errno));
        goto OnErrorExit;
    }

    // exec command
    if (pid == 0) {
        /* wait for parent to set uid/gid maps */
        close(pipe_fd[1]);
        close(pipe_fd[0]);

        /* unshare time ns */
        if (!(mergedConfTags->share_time & RED_CONF_OPT_ENABLED))
            unshare(CLONE_NEWTIME);

        argval[argcount]=NULL;
        if(execv(cliarg->bwrap, (char**) argval)) {
            RedLog(REDLOG_ERROR, "bwrap command issue: %s", strerror(errno));
            return 1;
        }
        return 0;
    }
    setuidgidmap(pid, mergedConfTags->maprootuser);
    /* signal child that uid/gid maps are set */
    close(pipe_fd[1]);
    int returnStatus;
    waitpid(pid, &returnStatus, 0);
    return returnStatus;

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    return -1;
}
