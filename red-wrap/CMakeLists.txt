###########################################################################
# Copyright 2022 IoT.bzh
#
# Author: Fulup Ar Foll <fulup@iot.bzh>
# Author: Valentin Lefebvre <valentin.lefebvre@iot.bzh>
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


PROJECT_TARGET_ADD(libredwrap)

    file(GLOB CSOURCES [a-z]*.c commands/redwrap-cmd.c)
    file(GLOB CHEADERS redwrap-main.h redwrap.h commands/redwrap-cmd.h)

    message("CSOURCES: ${CSOURCES}")

    add_library(${TARGET_NAME} SHARED ${CSOURCES})

    # compile + link depencencies
	pkg_search_module(RPM REQUIRED redrpm rpm)
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX ""
		LABELS "LIBRARY"
		OUTPUT_NAME ${TARGET_NAME}
        LINKER_LANGUAGE C
	    PUBLIC_HEADER "${CHEADERS}"
	)

	target_include_directories(${TARGET_NAME} PUBLIC ${RPM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../red-conf)
    target_link_libraries(${TARGET_NAME} libredconf ${RPM_LIBRARIES})
    install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})

foreach(CMD redwrap redwrap-dnf)
    message(STATUS "CMD=${CMD}")
    PROJECT_TARGET_ADD(${CMD})
        add_executable(${CMD} commands/${CMD}.c)
	    target_include_directories(${TARGET_NAME} PUBLIC ${RPM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../red-conf)
        target_link_libraries(${TARGET_NAME} libredwrap)
	    install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
endforeach()