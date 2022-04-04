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
#include "redconf-utils.h"
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

static void replaceSlashDash(const char *source, char *dest) {
    int i;
    char *tmp_source = (char *)source;
    if(tmp_source[0] == '/')
        tmp_source++;
    // replace / by - in parent name
    for (i = 0; tmp_source[i]; i++) {
        dest[i] = tmp_source[i];
        if (tmp_source[i] == '/')
            dest[i] = '-';
    }
    dest[i] = '\0';
}

static int get_parent_cgroup(char *cgroup_parent) {
	int count, cgProcFd;
	size_t size_cgroup_parent;
    char buf[PATH_MAX];

	//get current cgroup
    cgProcFd = open("/proc/self/cgroup", O_RDONLY);
    if (cgProcFd <= 0) {
        RedLog(REDLOG_ERROR, "[proc-cgroups-not-found] /proc/self/cgroup error=%s", strerror(errno));
        goto OnErrorExit;
    }

	//read 3 first characters to ignore them
	count = read(cgProcFd, (void *)buf, 3);
	if (count != 3) {
        RedLog(REDLOG_ERROR, "Cannot read 3 first characters count=%d buf=%s", count, buf);
		goto OnErrorExit;
    }

    count = read(cgProcFd, (void *)buf, PATH_MAX);
    if (count <= 0 ) {
        RedLog(REDLOG_ERROR, "[/proc/self/cgroup] cannot read current cgroup error=%s", strerror(errno));
        goto OnErrorExit;
    }

	if (count == PATH_MAX) {
		RedLog(REDLOG_ERROR, "[proc-cgroups-too-long] /proc/self/cgroup has a too long path cannot read");
		goto OnErrorExit;
	}
    // last character should be \n
    buf[count-1] = '\0';

    //add 4 because of /..
	size_cgroup_parent = strlen(CGROUPS_MOUNT_POINT) + strlen(buf) + 4;
    snprintf(cgroup_parent, size_cgroup_parent, "%s%s/..", CGROUPS_MOUNT_POINT, buf);
	return 0;

OnErrorExit:
	return -1;
}


int cgroups (redConfCgroupT *cgroups, const char *cgroup_name) {
    int err, cgRootFd, subgroupFd = 0, subgroupNodeFd;
    char pid[1000];
    char cgroup_parent[PATH_MAX];
    char *cgroup_name_unshlash;

	//remove / from cgroup name
    cgroup_name_unshlash = (char *)alloca(strlen(cgroup_name));
    replaceSlashDash(cgroup_name, cgroup_name_unshlash);

	// get current parent cgroup
	if (get_parent_cgroup(cgroup_parent) < 0) {
		goto OnErrorExit;
	}

    RedLog(REDLOG_DEBUG, "[cgroup=%s]: cgroup_parent=%s\n", cgroup_name_unshlash, cgroup_parent);

    // get fd parent cgroup
    cgRootFd = open(cgroup_parent, O_DIRECTORY);
    if (cgRootFd <= 0) {
        RedLog(REDLOG_ERROR, "[cgroups-not-found] cgroup='%s' error=%s", cgroup_name, cgroup_name, strerror(errno));
        goto OnErrorExit;
    }

    // get/create cgroup subgroup
    subgroupFd = openCreateCgroupFromDir(cgRootFd, cgroup_name_unshlash);
    if (subgroupFd < 0)
        goto OnErrorExit;


    // get/create cgroup leaf for current node
    subgroupNodeFd = openCreateCgroupFromDir(subgroupFd , "node");
    if (subgroupNodeFd < 0)
        goto OnErrorExit;

    //switch current process to node leaf cgroup
    sprintf(pid, "%d", (int)getpid());
    err = utilsFileAddControl(cgroup_name, subgroupNodeFd, "cgroup.procs", pid);
    if (err) {
        RedLog(REDLOG_ERROR, "Cannot switch current process %d to node cgroup error=%s", pid, strerror(errno));
        goto OnErrorExit;
    }

    // delegate controllers in parent cgroup
    err = utilsFileAddControl (cgroup_name, cgRootFd, "cgroup.subtree_control", "+cpuset +cpu +memory +io +pids");
    if (err) {
        RedLog(REDLOG_WARNING, "[cgroups-set-failed] node='%s' cgroups='%s' error=%s", cgroup_name, cgroup_name, strerror(errno));
    }

    if (!cgroups)
        goto OnSuccessExit;

    if (cgroups->cpuset) {
        if (cgroups->cpuset->cpus) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.cpus", cgroups->cpuset->cpus);
        if (cgroups->cpuset->mems) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.mems", cgroups->cpuset->mems);
        if (cgroups->cpuset->cpus_partition) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "cpuset.cpus.partition", cgroups->cpuset->cpus_partition);
        if (err) goto OnErrorExit;
    }

    if (cgroups->cpu) {
        // https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html
        if (cgroups->cpu->weight)  err =+ utilsFileAddControl (cgroup_name, subgroupFd, "cpu.weight", cgroups->cpu->weight);
        if (cgroups->cpu->max)  err =+ utilsFileAddControl (cgroup_name, subgroupFd, "cpu.max", cgroups->cpu->max);
        if (err) goto OnErrorExit;
    }

    if (cgroups->mem) {
        // https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html
        if (cgroups->mem->min)   err =+ utilsFileAddControl (cgroup_name, subgroupFd, "memory.min", cgroups->mem->min);
        if (cgroups->mem->max)   err =+ utilsFileAddControl (cgroup_name, subgroupFd, "memory.max", cgroups->mem->max);
        if (cgroups->mem->high) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "memory.high", cgroups->mem->high);
        if (cgroups->mem->swap_max)  err =+ utilsFileAddControl (cgroup_name, subgroupFd, "memory.swap.max", cgroups->mem->swap_max);
        if (cgroups->mem->swap_high)  err =+ utilsFileAddControl (cgroup_name, subgroupFd, "memory.swap.high", cgroups->mem->swap_high);
        if (err) goto OnErrorExit;
    }


    if (cgroups->io) {
        if (cgroups->io->max) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "io.max", cgroups->io->max);
        if (cgroups->io->weight) err =+ utilsFileAddControl (cgroup_name, subgroupFd, "io.weight", cgroups->io->weight);
        if (err) goto OnErrorExit;
    }

OnSuccessExit:
    close(cgRootFd);
    close(subgroupFd);
    close(subgroupNodeFd);
    return 0;

OnErrorExit:
    RedLog(REDLOG_ERROR,
        "\n[CGROUP ERROR]: cgroup error can be caused from several causes\n"
        "First you need to have rights to create cgroup into parent directory=%s\n"
        "if you don't, maybe it is because your not in an user session,\n"
        "or because some controllers are not delegate to children\n"
        "for that you can try: \n"
        "'systemd-run --user --pty -p \"Delegate=yes\"  bash'\n"
        "if it still fails, check %s/cgroup.controllers and %s/cgroup.subtree_control\n"
        "For controller issues see: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html",
        cgroup_parent, cgroup_parent, cgroup_parent
        );
    close(cgRootFd);
    close(subgroupFd);
    close(subgroupNodeFd);
    return -1;
} // end