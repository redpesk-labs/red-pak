/*
* Copyright (C) 2020-2025 IoT.bzh Company
* Author: Fulup Ar Foll <fulup@iot.bzh>
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

#ifndef _RED_DEFAULTS_INCLUDE_
#define _RED_DEFAULTS_INCLUDE_

#include <stddef.h>

// set some reasonnable defaults
#ifndef RED_MAXPATHLEN
#define RED_MAXPATHLEN 512
#endif

#ifndef RED_CONFIG_WARNING_DEFAULT
#define RED_CONFIG_WARNING_DEFAULT 1
#endif

#ifndef NODE_PREFIX
#define NODE_PREFIX ""
#endif

#ifndef redpak_MAIN
#define redpak_MAIN "/etc/redpak/main.yaml"
#endif

#ifndef redpak_MAIN_ADMIN
#define redpak_MAIN_ADMIN "/etc/redpak/main-admin.yaml"
#endif

#ifndef redpak_TMPL
#define redpak_TMPL "/etc/redpak/templates.d"
#endif

// Following file/dir are automatically prefixed by $REDPATH at runtime
#ifndef REDNODE_CONF
#define REDNODE_CONF "etc/redpak.yaml"
#endif

#ifndef REDNODE_ADMIN
#define REDNODE_ADMIN "etc/redpak-admin.yaml"
#endif

#ifndef LEGACY_REDNODE_CONF
#define LEGACY_REDNODE_CONF "etc/redpack.yaml"
#endif

#ifndef LEGACY_REDNODE_ADMIN
#define LEGACY_REDNODE_ADMIN "etc/redpack-admin.yaml"
#endif

#ifndef REDNODE_STATUS
#define REDNODE_STATUS ".rednode.yaml"
#endif

#ifndef REDNODE_VARDIR
#define REDNODE_VARDIR "var/rpm/lib"
#endif

#ifndef REDNODE_REPODIR
#define REDNODE_REPODIR "etc/yum.repos.d"
#endif

#ifndef REDNODE_LOCK
#define REDNODE_LOCK ".rpm.lock"
#endif

#ifndef REDSTATUS_INFO
#define REDSTATUS_INFO "Last update by $LOGNAME($HOSTNAME) the $TODAY"
#endif

#ifndef REDPESK_DFLT_VERSION
#define REDPESK_DFLT_VERSION "agl-redpesk9"
#endif

#ifndef CGROUPS_MOUNT_POINT
#define CGROUPS_MOUNT_POINT "/sys/fs/cgroup"
#endif

#endif