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
* Documentation: https://github.com/tlsa/libcyaml/blob/master/docs/guide.md
*/

#ifndef _RED_CONF_SCHEMA_INCLUDE_
#define _RED_CONF_SCHEMA_INCLUDE_

#include <stddef.h>

// ---- RedConfig Schema for configs ----

    /** structure for header of a rednode config */
    typedef struct {
        /** alias */
        const char *alias;
        /** name (an UUID) */
        const char *name;
        /** some info about the node */
        const char *info;
    }
    redConfHeaderT;

    /** structure for memory's cgroup settings */
    typedef struct {
        /** setting for cgroup-v2 memory.max */
        const char *max;
        /** setting for cgroup-v2 memory.high */
        const char *high;
        /** setting for cgroup-v2 memory.min */
        const char *min;
        /** setting for cgroup-v2 memory.low */
        const char *low;
        /** setting for cgroup-v2 memory.oom.group */
        const char *oom_group;
        /** setting for cgroup-v2 memory.swap.high */
        const char *swap_high;
        /** setting for cgroup-v2 memory.swap.max */
        const char *swap_max;
    }
    redMemT;

    /** structure for CPU's cgroup settings */
    typedef struct {
        /** setting for cgroup-v2 cpu.weight */
        const char *weight;
        /** setting for cgroup-v2 cpu.max */
        const char *max;
        /** setting for cgroup-v2 cpu.weight.nice */
        const char *weight_nice;
    }
    redCpuT;

    /** structure for IO's cgroup settings */
    typedef struct {
        /** setting for cgroup-v2 io.cost.qos */
        const char *cost_qos;
        /** setting for cgroup-v2 io.cost.model */
        const char *cost_model;
        /** setting for cgroup-v2 io.weight */
        const char *weight;
        /** setting for cgroup-v2 io.max */
        const char *max;
    }
    redIoT;

    /** structure for PID's cgroup settings */
    typedef struct {
        /** setting for cgroup-v2 pids.max */
        const char *max;
    }
    redPidsT;

    /** structure for cpuset's cgroup settings */
    typedef struct {
        /** setting for cgroup-v2 cpuset.cpus */
        const char *cpus;
        /** setting for cgroup-v2 cpuset.mems */
        const char *mems;
        /** setting for cgroup-v2 cpuset.cpus.partition */
        const char *cpus_partition;
    }
    redCpusetT;

    /** structure for holding cgroup settings */
    typedef struct {
        /** settings for cpuset */
        redCpusetT *cpuset;
        /** settings for memory */
        redMemT *mem;
        /** settings for CPU */
        redCpuT *cpu;
        /** settings for IO */
        redIoT *io;
        /** settings for PID */
        redPidsT *pids;
    }
    redConfCgroupT;

    /** definition of export modes as OR-able bits */
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
        RED_EXPORT_INTERNAL             = 1 << 10,
        RED_EXPORT_DEVFS                = 1 << 11,
        RED_EXPORT_TMPFS                = 1 << 12,
        RED_EXPORT_MQUEFS               = 1 << 13,
        RED_EXPORT_PROCFS               = 1 << 14,
        RED_EXPORT_LOCK                 = 1 << 15,
    }
    redExportFlagE;

    /** definition for querying if a mode is private */
    static const unsigned int RED_EXPORT_PRIVATES = RED_EXPORT_PRIVATE | RED_EXPORT_PRIVATE_FILE;

    /** definition for querying if a mode is for file */
    static const unsigned int RED_EXPORT_FILES = RED_EXPORT_PRIVATE_FILE | RED_EXPORT_RESTRICTED_FILE | RED_EXPORT_PUBLIC_FILE;

    /** definition for querying if a mode is for directory */
    static const unsigned int RED_EXPORT_DIRS = RED_EXPORT_PRIVATE | RED_EXPORT_RESTRICTED | RED_EXPORT_PUBLIC;

    /** structure describing a mounting */
    typedef struct {
        /** destination path */
        const char *mount;
        /** source path */
        const char *path;
        /** mode */
        redExportFlagE mode;
        /** documentation text */
        const char *info;
        /** warning text to be prompted when overwrite is attempted */
        const char *warn;
    }
    redConfExportPathT;

    /** definition of variable modes */
    typedef enum {
        /** default mode: set value after expansion */
        RED_CONFVAR_DEFLT=0,
        /** static mode: set value without expansion */
        RED_CONFVAR_STATIC,
        /** exec mode: set value with result of execution of value as shell command */
        RED_CONFVAR_EXECFD,
        /** remove mode: unset the variable */
        RED_CONFVAR_REMOVE,
        /** inherit mode: set variable from environment */
        RED_CONFVAR_INHERIT,
    }
    redVarEnvFlagE;

    /** structure describing a varible setting */
    typedef struct {
        /** setting mode of the variable */
        redVarEnvFlagE mode;
        /** name of the variable */
        const char *key;
        /** value of the variable, meaning depends of the mode */
        const char *value;
        /** some info about the variable */
        const char *info;
        /** warning message on overwriting */
        const char *warn;
    }
    redConfVarT;

    /** tri-state value of options */
    typedef enum {
        /** the option is not set */
        RED_CONF_OPT_UNSET = 0,
        /** the option is set to disabled */
        RED_CONF_OPT_DISABLED = 1,
        /** the option is set to enabled */
        RED_CONF_OPT_ENABLED = 2,
    }
    redConfOptFlagE;

    /** structure describing capability setting */
    typedef struct {
        /** name of the capability */
        const char *cap;
        /** value */
        int add;
        /** some info about the  capability */
        const char *info;
        /** warning message on overwriting */
        const char *warn;
    }
    redConfCapT;

    /** structure holding sharing values */
    typedef struct {
        /** if enabled/disabled share/unshare every namespace by default */
        const char *all;
        /** if enabled/disabled share/unshare user namespace */
        const char *user;
        /** if enabled/disabled share/unshare cgroup namespace */
        const char *cgroup;
        /** if enabled/disabled share/unshare ipc namespace */
        const char *ipc;
        /** if enabled/disabled share/unshare pid namespace */
        const char *pid;
        /** if enabled/disabled share/unshare net namespace */
        const char *net;
        /** if enabled/disabled share/unshare time namespace */
        const char *time;
    }
    redConfShareT;

    /** structure holding config values */
    typedef struct {
        /** directory of RPMs */
        const char *rpmdir;
        /** directory of DNF database */
        const char *persistdir;
        /** directory of DNF cache */
        const char *cachedir;
        /** value of PATH */
        const char *path;
        /** value of LD_LIBRARY_PATH */
        const char *ldpath;
        /** hostname value, implies unshare UTS*/
        const char *hostname;
        /** umask for file items creation */
        const char *umask;
        /** directory of execution */
        const char *chdir;
        /** root cgroup for the node */
        const char *cgrouproot;
        /** require check of repository GPG key */
        unsigned int gpgcheck;
        /** remove safety checks on date and inode */
        unsigned int unsafe;
        /** inherit environment */
        unsigned int inheritenv;
        /** set user as virtual root */
        unsigned int maprootuser;
        /** should terminate when parent die? */
        redConfOptFlagE diewithparent;
        /** should create a new session? */
        redConfOptFlagE newsession;
        /** configuration of sharings */
        redConfShareT share;
        /** configuration of cgroups */
        redConfCgroupT *cgroups;
        /** array of capability configurations */
        redConfCapT *capabilities;
        int capabilities_count; /**< count of capability configurations */
        /**  */
        int verbose;
    }
    redConfTagT;

    /** structure recording full node config */
    typedef struct {
        /** for section headers */
        redConfHeaderT headers;
        /** for section config */
        redConfTagT  conftag;
        /** for section environ */
        redConfVarT *confvar;
        unsigned int confvar_count; /**< count of var entries in confvar */
        /** for section exports */
        redConfExportPathT *exports;
        unsigned int exports_count; /**< count of export entries in exports */
    }
    redConfigT;


// ---- RedStatus Schema for ${redpath}/.status

    /** status of a node */
    typedef enum {
        /** node is disabled */
        RED_STATUS_DISABLE=0,
        /** node is enabled */
        RED_STATUS_ENABLE,
        /** node has unknown status */
        RED_STATUS_UNKNOWN,
        /** node has error */
        RED_STATUS_ERROR,
    }
    redStatusFlagE;

    /** Structure recording status of a node */
    typedef struct {
        /** state of the node */
        redStatusFlagE state;
        /** path of the node */
        const char *realpath;
        /** timestamp of the status in milliseconds since epoch */
        unsigned long timestamp;
        /** some info about the state of the node */
        const char *info;
    }
    redStatusT;

/** Load the configuration file of path 'filepath'
 *
 * @param filepath path of the file to load
 * @param warning  ?
 *
 * @return the config or NULL on error
 */
extern redConfigT* RedLoadConfig (const char* filepath, int warning);
extern int RedSaveConfig (const char* filepath, const redConfigT *config, int warning);
extern int RedGetConfigYAML(char **output, size_t *len, const redConfigT *config);
extern int RedFreeConfig(redConfigT *config, int wlevel);
extern int RedFreeConfTag(redConfTagT *conftag, int wlevel);

extern redStatusT* RedLoadStatus (const char* filepath, int warning);
extern int RedSaveStatus (const char* filepath, const redStatusT *status, int warning);
extern int RedFreeStatus(redStatusT *status, int wlevel);

extern int setLogYaml(int level);

extern const char *getExportFlagString(redExportFlagE value);
extern const char *getRedVarEnvString(redVarEnvFlagE value);
extern const char *getRedConfOptString(redConfOptFlagE value);
extern const char *getStatusFlagString(redStatusFlagE value);

#endif // _REDCONFIG_INCLUDE_
