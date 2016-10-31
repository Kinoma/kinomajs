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

export TARGET_SYSTEM = mc

CROSS_COMPILE ?= arm-none-eabi
export AS    := $(CROSS_COMPILE)-as
export CC    := $(CROSS_COMPILE)-gcc
export LD    := $(CROSS_COMPILE)-ld
export AR    := $(CROSS_COMPILE)-ar
export STRIP := $(CROSS_COMPILE)-strip

export SDK_PATH = $(XS6_MC_DIR)/wmsdk
#export SDK_PATH = $(F_HOME)/build/devices/wm/wmsdk_bundle-3.2.12/bin/wmsdk
ifdef AWS_WMSDK
	export AWS_SDK_PATH = $(AWS_WMSDK)/wmsdk
	export SDK_INC_PATH = $(AWS_SDK_PATH)/src/incl
	export SDK_EXTERNAL_PATH = $(AWS_SDK_PATH)/external
	export FREERTOS_INC_PATH = $(SDK_EXTERNAL_PATH)/freertos
	export LWIP_INC_PATH = $(SDK_EXTERNAL_PATH)/lwip/src/include
	WMLIB=
	# WM_TOOL_DIR=$(AWS_SDK_PATH)/tools	# doesn't work!!!
	WM_TOOL_DIR=$(XS6_MC_DIR)/wmlib/tools
	include $(XS6_MC_DIR)/wmlib/.config
	export AUTOCONF = $(XS6_MC_DIR)/wmlib/autoconf.h
	FLASH_FILES=
else
	export SDK_INC_PATH = $(SDK_PATH)/incl
	export SDK_EXTERNAL_PATH = $(SDK_INC_PATH)
	export FREERTOS_INC_PATH = $(SDK_INC_PATH)/freertos
	export LWIP_INC_PATH = $(SDK_INC_PATH)/lwip
	WMLIB=$(XS6_MC_DIR)/wmlib/libmc_wm.a
	WM_TOOL_DIR=$(SDK_PATH)/tools
	include $(SDK_PATH)/.config
	export AUTOCONF = $(SDK_PATH)/incl/autoconf.h
	FLASH_FILES=$(BIN_DIR)/flash_$(BOARD).config $(BIN_DIR)/layout.txt $(BIN_DIR)/mw30x_uapsta.bin $(BIN_DIR)/boot2.bin
endif
export USB_DRIVER_PATH = $(F_HOME)/libraries/kdriver

export WMSDK_VERSION = 3002012
export XIP = 1
export XS_COMPILER = 1
export FTFS = 1


PROGRAM = xsr6_mc

ifeq ($(XS_ARCHIVE), 1)
	XSC_OPTIONS = -e
	mod_target = archive
else
	XSC_OPTIONS =
	mod_target = all
endif

ifdef K5
BOARD = k5
else
BOARD = rd
endif

ifeq ($(DEBUG),)
	BIN_DIR = $(XS6)/bin/mc/release
	TMP_DIR = $(XS6)/tmp/mc/release/xsr6_$(BOARD)
	XSC_OPTIONS +=
else
	BIN_DIR = $(XS6)/bin/mc/debug
	TMP_DIR = $(XS6)/tmp/mc/debug/xsr6_$(BOARD)
#	XSC_OPTIONS += -d
# build archive without the debug option
#	XSC_OPTIONS +=
endif

ifeq ($(FTFS), 1)
	RODATA = rodata
else
	RODATA = $(TMP_DIR)/fs
endif

include common.mk

export BIN_DIR
export TMP_DIR
export DEST_DIR = $(TMP_DIR)/modules
export XSC_OPTIONS

ifeq ($(XIP), 1)
LDSCRIPT = $(PROGRAM)-xip.ld
else
LDSCRIPT = $(PROGRAM).ld
endif

LINK_OPTIONS = -T $(LDSCRIPT) -nostartfiles -Xlinker -M -Xlinker -Map=$(TMP_DIR)/$(PROGRAM).map -Xlinker --gc-section
ifeq ($(CONFIG_CPU_MW300), y)
	ifeq ($(CONFIG_ENABLE_ROM_LIBS), y)
		LINK_OPTIONS += -Xlinker --defsym=_rom_data=64
	else
		LINK_OPTIONS += -Xlinker --defsym=_rom_data=0
	endif
endif
export MOD_C_OPTIONS += -fPIC -DMC_MODULE=1
export MOD_LINK_OPTIONS = -T $(XS6_MC_DIR)/module.ld -pie -e $(ENTRY) -Xlinker -M -Xlinker -Map=$(TMP_DIR)/$(MODULE).map -Xlinker --gc-section -mthumb -flto -ffat-lto-objects
export MOD_LIBRARIES = -L$(BIN_DIR) -lstubs -lext

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
	$(TMP_DIR)/xs6Marshall.o

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

XS6_MC_OBJECTS = \
	$(TMP_DIR)/xs6Host.o \
	$(TMP_DIR)/xs6Platform.o

MC_OBJECTS = \
	$(TMP_DIR)/xsr6_mc.o \
	$(TMP_DIR)/xs_patch.o \
	$(TMP_DIR)/mc_event.o \
	$(TMP_DIR)/mc_memory.o \
	$(TMP_DIR)/mc_dl.o \
	$(TMP_DIR)/mc_elf.o \
	$(TMP_DIR)/mc_env.o \
	$(TMP_DIR)/mc_file.o \
	$(TMP_DIR)/mc_ffs.o \
	$(TMP_DIR)/mc_ipc.o \
	$(TMP_DIR)/mc_misc.o \
	$(TMP_DIR)/mc_stdio.o \
	$(TMP_DIR)/mc_time.o \
	$(TMP_DIR)/mc_usb.o \
	$(TMP_DIR)/mc_xs.o \
	$(TMP_DIR)/heap_4.o

USB_DRIVER_OBJECTS = \
	$(TMP_DIR)/ringbuf.o \
	$(TMP_DIR)/cdc.o \
	$(TMP_DIR)/dcd.o \
	$(TMP_DIR)/device.o \
	$(TMP_DIR)/rsrc.o

ifeq ($(XS_ARCHIVE), 1)
GEN_OBJECTS += $(TMP_DIR)/$(ARCHIVE).xs.o
GEN_FILES += $(TMP_DIR)/$(ARCHIVE).xs.c $(TMP_DIR)/$(ARCHIVE).xs.h $(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xsa.h $(TMP_DIR)/mc_mapped_files.h
archive = archive
endif

OBJECTS = $(XS6_OBJECTS) $(XS6_MC_OBJECTS) $(MC_OBJECTS) $(USB_DRIVER_OBJECTS) $(GEN_OBJECTS)

BINARIES = $(DEST_DIR)/application.xsb $(DEST_DIR)/inetd.xsb $(DEST_DIR)/launcher.xsb $(DEST_DIR)/config.xsb $(DEST_DIR)/synctime.xsb

export LIBMODULE = libmodule.a
ifeq ($(XIP), 1)
EXTRA_LIBS = $(TMP_DIR)/$(LIBMODULE)
endif

LIBRARIES = $(wildcard $(XS6_MC_DIR)/wmlib/*.a)
MC_WMSDK_LIB = $(TMP_DIR)/libmc_wmsdk.a
WMSDK_LIBS = $(addprefix $(SDK_PATH)/libs/, libc.a libdhcpd.a libdrv.a libftfs.a libmdev.a libnet.a libos.a libpart.a libpwrmgr.a libutil.a libwlcmgr.a libwmstdio.a libwifidriver.a libwmtime.a)

SIGNED_MODULES = setup/_download

.PHONY: lib modules extensions archive ftfs external proprietary
.SUFFIXES: .config .temp .update

all: $(TMP_DIR) $(BIN_DIR) $(DEST_DIR) $(BINARIES) ftfs $(BIN_DIR)/libext.a $(BIN_DIR)/libstubs.a modules extensions external proprietary $(BIN_DIR)/$(PROGRAM)_$(BOARD).bin $(FLASH_FILES)

ifndef AWS_WMSDK
lib: $(WMLIB)

$(MC_WMSDK_LIB):
	(cd wmlib; make -I $(XS6_MC_DIR) -f mc_wmsdk.make)

$(WMLIB): $(MC_WMSDK_LIB) $(WMSDK_LIBS)
	mkdir -p $(TMP_DIR)/wmsdk
	(cd $(TMP_DIR)/wmsdk; for i in $(WMSDK_LIBS); do $(AR) x $$i; done; $(AR) x $(MC_WMSDK_LIB); $(AR) cr $@ *.o)

$(XS6_MC_DIR)/wmlib/libfreertos.a: $(SDK_PATH)/libs/libfreertos.a
	-cp -p $*.a $@
$(XS6_MC_DIR)/wmlib/liblwip.a: $(SDK_PATH)/libs/liblwip.a
	-cp -p $*.a $@
endif

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(DEST_DIR):
	mkdir -p $(DEST_DIR)

$(BIN_DIR)/libext.a: $(XS6_MC_DIR)/ext_stubs.s
	rm -f $@
	$(AS) $(AS_OPTIONS) -c -o $(TMP_DIR)/`basename $< .s`.o $<
	$(AR) cr $@ $(TMP_DIR)/`basename $< .s`.o

$(BIN_DIR)/libstubs.a: $(XS6_MC_DIR)/ext_stubs.sym
	rm -fr $(TMP_DIR)/stubs $@
	mkdir -p $(TMP_DIR)/stubs
	awk '{fname="$(TMP_DIR)/stubs/" $$0 ".s"; printf(".include \"defstub.i\"\n\tdefstub\t%s,\t%d\n", $$0, NR) > fname; close(fname)}' $<
	(cd $(TMP_DIR)/stubs; for i in *.s; do $(AS) $(AS_OPTIONS) -c -o `basename $$i .s`.o $$i; done; $(AR) cr $@ *.o)

$(TMP_DIR)/$(ARCHIVE).xsa.h: $(TMP_DIR)/$(ARCHIVE).xsa
	(echo "static const unsigned char mc_xsa[] = { "; xxd -i < $(TMP_DIR)/$(ARCHIVE).xsa; echo "};") > $@

$(TMP_DIR)/$(ARCHIVE).xsa $(TMP_DIR)/$(ARCHIVE).xs.c: $(TMP_DIR)/.update
	rm -f $(TMP_DIR)/$(ARCHIVE).xsa
	$(XS6_TOOL_DIR)/xsl6 -a $(ARCHIVE) -o $(TMP_DIR) -b $(DEST_DIR) -r 97 `find $(DEST_DIR) -name '*.xsb' -print`
	mv -f $(DEST_DIR)/$(ARCHIVE).xs.[ch] $(TMP_DIR)

modules extensions external proprietary:
	(if [ -d $@ ]; then cd $@; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i $(mod_target); done; fi)

ftfs:
	mkdir -p $(TMP_DIR)/fs
	cp -fp $(XS6_MC_DIR)/data/* $(TMP_DIR)/fs
	if [ -d $(XS6_MC_DIR)/proprietary/data ]; then cp -fp $(XS6_MC_DIR)/proprietary/data/* $(TMP_DIR)/fs; fi
ifneq ($(XS_ARCHIVE), 1)
	cp -fp $(DEST_DIR)/* $(TMP_DIR)/fs
endif
ifndef AWS_WMSDK
	(cd $(TMP_DIR); $(SDK_PATH)/tools/bin/flash_pack.py 1 $(PROGRAM).ftfs fs)
	cp -pf $(TMP_DIR)/$(PROGRAM).ftfs $(BIN_DIR)/$(PROGRAM).ftfs
endif

$(TMP_DIR)/mc_mapped_files.h: $(RODATA)
	sh tools/mkmap.sh $(RODATA) $@

$(BIN_DIR)/flash_$(BOARD).config: flash/config.temp
	sed -e 's:$$(SDK_PATH):$(SDK_PATH):' -e 's:$$(BIN_DIR):$(BIN_DIR):' -e 's:$$(BOARD):$(BOARD):' $< > $@
$(BIN_DIR)/layout.txt: flash/layout.txt
	cp -p $< $@
$(BIN_DIR)/mw30x_uapsta.bin: flash/mw30x_uapsta.bin
	cp -p $< $@
$(BIN_DIR)/boot2.bin: flash/boot2.bin
	cp -p $< $@

$(XS6_OBJECTS) $(XS6_MC_OBJECTS): $(XS6_SRC_DIR)/mc/xs6Platform.h $(XS6_SRC_DIR)/xs6Common.h $(XS6_SRC_DIR)/xs6All.h $(XS6_SRC_DIR)/xs6Script.h
$(TMP_DIR)/xs6Host.o: $(TMP_DIR)/$(ARCHIVE).xsa.h $(TMP_DIR)/$(ARCHIVE).xs.c
$(TMP_DIR)/mc_dl.o: $(TMP_DIR)/ext_stubs.sym.h
$(TMP_DIR)/mc_file.o: $(TMP_DIR)/mc_mapped_files.h
$(XS6_OBJECTS): $(TMP_DIR)/%.o: $(XS6_SRC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(XS6_MC_OBJECTS): $(TMP_DIR)/%.o: $(XS6_SRC_DIR)/mc/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(MC_OBJECTS): $(TMP_DIR)/%.o: $(XS6_MC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(GEN_OBJECTS): $(TMP_DIR)/%.o: $(TMP_DIR)/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(TMP_DIR)/ringbuf.o: $(TMP_DIR)/%.o: $(USB_DRIVER_PATH)/src/%.c
	$(CC) $< $(C_OPTIONS) -c -o $@
$(TMP_DIR)/cdc.o $(TMP_DIR)/dcd.o $(TMP_DIR)/device.o $(TMP_DIR)/rsrc.o: $(TMP_DIR)/%.o: $(USB_DRIVER_PATH)/src/usb/%.c
	$(CC) $< -I$(USB_DRIVER_PATH)/include/usb -I$(USB_DRIVER_PATH)/Device/Marvell/88MW300/Include $(C_OPTIONS) -c -o $@

$(BIN_DIR)/$(PROGRAM)_$(BOARD).bin: $(BIN_DIR)/$(PROGRAM)_$(BOARD).axf
	$(WM_TOOL_DIR)/bin/$(SDK_PLATFORM)/axf2firmware $< $@

$(BIN_DIR)/$(PROGRAM)_$(BOARD).axf: $(OBJECTS) $(EXTRA_LIBS) $(LDSCRIPT) $(LIBRARIES)
	$(CC) $(C_OPTIONS) $(LINK_OPTIONS) -Xlinker --start-group $(OBJECTS) $(LIBRARIES) $(EXTRA_LIBS) -Xlinker --end-group -o $@ -lm

$(DEST_DIR)/%.xsb: $(TMP_DIR)/%.xsb
	cp -p $< $@
	touch $(TMP_DIR)/.update
$(TMP_DIR)/%.xsb: %.js
	$(XS6_TOOL_DIR)/xsc6 $(XSC_OPTIONS) -o $(TMP_DIR) $<

$(TMP_DIR)/ext_stubs.sym.h: ext_stubs.sym
	awk '\
BEGIN	{printf("void *const ext_stubs[] = {\n(void *)&_mc_global,\n")} \
	{printf("(void *)%s,\n", $$0)} \
END	{printf("};\n")}' $< > $@

%.temp:;

%.update:;

sign:
	$(XS6_TOOL_DIR)/xsr6 -a $(XS6_TOOL_DIR)/modules/$(ARCHIVE).xsa xssign -l $(TMP_DIR)/$(ARCHIVE).xsa $(SIGNED_MODULES)
	cp -pf ${HOME}/tmp/mc/k2/mc.xsa.sig $(XS6_MC_DIR)/data

clean:
	rm -f $(OBJECTS) $(GEN_FILES) $(TMP_DIR)/ext_stubs.sym.h $(TMP_DIR)/ext_stubs.o $(TMP_DIR)/$(LIBMODULE) $(TMP_DIR)/stubs/*.[so] $(addsuffix .d, $(basename $(OBJECTS))) $(TMP_DIR)/application.xsb $(TMP_DIR)/inetd.xsb $(TMP_DIR)/launcher.xsb $(TMP_DIR)/synctime.xsb
	(for dir in modules extensions external proprietary; do if [ -d $$dir ]; then (cd $$dir; for i in *.make; do make -I $(XS6_MC_DIR) -f $$i clean; done); fi; done)
