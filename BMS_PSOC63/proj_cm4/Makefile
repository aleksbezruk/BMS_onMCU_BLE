################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configurating the Cortex-M4 core project
#
################################################################################
# \copyright
# Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
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
################################################################################

include ../common.mk

################################################################################
# Basic Configuration
################################################################################

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=proj_cm4

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=FREERTOS WICED_BLE CM0P_BLESS

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=CM0P_SLEEP CM0P_SECURE CM0P_CRYPTO

CORE=CM4
CORE_NAME=CM4_0

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.

$(info Set BUILD CONFIGURATION IS -> $(BUILD_CONFIG)) 
ifeq ($(BUILD_CONFIG), Debug)
SOURCES=    ../../source/main/main.c	\
			../BSP/BSP.c \
			../../source/qspy/qs_64bit.c \
			../../source/qspy/qs_fp.c \
			../../source/qspy/qs_rx.c \
			../../source/qspy/qs.c \
			../../source/qspy/qstamp.c \
			../../source/qspyHelper/qspyHelper.c \
			../../source/ADC/ADC.c	\
			../../source/BLE/BLE.c	\
			../../source/LP/LP.c	\
			../../source/BLE/BatteryService.c	\
			../../source/BLE/AIOService.c	\
			../../source/HAL/PSOC63/hal/hal.c
else ifeq ($(BUILD_CONFIG), Release)
SOURCES=
else ifeq ($(BUILD_CONFIG), Qutest)
SOURCES=    ../../source/qspy/qutest.c \
		    ../../source/qutestHelper/qutestHelper.c	\
			../../source/main/main.c	\
		    ../BSP/BSP.c \
			../BSP/test_BSP.c \
			../../source/qspy/qs_64bit.c \
			../../source/qspy/qs_fp.c \
			../../source/qspy/qs_rx.c \
			../../source/qspy/qs.c \
			../../source/qspy/qstamp.c \
			../../source/qspyHelper/qspyHelper.c \
			../../source/ADC/ADC.c	\
			../../source/BLE/BLE.c	\
			../../source/LP/LP.c	\
			../../source/BLE/BatteryService.c	\
			../../source/BLE/AIOService.c	\
			../../source/HAL/PSOC63/hal/hal.c
else
# $(error !!! BUILD CONFIGURATION IS udefined !!!. Please set Debug or Qutest or Release)
$(info Set BUILD CONFIGURATION IS nor defined. Please doublecheck. Maybe it's Cypress Device Confidurator running) 
endif	# BUILD configuration

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=SOURCES= ../BSP/ \
				  ../../source/qspy/ \
				  ../../source/qspyHelper \
				  ../../source/qutestHelper \
				  ../../source/ADC	\
				  ../../source/BLE	\
				  ../../source/main \
				  ../../source/LP	\
				  ../../source/HAL/PSOC63/hal

# Add additional defines to the build process (without a leading -D).
ifeq ($(BUILD_CONFIG), Debug)
DEFINES=Q_SPY CY_RTOS_AWARE CYBSP_BT_PLATFORM_CFG_SLEEP_MODE_LP_ENABLED=0
else ifeq ($(BUILD_CONFIG), Release)
DEFINES=Q_SPY CY_RTOS_AWARE CYBSP_BT_PLATFORM_CFG_SLEEP_MODE_LP_ENABLED=0
else ifeq ($(BUILD_CONFIG), Qutest)
DEFINES=Q_SPY Q_UTEST=0 CY_RTOS_AWARE CYBSP_BT_PLATFORM_CFG_SLEEP_MODE_LP_ENABLED=0
else
#$(error !!! BUILD CONFIGURATION IS udefined !!!. Please set Debug or Qutest or Release)
endif	# BUILD configuration

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ifeq ($(BUILD_CONFIG), Qutest)
CFLAGS= -O0 -Wall -g3 -fprofile-arcs -ftest-coverage  -fprofile-filter-files="main.c;qspyHelper.c;../source/BSP/BSP.c"
else
CFLAGS= -O0 -Wall -g3
endif	# BUILD configuration

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
ifeq ($(BUILD_CONFIG), Qutest)
LDFLAGS=-fprofile-arcs -lc -lgcov -lrdimon -specs=rdimon.specs
else
LDFLAGS=
endif	# BUILD configuration

# Additional / custom libraries to link in to the application.
ifeq ($(BUILD_CONFIG), Qutest)
LDLIBS=
else
LDLIBS=
endif	# BUILD configuration

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field 
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by 
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level 
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
