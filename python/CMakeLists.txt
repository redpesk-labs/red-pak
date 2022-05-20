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

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

set(Python3_SITELIB_PREFIXED ${Python3_SITELIB})
# prepend CMAKE_INSTALL_PREFIX if not present
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
        "import distutils.sysconfig; print(distutils.sysconfig.get_python_lib(prefix='', plat_specific=True,standard_lib=False))"
    
    OUTPUT_VARIABLE Python3_SITELIB_PREFIXED
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(PREPEND Python3_SITELIB_PREFIXED "${CMAKE_INSTALL_PREFIX}/")
message(STATUS "Python3_SITELIB prepend with CMAKE_INSTALL_PREFIX = ${Python3_SITELIB_PREFIXED}")

ADD_SUBDIRECTORY (redconf-py)
