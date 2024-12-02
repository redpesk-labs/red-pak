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

/* this function tests if sharing of (item) is disabled  */
static bool can_unshare(redConfOptFlagE target, redConfOptFlagE all)
{
    return target == RED_CONF_OPT_DISABLED
                || (target == RED_CONF_OPT_UNSET && all != RED_CONF_OPT_ENABLED);
}

static int set_shares(redwrap_state_t *restate, const redConfShareT *shares)
{
    if (can_unshare(shares->user, shares->all))
        ADD(restate, "--unshare-user");

    if (can_unshare(shares->cgroup, shares->all))
        ADD(restate, "--unshare-cgroup");

    if (can_unshare(shares->ipc, shares->all))
        ADD(restate, "--unshare-ipc");

    if (can_unshare(shares->pid, shares->all))
        ADD(restate, "--unshare-pid");

    if (can_unshare(shares->net, shares->all))
        ADD(restate, "--unshare-net");
    else
        ADD(restate, "--share-net");

    restate->unshare_time = can_unshare(shares->time, shares->all);

    return 0;
}

static int set_conftag(redwrap_state_t *restate, const redConfTagT *conftag)
{
    set_capabilities(restate, conftag);

    set_cgroups(restate->rednode, conftag->cgrouproot);

    // set global merged config tags
    if (conftag->hostname)
        ADD3(restate, "--unshare-uts", "--hostname", RedNodeStringExpand (restate->rednode, conftag->hostname));

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

int redwrapExecBwrap (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {
    redwrap_state_t restate;
    int idx, error;
    int pipe_fd[2];
    pid_t pid;
    ssize_t ssz;

    if (cliarg->verbose)
        SetLogLevel(cliarg->verbose);

    // start argument list with red-wrap command name
    restate.cliarg = cliarg;
    restate.rednode = NULL;
    restate.unshare_time = 0;
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
        ssz = read(pipe_fd[0], &pid, sizeof(pid));
        close(pipe_fd[0]);
        if (ssz < 0) {
            RedLog(REDLOG_ERROR, "failed to receive sync pid: %s", strerror(errno));
            return EXIT_FAILURE;
        }

        /* unshare time nanespace if required */
        if (restate.unshare_time)
            unshare(CLONE_NEWTIME);

        /* exec bwrap now */
        restate.argval[restate.argcount] = NULL;
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
    ssz = write(pipe_fd[1], &pid, sizeof(pid));
    close(pipe_fd[1]);
    if (ssz < 0) {
        RedLog(REDLOG_ERROR, "failed to send sync pid: %s", strerror(errno));
        kill(pid, SIGKILL);
        goto OnErrorExit;
    }

    /* wait child completion */
    waitpid(pid, &error, 0);
    return error;

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    return 1;
}
