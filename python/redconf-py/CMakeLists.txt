###########################################################################
#* Copyright (C) 2022 "IoT.bzh"
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

PROJECT_TARGET_ADD(redconf-py)

  file(GLOB CSOURCES [a-z]*.c )
  file(GLOB CHEADERS [a-z]*.h )

  add_library(${TARGET_NAME} SHARED ${CSOURCES})


  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
               PREFIX ""
               LABELS "LIBRARY"
               OUTPUT_NAME redconf
    LINKER_LANGUAGE C
       )


  # compliation include path for library consuming modules
  target_link_libraries(${TARGET_NAME} libredconf)
  target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../red-conf ${_Python3_INCLUDE_DIR})

  # final install destination for sharelib
  install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${Python3_SITELIB_PREFIXED})
