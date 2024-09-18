/*
* Copyright (C) 2022 "IoT.bzh"
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//include subcommands
#include "cmd-config.h"
#include "cmd-mergeconfig.h"
#include "cmd-tree.h"
#include "cmd-create.h"
#include "options.h"

#include "../redconf.h"
#include "../redconf-defaults.h"


typedef struct {
    const char *cmd_name;
    int (*cmd)(const rGlobalConfigT *);
    const char *desc;
} rCommandT;

/***************************************
 **** Global command ****
 **************************************/

//all subcommand of redconf
static const rCommandT commands[] = {
    {"config", config, "get configuration files from nodes"},
    {"mergeconfig", mergeconfig, "get merge configuration files from nodes"},
    {"tree", tree, "get tree nodes"},
    {"create", create, "creates a node"},
    {0, 0, 0}
};

const RedLogLevelE verbose_default = REDLOG_WARNING;

static const rOption globalOptions[] = {
    {{"verbose",    optional_argument, (int *)&verbose_default, 'v'}, "increase verbosity to info"},
    {{"yaml",       no_argument      ,     0,                   'y'}, "yaml output"},
    {{"log-yaml",   required_argument,     0,                   'l'}, "yaml parse log level (max=4)"},
    {{"help",       no_argument      ,     0,                   'h'}, "print this help"},
    {{0, 0, 0, 0}, 0}
};

/*
* short options with a + at beginning for preserving order
* and stopping at the first argument not being an option
*/
static const char SHORTOPTS[] = "+v::hyl:";

static void globalUsage(const rOption *options, int exitcode) {
    printf("Usage: redconf [OPTION]... COMMAND [OPTION]...\n"
           "redpak configuration commands\n"
           "\n"
           "Commands:\n"
    );
    for (const rCommandT *cmd = commands; cmd->cmd; cmd++)
        printf("\t%-13s\t%s\n", cmd->cmd_name, cmd->desc);

    usageOptions(options);
    exit(exitcode);
}

static int globalParseArgs(int argc, char *argv[], rGlobalConfigT *gConfig) {
    int option;
    RedLogLevelE verbosity;
    struct option longOpts [sizeof(struct option) * sizeof(globalOptions) / sizeof(globalOptions[0])];

    setLongOptions(globalOptions, longOpts);
    optind = 0; /* force reset */
    while(1) {
        option = getopt_long(argc, argv, SHORTOPTS, longOpts, NULL);

        // option return short option even when long option is given
        switch (option) {
            case 'v':
                verbosity = REDLOG_INFO;
                if(optarg) {
                    size_t length = strlen(optarg);
                    if (!strncmp(optarg, "v", length)) {
                        verbosity = REDLOG_DEBUG;
                    } else if (!strncmp(optarg, "vv", length)) {
                        verbosity = REDLOG_TRACE;
                    } else {
                        optarg = NULL;
                    }
                }
                SetLogLevel(verbosity);
                gConfig->verbose = 1;
                break;

            case 'y':
                gConfig->yaml = 1;
                break;

            case 'l':
                setLogYaml(atoi(optarg));
                break;

            case 'h':
                globalUsage(globalOptions, EXIT_SUCCESS);
                break;

            default:
                globalUsage(globalOptions, EXIT_FAILURE);
                break;

            case -1: //no more options
                gConfig->cmd = argv[optind];
                gConfig->sub_argc = argc - optind;
                gConfig->sub_argv = argv + optind;
                optind = 0; /* force reset */
                return 0;
        }
    }
}

int main (int argc, char *argv[]) {
    rGlobalConfigT gConfig = {0};
    if (globalParseArgs(argc, argv, &gConfig) < 0)
        return 1;

    //if no sub command issue
    if(!gConfig.cmd) {
        RedLog(REDLOG_ERROR, "No command options found\n");
    }
    else {
        //call right sub command
        for(const rCommandT *cmd = commands; cmd->cmd_name; cmd++) {
            if (!strcmp(cmd->cmd_name, gConfig.cmd)) {
                return cmd->cmd(&gConfig) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
            }
        }
        RedLog(REDLOG_ERROR, "Invalid command %s\n", gConfig.cmd);
    }
    globalUsage(globalOptions, EXIT_FAILURE);
    return EXIT_FAILURE;
}