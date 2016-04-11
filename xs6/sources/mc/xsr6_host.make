#
#     Copyright (C) 2010-2016 Marvell International Ltd.
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

% : %.c
%.o : %.c

export TARGET_SYSTEM = mac

export MACOS_ARCH = -arch i386
export MACOS_VERSION_MIN = -mmacosx-version-min=10.7

C_OPTIONS = $(MACOS_ARCH) $(MAXOS_VERSION_MIN) -DmxRun=1 -DmxDebug=1 -DmxNoFunctionLength -DmxNoFunctionName -g -Wall -I$(XS6_INC_DIR) -I$(XS6_SRC_DIR) -I$(XS6_SRC_DIR)/tool -I$(XS6_MC_DIR) -I$(XS6_MC_DIR)/extensions/crypt -DXS_ARCHIVE=$(XS_ARCHIVE)
LINK_OPTIONS = $(MACOS_ARCH) $(MAXOS_VERSION_MIN)
LIBRARIES = -framework CoreServices

ifeq ($(XS_ARCHIVE), 1)
	XSC_OPTIONS = -e
	mod_target = archive
else
	XSC_OPTIONS =
	mod_target = all
endif

export XS_COMPILER = 1

ifeq ($(DEBUG),)
	BIN_DIR = $(XS6)/bin/mac/release
	TMP_DIR = $(XS6)/tmp/mac/release/mc
	C_OPTIONS += -D_RELEASE=1 -O3
else
	BIN_DIR = $(XS6)/bin/mac/debug
	TMP_DIR = $(XS6)/tmp/mac/debug/mc
	C_OPTIONS += -D_DEBUG=1 -DmxDebug=1 -DmxReport=0
	XSC_OPTIONS += -d
endif
C_OPTIONS += -I$(TMP_DIR)

ifdef XS_COMPILER
C_OPTIONS += -DmxParse=1
endif

export DEST_DIR = $(TMP_DIR)/modules
export BIN_DIR
export MOD_DIR = $(BIN_DIR)/modules
export TMP_DIR
export XSC_OPTIONS
export C_OPTIONS
export MOD_LINK_OPTIONS = -dynamiclib -flat_namespace -undefined suppress
export MOD_LIBRARIES =  -L$(TMP_DIR_HOST) -lmc -framework CoreServices -lSystem

export LIBMODULE = libmodule.a
LIBMC = $(TMP_DIR)/libmc.a

ARCHIVE = mc

XS6_OBJECTS = \
	$(TMP_DIR)/xs_dtoa.o \
	$(TMP_DIR)/xs6pcre.o \
	$(TMP_DIR)/xs6Common.o \
	$(TMP_DIR)/xs6All.o \
	$(TMP_DIR)/xs6Debug.o \
	$(TMP_DIR)/xs6Memory.o \
	$(TMP_DIR)/xs6Symbol.o \
	$(TMP_DIR)/xs6Type.o \
	$(TMP_DIR)/xs6Property.o \
	$(TMP_DIR)/xs6Global.o \
	$(TMP_DIR)/xs6Object.o \
	$(TMP_DIR)/xs6Function.o \
	$(TMP_DIR)/xs6Array.o \
	$(TMP_DIR)/xs6String.o \
	$(TMP_DIR)/xs6Boolean.o \
	$(TMP_DIR)/xs6Number.o \
	$(TMP_DIR)/xs6Math.o \
	$(TMP_DIR)/xs6Date.o \
	$(TMP_DIR)/xs6RegExp.o \
	$(TMP_DIR)/xs6Error.o \
	$(TMP_DIR)/xs6JSON.o \
	$(TMP_DIR)/xs6DataView.o \
	$(TMP_DIR)/xs6API.o \
	$(TMP_DIR)/xs6Profile.o \
	$(TMP_DIR)/xs6Run.o \
	$(TMP_DIR)/xs6Generator.o \
	$(TMP_DIR)/xs6Module.o \
	$(TMP_DIR)/xs6Promise.o \
	$(TMP_DIR)/xs6Proxy.o \
	$(TMP_DIR)/xs6MapSet.o \
	$(TMP_DIR)/xs6Marshall.o \
	$(TMP_DIR)/xsr6.o

ifdef XS_COMPILER
XS6_OBJECTS += \
	$(TMP_DIR)/xs6Script.o \
	$(TMP_DIR)/xs6Lexical.o \
	$(TMP_DIR)/xs6Syntaxical.o \
	$(TMP_DIR)/xs6Tree.o \
	$(TMP_DIR)/xs6SourceMap.o \
	$(TMP_DIR)/xs6Scope.o \
	$(TMP_DIR)/xs6Code.o
endif

XS6_TOOL_OBJECTS = \
	$(TMP_DIR)/xs6Host.o \
	$(TMP_DIR)/xs6Platform.o

MC_OBJECTS = \
	$(TMP_DIR)/xs_patch.o \
	$(TMP_DIR)/mc_event.o \
	$(TMP_DIR)/mc_env.o \
	$(TMP_DIR)/mc_file.o \
	$(TMP_DIR)/mc_ipc.o \
	$(TMP_DIR)/mc_misc.o \
	$(TMP_DIR)/mc_stdio.o \
	$(TMP_DIR)/mc_xs.o

.PHONY: xsr6 mc modules extensions external proprietary tools
.SUFFIXES: .update

all: $(TMP_DIR) $(BIN_DIR) $(MOD_DIR) $(DEST_DIR) xsr6 mc

xsr6: $(BIN_DIR)/xsr6

mc: $(DEST_DIR)/application.xsb $(DEST_DIR)/inetd.xsb $(DEST_DIR)/launcher.xsb $(DEST_DIR)/config.xsb $(DEST_DIR)/synctime.xsb $(LIBMC) modules extensions external proprietary tools $(MOD_DIR)/$(ARCHIVE).xsa $(MOD_DIR)/$(ARCHIVE).so host_fs

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(MOD_DIR):
	mkdir -p $(MOD_DIR)
$(DEST_DIR):
	mkdir -p $(DEST_DIR)
$(MOD_DIR)/$(ARCHIVE).xsa: $(TMP_DIR)/$(ARCHIVE).xsa
	cp -p $(TMP_DIR)/$(ARCHIVE).xsa $@
$(MOD_DIR)/$(ARCHIVE).so: $(TMP_DIR)/$(ARCHIVE).so
	cp -p $(TMP_DIR)/$(ARCHIVE).so $@
$(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xs.c: $(TMP_DIR)/.update
	rm -f $(TMP_DIR)/$(ARCHIVE).xsa
	$(XS6_TOOL_DIR)/xsl6 -a $(ARCHIVE) -o $(TMP_DIR) -b $(DEST_DIR) -r 97 `find $(DEST_DIR) -name '*.xsb' -print`
	mv -f $(DEST_DIR)/$(ARCHIVE).xs.[ch] $(TMP_DIR)
$(TMP_DIR)/$(ARCHIVE).so: $(TMP_DIR)/$(LIBMODULE) $(LIBMC) $(TMP_DIR)/$(ARCHIVE).xs.c
	$(CC) $(C_OPTIONS) $(MOD_LINK_OPTIONS) $(TMP_DIR)/$(ARCHIVE).xs.c -o $@ $(TMP_DIR)/$(LIBMODULE) $(LIBMC)
$(LIBMC): $(MC_OBJECTS)
	rm -f $@
	ar cr $@ $(MC_OBJECTS)
$(BIN_DIR)/xsr6: $(XS6_OBJECTS) $(XS6_TOOL_OBJECTS)
	$(CC) $(LINK_OPTIONS) $(LIBRARIES) $(XS6_OBJECTS) $(XS6_TOOL_OBJECTS) -o $@

modules extensions external proprietary tools:
	(if [ -d $@ ]; then cd $@; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i $(mod_target); done; fi)

host_fs:
	mkdir -p ~/tmp/mc
	cp -fp data/* ~/tmp/mc
	if [ -d proprietary/data ]; then cp -fp proprietary/data/* ~/tmp/mc; fi

$(MC_OBJECTS): $(TMP_DIR)/%.o: $(XS6_MC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(XS6_OBJECTS): $(TMP_DIR)/%.o: $(XS6_SRC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -DFSK_PCRE=1 -I$(XS6_INC_DIR) -I$(XS6_SRC_DIR) -I$(XS6_SRC_DIR)/tool -I$(XS6_SRC_DIR)/pcre -c -o $@
$(XS6_TOOL_OBJECTS): $(TMP_DIR)/%.o: $(XS6_SRC_DIR)/tool/%.c
	$(CC) $< $(C_OPTIONS) -I$(XS6_INC_DIR) -I$(XS6_SRC_DIR) -I$(XS6_SRC_DIR)/tool -c -o $@

$(DEST_DIR)/%.xsb: $(TMP_DIR)/%.xsb
	cp -p $< $@
	touch $(TMP_DIR)/.update
$(TMP_DIR)/%.xsb: %.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -o $(TMP_DIR) $<

%.update:;

clean:
	rm -f $(MC_OBJECTS) $(XS6_OBJECTS) $(XS6_TOOL_OBJECTS) $(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xs.[ch] $(TMP_DIR)/application.xsb $(TMP_DIR)/inetd.xsb $(TMP_DIR)/launcher.xsb $(TMP_DIR)/config.xsb $(TMP_DIR)/synctime.xsb $(DEST_DIR)/*.xsb
	(for dir in modules extensions external proprietary tools; do if [ -d $$dir ]; then (cd $$dir; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i clean; done); fi; done)

