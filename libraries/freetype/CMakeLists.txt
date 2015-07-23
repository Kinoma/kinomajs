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
cmake_minimum_required(VERSION 2.8)
project(LibraryFreetype)

set(FREETYPE_DIR ${F_HOME}/libraries/freetype)
set(FREETYPE_SRC_BASE ${FREETYPE_DIR}/src)
set(FREETYPE_SRC_DIR ${FREETYPE_SRC_BASE}/freetype-${FREETYPE_VERSION})

list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftsystem.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftdebug.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftinit.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftbbox.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftbitmap.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftcid.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftadvanc.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftcalc.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftdbgmem.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftgloadr.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftobjs.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftoutln.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftrfork.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftsnames.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftstream.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/fttrigon.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftutil.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftfstype.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftgasp.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftglyph.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftgxval.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftlcdfil.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftmm.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftotval.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftpatent.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftpfr.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftstroke.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftsynth.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/fttype1.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftwinfnt.c)
# list(APPEND sources ${FREETYPE_SRC_DIR}/src/base/ftxf86.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/truetype/truetype.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/type1/type1.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/cff/cff.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/cid/type1cid.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/pfr/pfr.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/type42/type42.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/winfonts/winfnt.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/pcf/pcf.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/bdf/bdf.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/sfnt/sfnt.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/autofit/autofit.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/pshinter/pshinter.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/raster/raster.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/smooth/smooth.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/cache/ftcache.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/gzip/ftgzip.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/lzw/ftlzw.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/bzip2/ftbzip2.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/psaux/psaux.c)
list(APPEND sources ${FREETYPE_SRC_DIR}/src/psnames/psmodule.c)

add_library(${PROJECT_NAME} OBJECT ${sources})

# First, compiler definitions for building the library
set_property(TARGET ${PROJECT_NAME}
	PROPERTY COMPILE_DEFINITIONS
	FT2_BUILD_LIBRARY
	DFT_CONFIG_MODULES_H=<ftmodule.h>
	)

# Workaround for BG3CDP
if (BG3CDP_GNUEABI)
	set(BG3CDP_INCLUDES ${BG3CDP_GNUEABI}/arm-marvell-linux-gnueabi/libc/usr/include)
endif ()

# Specify library include directories
set_property(TARGET ${PROJECT_NAME}
	PROPERTY INCLUDE_DIRECTORIES
	${FREETYPE_SRC_DIR}/builds/ansi
	${FREETYPE_SRC_DIR}/include
	${FREETYPE_SRC_DIR}/include/freetype
	${PROJECT_BINARY_DIR}/include
	${PROJECT_SOURCE_DIR}/objs
	${FREETYPE_SRC_DIR}/src/truetype
	${FREETYPE_SRC_DIR}/src/sfnt
	${FREETYPE_SRC_DIR}/src/autofit
	${FREETYPE_SRC_DIR}/src/smooth
	${FREETYPE_SRC_DIR}/src/raster
	${FREETYPE_SRC_DIR}/src/psaux
	${FREETYPE_SRC_DIR}/src/psnames
	${BG3CDP_INCLUDES}
	)
