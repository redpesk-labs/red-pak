/*
* Copyright (C) 2020-2025 IoT.bzh Company
* Author: Fulup Ar Foll <fulup@iot.bzh>
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
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

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

#ifndef BWRAP_HAS_CLEARENV
#define BWRAP_HAS_CLEARENV 0
#endif

extern char **environ;

/** redwrap exec state */
typedef
struct redwrap_state_s
{
    /** redwrap input arguments */
    rWrapConfigT *cliarg;
    /** the rednode */
    redNodeT     *rednode;
    /** shortcut to earlyconf */
    early_conf_t *earlyconf;
    /** copy of the shares */
    redConfShareT shares;
#if !BWRAP_HAS_CLEARENV
    /** for environment */
    char        **environ;
#endif
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
 * check if the @ref node and its parents are valid
 * according to function @ref RwrapValidateNode
 * Returns zero when the rednode is valid or return a
 * negative value when the node is not valid
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

/*
 * Process one environment variable specification
 * Add the bubble wrap setting that matches the requirement
 */
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
        value = secure_getenv(key);
        ADD3(restate, "--setenv", key, value != NULL ? value : "");
        break;

    default:
        break;
    }
}

/*
 * Process one exportation specification
 * When a file is exported, check that it exists
 * When a directory is exported check if it shoukd be created
 */
static void set_one_export(redwrap_state_t *restate, const redConfExportPathT *export, const redNodeT *node)
{
    char buffer[2049];

    // scan export directory
    redExportFlagE mode = export->mode;
    const char* mount = export->mount;
    const char* path = export->path;
    struct stat status;
    const char * expandpath = NULL;
    char *exp_mount;

    // if mouting path is not privide let's duplicate mount
    if (!path)
        path=mount;

    // if private and not last leaf: ignore
    if ((mode & RED_EXPORT_PRIVATES) != 0 && node != restate->rednode)
        return;

    /* get exported path */
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

/* create the file of given path with the given content */
static int create_file(const char *path, const char *content) {
    int rc;
    size_t len;
    ssize_t ssz;

    /* create the file entry */
    rc = open(path, O_WRONLY);
    if (rc < 0)
        RedLog(REDLOG_ERROR, "Issue open path=%s error=%s", path, strerror(errno));
    else {
        /* write the content to the file */
        len = strlen(content);
        ssz = write(rc, content, len);
        close(rc);
        if (ssz == (ssize_t)len)
            rc = 0;
        else {
            RedLog(REDLOG_ERROR, "Issue writing path=%s error=%s", path, strerror(errno));
            rc = -1;
        }
    }
    return rc;
}

/* create the mapping of uids and gids */
static int write_uid_gid_map(pid_t pid,
                             uid_t inner_uid, uid_t outer_uid,
                             gid_t inner_gid, gid_t outer_gid
) {
    const int BUF_SIZE = 100;
    char path[BUF_SIZE];
    char content[BUF_SIZE];
    int lenpref, rc;

    /* get current pid if needed */
    if (pid <= 0)
        pid = getpid();
    lenpref = snprintf(path, sizeof path, "/proc/%lld/", (long long)pid);

    /* set uid map */
    strncpy(&path[lenpref], "uid_map", sizeof path - lenpref);
    snprintf(content, sizeof content, "%lld %lld 1", (long long)inner_uid, (long long)outer_uid);
    rc = create_file(path, content);
    if (rc < 0)
        return rc;

    /* setgroups to deny */
    strncpy(&path[lenpref], "setgroups", sizeof path - lenpref);
    snprintf(content, sizeof content, "deny");
    rc = create_file(path, content);
    if (rc < 0)
        return rc;

    /* set gid map */
    strncpy(&path[lenpref], "gid_map", sizeof path - lenpref);
    snprintf(content, sizeof content, "%lld %lld 1", (long long)inner_gid, (long long)outer_gid);
    rc = create_file(path, content);
    if (rc < 0)
        return rc;

    return 0;
}

/* add bwrap arguments for setting or dropping a capability */
static int set_one_capability(redwrap_state_t *restate, const char *capability, int value)
{
    ADD2(restate, value ? "--cap-add" : "--cap-drop", capability);
    return 0;
}

/* process setting/dropping of capabilities */
static int set_capabilities(redwrap_state_t *restate, const redConfTagT *conftag) {

    int rc = 0;
    redConfCapT *cap, *end = &conftag->capabilities[conftag->capabilities_count];

    /* first pass for setting ALL */
    for(cap = conftag->capabilities ; rc == 0 && cap != end ; cap++) {
        if (strcasecmp(cap->cap, "ALL") == 0) {
            rc = set_one_capability(restate, "ALL", cap->add);
            break;
        }
    }

    /* second pass for setting other values */
    for(cap = conftag->capabilities ; rc == 0 && cap != end ; cap++) {
        if (strcasecmp(cap->cap, "ALL") != 0)
            rc = set_one_capability(restate, cap->cap, cap->add);
    }
    return rc;
}

/* add bwrap arguments for sharing or unsharing a namespace */
static void do_unshare(
                    redwrap_state_t *restate,
                    const char *setting,
                    redConfSharingE all,
                    const char *unshare,
                    const char *share
) {
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
static bool should_unshare(redConfSharingE target, redConfSharingE all)
{
    return target == RED_CONF_SHARING_DISABLED
                || (target == RED_CONF_SHARING_UNSET && all != RED_CONF_SHARING_ENABLED);
}

/* this function tests if sharing of (item) is disabled  */
static bool should_unshare_time(redConfSharingE time)
{
    return time == RED_CONF_SHARING_DISABLED || time == RED_CONF_SHARING_JOIN;
}

/* process the sharing or unsharing */
static int set_shares(redwrap_state_t *restate, const redConfShareT *shares)
{
    redConfSharingE sall = sharing_type(shares->all);

    if (!should_unshare_time(sharing_type(shares->time)))
        do_unshare(restate, shares->user, sall, "--unshare-user", NULL);
    do_unshare(restate, shares->cgroup, sall, "--unshare-cgroup", NULL);
    do_unshare(restate, shares->ipc, sall, "--unshare-ipc", NULL);
    do_unshare(restate, shares->pid, sall, "--unshare-pid", NULL);
    do_unshare(restate, shares->net, sall, "--unshare-net", "--share-net");

    restate->shares = *shares;

    return 0;
}

/* set bwrap arguments for given conftag */
static int set_conftag(redwrap_state_t *restate, const redConfTagT *conftag)
{
    /* set capabilities */
    set_capabilities(restate, conftag);

    /* set cgroups */
    set_cgroups(restate->rednode, conftag->cgrouproot);

    /* set global merged config tags */
    if (conftag->hostname) {
        ADD(restate, "--unshare-uts");
        ADD2(restate, "--hostname", RedNodeStringExpand (restate->rednode, conftag->hostname));
    }

    /* set the working directory */
    if (conftag->chdir)
        ADD2(restate, "--chdir", RedNodeStringExpand (restate->rednode, conftag->chdir));

    /* set if dying with parent */
    if (conftag->diewithparent & RED_CONF_OPT_ENABLED)
        ADD(restate, "--die-with-parent");

    /* set if make a new session */
    if (conftag->newsession & RED_CONF_OPT_ENABLED)
        ADD(restate, "--new-session");

    /* clear the environment unless kept */
#if BWRAP_HAS_CLEARENV
    if (!conftag->inheritenv)
        ADD(restate, "--clearenv");
#else
    if (conftag->inheritenv)
        restate->environ = environ;
#endif

    /* copy in state if mapping user to root */
    restate->map_user_root = conftag->maprootuser;

    /* process the sharing or unsharing */
    return set_shares(restate, &conftag->share);
}

/* Add bwrap argument for setting the environment variable of name with value */
static int set_env_value(void *closure, const char *value, const char *name)
{
    if (value)
        ADD3((redwrap_state_t*)closure, "--setenv", name, strdup(value));
    return 0;
}

/* get the PATH */
static const char *get_config_path(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.path;
}

/* set the PATH */
static int set_env_path(void *closure, const char *value, size_t length)
{
    (void)length;
    return set_env_value(closure, value, "PATH");
}

/* get the LD_LIBRARY_PATH */
static const char *get_config_ldpath(void *closure, const redConfigT *config)
{
    (void)closure;
    return config->conftag.ldpath;
}

/* set the LD_LIBRARY_PATH */
static int set_env_ldpath(void *closure, const char *value, size_t length)
{
    (void)length;
    return set_env_value(closure, value, "LD_LIBRARY_PATH");
}

/* set the special environment variable PATH and LD_LIBRARY_PATH */
static int set_special_confvar(redwrap_state_t *restate)
{
    int rc = mixPaths(restate->rednode, get_config_path, set_env_path, restate);
    return rc != 0 ? rc : mixPaths(restate->rednode, get_config_ldpath, set_env_ldpath, restate);
}

/* callback for setting environment variables */
static int mixvar_cb(void *closure, const redConfVarT *var, const redNodeT *node, unsigned index, unsigned count)
{
    (void)index;
    (void)count;
    set_one_envvar((redwrap_state_t*)closure, var, node);
    return 0;
}

/* callback for setting exports */
static int mixexp_cb(void *closure, const redConfExportPathT *exp, const redNodeT *node, unsigned index, unsigned count)
{
    (void)index;
    (void)count;
    set_one_export((redwrap_state_t*)closure, exp, node);
    return 0;
}

/* process zxports and environment variables */
static int set_exports_and_vars(redwrap_state_t *restate)
{
    int rc = mixExports(restate->rednode, mixexp_cb, restate);
    if (rc == 0)
        rc = mixVariables(restate->rednode, mixvar_cb, restate);
    return rc;
}

/* callback for setting conftags */
static int mixconftag_cb(void *closure, const redConfTagT *conftag)
{
    return set_conftag((redwrap_state_t*)closure, conftag);
}

/* process conftags */
static int set_for_conftag(redwrap_state_t *restate)
{
    return mixConfTags(restate->rednode, mixconftag_cb, restate);
}

/* join the namespace of nstype  referenced by file ns */
static void joinns(const char *filens, int nstype)
{
    /* open the file */
    int fd = open(filens, O_RDONLY);
    if (fd < 0) {
        RedLog(REDLOG_ERROR, "Failed to open %s: %s", filens, strerror(errno));
        exit(1);
    }
    else {
        /* use the file for setting the namespace */
        int rc = setns(fd, nstype);
        if (rc < 0) {
            RedLog(REDLOG_ERROR, "Failed to join %s: %s", filens, strerror(errno));
            exit(1);
        }
        close(fd);
    }
}

/* test if str encodes a positive integral number */
static int isnum(const char *str)
{
    do {
        if (*str < '0' || *str > '9')
            return 0;
    }
    while(*++str);
    return 1;
}

/* returns a fresh allocated string encoding the given value */
static char *num2str(long value)
{
    char buffer[100], *res;
    unsigned len, beg = (unsigned)sizeof buffer;
    if (value < 0)
        return NULL;
    do {
        ldiv_t x = ldiv(value, 10);
        buffer[--beg] = (char)('0' + (char)x.rem);
        value = x.quot;
    }
    while (value != 0);
    len = (unsigned)sizeof buffer - beg;
    res = malloc(len + 1);
    if (res != NULL) {
        memcpy(res, &buffer[beg], len);
        res[len] = 0;
    }
    return res;
}

/* get the numerical value of the user */
static int get_final_uid(const char **dest, const char *user)
{
    if (user == NULL) {
        *dest = NULL;
        return 0;
    }
    if (isnum(user))
        *dest = strdup(user);
    else {
        struct passwd *p = getpwnam(user);
        *dest = p != NULL ? num2str(p->pw_uid) : NULL;
    }
    return *dest == NULL;
}

/* get the numerical value of the group */
static int get_final_gid(const char **dest, const char *group)
{
    if (group == NULL) {
        *dest = NULL;
        return 0;
    }
    if (isnum(group))
        *dest = strdup(group);
    else {
        struct group *g = getgrnam(group);
        *dest = g != NULL ? num2str(g->gr_gid) : NULL;
    }
    return *dest == NULL;
}

/* get copy of smack label */
static int get_final_smk(const char **dest, const char *smk)
{
    if (smk == NULL) {
        *dest = NULL;
        return 0;
    }
    *dest = strdup(smk);
    return *dest == NULL;
}

/* callback of early config */
static int earlymix(void *closure, const early_conf_t *conf, const redNodeT *node)
{
    redwrap_state_t *restate = closure;
    const rWrapConfigT *cliarg = restate->cliarg;
    early_conf_t *final = &restate->rednode->leaf->earlyconf;

    (void)node;

    return get_final_uid(&final->setuser, cliarg->setuser != NULL ? cliarg->setuser : conf->setuser)
        || get_final_gid(&final->setgroup, cliarg->setgroup != NULL ? cliarg->setgroup : conf->setgroup)
        || get_final_smk(&final->smack, cliarg->smack != NULL ? cliarg->smack : conf->smack);
}

/* compute the early config */
static int set_early_conf(redwrap_state_t *restate)
{
    return mixEarlyConf(restate->rednode, earlymix, restate);
}

/* set smack label of current process */
static void setsmack(const char *label)
{
    size_t len = strlen(label);
    int fd = open("/proc/self/attr/current", O_WRONLY);
    if (fd >= 0) {
            ssize_t wrl = write(fd, label, len);
            close(fd);
            if (wrl == (ssize_t)len)
                return;
    }
    RedLog(REDLOG_ERROR, "not able to switch to SMACK %s: %s", label, strerror(errno));
    exit(EXIT_FAILURE);
}

/*
 * main processing of redwrap
 */
int redwrapExecBwrap (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {
    redwrap_state_t restate;
    int idx, error, unshareusr, cloflags;
    pid_t pid;
    redConfSharingE sall, stim;
    uid_t uid_to_map, uid, cur_uid;
    gid_t gid_to_map, gid, cur_gid;

    /* set verbosity */
    if (cliarg->verbose)
        SetLogLevel(cliarg->verbose);

    /* start argument list with red-wrap command name */
    memset(&restate, 0, sizeof restate);
    restate.cliarg = cliarg;
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

    /* setup from early config */
    error = set_early_conf(&restate);
    if (error)
        goto OnErrorExit;
    restate.earlyconf = &restate.rednode->leaf->earlyconf;

    /* compute uid and gid */
    cur_uid = getuid();
    cur_gid = getgid();
    uid = restate.earlyconf->setuser == NULL
                ? cur_uid : (uid_t)atoi(restate.earlyconf->setuser);
    gid = restate.earlyconf->setgroup == NULL
                ? cur_gid : (gid_t)atoi(restate.earlyconf->setgroup);

    /* set exports and vars */
    error = set_exports_and_vars(&restate);
    if (error)
        goto OnErrorExit;

    /* set config */
    error = set_for_conftag(&restate);
    if (error)
        goto OnErrorExit;

    /* add LD_PATH_LIBRARY & PATH */
    error = set_special_confvar(&restate);
    if (error)
        goto OnErrorExit;

    /* add program to execute at tail of arguments */
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
    cloflags = SIGCHLD;
    sall = sharing_type(restate.shares.all);
    stim = sharing_type(restate.shares.time);
    unshareusr = should_unshare_time(stim)
      && should_unshare(sharing_type(restate.shares.user), sall);
    if (unshareusr)
        cloflags |= CLONE_NEWUSER;
    pid = (pid_t) syscall (__NR_clone, cloflags, NULL);

    /* enter new user namespace now */
    if (pid < 0) {
        RedLog(REDLOG_ERROR, "Cannot clone error=%s", strerror(errno));
        goto OnErrorExit;
    }
    if (pid == 0) {
        /* in forked process */
        /* wait for parent to set uid/gid maps */
        if (unshareusr) {
            uid_to_map = restate.map_user_root ? 0 : uid;
            gid_to_map = restate.map_user_root ? 0 : gid;
            error = write_uid_gid_map(0, uid_to_map, uid, gid_to_map, gid);
            if (error < 0)
                return EXIT_FAILURE;
        }

        /* unshare time namespace if required */
        if (stim == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.time, CLONE_NEWTIME);
        else if (stim == RED_CONF_SHARING_DISABLED)
            unshare(CLONE_NEWTIME);

        /* join other namespaces */
        if (sharing_type(restate.shares.cgroup) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.cgroup, CLONE_NEWCGROUP);
        if (sharing_type(restate.shares.ipc) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.ipc, CLONE_NEWIPC);
        if (sharing_type(restate.shares.net) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.net, CLONE_NEWNET);
        if (sharing_type(restate.shares.pid) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.pid, CLONE_NEWPID);
        if (sharing_type(restate.shares.user) == RED_CONF_SHARING_JOIN)
            joinns(restate.shares.user, CLONE_NEWUSER);

        /* set smack label */
        if (restate.earlyconf->smack != NULL)
            setsmack(restate.earlyconf->smack);

        /* set new gid */
        if (gid != cur_gid && setgid(gid) != 0) {
            RedLog(REDLOG_ERROR, "not able to switch to group %d: %s", (int)gid, strerror(errno));
            return EXIT_FAILURE;
        }
        /* set new uid */
        if (uid != cur_uid && setuid(uid) != 0) {
            RedLog(REDLOG_ERROR, "not able to switch to user %d: %s", (int)uid, strerror(errno));
            return EXIT_FAILURE;
        }

        /* exec bwrap now */
        restate.argval[restate.argcount] = NULL;
#if BWRAP_HAS_CLEARENV
        if(execv(cliarg->bwrap, (char**) restate.argval)) {
#else
        if(execve(cliarg->bwrap, (char**) restate.argval, restate.environ)) {
#endif
            RedLog(REDLOG_ERROR, "bwrap command issue: %s", strerror(errno));
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* in parent process */
    /* wait child completion */
    waitpid(pid, &error, 0);
    return error;

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    return 1;
}
