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

#include "mergeconfig.h"

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
    {{0, 0, 0}, 0}
};

static const char SHORTOPTS[] = "r:he";

static void mergeconfigUsage(const rOption *options) {
    printf("Usage command mergeconfig: redconf [OPTION]... [config]... [OPTION]...\n"
    );
    usageOptions(options);
    exit(1);
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
                mergeconfigUsage(cOptions);
            case '?': //error getopt_long
                goto OnErrorExit;
            default:
                mergeconfigUsage(cOptions);
                break;
        }
    }
    return 0;
OnErrorExit:
    return -1;
}

/* main mergeconfig sub command */
int mergeconfig(const rGlobalConfigT * gConfig) {
    int err;

    rMergeconfigConfigT cConfig = {0};
    if(parseMergeconfigArgs(gConfig->sub_argc, gConfig->sub_argv, &cConfig) < 0)
        goto OnErrorExit;


    RedLog(REDLOG_DEBUG, "[mergeconfig]: redpath=%s", cConfig.redpath);
    err = RedDumpNodePathMerge(cConfig.redpath, cConfig.expand);

    return err;
OnErrorExit:
    return -1;
}