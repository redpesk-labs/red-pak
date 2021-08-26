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

// Small wrapper on top of bwrap. Read everynone node config from redpath
// generated corresponding environment variables LD_PATh, PATH, ...
// generate all sharing and mouting point
// exec bwrap
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "redwrap-main.h"

typedef struct {
    char *ldpathString;
    unsigned int ldpathIdx;
    char *pathString;
    unsigned int pathIdx;

} dataNodeT;

static redConfigT *loadRootNode(const char * path, int verbose, redConfTagT *mergedConfTags, dataNodeT *dataNode) {
    int error;

    redConfigT *redconf = RedLoadConfig (path, verbose);
    if (!redconf) {
        RedLog(REDLOG_ERROR, "redts_init: fail to parse redpak main cliarg at:'%s' ", path);
	    goto OnErrorExit;
    }

    // load 1st root tags (lowest priority)
    if(!redconf->conftag)
        return redconf;

    RedConfCopyConfTags (redconf->conftag, mergedConfTags);

    // update process default umask
    mode_t confmask=RedSetUmask (mergedConfTags);

    // node looks good extract path/ldpath before adding red-wrap cli program+arguments
    error= RedConfAppendEnvKey(dataNode->ldpathString, &dataNode->ldpathIdx, BWRAP_MAXVAR_LEN, redconf->conftag->ldpath, NULL, NULL, NULL);
    if (error) goto OnErrorExit;

    error= RedConfAppendEnvKey(dataNode->pathString, &dataNode->pathIdx, BWRAP_MAXVAR_LEN, redconf->conftag->path, NULL, NULL, NULL);
    if (error) goto OnErrorExit;

    return redconf;
OnErrorExit:
    return NULL;
}


void redwrapMain (const char *command_name, rWrapConfigT *cliarg, int subargc, char *subargv[]) {

    redConfTagT *mergedConfTags= calloc(1, sizeof(redConfTagT));
    int argcount=0;
    int error;
    const char *argval[MAX_BWRAP_ARGS];
    char pathString[BWRAP_MAXVAR_LEN];
    char ldpathString[BWRAP_MAXVAR_LEN];
    dataNodeT dataNode = {
        .ldpathString = ldpathString,
        .ldpathIdx = 0,
        .pathString = pathString,
        .pathIdx = 0
    };

    // start argument list with red-wrap command name
    argval[argcount++] = command_name;

    redConfigT *redconfadmin = NULL;
    redConfigT *redconf = loadRootNode(cliarg->cnfpath, cliarg->verbose, mergedConfTags, &dataNode);
    if(!redconf) goto OnErrorExit;
    // if admin: overload conf
    if (cliarg->adminpath) {
        redconfadmin = loadRootNode(cliarg->adminpath, cliarg->verbose, mergedConfTags, &dataNode);
        if(!redconfadmin) goto OnErrorExit;
    }

    // update verbose/redpath from cliarg
    const char *redpath = cliarg->redpath;

    redNodeT *redtree = RedNodesScan(redpath, cliarg->verbose);
    if (!redtree) {
        RedLog(REDLOG_ERROR, "Fail to scan rednodes family tree redpath=%s", redpath);
        goto OnErrorExit;
    }

    // push NODE_ALIAS in case some env var expand it.
    RedPutEnv("LEAF_ALIAS", redtree->config->headers->alias);
    RedPutEnv("LEAF_NAME" , redtree->config->headers->name);
    RedPutEnv("LEAF_PATH" , redtree->status->realpath);

    // build a phony root node for config variable expansion
    redNodeT rootnode;
    rootnode.redpath=redtree->status->realpath;
    rootnode.config= redconf;
    rootnode.config->headers=redtree->config->headers;
    rootnode.confadmin = redconfadmin;
    rootnode.ancestor=NULL;

    error = RwrapParseConfig (&rootnode, cliarg, 0, argval, &argcount);
    if (error) goto OnErrorExit;


    // build arguments from nodes family tree
        // Scan redpath family nodes from terminal leaf to root node
    for (redNodeT *node=redtree; node != NULL; node=node->ancestor) {

        RedConfCopyConfTags (node->config->conftag, mergedConfTags);
        if(node->confadmin && node->confadmin->conftag)
            RedConfCopyConfTags (node->confadmin->conftag, mergedConfTags);

        int error = RwrapParseNode (node, cliarg, (node == redtree), argval, &argcount);
        if (error) goto OnErrorExit;

        // node looks good extract path/ldpath before adding red-wrap cli program+arguments
        error= RedConfAppendEnvKey(dataNode.ldpathString, &dataNode.ldpathIdx, BWRAP_MAXVAR_LEN, node->config->conftag->ldpath, NULL, ":", NULL);
        if (error) goto OnErrorExit;

        error= RedConfAppendEnvKey(dataNode.pathString, &dataNode.pathIdx, BWRAP_MAXVAR_LEN, node->config->conftag->path, NULL, ":",NULL);
        if (error) goto OnErrorExit;

    }

    // add commulated LD_PATH_LIBRARY & PATH
    argval[argcount++]="--setenv";
    argval[argcount++]="PATH";
    argval[argcount++]=strdup(pathString);
    argval[argcount++]="--setenv";
    argval[argcount++]="LD_LIBRARY_PATH";
    argval[argcount++]=strdup(ldpathString);


    // set global merged config tags
    if (mergedConfTags->hostname) {
        argval[argcount++]="--unshare-uts";
        argval[argcount++]="--hostname";
        argval[argcount++]= RedNodeStringExpand (redtree, NULL, mergedConfTags->hostname, NULL, NULL);
    }

    if (mergedConfTags->chdir) {
        argval[argcount++]="--chdir";
        argval[argcount++]= RedNodeStringExpand (redtree, NULL, mergedConfTags->chdir, NULL, NULL);
    }

    if (mergedConfTags->share_all == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-all";
    } else if (mergedConfTags->share_all == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-all";
    }

    if (mergedConfTags->share_user == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-user";
    } else  if (mergedConfTags->share_user == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-user";
    }

    if (mergedConfTags->share_cgroup == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-cgroup";
    } else if (mergedConfTags->share_cgroup == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-cgroup";
    }
    if (mergedConfTags->share_ipc == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-ipc";
    } else if (mergedConfTags->share_ipc == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-pic";
    }

    if (mergedConfTags->share_pid == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-pid";
    } else if (mergedConfTags->share_pid == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-pid";
    }

    if (mergedConfTags->share_net == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--share-net";
    } else if (mergedConfTags->share_net == RED_CONF_OPT_DISABLED) {
        argval[argcount++]="--unshare-net";
    }

    if (mergedConfTags->diewithparent == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--die-with-parent";
    }

    if (mergedConfTags->newsession == RED_CONF_OPT_ENABLED) {
        argval[argcount++]="--new-session";
    }

    // add remaining program to execute arguments
    for (int idx=0; idx < subargc; idx++ ) {
        if (idx == MAX_BWRAP_ARGS) {
            RedLog(REDLOG_ERROR,"red-wrap too many arguments limit=[%s]", MAX_BWRAP_ARGS);
        }
        argval[argcount++]=subargv[idx];
    }

    if (cliarg->verbose) {
        for (int idx=1; idx < argcount; idx++) {
            printf (" %s", argval[idx]);
        }
        printf ("\n");
    }

    // exec command
    argval[argcount]=NULL;
    if(execv(cliarg->bwrap, (char**) argval));
        RedLog(REDLOG_ERROR, "bwrap commend issue: %s", strerror(errno));

OnErrorExit:
    RedLog(REDLOG_ERROR,"red-wrap aborted");
    exit(1);
}
