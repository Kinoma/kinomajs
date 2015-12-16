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
.SUFFIXES : .js

BIN_DIR_DBG = $(XS6)\bin\win\debug\modules
BIN_DIR_RLS = $(XS6)\bin\win\release\modules
EXPAT_DIR = $(F_HOME)\libraries\expat
INC_DIR = $(XS6)\includes
LIB_DIR_DBG = $(XS6)\bin\win\debug
LIB_DIR_RLS = $(XS6)\bin\win\release
MOD_DIR = $(XS6)\tools
SRC_DIR = $(XS6)\sources
TMP_DIR_DBG = $(XS6)\tmp\win\debug\tools
TMP_DIR_RLS = $(XS6)\tmp\win\release\tools

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/D XML_STATIC=1 \
	/I$(EXPAT_DIR) \
	/I$(INC_DIR) \
	/I$(SRC_DIR) \
	/I$(SRC_DIR)\tool \
	/nologo \
	/Zp1 
C_OPTIONS_DBG = $(C_OPTIONS) \
	/D _DEBUG \
	/D mxDebug \
	/I$(TMP_DIR_DBG) \
	/Fp$(TMP_DIR_DBG)\ \
	/Od \
	/W3 \
	/Z7
C_OPTIONS_RLS = $(C_OPTIONS) \
	/D NDEBUG \
	/I$(TMP_DIR_RLS) \
	/Fp$(TMP_DIR_RLS)\ \
	/O2 \
	/W0

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo /dll
LINK_OPTIONS_DBG = $(LINK_OPTIONS) /debug
LINK_OPTIONS_RLS = $(LINK_OPTIONS)

MODULES_DBG = \
	$(TMP_DIR_DBG)\fs.xsb \
	$(TMP_DIR_DBG)\grammar.xsb \
	$(TMP_DIR_DBG)\infoset.xsb \
	$(TMP_DIR_DBG)\kpr2js.xsb \
	$(TMP_DIR_DBG)\kprconfig.xsb \
	$(TMP_DIR_DBG)\make.xsb \
	$(TMP_DIR_DBG)\markup.xsb \
	$(TMP_DIR_DBG)\makefileGrammar.xsb \
	$(TMP_DIR_DBG)\manifestGrammar.xsb \
	$(TMP_DIR_DBG)\packageGrammar.xsb \
	$(TMP_DIR_DBG)\templateGrammar.xsb \
	$(TMP_DIR_DBG)\tool.xsb \
	$(TMP_DIR_DBG)\xs2js.xsb
	
MAKE_MODULES_DBG = \
	$(TMP_DIR_DBG)\make\win.xsb

MODULES_RLS = \
	$(TMP_DIR_RLS)\fs.xsb \
	$(TMP_DIR_RLS)\grammar.xsb \
	$(TMP_DIR_RLS)\infoset.xsb \
	$(TMP_DIR_RLS)\kpr2js.xsb \
	$(TMP_DIR_RLS)\kprconfig.xsb \
	$(TMP_DIR_RLS)\make.xsb \
	$(TMP_DIR_RLS)\markup.xsb \
	$(TMP_DIR_RLS)\makefileGrammar.xsb \
	$(TMP_DIR_RLS)\manifestGrammar.xsb \
	$(TMP_DIR_RLS)\packageGrammar.xsb \
	$(TMP_DIR_RLS)\templateGrammar.xsb \
	$(TMP_DIR_RLS)\tool.xsb \
	$(TMP_DIR_RLS)\xs2js.xsb
	
MAKE_MODULES_RLS = \
	$(TMP_DIR_RLS)\make\win.xsb

OBJECTS_DBG = \
	$(TMP_DIR_DBG)\fs.o \
	$(TMP_DIR_DBG)\grammar.o \
	$(TMP_DIR_DBG)\infoset.o \
	$(TMP_DIR_DBG)\markup.o \
	$(TMP_DIR_DBG)\tool.o
	
OBJECTS_RLS = \
	$(TMP_DIR_RLS)\fs.o \
	$(TMP_DIR_RLS)\grammar.o \
	$(TMP_DIR_RLS)\infoset.o \
	$(TMP_DIR_RLS)\markup.o \
	$(TMP_DIR_RLS)\tool.o

EXPAT_OBJECTS_DBG = \
	$(TMP_DIR_DBG)\xmlparse.o \
	$(TMP_DIR_DBG)\xmlrole.o \
	$(TMP_DIR_DBG)\xmltok.o
	
EXPAT_OBJECTS_RLS = \
	$(TMP_DIR_RLS)\xmlparse.o \
	$(TMP_DIR_RLS)\xmlrole.o \
	$(TMP_DIR_RLS)\xmltok.o


release : $(TMP_DIR_RLS) $(TMP_DIR_RLS)\make $(BIN_DIR_RLS) $(BIN_DIR_RLS)/tools.xsa $(BIN_DIR_RLS)\tools.dll

$(TMP_DIR_RLS) :
	if not exist $(TMP_DIR_RLS)\$(NULL) mkdir $(TMP_DIR_RLS)

$(TMP_DIR_RLS)\make :
	if not exist $(TMP_DIR_RLS)\make\$(NULL) mkdir $(TMP_DIR_RLS)\make

$(BIN_DIR_RLS) :
	if not exist $(BIN_DIR_RLS)\$(NULL) mkdir $(BIN_DIR_RLS)

$(BIN_DIR_RLS)\tools.xsa $(TMP_DIR_RLS)\tools.xs.c : $(MODULES_RLS) $(MAKE_MODULES_RLS)
	$(XS6)\bin\win\release\xsl6 -a tools -b $(TMP_DIR_RLS) -o $(BIN_DIR_RLS) $(MODULES_RLS) $(MAKE_MODULES_RLS)

{$(MOD_DIR)\}.js{$(TMP_DIR_RLS)\}.xsb:
	$(XS6)\bin\win\release\xsc6 $< -c -d -e -o $(TMP_DIR_RLS)
	
{$(MOD_DIR)\make}.js{$(TMP_DIR_RLS)\make}.xsb:
	$(XS6)\bin\win\release\xsc6 $< -c -d -e -o $(TMP_DIR_RLS)\make

$(BIN_DIR_RLS)\tools.dll : $(OBJECTS_RLS) $(EXPAT_OBJECTS_RLS) $(TMP_DIR_RLS)\tools.xs.o
	link \
		$(LINK_OPTIONS_RLS) \
		$(LIBRARIES) \
		$(LIB_DIR_RLS)\xsr6.lib \
		$(OBJECTS_RLS) \
		$(EXPAT_OBJECTS_RLS) \
		$(TMP_DIR_RLS)\tools.xs.o \
		/out:$(BIN_DIR_RLS)\tools.dll

$(OBJECTS_RLS) : $(INC_DIR)\xs.h

{$(MOD_DIR)\}.c{$(TMP_DIR_RLS)\}.o:
	cl $< $(C_OPTIONS_RLS) /Fo$@
	
{$(EXPAT_DIR)\}.c{$(TMP_DIR_RLS)\}.o:
	cl $< $(C_OPTIONS_RLS) /Fo$@
	
{$(TMP_DIR_RLS)\}.c{$(TMP_DIR_RLS)\}.o:
	cl $< $(C_OPTIONS_RLS) /Fo$@


debug : $(TMP_DIR_DBG) $(TMP_DIR_DBG)\make $(BIN_DIR_DBG) $(BIN_DIR_DBG)\tools.xsa $(BIN_DIR_DBG)\tools.dll

$(TMP_DIR_DBG) :
	if not exist $(TMP_DIR_DBG)\$(NULL) mkdir $(TMP_DIR_DBG)

$(TMP_DIR_DBG)\make :
	if not exist $(TMP_DIR_DBG)\make\$(NULL) mkdir $(TMP_DIR_DBG)\make

$(BIN_DIR_DBG) :
	if not exist $(BIN_DIR_DBG)\$(NULL) mkdir $(BIN_DIR_DBG)

$(BIN_DIR_DBG)\tools.xsa $(TMP_DIR_DBG)\tools.xs.c : $(MODULES_DBG) $(MAKE_MODULES_DBG)
	$(XS6)\bin\win\debug\xsl6 -a tools -b $(TMP_DIR_DBG) -o $(BIN_DIR_DBG) $(MODULES_DBG) $(MAKE_MODULES_DBG)

{$(MOD_DIR)\}.js{$(TMP_DIR_DBG)\}.xsb:
	$(XS6)\bin\win\debug\xsc6 $< -c -d -e -o $(TMP_DIR_DBG)
	
{$(MOD_DIR)\make}.js{$(TMP_DIR_DBG)\make}.xsb:
	$(XS6)\bin\win\debug\xsc6 $< -c -d -e -o $(TMP_DIR_DBG)\make

$(BIN_DIR_DBG)\tools.dll : $(OBJECTS_DBG) $(EXPAT_OBJECTS_DBG) $(TMP_DIR_DBG)\tools.xs.o
	link \
		$(LINK_OPTIONS_DBG) \
		$(LIBRARIES) \
		$(LIB_DIR_DBG)\xsr6.lib \
		$(OBJECTS_DBG) \
		$(EXPAT_OBJECTS_DBG) \
		$(TMP_DIR_DBG)\tools.xs.o \
		/out:$(BIN_DIR_DBG)\tools.dll

$(OBJECTS_DBG) : $(INC_DIR)\xs.h

{$(MOD_DIR)\}.c{$(TMP_DIR_DBG)\}.o:
	cl $< $(C_OPTIONS_DBG) /Fo$@
	
{$(EXPAT_DIR)\}.c{$(TMP_DIR_DBG)\}.o:
	cl $< $(C_OPTIONS_DBG) /Fo$@
	
{$(TMP_DIR_DBG)\}.c{$(TMP_DIR_DBG)\}.o:
	cl $< $(C_OPTIONS_DBG) /Fo$@
	

build: debug release

clean :
	del /Q $(BIN_DIR_DBG)\xsr6.exe
	del /Q $(TMP_DIR_DBG)
	del /Q $(BIN_DIR_RLS)\xsr6.exe
	del /Q $(TMP_DIR_RLS)

