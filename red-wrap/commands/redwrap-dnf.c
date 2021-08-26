/*
 Copyright (C) 2021 "IoT.bzh"

 author: Clément Bénier <clement.benier@iot.bzh>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include <string.h>
#include <errno.h>

#include "../redwrap-main.h"


struct red_cmd {
	const char *cmd; //cmd name
	int outnode; //context execution
};

/* list of redmicrodnf commands */
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

#define REDMICRODNF_CMD "redmicrodnf"
#define REDMICRODNF_CMD_PATH "/usr/bin/"REDMICRODNF_CMD

static const char * usage = "usage: redwrap-dnf --redpath=... [--verbose] [--force] [--admin[=.../main-admin.yaml]] [--rmain=.../main.yaml] [redmicrodnfcmd [args]]  \n\n";


static struct red_cmd *find_redmicrodnf_cmd (int argc, char *argv[]) {
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

static int exec_redmicrodnf(int argc, char *argv[], int argcount, char *subargv[]) {
    struct stat statinfo;
	int error = 0;

	/* check redmicrodnf exists */
   	error = stat(REDMICRODNF_CMD_PATH, &statinfo);
   	if (error || !S_ISREG(statinfo.st_mode)) {
		error = -1;
       	fprintf(stderr, "Not found redmicrodnf command: %s\n", REDMICRODNF_CMD_PATH);
		goto OnErrorExit;
	}

	for (int i = 1; i < argc; i++) {
		subargv[argcount++] = argv[i];
	}
	subargv[argcount] = NULL;

	error = execv(REDMICRODNF_CMD_PATH, (char**) subargv);
	if(error)
		fprintf(stderr, "Issue exec outnode command %s error:%s\n", REDMICRODNF_CMD_PATH, strerror(errno));

OnErrorExit:
	return error;
}

static int exec_bwrap(int argc, char *argv[], int argcount, char *subargv[]) {
    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, usage);
    if (!cliarg) goto OnErrorExit;

	//admin is needed for redmicrodnf
	if(!cliarg->adminpath)
		cliarg->adminpath = redpak_MAIN_ADMIN;

	subargv[argcount++] = "--redpath";
	subargv[argcount++] = (char *)cliarg->redpath;


	for (int i = cliarg->index; i < argc; i++)
		subargv[argcount++] = argv[i];

	redwrapMain(argv[0], cliarg, argcount, subargv);

OnErrorExit:
	return -1;
}

int main (int argc, char *argv[]) {
	int error = 0;
	int argcount = 0;
    char *subargv[MAX_BWRAP_ARGS];

	//add redmicrodnf command
	subargv[argcount++] = REDMICRODNF_CMD;

	struct red_cmd * redcmd = find_redmicrodnf_cmd(argc, argv);
	if (!redcmd) {
		fprintf(stderr, "No redmicrodnf command found!");
		goto OnErrorExit;
	}

	if (redcmd->outnode) {
		error = exec_redmicrodnf(argc, argv, argcount, subargv);
		if (error) goto OnErrorExit;
	} else {
		error = exec_bwrap(argc, argv, argcount, subargv);
		if (error) goto OnErrorExit;
	}

	return 0;

OnErrorExitHelp:
	subargv[1] = "--help";
	subargv[2] = NULL;
    execv(REDMICRODNF_CMD_PATH, (char**) subargv);
OnErrorExit:
	return error;
}