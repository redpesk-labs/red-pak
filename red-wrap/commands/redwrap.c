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

static const char * usage = "usage: redwrap --redpath=... [--verbose] [--force] [--admin[=.../main-admin.yaml]] [--rmain=.../main.yaml] -- program args\n";

int main (int argc, char *argv[]) {

    rWrapConfigT *cliarg = RwrapParseArgs (argc, argv, usage);
    if (!cliarg) exit(0);

	redwrapMain(argv[0], cliarg, argc-cliarg->index, (const char **)argv+cliarg->index);
}