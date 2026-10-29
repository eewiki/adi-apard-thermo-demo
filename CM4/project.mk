###############################################################################
#
# Copyright (C) 2026 Analog Devices, Inc.
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
#
##############################################################################

# This file can be used to set build configuration
# variables.  These variables are defined in a file called
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system

# **********************************************************

# Add your config here!

# To load an application on the RISC-V core, uncomment the following
# lines, and set the RISCV_APP variable to point to the RISC-V application.
# This is the folder name used for the RISC-V application in your workspace.
#RISCV_LOAD=1
#RISCV_APP=../RV

# Add library dependencies
include config.mk

# Touch project.mk if config.mk changes, so that we rebuild all sources and libraries
CONFIGMK ?= $(abspath ./config.mk)
$(PROJECTMK): $(CONFIGMK)
	@touch $(PROJECTMK)

PROJ_CFLAGS += -fdump-rtl-expand
PROJ_CFLAGS += -fdump-rtl-dfinish
PROJ_CFLAGS += -fdump-ipa-cgraph
PROJ_CFLAGS += -fstack-usage
PROJ_CFLAGS += -gdwarf-4

PROJ_CFLAGS += -DNO_OS_LWIP_NETWORKING=1
PROJ_CFLAGS += -DDISABLE_SECURE_SOCKET=1

PROJ_CFLAGS += -DCONFIG_NO_OS_STATIC_IP=1
PROJ_CFLAGS += -DCONFIG_NO_OS_IP='"192.168.97.40"'
PROJ_CFLAGS += -DCONFIG_NO_OS_NETMASK='"255.255.0.0"'
PROJ_CFLAGS += -DCONFIG_NO_OS_GATEWAY='"0.0.0.0"'
PROJ_CFLAGS += -DCONFIG_NO_OS_DOMAIN_NAME='"analog"'

# Use the generated linker file from the project
LINKERFILE = memory.ld
