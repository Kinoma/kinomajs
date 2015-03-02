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

<input name="sources"/>


<platform name="android">
<common>
CC = $(XS_XPREFIX)gcc
C_OPTIONS =
LIBRARIES =
LINKER = $(CC)
LINK_OPTIONS = -L$(XS_XROOT)/lib
XSC_OPTIONS =
INC_DIR = $(XS_HOME)/includes
COMMON_C_OPTIONS = -I$(TMP_DIR) -I$(INC_DIR)
COMMON_LIBRARIES = -lm -lnsl -lpthread -ldl
COMMON_LINK_OPTIONS =
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
XSC = $(XS_HOME)/bin/android/xsc
</common>
<debug>
LIB_DIR = $(XS_HOME)/android/lib/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -Wall
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS)
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/android/lib/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -Os -fno-strict-aliasing
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS)
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="Linux">
<common>
C_OPTIONS =
LIBRARIES =
LINKER = $(CC)
LINK_OPTIONS =
XSC_OPTIONS =
INC_DIR = $(XS_HOME)/includes
COMMON_C_OPTIONS = -I$(TMP_DIR) -I$(INC_DIR)
COMMON_LIBRARIES = -lm -lnsl -lpthread
COMMON_LINK_OPTIONS =
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
XSC = $(XS_HOME)/bin/Linux/xsc
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/Linux/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -Wall
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS)
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/Linux/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -O1
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS)
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="MacOSX">
<common>
C_OPTIONS =
LIBRARIES =
LINKER = $(CC)
LINK_OPTIONS =
XSC_OPTIONS =
INC_DIR = $(XS_HOME)/includes
COMMON_C_OPTIONS = $(UNIVERSAL_FLAGS) -I$(TMP_DIR) -I$(INC_DIR)
COMMON_LIBRARIES = $(UNIVERSAL_FLAGS) -framework CoreServices
COMMON_LINK_OPTIONS =
ifneq ("x$(SDKROOT)", "x")
	COMMON_C_OPTIONS += -isysroot $(SDKROOT)
	COMMON_LINK_OPTIONS += -isysroot $(SDKROOT)
endif
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
XSC = $(XS_HOME)/bin/mac/xsc
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/mac/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -O1 -Wall
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS)
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/mac/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -O3
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) -F$(LIB_DIR)/
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


<platform name="Solaris">
<common>
C_OPTIONS =
LIBRARIES =
LINK_OPTIONS =
LINKER = $(CC)
XSC_OPTIONS =
INC_DIR = $(XS_HOME)/includes
COMMON_C_OPTIONS = -I$(TMP_DIR) -I$(INC_DIR) 
COMMON_LIBRARIES = -lm -lsocket -lnsl -lpthread
COMMON_LINK_OPTIONS =
COMMON_XSC_OPTIONS = -o $(TMP_DIR)
XSC = $(XS_HOME)/bin/Solaris/xsc
</common>
<debug>
LIB_DIR = $(XS_HOME)/lib/debug
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS) -DmxDebug -g -Wall
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) 
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS) -d
</debug>
<release>
LIB_DIR = $(XS_HOME)/lib/release
DEFAULT_C_OPTIONS = $(COMMON_C_OPTIONS)
DEFAULT_LIBRARIES = $(COMMON_LIBRARIES)
DEFAULT_LINK_OPTIONS = $(COMMON_LINK_OPTIONS) 
DEFAULT_XSC_OPTIONS = $(COMMON_XSC_OPTIONS)
</release>
</platform>


</makefile>