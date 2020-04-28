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

PROJECT_TARGET_ADD(reddnf-py)

    file(GLOB CXXSOURCES [a-z]*.cpp)

    add_compile_options ("-DPYTHON_MODULE_ENTRY=PyInit__${PYTHON_MODULE_NAME}(void)")
    add_library(${TARGET_NAME} MODULE ${CXXSOURCES})

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX "_"
		LABELS "LIBRARY"
		OUTPUT_NAME reddnf
        LINKER_LANGUAGE CXX
    )

    target_link_directories(${TARGET_NAME}
     PUBLIC ${PYTHON_SITELIB_DIR}/hawkey
     PUBLIC ${PYTHON_INSTALL_DIR}/hawkey
    )

    target_link_libraries(${TARGET_NAME} :_hawkey.so)

    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    install(TARGETS ${TARGET_NAME} DESTINATION ${PYTHON_MODULE_DIR})



