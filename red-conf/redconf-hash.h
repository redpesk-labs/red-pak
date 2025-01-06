/*
* Copyright (C) 2022-2025 IoT.bzh Company
* Author: Clément Bénier <clement.benier@iot.bzh>
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

#ifndef _RED_CONF_HASH_INCLUDE_
#define _RED_CONF_HASH_INCLUDE_

#include <urcu.h>             /* RCU flavor */
#include <urcu/rculfhash.h>   /* RCU Lock-free hash table */
#include <urcu/compiler.h>    /* For CAA_ARRAY_SIZE */

#include "redconf-node.h"

// hash structure
typedef struct {
    const char *key;
    const void *value;
    const redNodeT *node;
    struct cds_lfht_node lfht_node;
} redHashT;

//callback to get data pointer from node and count nb
typedef const void *(*RedGetDataHash)(const redNodeT *node, int *count);
//callback to get key for hash
typedef const char *(*RedGetKeyHash)(const redNodeT *node, const void *srcdata, int *ignore);
//callback to set data values
typedef int (*RedSetDataHash)(const void *destdata, redHashT *srchash, redHashT *overload, int expand, int duplicate);

typedef struct {
    RedGetDataHash getdata;
    RedGetKeyHash getkey;
    RedSetDataHash setdata;
} redHashCbsT;

//merge data with hash table
void *mergeData(const redNodeT* rootnode, size_t dataLen, int *mergecount, redHashCbsT *hashcbs, int append_duplicate, int expand);

#endif
