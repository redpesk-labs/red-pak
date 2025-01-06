/*
* Copyright (C) 2020-2025 IoT.bzh Company
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

#ifndef _RED_CONF_UTILS_INCLUDE_
#define _RED_CONF_UTILS_INCLUDE_

#define STATIC_STR_CONCAT(S1,S2) S1 S2

#include <stddef.h>
#include <sys/types.h>

/* make the current date in today. return 0 in case of error else the length of the text */
extern int getDateOfToday(char *today, size_t size);

/* make a fresh random UUID */
#define RED_UUID_STR_LEN 37
extern void getFreshUUID(char *uuid, size_t size);

int remove_directories(const char *path);
unsigned long RedUtcGetTimeMs ();

mode_t RedSetUmask (const char *masktxt);

/**
 * Checks if path1 and path2 are refering or not to the same file.
 *
 * @param path1 path to one file
 * @param path2 path to the other file
 *
 * @return 1 when paths are referring the same file or 0 otherwise
 */
extern int RedConfIsSameFile(const char* path1, const char* path2);

/**
 * Executes the @ref command in a forked shell using "/usr/bin/bash -c 'command'"
 * If @ref restricted is not zero the --restricted option is passed to the shell.
 * Wait for the completion of the command and then returns a file descriptor that
 * when read returns the output of the command.
 *
 * @param tag        a tag for debugging
 * @param command    the command to be executed
 * @param restricted a flag telling if the shell is restricted or not
 *
 * @return A negative value on error, or if positive or null, a file descriptor to be used
 * for getting output of the command. The file decriptor, obviously, must be closed by the
 * caller.
 */
extern int MemFdExecCmd(const char* tag, const char* command, int restricted);

/**
 * Executes the @ref command in a forked shell using "/usr/bin/bash -c 'command'"
 * If @ref restricted is not zero the --restricted option is passed to the shell.
 * Wait for the completion of the command and put its output in the given buffer.
 *
 * @param tag        a tag for debugging
 * @param command    the command to be executed
 * @param buffer     the buffer receiving the output of the command
 * @param size       the length of @ref buffer in bytes
 * @param restricted a flag telling if the shell is restricted or not
 *
 * @return 0 on success, 1 on read error, 2 on buffer overflow, 3 on command error.
 */
extern int ExecCmd(const char* tag, const char* command, char *buffer, size_t size, int restricted);

/**
* Ensure that the given directory exists by on need creating it and any of its parent until base.
* The given path is iteratively modified and restored. At end it is unmodified.
*
* @param path   the path
* @param base   the base size (that must exist)
* @param length the length of the directory to ensure
* @param mode   mode of creation of directories (see mkdir, section 2)
* @param existing_length receive the length of the already existing directories (can be NULL)
* @return 0 on success or -1 on error
*/
extern int make_directories(const char *path, size_t base, size_t length, mode_t mode, size_t *existing_length);

/**
 * Locate the executable of name
 * @param name    name of the command
 * @param evar    name of an environment variable (or NULL) overwriting default search
 * @param dflt    default value if nothing is found (or NULL)
 * @return a freshly allocated path for the program or NULL or allocation error
 */
extern char *whichprog(const char *name, const char *evar, const char *dflt);

/**
 * Locate the mount point of cgroups and return it
 */
extern const char *cgroup_root();

#endif