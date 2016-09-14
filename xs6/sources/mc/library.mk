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

.SUFFIXES : .c .o .so .a
$(TMP_DIR)/%.o : $(MOD_HOME)%.c
	$(CC) -c $(C_OPTIONS) $(MOD_C_OPTIONS) $< -o $@
	$(AR) cr $(TMP_DIR)/$(LIBMODULE) $@

MOD_NAME = $(basename $(notdir $(JS_SRC)))
OBJS = $(addprefix $(TMP_DIR)/, $(addsuffix .o, $(basename $(C_SRCS))))
ENTRY = _mc_$(MODULE)_module
XSB = $(addprefix $(TMP_DIR)/, $(addsuffix .xsb, $(MOD_NAME)))

ifneq ($(XS_ARCHIVE), 1)
XS_C = $(addprefix $(TMP_DIR)/, $(addsuffix .xs.c, $(MOD_NAME)))
XS_H = $(addprefix $(TMP_DIR)/, $(addsuffix .xs.h, $(MOD_NAME)))
endif

ifndef USE_DEFAULT_LIBS
MOD_LINK_OPTIONS += -nodefaultlibs -nostartfiles
endif

.PHONY: archive

all: $(DEST_DIR)/$(SUB_DIR)$(MODULE).xsb $(DEST_DIR)/$(SUB_DIR)$(MODULE).so $(TMP_DIR)/$(LIBMODULE)

archive: $(DEST_DIR)/$(SUB_DIR)$(MODULE).xsb $(TMP_DIR)/$(LIBMODULE)

$(DEST_DIR)/$(SUB_DIR)$(MODULE).xsb: $(XSB)
	cp -p $(XSB) $@

$(XSB): $(MOD_HOME)$(JS_SRC)
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR) $(MOD_HOME)$(JS_SRC)
	touch $(TMP_DIR)/.update

$(DEST_DIR)/$(SUB_DIR)$(MODULE).so: $(OBJS)
	$(CC) $(C_OPTIONS) $(MOD_C_OPTIONS) $(MOD_LINK_OPTIONS) -o $@ $(OBJS) $(EXTRA_LDFLAGS) $(EXTRA_LIBS)
ifeq ($(TARGET_SYSTEM), mc)
	$(STRIP) $@
endif

$(TMP_DIR)/$(LIBMODULE): $(OBJS)

$(OBJS): $(XS_C) $(XS6_MC_DIR)/mc_module.h $(XS6_MC_DIR)/mc_xs.h $(XS6_MC_DIR)/xs_patch.h

clean:
	rm -f $(OBJS) $(addsuffix .d, $(basename $(OBJS))) $(XSB) $(XS_C) $(XS_H)
