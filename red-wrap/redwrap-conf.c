/*
 * Copyright (C) 2020 "IoT.bzh"
 * Author fulup Ar Foll <fulup@iot.bzh>
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>



#include "redwrap.h"

static struct option options[] = {
    {"verbose", optional_argument, 0,  'v' },
    {"redpath", required_argument, 0,  'r' },
    {"rpath"  , required_argument, 0,  'r' },
    {"rp"     , required_argument, 0,  'r' },
    {"redmain", required_argument, 0,  'c' },
    {"rmain"  , required_argument, 0,  'c' },
    {"bwrap"  , required_argument, 0,  'b' },
    {"admin"  , optional_argument, 0,  'a' },
    {"force"  , no_argument      , 0,  'f' },
    {"unsafe" , no_argument      , 0,  'u' },
    {"help"   , no_argument      , 0,  '?' },
    {"--"     , no_argument      , 0,  '-' },
    {0,         0,                 0,  0 }
};

rWrapConfigT *RwrapParseArgs(int argc, char *argv[], const char *usage) {
    rWrapConfigT *config = calloc (1, sizeof(rWrapConfigT));
     int index;

    for (int done=0; !done;) {
        int option = getopt_long(argc, argv, "vp:m:", options, &index);

        if (option == -1) {
            config->index= optind;
            break;
        }

        // option return short option even when long option is given
        switch (option) {
            case 'r':
                config->redpath=optarg;
                break;

            case 'v':
                config->verbose++;
                  if (optarg)	config->verbose = atoi(optarg);
                break;

            case 'c':
                config->cnfpath=optarg;
                break;

            case 'a':
                config->adminpath = getenv("redpak_MAIN_ADMIN");
                if (!config->adminpath) config->adminpath= redpak_MAIN_ADMIN;
                if (optarg) config->adminpath = optarg;
                break;

            case 'b':
                config->bwrap=optarg;
                break;

            case 'f':
                config->forcemod=1;
                break;

            case 'u':
                config->unsafe=1;
                break;

            case '-':
                done=1;
                break;

            default:
                goto OnErrorExit;
        }
    }

    if (!config->cnfpath) {
        config->cnfpath = getenv("redpak_MAIN");
        if (!config->cnfpath) config->cnfpath= redpak_MAIN;
    }

    if (!config->bwrap) {
        config->bwrap= BWRAP_CMD_PATH;
    }

    if (!config->redpath)
        goto OnErrorExit;

    return config;

OnErrorExit:
    fprintf (stderr, "%s", usage);
    return NULL;
}
