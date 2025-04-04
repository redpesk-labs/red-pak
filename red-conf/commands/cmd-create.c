/*
* Copyright (C) 2024-2025 IoT.bzh Company
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

#include "cmd-create.h"

#include <stdlib.h>
#include <string.h>

#include "options.h"

#include "../redconf.h"

#define DEFAULT_REDROOT "/"

/***************************************
 **** Tree sub command ****
 **************************************/

typedef struct {
    const char *redpath;
    const char *redroot;
    const char *alias;
    const char *normal;
    const char *admin;
    const char *tempdir;
    bool updt;
} rCreateConfigT;

static const rOption treeOptions[] = {
    {{"alias"     , required_argument, 0,  'a' }, "alias of the node"},
    {{"config"    , required_argument, 0,  'c' }, "template configuration of the node, name or path"},
    {{"config-adm", required_argument, 0,  'C' }, "template admin configuration of the node, name or path"},
    {{"full"      , no_argument      , 0,  'f' }, "like -m full"},
    {{"help"      , no_argument      , 0,  'h' }, "print this help"},
    {{"leaf"      , no_argument      , 0,  'l' }, "like -m leaf"},
    {{"model"     , required_argument, 0,  'm' }, "model name of template configuration"},
    {{"node"      , required_argument, 0,  'n' }, "path of the rednode"},
    {{"root"      , no_argument      , 0,  'r' }, "like -m root"},
    {{"redroot"   , required_argument, 0,  'R' }, "path to root node"},
    {{"templates" , required_argument, 0,  't' }, "default directory of templates"},
    {{"update"    , no_argument      , 0,  'u' }, "update an existing node"},
    {{0, 0, 0, 0}, 0}
};

static void createUsage(const rOption *options, int exitcode) {
    printf("Usage: redconf create [OPTION]... [redpath [redroot]]\n"
    );
    usageOptions(options);
    exit(exitcode);
}

static int parseCreateArgs(int argc, char * argv[], rCreateConfigT *createConfig) {
    struct option longOpts[sizeof(struct option) * sizeof(treeOptions) / sizeof(rOption)];

    createConfig->redpath = NULL;
    createConfig->redroot = NULL;
    createConfig->alias   = NULL;
    createConfig->normal  = NULL;
    createConfig->admin   = NULL;
    createConfig->tempdir = NULL;
    createConfig->updt    = false;

    setLongOptions(treeOptions, longOpts);
    while(1) {
        int option = getopt_long(argc, argv, "a:c:C:fhlm:n:rR:t:u", longOpts, NULL);
        if(option == -1)
            break;

        switch (option) {
            case 'a':
                createConfig->alias = optarg;
                break;
            case 'c':
                createConfig->normal = optarg;
                break;
            case 'C':
                createConfig->admin = optarg;
                break;
            case 'f':
                createConfig->normal = createConfig->admin = rednode_factory_deftmpl_full;
                break;
            case 'h':
                createUsage(treeOptions, EXIT_SUCCESS);
                break;
            case 'l':
                createConfig->normal = createConfig->admin = rednode_factory_deftmpl_leaf;
                break;
            case 'm':
                createConfig->normal = createConfig->admin = optarg;
                break;
            case 'n':
                createConfig->redpath = optarg;
                break;
            case 'r':
                createConfig->normal = createConfig->admin = rednode_factory_deftmpl_root;
                break;
            case 'R':
                createConfig->redroot = optarg;
                break;
            case 't':
                createConfig->tempdir = optarg;
                break;
            case 'u':
                createConfig->updt = true;
                break;
            default:
                createUsage(treeOptions, EXIT_FAILURE);
                break;
        }
    }
    if (optind < argc) {
        createConfig->redpath = argv[optind++];
        if (optind < argc)
            createConfig->redroot = argv[optind];
    }
    if (createConfig->redpath == NULL) {
        RedLog(REDLOG_ERROR, "redpath is missing");
        return -1;
    }
    if (createConfig->redroot == NULL)
        createConfig->redroot = DEFAULT_REDROOT;
    return 0;
}

static int perform_create_node(const rCreateConfigT *createConfig)
{
    int rc;
    rednode_factory_t rfab;
    rednode_factory_param_t params;

    params.alias = createConfig->alias;
    params.normal = createConfig->normal;
    params.admin = createConfig->admin;
    params.templatedir = createConfig->tempdir;

    RedLog(REDLOG_INFO, "[create]: redroot=%s", createConfig->redroot);
    RedLog(REDLOG_INFO, "[create]: redpath=%s", createConfig->redpath);
    RedLog(REDLOG_INFO, "[create]: alias=%s", createConfig->alias ?: "");
    RedLog(REDLOG_INFO, "[create]: normal=%s", createConfig->normal ?: "");
    RedLog(REDLOG_INFO, "[create]: admin=%s", createConfig->admin ?: "");
    RedLog(REDLOG_INFO, "[create]: update=%s", createConfig->updt ? "yes" : "no");

    rednode_factory_clear(&rfab);
    rc = rednode_factory_set_root(&rfab, createConfig->redroot);
    if (rc != RednodeFactory_OK) {
        RedLog(REDLOG_ERROR, "invalid rootpath %s: %s", createConfig->redroot, rednode_factory_error_text(-rc));
        return -1;
    }
    rc = rednode_factory_set_node(&rfab, createConfig->redpath);
    if (rc != RednodeFactory_OK) {
        RedLog(REDLOG_ERROR, "invalid redpath %s: %s", createConfig->redpath, rednode_factory_error_text(-rc));
        return -1;
    }
    RedLog(REDLOG_DEBUG, "[create]: rfab.root=%.*s", rfab.root_length, rfab.path);
    RedLog(REDLOG_DEBUG, "[create]: rfab.node=%.*s", rfab.node_length, rfab.path);

    rc = rednode_factory_create_node(&rfab, &params, createConfig->updt, RednodeFactory_Mode_Default);
    if (rc != RednodeFactory_OK) {
        RedLog(REDLOG_ERROR, "creation of node failed: %s", rednode_factory_error_text(-rc));
        return -1;
    }

    return 0;
}

/* main create sub command */
int create(const rGlobalConfigT *gConfig) {

    rCreateConfigT createConfig = {0};

    if(parseCreateArgs(gConfig->sub_argc, gConfig->sub_argv, &createConfig) < 0)
        return -1;

    return perform_create_node(&createConfig);
}

