#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
% : %.c
%.o : %.c

XSC6 = $(XS6)/bin/linux/debug/xsc6
XSL6 = $(XS6)/bin/linux/debug/xsl6

BIN_DIR_DBG = $(XS6)/bin/linux/debug/modules
BIN_DIR_RLS = $(XS6)/bin/linux/release/modules
EXPAT_DIR = $(F_HOME)/libraries/expat
INC_DIR = $(XS6)/includes
MOD_DIR = $(XS6)/tools
SRC_DIR = $(XS6)/sources
TMP_DIR_DBG = $(XS6)/tmp/linux/debug/tools
TMP_DIR_RLS = $(XS6)/tmp/linux/release/tools

C_OPTIONS = -fno-common -I$(EXPAT_DIR) -I$(INC_DIR) -I$(SRC_DIR) -I$(SRC_DIR)/tool -fPIC -DHAVE_MEMMOVE=1
ifneq ("x$(SDKROOT)", "x")
	C_OPTIONS += -isysroot $(SDKROOT)
endif
C_OPTIONS_DBG = $(C_OPTIONS) -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -I$(TMP_DIR_DBG)
C_OPTIONS_RLS = $(C_OPTIONS) -O3 -I$(TMP_DIR_RLS)

LIBRARIES = -lm -ldl

LINK_OPTIONS = -shared -Wl,-Bdynamic\,-Bsymbolic
ifneq ("x$(SDKROOT)", "x")
	LINK_OPTIONS += -isysroot $(SDKROOT)
endif
LINK_OPTIONS_DBG = $(LINK_OPTIONS)
LINK_OPTIONS_RLS = $(LINK_OPTIONS)
	
MODULES_DBG = \
	$(TMP_DIR_DBG)/fs.xsb \
	$(TMP_DIR_DBG)/grammar.xsb \
	$(TMP_DIR_DBG)/infoset.xsb \
	$(TMP_DIR_DBG)/kpr2js.xsb \
	$(TMP_DIR_DBG)/kprconfig.xsb \
	$(TMP_DIR_DBG)/make.xsb \
	$(TMP_DIR_DBG)/markup.xsb \
	$(TMP_DIR_DBG)/makefileGrammar.xsb \
	$(TMP_DIR_DBG)/manifestGrammar.xsb \
	$(TMP_DIR_DBG)/packageGrammar.xsb \
	$(TMP_DIR_DBG)/plistGrammar.xsb \
	$(TMP_DIR_DBG)/templateGrammar.xsb \
	$(TMP_DIR_DBG)/tool.xsb \
	$(TMP_DIR_DBG)/xs2js.xsb

MAKE_MODULES_DBG = \
	$(TMP_DIR_DBG)/make/linux/aspen.xsb \
	$(TMP_DIR_DBG)/make/linux/gtk.xsb

MODULES_RLS = \
	$(TMP_DIR_RLS)/fs.xsb \
	$(TMP_DIR_RLS)/grammar.xsb \
	$(TMP_DIR_RLS)/infoset.xsb \
	$(TMP_DIR_RLS)/kpr2js.xsb \
	$(TMP_DIR_RLS)/kprconfig.xsb \
	$(TMP_DIR_RLS)/make.xsb \
	$(TMP_DIR_RLS)/markup.xsb \
	$(TMP_DIR_RLS)/makefileGrammar.xsb \
	$(TMP_DIR_RLS)/manifestGrammar.xsb \
	$(TMP_DIR_RLS)/packageGrammar.xsb \
	$(TMP_DIR_RLS)/plistGrammar.xsb \
	$(TMP_DIR_RLS)/templateGrammar.xsb \
	$(TMP_DIR_RLS)/tool.xsb \
	$(TMP_DIR_RLS)/xs2js.xsb

MAKE_MODULES_RLS = \
	$(TMP_DIR_RLS)/make/linux/aspen.xsb \
	$(TMP_DIR_RLS)/make/linux/bg3cdp.xsb \
	$(TMP_DIR_RLS)/make/linux/gtk.xsb

OBJECTS_DBG = \
	$(TMP_DIR_DBG)/fs.o \
	$(TMP_DIR_DBG)/grammar.o \
	$(TMP_DIR_DBG)/infoset.o \
	$(TMP_DIR_DBG)/markup.o \
	$(TMP_DIR_DBG)/tool.o
	
OBJECTS_RLS = \
	$(TMP_DIR_RLS)/fs.o \
	$(TMP_DIR_RLS)/grammar.o \
	$(TMP_DIR_RLS)/infoset.o \
	$(TMP_DIR_RLS)/markup.o \
	$(TMP_DIR_RLS)/tool.o

EXPAT_OBJECTS_DBG = \
	$(TMP_DIR_DBG)/xmlparse.o \
	$(TMP_DIR_DBG)/xmlrole.o \
	$(TMP_DIR_DBG)/xmltok.o
	
EXPAT_OBJECTS_RLS = \
	$(TMP_DIR_RLS)/xmlparse.o \
	$(TMP_DIR_RLS)/xmlrole.o \
	$(TMP_DIR_RLS)/xmltok.o

debug: DEBUG_VARIABLES $(TMP_DIR_DBG) $(TMP_DIR_DBG)/make/linux $(BIN_DIR_DBG) $(BIN_DIR_DBG)/tools.xsa $(BIN_DIR_DBG)/tools.so 

DEBUG_VARIABLES:
	$(eval XSC6 = $(XS6)/bin/linux/debug/xsc6)
	$(eval XSL6 = $(XS6)/bin/linux/debug/xsl6)

$(TMP_DIR_DBG):
	mkdir -p $(TMP_DIR_DBG)
	
$(TMP_DIR_DBG)/make/linux:
	mkdir -p $(TMP_DIR_DBG)/make/linux

$(BIN_DIR_DBG):
	mkdir -p $(BIN_DIR_DBG)
	
$(BIN_DIR_DBG)/tools.xsa $(TMP_DIR_DBG)/tools.xs.c: $(MODULES_DBG) $(MAKE_MODULES_DBG)
	$(XSL6) -a tools -b $(TMP_DIR_DBG) -o $(BIN_DIR_DBG) $(MODULES_DBG) $(MAKE_MODULES_DBG)

$(MODULES_DBG): $(TMP_DIR_DBG)/%.xsb: $(MOD_DIR)/%.js
	$(XSC6) -c -d -e $< -o $(TMP_DIR_DBG)

$(MAKE_MODULES_DBG): $(TMP_DIR_DBG)/make/linux/%.xsb: $(MOD_DIR)/make/linux/%.js
	$(XSC6) -c -d -e $< -o $(TMP_DIR_DBG)/make/linux

$(BIN_DIR_DBG)/tools.so: $(OBJECTS_DBG) $(EXPAT_OBJECTS_DBG) $(TMP_DIR_DBG)/tools.xs.o
	$(CC) $(LINK_OPTIONS) $(OBJECTS_DBG) $(EXPAT_OBJECTS_DBG) $(TMP_DIR_DBG)/tools.xs.o $(LIBRARIES) -o $(BIN_DIR_DBG)/tools.so

$(OBJECTS_DBG): $(INC_DIR)/xs.h
$(OBJECTS_DBG): $(TMP_DIR_DBG)/%.o: $(MOD_DIR)/%.c
	$(CC) $< $(C_OPTIONS_DBG) -c -o $@

$(EXPAT_OBJECTS_DBG): $(TMP_DIR_DBG)/%.o: $(EXPAT_DIR)/%.c
	$(CC) $< $(C_OPTIONS_DBG) -c -o $@
	
$(TMP_DIR_DBG)/tools.xs.o: $(TMP_DIR_DBG)/tools.xs.c
	$(CC) $< $(C_OPTIONS_DBG) -c -o $@

	
release: RELEASE_VARIABLES $(TMP_DIR_RLS) $(TMP_DIR_RLS)/make/linux $(BIN_DIR_RLS) $(BIN_DIR_RLS)/tools.xsa $(BIN_DIR_RLS)/tools.so 

RELEASE_VARIABLES:
	$(eval XSC6 = $(XS6)/bin/linux/release/xsc6)
	$(eval XSL6 = $(XS6)/bin/linux/release/xsl6)

$(TMP_DIR_RLS):
	mkdir -p $(TMP_DIR_RLS)
	
$(TMP_DIR_RLS)/make/linux:
	mkdir -p $(TMP_DIR_RLS)/make/linux

$(BIN_DIR_RLS):
	mkdir -p $(BIN_DIR_RLS)
	
$(BIN_DIR_RLS)/tools.xsa $(TMP_DIR_RLS)/tools.xs.c: $(MODULES_RLS) $(MAKE_MODULES_RLS)
	$(XSL6) -a tools -b $(TMP_DIR_RLS) -o $(BIN_DIR_RLS) $(MODULES_RLS) $(MAKE_MODULES_RLS)

$(MODULES_RLS): $(TMP_DIR_RLS)/%.xsb: $(MOD_DIR)/%.js
	$(XSC6) -c -d -e $< -o $(TMP_DIR_RLS)

$(MAKE_MODULES_RLS): $(TMP_DIR_RLS)/make/linux/%.xsb: $(MOD_DIR)/make/linux/%.js
	$(XSC6) -c -d -e $< -o $(TMP_DIR_RLS)/make/linux

$(BIN_DIR_RLS)/tools.so: $(OBJECTS_RLS) $(EXPAT_OBJECTS_RLS) $(TMP_DIR_RLS)/tools.xs.o
	$(CC) $(LINK_OPTIONS) $(OBJECTS_RLS) $(EXPAT_OBJECTS_RLS) $(TMP_DIR_RLS)/tools.xs.o $(LIBRARIES) -o $(BIN_DIR_RLS)/tools.so

$(OBJECTS_RLS): $(INC_DIR)/xs.h
$(OBJECTS_RLS): $(TMP_DIR_RLS)/%.o: $(MOD_DIR)/%.c
	$(CC) $< $(C_OPTIONS_RLS) -c -o $@
	
$(EXPAT_OBJECTS_RLS): $(TMP_DIR_RLS)/%.o: $(EXPAT_DIR)/%.c
	$(CC) $< $(C_OPTIONS_RLS) -c -o $@
	
$(TMP_DIR_RLS)/tools.xs.o: $(TMP_DIR_RLS)/tools.xs.c
	$(CC) $< $(C_OPTIONS_DBG) -c -o $@


clean:
	rm -rf $(BIN_DIR_DBG)/tools.xsa $(BIN_DIR_DBG)/tools.so
	rm -rf $(BIN_DIR_RLS)/tools.xsa $(BIN_DIR_RLS)/tools.so
	rm -rf $(TMP_DIR_DBG)
	rm -rf $(TMP_DIR_RLS)
