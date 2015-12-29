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
# use $(TMP_DIR)/dial to put object/archive files, and $(DEST_DIR)/dial to put all xsb files
# assume CC, AR, C_OPTIONS, XSC_OPTIONS are exported

# uses the following variables
# APPNAME
# APP_JS_SRCS
# BLL_JS_SRCS

ifeq ($(TARGET_SYSTEM), mac)
	APP_BLL = simulator
else
	APP_BLL = device
endif

APP_BINARIES = $(addprefix $(DEST_DIR)/dial/$(APPNAME)/, $(addsuffix .xsb, $(patsubst %,%,$(basename $(APP_JS_SRCS)))))
BLL_BINARIES = $(addprefix $(DEST_DIR)/dial/$(APPNAME)/, $(addsuffix .xsb, $(patsubst %,%,$(basename $(BLL_JS_SRCS)))))
APP_DIR = $(F_HOME)/xs6/sources/mc/extensions/dial/$(APPNAME)

.PHONY: archive

archive: all
	echo $(APP_BINARIES) $(BLL_BINARIES)

all: $(TMP_DIR)/dial/$(APPNAME) $(DEST_DIR)/dial/$(APPNAME) $(APP_BINARIES) $(BLL_BINARIES)

$(TMP_DIR)/dial/$(APPNAME):
	mkdir -p $(TMP_DIR)/dial/$(APPNAME)
$(DEST_DIR)/dial/$(APPNAME):
	mkdir -p $(DEST_DIR)/dial/$(APPNAME)

# rules
$(DEST_DIR)/dial/$(APPNAME)/%.xsb: $(TMP_DIR)/dial/$(APPNAME)/%.xsb
	cp -p $< $@
$(TMP_DIR)/dial/$(APPNAME)/%.xsb: $(APP_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/dial/$(APPNAME) $<
$(TMP_DIR)/dial/$(APPNAME)/%.xsb: $(APP_DIR)/$(APP_BLL)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/dial/$(APPNAME) $<

clean:
	rm -f $(APP_BINARIES) $(BLL_BINARIES)
