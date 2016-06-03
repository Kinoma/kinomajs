C_OPTIONS = -c -arch i386 -DXS_ARCHIVE=1 -DmxRun=1 -DmxParse=1
ifeq ($(DEBUG),)
	C_OPTIONS += -D_RELEASE=1 -O3
else
	C_OPTIONS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_OPTIONS += -DMC_MEMORY_DEBUG=1
endif
LINK_OPTIONS = -arch i386 -dynamiclib -flat_namespace -undefined suppress
XSC = $(XS_DIR)/bin/mac/release/xsc6
XSL = $(XS_DIR)/bin/mac/release/xsl6
		
MC_OBJECTS = \
	$(TMP_DIR)/xs_patch.o \
	$(TMP_DIR)/mc_event.o \
	$(TMP_DIR)/mc_env.o \
	$(TMP_DIR)/mc_file.o \
	$(TMP_DIR)/mc_ipc.o \
	$(TMP_DIR)/mc_misc.o \
	$(TMP_DIR)/mc_stdio.o \
	$(TMP_DIR)/mc_xs.o
		
all: $(BIN_DIR)/mc.so host_fs
	
$(BIN_DIR)/mc.so: $(TMP_DIR)/mc.xs.o $(MC_OBJECTS) $(OBJECTS)
	$(CC) $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -o $@
	
$(TMP_DIR)/mc.xs.o: $(MOD_DIR)/mc.xs.c
	$(CC) $< $(C_OPTIONS) $(C_INCLUDES) -o $@
	
$(MOD_DIR)/mc.xs.c: $(MODULES)
	$(XSL) -a mc -b $(MOD_DIR) -o $(BIN_DIR) -r 97 $^

$(TMP_DIR)/mc_file.o: $(TMP_DIR)/mc_mapped_files.h
$(TMP_DIR)/mc_mapped_files.h: $(MC_DIR)/rodata
	sh $(MC_DIR)/tools/mkmap.sh $@
	
$(MC_OBJECTS): $(TMP_DIR)/%.o: $(MC_DIR)/%.c
	$(CC) $< $(C_OPTIONS) $(C_INCLUDES) -c -o $@

host_fs:
	mkdir -p ~/tmp/mc
	cp -fp $(MC_DIR)/data/* ~/tmp/mc
	if [ -d $(MC_DIR)/proprietary/data ]; then cp -fp $(MC_DIR)/proprietary/data/* ~/tmp/mc; fi
