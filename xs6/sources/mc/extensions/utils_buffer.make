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
MODULE = buffer
SUB_DIR = utils/
JS_SRC = $(F_HOME)/kinoma/kpr/libraries/Compat/src/utils/buffer.js
TMP_XSB = $(TMP_DIR)/$(MODULE).xsb
MODULE_XSB = $(DEST_DIR)/$(SUB_DIR)$(MODULE).xsb

.PHONY: archive

all: $(MODULE_XSB)

archive: $(MODULE_XSB)

$(MODULE_XSB): $(DEST_DIR)/$(SUB_DIR) $(TMP_XSB)
	cp -p $(TMP_XSB) $@

$(DEST_DIR)/$(SUB_DIR):
	mkdir -p $@

$(TMP_XSB): $(JS_SRC)
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR) $(JS_SRC)
	touch $(TMP_DIR)/.update

clean:
	rm -f $(TMP_XSB) $(MODULE_XSB)
