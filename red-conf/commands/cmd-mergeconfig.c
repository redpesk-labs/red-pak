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

#include "cmd-mergeconfig.h"

#include <string.h>

#include "options.h"

#include "../redconf.h"

/***************************************
 **** Mergeconfig sub command ****
 **************************************/

typedef struct {
    const char *redpath;
    int expand;
} rMergeconfigConfigT;

static rOption cOptions[] = {
    {{"redpath", required_argument, 0,  'r' }, "path to the node"},
    {{"expand",  no_argument,       0,  'e' }, "expand variables in config file"},
    {{"help"   , no_argument      , 0,  'h' }, "print this help"},
    {{0, 0, 0, 0}, 0}
};

static const char SHORTOPTS[] = "r:he";

static void mergeconfigUsage(const rOption *options, int exitcode) {
    printf("Usage: redconf mergeconfig [OPTION]... [redpath]\n"
    );
    usageOptions(options);
    exit(exitcode);
}

static int parseMergeconfigArgs(int argc, char * argv[], rMergeconfigConfigT *cConfig) {
    struct option longOpts[sizeof(struct option) * sizeof(cOptions) / sizeof(rOption)];
    setLongOptions(cOptions, longOpts);

    while(1) {
           int option = getopt_long(argc, argv, SHORTOPTS, longOpts, NULL);
        if(option == -1)
            break;

        // option return short option even when long option is given
          switch (option) {
            case 'r':
                cConfig->redpath = optarg;
                break;
            case 'e':
                cConfig->expand = 1;
                break;
            case 'h':
                mergeconfigUsage(cOptions, EXIT_SUCCESS);
                break;
            default:
                mergeconfigUsage(cOptions, EXIT_FAILURE);
                break;
        }
    }
    if (optind + 1 == argc)
        cConfig->redpath = argv[optind];
    if (!cConfig->redpath)
        cConfig->redpath = ".";
    return 0;
}

/* main mergeconfig sub command */
int mergeconfig(const rGlobalConfigT * gConfig) {

    rMergeconfigConfigT cConfig = {0};
    if(parseMergeconfigArgs(gConfig->sub_argc, gConfig->sub_argv, &cConfig) < 0)
        return -1;


    RedLog(REDLOG_DEBUG, "[mergeconfig]: redpath=%s", cConfig.redpath);
    return RedDumpNodePathMerge(cConfig.redpath, cConfig.expand);
}