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
% : %.c
%.o : %.c

export TARGET_SYSTEM = mac

C_OPTIONS = -arch i386 -mmacosx-version-min=10.7 -DmxRun=1 -DmxDebug=1 -g -Wall -I$(XS6_INC_DIR) -I$(XS6_SRC_DIR) -I$(XS6_SRC_DIR)/tool -I$(XS6_MC_DIR) -I$(XS6_MC_DIR)/extensions/crypt -DXS_ARCHIVE=$(XS_ARCHIVE)

ifeq ($(XS_ARCHIVE), 1)
	XSC_OPTIONS = -e
	mod_target = archive
else
	XSC_OPTIONS =
	mod_target = all
endif

ifeq ($(DEBUG),)
	BIN_DIR = $(XS6)/bin/mac/release/modules
	TMP_DIR = $(XS6)/tmp/mac/release/mc
	C_OPTIONS += -D_RELEASE=1 -O3
else
	BIN_DIR = $(XS6)/bin/mac/debug/modules
	TMP_DIR = $(XS6)/tmp/mac/debug/mc
	C_OPTIONS += -D_DEBUG=1 -DmxDebug=1 -DmxReport=1
	XSC_OPTIONS += -d
endif
C_OPTIONS += -I$(TMP_DIR)
# C_OPTIONS += -DUSE_NATIVE_STDIO

export DEST_DIR = $(TMP_DIR)/modules
export BIN_DIR
export TMP_DIR
export XSC_OPTIONS
export C_OPTIONS
export MOD_LINK_OPTIONS = -dynamiclib -flat_namespace -undefined suppress
export MOD_LIBRARIES =  -L$(TMP_DIR_HOST) -lmc -framework CoreServices -lSystem

export LIBMODULE = libmodule.a
LIBMC = $(TMP_DIR)/libmc.a

ARCHIVE = mc

OBJECTS = \
	$(TMP_DIR)/xs_patch.o \
	$(TMP_DIR)/mc_event.o \
	$(TMP_DIR)/mc_env.o \
	$(TMP_DIR)/mc_file.o \
	$(TMP_DIR)/mc_ipc.o \
	$(TMP_DIR)/mc_misc.o \
	$(TMP_DIR)/mc_stdio.o \
	$(TMP_DIR)/mc_xs.o

.PHONY: modules extensions tools
.SUFFIXES: .update

all: $(TMP_DIR) $(BIN_DIR) $(DEST_DIR) $(DEST_DIR)/application.xsb $(DEST_DIR)/inetd.xsb $(DEST_DIR)/launcher.xsb $(LIBMC) modules extensions tools $(BIN_DIR)/$(ARCHIVE).xsa $(BIN_DIR)/$(ARCHIVE).so host_fs

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(DEST_DIR):
	mkdir -p $(DEST_DIR)
$(BIN_DIR)/$(ARCHIVE).xsa: $(TMP_DIR)/$(ARCHIVE).xsa
	cp -p $(TMP_DIR)/$(ARCHIVE).xsa $@
$(BIN_DIR)/$(ARCHIVE).so: $(TMP_DIR)/$(ARCHIVE).so
	cp -p $(TMP_DIR)/$(ARCHIVE).so $@
$(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xs.c: $(TMP_DIR)/.update
	rm -f $(TMP_DIR)/$(ARCHIVE).xsa
	$(XS6_TOOL_DIR)/xsl6 -a $(ARCHIVE) -o $(TMP_DIR) -b $(DEST_DIR) -r 97 `find $(DEST_DIR) -name '*.xsb' -print`
	mv -f $(DEST_DIR)/$(ARCHIVE).xs.[ch] $(TMP_DIR)
$(TMP_DIR)/$(ARCHIVE).so: $(TMP_DIR)/$(LIBMODULE) $(LIBMC) $(TMP_DIR)/$(ARCHIVE).xs.c
	$(CC) $(C_OPTIONS) $(MOD_LINK_OPTIONS) $(TMP_DIR)/$(ARCHIVE).xs.c -o $@ $(TMP_DIR)/$(LIBMODULE) $(LIBMC)
$(LIBMC): $(OBJECTS)
	rm -f $@
	ar cr $@ $(OBJECTS)

modules extensions tools:
	(cd $@; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i $(mod_target); done)

host_fs:
	mkdir -p ~/tmp/mc
	cp -p data/* ~/tmp/mc

$(OBJECTS): $(TMP_DIR)/%.o: $(XS6_MC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@

$(DEST_DIR)/%.xsb: $(TMP_DIR)/%.xsb
	cp -p $< $@
	touch $(TMP_DIR)/.update
$(TMP_DIR)/%.xsb: %.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -o $(TMP_DIR) $<

%.update:;

clean:
	rm -f $(OBJECTS) $(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xs.[ch] $(TMP_DIR)/application.xsb $(TMP_DIR)/inetd.xsb $(TMP_DIR)/launcher.xsb $(DEST_DIR)/*.xsb
	(cd modules; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i clean; done)
	(cd extensions; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i clean; done)
