###########################################################################
# Copyright 2020 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

# Contains default values to push as environment variable
# Use to expand string from YAML config file
import os

RedDefaults = {
    'redpak_MAIN'    : '/opt/etc/redpak/main.yaml',
    'redpak_TMPL'    : '/opt/etc/redpak/templates.d',
    'REDNODE_CONF'    : '$NODE_PATH/etc/redpack.yaml',
    'REDNODE_STATUS'  : '$NODE_PATH/.rednode.yaml',
    'REDNODE_VARDIR'  : '$NODE_PATH/var/lib/rpm',
    'REDNODE_LOCK'    : '$NODE_PATH/.rpm.lock',
    'LOGNAME'         : 'Unknown',
    'HOSTNAME'        : 'localhost',
    'PERSISTDIR'      : '$redpak_HOME/var/lib/dnf',
    'REDNODE_REPODIR' : '$NODE_PATH/etc/yum.repos.d'
}

def values():
    return RedDefaults



