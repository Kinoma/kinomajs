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

MC_WMSDK_SRCS = mc_wmsdk.c mc_wifi.c
ifeq ($(TARGET_SYSTEM), mc)
	MC_WMSDK_SRCS += mw300_rd.c
endif
ifeq ($(FTFS), 1)
	MC_WMSDK_SRCS += mc_ftfs.c
endif
MC_WMSDK_OBJS = $(addprefix $(TMP_DIR)/, $(addsuffix .o, $(basename $(MC_WMSDK_SRCS))))
MC_WMSDK_LIB = $(TMP_DIR)/libmc_wmsdk.a

ifeq ($(TARGET_SYSTEM), mc)
include $(SDK_PATH)/.config
# reset C_OPTIONS
C_OPTIONS =
export SDK_INC_PATH = $(SDK_PATH)/incl
export SDK_EXTERNAL_PATH = $(SDK_INC_PATH)
export FREERTOS_INC_PATH = $(SDK_INC_PATH)/freertos
export LWIP_INC_PATH = $(SDK_INC_PATH)/lwip
include common.mk
C_OPTIONS += \
	  -I $(SDK_PATH)/incl/platform/net/lwip \
	  -I $(SDK_PATH)/incl/sdk/drivers/wlan \
	  -I $(SDK_PATH)/incl/usb
endif

all archive: $(MC_WMSDK_LIB)

$(MC_WMSDK_LIB): $(MC_WMSDK_OBJS)
	$(AR) cr $@ $(MC_WMSDK_OBJS)

$(TMP_DIR)/%.o : %.c
	$(CC) -c $(C_OPTIONS) $< -o $@

clean:
	rm -f $(MC_WMSDK_OBJS)
