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
*/

void try_become_root(void) {
    static int unshared = 0;
    uid_t uid = getuid();
    gid_t gid = getgid();
    if (!unshared && unshare(CLONE_NEWUSER | CLONE_NEWNS) == 0) {
        deny_setgroups();
        setup_map("/proc/self/uid_map", 0, uid);
        setup_map("/proc/self/gid_map", 0, gid);
        unshared = 1;
    }
    rpmlog(RPMLOG_DEBUG, "user ns: %d original user %d:%d current %d:%d\n",
	    unshared, uid, gid, getuid(), getgid());
}