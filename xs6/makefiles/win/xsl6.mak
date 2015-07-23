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
BIN_DIR_DBG = $(XS6)\bin\win\debug
BIN_DIR_RLS = $(XS6)\bin\win\release
INC_DIR = $(XS6)\includes
SRC_DIR = $(XS6)\sources
TMP_DIR_DBG = $(XS6)\tmp\win\debug\xsl6
TMP_DIR_RLS = $(XS6)\tmp\win\release\xsl6

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D mxLink \
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
	$(TMP_DIR_DBG)\xs6Common.o \
	$(TMP_DIR_DBG)\xsl6.o

OBJECTS_RLS = \
	$(TMP_DIR_RLS)\xs_dtoa.o \
	$(TMP_DIR_RLS)\xs6Common.o \
	$(TMP_DIR_RLS)\xsl6.o


release : $(TMP_DIR_RLS) $(BIN_DIR_RLS) $(BIN_DIR_RLS)\xsl6.exe

$(TMP_DIR_RLS) :
	if not exist $(TMP_DIR_RLS)\$(NULL) mkdir $(TMP_DIR_RLS)

$(BIN_DIR_RLS) :
	if not exist $(BIN_DIR_RLS)\$(NULL) mkdir $(BIN_DIR_RLS)

$(BIN_DIR_RLS)\xsl6.exe : $(OBJECTS_RLS)
	link \
		$(LINK_OPTIONS_RLS) \
		$(LIBRARIES) \
		$(OBJECTS_RLS) \
		/out:$(BIN_DIR_RLS)\xsl6.exe

$(OBJECTS_RLS) : $(SRC_DIR)\tool\xs6Platform.h
$(OBJECTS_RLS) : $(SRC_DIR)\xs6Common.h

{$(SRC_DIR)\}.c{$(TMP_DIR_RLS)\}.o:
	cl $< $(C_OPTIONS_RLS) /Fo$@


debug : $(TMP_DIR_DBG) $(BIN_DIR_DBG) $(BIN_DIR_DBG)\xsl6.exe

$(TMP_DIR_DBG) :
	if not exist $(TMP_DIR_DBG)\$(NULL) mkdir $(TMP_DIR_DBG)

$(BIN_DIR_DBG) :
	if not exist $(BIN_DIR_DBG)\$(NULL) mkdir $(BIN_DIR_DBG)

$(BIN_DIR_DBG)\xsl6.exe : $(OBJECTS_DBG)
	link \
		$(LINK_OPTIONS_DBG) \
		$(LIBRARIES) \
		$(OBJECTS_DBG) \
		/out:$(BIN_DIR_DBG)\xsl6.exe

$(OBJECTS_DBG) : $(SRC_DIR)\tool\xs6Platform.h
$(OBJECTS_DBG) : $(SRC_DIR)\xs6Common.h

{$(SRC_DIR)\}.c{$(TMP_DIR_DBG)\}.o:
	cl $< $(C_OPTIONS_DBG) /Fo$@

build: debug release

clean :
	del /Q $(BIN_DIR_DBG)\xsl6.exe
	del /Q $(TMP_DIR_DBG)
	del /Q $(BIN_DIR_RLS)\xsl6.exe
	del /Q $(TMP_DIR_RLS)

