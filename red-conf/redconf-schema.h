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
* Documentation: https://github.com/tlsa/libcyaml/blob/master/docs/guide.md
*/

#ifndef _RED_CONF_SCHEMA_INCLUDE_
#define _RED_CONF_SCHEMA_INCLUDE_

#include <unistd.h>
#include <sys/types.h>
#include <cyaml/cyaml.h>

#include <urcu/rculfhash.h>    /* RCU Lock-free hash table */

// ---- RedConfig Schema for ${redpath}/etc/redpack.yaml ----
    typedef struct {
        const char *alias;
        const char *name;
        const char *info;
    } redConfHeaderT;

    typedef struct {
        uid_t uid;
        gid_t gid_t;
        mode_t umask;
    } redConfAclT;

    typedef struct {
        const char *max;
        const char *high;
        const char *min;
        const char *low;
        const char *oom_group;
        const char *swap_high;
        const char *swap_max;
    } redMemT;

    typedef struct {
        const char *weight;
        const char *max;
        const char *weight_nice;
    } redCpuT;

    typedef struct {
        const char *cost_qos;
        const char *cost_model;
        const char *weight;
        const char *max;
    } redIoT;

    typedef struct {
        const char *max;
    } redPidsT;

    typedef struct {
        const char *cpus;
        const char *mems;
        const char *cpus_partition;
    } redCpusetT;

    typedef struct {
        redCpusetT *cpuset;
        redMemT *mem;
        redCpuT *cpu;
        redIoT *io;
        redPidsT *pids;
    } redConfCgroupT;

    typedef enum {
        RED_EXPORT_PRIVATE              = 1 << 0,
        RED_EXPORT_RESTRICTED           = 1 << 1,
        RED_EXPORT_PRIVATE_RESTRICTED   = 1 << 2,
        RED_EXPORT_PUBLIC               = 1 << 3,
        RED_EXPORT_PRIVATE_FILE         = 1 << 4,
        RED_EXPORT_RESTRICTED_FILE      = 1 << 5,
        RED_EXPORT_PUBLIC_FILE          = 1 << 6,
        RED_EXPORT_ANONYMOUS            = 1 << 7,
        RED_EXPORT_SYMLINK              = 1 << 8,
        RED_EXPORT_EXECFD               = 1 << 9,
        RED_EXPORT_DEFLT                = 1 << 10,
        RED_EXPORT_DEVFS                = 1 << 11,
        RED_EXPORT_TMPFS                = 1 << 12,
        RED_EXPORT_MQUEFS               = 1 << 13,
        RED_EXPORT_PROCFS               = 1 << 14,
        RED_EXPORT_LOCK                 = 1 << 15,
    } redExportFlagE;
    extern const cyaml_strval_t exportFlagStrings[];

    static const unsigned int RED_EXPORT_PRIVATES = RED_EXPORT_PRIVATE | RED_EXPORT_PRIVATE_FILE;
    static const unsigned int RED_EXPORT_FILES = RED_EXPORT_PRIVATE_FILE | RED_EXPORT_RESTRICTED_FILE | RED_EXPORT_PUBLIC_FILE;
    static const unsigned int RED_EXPORT_DIRS = RED_EXPORT_PRIVATE | RED_EXPORT_RESTRICTED | RED_EXPORT_PUBLIC;

    typedef enum {
        RED_CONFVAR_DEFLT=0,
        RED_CONFVAR_STATIC,
        RED_CONFVAR_EXECFD,
        RED_CONFVAR_REMOVE,
    } redVarEnvFlagE;
    extern const cyaml_strval_t redVarEnvStrings[];

    typedef struct {
        const char *mount;
        const char *path;
        redExportFlagE mode;
        const char *info;
        const char *warn;
    } redConfExportPathT;

    typedef struct {
        const char *old;
        const char *tnew;
    } redConfRelocationT;

    typedef struct {
        redVarEnvFlagE mode;
        const char *key;
        const char *value;
        const char *info;
        const char *warn;
    } redConfVarT;

    typedef enum {
       RED_CONF_OPT_UNSET = 0,
       RED_CONF_OPT_DISABLED = 1,
       RED_CONF_OPT_ENABLED = 2,
    } redConfOptFlagE;
    extern const cyaml_strval_t redConfOptStrings[];

    typedef struct {
        const char *cap;
        int add;
        const char *info;
        const char *warn;
    } redConfCapT;

    typedef struct {
        const char *rpmdir;
        const char *persistdir;
        const char *cachedir;
        const char *path;
        const char *ldpath;
        const char *hostname;
        const char *umask;
        const char *chdir;
        const char *cgrouproot;
        unsigned int gpgcheck;
        unsigned int inherit;
        unsigned int unsafe;
        unsigned int maxage;
        unsigned int maprootuser;
        redConfOptFlagE diewithparent;
        redConfOptFlagE newsession;
        redConfOptFlagE share_all;
        redConfOptFlagE share_user;
        redConfOptFlagE share_cgroup;
        redConfOptFlagE share_ipc;
        redConfOptFlagE share_pid;
        redConfOptFlagE share_net;
        redConfOptFlagE share_time;
        redConfCgroupT *cgroups;
        redConfCapT *capabilities;
        int capabilities_count;
        int verbose;
    } redConfTagT;

    typedef struct {
        redConfAclT *acl;
        redConfHeaderT *headers;
        redConfTagT  *conftag;
        redConfVarT *confvar;
        unsigned int confvar_count;
        redConfExportPathT *exports;
        unsigned int exports_count;
        redConfRelocationT *relocations;
        unsigned int relocations_count;
    } redConfigT;


// ---- RedStatus Schema for ${redpath}/.status
    typedef enum {
        RED_STATUS_DISABLE=0,
        RED_STATUS_ENABLE,
        RED_STATUS_UNKNOWN,
        RED_STATUS_ERROR,
    } redStatusFlagE;
    extern const cyaml_strval_t statusFlagStrings[];

    typedef struct {
        redStatusFlagE state;
        const char *realpath;
        unsigned long timestamp;
        const char *info;
    } redStatusT;

// ---- Family tree structure ----
typedef enum {
    RED_NODE_CONFIG_OK,
    RED_NODE_CONFIG_FX,
    RED_NODE_CONFIG_MISSING,
}  redNodeYamlE;

typedef struct redChildNodeS{
    struct redNodeS* child;
    struct redChildNodeS* brother;
} redChildNodeT;

typedef struct {
    char *leafalias;
    char *leafname;
    char *leafpath;
} redEnvValT;

// --- Redpath Node Directory Hierarchy (from leaf to root)
typedef struct redNodeS{
    redStatusT *status;
    redConfigT *config;
    redConfigT *confadmin;
    struct redNodeS *ancestor;
    redChildNodeT *childs;
    const char *redpath;
    redEnvValT env;
} redNodeT;

// ---- Special confvar
typedef struct {
    char *ldpathString;
    unsigned int ldpathIdx;
    char *pathString;
    unsigned int pathIdx;

} dataNodeT;

redStatusT* RedLoadStatus (const char* filepath, int warning);
redConfigT* RedLoadConfig (const char* filepath, int warning);
int RedSaveStatus (const char* filepath, redStatusT *status, int warning);
int RedSaveConfig (const char* filepath, redConfigT *config, int warning);

int RedGetConfig(char **output, size_t *len, redConfigT *config);

int RedFreeConfig(redConfigT *config, int wlevel);
int RedFreeStatus(redStatusT *status, int wlevel);

int setLogYaml(int level);

#endif // _REDCONFIG_INCLUDE_
