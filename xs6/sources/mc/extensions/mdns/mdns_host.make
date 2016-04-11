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
MDNS_JS_SRCS = mdns_resolver.js mdns_cache.js mdns_message.js
MDNS_C_SRCS = mdns_mac.c

MDNS_BINARIES = $(addprefix $(DEST_DIR)/mdns/, $(addsuffix .xsb, $(patsubst mdns_%,%,$(basename $(MDNS_JS_SRCS)))))
MDNS_OBJS = $(addprefix $(TMP_DIR)/mdns/, $(addsuffix .o, $(basename $(MDNS_C_SRCS))))

ifndef XS_ARCHIVE
MDNS_OBJS += $(addprefix $(TMP_DIR)/mdns/, $(addsuffix .xs.o, $(basename $(MDNS_C_SRCS))))
endif

ifndef MDNS_DIR
MDNS_DIR = $(F_HOME)/xs6/sources/mc/extensions/mdns
endif

.PHONY: archive

all archive: $(TMP_DIR)/mdns $(DEST_DIR)/mdns $(DEST_DIR)/mdns.xsb $(MDNS_BINARIES) $(MDNS_OBJS)
	$(AR) cr $(TMP_DIR)/$(LIBMODULE) $(MDNS_OBJS)

$(TMP_DIR)/mdns:
	mkdir -p $(TMP_DIR)/mdns
$(DEST_DIR)/mdns:
	mkdir -p $(DEST_DIR)/mdns

# rules
$(DEST_DIR)/mdns.xsb: $(TMP_DIR)/mdns_host.xsb
	cp -p $< $@
$(DEST_DIR)/mdns/%.xsb: $(TMP_DIR)/mdns/mdns_%.xsb
	cp -p $< $@
$(TMP_DIR)/%.xsb: $(MDNS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR) $<
$(TMP_DIR)/mdns/%.xsb: $(MDNS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/mdns $<
$(TMP_DIR)/mdns/%.o: $(MDNS_DIR)/%.c
	$(CC) -c $(C_OPTIONS) $< -o $@

clean:
	rm -f $(MDNS_OBJS) $(addsuffix .d, $(basename $(MDNS_OBJS))) $(TMP_DIR)/mdns_host.xsb
