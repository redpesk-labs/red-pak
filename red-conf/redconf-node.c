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

#include "redconf-node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "redconf-log.h"
#include "redconf-defaults.h"
#include "redconf-expand.h"
#include "redconf-utils.h"

typedef enum {
    RED_NODE_CONFIG_OK,
    RED_NODE_CONFIG_FX,
    RED_NODE_CONFIG_MISSING,
}  redNodeYamlE;

/**
 * Release the memory used by @ref node
 */
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

static int PopDownRedpath (char *redpath) {
    int idx;
    // to get cleaner path remove trailing '/' if any
    if (redpath[strlen(redpath)-1] == '/') redpath[strlen(redpath)-1] = 0;
    for (idx = strlen(redpath)-1; redpath[idx] != '/' && redpath[idx] != 0; idx--) {
        redpath[idx]=0;
    }
    return idx;
}

// return 0 on success, 1 when for none redpak node and -1 when paring fail
static redNodeYamlE RedNodesLoad(const char* redpath, redNodeT **pnode, int admin, int verbose) {
    char path[RED_MAXPATHLEN];
    struct stat statinfo;
    int error;
    redNodeYamlE rc;

    // check redpath is a readable directory
    error= stat(redpath, &statinfo);
    if (error || !S_ISDIR(statinfo.st_mode)) {
        RedLog(REDLOG_ERROR, "Not a readable directory redpath=%s error=%s", redpath, strerror(errno));
        goto OnErrorExit;
    }

    // check redpath look like a valid node
    error = snprintf (path, sizeof(path), "%s/%s", redpath, REDNODE_STATUS);
    if (error < 0 || (size_t)error >= sizeof(path)) {
        RedLog(REDLOG_ERROR, "Fail to get status path [path=%s/%s]", redpath, REDNODE_STATUS);
        goto OnErrorFreeExit;
    }
    error = stat(path, &statinfo);
    if (error || !S_ISREG(statinfo.st_mode)) {
        rc=RED_NODE_CONFIG_MISSING;
        goto OnExit;
    }

    *pnode = calloc (1, sizeof(redNodeT));
    if (!*pnode) {
        RedLog(REDLOG_ERROR, "Fail to calloc node");
        goto OnErrorExit;
    }

    // parse redpath node status file
    (*pnode)->status = RedLoadStatus (path, verbose);
    if (!(*pnode)->status) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak status [path=%s]", path);
        goto OnErrorFreeExit;
    }

    // parse redpath node config file
    error = snprintf (path, sizeof(path), "%s/%s", redpath, REDNODE_CONF);
    if (error > 0 && (size_t)error < sizeof(path) && access(path, F_OK) != 0)
        error = snprintf (path, sizeof(path), "%s/%s", redpath, LEGACY_REDNODE_CONF);
    if (error < 0 || (size_t)error >= sizeof(path)) {
        RedLog(REDLOG_ERROR, "Fail to get config path [path=%s/%s]", redpath, REDNODE_CONF);
        goto OnErrorFreeExit;
    }
    (*pnode)->config = RedLoadConfig (path, verbose);
    if (!(*pnode)->config) {
        RedLog(REDLOG_ERROR, "Fail to parse redpak config [path=%s]", path);
        goto OnErrorFreeExit;
    }

    if (admin) {
        // parse redpath node config admin file: if exists
        error = snprintf (path, sizeof(path), "%s/%s", redpath, REDNODE_ADMIN);
        if (error > 0 && (size_t)error < sizeof(path) && access(path, F_OK) != 0)
            error = snprintf (path, sizeof(path), "%s/%s", redpath, LEGACY_REDNODE_ADMIN);
        if (error < 0 || (size_t)error >= sizeof(path)) {
            RedLog(REDLOG_ERROR, "Fail to get admin path [path=%s/%s]", redpath, REDNODE_ADMIN);
            goto OnErrorFreeExit;
        }
        error= stat(path, &statinfo);
        if (error || !S_ISREG(statinfo.st_mode)) {
            RedLog(REDLOG_DEBUG, "No admin config file=%s.!!!!", path);
        } else {
            (*pnode)->confadmin = RedLoadConfig (path, verbose);
            if (!(*pnode)->confadmin) {
                RedLog(REDLOG_ERROR, "Fail to parse redpak admin config [path=%s]", path);
                goto OnErrorFreeExit;
            }
        }
    }

    // keep a copy of effective redpath for sanity check purpose
    (*pnode)->redpath = strdup(redpath);
    rc = RED_NODE_CONFIG_OK;
OnExit:
    return rc;

OnErrorFreeExit:
    freeNode(*pnode);
OnErrorExit:
    return RED_NODE_CONFIG_FX;
}

// return 0 on success, 1 when for none redpak node and -1 when paring fail
int RedUpdateStatus(redNodeT *node, int verbose) {

    char nodepath[RED_MAXPATHLEN];
    int error;

    // build status path from node
    (void)snprintf (nodepath, sizeof(nodepath), "%s/%s", node->redpath, REDNODE_STATUS);

    node->status->timestamp = RedUtcGetTimeMs();
    node->status->info = RedNodeStringExpand (node, REDSTATUS_INFO);

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
static int RedNodesDigToRoot(const char* redpath, redNodeT *leafNode, int admin, int verbose) {
    redNodeT *childNode = leafNode;
    char nodepath[RED_MAXPATHLEN + 1];
    strncpy(nodepath, redpath, RED_MAXPATHLEN);
    nodepath[RED_MAXPATHLEN] = 0;

    while (1) {

        // if we are not at root level dig down for ancestor node
        int index = PopDownRedpath (nodepath);
        if (index <= 1) {
            childNode->parent = NULL;
            break;
        }

        // We have parent directories let's check if they are rednode compatible
        redNodeT *parentNode = NULL;
        redNodeYamlE result= RedNodesLoad(nodepath, &parentNode, admin, verbose);

        switch (result) {
            case RED_NODE_CONFIG_OK:
                childNode->parent = parentNode;
                parentNode->first_child = childNode;
                parentNode->leaf = leafNode;
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

static int RedChildrenNodesLoad(redNodeT *node, int admin, int verbose) {
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
        redNodeYamlE result = RedNodesLoad(child_nodepath, &childrenNode, admin, verbose);
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
        childrenNode->parent = node;
        childrenNode->next_sibling = node->first_child;
        node->first_child = childrenNode;
    }

OnExit:
    closedir(fd);
    return rc;
OnErrorExit:
    return 1;
}

// loop up within all redpath nodes from a given familly
static int RedNodesDigToChilds(redNodeT *currentNode, int admin, int verbose) {

    // load all children nodes
    if (RedChildrenNodesLoad(currentNode, admin, verbose))
        goto OnErrorExit;

    // recursively load grand children nodes
    for (redNodeT *child_node = currentNode->first_child; child_node ; child_node = child_node->next_sibling) {
        if(RedNodesDigToChilds(child_node, admin, verbose)) {
            RedLog(REDLOG_ERROR, "Fail to dig to childs from %s", currentNode->redpath);
            goto OnErrorExit;
        }
    }
    return 0;

OnErrorExit:
    return 1;
}

// Read terminal redleaf and dig down directory hierarchie
redNodeT *RedNodesScan(const char* redpath, int admin, int verbose) {

    redNodeT *redleaf = NULL;
    int error;
    redNodeYamlE result;


    // allocate leaf terminal end node and search for redpak config
    result = RedNodesLoad(redpath, &redleaf, admin, verbose);
    if (result != RED_NODE_CONFIG_OK) {
        RedLog(REDLOG_ERROR, "redpak terminal leaf config & status not found [path=%s]", redpath);
        goto OnErrorExit;
    }

    // dig redpath familly hierarchie
    redleaf->leaf = redleaf;
    error = RedNodesDigToRoot (redpath, redleaf, admin, verbose);
    if (error) goto OnErrorFree;

    return redleaf;

OnErrorFree:
    freeRedLeaf(redleaf);
OnErrorExit:
    return NULL;
}

redNodeT *RedNodesDownScan(const char* redroot, int admin, int verbose) {

    int error;
    redNodeYamlE result;


    // allocate root node and search for redpak config
    redNodeT *redrootNode = NULL;
    result = RedNodesLoad(redroot, &redrootNode, admin, verbose);
    if (result != RED_NODE_CONFIG_OK) {
        RedLog(REDLOG_ERROR, "redpak redroot node config & status not found [path=%s]", redroot);
        goto OnErrorExit;
    }
    redrootNode->parent = NULL;
    RedLog(REDLOG_DEBUG, "redroot load redroot->redpath=%s", redrootNode->redpath);

    // dig down to child nodes
    error = RedNodesDigToChilds (redrootNode, admin, verbose);
    if (error) goto OnErrorExit;

    return redrootNode;

OnErrorExit:
    return NULL;
}

void freeRedLeaf(redNodeT *redleaf) {
    if (redleaf) {
        redNodeT *node = redleaf;
        while (node->parent != NULL)
            node = node->parent;
        freeRedRoot(node);
    }
}

void freeRedRoot(redNodeT *redroot) {
    if (redroot) {
        redNodeT *child = redroot->first_child, *nextchild;
        while(child) {
            nextchild = child->next_sibling;
            freeRedRoot(child);
            child = nextchild;
        }
        freeNode(redroot);
    }
}

void RedNodeEnvSet(const redNodeT *node)
{
    const redNodeT *leaf = node->leaf ?: node;
    setenv("NODE_ALIAS", node->config->headers.alias, 1);
    setenv("NODE_NAME",  node->config->headers.name, 1);
    setenv("NODE_PATH",  node->redpath, 1);
    setenv("NODE_INFO",  node->config->headers.info, 1);
    setenv("LEAF_ALIAS", leaf->config->headers.alias, 1);
    setenv("LEAF_NAME",  leaf->config->headers.name, 1);
    setenv("LEAF_PATH",  leaf->redpath, 1);
    setenv("LEAF_INFO",  leaf->config->headers.info, 1);
}

void RedNodeEnvUnset()
{
    unsetenv("NODE_ALIAS");
    unsetenv("NODE_NAME");
    unsetenv("NODE_PATH");
    unsetenv("NODE_INFO");
    unsetenv("LEAF_ALIAS");
    unsetenv("LEAF_NAME");
    unsetenv("LEAF_PATH");
    unsetenv("LEAF_INFO");
}

