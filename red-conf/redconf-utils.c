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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
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
    return (int)strftime(today, size, "%d-%b-%Y %H:%M (%Z)", localtime_r(&t, &tm));
}

/* make a fresh random UUID */
void getFreshUUID(char *uuid, size_t size)
{
    uuid_t u;
    char uu[RED_UUID_STR_LEN];
    uuid_generate(u);
    uuid_unparse_lower(u, uu);
    strncpy(uuid, uu, size);
}

static int remove_cb(const char *path, const struct stat *s, int flag, struct FTW *ftw)
{
    (void)s;
    (void)flag;
    (void)ftw;

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
mode_t RedSetUmask (const char *masktxt) {
    mode_t mask;
    if (!masktxt)
        mask = umask(0);
    else {
        char *end;
        mask = strtol(masktxt, &end, 8);
        if (*end)
            mask = umask(0);
    }
    // posix umask would need some helpers
    (void)umask(mask);
    return mask;
}

int memfd_create (const char *__name, unsigned int __flags);

// Exec a command in a memory buffer and return stdout result as FD
int MemFdExecCmd (const char* tag, const char* command, int restricted) {

    int fd = memfd_create (tag, 0);
    if (fd < 0) {
        RedLog(REDLOG_ERROR, "Failed to exec command=%s", command);
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // redirect stdout to fd and exec command
        char *argv[5];
        int idx = 0;

        argv[idx++] = "bash";
        if (restricted)
            argv[idx++] = "-r";
        argv[idx++] = "-c";
        argv[idx++] = (char*)command;
        argv[idx] = NULL;

        dup2(fd, 1);
        close (fd);
        execv("/usr/bin/bash", argv);
        RedLog(REDLOG_ERROR, "Exec failure for command=%s", command);
        _exit(1);
    }

    // wait for child to finish
    for (;;) {
        int status;
        pid_t expid = waitpid(pid, &status, 0);
        if (expid == pid) {
            /* TODO: include status in processing */
            lseek (fd, 0, SEEK_SET);
            syncfs(fd);
            return fd;
        }
        if (expid < 0 && errno != EINTR) {
            close(fd);
            RedLog(REDLOG_ERROR, "Failed to wait for command=%s", command);
            return -1;
        }
    }
}

int ExecCmd (const char* tag, const char* command, char *res, size_t size, int restricted) {
    int err = 0;
    int fd = MemFdExecCmd(tag, command, restricted);

    if (fd < 0)
        err = 3;
    else {
        ssize_t bytes = read(fd, res, size);
        if (bytes <= 0) {
            RedLog(REDLOG_WARNING, "Cannot read stdout of command %s", command);
            err = 1;
        }
        else if((size_t)bytes == size) {
            RedLog(REDLOG_WARNING, "Cannot read entire stdout of command %s (max size is=%d)", command, (int)size);
            err = 2;
        }
        else {
            res[bytes] = '\0';
        }
        close(fd);
    }

    return err;
}

int make_directories(const char *path, size_t base, size_t length, mode_t mode, size_t *existing_length)
{
    int chk = 1, rc;
    struct stat st;
    size_t idx = base;
    char tmp[length + 1];

    /* check arguments */
    if (idx > 0 && path[idx - 1] == '/')
        idx--;
    if (length == 0 || idx > length || (idx < length && path[idx] != '/')) {
        errno = EINVAL;
        return -1;
    }
    if (idx == 0) {
        if (path[0] == '/')
            idx = 1;
        else {
            while (idx < length && path[idx] != '/')
                idx++;
        }
    }
    /* check base */
    memcpy(tmp, path, idx);
    tmp[idx] = 0;
    rc = stat(tmp, &st);
    if (rc < 0)
        return rc;
    if (rc == 0 && (st.st_mode & S_IFMT) != S_IFDIR) {
        errno = ENOTDIR;
        return -1;
    }
    if (existing_length != NULL)
        *existing_length = idx;

    /* avance remaining directories */
    for ( ; ; ) {
        while(idx < length && path[idx] == '/')
            tmp[idx++] = '/';
        if (idx == length)
            return 0;
        while(idx < length && path[idx] != '/') {
            tmp[idx] = path[idx];
            idx++;
        }
        tmp[idx] = 0;
        if (chk) {
            rc = stat(tmp, &st);
            if (rc < 0)
                chk = 0;
            else if ((st.st_mode & S_IFMT) == S_IFDIR) {
                if (existing_length != NULL)
                    *existing_length = idx;
            }
            else {
                errno = ENOTDIR;
                return -1;
            }
        }
        if (!chk) {
            rc = mkdir(tmp, mode);
            if (rc < 0)
                return rc;
        }
    }
}

char *whichprog(const char *name, const char *evar, const char *dflt)
{
    size_t off, lename = 1 + strlen(name);
    char buffer[PATH_MAX];
    const char *path, *next, *result = NULL;

    /* if environment variable exist it takes precedence */
    if (evar != NULL)
        result = getenv(evar);
    /* search in path */
    for(path = secure_getenv("PATH") ; path != NULL && result == NULL ; path = next) {
        /* extract part between colons */
        for( ; *path == ':' ; path++);
        for(off = 1 ; path[off] && path[off] != ':' ; off++);
        /* prepare iteration (because off can change) */
        next = path[off] ? &path[off + 1] : NULL;
        if (off + lename < sizeof buffer) {
            /* enougth space, makes PATH/NAME in buffer */
            memcpy(buffer, path, off);
            if (off == 0 || path[off - 1] != '/')
                buffer[off++] = '/';
            memcpy(&buffer[off], name, lename);
            /* if matching an executable, its done */
            if (access(buffer, X_OK) == 0)
                result = buffer;
        }
    }
    /* return the found result or els the default value or else the name  */
    return strdup(result ?: dflt ?: name);
}

