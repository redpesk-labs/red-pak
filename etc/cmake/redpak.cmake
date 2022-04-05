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

# default Module requirements
# ------------------------------
include (FindPkgConfig)
include (CMakeCXXInformation)

# few usefull macro from AGL cmake template
# -----------------------------------------
foreach (PKG_CONFIG ${PKG_REQUIRED_LIST})
        string(REGEX REPLACE "[<>]?=.*$" "" XPREFIX ${PKG_CONFIG})
        PKG_CHECK_MODULES(${XPREFIX} REQUIRED ${PKG_CONFIG})

        INCLUDE_DIRECTORIES(${${XPREFIX}_INCLUDE_DIRS})
        add_link_options(${${XPREFIX}_LDFLAGS} ${${XPREFIX}_LDFLAGS})
        add_compile_options (${${XPREFIX}_CFLAGS})
endforeach(PKG_CONFIG)

# provide Installation directory for default path generation
add_compile_options (-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -Dredpak_MAIN="${CMAKE_INSTALL_FULL_SYSCONFDIR}/redpak/main.yaml")

macro(PROJECT_TARGET_ADD TARGET_NAME)
    set_property(GLOBAL APPEND PROPERTY PROJECT_TARGETS ${TARGET_NAME})
    set(TARGET_NAME ${TARGET_NAME})
endmacro(PROJECT_TARGET_ADD)

# Pass variables from CMAKE to C-preprocessor
add_compile_options (-DPROJECT_ALIAS=${PROJECT_ALIAS} -DPROJECT_VERSION=${PROJECT_VERSION})

add_compile_options("-Wall" "-Werror=format-security" "-fexceptions" "-fstack-protector-strong" "-grecord-gcc-switches" "-fasynchronous-unwind-tables" "-fstack-clash-protection" )
