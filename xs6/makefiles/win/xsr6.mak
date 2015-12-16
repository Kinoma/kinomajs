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
BIN_DIR_DBG = $(XS6)\bin\win\debug
BIN_DIR_RLS = $(XS6)\bin\win\release
INC_DIR = $(XS6)\includes
SRC_DIR = $(XS6)\sources
TMP_DIR_DBG = $(XS6)\tmp\win\debug\xsr6
TMP_DIR_RLS = $(XS6)\tmp\win\release\xsr6

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D mxParse=1 \
	/D mxRun=1 \
	/I$(INC_DIR) \
	/I$(SRC_DIR) \
	/I$(SRC_DIR)\tool \
	/nologo \
	/Zp1 
C_OPTIONS_DBG = $(C_OPTIONS) \
	/D _DEBUG \
	/D mxDebug \
	/Fp$(TMP_DIR_DBG)\ \
	/Od \
	/W3 \
	/Z7
C_OPTIONS_RLS = $(C_OPTIONS) \
	/D NDEBUG \
	/Fp$(TMP_DIR_RLS)\ \
	/O2 \
	/W0

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo /subsystem:console
LINK_OPTIONS_DBG = $(LINK_OPTIONS) /debug
LINK_OPTIONS_RLS = $(LINK_OPTIONS)

OBJECTS_DBG = \
	$(TMP_DIR_DBG)\xs_dtoa.o \
	$(TMP_DIR_DBG)\xs_pcre.o \
	$(TMP_DIR_DBG)\xs6Common.o \
	$(TMP_DIR_DBG)\xs6All.o \
	$(TMP_DIR_DBG)\xs6Debug.o \
	$(TMP_DIR_DBG)\xs6Memory.o \
	$(TMP_DIR_DBG)\xs6Symbol.o \
	$(TMP_DIR_DBG)\xs6Type.o \
	$(TMP_DIR_DBG)\xs6Property.o \
	$(TMP_DIR_DBG)\xs6Global.o \
	$(TMP_DIR_DBG)\xs6Object.o \
	$(TMP_DIR_DBG)\xs6Function.o \
	$(TMP_DIR_DBG)\xs6Array.o \
	$(TMP_DIR_DBG)\xs6String.o \
	$(TMP_DIR_DBG)\xs6Boolean.o \
	$(TMP_DIR_DBG)\xs6Number.o \
	$(TMP_DIR_DBG)\xs6Math.o \
	$(TMP_DIR_DBG)\xs6Date.o \
	$(TMP_DIR_DBG)\xs6RegExp.o \
	$(TMP_DIR_DBG)\xs6Error.o \
	$(TMP_DIR_DBG)\xs6JSON.o \
	$(TMP_DIR_DBG)\xs6DataView.o \
	$(TMP_DIR_DBG)\xs6API.o \
	$(TMP_DIR_DBG)\xs6Profile.o \
	$(TMP_DIR_DBG)\xs6Run.o \
	$(TMP_DIR_DBG)\xs6Generator.o \
	$(TMP_DIR_DBG)\xs6Module.o \
	$(TMP_DIR_DBG)\xs6Promise.o \
	$(TMP_DIR_DBG)\xs6Proxy.o \
	$(TMP_DIR_DBG)\xs6MapSet.o \
	$(TMP_DIR_DBG)\xs6Marshall.o \
	$(TMP_DIR_DBG)\xs6Script.o \
	$(TMP_DIR_DBG)\xs6Lexical.o \
	$(TMP_DIR_DBG)\xs6Syntaxical.o \
	$(TMP_DIR_DBG)\xs6Tree.o \
	$(TMP_DIR_DBG)\xs6SourceMap.o \
	$(TMP_DIR_DBG)\xs6Scope.o \
	$(TMP_DIR_DBG)\xs6Code.o \
	$(TMP_DIR_DBG)\xsr6.o

OBJECTS_RLS = \
	$(TMP_DIR_RLS)\xs_dtoa.o \
	$(TMP_DIR_RLS)\xs_pcre.o \
	$(TMP_DIR_RLS)\xs6Common.o \
	$(TMP_DIR_RLS)\xs6All.o \
	$(TMP_DIR_RLS)\xs6Debug.o \
	$(TMP_DIR_RLS)\xs6Memory.o \
	$(TMP_DIR_RLS)\xs6Symbol.o \
	$(TMP_DIR_RLS)\xs6Type.o \
	$(TMP_DIR_RLS)\xs6Property.o \
	$(TMP_DIR_RLS)\xs6Global.o \
	$(TMP_DIR_RLS)\xs6Object.o \
	$(TMP_DIR_RLS)\xs6Function.o \
	$(TMP_DIR_RLS)\xs6Array.o \
	$(TMP_DIR_RLS)\xs6String.o \
	$(TMP_DIR_RLS)\xs6Boolean.o \
	$(TMP_DIR_RLS)\xs6Number.o \
	$(TMP_DIR_RLS)\xs6Math.o \
	$(TMP_DIR_RLS)\xs6Date.o \
	$(TMP_DIR_RLS)\xs6RegExp.o \
	$(TMP_DIR_RLS)\xs6Error.o \
	$(TMP_DIR_RLS)\xs6JSON.o \
	$(TMP_DIR_RLS)\xs6DataView.o \
	$(TMP_DIR_RLS)\xs6API.o \
	$(TMP_DIR_RLS)\xs6Profile.o \
	$(TMP_DIR_RLS)\xs6Run.o \
	$(TMP_DIR_RLS)\xs6Generator.o \
	$(TMP_DIR_RLS)\xs6Module.o \
	$(TMP_DIR_RLS)\xs6Promise.o \
	$(TMP_DIR_RLS)\xs6Proxy.o \
	$(TMP_DIR_RLS)\xs6MapSet.o \
	$(TMP_DIR_RLS)\xs6Marshall.o \
	$(TMP_DIR_RLS)\xs6Script.o \
	$(TMP_DIR_RLS)\xs6Lexical.o \
	$(TMP_DIR_RLS)\xs6Syntaxical.o \
	$(TMP_DIR_RLS)\xs6Tree.o \
	$(TMP_DIR_RLS)\xs6SourceMap.o \
	$(TMP_DIR_RLS)\xs6Scope.o \
	$(TMP_DIR_RLS)\xs6Code.o \
	$(TMP_DIR_RLS)\xsr6.o


release : $(TMP_DIR_RLS) $(BIN_DIR_RLS) $(BIN_DIR_RLS)\xsr6.exe

$(TMP_DIR_RLS) :
	if not exist $(TMP_DIR_RLS)\$(NULL) mkdir $(TMP_DIR_RLS)

$(BIN_DIR_RLS) :
	if not exist $(BIN_DIR_RLS)\$(NULL) mkdir $(BIN_DIR_RLS)

$(BIN_DIR_RLS)\xsr6.exe : $(OBJECTS_RLS) $(TMP_DIR_RLS)\xs6Host.o $(TMP_DIR_RLS)\xs6Platform.o
	link \
		$(LINK_OPTIONS_RLS) \
		$(LIBRARIES) \
		$(OBJECTS_RLS) \
		$(TMP_DIR_RLS)\xs6Host.o \
		$(TMP_DIR_RLS)\xs6Platform.o \
		/out:$(BIN_DIR_RLS)\xsr6.exe

$(OBJECTS_RLS) : $(SRC_DIR)\tool\xs6Platform.h
$(OBJECTS_RLS) : $(SRC_DIR)\xs6Common.h
$(OBJECTS_RLS) : $(SRC_DIR)\xs6All.h
$(OBJECTS_RLS) : $(SRC_DIR)\xs6Script.h

{$(SRC_DIR)\}.c{$(TMP_DIR_RLS)\}.o:
	cl $< $(C_OPTIONS_RLS) /Fo$@
$(TMP_DIR_RLS)\xs6Host.o : $(SRC_DIR)\tool\xs6Host.c
	cl $(SRC_DIR)\tool\xs6Host.c $(C_OPTIONS_RLS) /Fo$@
$(TMP_DIR_RLS)\xs6Platform.o : $(SRC_DIR)\tool\xs6Platform.c
	cl $(SRC_DIR)\tool\xs6Platform.c $(C_OPTIONS_RLS) /Fo$@


debug : $(TMP_DIR_DBG) $(BIN_DIR_DBG) $(BIN_DIR_DBG)\xsr6.exe

$(TMP_DIR_DBG) :
	if not exist $(TMP_DIR_DBG)\$(NULL) mkdir $(TMP_DIR_DBG)

$(BIN_DIR_DBG) :
	if not exist $(BIN_DIR_DBG)\$(NULL) mkdir $(BIN_DIR_DBG)

$(BIN_DIR_DBG)\xsr6.exe : $(OBJECTS_DBG) $(TMP_DIR_DBG)\xs6Host.o $(TMP_DIR_DBG)\xs6Platform.o
	link \
		$(LINK_OPTIONS_DBG) \
		$(LIBRARIES) \
		$(OBJECTS_DBG) \
		$(TMP_DIR_DBG)\xs6Host.o \
		$(TMP_DIR_DBG)\xs6Platform.o \
		/out:$(BIN_DIR_DBG)\xsr6.exe

$(OBJECTS_DBG) : $(SRC_DIR)\tool\xs6Platform.h
$(OBJECTS_DBG) : $(SRC_DIR)\xs6Common.h
$(OBJECTS_DBG) : $(SRC_DIR)\xs6All.h
$(OBJECTS_DBG) : $(SRC_DIR)\xs6Script.h

{$(SRC_DIR)\}.c{$(TMP_DIR_DBG)\}.o:
	cl $< $(C_OPTIONS_DBG) /Fo$@
$(TMP_DIR_DBG)\xs6Host.o : $(SRC_DIR)\tool\xs6Host.c
	cl $(SRC_DIR)\tool\xs6Host.c $(C_OPTIONS_DBG) /Fo$@
$(TMP_DIR_DBG)\xs6Platform.o : $(SRC_DIR)\tool\xs6Platform.c
	cl $(SRC_DIR)\tool\xs6Platform.c $(C_OPTIONS_DBG) /Fo$@


build: debug release


clean :
	del /Q $(BIN_DIR_DBG)\xsr6.exe
	del /Q $(TMP_DIR_DBG)
	del /Q $(BIN_DIR_RLS)\xsr6.exe
	del /Q $(TMP_DIR_RLS)

