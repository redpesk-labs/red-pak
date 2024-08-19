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

#include <string.h>

#include "redconf-log.h"
#include "redconf-utils.h"

#ifndef MAX_CYAML_FORMAT_ENV
#define MAX_CYAML_FORMAT_ENV 10
#endif
#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif


static int stringExpand(const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, int *idxOut, char *outputS) {
    int count = 0;
    // search for a $within string input format
    for (int idxIn=0; inputS[idxIn] != '\0'; idxIn++) {


        /* if escaped: iterate without adding \ */
        if (inputS[idxIn] == '\\' && inputS[idxIn+1] == '$') {
            if (*idxOut == MAX_CYAML_FORMAT_STR)  goto OnErrorExit;
            outputS[(*idxOut)++] = inputS[++idxIn];
        }
        /* ignore escape $=\$ */
        else if (inputS[idxIn] == '$' && (idxIn == 0 || inputS[idxIn - 1] != '\\')) {
            if (count == MAX_CYAML_FORMAT_ENV) goto OnErrorExit;

            //check command to exec is $()
            if(inputS[idxIn + 1] == '(') {
                int command_size = 0;
                while(inputS [idxIn + 2 + command_size] != ')') {
                    if (inputS [idxIn + 2 + command_size] == '\0') {
                        RedLog(REDLOG_WARNING, "No end parenthesis ')' found in %s: Ignoring", inputS+idxIn);
                        command_size = 0;
                        break;
                    }
                    command_size++;
                }

                if (command_size > 0) {
                    char command_res[100];
                    char command[command_size + 1];
                    size_t command_res_size = 0;

                    strncpy(command, inputS+idxIn+2, command_size);
                    command[command_size] = '\0';
                    if(ExecCmd(command, command, command_res, sizeof command_res, 1)) goto OnErrorExit;
                    command_res_size = strlen(command_res);

                    //copy command res to output
                    strncpy(outputS+(*idxOut), command_res, command_res_size + 1);
                    *idxOut += command_res_size;
                    idxIn += 2 + command_size; //2: because 2 parenthesis
                }
            } else {
                if(RedConfGetEnvKey (node, defaults, &idxIn, inputS, idxOut, outputS, MAX_CYAML_FORMAT_STR)) goto OnErrorExit;
            }
            count ++;
        } else {
            if (*idxOut == MAX_CYAML_FORMAT_STR)  goto OnErrorExit;
            outputS[(*idxOut)++] = inputS[idxIn];
        }
    }
    return 0;
OnErrorExit:
    return 1;
}


static int RedGetDefault(const char *envkey, RedConfDefaultsT *defaults, const redNodeT * node, int *idxOut, char *outputS, int maxlen) {

    char envval[MAX_CYAML_FORMAT_STR] = {0};
    for (int idx=0; defaults[idx].label; idx++) {
        if (!strcmp (envkey, defaults[idx].label)) {
            (*(RedGetDefaultCbT)defaults[idx].callback) (defaults[idx].label, (void*)node, defaults[idx].handle, envval, MAX_CYAML_FORMAT_STR);
            break;
        }
    }

    //try to expand envkey
    char envvalExp[MAX_CYAML_FORMAT_STR];
    int idxOutExp = 0;
    stringExpand(node, defaults, envval, &idxOutExp, envvalExp);
    envvalExp[idxOutExp] = '\0';

    for (int jdx=0; envvalExp[jdx] != '\0'; jdx++) {
        if (*idxOut >= maxlen) goto OnErrorExit;
        outputS[(*idxOut)++]= envvalExp[jdx];
    }

    return 0;
OnErrorExit:
    return 1;
}

// Extract $KeyName and replace with $Key Env or default Value
int RedConfGetEnvKey (const redNodeT *node, RedConfDefaultsT *defaults, int *idxIn, const char *inputS, int *idxOut, char *outputS, int maxlen) {
    char envkey[64];
    char *envval;

    // get envkey from $ to any 1st non alphanum character
    for (int idx=0; inputS[*idxIn] != '\0'; idx++) {

        (*idxIn)++; // intial $ is ignored
        if (inputS[*idxIn] >= '0' && inputS[*idxIn] <= 'z') {
            if (idx == sizeof(envkey)) goto OnErrorExit;
            envkey[idx]= inputS[*idxIn] & ~32; // move to uppercase
        } else {
            (*idxIn)--; // keep input separation character
            envkey[idx]='\0';
            break;
        }
    }

    // search for a default key
    if(RedGetDefault(envkey, defaults, node, idxOut, outputS, maxlen)) goto OnErrorExit;

    return 0;

OnErrorExit:
    envval="#UNKNOWN_KEY";
    for (int jdx=0; envval[jdx]; jdx++) {
        if (*idxOut >= maxlen) {
            outputS[(*idxOut)-1] = '\0';
            return 1;
        }
        outputS[(*idxOut)++]= envval[jdx];
    }
    return 1;
}

int RedConfAppendEnvKey (const redNodeT *node, char *outputS, int *idxOut, int maxlen, const char *inputS,  RedConfDefaultsT *defaults, const char* prefix, const char *trailler) {
    int err;

    if (!defaults) defaults=nodeConfigDefaults;

    if (prefix) {
        for (int idx=0; prefix[idx]; idx++) {
            if (*idxOut >= maxlen) goto OnErrorExit;
            outputS[(*idxOut)++]=prefix[idx];
        }
    }

    // search for a $within string input format
    for (int idxIn=0; inputS[idxIn] != '\0'; idxIn++) {

        if (inputS[idxIn] != '$') {
            if (*idxOut >= maxlen)  goto OnErrorExit;
            outputS[(*idxOut)++] = inputS[idxIn];

        } else {
            err = RedConfGetEnvKey (node, defaults, &idxIn, inputS, idxOut, outputS, maxlen);
            if (err) goto OnErrorExit;
        }
    }

    if (trailler) {
        for (int idx=0; trailler[idx]; idx++) {
            if (*idxOut >= maxlen) goto OnErrorExit;
            outputS[(*idxOut)++]=trailler[idx];
        }
    }

    // close string
    outputS[(*idxOut)]='\0';

    return 0;

OnErrorExit:
    return 1;
}

char *RedGetDefaultExpand(const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS) {
    int idxOut = 0;
    char outputS[MAX_CYAML_FORMAT_STR];

    if (!defaults) defaults=nodeConfigDefaults;

    if (RedGetDefault(inputS, defaults, node, &idxOut, outputS, MAX_CYAML_FORMAT_STR)) return NULL;

    // close the string
    outputS[idxOut]='\0';

    return strdup(outputS);
}

// Expand string with environnement variable
char *RedNodeStringExpand (const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, const char* prefix, const char* trailler) {
    int idxOut=0;
    char outputS[MAX_CYAML_FORMAT_STR] = {0};

    if (!defaults) defaults=nodeConfigDefaults;

    if (prefix) {
        for (int idx=0; prefix[idx]; idx++) {
            if (idxOut == MAX_CYAML_FORMAT_STR) goto OnErrorExit;
            outputS[idxOut]=prefix[idx];
            idxOut++;
        }
    }

    stringExpand(node, defaults, inputS, &idxOut, outputS);

    // if we have a trailler add it now
    if (trailler) {
        for (int idx=0; trailler[idx]; idx++) {
            if (idxOut == MAX_CYAML_FORMAT_STR) goto OnErrorExit;
            outputS[idxOut]=trailler[idx];
            idxOut++;
        }
    }

    // close the string
    outputS[idxOut]='\0';

    // string is formated replace original with expanded value
    return strdup(outputS);

OnErrorExit:
    RedLog(REDLOG_ERROR, "Fail Expending cyaml entry=%s value=%s err=[Len or $ count]\n",node->config->headers->name, inputS);
    return NULL;
}

char *expandAlloc(const redNodeT *node, const char *input, int expand) {
    if (!input)
        return NULL;

    if(expand)
        return RedNodeStringExpand(node, NULL, input, NULL, NULL);
    else
        return strdup(input);
}

