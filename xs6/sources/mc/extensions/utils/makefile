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
SUB_DIR = utils

JS_DIR = $(F_HOME)/xs6/sources/mc/extensions/$(SUB_DIR)
SOURCES = \
	xorshift.js
OBJECTS = $(addprefix $(DEST_DIR)/$(SUB_DIR)/, $(addsuffix .xsb, $(basename $(SOURCES))))

EX_JS_DIR = $(F_HOME)/kinoma/kpr/libraries/Compat/src/utils
EX_SOURCES = \
	net.js
EX_OBJECTS = $(addprefix $(DEST_DIR)/$(SUB_DIR)/, $(addsuffix .xsb, $(basename $(EX_SOURCES))))

.PHONY: archive

all archive: $(DEST_DIR)/$(SUB_DIR) $(OBJECTS) $(EX_OBJECTS)

clean:
	rm -f $(OBJECTS) $(EX_OBJECTS)

$(DEST_DIR)/$(SUB_DIR):
	mkdir -p $(DEST_DIR)/$(SUB_DIR)

# rules
$(DEST_DIR)/$(SUB_DIR)/%.xsb: $(JS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(DEST_DIR)/$(SUB_DIR) $<

$(DEST_DIR)/$(SUB_DIR)/%.xsb: $(EX_JS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(DEST_DIR)/$(SUB_DIR) $<

