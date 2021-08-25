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
#include "../redwrap-main.h"

int main (int argc, char *argv[]) {
	int argcount = 0;
    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv);
    if (!cliarg) exit(0);

	//admin is needed for redmicrodnf
	if(!cliarg->adminpath)
		cliarg->adminpath = redpak_MAIN_ADMIN;

    const char *subargv[MAX_BWRAP_ARGS];
	//add redmicrodnf command
	subargv[argcount++] = "redmicrodnf";
	subargv[argcount++] = "--redpath";
	subargv[argcount++] = cliarg->redpath;
	for (int i = cliarg->index; i < argc; i++)
		subargv[argcount++] = argv[i];

	redwrapMain(argv[0], cliarg, argcount, (const char **)&subargv);
}