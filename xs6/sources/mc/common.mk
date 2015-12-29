#
#     Copyright (C) 2010-2015 Marvell International Ltd.
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
C_OPTIONS += -mthumb -ffunction-sections -ffreestanding -MMD -Wall \
	-fno-strict-aliasing -Wno-unknown-pragmas -DUSE_GCC \
	-fdata-sections
AS_OPTIONS += -mthumb
ifeq (y, $(CONFIG_CPU_MW300))
C_OPTIONS += -mcpu=cortex-m4
AS_OPTIONS += -mcpu=cortex-m4
else
C_OPTIONS += -mcpu=cortex-m3
AS_OPTIONS += -mcpu=cortex-m3
endif
C_OPTIONS += -DmxMC=1 -DWMSDK_VERSION=$(WMSDK_VERSION)

C_OPTIONS += -include $(SDK_PATH)/incl/autoconf.h \
	  -I $(SDK_PATH)/incl/libc \
	  -I $(SDK_PATH)/incl/platform/os/freertos \
	  -I $(SDK_PATH)/incl/platform/net/lwip \
	  -I $(SDK_PATH)/incl/sdk \
	  -I $(SDK_PATH)/incl/sdk/drivers \
	  -I $(SDK_PATH)/incl \
	  -I $(SDK_PATH)/incl/platform/arch \
	  -I $(SDK_PATH)/incl/sdk/drivers/wlan \
	  -I $(SDK_PATH)/incl/freertos \
	  -I $(SDK_PATH)/incl/lwip \
	  -I $(SDK_PATH)/incl/lwip/ipv4 \
	  -I $(SDK_PATH)/incl/lwip/ipv6 \
	  -I $(SDK_PATH)/incl/lwip/arch \
	  -I $(SDK_PATH)/incl/usb

ifeq (y, $(CONFIG_CPU_MW300))
C_OPTIONS += \
	  -I $(SDK_PATH)/incl/sdk/drivers/mw300 \
	  -I $(SDK_PATH)/incl/sdk/drivers/mw300/regs
else
C_OPTIONS += \
	  -I $(SDK_PATH)/incl/sdk/drivers/mc200 \
	  -I $(SDK_PATH)/incl/sdk/drivers/mc200/regs
endif
