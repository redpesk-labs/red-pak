/*
 * Copyright (C) 2020 "IoT.bzh"
 * Author fulup Ar Foll <fulup@iot.bzh>
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
 */


#define _GNU_SOURCE

#include "redwrap-conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "redconf-defaults.h"
#include "redconf-log.h"

#ifndef BWRAP_CMD_PATH
#define BWRAP_CMD_PATH "/usr/bin/bwrap"
#endif

static struct option options[] = {
    {"admin"  , optional_argument, 0,  'a' },
    {"bwrap"  , required_argument, 0,  'b' },
    {"dump"   , no_argument      , 0,  'd' },
    {"dump-only", no_argument    , 0,  'D' },
    {"force"  , optional_argument, 0,  'f' },
    {"help"   , no_argument      , 0,  '?' },
    {"redpath", required_argument, 0,  'r' },
    {"rpath"  , required_argument, 0,  'r' },
    {"rp"     , required_argument, 0,  'r' },
    {"strict" , optional_argument, 0,  's' },
    {"unsafe" , optional_argument, 0,  'u' },
    {"verbose", optional_argument, 0,  'v' },
    {0,         0,                 0,  0 }
};

static const char short_options[] = "a::b:Df::?r:s::u::v::";

static int optbool(const char *arg, int def, const char *option)
{
    if (arg == NULL)
        return def;
    if (!strcasecmp(arg, "yes"))
        return 1;
    if (!strcasecmp(arg, "no"))
        return 0;

    fprintf (stderr, "illegal boolean value '%s' for option %s (valid values: yes or no)", arg, option);
    exit(EXIT_FAILURE);
    return def;
}

rWrapConfigT *RwrapParseArgs(int argc, char *argv[], const char *usage) {
    rWrapConfigT *config = calloc (1, sizeof(rWrapConfigT));
    int index;

    optind = 0; /* force reset */
    for (int done=0; !done;) {
        int option = getopt_long(argc, argv, short_options, options, &index);

        if (option == -1) {
            config->index= optind;
            break;
        }

        // option return short option even when long option is given
        switch (option) {
            case 'a':
                config->isadmin = 1;
                if (optarg)
                    config->adminpath = optarg;
                break;

            case 'b':
                config->bwrap=optarg;
                break;

            case 'd':
                config->dump = 1;
                break;

            case 'D':
                config->dump = 2;
                break;

            case 'f':
                config->strict = !optbool(optarg, 1, "force");
                break;

            case 'r':
                config->redpath=optarg;
                break;

            case 's':
                config->strict = optbool(optarg, 1, "strict");
                break;

            case 'u':
                config->unsafe = optbool(optarg, 1, "unsafe");
                break;

            case 'v':
                if (optarg)
                    config->verbose = atoi(optarg);
                else
                    config->verbose = REDLOG_NOTICE;
                break;

            default:
                goto OnErrorExit;
        }
    }

    if (!config->bwrap)
        config->bwrap= BWRAP_CMD_PATH;

    if (!config->redpath)
        goto OnErrorExit;

    return config;

OnErrorExit:
    fprintf (stderr, "%s", usage);
    exit(EXIT_FAILURE);
    return NULL;
}
