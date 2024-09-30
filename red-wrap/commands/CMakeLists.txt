###########################################################################
# Copyright 2022 IoT.bzh
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

add_executable(
    redwrap
        redwrap.c
)
target_link_libraries(
    redwrap
        PRIVATE
            libredwrap
)

add_executable(
    redwrap-dnf
        redwrap-dnf.c
)
target_compile_definitions(
    redwrap-dnf
        PRIVATE REDMICRODNF_BIN_DIR="${CMAKE_INSTALL_FULL_BINDIR}"
)
target_link_libraries(
    redwrap-dnf
        PRIVATE
            libredwrap
)

install(
    TARGETS redwrap redwrap-dnf
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
