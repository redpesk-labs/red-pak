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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include "cgroups.h"
#include "redconf-log.h"
#include "redconf-defaults.h"

//https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html

// write group within corresponding sandbox/subgroup dir FD
static int utilsFileAddControl (const char *node_name, int dirFd, const char *ctrlname, const char *ctrlval) {
    int ctrlfd;
    size_t len, count;

    ctrlfd = openat (dirFd, ctrlname, O_WRONLY);
    if (ctrlfd < 0) {
        RedLog(REDLOG_ERROR, "[cgroup-ctrl-not-found] node='%s' ctrlname='%s' error=%s (nsCgroupSetControl)", node_name, ctrlname, strerror(errno));
        goto OnErrorExit;
    }

    len= strlen(ctrlval);
    count= write(ctrlfd, ctrlval, len);

    // when error add inod to help debuging
    if (count != len) {
        struct stat statfd;
        fstat (ctrlfd, &statfd);
        RedLog(REDLOG_ERROR, "[cgroup-control-refused] node='%s' ctrlname='%s' inode=%ld value=%s error=%s (nsCgroupSetControl)", node_name, ctrlname, statfd.st_ino, ctrlval, strerror(errno));
        close (ctrlfd);
        goto OnErrorExit;
    }

    close (ctrlfd);
    return 0;

OnErrorExit:
    return 1;
}

static int openCreateCgroupFromDir(int dirFd, const char *cgroupName) {
    int err;
    int cgroupFd;

    cgroupFd = openat(dirFd, cgroupName, O_DIRECTORY);
    if (cgroupFd < 0) {
        err = mkdirat(dirFd, cgroupName, 0755);
        if (err) {
            RedLog(REDLOG_ERROR, "[cgroups-create-fail] cgroup name='%s' dirFd='%d' error=%s", cgroupName, dirFd, strerror(errno));
            goto OnErrorExit;
        }

        // open newly create cgroup
        cgroupFd = openat(dirFd, cgroupName, O_DIRECTORY);
        if (cgroupFd < 0) {
            RedLog(REDLOG_ERROR, "[cgroups-open-fail] cgroup name='%s' dirFd='%d' error=%s", cgroupName, dirFd, strerror(errno));
            goto OnErrorExit;
        }
    }

    return cgroupFd;

OnErrorExit:
    return -1;
}

// copy source to dest replacing any / by -
void replaceSlashDash(const char *source, char *dest) {
    char c = *source;
    while (c) {
        *dest++ = c == '/' ? '-' : c;
        c = *++source;
    }
    *dest = c;
}

static int get_parent_cgroup(char cgroup_parent[PATH_MAX]) {
    int lenbuf, cgProcFd;
    ssize_t count;
    char buf[PATH_MAX + 1 - strlen(CGROUPS_MOUNT_POINT)];

    //get current cgroup
    cgProcFd = open("/proc/self/cgroup", O_RDONLY);
    if (cgProcFd <= 0) {
        RedLog(REDLOG_ERROR, "[proc-cgroups-not-found] /proc/self/cgroup error=%s", strerror(errno));
        goto OnErrorExit;
    }

    //read 3 first characters to ignore them, valid for cgroup2
    count = read(cgProcFd, (void *)buf, 3);
    if (count != 3) {
        RedLog(REDLOG_ERROR, "Cannot read 3 first characters count=%d buf=%s", count, buf);
        goto OnErrorCloseExit;
    }
    //check cgroup2
    if (buf[0] != '0' || buf[1] != ':' || buf[2] != ':') {
        RedLog(REDLOG_ERROR, "expected CGROUPv2 not matching /proc/self/cgroup");
        goto OnErrorCloseExit;
    }

    count = read(cgProcFd, (void *)buf, sizeof buf);
    if (count <= 0 ) {
        RedLog(REDLOG_ERROR, "[/proc/self/cgroup] cannot read current cgroup error=%s", strerror(errno));
        goto OnErrorCloseExit;
    }

    //search first \n
    for (lenbuf = 0 ; lenbuf < (int)count && buf[lenbuf] != '\n' ; lenbuf++);
    if (lenbuf == 0 || lenbuf == (int)sizeof buf) {
        RedLog(REDLOG_ERROR, "[proc-cgroups-too-long] /proc/self/cgroup has a too %s path", lenbuf ? "long" : "short");
        goto OnErrorCloseExit;
    }
    buf[lenbuf] = '\0';

    snprintf(cgroup_parent, PATH_MAX, "%s%s", CGROUPS_MOUNT_POINT, buf);
    cgroup_parent[PATH_MAX - 1] = 0;

    //check if cgroup is matching CGROUPS_ROOT_LEAF_NAME and so return parent/..
    int off = lenbuf - strlen(CGROUPS_ROOT_LEAF_NAME);
    if (off >= 0 && !strcmp(buf+off, CGROUPS_ROOT_LEAF_NAME))
        strncat(cgroup_parent, "/..", PATH_MAX - 1 - strlen("/..") - strlen(cgroup_parent));

    close(cgProcFd);
    return 0;

OnErrorCloseExit:
    close(cgProcFd);
OnErrorExit:
    return -1;
}

static int moveProcstoLeaf(int parentFd) {
    int procsFd, parentLeafFd = 0, count;
    char buf[20];
    char pid[10];

    procsFd = openat(parentFd, "cgroup.procs", O_RDONLY);
    if (procsFd < 0) {
        RedLog(REDLOG_ERROR, "[cgroups-procs-not-found] error=%s", strerror(errno));
        goto OnErrorExit;
    }

    int j=0;
    do {
        count = read(procsFd, (void *)buf, 2);
        if (count < 0 ) {
            RedLog(REDLOG_ERROR, "[cgroups-procs] cannot read current cgroup.procs error=%s", strerror(errno));
            goto OnErrorCloseExit;
        }

        for (int i = 0; i < count; i++) {
            if (buf[i] != '\n') {
                pid[j] = buf[i];
                j++;
            }
            else {
                pid[j] = '\0';
                if (!parentLeafFd) {
                    //create leaf cgroup for moving process pids
                    parentLeafFd = openCreateCgroupFromDir(parentFd, CGROUPS_ROOT_LEAF_NAME);
                    if(parentLeafFd < 0) {
                        goto OnErrorCloseExit;
                    }
                }

                //move process
                if (utilsFileAddControl("cgroup.procs", parentLeafFd, "cgroup.procs", pid)) {
                    RedLog(REDLOG_ERROR, "Cannot switch process pids %d to node cgroup error=%s", pid, strerror(errno));
                    goto OnErrorCloseExit2;
                }
                j = 0;
            }
        }
    } while(count > 0);


    close(procsFd);
    return 0;

OnErrorCloseExit2:
    close(parentLeafFd);
OnErrorCloseExit:
    close(procsFd);
OnErrorExit:
    return -1;
}

int cgroups (redConfCgroupT *cgroups, const char *cgroup_name, char cgroup_parent[PATH_MAX]) {
    int err, cgRootFd, subgroupFd = 0, subgroupNodeFd;
    char pid[1000];

    // get current parent cgroup
    if (strlen(cgroup_parent) == 0) {
        if (get_parent_cgroup(cgroup_parent) < 0) {
            goto OnErrorExit;
        }
    }

    RedLog(REDLOG_DEBUG, "[cgroup=%s]: cgroup_parent=%s", cgroup_name, cgroup_parent);

    // get fd parent cgroup
    cgRootFd = open(cgroup_parent, O_DIRECTORY);
    if (cgRootFd <= 0) {
        RedLog(REDLOG_ERROR, "[cgroups-not-found] cgroup_parent=%s error=%s", cgroup_parent, strerror(errno));
        goto OnErrorExit;
    }

    // get/create cgroup subgroup
    subgroupFd = openCreateCgroupFromDir(cgRootFd, cgroup_name);
    if (subgroupFd < 0)
        goto OnErrorClose1Exit;

    // get/create cgroup leaf for current node
    subgroupNodeFd = openCreateCgroupFromDir(subgroupFd , "node");
    if (subgroupNodeFd < 0)
        goto OnErrorClose2Exit;

    // move all process into cgroups leaf
    // see the no internal process contraint
    // https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html#no-internal-process-constraint
    err = moveProcstoLeaf(cgRootFd);
    if (err) {
        RedLog(REDLOG_ERROR, "Cannot move all pids from cgroup_parent=%s to leaf error=%s", cgroup_parent, strerror(errno));
        goto OnErrorCloseExit;
    }

    //switch current process to node leaf cgroup
    sprintf(pid, "%d", (int)getpid());
    err = utilsFileAddControl(cgroup_name, subgroupNodeFd, "cgroup.procs", pid);
    if (err) {
        RedLog(REDLOG_ERROR, "Cannot switch current process %s to node cgroup error=%s", pid, strerror(errno));
        goto OnErrorCloseExit;
    }

    // delegate controllers in parent cgroup
    err = utilsFileAddControl (cgroup_name, cgRootFd, "cgroup.subtree_control", "+cpuset +cpu +memory +io +pids");
    if (err) {
        RedLog(REDLOG_WARNING, "[cgroups-set-failed] node='%s' cgroups='%s' error=%s", cgroup_name, cgroup_name, strerror(errno));
    }

    RedLog(REDLOG_DEBUG, "[cgroups]: create cgroup from parent %s cgroup=%s", cgroup_parent, cgroup_name);

    if (!cgroups)
        goto OnSuccessExit;

    if (cgroups->cpuset) {
        if (cgroups->cpuset->cpus) err += utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.cpus", cgroups->cpuset->cpus);
        if (cgroups->cpuset->mems) err += utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.mems", cgroups->cpuset->mems);
        if (cgroups->cpuset->cpus_partition) err += utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.cpus.partition", cgroups->cpuset->cpus_partition);
        if (err) goto OnErrorExit;
    }

    if (cgroups->cpu) {
        // https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html
        if (cgroups->cpu->weight)  err += utilsFileAddControl (cgroup_name, subgroupFd, "cpu.weight", cgroups->cpu->weight);
        if (cgroups->cpu->max)  err += utilsFileAddControl (cgroup_name, subgroupFd, "cpu.max", cgroups->cpu->max);
        if (err) goto OnErrorExit;
    }

    if (cgroups->mem) {
        // https://facebookmicrosites.github.io/cgroup2/docs/memory-controller.html
        if (cgroups->mem->min)   err += utilsFileAddControl (cgroup_name, subgroupFd, "memory.min", cgroups->mem->min);
        if (cgroups->mem->max)   err += utilsFileAddControl (cgroup_name, subgroupFd, "memory.max", cgroups->mem->max);
        if (cgroups->mem->high) err += utilsFileAddControl (cgroup_name, subgroupFd, "memory.high", cgroups->mem->high);
        if (cgroups->mem->swap_max)  err += utilsFileAddControl (cgroup_name, subgroupFd, "memory.swap.max", cgroups->mem->swap_max);
        if (cgroups->mem->swap_high)  err += utilsFileAddControl (cgroup_name, subgroupFd, "memory.swap.high", cgroups->mem->swap_high);
        if (err) goto OnErrorExit;
    }


    if (cgroups->io) {
        if (cgroups->io->max) err += utilsFileAddControl (cgroup_name, subgroupFd, "io.max", cgroups->io->max);
        if (cgroups->io->weight) err += utilsFileAddControl (cgroup_name, subgroupFd, "io.weight", cgroups->io->weight);
        if (err) goto OnErrorExit;
    }

OnSuccessExit:
    close(cgRootFd);
    close(subgroupFd);
    close(subgroupNodeFd);
    return 0;

OnErrorCloseExit:
    close(subgroupNodeFd);
OnErrorClose2Exit:
    close(subgroupFd);
OnErrorClose1Exit:
    close(cgRootFd);
OnErrorExit:
    RedLog(REDLOG_ERROR,
        "\n[CGROUP ERROR]: cgroup error can be caused from several causes:\n"
        "First you need to have rights to create cgroup into parent directory=%s\n"
        "or to move the process into created cgroup\n"
        "if you don't, maybe it is because your not in an user session,\n"
        "or because some controllers are not delegate to children\n"
        "for that you can try: \n"
        "'systemd-run --user --pty -p \"Delegate=yes\"  bash'\n"
        "if it still fails, check %s/cgroup.controllers and %s/cgroup.subtree_control\n"
        "See redpak documentation troubleshooting: https://docs.redpesk.bzh/docs/en/master/redpesk-os/redpak/6-Troubleshooting.html\n"
        "For controller issues see: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html",
        cgroup_parent, cgroup_parent, cgroup_parent
        );
    return -1;
} // end
