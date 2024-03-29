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
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <uuid/uuid.h>
#include <time.h>

#include "redconf.h"

static const char undef[] = "#undef";

static char*GetUuidString(const char *label, void *ctx, void*handle, char *output, size_t size) {
    uuid_t binuuid;

    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, output);

    return output;
}

static char*GetDateString(const char *label, void *ctx,  void*handle, char *output, size_t size) {
    time_t now= time(NULL);
    struct tm *time= localtime(&now);

    strftime (output, size, "%d-%b-%Y %h:%M (%Z)",time);
    return output;
}

static char*GetUid(const char *label, void *ctx, void*handle, char *output, size_t size) {
    uid_t uid= getuid();
    snprintf (output, size, "%u",uid);
    return output;
}

static char*GetGid(const char *label, void *ctx, void*handle, char *output, size_t size) {
    gid_t gid= getgid();
    snprintf (output, size, "%d",gid);
    return output;
}

static char*GetPid(const char *label, void *ctx,  void*handle, char *output, size_t size) {
    pid_t pid= getpid();
    snprintf (output, size, "%d", pid);
    return output;
}

static char * GetEnviron(const char *label, void *ctx, void*handle, char *output, size_t size) {
    const char*key= handle;
    const char*value;

    value= getenv(label);
    if (!value) {
        if (key) {
            value=(char *)key;
        } else {
            value=undef;
        }
    }
    strncpy(output, value, size);
    return output;
}

typedef enum {
    REDNODE_INFO_ALIAS,
    REDNODE_INFO_NAME,
    REDNODE_INFO_PATH,
    REDNODE_INFO_INFO,
} RednodeInfoE;

static char*GetNodeInfo(const char *label, void *ctx, void*handle, char *output, size_t size) {
    redNodeT *node =ctx;
    const char*value;
    if (!node) goto OnErrorExit;

    switch ((RednodeInfoE)handle) {
        case REDNODE_INFO_ALIAS:
            value= node->config->headers->alias;
            break;
        case REDNODE_INFO_NAME:
            value= node->config->headers->name;
            break;
        case REDNODE_INFO_PATH:
            value= node->redpath;
            break;
        case REDNODE_INFO_INFO:
            value= node->config->headers->info;
            break;
        default:
            break;
    }

    if (!value) value = undef;
    strncpy(output, value, size);

    return output;

OnErrorExit:
    return GetEnviron(label, ctx, handle, output, size);
}

// Warning: REDDEFLT_CB will get its return free
RedConfDefaultsT nodeConfigDefaults[]= {
    // static strings
    {"NODE_PREFIX"    , GetEnviron, (void*)NODE_PREFIX},
    {"redpak_MAIN"    , GetEnviron, (void*)"$NODE_PREFIX"redpak_MAIN},
    {"redpak_TMPL"    , GetEnviron, (void*)"$NODE_PREFIX"redpak_TMPL},
    {"REDNODE_CONF"   , GetEnviron, (void*)"$NODE_PATH/"REDNODE_CONF},
    {"REDNODE_ADMIN"  , GetEnviron, (void*)"$NODE_PATH/"REDNODE_ADMIN},
    {"REDNODE_STATUS" , GetEnviron, (void*)"$NODE_PATH/"REDNODE_STATUS},
    {"REDNODE_VARDIR" , GetEnviron, (void*)"$NODE_PATH/"REDNODE_VARDIR},
    {"REDNODE_REPODIR", GetEnviron, (void*)"$NODE_PATH/"REDNODE_REPODIR},
    {"REDNODE_LOCK"   , GetEnviron, (void*)"$NODE_PATH/"REDNODE_LOCK},
    {"LOGNAME"        , GetEnviron, (void*)"Unknown"},
    {"HOSTNAME"       , GetEnviron, (void*)"localhost"},
    {"CGROUPS_MOUNT_POINT", GetEnviron, (void*)CGROUPS_MOUNT_POINT},
    {"LEAF_ALIAS"     , GetEnviron, NULL},
    {"LEAF_NAME"      , GetEnviron, NULL},
    {"LEAF_PATH"      , GetEnviron, NULL},

    {"PID"            , GetPid, NULL},
    {"UID"            , GetUid, NULL},
    {"GID"            , GetGid, NULL},
    {"TODAY"          , GetDateString, NULL},
    {"UUID"           , GetUuidString, NULL},

    {"NODE_ALIAS"     , GetNodeInfo, (void*) REDNODE_INFO_ALIAS},
    {"NODE_NAME"      , GetNodeInfo, (void*) REDNODE_INFO_NAME},
    {"NODE_PATH"      , GetNodeInfo, (void*) REDNODE_INFO_PATH},
    {"NODE_INFO"      , GetNodeInfo, (void*) REDNODE_INFO_INFO},

    {"REDPESK_VERSION", GetEnviron, REDPESK_DFLT_VERSION},

    {NULL} /* sentinel */
};
