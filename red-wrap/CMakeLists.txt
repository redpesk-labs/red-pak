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

PROJECT_TARGET_ADD(red-wrap)

    file(GLOB CSOURCES [a-z]*.c)

    add_executable (${TARGET_NAME} ${CSOURCES})

    # compile + link depencencies
	pkg_search_module(RPM REQUIRES redrpm rpm)
	target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../red-conf ${RPM_INCLUDEDIR})
    target_link_libraries(${TARGET_NAME} redconf ${RPM_LIBRARIES})
	install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})

