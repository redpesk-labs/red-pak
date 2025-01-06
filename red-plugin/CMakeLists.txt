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


message(STATUS "Creating rpm plugin for redpak")

pkg_search_module(RPM REQUIRED redrpm rpm)

add_library(
    redpak-rpm-plugin
        SHARED
            rpm-redpak.c
)
set_target_properties(
    redpak-rpm-plugin
        PROPERTIES
            PREFIX ""
            OUTPUT_NAME "redpak"
)

target_include_directories(
    redpak-rpm-plugin
        PUBLIC
            ${RPM_INCLUDE_DIRS}
)

target_link_libraries(
    redpak-rpm-plugin
        PRIVATE
            ${RPM_LIBRARIES}
)

install(
    TARGETS redpak-rpm-plugin
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/rpm-plugins
)
