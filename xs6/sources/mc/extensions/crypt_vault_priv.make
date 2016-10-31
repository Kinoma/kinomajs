#
#     Copyright (C) 2002-2015 Kinoma, Inc.
#
#     All rights reserved.
#
#
#
#
#
#
#
#
#
#
#
#
CRYPT_BINARIES = $(DEST_DIR)/crypt/vault.xsb

.PHONY: archive rm

all: archive

archive: rm $(CRYPT_BINARIES)

rm:
	rm -f $(CRYPT_BINARIES)

$(DEST_DIR)/crypt/%.xsb: $(TMP_DIR)/crypt/crypt_%_priv.xsb
	cp -p $< $@
$(TMP_DIR)/crypt/%.xsb: %.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/crypt $<

clean:;
