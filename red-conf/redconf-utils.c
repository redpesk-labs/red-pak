/*
* Copyright (C) 2020 "IoT.bzh"
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

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ftw.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <uuid/uuid.h>

#include "redconf-utils.h"
#include "redconf-log.h"

#ifndef MAX_CYAML_FORMAT_ENV
#define MAX_CYAML_FORMAT_ENV 10
#endif
#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif

/* make the current date in today. return 0 in case of error else the length of the text */
int getDateOfToday(char *today, size_t size)
{
    struct tm tm;
    time_t t = time(NULL);
    return strftime(today, size, "%d-%b-%Y %H:%M (%Z)", localtime_r(&t, &tm));
}

/* make a fresh random UUID */
void getFreshUUID(char *uuid, size_t size)
{
    uuid_t u;
    char uu[UUID_STR_LEN];
    uuid_generate(u);
    uuid_unparse_lower(u, uu);
    strncpy(uuid, uu, size);
}

static int remove_cb(const char *path, const struct stat *s, int flag, struct FTW *ftw)
{
    int err;
    err = remove(path);

    if (err)
        RedLog(REDLOG_ERROR, "Cannot remove %s error=%s", path, strerror(errno));

    return err;
}

int remove_directories(const char *path)
{
    return nftw(path, remove_cb, 64, FTW_DEPTH | FTW_PHYS);
}

// Return current UTC time in ms
unsigned long RedUtcGetTimeMs() {
    struct timeval tp;

    // get UTC time
    gettimeofday(&tp, NULL);
    return (unsigned long)(tp.tv_sec * 1000 + tp.tv_usec / 1000);
}

/* see redconf-utils.h */
int RedConfIsSameFile(const char* path1, const char* path2) {
    struct stat st1, st2;
    return stat(path1, &st1) == 0
        && stat(path2, &st2) == 0
        && st1.st_ino == st2.st_ino
        && st1.st_dev == st2.st_dev;
}


// if string is not null extract umask and apply
mode_t RedSetUmask (redConfTagT *conftag) {
    mode_t mask;
    if (!conftag || !conftag->umask) {
        mask= umask(0);
    } else {
        sscanf (conftag->umask, "%o", &mask);
    }
    // posix umask would need some helpers
    (void)umask(mask);
    return mask;
}

int memfd_create (const char *__name, unsigned int __flags);

// Exec a command in a memory buffer and return stdout result as FD
int MemFdExecCmd (const char* mount, const char* command) {
    int fd = memfd_create (mount, 0);
    if (fd <0) goto OnErrorExit;

    int pid = fork();
    if (pid != 0) {
        // wait for child to finish
        (void)wait(NULL);
        lseek (fd, 0, SEEK_SET);
        syncfs(fd);
    } else {
        // redirect stdout to fd and exec command
        char *argv[4];
        argv[0]="bash";
        argv[1]="-c";
        argv[2]=(char*)command;
        argv[3]=NULL;

        dup2(fd, 1);
        close (fd);
        execv("/usr/bin/bash", argv);
        fprintf (stderr, "hoops: red-wrap exec command return command=%s\n", command);
    }

    return fd;

OnErrorExit:
    fprintf (stderr, "error: red-wrap Fail to exec command=%s\n", command);
    return -1;
}

int ExecCmd (const char* mount, const char* command, char *res, size_t size) {
    int err = 0;
    int fd = MemFdExecCmd(mount, command);

    ssize_t bytes = read(fd, res, size);
    if (bytes <= 0) {
        RedLog(REDLOG_WARNING, "Cannot read stdout of command %s", command);
        err = 1;
        goto Error;
    } else if(bytes >= 100) {
        RedLog(REDLOG_WARNING, "Cannot read entire stdout of command %s (max size is=%d)", command, size);
        err = 2;
        goto Error;
    }

    res[bytes] = '\0';

Error:
    close(fd);
    return err;
}
