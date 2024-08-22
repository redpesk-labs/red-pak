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

#include <getopt.h>
#include <string.h>

#include "tree.h"

/***************************************
 **** Tree sub command ****
 **************************************/

typedef struct {
    const char *redroot;
} rTreeConfigT;

static const rOption treeOptions[] = {
    {{"redroot", required_argument, 0,  'r' }, "path to root nodes"},
    {{"help"   , no_argument      , 0,  'h' }, "print this help"},
    {{0, 0, 0}, 0}
};

static void treeUsage(const rOption *options) {
    printf("Usage command tree: redconf [OPTION]... [tree]... [OPTION]...\n"
    );
    usageOptions(options);
    exit(1);
}

static int parseTreeArgs(int argc, char * argv[], rTreeConfigT *treeConfig) {
    struct option longOpts[sizeof(struct option) * sizeof(treeOptions) / sizeof(rOption)];
    setLongOptions(treeOptions, longOpts);

    while(1) {
        int option = getopt_long(argc, argv, "r:h", longOpts, NULL);
        if(option == -1)
            break;

        // option return short option even when long option is given
        switch (option) {
            case 'r':
                treeConfig->redroot = optarg;
                break;
            case 'h':
                treeUsage(treeOptions);
            case '?': //error getopt_long
                goto OnErrorExit;
            default:
                treeUsage(treeOptions);
                break;
        }
    }
    return 0;
OnErrorExit:
    return -1;
}

/* main tree sub command */
int tree(const rGlobalConfigT *gConfig) {
    int err;

    rTreeConfigT treeConfig = {0};
    if(parseTreeArgs(gConfig->sub_argc, gConfig->sub_argv, &treeConfig) < 0)
        goto OnErrorExit;

    RedLog(REDLOG_DEBUG, "option: redroot=%s", treeConfig.redroot);

    err = RedDumpTree(treeConfig.redroot, 0);

    return err;
OnErrorExit:
    return -1;
}