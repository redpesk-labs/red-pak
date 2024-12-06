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
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#ifndef CLONE_NEWTIME
#include <linux/sched.h>
#endif
#include <sys/syscall.h>

#include "redconf-log.h"
#include "redconf-schema.h"
#include "redconf-mix.h"
#include "redconf-expand.h"
#include "redconf-defaults.h"
#include "redconf-utils.h"
#include "redwrap-node.h"
#include "cgroups.h"
#include "redconf-sharing.h"

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
    /** copy of the shares */
    redConfShareT shares;
    /** flag indicating if rednode user must map to root */
    int           map_user_root;
    /** arguments' count of the bwrap command */
    int           argcount;
    /** arguments' array of the bwrap command */
    const char   *argval[MAX_BWRAP_ARGS];
}
    redwrap_state_t;

/**
 * Macro for adding a value in the redwrap state
 */
#define ADD(ars,value) do{                           \
            redwrap_state_t *rs = (ars);              \
            if (rs->argcount < MAX_BWRAP_ARGS)       \
                rs->argval[rs->argcount++] = (value);\
            else                                     \
                too_many_parameters();               \
    }while(0)

#define ADD2(ars,val1,val2)       do{ADD((ars),(val1));ADD((ars),(val2));}while(0)
#define ADD3(ars,val1,val2,val3)  do{ADD((ars),(val1));ADD((ars),(val2));ADD((ars),(val3));}while(0)

/**
 * emit an error if there are too many declared parameters
 */
static void too_many_parameters()
{
    RedLog(REDLOG_ERROR, "red-wrap too many arguments limit=[%d]", MAX_BWRAP_ARGS);
    exit(EXIT_FAILURE);
}

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

static void set_one_envvar(redwrap_state_t *restate, const redConfVarT *confvar, const redNodeT *node)
{
// scan export environment variables
    char buffer[2049];
    redVarEnvFlagE mode = confvar->mode;
    const char *key = confvar->key;
    const char *value = confvar->value;

    switch (mode) {
    case RED_CONFVAR_STATIC:
        ADD3(restate, "--setenv", key, value);
        break;

    case RED_CONFVAR_EXECFD:
        ExecCmd(key, value, buffer, sizeof(buffer), 1);
        ADD3(restate, "--setenv", key, strdup(buffer));
        break;

    case RED_CONFVAR_DEFLT:
        ADD3(restate, "--setenv", key, RedNodeStringExpand (node, value));
        break;

    case RED_CONFVAR_REMOVE:
        ADD2(restate, "--unsetenv", key);
        break;

    case RED_CONFVAR_INHERIT:
        ADD3(restate, "--setenv", key, secure_getenv(key) ?: "");
        break;

    default:
        break;
    }
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
            if (restate->cliarg->dump > 1)
                err = 0;
            else
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
        ADD3(restate, "--bind", expandpath, exp_mount);
        break;

    case RED_EXPORT_RESTRICTED_FILE:
    case RED_EXPORT_RESTRICTED:
    case RED_EXPORT_PRIVATE_RESTRICTED:
        ADD3(restate, "--ro-bind", expandpath, exp_mount);
        break;

    case RED_EXPORT_SYMLINK:
        ADD3(restate, "--symlink", expandpath, exp_mount);
        break;

    case RED_EXPORT_EXECFD:
        snprintf(buffer, sizeof(buffer), "%d", MemFdExecCmd(mount, path, 1));
        ADD3(restate, "--file", strdup(buffer),  exp_mount);
        break;

    case RED_EXPORT_INTERNAL:
        ADD3(restate, "--file", expandpath, exp_mount);
        break;

    case RED_EXPORT_ANONYMOUS:
        ADD2(restate, "--dir", exp_mount);
        break;

    case RED_EXPORT_TMPFS:
        ADD2(restate, "--tmpfs", exp_mount);
        break;

    case RED_EXPORT_DEVFS:
        ADD2(restate, "--dev", exp_mount);
        break;

    case RED_EXPORT_PROCFS:
        ADD2(restate, "--proc", exp_mount);
        break;

    case RED_EXPORT_MQUEFS:
        ADD2(restate, "--mqueue", exp_mount);
        break;

    case RED_EXPORT_LOCK:
        ADD2(restate, "--lock-file", exp_mount);
        break;

    default:
        free(exp_mount);
        break;
    }
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

static int set_one_capability(redwrap_state_t *restate, const char *capability, int value)
{
    ADD2(restate, value ? "--cap-add" : "--cap-drop", capability);
    return 0;
}

static int set_capabilities(redwrap_state_t *restate, const redConfTagT *conftag) {

    int rc = 0;
    redConfCapT *cap, *end = &conftag->capabilities[conftag->capabilities_count];

    /* first pass for setting ALL */
    for(cap = conftag->capabilities ; rc == 0 && cap != end ; cap++) {
        if (strcasecmp(cap->cap, "ALL") == 0) {
            rc = set_one_capability(restate, cap->cap, cap->add);
            break;
        }
    }

    /* first pass for setting other values */
    for(cap = conftag->capabilities ; rc == 0 && cap != end ; cap++) {
        if (strcasecmp(cap->cap, "ALL") != 0)
            rc = set_one_capability(restate, cap->cap, cap->add);
    }
    return rc;
}

static void do_unshare(redwrap_state_t *restate, const char *setting, redConfSharingE all, const char *unshare, const char *share)
{
    redConfSharingE type = sharing_type(setting);

    switch (type == RED_CONF_SHARING_UNSET ? all : type) {
    default:
    case RED_CONF_SHARING_UNSET:
    case RED_CONF_SHARING_DISABLED:
        ADD(restate, unshare);
        break;
    case RED_CONF_SHARING_ENABLED:
        if (share != NULL)
            ADD(restate, share);
        break;
    case RED_CONF_SHARING_JOIN:
        break;
    }
}

/* this function tests if sharing of (item) is disabled  */
static bool can_unshare(redConfSharingE target, redConfSharingE all)
{
    return target == RED_CONF_SHARING_DISABLED
                || (target == RED_CONF_SHARING_UNSET && all != RED_CONF_SHARING_ENABLED);
}

static int set_shares(redwrap_state_t *restate, const redConfShareT *shares)
{
    redConfSharingE sall = sharing_type(shares->all);

    do_unshare(restate, shares->user, sall, "--unshare-user", NULL);
    do_unshare(restate, shares->cgroup, sall, "--unshare-cgroup", NULL);
    do_unshare(restate, shares->ipc, sall, "--unshare-ipc", NULL);
    do_unshare(restate, shares->pid, sall, "--unshare-pid", NULL);
    do_unshare(restate, shares->net, sall, "--unshare-net", "--share-net");

    restate->shares = *shares;

    return 0;
}

static int set_conftag(redwrap_state_t *restate, const redConfTagT *conftag)
{
    set_capabilities(restate, conftag);

    set_cgroups(restate->rednode, conftag->cgrouproot);

    // set global merged config tags
    if (conftag->hostname) {
        ADD(restate, "--unshare-uts");
        ADD2(restate, "--hostname", RedNodeStringExpand (restate->rednode, conftag->hostname));
    }

    if (conftag->chdir)
        ADD2(restate, "--chdir", RedNodeStringExpand (restate->rednode, conftag->chdir));

    if (conftag->diewithparent & RED_CONF_OPT_ENABLED)
        ADD(restate, "--die-with-parent");

    if (conftag->newsession & RED_CONF_OPT_ENABLED)
        ADD(restate, "--new-session");

    restate->map_user_root = conftag->maprootuser;

    return set_shares(restate, &conftag->share);
}

static int set_env_value(void *closure, const char *value, const char *name)
{
    if (value)
        ADD3((redwrap_state_t*)closure, "--setenv", name, strdup(value));
    return 0;
}

static const char *get_config_path(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.path;
}

static int set_env_path(void *closure, const char *value, size_t length)
{
    (void)length;
    return set_env_value(closure, value, "PATH");
}

static const char *get_config_ldpath(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.ldpath;
}

static int set_env_ldpath(void *closure, const char *value, size_t length)
{
    (void)length;
    return set_env_value(closure, value, "LD_LIBRARY_PATH");
}

static int set_special_confvar(redwrap_state_t *restate)
{
    int rc = mixPaths(restate->rednode, get_config_path, set_env_path, restate);
    return rc != 0 ? rc : mixPaths(restate->rednode, get_config_ldpath, set_env_ldpath, restate);
}

static int mixvar(void *closure, const redConfVarT *var, const redNodeT *node, unsigned index, unsigned count)
{
    (void)index;
    (void)count;
    set_one_envvar((redwrap_state_t*)closure, var, node);
    return 0;
}
static int mixexp(void *closure, const redConfExportPathT *exp, const redNodeT *node, unsigned index, unsigned count)
{
    (void)index;
    (void)count;
    set_one_export((redwrap_state_t*)closure, exp, node);
    return 0;
}
static int set_exports_and_vars(redwrap_state_t *restate)
{
    int rc = mixExports(restate->rednode, mixexp, restate);
    if (rc == 0)
        rc = mixVariables(restate->rednode, mixvar, restate);
    return rc;
}

static int mix_conftag(void *closure, const redConfTagT *conftag)
{
    return set_conftag((redwrap_state_t*)closure, conftag);
}

static int set_for_conftag(redwrap_state_t *restate)
{
    return mixConfTags(restate->rednode, mix_conftag, restate);
}

static void joinns(const char *filens, int nstype)
{
    int fd = open(filens, O_RDONLY);
    if (fd < 0) {
        RedLog(REDLOG_ERROR, "Failed to open %s: %s", filens, strerror(errno));
        exit(1);
    }
    else {
        int rc = setns(fd, nstype);
        if (rc < 0) {
            RedLog(REDLOG_ERROR, "Failed to join %s: %s", filens, strerror(errno));
            exit(1);
        }
        close(fd);
    }
}

int redwrapExecBwrap (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {
    redwrap_state_t restate;
    int idx, error, unshareusr;
    int pipe_fd[2];
    pid_t pid;
    ssize_t ssz;

    if (cliarg->verbose)
        SetLogLevel(cliarg->verbose);

    // start argument list with red-wrap command name
    restate.cliarg = cliarg;
    restate.rednode = NULL;
    memset(&restate.shares, 0, sizeof restate.shares);
    restate.map_user_root = 0;
    restate.argcount = 1;
    restate.argval[0] = command_name;

    /* get the rednode */
    restate.rednode = RedNodesScan(cliarg->redpath, cliarg->isadmin, cliarg->verbose);
    if (!restate.rednode) {
        RedLog(REDLOG_ERROR, "Fail to scan rednodes family tree redpath=%s", cliarg->redpath);
        goto OnErrorExit;
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
    for (idx = 0; idx < subargc; idx++ )
        ADD(&restate, subargv[idx]);

    /* dump the call to bwrap */
    if (cliarg->dump) {
        FILE *fout = cliarg->dump > 1 ? stdout : stderr;
        fprintf(fout, "DUMP: %s (as %s)", cliarg->bwrap, restate.argval[0]);
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

    /* clone now */
    unshareusr = can_unshare(sharing_type(restate.shares.user),  sharing_type(restate.shares.all));
    if (!unshareusr)
        pid = (pid_t) syscall (__NR_clone, SIGCHLD, NULL);
    else {
        /* setup a connexion from parent to child */
        if (pipe(pipe_fd) == -1) {
            RedLog(REDLOG_ERROR, "Cannot pipe error=%s", strerror(errno));
            goto OnErrorExit;
        }
        /* enter new user namespace now */
        pid = (pid_t) syscall (__NR_clone, CLONE_NEWUSER|SIGCHLD, NULL);
    }

    /* enter new user namespace now */
    if (pid < 0) {
        RedLog(REDLOG_ERROR, "Cannot clone error=%s", strerror(errno));
        goto OnErrorExit;
    }
    if (pid == 0) {
        /* in forked process */
        /* wait for parent to set uid/gid maps */
        if (unshareusr) {
            close(pipe_fd[1]);
            ssz = read(pipe_fd[0], &pid, sizeof(pid));
            close(pipe_fd[0]);
            if (ssz < 0) {
                RedLog(REDLOG_ERROR, "failed to receive sync pid: %s", strerror(errno));
                return EXIT_FAILURE;
            }
        }

        /* unshare time namespace if required */
        if (sharing_type(restate.shares.time) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.time, CLONE_NEWTIME);
        else if (can_unshare(sharing_type(restate.shares.time), sharing_type(restate.shares.all)))
            unshare(CLONE_NEWTIME);

        /* join other namespaces */
        if (sharing_type(restate.shares.user) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.user, CLONE_NEWUSER);
        if (sharing_type(restate.shares.cgroup) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.cgroup, CLONE_NEWCGROUP);
        if (sharing_type(restate.shares.ipc) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.ipc, CLONE_NEWIPC);
        if (sharing_type(restate.shares.net) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.net, CLONE_NEWNET);
        if (sharing_type(restate.shares.pid) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.pid, CLONE_NEWPID);

        /* exec bwrap now */
        restate.argval[restate.argcount] = NULL;
        if(execve(cliarg->bwrap, (char**) restate.argval, NULL)) {
            RedLog(REDLOG_ERROR, "bwrap command issue: %s", strerror(errno));
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* in parent process */
    if (unshareusr) {
        /* set uid/gid maps and signal child */
        close(pipe_fd[0]);
        setuidgidmap(pid, restate.map_user_root);
        ssz = write(pipe_fd[1], &pid, sizeof(pid));
        close(pipe_fd[1]);
        if (ssz < 0) {
            RedLog(REDLOG_ERROR, "failed to send sync pid: %s", strerror(errno));
            kill(pid, SIGKILL);
            goto OnErrorExit;
        }
    }

    /* wait child completion */
    waitpid(pid, &error, 0);
    return error;

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    return 1;
}
