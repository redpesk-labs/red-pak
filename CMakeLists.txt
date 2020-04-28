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

PROJECT (redpak-dnf-plugin C CXX)
set(PROJECT_VERSION "0.1")
set(PROJECT_PRETTY_NAME "redpak Lightweigh Containers for embedded critical systems")
set(PROJECT_DESCRIPTION "Allow to split rootfs in a hierarchie of dependencies")
set(PROJECT_URL "https://github.com/iotbzh/redpak-dnf")
set(PROJECT_AUTHOR "Fulup, Ar Foll")
set(PROJECT_AUTHOR_MAIL "fulup@iot.bzh")
set(PROJECT_LICENSE "Apache-V2")
set(PROJECT_ALIAS "redpak")

# Default version requirements
# -------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
set (PYTHON_VERSION 3)
set (INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib64)
set (INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)

# External Packages requirements
# ------------------------------
set (PKG_REQUIRED_LIST
	python${PYTHON_VERSION}
	libdnf
	rpm
	yaml-0.1
	libcyaml
  uuid
)

include(${CMAKE_CURRENT_SOURCE_DIR}/etc/cmake/redpak.cmake)

ADD_SUBDIRECTORY (etc)
ADD_SUBDIRECTORY (red-conf)
ADD_SUBDIRECTORY (red-dnf)
ADD_SUBDIRECTORY (red-rpm)
ADD_SUBDIRECTORY (red-wrap)
