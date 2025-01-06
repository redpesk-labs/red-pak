###########################################################################
# Copyright (C) 2021-2025 IoT.bzh Company
#
# Author: Clément Bénier <clement.benier@iot.bzh>
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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pkg_search_module(LIBDNF REQUIRED libdnf5 libdnf)
pkg_search_module(LIBDNFCLI REQUIRED libdnf5-cli libdnf-cli)

set(
    SOURCES
        redconfig.cpp
        redtransaction.cpp
)

set(
    HEADERS
        redconfig.hpp
        redtransaction.hpp
)

add_library(
    redlib
        SHARED
            ${SOURCES}
)

set_target_properties(
    redlib
        PROPERTIES
            PUBLIC_HEADER "${HEADERS}"
)

target_include_directories(
    redlib
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/red-conf
            ${LIBDNF_INCLUDE_DIRS}
            ${LIBDNFCLI_INCLUDE_DIRS}
            ${RPM_INCLUDE_DIRS}
)

target_link_libraries(
    redlib
        PRIVATE
            libredconf
            ${LIBDNF_LIBRARIES}
            ${LIBDNFCLI_LIBRARIES}
            ${RPM_LIBRARIES}
)

install(
    TARGETS redlib
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
