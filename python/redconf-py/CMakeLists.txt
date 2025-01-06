###########################################################################
#* Copyright (C) 2022-2025 IoT.bzh Company
#* Clément Bénier <clement.benier@iot.bzh>
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

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

add_library(
    redconf-py
        SHARED
            redconf-py.c
)

set_target_properties(
    redconf-py
        PROPERTIES
            PREFIX ""
            OUTPUT_NAME redconf
)

# compliation include path for library consuming modules
target_link_libraries(
    redconf-py
        PRIVATE
            libredconf
)
target_include_directories(
    redconf-py
        PRIVATE
            ${CMAKE_SOURCE_DIR}/red-conf
            ${_Python3_INCLUDE_DIR}
)

# final install destination for sharelib
install(
    TARGETS redconf-py
        LIBRARY DESTINATION ${Python3_SITEARCH}
)
