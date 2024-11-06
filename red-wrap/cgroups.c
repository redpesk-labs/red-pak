/*
* Copyright (C) 2021 "IoT.bzh"
* Author Fulup Ar Foll <fulup@iot.bzh>
* Author Clément Bénier <clement.benier@iot.bzh>
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

#define _GNU_SOURCE

#include "cgroups.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <alloca.h>

#include "redconf-log.h"
#include "redconf-utils.h"

#define MAX_CTRL_SIZE   300

#ifndef CGROUPS_PIDS_LEAF_NAME
#define CGROUPS_PIDS_LEAF_NAME "redpak-pids-leaf"
#endif

#ifndef CGROUPS_ADMIN_SUFFIX
#define CGROUPS_ADMIN_SUFFIX ".admin"
#endif

/*
https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html#no-internal-process-constraint
https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html
https://facebookmicrosites.github.io/cgroup2/docs/memory-controller.html
https://docs.redpesk.bzh/docs/en/master/redpesk-os/redpak/6-Troubleshooting.html
*/

/*
distribution of cgroups for rednodes is fitting below rules:

- each rednode leads to 2 cgroups: one for setting cgroup properties (S)
  and one for holding pids of the cgroup (P)

  The setting cgroup (S) is named following the rednode name
  followed by admin extension for admin nodes

  The pids cgroup (P) is child of the setting cgroup (S) and has no
  child (restriction of cgroups)
*/

typedef enum {
    Ctrlid_CPUSET,
    Ctrlid_CPU,
    Ctrlid_MEMORY,
    Ctrlid_IO,
    Ctrlid_PIDS,
    Ctrlid_MISC,
    Ctrlid_HUGETLB,
    Ctrlid_RDMA,
}
    ctrlid_t;

static const char * const ctrlnames[] = {
    "cpuset",
    "cpu",
    "memory",
    "io",
    "pids",
    "misc",
    "hugetlb",
    "rdma",
};

typedef unsigned ctrlmask_t;

typedef struct {
    const char *rootpath;
    ctrlmask_t final;
    int dirfd;
    int pathlen;
    char path[PATH_MAX];
}
    cgroup_t;

static int prepare_cgroups(cgroup_t *state, const redNodeT *node, ctrlmask_t mask);
static int prepare_root_cgroup(cgroup_t *state, ctrlmask_t mask);
static int prepare_middle_cgroup(cgroup_t *state, const redNodeT *node, ctrlmask_t mask, ctrlmask_t childmask);
static int enter_leaf_cgroup(cgroup_t *state, const redNodeT *node);
static int make_root_path(cgroup_t *state);

static ctrlmask_t get_expected_controllers(redConfCgroupT *cgroups);

static void get_node_cpuset(redCpusetT *dest, const redNodeT *node);
static void get_node_cpu(redCpuT *dest, const redNodeT *node);
static void get_node_mem(redMemT *dest, const redNodeT *node);
static void get_node_io(redIoT *dest, const redNodeT *node);
static void get_node_pids(redPidsT *dest, const redNodeT *node);

static void get_conf_cpuset(redCpusetT *dest, const redCpusetT *src);
static void get_conf_cpu(redCpuT *dest, const redCpuT *src);
static void get_conf_mem(redMemT *dest, const redMemT *src);
static void get_conf_io(redIoT *dest, const redIoT *src);
static void get_conf_pids(redPidsT *dest, const redPidsT *src);

static int set_cgroup_node_conf(const redNodeT *node, int gfd);

static int get_effective_controllers(int dfd, ctrlmask_t *mask);
static int add_required_subcontrollers(int dfd, ctrlmask_t mask);
static int ensure_required_controllers(int dfd, ctrlmask_t expected);

static const char *fdname(int dfd);

static int write_entry(int dfd, const char *entry, const char *value);

#define NODE_MODE 0755
#define LEAF_MODE 0755

static const bool paranoiac = true;
static const char pids_leaf_name[] = CGROUPS_PIDS_LEAF_NAME;
static const char admin_suffix[] = CGROUPS_ADMIN_SUFFIX;

/****************************************************************************************
            U T I L I T I E S
****************************************************************************************/

/*
* diagnostic function returning the name of the directory pointed by 'dfd'
* caution: it use a static buffer, avoid concurrency or holding the value
*/
static const char *fdname(int dfd)
{
    char proc[50];
    static char buffer[500];

    ssize_t sz;
    sprintf(proc, "/proc/self/fd/%d", dfd);
    sz = readlink(proc, buffer, sizeof buffer);
    if (sz <= 0) {
        buffer[0] = '?';
        sz = 1;
    }
    else if ((size_t)sz >= sizeof buffer) {
        sz = sizeof buffer - 4;
        buffer[sz++] = '.';
        buffer[sz++] = '.';
        buffer[sz++] = '.';
    }
    buffer[sz] = 0;
    return buffer;
}

/*
* Copy nul terminated 'source' to 'dest' replacing any / by -
* Return the length of the zero terminated copy (excluding tailing nul)
* Or return -1 if the copied length exceeds the given length 'lendest' of destination.
*/
static int replaceSlashDash(const char *source, char *dest, int lendest) {
    int idx;
    for (idx = 0 ; idx < lendest ; idx++) {
        char c = source[idx];
        dest[idx] = c == '/' ? '-' : c;
        if (c == 0)
            return idx;
    }
    return -1;
}

/*
* Set the buffer 'dest' with the given path 'src' of length 'lensrc'.
* If src isn't starting with the prefix of cgroup mounting point, prepend it.
* Remove any / at end.
* Return the length of the buffer dest or -1 if the result can't fit the buffer size.
*/
static int set_cgroup_path(char dest[PATH_MAX], const char *src, int lensrc) {
    const char *prefix = cgroup_root();
    int prefixlen = (int)strlen(prefix);
    int pos = 0;

    /* copy the prefix if needed */
    if (lensrc < prefixlen || strncmp(src, prefix, prefixlen) != 0) {
        memcpy(dest, prefix, (unsigned)prefixlen);
        pos = prefixlen;
    }

    /* ensure slash at start (except if copied prefix doesn't) */
    if (pos == 0 || prefix[pos - 1] != '/')
        dest[pos++] = '/';
    while (lensrc && *src == '/') {
        src++;
        lensrc--;
    }
    while (lensrc && src[lensrc - 1] == '/')
        lensrc--;

    /* check resulting length */
    if (lensrc >= PATH_MAX - pos) {
        RedLog(REDLOG_ERROR, "cgroups path too long: %.*s%.*s", pos, dest, lensrc, src);
        return -1;
    }

    /* copy the src */
    memcpy(&dest[pos], src, lensrc);
    pos += lensrc;
    dest[pos] = 0;
    return pos;
}

/*
* Scan 'path' of 'length' for the suffix /pids_leaf_name
* If not found, return the given length.
* If found, return the length without the suffix
*/
static int length_without_pids_leaf(const char *path, int length) {
    const int suffixlen = (int)strlen(pids_leaf_name);
    const char *const suffix = pids_leaf_name;
    for (;;) {
        int off = length - suffixlen;
        if (off < 0 || memcmp(suffix, &path[off], suffixlen) != 0
            || (off > 0 && path[off - 1] != '/'))
            return length;
        while (off && path[off - 1] == '/')
            off--;
        length = off;
    }
}

/*
* Read from /proc/self/cgroup the current cgroup and return its
* controlling path in 'dest'.
* Return the length of the computed path or -1 in any error case.
*/
static int get_current_cgroup(char dest[PATH_MAX]) {
    int lenbuf, cgProcFd;
    ssize_t count;
    char buf[PATH_MAX];

    /* get current cgroup */
    cgProcFd = open("/proc/self/cgroup", O_RDONLY);
    if (cgProcFd < 0) {
        RedLog(REDLOG_ERROR, "Can not open /proc/self/cgroup: %s", strerror(errno));
        return -1;
    }
    count = read(cgProcFd, (void *)buf, sizeof buf);
    close(cgProcFd);
    if (count <= 0 ) {
        RedLog(REDLOG_ERROR, "Can not read /proc/self/cgroup: %s", strerror(errno));
        return -1;
    }

    /* check running cgroup2 */
    if (count < 3 || buf[0] != '0' || buf[1] != ':' || buf[2] != ':') {
        RedLog(REDLOG_ERROR, "Expected CGROUPv2 not found in /proc/self/cgroup");
        return -1;
    }

    /* search first \n or end */
    for (lenbuf = 3 ; lenbuf < count && buf[lenbuf] != '\n' ; lenbuf++);

    /* set the found cgroup */
    return set_cgroup_path(dest, &buf[3], lenbuf - 3);
}

/*
* Scan the given path. If (1) it does not exist, or, (2) it is not a directory, or,
* (3) it parent does not contain the redpak's pids leaf directory, then, return the given length
* Otherwise, removes the last directory and return the result of resursive scan.
*/
static int find_rednode_root(char path[PATH_MAX], int length) {
    char savec;
    int dirfd, found, rc;

    /* remove any / at end */    
    while (length > 0 && path[length - 1] == '/')
        length--;
    length = length_without_pids_leaf(path, length);


    /* loop until root found */
    found = length; /* initial search as default result */
    while (length > 0) {

        /* open the current directory at length */
        savec = path[length];
        path[length] = 0;
        dirfd = open(path, O_DIRECTORY);
        path[length] = savec;
        if (dirfd < 0)
            break; /* can't open it, end */

        /* check if redpak's pids leaf exists */
        rc = faccessat(dirfd, pids_leaf_name, F_OK, 0);
        close(dirfd);
        if (rc < 0)
            break; /* no redpak's pids leaf */

        /* record last cgroup with a redpak's pids leaf as the found value */
        found = length;

        /* find the parent length */
        while (length > 0 && path[length - 1] != '/')
            length--;
        while (length > 0 && path[length - 1] == '/')
            length--;
    }
    return found;
}

/*
* Write the 'value' in the 'entry' of the directory pointed by 'dfd'
* except if value is NULL and in that case do nothing.
* Return 0 on success or -1 on error.
*/
static int write_entry(int dfd, const char *entry, const char *value)
{
    if (value == NULL)
        return 0;
    else {
        int efd = openat (dfd, entry, O_WRONLY);
        if (efd >= 0) {
            size_t len = strlen(value);
            ssize_t count = write(efd, value, len);
            close(efd);
            if (count == (ssize_t)len)
                return 0;
        }
        RedLog(REDLOG_ERROR, "can't %s entry %s/%s: %s", efd < 0 ? "open" : "write",
               fdname(dfd), entry, strerror(errno));
        return -1;
    }
}

/**
* Create the cgroup of name 'cgroupName' in the cgroup pointed by 'dirFd'
* Return a negative value on error
* Return 0 on success but cgroup already existed
* Return 1 on success and cgroup was freshly created
*/
static int createCgroupAtDir(int dirFd, const char *cgroupName, int mode) {
    /* try first to open */
    int rc = mkdirat(dirFd, cgroupName, mode);
    if (rc < 0 && errno != EEXIST) {
        RedLog(REDLOG_ERROR, "can not create cgroup %s in %s: %s", cgroupName,
               fdname(dirFd), strerror(errno));
        return -1;
    }
    return rc >= 0;
}

/**
* Open the cgroup of name 'cgroupName' in the cgroup pointed by 'dirFd'
* If it does not exist, create it if 'created' is not NULL
* Return a negative value on error
* Return the positive or zero fileno of the open directory for the cgroup
* When 'created' is not NULL and the result is positive or zero, the
* int pointed by 'created' will receive 1 when the Cgroup was created
* or 0 else, when the cgroup already existed
*/
static int openCgroupAtDir(int dirFd, const char *cgroupName, int *created) {
    /* try first to open */
    int rc = openat(dirFd, cgroupName, O_DIRECTORY);
    if (created == NULL) {
        if (rc < 0)
            RedLog(REDLOG_ERROR, "can not open cgroup %s/%s: %s", fdname(dirFd),
                   cgroupName, strerror(errno));
    }
    else {
        *created = rc < 0;
        if (rc < 0) {
            /* try then to create */
            rc = createCgroupAtDir(dirFd, cgroupName, 0755);
            if (rc >= 0)
                rc = openCgroupAtDir(dirFd, cgroupName, NULL);
        }
    }
    return rc;
}

/**
* move all the process of cgroup pointed by 'fromFd' to cgroup pointed by 'toFd'
* return 0 on success or a negative value other wise
*/
static int moveProcs(int fromFd, int toFd) {
    int rpos, pidpos, count, procsFd;
    char buffer[200];

    /* opens the source procs for reading */
    procsFd = openat(fromFd, "cgroup.procs", O_RDONLY);
    if (procsFd < 0) {
        RedLog(REDLOG_ERROR, "can not open %s/cgroup.procs: %s", fdname(fromFd), strerror(errno));
        return -1;
    }

    for(rpos = 0;;) {
        /* read pids in buffer */
        count = (int)read(procsFd, buffer, sizeof(buffer) - (size_t)rpos);
        if (count < 0 ) {
            RedLog(REDLOG_ERROR, "cannot read %s: %s", fdname(procsFd), strerror(errno));
            close(procsFd);
            return -1;
        }
        /*
        * each pid is followed with a \n.
        * it implies that count==0 (end of file) implies rpos==0
        */
        if (count == 0) {
            close(procsFd);
            return 0;
        }
        /* scan pids of buffer */
        for (pidpos = 0, count += rpos ; rpos < count ; rpos++) {
            if (buffer[rpos] == '\n') {
                /* move process */
                buffer[rpos] = '\0';
                if (write_entry(toFd, "cgroup.procs", &buffer[pidpos])) {
                    close(procsFd);
                    return -1;
                }
                pidpos = rpos + 1;
            }
        }
        memmove(buffer, &buffer[pidpos], (size_t)(rpos - pidpos));
        rpos -= pidpos;
    }
}

/****************************************************************************************
            U T I L I T I E S
****************************************************************************************/

/*
* setup the configuration of cgroups for the given node
* rootpath is the computed root path as set in configuration files
* rootpath can be NULL in wich case it is computed
*/
int set_cgroups(const redNodeT *node, const char *rootpath)
{
    int rc;
    cgroup_t state;

    /* setup processing state */
    state.final = 0;
    state.dirfd = -1;
    state.rootpath = rootpath;
    state.pathlen = 0;

    /* process down and then up for node and at end make leaf */
    rc = prepare_cgroups(&state, node, 0);
    if (rc >= 0)
        rc = enter_leaf_cgroup(&state, node);

    /* close the current fd */
    if (state.dirfd >= 0)
        close(state.dirfd);

    if (rc < 0)
        RedLog(REDLOG_ERROR,
            "\n"
            "[CGROUP ERROR]: cgroup error can be caused from several causes:\n"
            "First you need to have rights to create cgroup in %s\n"
            "or to move the process into created cgroup.\n"
            "If you don't, maybe it is because your not in an user session,\n"
            "or because some controllers are not delegate to children.\n"
            "For that you can try: \n"
            "\n"
            "   systemd-run --user --pty -p \"Delegate=yes\"  bash\n"
            "\n"
            "See redpak documentation troubleshooting: https://docs.redpesk.bzh/docs/en/master/redpesk-os/redpak/6-Troubleshooting.html\n"
            "For controller issues see: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html\n",
            cgroup_root()
        );

    return rc;
}

/*
* process cgroup settings recursively
*/
static int prepare_cgroups(cgroup_t *state, const redNodeT *node, ctrlmask_t childmask)
{
    int rc;
    ctrlmask_t mask = childmask;

    /* below root case, at bottom, ensure root */
    if (node == NULL)
        return prepare_root_cgroup(state, childmask);

    /* compute currently required cgroups mask */
    mask |= get_expected_controllers(node->config->conftag.cgroups);
    if (node->confadmin != NULL)
        mask |= get_expected_controllers(node->confadmin->conftag.cgroups);

    /* process parent cgroups */
    rc = prepare_cgroups(state, node->parent, mask);

    /* process current node cgroups */
    if (rc >= 0)
        rc = prepare_middle_cgroup(state, node, mask, childmask);

    return rc;
}

/*
* prepare root cgroup if required
*/
static int prepare_root_cgroup(cgroup_t *state, ctrlmask_t childmask)
{
    int rc, created, pidfd;

    /* record final mask */
    state->final = childmask;

    /* action only when needed */
    if (childmask == 0)
        return 0;

    /* make the root path */
    rc = make_root_path(state);
    if (rc < 0)
        return rc;

    /* opens the root */
    rc = open(state->path, O_DIRECTORY);
    if (rc < 0) {
        RedLog(REDLOG_ERROR, "can't open Cgroup %.*s: %s", state->pathlen, state->path, strerror(rc));
        return rc;
    }
    state->dirfd = rc;

    /*
    * Creates or open the leaf cgroup for pids
    */
    rc = openCgroupAtDir(state->dirfd, pids_leaf_name, &created);
    if (rc < 0)
        return rc;
    pidfd = rc;

    /*
    * Moves processes out of this cgroup to that leaf cgroup for pids
    * This is achived only if the leaf cgroup for pids was created
    * because otherwise, pids are already out of current cgroup.
    * Except that paranoiac mode does it always.
    */
    if (created || paranoiac)
        rc = moveProcs(state->dirfd, pidfd);
    close(pidfd);
    if (rc < 0)
        return rc;

    /* requires the controlers (add pids for moving pids at root) */
    childmask |= (1u << Ctrlid_PIDS);
    return rc < 0 ? rc : ensure_required_controllers(state->dirfd, childmask);
}

/*
* prepare cgroups at intermediate or final level
* When the final mask is zero, there is no 
* state should contain 
*/
static int prepare_middle_cgroup(cgroup_t *state, const redNodeT *node, ctrlmask_t mask, ctrlmask_t childmask)
{
    int rc, off, len, created;

    (void)mask;

    /* action only when needed */
    if (state->final == 0)
        return 0;

    /*
    * Compute the translated name of the rednode
    * The translated name is the path of the node with slashes replaced by dashes
    * exemple: /rednodes/applis/appli1 is translated to -rednodes-applis-appli1
    *
    * The name of the parent cgroup is in the state-path.
    * Put the translated name one character after the end of the parent cgroup name.
    * This is an optimisation because the name of current cgroup is at the end
    * obtaned by put the character slash (/) between the parent path and the
    * translated name.
    */
    off = state->pathlen + 1;
    rc = replaceSlashDash(node->status->realpath, &state->path[off],
                          (int)(sizeof(state->path) - off));
    if (rc < 0) {
        RedLog(REDLOG_ERROR, "Cgroup name too long for %s after %.*s",
               node->status->realpath, state->pathlen, state->path);
        return -1;
    }
    len = rc;

    /*
    * Append admin suffix if needed
    * When the current node has an admin part then the cgroup is for admin.
    * In order to not mix admin and normal restrictions, two different cgroups are used.
    */
    if (node->confadmin != NULL) {
        const int lensuffix = (int)strlen(admin_suffix);
        if ((int)sizeof(state->path) <= off + len + lensuffix) {
            RedLog(REDLOG_ERROR, "Cgroup name too long for %s after %.*s%s",
                   node->status->realpath, state->pathlen, state->path, admin_suffix);
            return -1;
        }
        memcpy(&state->path[off], admin_suffix, (size_t)lensuffix);
        len += lensuffix;
    }

    /*
    * Create the directory for the new cgroup (if not already existing)
    */
    rc = openCgroupAtDir(state->dirfd, &state->path[off], &created);
    if (rc < 0)
        return rc;

    /*
    * Switch to the newly created cgroup
    */
    close(state->dirfd);
    state->dirfd = rc;
    state->path[state->pathlen] = '/';
    state->pathlen += 1 + len;

    /*
    * Create pids leafs cgroup if the current cgroup was created.
    * Except that paranoiac mode does it always.
    */
    if (created || paranoiac) {
        rc = createCgroupAtDir(state->dirfd, pids_leaf_name, LEAF_MODE);
        if (rc < 0)
            return rc;
    }

    /*
    * set node config for current cgroup
    */
    rc = set_cgroup_node_conf(node, state->dirfd);
    if (rc < 0)
        return rc;

    /*
    * prepare controllers for children
    */
    childmask |= (1u << Ctrlid_PIDS);
    return ensure_required_controllers(state->dirfd, childmask);
}

/*
* After preparing all cgroup hierarchy for the target node (the leaf),
* moves the current process in that node.
*/
static int enter_leaf_cgroup(cgroup_t *state, const redNodeT *node)
{
    int rc, dirfd;
    char bpid[30];

    (void)node;

    /* action only when needed */
    if (state->final == 0)
        return 0;

    /* get current pids leaf pointer */
    rc = openCgroupAtDir(state->dirfd, pids_leaf_name, NULL);
    if (rc < 0)
        return rc;

    /* moves the current process to that leaf */
    sprintf(bpid, "%lu", (long unsigned)getpid());
    dirfd = rc;
    rc = write_entry(dirfd, "cgroup.procs", bpid);
    close(dirfd);
    return rc;
}

/*
* compute in state the root path and its length
* returns 0 on success, -1 on error
*/
static int make_root_path(cgroup_t *state)
{
    int rc;

    /* set in state the initial path for cgroups */
    if (state->rootpath == NULL)
        rc = get_current_cgroup(state->path);
    else {
        int len = (int)strlen(state->rootpath);
        rc = set_cgroup_path(state->path, state->rootpath, len);
    }
    if (rc < 0)
        return rc;

    state->pathlen = find_rednode_root(state->path, rc);
    state->path[state->pathlen] = 0;
    return 0;
}

/*
* write to the controllers of the cgroup pointed by 'gfd'
* settings required by 'node'
*/
static int set_cgroup_node_conf(const redNodeT *node, int gfd)
{
    redCpusetT cpuset;
    redCpuT cpu;
    redMemT mem;
    redIoT io;
    redPidsT pids;

    int nerr = 0;

    /* cpuset */
    get_node_cpuset(&cpuset, node);
    nerr += write_entry(gfd, "cpuset.cpus", cpuset.cpus);
    nerr += write_entry(gfd, "cpuset.mems", cpuset.mems);
    nerr += write_entry(gfd, "cpuset.cpus.partition", cpuset.cpus_partition);

    /* cpu */
    get_node_cpu(&cpu, node);
    nerr += write_entry(gfd, "cpu.weight", cpu.weight);
    nerr += write_entry(gfd, "cpu.max", cpu.max);

    /* memory */
    get_node_mem(&mem, node);
    nerr += write_entry(gfd, "memory.min", mem.min);
    nerr += write_entry(gfd, "memory.max", mem.max);
    nerr += write_entry(gfd, "memory.high", mem.high);
    nerr += write_entry(gfd, "memory.swap.max", mem.swap_max);
    nerr += write_entry(gfd, "memory.swap.high", mem.swap_high);

    /* io */
    get_node_io(&io, node);
    nerr += write_entry(gfd, "io.max", io.max);
    nerr += write_entry(gfd, "io.weight", io.weight);

    /* pids */
    get_node_pids(&pids, node);
    nerr += write_entry(gfd, "pids.max", pids.max);

    return nerr;
}

/*
* scan the cgroups directory pointed by 'dfd' and returns the mask
* of the available controllers
*/
static int get_effective_controllers(int dfd, ctrlmask_t *mask)
{
    char buffer[MAX_CTRL_SIZE];
    int sz, pos, len, idx, cfd;
    ctrlmask_t result;

    /* read the content of cgroup.controllers */
    cfd = openat (dfd, "cgroup.controllers", O_RDONLY);
    if (cfd < 0) {
        RedLog(REDLOG_ERROR, "can't open %s/cgroup.controllers: %s", fdname(dfd), strerror(errno));
        return -1;
    }
    sz = (int)read(cfd, buffer, sizeof buffer);
    close(cfd);
    if (sz < 0) {
        RedLog(REDLOG_ERROR, "can't read %s/cgroup.controllers: %s", fdname(dfd), strerror(errno));
        return -1;
    }

    /* compute the resulting mask */
    result = 0;
    for (pos = 0 ; pos < sz ; pos++) {
        /* remove separators */
        while (pos < sz && buffer[pos] <= ' ') pos++;
        /* find len without separators */
        for (len = 0 ; pos + len < sz && buffer[pos + len] > ' ' ; len++);
        if (len > 0) {
            idx = sizeof ctrlnames / sizeof *ctrlnames;
            while (idx-- && strncmp(ctrlnames[idx], &buffer[pos], (size_t)len) != 0);
            if (idx >= 0)
                result |= 1u << idx;
        }
        pos += len;
    }
    *mask = result;
    return 0;
}

/*
* write in cgroup pointed by 'dfd' the value required to enable
* controllers of given mask in subtrees.
*/
static int add_required_subcontrollers(int dfd, ctrlmask_t mask)
{
    char buffer[MAX_CTRL_SIZE];
    int len, idx;

    for (idx = len = 0 ; mask != 0 ; idx++) {
        if (mask & (1u << idx)) {
            mask ^= (1u << idx);
            if (len)
                buffer[len++] = ' ';
            buffer[len++] = '+';
            strcpy(&buffer[len], ctrlnames[idx]);
            len += (int)strlen(&buffer[len]);
        }
    }
    return len == 0 ? 0 : write_entry(dfd, "cgroup.subtree_control", buffer);
}

/*
* ensure cgroup pointed by 'dfd' has controllers as 'expected'
*/
static int ensure_required_controllers(int dfd, ctrlmask_t expected)
{
    int rc, pfd;
    ctrlmask_t current, missing;

    /* get current effective controllers */
    rc = get_effective_controllers(dfd, &current);
    if (rc < 0)
        return rc;

    /* compute missing controllers */
    missing = expected & ~current;

    /* nothing is missing */
    if (missing != 0) {
        /* enter parent */
        pfd = openat(dfd, "..", O_DIRECTORY);
        if (pfd < 0) {
            RedLog(REDLOG_ERROR, "can't open %s/..: %s", fdname(dfd), strerror(errno));
            return -1;
        }

        /* activate missing controllers on parent cgroup */
        rc = ensure_required_controllers(pfd, missing);
        close (pfd);
    }

    return rc < 0 ? rc : add_required_subcontrollers(dfd, expected);
}

/*
* inspect a redConfCgroupT object (possibly NULL)
* and returns a mask of expected controlers
*/
static ctrlmask_t get_expected_controllers(redConfCgroupT *cgroups)
{
    ctrlmask_t result = 0;
    if (cgroups != NULL) {
        if (cgroups->cpuset)
            result = 1u << Ctrlid_CPUSET;
        if (cgroups->cpu)
            result |= 1u << Ctrlid_CPU;
        if (cgroups->mem)
            result |= 1u << Ctrlid_MEMORY;
        if (cgroups->io)
            result |= 1u << Ctrlid_IO;
        if (cgroups->pids)
            result |= 1u << Ctrlid_PIDS;
    }
    return result;
}

/*
* set in 'dest' the merged cgroups cpuset values of 'node'
*/
static void get_node_cpuset(redCpusetT *dest, const redNodeT *node)
{
    memset(dest, 0, sizeof *dest);
    if (node->config->conftag.cgroups != NULL)
        get_conf_cpuset(dest, node->config->conftag.cgroups->cpuset);
    if (node->confadmin != NULL && node->confadmin->conftag.cgroups != NULL)
        get_conf_cpuset(dest, node->confadmin->conftag.cgroups->cpuset);
}

/*
* set in 'dest' the merged cgroups cpu values of 'node'
*/
static void get_node_cpu(redCpuT *dest, const redNodeT *node)
{
    memset(dest, 0, sizeof *dest);
    if (node->config->conftag.cgroups != NULL)
        get_conf_cpu(dest, node->config->conftag.cgroups->cpu);
    if (node->confadmin != NULL && node->confadmin->conftag.cgroups != NULL)
        get_conf_cpu(dest, node->confadmin->conftag.cgroups->cpu);
}

/*
* set in 'dest' the merged cgroups memory values of 'node'
*/
static void get_node_mem(redMemT *dest, const redNodeT *node)
{
    memset(dest, 0, sizeof *dest);
    if (node->config->conftag.cgroups != NULL)
        get_conf_mem(dest, node->config->conftag.cgroups->mem);
    if (node->confadmin != NULL && node->confadmin->conftag.cgroups != NULL)
        get_conf_mem(dest, node->confadmin->conftag.cgroups->mem);
}

/*
* set in 'dest' the merged cgroups io values of 'node'
*/
static void get_node_io(redIoT *dest, const redNodeT *node)
{
    memset(dest, 0, sizeof *dest);
    if (node->config->conftag.cgroups != NULL)
        get_conf_io(dest, node->config->conftag.cgroups->io);
    if (node->confadmin != NULL && node->confadmin->conftag.cgroups != NULL)
        get_conf_io(dest, node->confadmin->conftag.cgroups->io);
}

/*
* set in 'dest' the merged cgroups pids values of 'node'
*/
static void get_node_pids(redPidsT *dest, const redNodeT *node)
{
    memset(dest, 0, sizeof *dest);
    if (node->config->conftag.cgroups != NULL)
        get_conf_pids(dest, node->config->conftag.cgroups->pids);
    if (node->confadmin != NULL && node->confadmin->conftag.cgroups != NULL)
        get_conf_pids(dest, node->confadmin->conftag.cgroups->pids);
}

/*
* add in 'dest' the effective cgroups cpuset values of 'src'
*/
static void get_conf_cpuset(redCpusetT *dest, const redCpusetT *src)
{
    if (src != NULL) {
        if (src->cpus != NULL)
            dest->cpus = src->cpus;
        if (src->mems != NULL)
            dest->mems = src->mems;
        if (src->cpus_partition != NULL)
            dest->cpus_partition = src->cpus_partition;
    }
}

/*
* add in 'dest' the effective cgroups cpu values of 'src'
*/
static void get_conf_cpu(redCpuT *dest, const redCpuT *src)
{
    if (src != NULL) {
        if (src->weight != NULL)
            dest->weight = src->weight;
        if (src->max != NULL)
            dest->max = src->max;
        if (src->weight_nice != NULL)
            dest->weight_nice = src->weight_nice;
    }
}

/*
* add in 'dest' the effective cgroups memory values of 'src'
*/
static void get_conf_mem(redMemT *dest, const redMemT *src)
{
    if (src != NULL) {
        if (src->max != NULL)
            dest->max = src->max;
        if (src->high != NULL)
            dest->high = src->high;
        if (src->min != NULL)
            dest->min = src->min;
        if (src->low != NULL)
            dest->low = src->low;
        if (src->oom_group != NULL)
            dest->oom_group = src->oom_group;
        if (src->swap_high != NULL)
            dest->swap_high = src->swap_high;
        if (src->swap_max != NULL)
            dest->swap_max = src->swap_max;
    }
}

/*
* add in 'dest' the effective cgroups io values of 'src'
*/
static void get_conf_io(redIoT *dest, const redIoT *src)
{
    if (src != NULL) {
        if (src->cost_qos != NULL)
            dest->cost_qos = src->cost_qos;
        if (src->cost_model != NULL)
            dest->cost_model = src->cost_model;
        if (src->weight != NULL)
            dest->weight = src->weight;
        if (src->max != NULL)
            dest->max = src->max;
    }
}

/*
* add in 'dest' the effective cgroups pids values of 'src'
*/
static void get_conf_pids(redPidsT *dest, const redPidsT *src)
{
    if (src != NULL) {
        if (src->max != NULL)
            dest->max = src->max;
    }
}

