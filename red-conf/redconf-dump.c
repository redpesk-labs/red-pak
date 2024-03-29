/*
* Copyright (C) 2022 "IoT.bzh"
* Author Clément Bénier <clement.benier@iot.bzh>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "redconf-dump.h"
#include "redconf-utils.h"
#include "redconf-merge.h"

void RedDumpStatusHandle (redStatusT *status) {
   // retrieve label attach to state flag
    const char* flag = statusFlagStrings[status->state].str;

    printf ("---\n");
    printf ("  state=%s'\n", flag);
    printf ("  realpath=%s'\n", status->realpath);
    printf ("  timestamp=%ld'\n", status->timestamp);
    printf ("  info=%s'\n", status->info);
    printf ("---\n\n");
}


// Dump config filr, in verbose mode print ongoing parsing to help find errors
int RedDumpStatusPath (const char *filepath, int warning) {

    redStatusT *status;

    status = RedLoadStatus(filepath, warning);
    if (!status) goto OnErrorExit;
    printf ("# Dumping yaml red-status path=%s\n\n", filepath);
    RedDumpStatusHandle(status);
    return 0;

OnErrorExit:
    return 1;
}

void RedDumpConftag(redConfTagT *conftag) {
    printf ("config:\n");
    printf("\tcachedir: %s\n", conftag->cachedir);
    printf("\thostname: %s\n", conftag->hostname);
    printf("\tchdir: %s\n", conftag->chdir);
    printf("\tumask: %s\n", conftag->umask);
    printf("\tverbose: %d\n", conftag->verbose);
    printf("\tdiewithparent: %d\n", conftag->diewithparent);
    printf("\tumask: %s\n", conftag->umask);
    printf("\tnewsession: %s\n", redConfOptStrings[conftag->newsession].str);
    printf("\tshare_all: %s\n",  redConfOptStrings[conftag->share_all].str);
    printf("\tshare_user: %s\n", redConfOptStrings[conftag->share_user].str);
    printf("\tshare_cgroup: %s\n", redConfOptStrings[conftag->share_cgroup].str);
    printf("\tshare_ipc: %s\n", redConfOptStrings[conftag->share_ipc].str);
    printf("\tshare_pid: %s\n", redConfOptStrings[conftag->share_pid].str);
    printf("\tshare_net: %s\n", redConfOptStrings[conftag->share_net].str);
    printf("\tshare_time: %s\n", redConfOptStrings[conftag->share_time].str);
    //printf("cgroups=%s\n", conftag->cgroups);
}

static void RedDumpExport(redConfExportPathT *export, int idx) {
    printf ("  - [%d] mode:  %s\n", idx, exportFlagStrings[export->mode].str);
    printf ("         mount: %s\n", export->mount);
    printf ("         path:  %s\n", export->path);
}

void RedDumpConfigHandle(redConfigT *config) {

    printf ("---\n");
    printf ("headers=> alias=%s name=%s info='%s'\n", config->headers->alias, config->headers->name, config->headers->info);
    RedDumpConftag(config->conftag);
    printf ("exports:\n");
    for (int idx=0; idx < config->exports_count; idx++) {
        RedDumpExport(config->exports+idx, idx);
    }
    printf ("---\n\n");
}

// Dump config filr, in verbose mode print ongoing parsing to help find errors
int RedDumpConfigPath (const char *filepath, int warning) {

    redConfigT *config;

    config = RedLoadConfig (filepath, warning);
    if (!config) goto OnErrorExit;
    printf ("# Dumping yaml red-config path=%s\n\n", filepath);
    RedDumpConfigHandle (config);
    return 0;

OnErrorExit:
    return 1;
}

// Debug tool dump a redpak node family tree
void RedDumpFamilyNodeHandle(redNodeT *familyTree, int yaml) {

    for (redNodeT *node=familyTree; node != NULL; node=node->ancestor) {
        if(yaml) {
            char *output;
            size_t len;
            RedGetConfig(&output, &len, node->config);
            printf("----CONFIG %s\n", node->redpath);
            printf("%s\n", output);
            printf("----\n");
            free(output);
        } else {
            RedDumpConfigHandle(node->config);
            RedDumpStatusHandle(node->status);
        }
    }
}

// Dump a redpath family node tree
int RedDumpFamilyNodePath (const char* redpath, int yaml, int verbose) {
    int ret = 0;
    redNodeT *familyTree = RedNodesScan(redpath, verbose);
    if (!familyTree) {
        RedLog(REDLOG_ERROR, "Fail to scandown redpath family tree path=%s\n", redpath);
        goto OnErrorExit;
    }

    RedDumpFamilyNodeHandle (familyTree, yaml);

OnErrorExit:
    freeRedLeaf(familyTree);
    return ret;
}

int RedDumpRoot(redNodeT *node, const char* prefix, int verbose, int last) {
    const char *s_last = "\U00002514";
    const char *s_transition = "\U0000251C";
    const char *s_transition_prefix = "\U00002502";
    const char *s_link = "\U00002500\U00002500";
    char subprefix[RED_MAXPATHLEN];
    char node_basename[RED_MAXPATHLEN + 1];

    strncpy(node_basename, node->redpath, sizeof(node_basename) - 1);
    node_basename[sizeof(node_basename) - 1] = 0;
    //subprefix for children depends on last
    (void)snprintf(subprefix, sizeof(subprefix), "  %s%s ", prefix, last ? " " : s_transition_prefix);

    //print current node
    printf( "%s  %s%s %s  (%s)\n", prefix, last ? s_last : s_transition, s_link, basename(node_basename), node->config->headers->alias);

    //handle children
    for (redChildNodeT *childs = node->childs; childs && childs->child; childs = childs->brother) {
        if(RedDumpRoot(childs->child, subprefix, verbose, childs->brother ? 0 : 1))
            goto OnErrorExit;
    }

    return 0;
OnErrorExit:
    return 1;
}

int RedDumpTree(const char *redrootpath, int verbose) {
    int ret = 0;
    redNodeT *redroot = RedNodesDownScan(redrootpath, verbose);
    if (!redroot) {
        RedLog(REDLOG_ERROR, "Fail to scandown redroot family tree path=%s\n", redroot);
        ret = 1;
        goto OnErrorExit;
    }

    char node_dirname[RED_MAXPATHLEN + 1];
    strncpy(node_dirname, redroot->redpath, sizeof(node_dirname) - 1);
    node_dirname[sizeof(node_dirname) - 1] = 0;

    //print root path
    printf("%s\n", dirname(node_dirname));
    if(RedDumpRoot(redroot, "", verbose, 1)) {
        goto OnErrorFreeExit;
        ret = 2;
    }

OnErrorFreeExit:
    freeRedRoot(redroot);
OnErrorExit:
    return ret;
}

/*
// Debug tool dump a merge redpak node tree
static void RedDumpNodeMerge(redNodeT *redroot) {
    redConfTagT * mergeConftag = mergedConftags(redroot, 0);
    RedDumpConftag(mergeConftag);
    free(mergeConftag);
}
*/

static int DumpGetConfig(redConfigT *config) {
    char *output;
    size_t len;
    int ret = RedGetConfig(&output, &len, config);
    if (ret){
        free(output);
        return ret;
    }
    printf("----CONFIG %ld\n",len);
    for (int i = 0; i < len; i++)
        printf("%c", output[i]);
    printf("----\n");
    free(output);
    return 0;
}

int RedDumpNodePathMerge(const char* redpath, int expand) {
    redNodeT *redroot, *mergedNode, *node;
    int ret = 0;

    node = RedNodesScan(redpath, 0);
    if(!node)
        goto OnErrorExit;

    //looking for redroot
    for (redroot = node; redroot->ancestor; redroot = redroot->ancestor);
    mergedNode = mergeNode(node, redroot, expand, 1);

    RedLog(REDLOG_DEBUG, "redroot path:%s", redroot->redpath);
    ret = DumpGetConfig(mergedNode->config);

    freeRedLeaf(mergedNode);
    freeRedLeaf(node);

    return ret;

OnErrorExit:
    return 1;
}