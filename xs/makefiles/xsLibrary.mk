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
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_INCLUDES) $(XSC_OPTIONS)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS)
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES)

% : %.c
%.o : %.c

ifeq "$(FSK_EXTENSION_EMBED)" "true"
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).a
else
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).so
ALL_C_OPTIONS += -fPIC
endif

build: $(TMP_DIR) $(BIN_DIR) $(BUILD_OUTPUT)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM).so: $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM).so

$(BIN_DIR)/$(PROGRAM).a : $(OBJECTS)
	echo '$(OBJECTS) ' >> $(BIN_DIR)/libobjectfiles.txt
#	$(ARCHIVER) \
#		$(TMP_DIR)/$(PROGRAM).a \
#		$(OBJECTS)
#	cp $(TMP_DIR)/$(PROGRAM).a $(BIN_DIR)/$(PROGRAM).a

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<

$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).so
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>



<platform name="Linux">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) -shared -Wl,-Bdynamic,-Bsymbolic
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

ifeq "$(FSK_EXTENSION_EMBED)" "true"
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).a
else
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).so
ALL_C_OPTIONS += -fPIC
endif

build: $(TMP_DIR) $(BIN_DIR) $(BUILD_OUTPUT)


$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM).so: $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM).so

$(BIN_DIR)/$(PROGRAM).a : $(OBJECTS)
	$(ARCHIVER) \
		$(TMP_DIR)/$(PROGRAM).a	\
		$(OBJECTS)
#	cp $(TMP_DIR)/$(PROGRAM).a $(BIN_DIR)/$(PROGRAM).a
	echo '$(OBJECTS)' >> $(BIN_DIR)/libobjectfiles.txt


$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).so
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="MacOSX">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES) -fno-common
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) -dynamiclib
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

ifeq "$(FSK_EXTENSION_EMBED)" "true"
BUILD_OUTPUT = $(OBJECTS) embed
else
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).so
endif

build: $(TMP_DIR) $(BIN_DIR) $(BUILD_OUTPUT)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

embed:
	@(echo '$(OBJECTS)' >> $(BIN_DIR)/libobjectfiles.txt)

$(BIN_DIR)/$(PROGRAM).so: $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM).so

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).so
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="iPhone">
<common><![CDATA[
CC = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES) -fno-common
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) -dynamiclib
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

ifeq "$(FSK_EXTENSION_EMBED)" "true"
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).a
else
BUILD_OUTPUT = $(BIN_DIR)/$(PROGRAM).so
endif

build: $(TMP_DIR) $(BIN_DIR) $(BUILD_OUTPUT)


$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM).a: $(OBJECTS)
	rm -f $(TMP_DIR)/$(PROGRAM).a
	ar rcs \
		$(TMP_DIR)/$(PROGRAM).a \
		$(OBJECTS)
	cp $(TMP_DIR)/$(PROGRAM).a $(BIN_DIR)/$(PROGRAM).a
	echo '$(OBJECTS)' >> $(BIN_DIR)/libobjectfiles.txt

$(BIN_DIR)/$(PROGRAM).so: $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM).so

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).so
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="Solaris">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES) -fPIC
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) -shared -Wl\,-B\,dynamic\,-B\,symbolic 
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

% : %.c
%.o : %.c

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM).so

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM).so: $(TMP_DIR)/$(PROGRAM).xs.o $(OBJECTS)
	$(LINKER) \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)/$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		-o $(BIN_DIR)/$(PROGRAM).so

$(OBJECTS): $(INC_DIR)/xs.h

$(TMP_DIR)/$(PROGRAM).xs.o: $(TMP_DIR)/$(PROGRAM).xs.c
	$(CC) $(ALL_C_OPTIONS) -c -o $@ $<
	
$(TMP_DIR)/$(PROGRAM).xs.c: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).so
	rm -f $(TMP_DIR)/$(PROGRAM).*
	rm -f $(OBJECTS)
]]></common>
</platform>


<platform name="Windows">
<common><![CDATA[
ALL_C_OPTIONS = $(DEFAULT_C_OPTIONS) $(C_OPTIONS) $(C_INCLUDES)
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_OPTIONS) $(XSC_INCLUDES)
ALL_LINK_OPTIONS = $(DEFAULT_LINK_OPTIONS) $(LINK_OPTIONS) /dll
ALL_LIBRARIES = $(DEFAULT_LIBRARIES) $(LIBRARIES) 

!IF "$(FSK_EXTENSION_EMBED)" == "true"
BUILD_OUTPUT = $(BIN_DIR)\$(PROGRAM).lib
!ELSE
BUILD_OUTPUT = $(BIN_DIR)\$(PROGRAM).dll
!ENDIF

!IF "$(FSK_EXTENSION_EMBED)" != "true"
FSK_EXTENSIONS_EMBED_LIBRARIES =
!ELSE IF "$(LIBRARIES)" == ""
FSK_EXTENSIONS_EMBED_LIBRARIES =
!ELSE
FSK_EXTENSIONS_EMBED_LIBRARIES = FskExtensionsEmbedLibraries.rsp
!ENDIF

build: $(TMP_DIR) $(BIN_DIR) $(FSK_EXTENSIONS_EMBED_LIBRARIES) $(BUILD_OUTPUT)

$(TMP_DIR):
	if not exist $(TMP_DIR)/$(NULL) mkdir $(TMP_DIR)

$(BIN_DIR):
	if not exist $(BIN_DIR)/$(NULL) mkdir $(BIN_DIR)

FskExtensionsEmbedLibraries.rsp:
	echo $(LIBRARIES) >> $(FSK_TMP_DIR)\FskExtensionsEmbedLibraries.rsp

$(BIN_DIR)\$(PROGRAM).dll : $(TMP_DIR)\$(PROGRAM).xs.o $(OBJECTS)
	link \
		$(ALL_LINK_OPTIONS) \
		$(TMP_DIR)\$(PROGRAM).xs.o \
		$(OBJECTS) \
		$(ALL_LIBRARIES) \
		/out:$(TMP_DIR)\$(PROGRAM).dll
	if defined SUPPORT_MANIFESTS mt /manifest $(TMP_DIR)\$(PROGRAM).dll.manifest /outputresource:$(TMP_DIR)\$(PROGRAM).dll;#2
	copy $(TMP_DIR)\$(PROGRAM).dll $(BIN_DIR)\$(PROGRAM).dll

$(BIN_DIR)\$(PROGRAM).lib : $(OBJECTS)
	lib \
		$(OBJECTS) \
		/out:$(TMP_DIR)\$(PROGRAM).lib
	copy $(TMP_DIR)\$(PROGRAM).lib $(BIN_DIR)\$(PROGRAM).lib

$(OBJECTS) : $(INC_DIR)\xs.h

$(TMP_DIR)\$(PROGRAM).xs.o : $(INC_DIR)\xs.h $(TMP_DIR)\$(PROGRAM).xs.c
	cl $(ALL_C_OPTIONS) /Fo$@ $(TMP_DIR)\$(PROGRAM).xs.c

$(TMP_DIR)\$(PROGRAM).xs.c : $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	del /Q $(BIN_DIR)/$(PROGRAM).dll
	del /Q $(TMP_DIR)/$(PROGRAM).*
	del /Q $(OBJECTS)
]]></common>
</platform>


</makefile>