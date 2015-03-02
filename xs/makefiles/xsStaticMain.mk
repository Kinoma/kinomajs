<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->

<makefile>
<!-- Do not indent! Tabs are significant in makefiles... -->


<platform name="android">
<common><![CDATA[
#LIB_DIR =
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS)
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES)

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(LIB_DIR)/libxs.a $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(LIB_DIR)/libxs.a \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM)

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<

$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM)
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="Linux">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(LIB_DIR)/libxs.a $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(LIB_DIR)/libxs.a \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM)

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM)
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="MacOSX">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(LIB_DIR)/libxs.a $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(LIB_DIR)/libxs.a \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM)

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM)
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="iPhone">
<common><![CDATA[
CC = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(LIB_DIR)/libxs.a $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(LIB_DIR)/libxs.a \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM)

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM)
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="Solaris">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(LIB_DIR)/libxs.a $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(LIB_DIR)/libxs.a \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM)

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM)
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


</makefile>