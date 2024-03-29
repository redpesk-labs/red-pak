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

file(GLOB redpak_CONF [a-z]*.yaml)

INSTALL (FILES ${redpak_CONF} DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/redpak/)

file(GLOB TEMPLATES_CONF templates.d/[a-z]*.yaml)
foreach(TEMP IN ITEMS ${TEMPLATES_CONF})
    get_filename_component(TEMP_REAL ${TEMP} REALPATH)
    get_filename_component(TEMP_FILENAME ${TEMP} NAME)
    INSTALL (FILES ${TEMP_REAL} DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/redpak/templates.d/ RENAME ${TEMP_FILENAME})
endforeach()
