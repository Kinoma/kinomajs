#
#     Copyright (C) 2002-2015 Kinoma, Inc.
#     
#     All rights reserved.
#
#
#
#
#
#
#
#
#
#
#
#

FREETYPE_VERSION = 2.6
FREETYPE_DIR = $(F_HOME)/libraries/freetype
FREETYPE_SRC_BASE = $(FREETYPE_DIR)/src
FREETYPE_SRC_DIR = $(FREETYPE_SRC_BASE)/freetype-$(FREETYPE_VERSION)

FREETYPE_OBJECTS_DIRS = \
	$(TMP_DIR)/freetype/base		\
	$(TMP_DIR)/freetype/truetype	\
	$(TMP_DIR)/freetype/type1		\
	$(TMP_DIR)/freetype/cff			\
	$(TMP_DIR)/freetype/cid			\
	$(TMP_DIR)/freetype/pfr			\
	$(TMP_DIR)/freetype/type42		\
	$(TMP_DIR)/freetype/winfonts	\
	$(TMP_DIR)/freetype/pcf			\
	$(TMP_DIR)/freetype/bdf			\
	$(TMP_DIR)/freetype/sfnt		\
	$(TMP_DIR)/freetype/autofit		\
	$(TMP_DIR)/freetype/pshinter	\
	$(TMP_DIR)/freetype/raster		\
	$(TMP_DIR)/freetype/smooth		\
	$(TMP_DIR)/freetype/cache		\
	$(TMP_DIR)/freetype/gzip		\
	$(TMP_DIR)/freetype/lzw			\
	$(TMP_DIR)/freetype/bzip2		\
	$(TMP_DIR)/freetype/psaux		\
	$(TMP_DIR)/freetype/psnames

FREETYPE_OBJECTS = \
	$(TMP_DIR)/freetype/base/ftsystem.o		\
	$(TMP_DIR)/freetype/base/ftdebug.o		\
	$(TMP_DIR)/freetype/base/ftinit.o		\
	$(TMP_DIR)/freetype/base/ftbbox.o		\
	$(TMP_DIR)/freetype/base/ftbitmap.o		\
	$(TMP_DIR)/freetype/base/ftcid.o		\
	$(TMP_DIR)/freetype/base/ftadvanc.o		\
	$(TMP_DIR)/freetype/base/ftcalc.o		\
	$(TMP_DIR)/freetype/base/ftdbgmem.o		\
	$(TMP_DIR)/freetype/base/ftgloadr.o		\
	$(TMP_DIR)/freetype/base/ftobjs.o		\
	$(TMP_DIR)/freetype/base/ftoutln.o		\
	$(TMP_DIR)/freetype/base/ftrfork.o		\
	$(TMP_DIR)/freetype/base/ftsnames.o		\
	$(TMP_DIR)/freetype/base/ftstream.o		\
	$(TMP_DIR)/freetype/base/fttrigon.o		\
	$(TMP_DIR)/freetype/base/ftutil.o		\
	$(TMP_DIR)/freetype/base/ftfstype.o		\
	$(TMP_DIR)/freetype/base/ftgasp.o		\
	$(TMP_DIR)/freetype/base/ftglyph.o		\
	$(TMP_DIR)/freetype/base/ftgxval.o		\
	$(TMP_DIR)/freetype/base/ftlcdfil.o		\
	$(TMP_DIR)/freetype/base/ftmm.o			\
	$(TMP_DIR)/freetype/base/ftotval.o		\
	$(TMP_DIR)/freetype/base/ftpatent.o		\
	$(TMP_DIR)/freetype/base/ftpfr.o		\
	$(TMP_DIR)/freetype/base/ftstroke.o		\
	$(TMP_DIR)/freetype/base/ftsynth.o		\
	$(TMP_DIR)/freetype/base/fttype1.o		\
	$(TMP_DIR)/freetype/base/ftwinfnt.o		\
	$(TMP_DIR)/freetype/truetype/truetype.o	\
	$(TMP_DIR)/freetype/type1/type1.o		\
	$(TMP_DIR)/freetype/cff/cff.o			\
	$(TMP_DIR)/freetype/cid/type1cid.o		\
	$(TMP_DIR)/freetype/pfr/pfr.o			\
	$(TMP_DIR)/freetype/type42/type42.o		\
	$(TMP_DIR)/freetype/winfonts/winfnt.o	\
	$(TMP_DIR)/freetype/pcf/pcf.o			\
	$(TMP_DIR)/freetype/bdf/bdf.o			\
	$(TMP_DIR)/freetype/sfnt/sfnt.o			\
	$(TMP_DIR)/freetype/autofit/autofit.o	\
	$(TMP_DIR)/freetype/pshinter/pshinter.o	\
	$(TMP_DIR)/freetype/raster/raster.o		\
	$(TMP_DIR)/freetype/smooth/smooth.o		\
	$(TMP_DIR)/freetype/cache/ftcache.o		\
	$(TMP_DIR)/freetype/gzip/ftgzip.o		\
	$(TMP_DIR)/freetype/lzw/ftlzw.o			\
	$(TMP_DIR)/freetype/bzip2/ftbzip2.o		\
	$(TMP_DIR)/freetype/psaux/psaux.o		\
	$(TMP_DIR)/freetype/psnames/psmodule.o		
#	$(TMP_DIR)/freetype/base/ftxf86.o		\

FREETYPE_C_OPTIONS = 						\
	-DFT2_BUILD_LIBRARY 					\
	-DDFT_CONFIG_MODULES_H="<ftmodule.h>"

FREETYPE_C_INCLUDES = 						\
	-I$(FREETYPE_SRC_DIR)/builds/ansi		\
	-I$(FREETYPE_SRC_DIR)/include			\
	-I$(FREETYPE_SRC_DIR)/include/freetype	\
	-I$(FREETYPE_SRC_DIR)/src/truetype		\
	-I$(FREETYPE_SRC_DIR)/src/sfnt			\
	-I$(FREETYPE_SRC_DIR)/src/autofit		\
	-I$(FREETYPE_SRC_DIR)/src/smooth		\
	-I$(FREETYPE_SRC_DIR)/src/raster		\
	-I$(FREETYPE_SRC_DIR)/src/psaux			\
	-I$(FREETYPE_SRC_DIR)/src/psnames

OBJECTS += $(FREETYPE_OBJECTS)
C_OPTIONS += -I$(FREETYPE_SRC_DIR)/include

FREETYPE_C_OPTIONS += $(FREETYPE_PLATFORM_C_OPTIONS)

FREETYPE: $(FREETYPE_SRC_DIR) $(FREETYPE_OBJECTS_DIRS) $(FREETYPE_OBJECTS)

$(FREETYPE_SRC_DIR):
	mkdir -p $(FREETYPE_SRC_BASE)
	cd $(FREETYPE_DIR)/src; tar xjf $(FREETYPE_DIR)/freetype-$(FREETYPE_VERSION).tar.bz2 
	
$(FREETYPE_OBJECTS_DIRS):
	mkdir -p $(FREETYPE_OBJECTS_DIRS)

$(TMP_DIR)/freetype/base/ftsystem.o: $(FREETYPE_SRC_DIR)/src/base/ftsystem.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftdebug.o: $(FREETYPE_SRC_DIR)/src/base/ftdebug.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftinit.o: $(FREETYPE_SRC_DIR)/src/base/ftinit.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftbbox.o: $(FREETYPE_SRC_DIR)/src/base/ftbbox.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftbitmap.o: $(FREETYPE_SRC_DIR)/src/base/ftbitmap.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftcid.o: $(FREETYPE_SRC_DIR)/src/base/ftcid.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftadvanc.o: $(FREETYPE_SRC_DIR)/src/base/ftadvanc.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftcalc.o: $(FREETYPE_SRC_DIR)/src/base/ftcalc.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftdbgmem.o: $(FREETYPE_SRC_DIR)/src/base/ftdbgmem.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftgloadr.o: $(FREETYPE_SRC_DIR)/src/base/ftgloadr.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftobjs.o: $(FREETYPE_SRC_DIR)/src/base/ftobjs.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftoutln.o: $(FREETYPE_SRC_DIR)/src/base/ftoutln.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftrfork.o: $(FREETYPE_SRC_DIR)/src/base/ftrfork.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftsnames.o: $(FREETYPE_SRC_DIR)/src/base/ftsnames.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftstream.o: $(FREETYPE_SRC_DIR)/src/base/ftstream.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/fttrigon.o: $(FREETYPE_SRC_DIR)/src/base/fttrigon.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftutil.o: $(FREETYPE_SRC_DIR)/src/base/ftutil.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftfstype.o: $(FREETYPE_SRC_DIR)/src/base/ftfstype.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftgasp.o: $(FREETYPE_SRC_DIR)/src/base/ftgasp.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftglyph.o: $(FREETYPE_SRC_DIR)/src/base/ftglyph.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftgxval.o: $(FREETYPE_SRC_DIR)/src/base/ftgxval.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftlcdfil.o: $(FREETYPE_SRC_DIR)/src/base/ftlcdfil.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftmm.o: $(FREETYPE_SRC_DIR)/src/base/ftmm.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftotval.o: $(FREETYPE_SRC_DIR)/src/base/ftotval.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftpatent.o: $(FREETYPE_SRC_DIR)/src/base/ftpatent.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftpfr.o: $(FREETYPE_SRC_DIR)/src/base/ftpfr.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftstroke.o: $(FREETYPE_SRC_DIR)/src/base/ftstroke.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftsynth.o: $(FREETYPE_SRC_DIR)/src/base/ftsynth.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/fttype1.o: $(FREETYPE_SRC_DIR)/src/base/fttype1.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/base/ftwinfnt.o: $(FREETYPE_SRC_DIR)/src/base/ftwinfnt.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
#$(TMP_DIR)/freetype/base/ftxf86.o: $(FREETYPE_SRC_DIR)/src/base/ftxf86.c
#	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/truetype/truetype.o: $(FREETYPE_SRC_DIR)/src/truetype/truetype.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/type1/type1.o: $(FREETYPE_SRC_DIR)/src/type1/type1.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/cff/cff.o: $(FREETYPE_SRC_DIR)/src/cff/cff.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/cid/type1cid.o: $(FREETYPE_SRC_DIR)/src/cid/type1cid.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/pfr/pfr.o: $(FREETYPE_SRC_DIR)/src/pfr/pfr.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/type42/type42.o: $(FREETYPE_SRC_DIR)/src/type42/type42.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/winfonts/winfnt.o: $(FREETYPE_SRC_DIR)/src/winfonts/winfnt.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/pcf/pcf.o: $(FREETYPE_SRC_DIR)/src/pcf/pcf.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/bdf/bdf.o: $(FREETYPE_SRC_DIR)/src/bdf/bdf.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/sfnt/sfnt.o	: $(FREETYPE_SRC_DIR)/src/sfnt/sfnt.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/autofit/autofit.o: $(FREETYPE_SRC_DIR)/src/autofit/autofit.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/pshinter/pshinter.o: $(FREETYPE_SRC_DIR)/src/pshinter/pshinter.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/raster/raster.o: $(FREETYPE_SRC_DIR)/src/raster/raster.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/smooth/smooth.o: $(FREETYPE_SRC_DIR)/src/smooth/smooth.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/cache/ftcache.o: $(FREETYPE_SRC_DIR)/src/cache/ftcache.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/gzip/ftgzip.o: $(FREETYPE_SRC_DIR)/src/gzip/ftgzip.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/lzw/ftlzw.o: $(FREETYPE_SRC_DIR)/src/lzw/ftlzw.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/bzip2/ftbzip2.o: $(FREETYPE_SRC_DIR)/src/bzip2/ftbzip2.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/psaux/psaux.o: $(FREETYPE_SRC_DIR)/src/psaux/psaux.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@
$(TMP_DIR)/freetype/psnames/psmodule.o: $(FREETYPE_SRC_DIR)/src/psnames/psmodule.c
	$(CC) $(C_OPTIONS) $(FREETYPE_C_OPTIONS) $(FREETYPE_C_INCLUDES) -c $< -o $@


