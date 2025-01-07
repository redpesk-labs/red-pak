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

#define _GNU_SOURCE

#include "redconf-hash.h"

#include <string.h>
#include <stdlib.h>

#include "redconf-log.h"
#include "redconf-defaults.h"

//hash function
#include "lookup3.h"

static int destroyHashExport(struct cds_lfht *ht) {
    int err;
    struct cds_lfht_iter iter;    /* For iteration on hash table */
    redHashT *hnode;

    cds_lfht_for_each_entry(ht, &iter, hnode, lfht_node) {
        cds_lfht_del(ht, &hnode->lfht_node);
        free(hnode);
    }

    err = cds_lfht_destroy(ht, NULL);
    if(err)
        RedLog(REDLOG_ERROR, "Destroying hash table failed");
    return err;
}

static int hashmatch(struct cds_lfht_node *ht_node, const void *_key) {
    redHashT *hnode = caa_container_of(ht_node, redHashT, lfht_node);
    const char *key = _key;

    return !strncmp(key, hnode->key, 1 + strlen(key));
}

static int add_hash(struct cds_lfht *ht, redHashT *value, redHashT **hoverload) {
    struct cds_lfht_iter iter;    /* For iteration on hash table */
    struct cds_lfht_node *lookup_node; /* For checking if key already present */
    redHashT *hnode = NULL;  /* New Entry Hash table */
    unsigned long hash; /* hash number of string entry */
    size_t bytes; /* size in bytes of string */

    //get hash from string path
    bytes = strlen(value->key);
    hash = hashlittle(value->key, bytes, 0);

    //looking if path already in hash table
    cds_lfht_lookup(ht, hash, hashmatch, value->key, &iter);
    lookup_node = cds_lfht_iter_get_node(&iter);
    if(lookup_node) {
        redHashT *hnodeoverload = caa_container_of(lookup_node, redHashT, lfht_node);

        *hoverload = hnodeoverload;
    }


    //allocate hash entry
    hnode = calloc(1, sizeof(*hnode) + 1 + bytes);
    if (!hnode) {
        RedLog(REDLOG_ERROR, "Issue allocate hnode");
        goto Exit;
    }
    hnode->key = (const char*)(hnode + 1);
    strcpy((char*)hnode->key, value->key);
    hnode->value = value->value;
    hnode->node = value->node;

    //add entry to hash table
    cds_lfht_node_init(&hnode->lfht_node);
    cds_lfht_add(ht, hash, &hnode->lfht_node);
    RedLog(REDLOG_TRACE, "append hash=%lu sizeof=%d -%s-", hash, bytes, value->key);

    return 0;
Exit:
    return 1;
}

static inline int allocdata(void **data, size_t nb, size_t dataLen) {
    void *tmpdata = NULL;
    /* realloc at the nb of data: will be reajusted at the loop exit */
    tmpdata = reallocarray(*data, nb, dataLen);
    if(!tmpdata) {
        RedLog(REDLOG_ERROR, "Issue reallocacarray nb members=%d, dataLen=%d", nb, dataLen);
        goto OnErrorExit;
    }

    *data = tmpdata;
    return 0;
OnErrorExit:
    return 1;
}

void *mergeData(const redNodeT* rootnode, size_t dataLen, int *mergecount, redHashCbsT *hashcbs, int duplicate, int expand) {
    int offset = 0, count = 0, ignore = 0;
    struct cds_lfht *ht = NULL;    /* Hash table */
    const char *hashkey; /* key to hash */
    const void *data, *itdata; // data and iterator on source data
    void *destdata = NULL, *itdestdata; // data and iterator on destination data
    redHashT *overload = NULL; //value stored in hash table if overloaded

    //allocate hash_table
    ht = cds_lfht_new(1, 1, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, NULL);
    if (!ht) {
        RedLog(REDLOG_ERROR, "Error allocating hash table");
        goto OnError;
    }

    // fill hash table
    for (const redNodeT *node = rootnode; node; node = node->first_child) {
        data = hashcbs->getdata(node, &count);
        if (count == 0)
            continue;

        /* realloc at the nb of data: will be reajusted at the loop exit */
        if(allocdata(&destdata, offset + count, dataLen)) goto OnError;

        RedLog(REDLOG_TRACE, "redpath %s", node->redpath);

        for (int i = 0; i < count; i++) {
            overload = NULL; //reset overload value

            itdata = data + i * dataLen;
            itdestdata = destdata + offset * dataLen;
            memset(itdestdata, 0, dataLen);

            ignore = 0; //reset ignore
            // get key to hash
            hashkey = hashcbs->getkey(node, itdata, &ignore);

            if(!hashkey || ignore)
                continue;

            redHashT value = {
                .key = hashkey,
                .node = node,
                .value = itdata,
            };

            if(add_hash(ht, &value, &overload)) {
                free((char *)hashkey);
                goto OnError;
            }

            if(hashcbs->setdata(itdestdata, &value, overload, expand, duplicate)) {
                RedLog(REDLOG_WARNING, "No data set for hashkey=%s redpath=%s", hashkey, node->redpath);
                continue;
            }

            offset += 1;
        }
    }
    /* realloc with the true number of export added */
    if(allocdata(&destdata, offset, dataLen)) goto OnError;
    *mergecount = offset;

    destroyHashExport(ht);

    return destdata;
OnError:
    if (destdata) free(destdata);
    return NULL;
}
