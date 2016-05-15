#
#     Copyright (C) 2010-2016 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
.NOTPARALLEL:

XS6 = $(F_HOME)/xs6
export XS6

export XS_ARCHIVE = 1

export PLATFORM = mc

BUILD_OS := $(shell uname)
ifeq ($(BUILD_OS), Darwin)
	HOST = mac
	SDK_PLATFORM = Darwin
else
ifeq ($(BUILD_OS), Linux)
	HOST = linux
	SDK_PLATFORM = Linux
else
	HOST = win
	SDK_PLATFORM = Windows
endif
endif
export SDK_PLATFORM
export XS6_TOOL_DIR ?= $(XS6)/bin/$(HOST)/debug
export XS6_INC_DIR = $(XS6)/includes
export XS6_SRC_DIR = $(XS6)/sources
export XS6_MC_DIR = $(XS6_SRC_DIR)/mc

debug:
	make DEBUG=1 -f xsr6.make

release:
	make -f xsr6.make

clean:
	make DEBUG=1 clean -f xsr6.make
	make clean -f xsr6.make

host:
	make DEBUG=1 -f xsr6_host.make

mc:
	make DEBUG=1 -f xsr6_host.make mc

host_clean:
	make DEBUG=1 -f xsr6_host.make clean

sign:
	make DEBUG=1 -f xsr6.make sign
	make -f xsr6_host.make sign
