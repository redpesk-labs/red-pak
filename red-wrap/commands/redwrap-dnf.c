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

#include "../redwrap-main.h"

/* list of out node context commands */
static const char * outnode_commands [] = {
	"manager"
};

#define REDMICRODNF_CMD "redmicrodnf"
#define REDMICRODNF_CMD_PATH "/usr/bin/"REDMICRODNF_CMD

static const char * usage = "usage: redwrap-dnf --redpath=... [--verbose] [--force] [--admin[=.../main-admin.yaml]] [--rmain=.../main.yaml] [redmicrodnfcmd [args]]  \n\n";

int main (int argc, char *argv[]) {
	int outnode = 0; //out node context
	int argcount = 0;
    const char *subargv[MAX_BWRAP_ARGS];

	//add redmicrodnf command
	subargv[argcount++] = REDMICRODNF_CMD;

    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, usage);
    if (!cliarg) goto OnErrorExit;

	//admin is needed for redmicrodnf
	if(!cliarg->adminpath)
		cliarg->adminpath = redpak_MAIN_ADMIN;

	subargv[argcount++] = "--redpath";
	subargv[argcount++] = cliarg->redpath;


	for (int i = cliarg->index; i < argc; i++)
		subargv[argcount++] = argv[i];

	for (int i = 0; i < sizeof(outnode_commands)/sizeof(outnode_commands[0]); i++) {
		if(!strcmp(argv[cliarg->index], outnode_commands[i]))
			outnode = 1;
	}

	if (outnode)
		return execv(REDMICRODNF_CMD_PATH, (char**) subargv);
	else
		redwrapMain(argv[0], cliarg, argcount, (const char **)&subargv);

	return 0;

OnErrorExit:
	subargv[1] = "--help";
	subargv[2] = NULL;
    return execv(REDMICRODNF_CMD_PATH, (char**) subargv);
}