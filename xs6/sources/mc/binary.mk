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
XSB = $(addsuffix .xsb, $(basename $(JS_SRC) $(XS_SRC)))
ifneq (x$(XS_SRC), x)
JS = $(TMP_DIR)/$(basename $(XS_SRC)).js
endif

all archive: $(DEST_DIR)/$(SUBDIR)$(MODULE).xsb

$(DEST_DIR)/$(SUBDIR)$(MODULE).xsb: $(TMP_DIR)/$(XSB)
	cp -pf $(TMP_DIR)/$(XSB) $@

ifneq (x$(JS_SRC), x)
$(TMP_DIR)/$(XSB): $(MOD_HOME)$(JS_SRC)
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -o $(TMP_DIR) $(MOD_HOME)$(JS_SRC)
	touch $(TMP_DIR)/.update
endif
ifneq (x$(XS_SRC), x)
$(TMP_DIR)/$(XSB): $(XS_SRC)
	$(XS6_TOOL_DIR)/xsr6 -a $(XS6_TOOL_DIR)/modules/tools.xsa xs2js $(XS_SRC) -m $(MODULE) -o $(TMP_DIR)
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -o $(TMP_DIR) $(JS)
	touch $(TMP_DIR)/.update
endif

clean:
	rm -f $(TMP_DIR)/$(XSB) $(JS)
