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

C_OPTIONS = -I$(XS6_MC_DIR) -I$(XS6_MC_DIR)/wmlib -I$(XS6_INC_DIR) -I$(XS6_SRC_DIR) -I$(XS6_SRC_DIR)/pcre -I$(XS6)/extensions/crypt -I$(USB_DRIVER_PATH)/include -DmxRun=1 -DmxMC=1 -DmxNoFunctionLength -DmxNoFunctionName -DWMSDK_VERSION=$(WMSDK_VERSION) -DXIP=$(XIP) -DXS_ARCHIVE=$(XS_ARCHIVE)
AS_OPTIONS = -I $(XS6_MC_DIR)

ifdef K5
C_OPTIONS += -DK5=1
endif

ifeq ($(DEBUG),)
	C_OPTIONS += -D_RELEASE=1 -O2
else
	C_OPTIONS += -D_DEBUG=1 -DmxDebug=1 -DmxReport=0 -DmxHostReport=0 -DmxStress=1 -Os -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_OPTIONS += -DMC_MEMORY_DEBUG=1
endif
C_OPTIONS += -I$(TMP_DIR)
ifeq ($(XS_COMPILER), 1)
	C_OPTIONS += -DmxParse=1
endif
ifeq ($(FTFS), 1)
	C_OPTIONS += -DFTFS=1
endif

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

C_OPTIONS += -include $(AUTOCONF) \
	  -I $(SDK_INC_PATH)/libc \
	  -I $(SDK_INC_PATH)/platform/os/freertos \
	  -I $(SDK_INC_PATH)/sdk \
	  -I $(SDK_INC_PATH)/sdk/drivers \
	  -I $(SDK_INC_PATH) \
	  -I $(FREERTOS_INC_PATH) \
	  -I $(FREERTOS_INC_PATH)/Source/include \
	  -I $(FREERTOS_INC_PATH)/Source/portable/GCC/ARM_CM3 \
	  -I $(LWIP_INC_PATH) \
	  -I $(LWIP_INC_PATH)/ipv4 \
	  -I $(LWIP_INC_PATH)/ipv6 \
	  -I $(SDK_EXTERNAL_PATH)/lwip/contrib/port/FreeRTOS/wmsdk

ifeq (y, $(CONFIG_CPU_MW300))
C_OPTIONS += \
	  -I $(SDK_INC_PATH)/sdk/drivers/mw300 \
	  -I $(SDK_INC_PATH)/sdk/drivers/mw300/regs
else
C_OPTIONS += \
	  -I $(SDK_INC_PATH)/sdk/drivers/mc200 \
	  -I $(SDK_INC_PATH)/sdk/drivers/mc200/regs
endif

export C_OPTIONS
export AS_OPTIONS
