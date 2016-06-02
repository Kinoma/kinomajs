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

SSL_JS_SRCS = \
	ssl_alert.js \
	ssl_cache.js \
	ssl_changecipher.js \
	ssl_ciphersuites.js \
	ssl_handshake.js \
	ssl_prf.js \
	ssl_protocol.js \
	ssl_record.js \
	ssl_session.js \
	ssl_setup.js \
	ssl_stream.js

SSL_BINARIES = $(addprefix $(DEST_DIR)/ssl/, $(addsuffix .xsb, $(patsubst ssl_%,%,$(basename $(SSL_JS_SRCS)))))
SSL_BINARIES += $(DEST_DIR)/ssl/cert.xsb

ifndef SSL_DIR
SSL_DIR = $(F_HOME)/xs6/extensions/ssl
endif

.PHONY: archive

all: archive

archive: $(TMP_DIR)/ssl $(DEST_DIR)/ssl $(DEST_DIR)/ssl.xsb $(SSL_BINARIES)

$(TMP_DIR)/ssl:
	mkdir -p $(TMP_DIR)/ssl
$(DEST_DIR)/ssl:
	mkdir -p $(DEST_DIR)/ssl

# rules
$(DEST_DIR)/%.xsb: $(TMP_DIR)/%.xsb
	cp -p $< $@
$(DEST_DIR)/ssl/cert.xsb: $(TMP_DIR)/ssl/ssl_cert_$(PLATFORM).xsb
	mv -f $< $@
$(DEST_DIR)/ssl/%.xsb: $(TMP_DIR)/ssl/ssl_%.xsb
	cp -p $< $@
$(TMP_DIR)/%.xsb: $(SSL_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR) $<
	touch $(TMP_DIR)/.update
$(TMP_DIR)/ssl/%.xsb: $(SSL_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/ssl $<
	touch $(TMP_DIR)/.update

clean:
