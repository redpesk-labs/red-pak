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
#include <sys/wait.h>
#include <string.h>


// missing from fedora-30 !!!
int memfd_create (const char *__name, unsigned int __flags);



#include "redwrap.h"

static struct option options[] = {
	{"verbose", optional_argument, 0,  'v' },
	{"redpath", required_argument, 0,  'r' },
	{"rpath"  , required_argument, 0,  'r' },
	{"rp"     , required_argument, 0,  'r' },
	{"redmain", required_argument, 0,  'c' },
	{"rmain"  , required_argument, 0,  'c' },
	{"bwrap"  , required_argument, 0,  'b' },
	{"force"  , no_argument      , 0,  'f' },
	{"unsafe" , no_argument      , 0,  'u' },
	{"help"   , no_argument      , 0,  '?' },
	{"--"     , no_argument      , 0,  '-' },
	{0,         0,                 0,  0 }
};


rWrapConfigT *RwrapParseArgs(int argc, char *argv[]) {
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
	fprintf (stderr, "usage: red-wrap --rpath=... [--verbose] [--force] [--rmain=.../main.yaml] [--] program arg-1... arg-n\n");
	return NULL;
}


// Exec a command in a memory buffer and return stdout result as FD
const char* MemFdExecCmd (const char* mount, const char* command) {
	int err;
	char fdstr[32];
	int fd = memfd_create (mount, 0);
	if (fd <0) goto OnErrorExit;

	int pid = fork();
	if (pid != 0) {
		// wait for child to finish
		(void)wait(NULL);
		lseek (fd, 0, SEEK_SET);
		syncfs(fd);
	} else {
		// redirect stdout to fd and exec command
		char *argv[4];
		argv[0]="bash";
		argv[1]="-c";
		argv[2]=(char*)command;
		argv[3]=NULL;

		dup2(fd, 1);
		close (fd);
		execv("/usr/bin/bash", argv);
		fprintf (stderr, "hoops: red-wrap exec command return command=%s\n", command);
		exit;
	}

	// argv require string
    snprintf (fdstr, sizeof(fdstr), "%d", fd);
	return strdup(fdstr);

OnErrorExit:
	fprintf (stderr, "error: red-wrap Fail to exec command=%s\n", command);
	return NULL;
}

void RedPutEnv (const char*key, const char*value) {
	char string[RED_MAXPATHLEN];

	snprintf(string, sizeof(string),"%s=%s", key, value);
	putenv (strdup(string));

	const char*test= getenv(key);
}

// if string is not null extract umask and apply
mode_t RedSetUmask (redConfTagT *conftag) {
	mode_t mask;
	if (!conftag || !conftag->umask) {
        mask= umask(0);
    } else {
		sscanf (conftag->umask, "%o", &mask);
	}
    // posix umask would need some helpers
	(void)umask(mask);
	return mask;
}
