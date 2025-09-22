###########################################################################
# Copyright (C) 2020-2025 IoT.bzh Company
#
# Author: Fulup Ar Foll <fulup@iot.bzh>
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

pkg_check_modules(libredconfreqs REQUIRED yaml-0.1 libcyaml uuid)

set(
    SOURCES
        redconf-dump.c
        redconf-expand.c
        rednode-factory.c
        redconf-log.c
        redconf-merge.c
        redconf-mix.c
        redconf-node.c
        redconf-schema.c
        redconf-sharing.c
        redconf-utils.c
        redconf-valid.c
)

set(
    HEADERS
        redconf.h
        redconf-defaults.h
        redconf-dump.h
        redconf-expand.h
        rednode-factory.h
        redconf-log.h
        redconf-merge.h
        redconf-mix.h
        redconf-node.h
        redconf-schema.h
        redconf-sharing.h
        redconf-utils.h
        redconf-valid.h
)

add_library(
    libredconf
        SHARED
            ${SOURCES}
)

set_target_properties(
    libredconf
        PROPERTIES
            PREFIX ""
            PUBLIC_HEADER "${HEADERS}"
)

# compliation include path for library consuming modules
target_compile_options(${${XPREFIX}_CFLAGS}
    libredconf
        PRIVATE
            ${libredconfreqs_CFLAGS}
)

target_include_directories(
    libredconf
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${libredconfreqs_INCLUDE_DIRS}
)

target_link_options(
    libredconf
        PRIVATE
            ${libredconfreqs_LDFLAGS}
)

target_link_directories(
    libredconf
        PRIVATE
            ${libredconfreqs_LIBRARY_DIRS}
)

target_link_libraries(
    libredconf
        PRIVATE
            ${libredconfreqs_LIBRARIES}
)

# final install destination for sharelib
install(
    TARGETS libredconf
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)

add_subdirectory(commands)
