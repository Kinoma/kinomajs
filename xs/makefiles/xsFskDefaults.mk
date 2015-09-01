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
<common>

LINKER = $(deviceLINK)
XSC = $(XS_HOME)/bin/android/xsc
INC_DIR = $(XS_HOME)/includes
FINC_DIR = $(F_HOME)/tmp/include
ifeq "$(FSK_EXTENSION_EMBED)" "true"
    EMBED_DEFINE= -DFSK_EXTENSION_EMBED=1
endif

NDK_LIBS = $(NDK_DIR)platforms/android-14/arch-arm/usr/lib/
NDK_TOOCHAIN_VERSION?=4.4.3
NDK_TOOLCHAIN_HOME?=$(NDK_DIR)toolchains/arm-linux-androideabi-$(NDK_TOOCHAIN_VERSION)/prebuilt/darwin-x86
LIB_GCC = 	$(NDK_TOOLCHAIN_HOME)/lib/gcc/arm-linux-androideabi/$(NDK_TOOCHAIN_VERSION)/libgcc.a

LIB_C = 	-lc
LIB_GPLUS = -lstdc++
LIB_M =		-lm

LIB_KINOMA = $(F_HOME)build/android/inNDK/Play/project/libs/armeabi/libFsk.so

COMMON_C_OPTIONS = \
    $(PLATFORM_OPTS) \
    -D__FSK_LAYER__=1\
    -DSUPPORT_QT=1 \
    -D_REENTRANT \
    -fsigned-char \
    $(EMBED_DEFINE) \
    -DFSK_APPLICATION_"$(FSK_APPLICATION)"=1 \
    -I$(TMP_DIR) \
    -I$(INC_DIR) \
    -I$(FINC_DIR) \
	-I$(NDK_DIR)platforms/android-14/arch-arm/usr/include/	\
    -Wno-multichar \
    -Werror-implicit-function-declaration
COMMON_LIBRARIES =
COMMON_LINK_OPTIONS = -nostdlib -Wl,-soname,lib$(PROGRAM).so -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--fix-cortex-a8
COMMON_XSC_OPTIONS = -o $(TMP_DIR) -t android
C_OPTIONS =
LIBRARIES = $(LIB_GCC) -L$(NDK_LIBS) $(LIB_C) $(LIB_GPLUS) $(LIB_M) $(LIB_KINOMA)

ifeq "$(OSS2)" ""
	NATIVELIBS = $(OSS)lib/
else
	NATIVELIBS = $(OSS2)lib/
endif

LIBRARIES += -L$(NATIVELIBS) -ldl -llog

LINK_OPTIONS = -Wl,-rpath-link=platforms/android-14/arch-arm/usr/lib
XSC_OPTIONS =
</common>
<debug>
LIB_DIR = $(XS_HOME)/android/lib/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -Wall -O0
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -L$(LIB_DIR)/
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/android/lib/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -Os -fno-strict-aliasing
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -L$(LIB_DIR)/
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>



<platform name="Linux">
<common>
ifeq "$(deviceLIBNSL)" ""
	deviceLIBNSL = -lnsl
endif
ifeq "$(devicePTHREAD)" ""
	devicePTHREAD = -lpthread
endif
ifeq "$(deviceAR)" ""
	ARCHIVER = $(AR) rcs
else
	ARCHIVER = $(deviceAR) rcs
endif

LINKER = $(CC)
XSC = $(XS_HOME)/bin/Linux/xsc
INC_DIR = $(XS_HOME)/includes
FINC_DIR = $(F_HOME)/tmp/include
ifeq "$(FSK_EXTENSION_EMBED)" "true"
	EMBED_DEFINE= -DFSK_EXTENSION_EMBED=1
endif

COMMON_C_OPTIONS = \
	$(PLATFORM_OPTS) \
	-D__FSK_LAYER__=1 \
	-DSUPPORT_QT=1 \
	-D_REENTRANT \
	-fsigned-char \
	$(EMBED_DEFINE) \
	-DFSK_APPLICATION_"$(FSK_APPLICATION)"=1 \
	-I$(TMP_DIR) \
	-I$(INC_DIR) \
	-I$(FINC_DIR) \
	-Wno-multichar \
	-Werror-implicit-function-declaration
COMMON_LIBRARIES = $(deviceLIBNSL) $(devicePTHREAD) -lfsk
COMMON_LINK_OPTIONS = -L$(BIN_DIR)/ $(deviceLINKOPTS)
COMMON_XSC_OPTIONS = -o $(TMP_DIR) -t linux
C_OPTIONS =
LIBRARIES =
LINK_OPTIONS =
XSC_OPTIONS =
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -Wall
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -L$(LIB_DIR)/
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -O1
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -L$(LIB_DIR)/
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="MacOSX">
<common>
LINKER = $(CC)
XSC = $(XS_HOME)/bin/mac/xsc
INC_DIR = $(XS_HOME)/includes
FINC_DIR = $(F_HOME)/tmp/include
ifeq "$(FSK_EXTENSION_EMBED)" "true"
	EMBED_DEFINE= -DFSK_EXTENSION_EMBED=1
endif
COMMON_C_OPTIONS = \
	-D__FSK_LAYER__=1 \
	-Dmacintosh=1 \
	-DSUPPORT_QT=1 \
	-DFSK_APPLICATION_$(FSK_APPLICATION)=1 \
	-fvisibility=hidden						\
	$(EMBED_DEFINE)	\
	-I$(F_HOME)/tmp/mac \
	-I$(TMP_DIR) \
	-I$(FINC_DIR) \
	-I$(INC_DIR) $(UNIVERSAL_FLAGS) \
	-Wall
COMMON_LIBRARIES = \
	-framework CoreFoundation \
	-framework QuickTime \
	-framework fsk $(UNIVERSAL_FLAGS)
COMMON_LINK_OPTIONS = -ggdb -multiply_defined suppress -single_module
ifneq ("x$(SDKROOT)", "x")
	COMMON_C_OPTIONS += -isysroot $(SDKROOT)
	COMMON_LINK_OPTIONS += -isysroot $(SDKROOT)
endif
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
C_OPTIONS =
LIBRARIES =
LINK_OPTIONS =
XSC_OPTIONS = -t $(FSK_APPLICATION) -t mac
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/mac/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -O0
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -F$(F_HOME)/bin/mac/debug
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/mac/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -O3
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -F$(F_HOME)/bin/mac/release
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="iPhone">
<common>
ifeq ("$(PLATFORM_NAME)", "iphonesimulator")
	ARCH = -arch i386
	VERSION_MIN = -mmacosx-version-min=10.6 \
	-D__IPHONE_OS_VERSION_MIN_REQUIRED=50100
else
	ARCH = -arch armv7 -arch armv7s
	VERSION_MIN = -miphoneos-version-min=5.1
endif

CC = clang
LINKER = $(CC)
XSC = $(XS_HOME)/bin/mac/xsc
INC_DIR = $(XS_HOME)/includes
FINC_DIR = $(F_HOME)/tmp/include
ifeq "$(FSK_EXTENSION_EMBED)" "true"
	EMBED_DEFINE= -DFSK_EXTENSION_EMBED=1
endif
COMMON_C_OPTIONS = \
	-D__FSK_LAYER__=1 \
	-Dmacintosh=1 \
	-Diphone=1 \
	-DFSK_APPLICATION_$(FSK_APPLICATION)=1 \
	$(EMBED_DEFINE)	\
	-x objective-c \
	$(ARCH) \
	-fmessage-length=0 \
	-Wno-trigraphs \
	-fpascal-strings \
	-Wno-missing-field-initializers \
	-Wno-missing-prototypes \
	-Wreturn-type \
	-Wno-implicit-atomic-properties \
	-Wno-receiver-is-weak \
	-Wduplicate-method-match \
	-Wformat \
	-Wno-missing-braces \
	-Wparentheses \
	-Wswitch \
	-Wno-unused-function \
	-Wno-unused-label \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-Wno-unused-value \
	-Wuninitialized \
	-Wno-unknown-pragmas \
	-Wno-shadow \
	-Wno-four-char-constants \
	-Wno-conversion \
	-Wno-sign-compare \
	-Wno-shorten-64-to-32 \
	-Wpointer-sign \
	-Wno-newline-eof \
	-Wno-selector \
	-Wno-strict-selector-match \
	-Wno-undeclared-selector \
	-Wno-deprecated-implementations \
	-isysroot $(SDKROOT) \
	-Wprotocol \
	-Wdeprecated-declarations \
	-fvisibility=hidden \
	-Wno-sign-conversion \
	$(VERSION_MIN) \
	"-DIBOutlet=__attribute__((iboutlet))" \
	"-DIBOutletCollection(ClassName)=__attribute__((iboutletcollection(ClassName)))" \
	"-DIBAction=void)__attribute__((ibaction)" \
	-I$(F_HOME)/tmp/$(PLATFORM_NAME) \
	-I$(TMP_DIR) \
	-I$(FINC_DIR) \
	-I$(INC_DIR) $(UNIVERSAL_FLAGS)

# removed Xcode 4.4 default options
#	-Wunused-variable \
#	-Wunused-value \

ifeq ("$(PLATFORM_NAME)", "iphonesimulator")
COMMON_C_OPTIONS += -fasm-blocks \
	 -fobjc-abi-version=2 \
	 -fobjc-legacy-dispatch \
	 -fexceptions
endif

COMMON_LIBRARIES = \
	-framework CoreFoundation \
	-framework fsk $(UNIVERSAL_FLAGS)
COMMON_LINK_OPTIONS = -multiply_defined suppress -single_module -isysroot $(SDKROOT)
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
C_OPTIONS =
LIBRARIES =
LINK_OPTIONS =
XSC_OPTIONS = -t $(FSK_APPLICATION) -t iphone
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/$(PLATFORM_NAME)/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -O0
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -F$(F_HOME)/bin/$(PLATFORM_NAME)/debug
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/$(PLATFORM_NAME)/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -Os -DSUPPORT_XS_DEBUG=0
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -F$(F_HOME)/bin/$(PLATFORM_NAME)/release
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="Windows">
<common>
XSC = $(XS_HOME)\bin\win32\xsc
INC_DIR = $(XS_HOME)\includes
FINC_DIR = $(F_HOME)\tmp\include
PLAT_TMP_DIR = $(F_HOME)\tmp\win32
COMMON_C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D "__FSK_LAYER__=1" \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
!IF "$(FSK_EXTENSION_EMBED)" == "true"
	/D FSK_EXTENSION_EMBED=1 \
!ENDIF
	/D FSK_APPLICATION_"$(FSK_APPLICATION)"=1 \
	/I$(INC_DIR) \
	/I$(FINC_DIR) \
	/I$(PLAT_TMP_DIR) \
	/I$(SRC_DIR) \
	/nologo
COMMON_LIBRARIES = \
	ws2_32.lib \
	advapi32.lib \
	comctl32.lib \
	comdlg32.lib \
	gdi32.lib \
	kernel32.lib \
	user32.lib \
  shell32.lib
COMMON_LINK_OPTIONS = /machine:I386 /nologo /subsystem:console
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
C_OPTIONS =
LIBRARIES =
LINK_OPTIONS =
XSC_OPTIONS = -t $(FSK_APPLICATION) -t win
</common>
<debug>
LIB_DIR = $(XS_HOME)\lib\debug
FSK_TMP_DIR = $(PLAT_TMP_DIR)\fsk\debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) \
	/D _DEBUG \
	/D mxDebug \
	/I$(TMP_DIR) \
	/Fp$(TMP_DIR)\ \
!IF "$(FSK_EXTENSION_EMBED)" == "true"
	/MT \
!ELSE
	/MD \
!ENDIF
	/Od \
	/W3 \
	/Z7
!IF "$(FSK_EMBED)" == "1"
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES) $(F_HOME)\bin\win32\debug\fsk.exe.lib
!ELSE
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES) $(F_HOME)\bin\win32\debug\fsk.dll.lib
!ENDIF
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) /debug /incremental:no
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)\lib\release
FSK_TMP_DIR = $(PLAT_TMP_DIR)\fsk\release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) \
	/D NDEBUG \
	/I$(TMP_DIR) \
	/Fp$(TMP_DIR)\ \
!IF "$(FSK_EXTENSION_EMBED)" == "true"
	/MT \
!ELSE
	/MD \
!ENDIF
	/GL \
	/O2 \
	/W0
!IF "$(FSK_EMBED)" == "1"
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES) $(F_HOME)\bin\win32\release\fsk.exe.lib
!ELSE
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES) $(F_HOME)\bin\win32\release\fsk.dll.lib
!ENDIF
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) /incremental:no /LTCG
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<input name="sources"/>
<input name="$(F_HOME)/core/grammars"/>
<input name="$(F_HOME)/core/base"/>
<input name="$(F_HOME)/core/managers"/>
<input name="$(F_HOME)/core/scripting"/>
<input name="$(F_HOME)/core/graphics"/>
<input name="$(F_HOME)/core/network"/>
<input name="$(F_HOME)/core/misc"/>
<input name="$(F_HOME)/core/ui"/>


<platform name="Linux,MacOSX,iPhone,Solaris,Windows">
<output name="$(F_HOME)"/>
</platform>

</makefile>
