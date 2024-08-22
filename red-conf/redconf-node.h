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

/*
 ** Redpath hierachie

    -'root' path='/' -> alias='coreos'
    -'node' path=${redroot}/xxxxxx -> alias=(template, plateform, ....)
    -'leaf' same as node but without decendants (typically a projet)
    -'family' every nodes in direct line from a leaf to root node.

 ** export visibility

    -'private' visible only with one node/leaf
    -'restricted' visible from all descendants in readonly
    -'public' visible from all descendants in writemode

    Note: when sibling node need to share data, then need to use ancestor (public/restricted directories)
*/

#ifndef _RED_CONF_NODE_INCLUDE_
#define _RED_CONF_NODE_INCLUDE_

#include "redconf-schema.h"

// --- Redpath Node Directory Hierarchy (from leaf to root)
typedef struct redNodeS{
    redStatusT *status;
    redConfigT *config;
    redConfigT *confadmin;
    struct redNodeS *parent;
    struct redNodeS *first_child;
    struct redNodeS *next_sibling;
    const char *redpath;
} redNodeT;

/**
 * Read the node description from the file system files.
 * The node is located at the directory @ref redpath.
 *
 * @param redpath location of the node
 * @param admin   boolean telling if for administration or not
 * @param verbose boolean telling if verbose output is required
 *
 * @return the node or NULL if something went wrong
 */
redNodeT *RedNodesScan(const char* redpath, int admin, int verbose);

redNodeT *RedNodesDownScan(const char* redroot, int admin, int verbose);
int RedUpdateStatus(redNodeT *node, int verbose);

void freeRedLeaf(redNodeT *redleaf);
void freeRedRoot(redNodeT *redroot);


#endif
