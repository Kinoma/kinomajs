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
set(LINUX true)
set(ASPEN true)
include (CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "aspen")

set(TOOL_PREFIX arm-marvell-linux-gnueabi-)
set(FSK_SYSROOT_LIB $ENV{FSK_SYSROOT_LIB})

find_path(TOOLCHAIN_BIN ${TOOL_PREFIX}gcc $ENV{PATH} $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin)

set(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc CACHE PATH "assembler")
cmake_force_c_compiler(${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
cmake_force_cxx_compiler(${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -c -x assembler-with-cpp -o <OBJECT> <SOURCE>")

set(AS_NEON ${TOOL_PREFIX}as)
set(AS_NEON_OPTIONS -mfpu=neon)
set(AS_OPTIONS -c -x assembler-with-cpp)
set(AS_V7 ${TOOL_PREFIX}as)
set(AS_V7_OPTIONS )
set(AS_WMMX ${TOOL_PREFIX}as)
set(AS_WMMX_OPTIONS -mwmmxt)

set(FREETYPE_VERSION 2.6)
set(FREETYPE_DIR ${TMP_DIR}/freetype-${FREETYPE_VERSION})
