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

PROJECT_TARGET_ADD(redrpm-py)

    set (RPMLIB_PY_DIR rpmlib-ori)

    # include C files from both decendant directory inside a unique shared library
    file(GLOB CSOURCES [a-z]*.c ${RPMLIB_PY_DIR}/[a-z]*.c)

    add_compile_options ("-DPYTHON_MODULE_ENTRY=PyInit__${PYTHON_MODULE_NAME}(void)")

    # rpmsign mightt not be defined within librpm.pc !
    add_link_options(-lrpmsign)

    add_library(${TARGET_NAME} MODULE ${CSOURCES})

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX "_"
		LABELS "LIBRARY"
		OUTPUT_NAME ${PYTHON_MODULE_NAME}
        LINKER_LANGUAGE C
	)

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    install(TARGETS ${TARGET_NAME} DESTINATION ${PYTHON_MODULE_DIR})

    # external lib dependencies
    target_link_libraries(${TARGET_NAME} redrpm)

