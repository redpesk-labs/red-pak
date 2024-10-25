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

#include "redwrap-exec.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#ifndef CLONE_NEWTIME
#include <linux/sched.h>
#endif
#include <sys/syscall.h>

#include "redconf-log.h"
#include "redconf-schema.h"
#include "redconf-merge.h"
#include "redconf-hashmerge.h"
#include "redconf-expand.h"
#include "redconf-defaults.h"
#include "redconf-utils.h"
#include "redwrap-node.h"
#include "cgroups.h"

#ifndef BWRAP_MAXVAR_LEN
#define BWRAP_MAXVAR_LEN 1024
#endif

/** redwrap exec state */
typedef
struct redwrap_state_s
{
    /** redwrap input arguments */
    rWrapConfigT *cliarg;
    /** the rednode */
    redNodeT     *rednode;
    /** the root node of the rednode */
    redNodeT     *rootnode;
    /** flag indicating if cgroups are required */
    int           has_cgroups;
    /** flag indicating if time should be unshared */
    int           unshare_time;
    /** flag indicating if rednode user must map to root */
    int           map_user_root;
    /** arguments' count of the bwrap command */
    int           argcount;
    /** arguments' array of the bwrap command */
    const char   *argval[MAX_BWRAP_ARGS];
}
    redwrap_state_t;

/**
 * check if the node and its parents are valid
 */
static int validateNode(redNodeT *node, int unsafe)
{
    int result = 0;
    while (node != NULL && result == 0) {
        result = RwrapValidateNode(node, unsafe);
        node = node->parent;
    }
    return result;
}

/**
 * set the special conf variables
 */
static int set_special_confvar(redwrap_state_t *restate)
{
    const redNodeT *node;
    char pathString[BWRAP_MAXVAR_LEN];
    char ldpathString[BWRAP_MAXVAR_LEN];
    dataNodeT dataNode = {
        .ldpathString = ldpathString,
        .ldpathIdx = 0,
        .pathString = pathString,
        .pathIdx = 0
    };
    int result = 0;

    pathString[0] = ldpathString[0] = 0;
    for (node = restate->rednode; node != NULL && result == 0; node = node->parent)
        result = mergeSpecialConfVar(node, &dataNode);

    if (result == 0) {
        restate->argval[restate->argcount++] = "--setenv";
        restate->argval[restate->argcount++] = "PATH";
        restate->argval[restate->argcount++] = strdup(pathString);
        restate->argval[restate->argcount++] = "--setenv";
        restate->argval[restate->argcount++] = "LD_LIBRARY_PATH";
        restate->argval[restate->argcount++] = strdup(ldpathString);
    }

    return result;
}

static void set_one_envvar(redwrap_state_t *restate, const redConfVarT *confvar, const redNodeT *node)
{
// scan export environment variables
    char buffer[2049];
    redVarEnvFlagE mode = confvar->mode;
    const char *key = confvar->key;
    const char *value = confvar->value;

    switch (mode) {
    case RED_CONFVAR_STATIC:
        restate->argval[restate->argcount++] = "--setenv";
        restate->argval[restate->argcount++] = key;
        restate->argval[restate->argcount++] = value;
        break;

    case RED_CONFVAR_EXECFD:
        restate->argval[restate->argcount++] = "--setenv";
        restate->argval[restate->argcount++] = key;
        ExecCmd(key, value, buffer, sizeof(buffer), 1);
        restate->argval[restate->argcount++] = strdup(buffer);
        break;

    case RED_CONFVAR_DEFLT:
        restate->argval[restate->argcount++] = "--setenv";
        restate->argval[restate->argcount++] = key;
        restate->argval[restate->argcount++] = RedNodeStringExpand (node, value);
        break;

    case RED_CONFVAR_REMOVE:
        restate->argval[restate->argcount++] = "--unsetenv";
        restate->argval[restate->argcount++] = key;
        break;

    default:
        break;
    }
}

static void set_envvars(redwrap_state_t *restate, const redNodeT *node, const redConfigT *configN)
{
    const redConfVarT *iter = configN->confvar;
    const redConfVarT *end  = &iter[configN->confvar_count];
    while(iter != end)
        set_one_envvar(restate, iter++, node);
}

static void set_one_export(redwrap_state_t *restate, const redConfExportPathT *export, const redNodeT *node)
{
    char buffer[2049];

    // scan export directory
    redExportFlagE mode = export->mode;
    const char* mount = export->mount;
    const char* path =export->path;
    struct stat status;
    const char * expandpath = NULL;
    char *exp_mount;

    // if mouting path is not privide let's duplicate mount
    if (!path) path=mount;

    // if private and not last leaf: ignore
    if ((mode & RED_EXPORT_PRIVATES) != 0 && node != restate->rednode)
        return;

    expandpath = RedNodeStringExpand (node, path);

    // if directory: check/create it
    if (mode & RED_EXPORT_DIRS) {
        int err = stat(expandpath, &status);
        if (err && !restate->cliarg->strict) {
            err = make_directories(expandpath, 0, strlen(expandpath), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH, NULL) != 0;
        }
        if (err) {
            RedLog(REDLOG_WARNING, "*** Node [%s] export expanded path=%s does not exist (error=%s) [use --force]", node->config->headers.alias, path, strerror(errno));
            return;
        }
    }
    // if file: check file exists
    else if (mode & RED_EXPORT_FILES) {
        if (stat(expandpath, &status) >= 0) {
            RedLog(REDLOG_WARNING,
                   "*** Node [%s] export path=%s Missing file, not mount for now(error=%s)",
                   node->config->headers.alias, expandpath, strerror(errno));
            return;
        }
    }

    exp_mount = RedNodeStringExpand (node, mount);

    switch (mode) {

    case RED_EXPORT_PRIVATE:
    case RED_EXPORT_PUBLIC:
    case RED_EXPORT_PRIVATE_FILE:
    case RED_EXPORT_PUBLIC_FILE:
        restate->argval[restate->argcount++] = "--bind";
        restate->argval[restate->argcount++] = expandpath;
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_RESTRICTED_FILE:
    case RED_EXPORT_RESTRICTED:
    case RED_EXPORT_PRIVATE_RESTRICTED:
        restate->argval[restate->argcount++] = "--ro-bind";
        restate->argval[restate->argcount++] = expandpath;
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_SYMLINK:
        restate->argval[restate->argcount++] = "--symlink";
        restate->argval[restate->argcount++] = expandpath;
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_EXECFD:
        restate->argval[restate->argcount++] = "--file";
        snprintf(buffer, sizeof(buffer), "%d", MemFdExecCmd(mount, path, 1));
        restate->argval[restate->argcount++] = strdup(buffer);
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_INTERNAL:
        restate->argval[restate->argcount++] = "--file";
        restate->argval[restate->argcount++] = expandpath;
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_ANONYMOUS:
        restate->argval[restate->argcount++] = "--dir";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_TMPFS:
        restate->argval[restate->argcount++] = "--tmpfs";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_DEVFS:
        restate->argval[restate->argcount++] = "--dev";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_PROCFS:
        restate->argval[restate->argcount++] = "--proc";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_MQUEFS:
        restate->argval[restate->argcount++] = "--mqueue";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    case RED_EXPORT_LOCK:
        restate->argval[restate->argcount++] = "--lock-file";
        restate->argval[restate->argcount++] =  exp_mount;
        break;

    default:
        free(exp_mount);
        break;
    }
}

static void set_exports(redwrap_state_t *restate, const redNodeT *node, const redConfigT *configN)
{
    const redConfExportPathT *iter = configN->exports;
    const redConfExportPathT *end  = &iter[configN->exports_count];
    while(iter != end)
        set_one_export(restate, iter++, node);
}

static int RwrapParseSubConfig (redwrap_state_t *restate, const redNodeT *node, const redConfigT *configN)
{
    set_exports(restate, node, configN);
    set_envvars(restate, node, configN);
    return 0;
}

static int set_exports_and_vars(redwrap_state_t *restate)
{
    redNodeT *node = restate->rednode;
    int error = 0;

    do {
        error = RwrapParseSubConfig(restate, node, node->config);
        if (error == 0 && node->confadmin != NULL)
            error = RwrapParseSubConfig(restate, node, node->confadmin);
        node=node->parent;
    }
    while (node != NULL && error == 0);
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

    if (write(fd, map_buf, strlen(map_buf)) != (ssize_t)strlen(map_buf)) {
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

static int setcgroups(const redConfTagT* conftag, redNodeT *rootNode) {
    char cgroupParent[PATH_MAX] = {0};

    if (conftag->cgrouproot)
        strncpy(cgroupParent, conftag->cgrouproot, PATH_MAX);

    RedLog(REDLOG_DEBUG, "[redwrap-main]: set cgroup");
    for (redNodeT *node=rootNode; node != NULL; node=node->first_child) {
        //remove / from cgroup name
        char *cgroup_name = (char *)alloca(strlen(node->status->realpath));
        replaceSlashDash(node->status->realpath, cgroup_name);

        if (cgroups(node->config->conftag.cgroups, cgroup_name, cgroupParent))
            break;

        //set next cgroup parent
        strncat(cgroupParent, "/", PATH_MAX - 1 - strlen("/") - strlen(cgroupParent));
        strncat(cgroupParent, cgroup_name, PATH_MAX - 1 - strlen(cgroup_name) - strlen(cgroupParent));
    }
    return 0;
}

static int set_one_capability(redwrap_state_t *restate, const char *capability, int value)
{
    restate->argval[restate->argcount++] = value ? "--cap-add" : "--cap-drop";
    restate->argval[restate->argcount++] = capability;
    return 0;
}

static int set_capabilities(redwrap_state_t *restate, const redConfTagT *conftag) {

    for(int i = 0; i < conftag->capabilities_count; i++) {
        redConfCapT *cap = conftag->capabilities+i;
        set_one_capability(restate, cap->cap, cap->add);
    }

    return 0;
}

/* this function tests if sharing of (item) is disabled  */
static bool can_unshare(redConfOptFlagE target, redConfOptFlagE all)
{
    return target == RED_CONF_OPT_DISABLED
                || (target == RED_CONF_OPT_UNSET && all != RED_CONF_OPT_ENABLED);
}

static int set_shares(redwrap_state_t *restate, const redConfShareT *shares)
{
    if (can_unshare(shares->user, shares->all))
        restate->argval[restate->argcount++]="--unshare-user";

    if (can_unshare(shares->cgroup, shares->all))
        restate->argval[restate->argcount++]="--unshare-cgroup";

    if (can_unshare(shares->ipc, shares->all))
        restate->argval[restate->argcount++]="--unshare-ipc";

    if (can_unshare(shares->pid, shares->all))
        restate->argval[restate->argcount++]="--unshare-pid";

    if (can_unshare(shares->net, shares->all))
        restate->argval[restate->argcount++]="--unshare-net";
    else
        restate->argval[restate->argcount++]="--share-net";

    restate->unshare_time = can_unshare(shares->time, shares->all);

    return 0;
}

static int set_conftag(redwrap_state_t *restate, const redConfTagT *conftag)
{
    set_capabilities(restate, conftag);

    if (restate->has_cgroups)
        setcgroups(conftag, restate->rootnode);

    // set global merged config tags
    if (conftag->hostname) {
        restate->argval[restate->argcount++]="--unshare-uts";
        restate->argval[restate->argcount++]="--hostname";
        restate->argval[restate->argcount++]= RedNodeStringExpand (restate->rednode, conftag->hostname);
    }

    if (conftag->chdir) {
        restate->argval[restate->argcount++]="--chdir";
        restate->argval[restate->argcount++]= RedNodeStringExpand (restate->rednode, conftag->chdir);
    }

    if (conftag->diewithparent & RED_CONF_OPT_ENABLED)
        restate->argval[restate->argcount++]="--die-with-parent";

    if (conftag->newsession & RED_CONF_OPT_ENABLED)
        restate->argval[restate->argcount++]="--new-session";

    restate->map_user_root = conftag->maprootuser;

    return set_shares(restate, &conftag->share);
}

static int set_for_conftag(redwrap_state_t *restate)
{
    redConfTagT mct;
    memset(&mct, 0, sizeof mct);
    if(mergeConfTag(restate->rootnode, &mct, 0)) {
        RedLog(REDLOG_ERROR, "Issue to merge conftag");
        return 1;
    }
    if(mergeCapabilities(restate->rootnode, &mct, 0)) {
        RedLog(REDLOG_ERROR, "Issue to merge capabilities");
        return 1;
    }
    if (restate->has_cgroups && mct.cgrouproot)
        mct.cgrouproot = RedNodeStringExpand (restate->rootnode, mct.cgrouproot);
    return set_conftag(restate, &mct);
}

int redwrapExecBwrap (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {
    redwrap_state_t restate;
    int idx, error;
    int pipe_fd[2];
    pid_t pid;

    if (cliarg->verbose)
        SetLogLevel(REDLOG_DEBUG);

    // start argument list with red-wrap command name
    restate.cliarg = cliarg;
    restate.rednode = NULL;
    restate.rootnode = NULL;
    restate.has_cgroups = 0;
    restate.unshare_time = 0;
    restate.map_user_root = 0;
    restate.argcount = 1;
    restate.argval[0] = command_name;

    /* get the rootnode */
    restate.rednode = RedNodesScan(cliarg->redpath, cliarg->isadmin, cliarg->verbose);
    if (!restate.rednode) {
        RedLog(REDLOG_ERROR, "Fail to scan rednodes family tree redpath=%s", cliarg->redpath);
        goto OnErrorExit;
    }

    /* search the rootnode */
    restate.rootnode = restate.rednode;
    restate.has_cgroups = (restate.rednode->config->conftag.cgroups != NULL);
    while (restate.rootnode->parent != NULL) {
        restate.rootnode = restate.rootnode->parent;
        restate.has_cgroups |= (restate.rootnode->config->conftag.cgroups != NULL);
    }

    /* validate the node */
    error = validateNode(restate.rednode, restate.cliarg->unsafe);
    if (error)
        goto OnErrorExit;

    /* set exports and vars */
    error = set_exports_and_vars(&restate);
    if (error)
        goto OnErrorExit;

    /* set config */
    error = set_for_conftag(&restate);
    if (error)
        goto OnErrorExit;

    // add LD_PATH_LIBRARY & PATH
    error = set_special_confvar(&restate);
    if (error)
        goto OnErrorExit;

    // add program to execute at tail of arguments
    for (idx = 0; idx < subargc; idx++ ) {
        if (idx == MAX_BWRAP_ARGS) {
            RedLog(REDLOG_ERROR,"red-wrap too many arguments limit=[%d]", MAX_BWRAP_ARGS);
        }
        restate.argval[restate.argcount++]=subargv[idx];
    }

    /* dump the call to bwrap */
    if (cliarg->dump) {
        FILE *fout = cliarg->dump > 1 ? stdout : stderr;
        fprintf(fout, "\nDUMP: %s (as %s)", cliarg->bwrap, restate.argval[0]);
        for (int idx = 1; idx < restate.argcount; idx++ ) {
            if (cliarg->dump > 1 && restate.argval[idx][0] == '-')
                fprintf(fout, "\n ");
            fprintf(fout, " %s", restate.argval[idx]);
        }
        fprintf(fout, "\n");
        fflush(fout);
        if (cliarg->dump > 1)
            exit(0);
    }

    /* setup a connexion from parent to child */
    if (pipe(pipe_fd) == -1) {
        RedLog(REDLOG_ERROR, "Cannot pipe error=%s", strerror(errno));
        goto OnErrorExit;
    }

    /* enter new user namespace now */
    pid = (pid_t) syscall (__NR_clone, CLONE_NEWUSER|SIGCHLD, NULL);
    if (pid < 0) {
        RedLog(REDLOG_ERROR, "Cannot clone with NEWUSER error=%s", strerror(errno));
        goto OnErrorExit;
    }
    if (pid == 0) {
        /* in forked process */
        /* wait for parent to set uid/gid maps */
        close(pipe_fd[1]);
        read(pipe_fd[0], &pid, sizeof(pid));
        close(pipe_fd[0]);

        /* unshare time nanespace if required */
        if (restate.unshare_time)
            unshare(CLONE_NEWTIME);

        /* exec bwrap now */
        restate.argval[restate.argcount]=NULL;
        if(execve(cliarg->bwrap, (char**) restate.argval, NULL)) {
            RedLog(REDLOG_ERROR, "bwrap command issue: %s", strerror(errno));
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* in parent process */
    /* set uid/gid maps and signal child */
    close(pipe_fd[0]);
    setuidgidmap(pid, restate.map_user_root);
    write(pipe_fd[1], &pid, sizeof(pid));
    close(pipe_fd[1]);

    /* wait child completion */
    waitpid(pid, &error, 0);
    return error;

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    return 1;
}
