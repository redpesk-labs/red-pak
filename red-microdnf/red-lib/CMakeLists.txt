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

configure_file(reddefaults.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/reddefaults.hpp)

pkg_search_module(LIBDNF REQUIRES libdnf5 libdnf)
pkg_search_module(LIBDNFCLI REQUIRES libdnf5-cli libdnf-cli)

PROJECT_TARGET_ADD(red-lib)

    file(GLOB CXXSOURCES [a-z]*.cpp)

    add_library(${TARGET_NAME} SHARED ${CXXSOURCES})

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		LABELS "LIBRARY"
		OUTPUT_NAME redlib
        LINKER_LANGUAGE CXX
    PUBLIC_HEADER redconfig.hpp
    )

	target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../red-conf ${CMAKE_CURRENT_SOURCE_DIR}/../microdnf ${LIBDNF_INCLUDE_DIRS} ${LIBDNFCLI_INCLUDE_DIRS} ${RPM_INCLUDE_DIRS})
	target_link_libraries(${TARGET_NAME} redconf ${LIBDNF_LIBRARIES} ${LIBDNFCLI_LIBRARIES} ${RPM_LIBRARIES})

	install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
