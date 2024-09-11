###########################################################################
# Copyright 2022 IoT.bzh
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

add_executable(
    redconf
        cmd-config.c
        cmd-mergeconfig.c
        cmd-tree.c
        options.c
        redconf.c
)

target_include_directories(
    redconf
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    redconf
        PRIVATE
            libredconf
)

install(
    TARGETS redconf
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)