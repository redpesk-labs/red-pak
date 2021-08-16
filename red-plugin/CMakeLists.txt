###########################################################################
# Copyright 2021 IoT.bzh
#
# author: Clément Bénier <clement.benier@iot.bzh>
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
PROJECT_TARGET_ADD(red-plugin)
	MESSAGE(STATUS "Creating rpm plugin for redpak")

	pkg_search_module(RPM REQUIRED redrpm rpm)

	add_library(${TARGET_NAME} SHARED rpm-redpak.c)
	set_target_properties(${TARGET_NAME} PROPERTIES
		PREFIX ""
		LABELS "LIBRARY"
		OUTPUT_NAME "redpak"
		)

	target_include_directories(${TARGET_NAME} PUBLIC ${RPM_INCLUDE_DIRS})
	target_link_libraries(${TARGET_NAME} LINK_PUBLIC ${RPM_LIBRARIES})

	install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/rpm-plugins)
