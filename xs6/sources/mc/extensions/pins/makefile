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
.NOTPARALLEL: 

PINS_JS_SRCS = \
	pins_Analog.js \
	pins_Digital.js \
	pins_GPIO.js \
	pins_Ground.js \
	pins_I2C.js \
	pins_K5PinMux.js \
	pins_Power.js \
	pins_PowerGround.js \
	pins_Serial.js \
	pins_PWM.js \
	pins__http.js \
	pins__ws.js \
	pins_map.js

BLL_JS_SRCS = \
	bll_Analog.js \
	bll_Digital.js \
	bll_I2C.js \
	bll_Serial.js \
	bll_Ground.js \
	bll_Power.js \
	bll_PWM.js

PINS_C_SRCS = pins.c

PINS_BINARIES = $(addprefix $(DEST_DIR)/pins/, $(addsuffix .xsb, $(patsubst pins_%,%,$(basename $(PINS_JS_SRCS)))))
# BLLs go to the top dir
BLL_BINARIES = $(addprefix $(DEST_DIR)/, $(addsuffix .xsb, $(patsubst bll_%,%,$(basename $(BLL_JS_SRCS)))))
PINS_OBJS = $(addprefix $(TMP_DIR)/pins/, $(addsuffix .o, $(basename $(PINS_C_SRCS))))

PINS_DIR = $(F_HOME)/xs6/sources/mc/extensions/pins

C_OPTIONS += $(MOD_C_OPTIONS)

.PHONY: archive

all: archive

archive: $(TMP_DIR)/pins $(DEST_DIR)/pins $(DEST_DIR)/pins.xsb $(PINS_BINARIES) $(PINS_OBJS) $(BLL_BINARIES)

$(TMP_DIR)/pins:
	mkdir -p $@
$(DEST_DIR)/pins:
	mkdir -p $@

# rules
$(DEST_DIR)/%.xsb: $(TMP_DIR)/%.xsb
	cp -p $< $@
$(DEST_DIR)/pins/%.xsb: $(TMP_DIR)/pins/pins_%.xsb
	cp -p $< $@
$(DEST_DIR)/%.xsb: $(TMP_DIR)/pins/bll_%.xsb
	cp -p $< $@
$(TMP_DIR)/%.xsb: $(PINS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR) $<
$(TMP_DIR)/pins/%.xsb: $(PINS_DIR)/%.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -c -o $(TMP_DIR)/pins $<
$(TMP_DIR)/pins/%.o : $(PINS_DIR)/%.c
	$(CC) -c $(C_OPTIONS) $< -o $@

clean:
	rm -f $(PINS_OBJS) $(addsuffix .d, $(basename $(PINS_OBJS)))
