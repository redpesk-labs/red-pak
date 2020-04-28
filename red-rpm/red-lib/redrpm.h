/*
* Copyright (C) 2020 "IoT.bzh"
* Author Fulup Ar Foll <fulup@iot.bzh>
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
* Documentation: https://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/ch-programming-c.html#id752589
*/

#ifndef _RED_LIB_INCLUDE_
#define _RED_LIB_INCLUDE_

#include <stdio.h>
#include <stdlib.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmlog.h>

#include "redconf.h"
#include "redlib-glue.h"

// set some reasonnable defaults
#ifndef RED_MAXPATHLEN
#define RED_MAXPATHLEN 512
#endif

#ifndef REDNODE_CONF
#define REDNODE_CONF "etc/redpack.yaml"
#endif

#ifndef REDNODE_STATUS
#define REDNODE_STATUS ".rednode.yaml"
#endif

#ifndef REDNODE_VARDIR
#define REDNODE_VARDIR "var/rpm/lib"
#endif

#ifndef REDNODE_LOCK
#define REDNODE_LOCK ".rpm.lock"
#endif

#endif // _RED_LIB_INCLUDE_
