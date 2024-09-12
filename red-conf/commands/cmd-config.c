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

#include "cmd-config.h"

#include <string.h>

#include "options.h"

#include "../redconf.h"

/***************************************
 **** Config sub command ****
 **************************************/

typedef struct {
    const char *redpath;
} rConfigConfigT;

static rOption cOptions[] = {
    {{"redpath", required_argument, 0,  'r' }, "path to the node"},
    {{"help"   , no_argument      , 0,  'h' }, "print this help"},
    {{0, 0, 0, 0}, 0}
};

static void configUsage(const rOption *options, int exitcode) {
    printf("Usage: redconf config [OPTION]... [redpath]\n"
    );
    usageOptions(options);
    exit(exitcode);
}

static int parseConfigArgs(int argc, char * argv[], rConfigConfigT *cConfig) {
    struct option longOpts[sizeof(struct option) * sizeof(cOptions) / sizeof(rOption)];
    setLongOptions(cOptions, longOpts);

    while(1) {
        int option = getopt_long(argc, argv, "r:h", longOpts, NULL);
        if(option == -1)
            break;

        // option return short option even when long option is given
        switch (option) {
            case 'r':
                cConfig->redpath = optarg;
                break;
            case 'h':
                configUsage(cOptions, EXIT_SUCCESS);
                break;
            default:
                configUsage(cOptions, EXIT_FAILURE);
                break;
        }
    }
    if (optind + 1 == argc)
        cConfig->redpath = argv[optind];
    if (!cConfig->redpath)
        cConfig->redpath = ".";
    return 0;
}

/* main config sub command */
int config(const rGlobalConfigT * gConfig) {
    rConfigConfigT cConfig = {0};
    if(parseConfigArgs(gConfig->sub_argc, gConfig->sub_argv, &cConfig) < 0)
        return -1;

    RedLog(REDLOG_DEBUG, "[config]: redpath=%s", cConfig.redpath);
    return RedDumpFamilyNodePath(cConfig.redpath, gConfig->yaml, gConfig->verbose);
}