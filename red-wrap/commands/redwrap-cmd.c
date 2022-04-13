/*
 * Copyright (C) 2022 "IoT.bzh"
 *
 * Author: Valentin Lefebvre <valentin.lefebvre@iot.bzh
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

//////////////////////////////////////////////////////////////////////////////
//                             DEFINE                                       //
//////////////////////////////////////////////////////////////////////////////

#define REDMICRODNF_CMD "redmicrodnf"
#define REDMICRODNF_CMD_PATH "/usr/bin/"REDMICRODNF_CMD

//////////////////////////////////////////////////////////////////////////////
//                             INCLUDE                                      //
//////////////////////////////////////////////////////////////////////////////

#include "redwrap-cmd.h"
#include <sys/wait.h>

//////////////////////////////////////////////////////////////////////////////
//                             STRUCTUERS                                   //
//////////////////////////////////////////////////////////////////////////////

struct red_cmd {
	const char *cmd; //cmd name
	int outnode; //context execution
};

/**
 * @brief list of redmicrodnf commands
 *
 */
static struct red_cmd redmicrodnf_commands [] = {
	{"install", 0},
	{"download", 0},
	{"downgrade", 0},
	{"groupinfo", 0},
	{"grouplist", 0},
	{"reinstall", 0},
	{"remove", 0},
	{"repolist", 0},
	{"repoinfo", 0},
	{"repoquery", 0},
	{"upgrade", 0},
	{"advisory", 0},
	{"manager", 1},
};

//////////////////////////////////////////////////////////////////////////////
//                             PRIVATE VARIABLES                            //
//////////////////////////////////////////////////////////////////////////////

static const char * redwrap_usage = "usage: redwrap --redpath=... [--verbose] [--force] [--admin[=.../main-admin.yaml]] [--rmain=.../main.yaml] -- program args\n";
static const char * redwrap_dnf_usage = "usage: redwrap-dnf --redpath=... [--verbose] [--force] [--admin[=.../main-admin.yaml]] [--rmain=.../main.yaml] [redmicrodnfcmd [args]]  \n\n";

//////////////////////////////////////////////////////////////////////////////
//                             PRIVATE FUNCTIONS                            //
//////////////////////////////////////////////////////////////////////////////

static struct red_cmd *_find_redmicrodnf_cmd (int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			continue;

		for (int j = 0; j < sizeof(redmicrodnf_commands)/sizeof(redmicrodnf_commands[0]); j++) {
			if(!strcmp(argv[i], redmicrodnf_commands[j].cmd)) {
				return &redmicrodnf_commands[j];
			}
		}
	}
	return NULL;
}

static int _exec_redmicrodnf(int argc, char *argv[], int argcount, char *subargv[]) {
    struct stat statinfo;
	int error = 0;

	/* check redmicrodnf exists */
   	error = stat(REDMICRODNF_CMD_PATH, &statinfo);
   	if (error || !S_ISREG(statinfo.st_mode)) {
		error = -1;
       	fprintf(stderr, "Not found redmicrodnf command: %s\n", REDMICRODNF_CMD_PATH);
		return error;
	}

	for (int i = 1; i < argc; i++) {
		subargv[argcount++] = argv[i];
	}
	subargv[argcount] = NULL;

	// Run in child process to dont stop the main runnning process
	int pid = fork();
	if (pid == 0) {
		error = execv(REDMICRODNF_CMD_PATH, (char**) subargv);
		if(error) {
			fprintf(stderr, "Issue exec outnode command %s error:%s\n", REDMICRODNF_CMD_PATH, strerror(errno));
			return 1;
		}
	}
	int returnStatus;
	waitpid(pid, &returnStatus, 0);
	return returnStatus;
}

static int _exec_bwrap(int argc, char *argv[], int argcount, char *subargv[]) {
    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, redwrap_dnf_usage);
    if (!cliarg) {
        return -1;
	}

	//admin is needed for redmicrodnf
	if(!cliarg->adminpath) {
		cliarg->adminpath = redpak_MAIN_ADMIN;
	}
	subargv[argcount++] = "--redpath";
	subargv[argcount++] = (char *)cliarg->redpath;


	for (int i = cliarg->index; i < argc; i++)
		subargv[argcount++] = argv[i];

	fprintf(stdout, "wrap main from bwrap!\n");
	return redwrapMain(argv[0], cliarg, argcount, subargv);
}

//////////////////////////////////////////////////////////////////////////////
//                             PUBLIC FUNCTIONS                             //
//////////////////////////////////////////////////////////////////////////////

int redwrap_dnf_cmd_exec(int argc, char *argv[]) {
	int error = 0;
	int argcount = 0;
    char *subargv[MAX_BWRAP_ARGS];

	//add redmicrodnf command
	subargv[argcount++] = REDMICRODNF_CMD;

    struct red_cmd * redcmd = _find_redmicrodnf_cmd(argc, argv);
	if (!redcmd) {
		fprintf(stderr, "No redmicrodnf command found!");
		return -1;
	}

    if (redcmd->outnode) {
		error = _exec_redmicrodnf(argc, argv, argcount, subargv);
		if (error) return -1;
	} else {
		error = _exec_bwrap(argc, argv, argcount, subargv);
		if (error) return -1;
	}

    return 0;
}

int redwrap_cmd_exec(int argc, char *argv[]) {
    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, redwrap_usage);
    if (!cliarg)
        return -1;
    return redwrapMain(argv[0], cliarg, argc - cliarg->index, argv + cliarg->index);
}