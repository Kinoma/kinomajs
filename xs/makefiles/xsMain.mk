<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->

<makefile>
<!-- Do not indent! Tabs are significant in makefiles... -->


<platform name="android">
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

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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


<platform name="MacOSX64">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/xslib.framework $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/xslib.framework: $(LIB_DIR)/xslib.framework
	rm -fr $(BIN_DIR)/xslib.framework
	cp -R $(LIB_DIR)/xslib.framework $(BIN_DIR)/

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/xslib.framework $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/xslib.framework: $(LIB_DIR)/xslib.framework
	rm -fr $(BIN_DIR)/xslib.framework
	cp -R $(LIB_DIR)/xslib.framework $(BIN_DIR)/

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/xslib.framework $(BIN_DIR)/$(PROGRAM)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/xslib.framework: $(LIB_DIR)/xslib.framework
	rm -fr $(BIN_DIR)/xslib.framework
	cp -R $(LIB_DIR)/xslib.framework $(BIN_DIR)/

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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

$(BIN_DIR)/$(PROGRAM): $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
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


<platform name="Windows">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)\xslib.dll $(BIN_DIR)\$(PROGRAM).exe

$(TMP_DIR):
	if not exist $(TMP_DIR)/$(NULL) mkdir $(TMP_DIR)

$(BIN_DIR):
	if not exist $(BIN_DIR)/$(NULL) mkdir $(BIN_DIR)

$(BIN_DIR)\xslib.dll : $(LIB_DIR)\xslib.dll
	echo Copying $(LIB_DIR)\xslib.dll to $(BIN_DIR)\xslib.dll
	copy $(LIB_DIR)\xslib.dll $(BIN_DIR)\xslib.dll

$(BIN_DIR)\$(PROGRAM).exe : $(TMP_DIR)\$(PROGRAM).xs.o $(OBJECTS)
	link \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)\$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		/out:$(TMP_DIR)\$(PROGRAM).exe
	if defined SUPPORT_MANIFESTS mt /manifest $(TMP_DIR)\$(PROGRAM).exe.manifest /outputresource:$(TMP_DIR)\$(PROGRAM).exe;#2
	copy $(TMP_DIR)\$(PROGRAM).exe $(BIN_DIR)\$(PROGRAM).exe

$(OBJECTS) : $(INC_DIR)\xs.h

$(TMP_DIR)\$(PROGRAM).xs.o : $(INC_DIR)\xs.h $(TMP_DIR)\$(PROGRAM).xs.c
	cl $(ALL_C_OPTIONS) /Fo$@ $(TMP_DIR)\$(PROGRAM).xs.c

$(TMP_DIR)\$(PROGRAM).xs.c : $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	del /Q $(BIN_DIR)/$(PROGRAM).exe
	del /Q $(TMP_DIR)/$(PROGRAM).*
	del /Q $(OBJECTS)
]]></common>
</platform>


</makefile>