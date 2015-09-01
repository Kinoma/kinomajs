#
#      Copyright (C) 2010-2015 Marvell International Ltd.
#      Copyright (C) 2002-2010 Kinoma, Inc.
# 
#      Licensed under the Apache License, Version 2.0 (the "License");
#      you may not use this file except in compliance with the License.
#      You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#      Unless required by applicable law or agreed to in writing, software
#      distributed under the License is distributed on an "AS IS" BASIS,
#      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#      See the License for the specific language governing permissions and
#      limitations under the License.
#
include (CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR "aspen")

SET(TOOL_PREFIX arm-marvell-linux-gnueabi-)
SET(FSK_SYSROOT_LIB $ENV{FSK_SYSROOT_LIB})

FIND_PATH(TOOLCHAIN_BIN ${TOOL_PREFIX}gcc $ENV{PATH} $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin)

SET(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc CACHE PATH "assembler")
CMAKE_FORCE_C_COMPILER (${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
CMAKE_FORCE_CXX_COMPILER (${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
SET(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
SET(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -c -x assembler-with-cpp -o <OBJECT> <SOURCE>")

SET(AS_NEON ${TOOL_PREFIX}as)
SET(AS_NEON_OPTIONS -mfpu=neon)
SET(AS_OPTIONS -c -x assembler-with-cpp)
SET(AS_V7 ${TOOL_PREFIX}as)
SET(AS_V7_OPTIONS )
SET(AS_WMMX ${TOOL_PREFIX}as)
SET(AS_WMMX_OPTIONS -mwmmxt)

SET(FREETYPE_VERSION 2.6)
SET(FREETYPE_DIR ${TMP_DIR}/freetype-${FREETYPE_VERSION})
