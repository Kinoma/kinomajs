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
SET(FREETYPE_VERSION 2.6)
SET(FREETYPE_DIR ${TMP_DIR}/freetype-${FREETYPE_VERSION})
IF (SYSROOT)
	SET(FREETYPE_PLATFORM_C_OPTIONS --sysroot=${SYSROOT} -fno-strict-aliasing)
ENDIF()

LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftsystem.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftdebug.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftinit.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftbbox.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftbitmap.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftcid.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftadvanc.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftcalc.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftdbgmem.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftgloadr.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftobjs.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftoutln.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftrfork.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftsnames.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftstream.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/fttrigon.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftutil.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftfstype.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftgasp.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftglyph.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftgxval.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftlcdfil.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftmm.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftotval.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftpatent.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftpfr.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftstroke.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftsynth.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/fttype1.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftwinfnt.c)
# LIst(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/base/ftxf86.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/truetype/truetype.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/type1/type1.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/cff/cff.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/cid/type1cid.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/pfr/pfr.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/type42/type42.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/winfonts/winfnt.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/pcf/pcf.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/bdf/bdf.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/sfnt/sfnt.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/autofit/autofit.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/pshinter/pshinter.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/raster/raster.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/smooth/smooth.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/cache/ftcache.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/gzip/ftgzip.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/lzw/ftlzw.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/bzip2/ftbzip2.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/psaux/psaux.c)
LIST(APPEND Freetype_SOURCES ${FREETYPE_DIR}/src/psnames/psmodule.c)

SET_SOURCE_FILES_PROPERTIES(${Freetype_SOURCES} PROPERTIES GENERATED TRUE)

FILE(MAKE_DIRECTORY ${FREETYPE_DIR})

IF(NOT EXISTS ${FREETYPE_DIR}/README)
	EXECUTE_PROCESS(
		COMMAND ${CMAKE_COMMAND} -E echo "Extracting FreeType"
		COMMAND ${CMAKE_COMMAND} -E tar xjf ${F_HOME}/libraries/freetype/freetype-${FREETYPE_VERSION}.tar.bz2
		WORKING_DIRECTORY ${TMP_DIR}
		)
ENDIF()

ADD_LIBRARY(FreeType OBJECT ${Freetype_SOURCES})
LIST(APPEND TARGET_OBJECTS $<TARGET_OBJECTS:FreeType>)
SET(TARGET_OBJECTS ${TARGET_OBJECTS} PARENT_SCOPE)

# First, compiler definitions for building the library
SET_PROPERTY(TARGET FreeType
	PROPERTY COMPILE_DEFINITIONS
	FT2_BUILD_LIBRARY
	DFT_CONFIG_MODULES_H=<ftmodule.h>
	)
SET_PROPERTY(TARGET FreeType
	PROPERTY COMPILE_OPTIONS
	${FREETYPE_PLATFORM_C_OPTIONS}
	)

# Workaround for BG3CDP
if (BG3CDP_GNUEABI)
	set(BG3CDP_INCLUDES ${BG3CDP_GNUEABI}/arm-linux-gnueabihf/libc/usr/include)
endif ()

# Specify library include directories
SET_PROPERTY(TARGET FreeType
	PROPERTY INCLUDE_DIRECTORIES
	${FREETYPE_DIR}/builds/ansi
	${FREETYPE_DIR}/include
	${FREETYPE_DIR}/include/freetype
	${PROJECT_BINARY_DIR}/include
	${PROJECT_SOURCE_DIR}/objs
	${FREETYPE_DIR}/src/truetype
	${FREETYPE_DIR}/src/sfnt
	${FREETYPE_DIR}/src/autofit
	${FREETYPE_DIR}/src/smooth
	${FREETYPE_DIR}/src/raster
	${FREETYPE_DIR}/src/psaux
	${FREETYPE_DIR}/src/psnames
	${BG3CDP_INCLUDES}
	)
