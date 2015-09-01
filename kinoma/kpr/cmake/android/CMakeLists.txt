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

set(CMAKE_MODULE_PATH ${CONFIG_BASEDIR}/${TARGET_PLATFORM}/CMake-Modules)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")

enable_language(CXX)
enable_language(ASM-wmmx)
enable_language(ASM-gas7)

message("Using Toolchain File: ${CMAKE_TOOLCHAIN_FILE}")

set(BIN_DIR ${BUILD_TMP}/jet)
set(NDK_PLATFORM_VER 14)
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(NDK_OBJ_PATH ${NDK_PROJECT_PATH}/obj/local/armeabi)
set(NDK_LIBS_PATH ${NDK_PROJECT_PATH}/libs/armeabi)
set(FSK_A_PATH  ${NDK_OBJ_PATH}/libfsk.a)
set(FSK_SO_PATH ${NDK_LIBS_PATH}/libFsk.so)
set(LIBRARY_OUTPUT_PATH ${BUILD_TMP})
set(DEPENDS_TARGET "apk")
set(SHARED_LINK_FLAGS "-lFsk")
set(SHARED_DEPENDS ndk)

set(CMAKE_ASM-gas7_FLAGS "${CMAKE_C_FLAGS}")

include_directories(${F_HOME}/libraries/freetype/src/freetype-${FREETYPE_VERSION}/include)
link_directories(${NDK_PROJECT_PATH}/obj/local/armeabi)
link_directories(${F_HOME}/build/android/marvelllibs)

add_subdirectory(${F_HOME}/libraries/freetype "freetype")

list(APPEND PLATFORM_OBJECTS $<TARGET_OBJECTS:LibraryFreetype>)
set(INSTALL_PATH ${NDK_PROJECT_PATH}/obj/local/armeabi)
