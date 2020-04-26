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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include "redconf.h"

#ifndef MAX_CYAML_FORMAT_ENV
#define MAX_CYAML_FORMAT_ENV 10
#endif
#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif

// Allow log function to be mapped on RedLog, syslog, ...
static RedLogCbT *redLogRegisteredCb;
void RedLogRegister (RedLogCbT *redlogcb) {

    redLogRegisteredCb= redlogcb;
}
// redlib and redconf use RedLog to display error messages
void RedLog (RedLogLevelE level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    if (redLogRegisteredCb) {
        (*redLogRegisteredCb) (level, format, args);
    } else {
        vfprintf (stderr, format, args);
        fprintf(stderr,"\n");
    }
    va_end(args);
}

void RedDumpStatusHandle (redStatusT *status) {
   // retrieve label attach to state flag
    const char* flag = statusFlagStrings[status->state].str;

    printf ("---\n");
    printf ("  state=%s'\n", flag);
    printf ("  realpath=%s'\n", status->realpath);
    printf ("  timestamp=%d'\n", status->timestamp);
    printf ("  info=%s'\n", status->info);
    printf ("---\n\n");
}


// Dump config filr, in verbose mode print ongoing parsing to help find errors
int RedDumpStatusPath (const char *filepath, int warning) {

    redStatusT *status;
    cyaml_log_t logLevel;

    status = RedLoadStatus(filepath, warning);
    if (!status) goto OnErrorExit;
    printf ("# Dumping yaml red-status path=%s\n\n", filepath);
    RedDumpStatusHandle(status);
    return 0;

OnErrorExit:
    return 1;    
}

void RedDumpConfigHandle(redConfigT *config) {

    printf ("---\n");
    printf ("headers=> alias=%s name=%s info='%s'\n", config->headers->alias, config->headers->name, config->headers->info);
    printf ("packages=> persistdir=%s gpgcheck=%d\n", config->conftag->persistdir, config->conftag->gpgcheck);
    printf ("exports:\n");
    for (int idx=0; idx < config->exports_count; idx++) {

        redExportFlagE mode= config->exports[idx].mode;
        const char* mount= config->exports[idx].mount;
        const char* path=config->exports[idx].path;

        printf ("- [%d] mode:%s mount=%s path=%s\n", idx, exportFlagStrings[mode].str, mount, path);       
    }
    printf ("---\n\n");
}

// Dump config filr, in verbose mode print ongoing parsing to help find errors
int RedDumpConfigPath (const char *filepath, int warning) {

    redConfigT *config;
    cyaml_log_t logLevel;

    config = RedLoadConfig (filepath, warning);
    if (!config) goto OnErrorExit;
    printf ("# Dumping yaml red-config path=%s\n\n", filepath);
    RedDumpConfigHandle (config);
    return 0;

OnErrorExit:
    return 1;    
}

static int PopDownRedpath (char *redpath) {
    int idx;
    for (idx = strlen(redpath)-1; redpath[idx] != '/' && redpath[idx] != 0; idx--) {
        redpath[idx]=0;
    }
    // to get cleaner path remove trailing '/' if any
    if (redpath[idx]='/') redpath[idx]=0;
    return idx;
}

// return 0 on success, 1 when for none redpak node and -1 when paring fail
redNodeYamlE RedNodesLoad(const char* redpath, redNodeT *node, int verbose) {

    char nodepath[RED_MAXPATHLEN];
    struct stat statinfo;
    int error;
    redNodeYamlE rc;

    // check redpath is a readable directory
    error= stat(redpath, &statinfo);
    if (error || !S_ISDIR(statinfo.st_mode)) {
        RedLog(REDLOG_ERROR, "Not a readable directory redpath=%s", redpath);
        goto OnErrorExit;
    }

    // check redpath look like a valide node
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", redpath, REDNODE_STATUS);

    error= stat(nodepath, &statinfo);
    if (error || !S_ISREG(statinfo.st_mode)) {
        rc=RED_NODE_CONFIG_MISSING;    
        goto OnExit;
    }

    // parse redpath node status file
    node->status = RedLoadStatus (nodepath, verbose);
    if (!node->status) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak status [path=%s]", nodepath);
        goto OnErrorExit;
    }

    // parse redpath node config file
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", redpath, REDNODE_CONF);
    node->config = RedLoadConfig (nodepath, verbose);
    if (!node->config) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak config [path=%s]", nodepath);
        goto OnErrorExit;
    }

    // keep a copy of effective redpath for sanity check purpose
    node->redpath=strdup(redpath);
    rc = RED_NODE_CONFIG_OK;
OnExit:
    return rc;

OnErrorExit:
    return RED_NODE_CONFIG_FX;    
}

// return 0 on success, 1 when for none redpak node and -1 when paring fail
int RedUpdateStatus(redNodeT *node, int verbose) {

    char nodepath[RED_MAXPATHLEN];
    int error;

    // build status path from node
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", node->redpath, REDNODE_STATUS);

    node->status->timestamp= RedUtcGetTimeMs();
    node->status->info=RedNodeStringExpand (node, NULL, REDSTATUS_INFO, NULL, NULL);

    // Save update status
    error = RedSaveStatus (nodepath, node->status, verbose);
    if (error) {
        RedLog(REDLOG_ERROR, "Fail to update redpak status [path=%s]", nodepath);
        goto OnErrorExit;
    }

    return 0;

OnErrorExit:
    return 1;
}

// loop down within all redpath nodes from a given familly
static int RedNodesDigToRoot(char* nodepath, redNodeT *childrenNode, int verbose) {
    int index;
    redNodeT *parentNode;

    while (1) { 

        // if we are not at root level dig down for ancestor node
        index = PopDownRedpath (nodepath);
        if (index <= 1) {
            childrenNode->ancestor = NULL;
            break;
        }

        // We have parent directories let's check if they are rednode compatible
        redNodeT *parentNode = calloc (1, sizeof(redNodeT));
        redNodeYamlE result= RedNodesLoad(nodepath, parentNode, verbose);
        
        switch (result) {
            case RED_NODE_CONFIG_OK:
                childrenNode->ancestor = parentNode;  
                childrenNode=parentNode;
                break;
            case  RED_NODE_CONFIG_MISSING:
                if (verbose) 
                    RedLog(REDLOG_WARNING, "redpak ignoring empty node [path=%s]", nodepath);
                free (parentNode);
                break;
            default:
                goto OnErrorExit;       
        }
    } 

OnSuccessExit:
    return 0;

OnErrorExit:
    return 1;
}

// Read terminal redleaf and dig down directory hierarchie
redNodeT *RedNodesScan(const char* redpath, int verbose) {

    char *nodepath;
    int error, index;
    redNodeYamlE result;


    // store a copy of redpath before it disapear and loop recursively down toward rootdir.
    nodepath=strdup(redpath);

    // allocate leaf terminal end node and search for redpak config
    redNodeT *redleaf = calloc (1, sizeof(redNodeT));
    result = RedNodesLoad(nodepath, redleaf, verbose);
    if (result != RED_NODE_CONFIG_OK) {
        RedLog(REDLOG_ERROR, "redpak terminal leaf config & status not found [path=%s]", nodepath);
        goto OnErrorExit;
    }

    // gig redpath familly hierarchie
    error= RedNodesDigToRoot (nodepath, redleaf, verbose);
    if (error) goto OnErrorExit;

    free (nodepath);
    return redleaf;

OnErrorExit:
    return NULL;   
}

// Debug tool dump a redpak node family tree
void RedDumpFamilyNodeHandle(redNodeT *familyTree) {

    for (redNodeT *node=familyTree; node != NULL; node=node->ancestor) {
        redStatusT *status;
        redConfigT *config;
        struct redNodeS *ancestor;
        const char *redpath;

        RedDumpConfigHandle(node->config);
        RedDumpStatusHandle(node->status);
    }

}

// Dump a redpath family node tree
int RedDumpFamilyNodePath (const char* redpath, int verbose) {

    redNodeT *familyTree = RedNodesScan(redpath, verbose);
    if (!familyTree) {
        RedLog(REDLOG_ERROR, "Fail to scandown redpath family tree path=%s", redpath);
        goto OnErrorExit;
    }

    RedDumpFamilyNodeHandle (familyTree);
    return 0;

OnErrorExit:
    return 1;    
}

// Return current UTC time in ms
unsigned long RedUtcGetTimeMs () {
    struct timeval tp;
    unsigned long epocms;

    // get UTC time
    gettimeofday(&tp, NULL);
    epocms = tp.tv_sec * 1000 + tp.tv_usec / 1000; 

    return (epocms);
}



// Extract $KeyName and replace with $Key Env or default Value
static void RedConfGetEnvKey (redNodeT *node, RedConfDefaultsT *defaults, int *idxIn, const char *inputS, int *idxOut, char *outputS) {
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

    envval = getenv(envkey);
    if (envval) {
        for (int jdx=0; envval[jdx]; jdx++) outputS[(*idxOut)++]= envval[jdx];

    } else {
        // Search for a default key
        for (int idx=0; defaults[idx].label; idx++) {
            if (!strcmp (envkey, defaults[idx].label)) {
                switch (defaults[idx].type) {
                    case REDDEFLT_STR:
                        envval = defaults[idx].value;
                        for (int jdx=0; envval[jdx]; jdx++) outputS[(*idxOut)++]= envval[jdx];
                        break;
                    case REDDEFLT_CB:
                        envval = (*(RedGetDefaultCbT)defaults[idx].value) (node, defaults[idx].label); 
                        for (int jdx=0; envval[jdx]; jdx++) outputS[(*idxOut)++]= envval[jdx];
                        free (envval);
                        break;
                    default:
                        envval = "#UNSUPPORTED_ENVCB";       
                        for (int jdx=0; envval[jdx]; jdx++) outputS[(*idxOut)++]= envval[jdx];
                }
                break;
            }
        }
    }
    if (!envval) goto OnErrorExit;
    return;

OnErrorExit:
    envval="#INVALID_ENVKEY";
    for (int jdx=0; envval[jdx]; jdx++) outputS[(*idxOut)++]= envval[jdx];
    return;    
}

// Expand string with environnement variable
const char * RedNodeStringExpand (redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, const char* prefix, const char* trailler) {
    int count=0, idxIn, idxOut=0;
    char outputS[MAX_CYAML_FORMAT_STR];

    if (!defaults) defaults=nodeConfigDefaults;

    if (prefix) {
        for (int idx=0; prefix[idx]; idx++) {
            outputS[idxOut]=trailler[idx];
            idxOut++;
        }
    }

    // search for a $within string input format 
    for (idxIn=0; inputS[idxIn] != '\0'; idxIn++) {

        if (inputS[idxIn] != '$') {
            if (idxOut == MAX_CYAML_FORMAT_STR)  goto OnErrorExit;
            outputS[idxOut++] = inputS[idxIn];

        } else {    
            if (count == MAX_CYAML_FORMAT_ENV) goto OnErrorExit;
            RedConfGetEnvKey (node, defaults, &idxIn, inputS, &idxOut, outputS);
            count ++;
        }
    }

    // if we have a trailler add it now
    if (trailler) {
        for (int idx=0; trailler[idx]; idx++) {
            outputS[idxOut]=trailler[idx];
            idxOut++;
        }
    }
    
    // close the string
    outputS[idxOut]='\0';

    // string is formated replace original with expanded value
    return strdup(outputS);

OnErrorExit:
    RedLog(REDLOG_ERROR, "Fail Expending cyaml entry=%s value=%s err=[Len or $ count]",node->config->headers->name, inputS);
    return NULL;

}

