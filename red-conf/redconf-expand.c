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
#include "redconf-defaults.h"
#include "redconf-utils.h"

#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif


typedef struct vardef vardef_t;

typedef int (*varcb_t)(const redNodeT *node, const vardef_t *vardef, char *output, size_t size);

struct vardef {
    const char *key;
    varcb_t     callback;
    const char *svalue;
    int         ivalue;
};

static int GetNewUUID(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    (void)vardef;
    if (size < RED_UUID_STR_LEN)
        return 1;
    getFreshUUID(output, size);
    return 0;
}

static int GetDateString(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    (void)vardef;
    return !getDateOfToday(output, size);
}

static int GetDecimal(long long unsigned value, char *output, size_t size) {
    int rc = snprintf(output, size, "%llu", value);
    return rc < 0 || (size_t)rc >= size;
}
static int GetUid(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    (void)vardef;
    return GetDecimal((long long unsigned)getuid(), output, size);
}

static int GetGid(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    (void)vardef;
    return GetDecimal((long long unsigned)getgid(), output, size);
}

static int GetPid(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    (void)vardef;
    return GetDecimal((long long unsigned)getpid(), output, size);
}

static int PutString(const char *value, char *output, size_t size) {
    size_t idx = 0;
    while (idx < size && (output[idx] = value[idx]) != '\0')
        idx++;
    return idx >= size;
}

static int GetUndefined(const char *key, char *output, size_t size) {
    static const char undef[] = "#undef:";
    if (size < sizeof undef -1)
        return 1;
    memcpy(output, undef, sizeof undef - 1);
    return PutString(key, &output[sizeof undef - 1], size - (sizeof undef - 1));
}

static int GetString(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    return PutString(vardef->svalue, output, size);
}

static int GetEnviron(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    (void)node;
    const char *value = secure_getenv(vardef->key);
    if (value == NULL)
        value = vardef->svalue;
    if (value == NULL)
        return GetUndefined(vardef->key, output, size);
    return PutString(value, output, size);
}

typedef enum {
    REDNODE_INFO_ALIAS,
    REDNODE_INFO_NAME,
    REDNODE_INFO_PATH,
    REDNODE_INFO_INFO,
} RednodeInfoE;

static int GetThisNodeInfo(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    const char *value = NULL;

    switch (vardef->ivalue) {
    case REDNODE_INFO_ALIAS:
        if (node && node->config && node->config->headers)
            value = node->config->headers->alias;
        break;
    case REDNODE_INFO_NAME:
        if (node && node->config && node->config->headers)
            value = node->config->headers->name;
        break;
    case REDNODE_INFO_PATH:
        if (node)
            value = node->redpath;
        break;
    case REDNODE_INFO_INFO:
        if (node && node->config && node->config->headers)
            value = node->config->headers->info;
        break;
    default:
        break;
    }

    if (value == NULL)
        return GetUndefined(vardef->key, output, size);

    return PutString(value, output, size);
}

static int GetNodeInfo(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    return GetThisNodeInfo(node, vardef, output, size);
}

static int GetLeafInfo(const redNodeT *node, const vardef_t *vardef, char *output, size_t size) {
    return GetThisNodeInfo(node->leaf ?: node, vardef, output, size);
}

static const vardef_t vardefs[] = {
    // static strings
    { "NODE_PREFIX"    , GetEnviron,  NODE_PREFIX, 0 },
    { "redpak_MAIN"    , GetEnviron,  "$NODE_PREFIX"redpak_MAIN, 0 },
    { "redpak_TMPL"    , GetEnviron,  "$NODE_PREFIX"redpak_TMPL, 0 },
    { "REDNODE_CONF"   , GetString,   "$NODE_PATH/"REDNODE_CONF, 0 },
    { "REDNODE_ADMIN"  , GetString,   "$NODE_PATH/"REDNODE_ADMIN, 0 },
    { "REDNODE_STATUS" , GetString,   "$NODE_PATH/"REDNODE_STATUS, 0 },
    { "REDNODE_VARDIR" , GetString,   "$NODE_PATH/"REDNODE_VARDIR, 0 },
    { "REDNODE_REPODIR", GetString,   "$NODE_PATH/"REDNODE_REPODIR, 0 },
    { "REDNODE_LOCK"   , GetString,   "$NODE_PATH/"REDNODE_LOCK, 0 },
    { "USER"           , GetEnviron,  "Unknown", 0 },
    { "LOGNAME"        , GetEnviron,  "Unknown", 0 },
    { "HOSTNAME"       , GetEnviron,  "localhost", 0 },
    { "CGROUPS_MOUNT_POINT", GetEnviron, CGROUPS_MOUNT_POINT, 0 },
    { "PID"            , GetPid,      NULL, 0 },
    { "UID"            , GetUid,      NULL, 0 },
    { "GID"            , GetGid,      NULL, 0 },
    { "TODAY"          , GetDateString, NULL, 0 },
    { "UUID"           , GetNewUUID,  NULL, 0 },
    { "NODE_ALIAS"     , GetNodeInfo, NULL, REDNODE_INFO_ALIAS },
    { "NODE_NAME"      , GetNodeInfo, NULL, REDNODE_INFO_NAME },
    { "NODE_PATH"      , GetNodeInfo, NULL, REDNODE_INFO_PATH },
    { "NODE_INFO"      , GetNodeInfo, NULL, REDNODE_INFO_INFO },
    { "LEAF_ALIAS"     , GetLeafInfo, NULL, REDNODE_INFO_ALIAS },
    { "LEAF_NAME"      , GetLeafInfo, NULL, REDNODE_INFO_NAME },
    { "LEAF_PATH"      , GetLeafInfo, NULL, REDNODE_INFO_PATH },
    { "LEAF_INFO"      , GetLeafInfo, NULL, REDNODE_INFO_INFO },
    { "REDPESK_VERSION", GetEnviron,  REDPESK_DFLT_VERSION, 0 }
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
    const vardef_t *iter = vardefs;
    const vardef_t *end = &vardefs[sizeof vardefs / sizeof *vardefs];
    for (; iter != end ; iter++) {
        if (!memcmp (key, iter->key, (size_t)keylen) && !iter->key[keylen]) {
            /* get its value in local buffer */
            char value[MAX_CYAML_FORMAT_STR];
            int rc = iter->callback(node, iter, value, sizeof value);
            /* recursive expansion of found value in outputS */
            return rc != 0 ? rc : defaultsExpand(node, value, idxOut, outputS, maxlen, 0);
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
        return NULL;

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

/* see redconf-expand.h */
int RedConfAppendPath(const redNodeT *node,
                      char *outputS, int *idxOut, int maxlen,
                      const char *inputS
) {
    int idx, idxbeg, idxend, idxrd, idxwr, idxse;

    /* copy the prefix */
    idxbeg = *idxOut;
    if (idxbeg >= maxlen)
        return 1;

    /* add default separator */
    if (idxbeg > 0)
        outputS[idxbeg++] = ':';

    /* expand the middle */
    *idxOut = idxbeg;
    if (defaultsExpand(node, inputS, idxOut, outputS, maxlen, 0))
        return 1;

    /* copy the suffix */
    idxend = *idxOut;
    idxrd = idxwr = idxbeg;
    while (idxrd < idxend) {
        /* remove heading colon */
        while (idxrd < idxend && outputS[idxrd] == ':')
            idxrd++;

        /* search any previously existing path */
        idxse = 0;
        while (idxse < idxwr) {
            while (idxse < idxwr && outputS[idxse] == ':')
                idxse++;
            if (idxse < idxwr) {
                idx = 0;
                while (idxse + idx < idxwr
                    && outputS[idxse + idx] != ':'
                    && idxrd + idx < idxend
                    && outputS[idxse + idx] == outputS[idxrd + idx])
                    idx++;
                if (idxse + idx < idxwr
                    && outputS[idxse + idx] == ':'
                    && (idxrd + idx == idxend || outputS[idxrd + idx] == ':')) {
                    break;
                }
                else {
                    idxse += idx;
                    while (idxse < idxwr && outputS[idxse] != ':')
                        idxse++;
                }
            }
        }

        if (idxse == idxwr)
            /* not found, copy */
            while (idxrd < idxend && (outputS[idxwr++] = outputS[idxrd++]) != ':');
        else
            /* found so skip */
            while (idxrd < idxend && outputS[idxrd++] != ':');
    }

    /* remove tailing colon */
    while (idxwr > 0 && outputS[idxwr - 1] == ':')
        idxwr--;

    /* append a zero */
    outputS[*idxOut = idxwr] = 0;
    return 0;
}

