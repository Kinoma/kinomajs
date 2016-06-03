TARGET_SYSTEM = mc

CROSS_COMPILE ?= arm-none-eabi
AS    := $(CROSS_COMPILE)-as
CC    := $(CROSS_COMPILE)-gcc
LD    := $(CROSS_COMPILE)-ld
AR    := $(CROSS_COMPILE)-ar
STRIP := $(CROSS_COMPILE)-strip
XSC = $(XS_DIR)/bin/mac/release/xsc6
XSL = $(XS_DIR)/bin/mac/release/xsl6

SDK_PATH = $(MC_DIR)/wmsdk
#SDK_PATH = $(F_HOME)/build/devices/wm/wmsdk_bundle-3.2.12/bin/wmsdk
WMSDK_VERSION = 3002012

include $(SDK_PATH)/.config

PROGRAM = xsr6_mc

C_OPTIONS = -DmxParse=1 -DmxRun=1 -DmxMC=1 -DmxNoFunctionLength -DmxNoFunctionName -DWMSDK_VERSION=$(WMSDK_VERSION) -DXIP=1 -DXS_ARCHIVE=1 -DK5=1
AS_OPTIONS = -I $(MC_DIR)
ifeq ($(DEBUG),)
	C_OPTIONS += -D_RELEASE=1 -O2
else
	C_OPTIONS += -D_DEBUG=1 -DmxDebug=1 -DmxReport=0 -DmxHostReport=0 -DmxStress=1 -Os -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_OPTIONS += -DMC_MEMORY_DEBUG=1
endif
# ??? C_OPTIONS += -fPIC

include common.mk

LDSCRIPT = $(PROGRAM)-xip.ld
LINK_OPTIONS = -T $(LDSCRIPT) -nostartfiles -Xlinker -M -Xlinker -Map=$(TMP_DIR)/$(PROGRAM).map -Xlinker --gc-section
ifeq ($(CONFIG_CPU_MW300), y)
	ifeq ($(CONFIG_ENABLE_ROM_LIBS), y)
		LINK_OPTIONS += -Xlinker --defsym=_rom_data=64
	else
		LINK_OPTIONS += -Xlinker --defsym=_rom_data=0
	endif
endif
LIBRARIES = -L$(BIN_DIR) -lstubs -lext

XS_OBJECTS = \
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
	$(TMP_DIR)/xs6Script.o \
	$(TMP_DIR)/xs6Lexical.o \
	$(TMP_DIR)/xs6Syntaxical.o \
	$(TMP_DIR)/xs6Tree.o \
	$(TMP_DIR)/xs6SourceMap.o \
	$(TMP_DIR)/xs6Scope.o \
	$(TMP_DIR)/xs6Code.o

MC_OBJECTS = \
	$(TMP_DIR)/xs6Host.o \
	$(TMP_DIR)/xs6Platform.o \
	$(TMP_DIR)/xsr6_mc.o \
	$(TMP_DIR)/xs_patch.o \
	$(TMP_DIR)/mc_event.o \
	$(TMP_DIR)/mc_memory.o \
	$(TMP_DIR)/mc_dl.o \
	$(TMP_DIR)/mc_elf.o \
	$(TMP_DIR)/mc_env.o \
	$(TMP_DIR)/mc_file.o \
	$(TMP_DIR)/mc_ipc.o \
	$(TMP_DIR)/mc_misc.o \
	$(TMP_DIR)/mc_stdio.o \
	$(TMP_DIR)/mc_time.o \
	$(TMP_DIR)/mc_usb.o \
	$(TMP_DIR)/mc_xs.o \
	$(TMP_DIR)/heap_4.o \
	$(TMP_DIR)/mw300_rd.o

all: $(BIN_DIR)/libext.a $(BIN_DIR)/libstubs.a $(BIN_DIR)/layout.txt $(BIN_DIR)/flash_k5.config ftfs

$(BIN_DIR)/libext.a: $(MC_DIR)/ext_stubs.s
	rm -f $@
	$(AS) $(AS_OPTIONS) -c -o $(TMP_DIR)/`basename $< .s`.o $<
	$(AR) cr $@ $(TMP_DIR)/`basename $< .s`.o

$(BIN_DIR)/libstubs.a: $(MC_DIR)/ext_stubs.sym
	rm -fr $(TMP_DIR)/stubs $@
	mkdir -p $(TMP_DIR)/stubs
	awk '{fname="$(TMP_DIR)/stubs/" $$0 ".s"; printf(".include \"defstub.i\"\n\tdefstub\t%s,\t%d\n", $$0, NR) > fname; close(fname)}' $<
	(cd $(TMP_DIR)/stubs; for i in *.s; do $(AS) $(AS_OPTIONS) -c -o `basename $$i .s`.o $$i; done; $(AR) cr $@ *.o)

$(BIN_DIR)/layout.txt: flash/layout.txt
	cp -p $< $@

$(BIN_DIR)/flash_k5.config: flash/config.temp $(BIN_DIR)/boot2.bin $(BIN_DIR)/mw30x_uapsta.bin $(BIN_DIR)/$(PROGRAM)_k5.bin
	sed -e 's:$$(SDK_PATH):$(SDK_PATH):' -e 's:$$(BIN_DIR):$(BIN_DIR):' -e 's:$k5:k5:' $< > $@

$(BIN_DIR)/boot2.bin: flash/boot2.bin
	cp -p $< $@
	
$(BIN_DIR)/mw30x_uapsta.bin: flash/mw30x_uapsta.bin
	cp -p $< $@

$(BIN_DIR)/$(PROGRAM)_k5.bin: $(BIN_DIR)/$(PROGRAM)_k5.axf
	$(SDK_PATH)/tools/bin/$(SDK_PLATFORM)/axf2firmware $< $@

$(BIN_DIR)/$(PROGRAM)_k5.axf: $(TMP_DIR)/mc.xs.o $(XS_OBJECTS) $(MC_OBJECTS) $(OBJECTS) $(LDSCRIPT)
	$(CC) $(LINK_OPTIONS) -Xlinker --start-group $(TMP_DIR)/mc.xs.o $(XS_OBJECTS) $(MC_OBJECTS) $(OBJECTS) $(LIBRARIES) -Xlinker --end-group -o $@ -lm

$(TMP_DIR)/mc.xs.o: $(MOD_DIR)/mc.xs.c
	$(CC) $< $(C_OPTIONS) $(C_INCLUDES) -o $@
$(MOD_DIR)/mc.xs.c: $(MODULES)
	$(XSL) -a mc -b $(MOD_DIR) -o $(TMP_DIR) -r 97 $^

$(TMP_DIR)/mc_dl.o: $(TMP_DIR)/ext_stubs.sym.h
$(TMP_DIR)/ext_stubs.sym.h: ext_stubs.sym
	awk '\
BEGIN	{printf("void *const ext_stubs[] = {\n(void *)&_mc_global,\n")} \
	{printf("(void *)%s,\n", $$0)} \
END	{printf("};\n")}' $< > $@

$(TMP_DIR)/mc_file.o: $(TMP_DIR)/mc_mapped_files.h
$(TMP_DIR)/mc_mapped_files.h: rodata
	sh tools/mkmap.sh $@

$(TMP_DIR)/xs6Host.o: $(TMP_DIR)/mc.xsa.h
$(TMP_DIR)/mc.xsa.h: $(MOD_DIR)/mc.xs.c
	(echo "static const unsigned char mc_xsa[] = { "; xxd -i < $(TMP_DIR)/mc.xsa; echo "};") > $(TMP_DIR)/mc.xsa.h

$(XS_OBJECTS): $(TMP_DIR)/%.o: $(XS_DIR)/sources/%.c
	$(CC) $< $(C_OPTIONS) $(C_INCLUDES) -c -o $@
$(MC_OBJECTS): $(TMP_DIR)/%.o: $(MC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) $(C_INCLUDES) -c -o $@

ftfs:
	mkdir -p $(TMP_DIR)/fs
	cp -fp $(MC_DIR)/data/* $(TMP_DIR)/fs
	if [ -d $(MC_DIR)/proprietary/data ]; then cp -fp $(MC_DIR)/proprietary/data/* $(TMP_DIR)/fs; fi
	(cd $(TMP_DIR); $(SDK_PATH)/tools/bin/flash_pack.py 1 $(PROGRAM).ftfs fs)
	cp -pf $(TMP_DIR)/$(PROGRAM).ftfs $(BIN_DIR)/$(PROGRAM).ftfs

