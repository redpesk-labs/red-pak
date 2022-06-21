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
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <search.h>
#include <ftw.h>
#include <errno.h>

#include "redconf.h"
#include "redconf-utils.h"

#ifndef MAX_CYAML_FORMAT_ENV
#define MAX_CYAML_FORMAT_ENV 10
#endif
#ifndef MAX_CYAML_FORMAT_STR
#define MAX_CYAML_FORMAT_STR 512
#endif

static RedLogLevelE RedLogLevel = REDLOG_INFO;

void SetLogLevel(RedLogLevelE level) {
    RedLogLevel = level;
}

// Allow log function to be mapped on RedLog, syslog, ...
static RedLogCbT *redLogRegisteredCb;
void RedLogRegister (RedLogCbT *redlogcb) {

    redLogRegisteredCb= redlogcb;
}

typedef struct {
    const char *str;
    int val;

} debugStrValT;

#define COLOR_REDPRINT "\033[0;31m"
#define COLOR_RESET "\033[0m"

static const debugStrValT debugPrefixFormat[] = {
    {COLOR_REDPRINT"[EMERGENCY]: ", REDLOG_EMERGENCY},
    {COLOR_REDPRINT"[ALERT]: ", REDLOG_ALERT},
    {COLOR_REDPRINT"[CRITICAL]: ", REDLOG_CRITICAL},
    {COLOR_REDPRINT"[ERROR]: ", REDLOG_ERROR},
    {COLOR_REDPRINT"[WARNING]: ", REDLOG_WARNING},
    {              "[NOTICE]: ", REDLOG_NOTICE},
    {              "[INFO]: ", REDLOG_INFO},
    {              "[DEBUG]: ", REDLOG_DEBUG},
    {              "[TRACE]: ", REDLOG_TRACE},
};

// redlib and redconf use RedLog to display error messages
void redlog(RedLogLevelE level, const char *file, int line, const char *format, ...) {
    if (level > RedLogLevel)
        return;
    va_list args;
    va_start(args, format);

    if (redLogRegisteredCb) {
        (*redLogRegisteredCb) (level, format, args);
    } else {
        fprintf(stderr, "%s ", debugPrefixFormat[level].str);
        vfprintf (stderr, format, args);
        fprintf(stderr, "\t[%s:%d] ", file, line);
        fprintf(stderr,"\n"COLOR_RESET);
    }
    va_end(args);
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

char *RedPutEnv (const char*key, const char*value) {
    size_t length = sizeof(char)*(strlen(key) + strlen(value) + 2);
    char *envval = malloc(length);

	snprintf(envval, length, "%s=%s", key, value);
    RedLog(REDLOG_INFO, "set environment variable %s", envval);
	putenv(envval);
    return envval;
}

static int PopDownRedpath (char *redpath) {
    int idx;
    // to get cleaner path remove trailing '/' if any
    if (redpath[strlen(redpath)-1] == '/') redpath[strlen(redpath)-1] = 0;
    for (idx = strlen(redpath)-1; redpath[idx] != '/' && redpath[idx] != 0; idx--) {
        redpath[idx]=0;
    }
    return idx;
}

int RedCheckSetStatusPath(const char *nodepath, char *statuspath, size_t size) {
    struct stat statinfo;
    int error;

    // check look like a valide node
    (void)snprintf (statuspath, size, "%s/%s", nodepath, REDNODE_STATUS);

    error = stat(statuspath, &statinfo);
    if (error || !S_ISREG(statinfo.st_mode)) {
        goto OnExit;
    }

    return 0;
OnExit:
    return 1;
}

// return 0 on success, 1 when for none redpak node and -1 when paring fail
redNodeYamlE RedNodesLoad(const char* redpath, redNodeT **pnode, int verbose) {
    char nodepath[RED_MAXPATHLEN];
    struct stat statinfo;
    int error;
    redNodeYamlE rc;

    if (!pnode) {
        RedLog(REDLOG_ERROR, "[load node %s]: pnode is null, aborting...", redpath);
        goto OnErrorExit;
    }

    // check redpath is a readable directory
    error= stat(redpath, &statinfo);
    if (error || !S_ISDIR(statinfo.st_mode)) {
        RedLog(REDLOG_ERROR, "Not a readable directory redpath=%s", redpath);
        goto OnErrorExit;
    }

    // check redpath look like a valide node
    if (RedCheckSetStatusPath(redpath, nodepath, sizeof(nodepath))) {
        rc=RED_NODE_CONFIG_MISSING;
        goto OnExit;
    }

    *pnode = calloc (1, sizeof(redNodeT));
    if (!*pnode) {
        RedLog(REDLOG_ERROR, "Fail to calloc node");
        goto OnErrorExit;
    }

    // parse redpath node status file
    (*pnode)->status = RedLoadStatus (nodepath, verbose);
    if (!(*pnode)->status) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak status [path=%s]", nodepath);
        goto OnErrorFreeExit;
    }

    // parse redpath node config file
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", redpath, REDNODE_CONF);
    (*pnode)->config = RedLoadConfig (nodepath, verbose);
    if (!(*pnode)->config) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak config [path=%s]", nodepath);
        goto OnErrorFreeExit;
    }

    // parse redpath node config admin file: if exists
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", redpath, REDNODE_ADMIN);
    error= stat(nodepath, &statinfo);
    if (error || !S_ISREG(statinfo.st_mode)) {
        RedLog(REDLOG_DEBUG, "No admin config file=%s.!!!!", nodepath);
    } else {
        (*pnode)->confadmin = RedLoadConfig (nodepath, verbose);
        if (!(*pnode)->confadmin) {
            RedLog(REDLOG_ERROR, "Fail to parse redpak admin config [path=%s]", nodepath);
            goto OnErrorFreeExit;
        }
    }

    //allocate childs
    (*pnode)->childs = calloc(1, sizeof(redChildNodeT));
    if(!(*pnode)->childs) {
        RedLog(REDLOG_ERROR, "Cannot allocatate childs for %s", nodepath);
        goto OnErrorFreeExit;
    }
    (*pnode)->childs->child = NULL;
    (*pnode)->childs->brother = NULL;

    // keep a copy of effective redpath for sanity check purpose
    (*pnode)->redpath = strdup(redpath);
    rc = RED_NODE_CONFIG_OK;
OnExit:
    return rc;

OnErrorFreeExit:
    free(*pnode);
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
static int RedNodesDigToRoot(const char* redpath, redNodeT *childNode, int verbose) {
    char nodepath[RED_MAXPATHLEN];
    strncpy(nodepath, redpath, RED_MAXPATHLEN);
    int index;

    while (1) {

        // if we are not at root level dig down for ancestor node
        index = PopDownRedpath (nodepath);
        if (index <= 1) {
            childNode->ancestor = NULL;
            break;
        }

        // We have parent directories let's check if they are rednode compatible
        redNodeT *parentNode = NULL;
        redNodeYamlE result= RedNodesLoad(nodepath, &parentNode, verbose);

        switch (result) {
            case RED_NODE_CONFIG_OK:
                childNode->ancestor = parentNode;
                parentNode->childs->child = childNode;
                parentNode->childs->brother = NULL;

                childNode = parentNode;
                break;
            case  RED_NODE_CONFIG_MISSING:
                if (verbose > 1)
                    RedLog(REDLOG_WARNING, "Ignoring rednode [%s]", nodepath);
                break;
            default:
                goto OnErrorExit;
        }
    }

    return 0;

OnErrorExit:
    return 1;
}

static int RedChildrenNodesLoad(redNodeT *node, int verbose) {
    int rc = 0;
    char child_nodepath[RED_MAXPATHLEN];
    struct dirent *de;
    DIR *fd;

    // open redpath
    fd = opendir(node->redpath);
    if (!fd)
        goto OnErrorExit;

    // readdir redpath
    while((de = readdir(fd))) {

        // if not a directory: ignore
        if (de->d_type != DT_DIR)
            continue;

        //ignore hidden directories starting with '.'hidden
        //  - also means ignore '.' and '..' directories
        if (de->d_name[0] == '.')
            continue;

        if (!strncmp (de->d_name, "..", 2))
            continue;

        // get child redpath node->redpath + dir name
        (void)snprintf (child_nodepath, sizeof(child_nodepath), "%s/%s", node->redpath, de->d_name);

        // try to load child node, if it is not a node ignore
        redNodeT *childrenNode = NULL;
        redNodeYamlE result = RedNodesLoad(child_nodepath, &childrenNode, verbose);
        switch (result)
        {
            case RED_NODE_CONFIG_OK:
                break;

            // ignore if it is not a node
            case RED_NODE_CONFIG_MISSING:
                if (verbose > 1)
                    RedLog(REDLOG_WARNING, "Ignoring rednode [%s]", child_nodepath);
                continue;

            default:
                rc = 2;
                goto OnExit;
        }

        //add node as ancestor for childrenNode
        childrenNode->ancestor = node;

        redChildNodeT *childs = NULL;
        //if there are children: prepend a new one
        if (node->childs->child) {
            childs = calloc(1, sizeof(redChildNodeT));
            if (!childs) {
                RedLog(REDLOG_ERROR, "issue calloc childs for %s", child_nodepath);
                rc = 3;
                goto OnExit;
            }
            childs->brother = node->childs;
            node->childs = childs;
        }
        node->childs->child = childrenNode;

    }

OnExit:
    closedir(fd);
    return rc;
OnErrorExit:
    return 1;
}

// loop up within all redpath nodes from a given familly
static int RedNodesDigToChilds(redNodeT *currentNode, int verbose) {

    // load all children nodes
    if (RedChildrenNodesLoad(currentNode, verbose))
        goto OnErrorExit;

    // if no grand children return
    if (!currentNode->childs)
        goto OnSuccessExit;

    // recursively load grand children nodes
    for (redChildNodeT *child_node = currentNode->childs; child_node && child_node->child; child_node = child_node->brother) {
        if(RedNodesDigToChilds(child_node->child, verbose)) {
            RedLog(REDLOG_ERROR, "Fail to dig to childs from %s", currentNode->redpath);
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

    redNodeT *redleaf = NULL;
    int error;
    redNodeYamlE result;


    // allocate leaf terminal end node and search for redpak config
    result = RedNodesLoad(redpath, &redleaf, verbose);
    if (result != RED_NODE_CONFIG_OK) {
        RedLog(REDLOG_ERROR, "redpak terminal leaf config & status not found [path=%s]", redpath);
        goto OnErrorExit;
    }

    // dig redpath familly hierarchie
    error= RedNodesDigToRoot (redpath, redleaf, verbose);
    if (error) goto OnErrorFree;

    // set NODE_ALIAS in case some env var expand it.
    setenv("LEAF_ALIAS", redleaf->config->headers->alias, 1);
    setenv("LEAF_NAME", redleaf->config->headers->name, 1);
    setenv("LEAF_PATH", redleaf->status->realpath, 1);

    return redleaf;

OnErrorFree:
    freeRedLeaf(redleaf);
OnErrorExit:
    return NULL;
}

redNodeT *RedNodesDownScan(const char* redroot, int verbose) {

    int error;
    redNodeYamlE result;


    // allocate root node and search for redpak config
    redNodeT *redrootNode = NULL;
    result = RedNodesLoad(redroot, &redrootNode, verbose);
    if (result != RED_NODE_CONFIG_OK) {
        RedLog(REDLOG_ERROR, "redpak redroot node config & status not found [path=%s]", redroot);
        goto OnErrorExit;
    }
    redrootNode->ancestor = NULL;
    RedLog(REDLOG_DEBUG, "redroot load redroot->redpath=%s", redrootNode->redpath);

    // dig down to child nodes
    error = RedNodesDigToChilds (redrootNode, verbose);
    if (error) goto OnErrorExit;

    return redrootNode;

OnErrorExit:
    return NULL;
}

static void freeNode(redNodeT *node) {
    if(node->status)
        RedFreeStatus(node->status, 0);
    if(node->config)
        RedFreeConfig(node->config, 0);
    if(node->confadmin)
        RedFreeConfig(node->confadmin, 0);

    free((char *)node->redpath);
    free(node);
}

void freeRedLeaf(redNodeT *redleaf) {
    //unset env values
    unsetenv("LEAF_ALIAS");
    unsetenv("LEAF_NAME");
    unsetenv("LEAF_PATH");

    redNodeT *node = redleaf, *nextnode;
    while(node) {
        nextnode = node->ancestor;
        free(node->childs);
        freeNode(node);
        node = nextnode;
    }
}

void freeRedRoot(redNodeT *redroot) {
    redChildNodeT *childs = redroot->childs, *nextchilds;
    while(childs) {
        nextchilds = childs->brother;
        if(childs->child)
            freeRedRoot(childs->child);
        free(childs);
        childs = nextchilds;
    }
    freeNode(redroot);
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
                    if(ExecCmd(command, command, command_res, 100)) goto OnErrorExit;
                    command_res_size = strlen(command_res);

                    //copy command res to output
                    strncpy(outputS+(*idxOut), command_res, command_res_size);
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

const char *RedGetDefaultExpand(redNodeT *node, RedConfDefaultsT *defaults, const char* inputS) {
    int idxOut = 0;
    char outputS[MAX_CYAML_FORMAT_STR];

    if (!defaults) defaults=nodeConfigDefaults;

    if (RedGetDefault(inputS, defaults, node, &idxOut, outputS, MAX_CYAML_FORMAT_STR)) return NULL;

    // close the string
    outputS[idxOut]='\0';

    return strdup(outputS);
}

// Expand string with environnement variable
const char * RedNodeStringExpand (const redNodeT *node, RedConfDefaultsT *defaults, const char* inputS, const char* prefix, const char* trailler) {
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

const char *expandAlloc(const redNodeT *node, const char *input, int expand) {
    if (!input)
        return NULL;

    if(expand)
        return RedNodeStringExpand(node, NULL, input, NULL, NULL);
    else
        return strdup(input);
}

// return file inode (use to check if two path are pointing on the same file)
int RedConfGetInod (const char* path) {
    struct stat fstat;
    int status;
    status = stat (path, &fstat);
    if (status <0) goto OnErrorExit;

    return (fstat.st_ino);

OnErrorExit:
    return -1;
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