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

PROJECT_TARGET_ADD(redconf)

  file(GLOB CSOURCES [a-z]*.c )

  add_library(${TARGET_NAME} SHARED ${CSOURCES} ${TARGET_NAME}.h)

  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX "lib"
		LABELS "LIBRARY"
		OUTPUT_NAME ${TARGET_NAME}
    LINKER_LANGUAGE C
    PUBLIC_HEADER ${TARGET_NAME}.h
	)

  # compliation include path for library consuming modules
	target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

  # final install destination for sharelib
  install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR)


