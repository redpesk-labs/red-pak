/*
* Copyright (C) 2022 "IoT.bzh"
* Author Clément Bénier <clement.benier@iot.bzh>
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

#include "redconf-schema.h"

//callback to get data pointer from node and count nb
typedef const void *(*RedGetDataHash)(const redNodeT *node, int *count);
typedef const char *(*RedGetKeyHash)(const redNodeT *node, const void *srcdata, int *ignore);
//callback to set data values and need to return hashkey, pwarn is the pointer address of warn
typedef int (*RedSetDataHash)(const redNodeT *node, const void *destdata, const void *srcdata, const char *hashkey, const char *warn, int expand);

typedef struct {
    RedGetDataHash getdata;
    RedGetKeyHash getkey;
    RedSetDataHash setdata;
} redHashCbsT;

//merge data with hash table
void * mergeData(const redNodeT* rootnode, size_t dataLen, int *mergecount, redHashCbsT *hashcbs, int append_duplicate, int expand);

#endif