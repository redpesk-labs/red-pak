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

CMAKE_MINIMUM_REQUIRED(VERSION 3.13)

PROJECT(redpak
    VERSION 2.3.2
    DESCRIPTION "redpak is a lightweight container solution for embedded critical systems"
    HOMEPAGE_URL "https://github.com/redpesk-labs/red-pak"
    LANGUAGES C CXX)

# possible options
# ----------------
option(WITHOUT_TESTS    "Avoid compiling tests"     OFF)

set(REDMICRODNF         "/usr/bin/redmicrodnf"      CACHE STRING "redmicrodnf path")
set(PYTHON_VERSION       3)

# commons
# -------
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
#include(FindPkgConfig)
#include(CMakeCXXInformation)

add_compile_options(
    "-Wall"
    "-Wextra"
    "-Werror=format-security"
    "-fexceptions"
    "-fstack-protector-strong"
    "-grecord-gcc-switches"
    "-fasynchronous-unwind-tables"
    "-fstack-clash-protection"
)

add_compile_definitions(
    redpak_MAIN="${CMAKE_INSTALL_FULL_SYSCONFDIR}/redpak/main.yaml"
    REDPACK_VERSION="${PROJECT_VERSION}"
)


# add sub directories
add_subdirectory(docs)
add_subdirectory(etc)
add_subdirectory(red-conf)
add_subdirectory(red-wrap)
add_subdirectory(red-lib)
add_subdirectory(red-plugin)
add_subdirectory(python)
if(NOT WITHOUT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# add pkgconfig file
configure_file("red-pak.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/red-pak.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/red-pak.pc DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig)
