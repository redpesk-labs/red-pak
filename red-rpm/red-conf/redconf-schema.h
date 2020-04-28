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

    typedef enum {
        RED_EXPORT_PRIVATE=0,
        RED_EXPORT_RESTRICTED,
        RED_EXPORT_PUBLIC,
        RED_EXPORT_ANONYMOUS,
    } redExportFlagE;
    extern const cyaml_strval_t exportFlagStrings[];

    typedef struct {
        const char *mount;
        const char *path;
        redExportFlagE mode;
    } redConfExportPathT;

    typedef struct {
        const char *old;
        const char *new;
    } redConfRelocationT;

    typedef struct {
        const char *rpmdir;
        const char *persistdir;
        const char *path;
        const char *ldpath;
        unsigned int gpgcheck;
        unsigned int inherit;
        unsigned int forcemod;
        unsigned int maxage;
        unsigned int umask;
        int verbose;
    } redConfVarT;


    typedef struct {
        redConfAclT *acl;
        redConfHeaderT *headers;
        redConfVarT  *environment;
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

// --- Redpath Node Directory Hierarchy (from leaf to root)
typedef struct redNodeS{
    redStatusT *status;
    redConfigT *config;
    struct redNodeS *ancestor;
    const char *redpath;
} redNodeT;

redStatusT* RedLoadStatus (const char* filepath, int warning);
redConfigT* RedLoadConfig (const char* filepath, int warning);
int RedSaveStatus (const char* filepath, redStatusT *status, int warning);
int RedSaveConfig (const char* filepath, redConfigT *config, int warning);


#endif // _REDCONFIG_INCLUDE_
