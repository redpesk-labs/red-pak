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

pkg_search_module(RPM REQUIRED redrpm rpm)

add_library(
    libredwrap
        SHARED
            cgroups.c
            redwrap-conf.c
            redwrap-exec.c
            redwrap-node.c
)

# compile + link depencencies
set_target_properties(
    libredwrap
        PROPERTIES
            PREFIX ""
            OUTPUT_NAME libredwrap
)

target_include_directories(
    libredwrap
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/red-conf
)

target_link_libraries(
    libredwrap
        PRIVATE
            libredconf
)

install(
    TARGETS libredwrap
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

install(
    FILES 
            redwrap-conf.h
            redwrap-exec.h
            redwrap-node.h
        DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)

add_subdirectory(commands)
