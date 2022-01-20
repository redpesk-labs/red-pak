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

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>

#include <urcu.h>		/* RCU flavor */
#include <urcu/rculfhash.h>	/* RCU Lock-free hash table */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

#include "redconf-hash.h"
#include "redconf-utils.h"
#include "redconf-defaults.h"

//hash function
#include "lookup3.h"

typedef struct {
    const char *key;
    struct cds_lfht_node lfht_node;
} redHashT;

static int destroyHashExport(struct cds_lfht *ht) {
    int err;
	struct cds_lfht_iter iter;	/* For iteration on hash table */
    redHashT *hnode;

    int idx = 0;
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

    return !strncmp(key, hnode->key, sizeof(key)*strlen(key));
}

static int add_hash(struct cds_lfht *ht, const char *hashkey, char *warn, size_t warnlen, const char *info, int append_duplicate) {
    struct cds_lfht_iter iter;	/* For iteration on hash table */
    struct cds_lfht_node *lookup_node; /* For checking if key already present */
    redHashT *hnode = NULL;  /* New Entry Hash table */
    unsigned long hash; /* hash number of string entry */
    size_t bytes; /* size in bytes of string */

    //get hash from string path
    bytes = strlen(hashkey);
    hash = hashlittle(hashkey, bytes, 0);

    //looking if path already in hash table
    cds_lfht_lookup(ht, hash, hashmatch, hashkey, &iter);
    lookup_node = cds_lfht_iter_get_node(&iter);
    if(lookup_node) {
        RedLog(REDLOG_WARNING, "hashkey=%s is overload by %s", hashkey, info);

        // do not append in hash
        if(!append_duplicate)
            goto Exit;

		redHashT *from_hnode = caa_container_of(lookup_node, redHashT, lfht_node);
        snprintf(warn, warnlen, "value is overload by %s", info);
    }


    //allocate hash entry
    hnode = malloc(sizeof(*hnode));
    if (!hnode) {
        RedLog(REDLOG_ERROR, "Issue allocate hnode");
        goto Exit;
    }
	memset(hnode, 0, sizeof(*hnode));
    hnode->key = hashkey;

    //add entry to hash table
    cds_lfht_node_init(&hnode->lfht_node);
    cds_lfht_add(ht, hash, &hnode->lfht_node);
    RedLog(REDLOG_TRACE, "append hash=%lu sizeof=%d -%s-", hash, bytes, hashkey);

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

void * mergeData(const redNodeT* rootnode, size_t dataLen, int *mergecount, redHashCbsT *hashcbs, int append_duplicate, int expand) {
    int offset = 0, count = 0, ignore = 0;
    struct cds_lfht *ht = NULL;	/* Hash table */
    const void *data;
    void *destdata = NULL, *tmpdestdata;
    char warn[RED_MAXPATHLEN] = {0};
    const char *hashkey;

    //allocate hash_table
    ht = cds_lfht_new(1, 1, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, NULL);
    if (!ht) {
        RedLog(REDLOG_ERROR, "Error allocating hash table");
        goto OnError;
    }

    // fill hash table
    for (const redNodeT *node = rootnode; node; node = node->childs->child) {
        data = hashcbs->getdata(node, &count);

        /* realloc at the nb of data: will be reajusted at the loop exit */
        if(allocdata(&destdata, offset + count, dataLen)) goto OnError;

        RedLog(REDLOG_TRACE, "redpath %s", node->redpath);

        const void *itdata;
        void *itdestdata;
        for (int i = 0; i < count; i++) {
            itdata = data + i * dataLen;
            itdestdata = destdata + offset * dataLen;
	        memset(itdestdata, 0, dataLen);

            ignore = 0;
            hashkey = hashcbs->getkey(node, itdata, &ignore);

            if(ignore)
                continue;

            if(hashkey && add_hash(ht, hashkey, warn, sizeof(warn), node->redpath, append_duplicate)) {
                free((char *)hashkey);
                continue;
            }

            if(hashcbs->setdata(node, itdestdata, itdata, hashkey, strlen(warn) > 0 ? warn : NULL, expand)) {
                RedLog(REDLOG_ERROR, "Issue set data");
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
