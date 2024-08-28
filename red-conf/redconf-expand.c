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

#include "redconf-expand.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "redconf-log.h"
#include "redconf-utils.h"

#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif

typedef char*(*RedGetDefaultCbT)(const char *label, void *ctx, void*handle, char *output, size_t size);

struct RedConfDefaultsS {
    const char *label;
    RedGetDefaultCbT callback;
    void *handle;
};

static const char undef[] = "#undef";

static char*GetUuidString(const char *label, void *ctx, void*handle, char *output, size_t size) {
    getFreshUUID(output, size);
    return output;
}

static char*GetDateString(const char *label, void *ctx,  void*handle, char *output, size_t size) {
    getDateOfToday(output, size);
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

    value= secure_getenv(label);
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
    return GetEnviron(label, ctx, NULL, output, size);
}

static RedConfDefaultsT nodeConfigDefaults[] = {
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


static int defaultsExpand(const redNodeT *node,
                        const char* inputS, int *idxOut, char *outputS, int maxlen,
                        int withcmd);

/**
* This function expands in the buffer @ref outputS, of @ref maxlen length
* and indexed by *@ref idxOut, the "default value" of the @ref key of
* length @ref keylen.
*
* Expansion of defaults is using default definitions given by @ref defaults
* (that can be NULL for default defaults) and the context given by node.
*
* On termination, *@ref idxOut is updated.
*
* CAUTION: when inputS == outputS or just overlap, the result is undefined
*
* NOTE: when key is not found (or not valid) nothing is put in outputS
* and zero is returned.
*
* @param key      pointer to the first char of the key
* @param keylen   length of the key
* @param node     the node to use for contextual expansion
* @param defaults the definition of the defaults (or NULL for defult defaults)
* @param idxOut   pointer to the integer index in @ref outputS
* @param outputS  output string buffer receiving expansion
* @param maxlen   length of the output string buffer
*
* @return 0 on success, 1 if the bounds of the output buffer are reached, meaning
* that expansion process isn't complete.
*/
static int getDefault(const redNodeT *node,
                        const char *key, int keylen,
                        int *idxOut, char *outputS, int maxlen) {

    /* serach the key in defaults */
    RedConfDefaultsT *iter = defaults ?: nodeConfigDefaults;
    for (; iter->label ; iter++) {
        if (!memcmp (key, iter->label, (size_t)keylen) && !iter->label[keylen]) {
            /* get its value in local buffer */
            char value[MAX_CYAML_FORMAT_STR];
            iter->callback(iter->label, (void*)node, iter->handle, value, sizeof value);
            /* recursive expansion of found value in outputS */
            return defaultsExpand(node, value, idxOut, outputS, maxlen, 0);
        }
    }
    return 0;
}

/**
* This function computes the length matching a key name in the string @ref inputS
*
* @param inputS the string to scan for finding key length
*
* @return the length of the valid keyname starting the string inputS, can be 0
*/
static int computeKeyLength(const char *inputS) {
    int len = 0, npar = 0;
    for (;;len++) {
        char c = inputS[len];

        /* end of the string */
        if (c == '\0')
            break;

        if (c == '(' && (len == 0 || npar > 0))
            /* begin or nested command */
            npar++;
        else if (npar > 0) {
            /* in command */
            if (c == ')' && !--npar)
                /* end of command (include it) */
                return len + 1;
        }
        else {
            /* not in command */
            if (c < '0' || c > 'z')
                break;
        }
    }
    return npar ? 0 : len;
}

/**
* This function expands in the buffer @ref outputS, of @ref maxlen length
* and indexed by *@ref idxOut, the "default values" of the keys expansions
* specified in @ref inputS.
*
* Expansion of defaults is using default definitions given by @ref defaults
* (that can be NULL for default defaults) and the context given by node.
*
* On termination, *@ref idxOut is updated.
*
* CAUTION: when inputS == outputS or just overlap, the result is undefined
*
* @param node     the node to use for contextual expansion
* @param defaults the definition of the defaults (or NULL for defult defaults)
* @param inputS   input string buffer to be expanded
* @param idxOut   pointer to the integer index in @ref outputS
* @param outputS  output string buffer receiving expansion
* @param maxlen   length of the output string buffer
* @param withcmd  accept commands substitution (0=no, 1=restricted, >1=full)
*
* @return 0 on success, 1 if the bounds of the output buffer are reached, meaning
* that expansion process isn't complete.
*/
static int defaultsExpand(const redNodeT *node,
                        const char* inputS, int *idxOut, char *outputS, int maxlen,
                        int withcmd) {

    // search for a $within string input format
    for (int idxIn=0; inputS[idxIn] != '\0'; idxIn++) {

        /* if escaped: iterate without adding \ */
        if (inputS[idxIn] == '\\' && inputS[idxIn+1] == '$') {
            if (*idxOut == maxlen)
                goto OnErrorExit;
            outputS[(*idxOut)++] = inputS[++idxIn];
        }
        /* ignore escape $=\$ */
        else if (inputS[idxIn] == '$' && (idxIn == 0 || inputS[idxIn - 1] != '\\')) {

            const char *key = &inputS[idxIn + 1];
            int keylen = computeKeyLength(key);

            idxIn += keylen;
            if (keylen >= 2 && *key == '(' && withcmd > 0) {

                /* command expansion */
                char pre_command[keylen - 1];
                char command[MAX_CYAML_FORMAT_STR];
                int lencmd = 0;

                /* prepare the command to expand */
                strncpy(pre_command, &key[1], (size_t)(keylen - 2));
                pre_command[keylen - 2] = 0;

                /* commands can contains substitution (with or without commands?) */
                if (defaultsExpand(node, pre_command, &lencmd, command, sizeof command, 0))
                    goto OnErrorExit;
                command[lencmd] = '\0';

                /* execute the command and get its output in outputS */
                if(ExecCmd("command", command, &outputS[*idxOut], maxlen - *idxOut, withcmd == 1))
                    goto OnErrorExit;

                /* update output pointer */
                *idxOut += strlen(&outputS[*idxOut]);
            }
            else {
    
                /* default expansion */
                if (getDefault(node, key, keylen, idxOut, outputS, maxlen))
                    goto OnErrorExit;
            }
        } else {
            if (*idxOut == maxlen)
                goto OnErrorExit;
            outputS[(*idxOut)++] = inputS[idxIn];
        }
    }
    return 0;
OnErrorExit:
    return 1;
}

/* see redconf-expand.h */
int RedConfAppendEnvKey (const redNodeT *node,
                            char *outputS, int *idxOut, int maxlen,
                            const char *inputS, const char* prefix, const char *suffix) {

    int idx, idxout0, idxout1, idxout2;

    /* copy the prefix */
    idxout0 = *idxOut;
    if (prefix) {
        for (idx=0; prefix[idx]; idx++) {
            if (*idxOut >= maxlen)
                goto OnErrorExit;
            outputS[(*idxOut)++]=prefix[idx];
        }
    }

    /* expand the middle */
    idxout1 = *idxOut;
    if (defaultsExpand(node, inputS, idxOut, outputS, maxlen, 0))
        goto OnErrorExit;

    /* copy the suffix */
    idxout2 = *idxOut;
    if (idxout2 == idxout1) {
        /* nothing added, remove prefix */
        *idxOut = idxout0;
    }
    else {
        if (suffix) {
            for (idx=0; suffix[idx]; idx++) {
                if (*idxOut >= maxlen)
                    goto OnErrorExit;
                outputS[(*idxOut)++]=suffix[idx];
            }
        }
    }

    /* append a zero */
    if (*idxOut >= maxlen)
        goto OnErrorExit;
    outputS[(*idxOut)]='\0';
    return 0;

OnErrorExit:
    return 1;
}

/* see redconf-expand.h */
char *RedGetDefaultExpand(const redNodeT *node, const char* key) {
    int idxOut = 0;
    char outputS[MAX_CYAML_FORMAT_STR];

    if (getDefault(node, key, (int)strlen(key), &idxOut, outputS, MAX_CYAML_FORMAT_STR))
        return NULL;

    return strndup(outputS, (size_t)idxOut);
}

/* see redconf-expand.h */
char *RedNodeStringExpand (const redNodeT *node, const char* inputS) {
    int idxOut = 0;
    char outputS[MAX_CYAML_FORMAT_STR];

    if (defaultsExpand(node, inputS, &idxOut, outputS, MAX_CYAML_FORMAT_STR, 1))
        return NULL;;

    return strndup(outputS, (size_t)idxOut);
}

/* see redconf-expand.h */
char *expandAlloc(const redNodeT *node, const char *input, int expand) {
    if (!input)
        return NULL;

    if(!expand)
        return strdup(input);

    return RedNodeStringExpand(node, input);
}

