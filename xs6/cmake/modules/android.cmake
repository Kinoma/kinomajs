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
set(ANDROID true)
include (CMakeForceCompiler)

set(ANDROID_NDK $ENV{ANDROID_NDK})
if(NOT ANDROID_NDK)
	set(ANDROID_NDK $ENV{NDK_DIR})
endif()

set(ANDROID_SDK $ENV{ANDROID_SDK})

if(APPLE)
	set(HOST_SYSTEM_NAME "darwin-x86_64")
	set(HOST_SYSTEM_NAME2 "darwin-x86")
elseif(WIN32)
	set(HOST_SYSTEM_NAME "windows-x86_64")
	set(HOST_SYSTEM_NAME2 "windows")
	set(TOOL_OS_SUFFIX ".exe")
elseif(UNIX)
	set(HOST_SYSTEM_NAME "linux-x86_64")
	set(HOST_SYSTEM_NAME2 "linux-x86")
else()
	message(FATAL_ERROR "Cross-compilation on your platform is not supported.")
endif()

if(CMAKE_VERSION VERSION_GREATER "3.0.99")
 set(CMAKE_SYSTEM_NAME Android)
else()
 set(CMAKE_SYSTEM_NAME Linux)
endif()
set(APPLE true CACHE BOOL "Host Build Type")

set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_SYSTEM_PROCESSOR "armv7-a")
set(ANDROID_ABI "armeabi-v7a")

set(TOOLCHAINS_PATH "${ANDROID_NDK}/toolchains")
set(TOOLCHAIN_SUBPATH  "prebuilt/${HOST_SYSTEM_NAME}")
set(TOOLCHAIN_SUBPATH2 "prebuilt/${HOST_SYSTEM_NAME2}")

file(GLOB TOOLCHAINS RELATIVE ${TOOLCHAINS_PATH} "${TOOLCHAINS_PATH}/arm-linux-androideabi-*")
list(SORT TOOLCHAINS)

if(NOT TOOLCHAIN)
	foreach(TOOLCHAIN ${TOOLCHAINS})
		string(FIND ${TOOLCHAIN} "-clang" INDEX)
		if(INDEX GREATER -1)
			list(REMOVE_ITEM TOOLCHAINS ${TOOLCHAIN})
		endif()
	endforeach()
	list(GET TOOLCHAINS -1 TOOLCHAIN)
else()
	list(FIND TOOLCHAINS ${TOOLCHAIN} ITEM)
	if(${ITEM} EQUAL -1)
		message(FATAL_ERROR "${TOOLCHAIN} does not exist")
	endif()
endif()

set(TOOLCHAIN_ROOT "${TOOLCHAINS_PATH}/${TOOLCHAIN}/${TOOLCHAIN_SUBPATH}")
if(NOT EXISTS ${TOOLCHAIN_ROOT})
	set(TOOLCHAIN_ROOT "${TOOLCHAINS_PATH}/${TOOLCHAIN}/${TOOLCHAIN_SUBPATH2}")
endif()

set(TOOLCHAIN_BIN ${TOOLCHAIN_ROOT}/bin)

string(REGEX MATCH "[0-9]+[.][0-9]+([.][0-9x]+)?$" COMPILER_VERSION ${TOOLCHAIN})
string(REGEX REPLACE "-${COMPILER_VERSION}$" "" TOOLCHAIN_NAME "${TOOLCHAIN}")

set(NDK_PLATFORM_VER			"14")
set(SYSROOT				"${ANDROID_NDK}/platforms/android-${NDK_PLATFORM_VER}/arch-arm")
set(NDK_PLATFORM			"${SYSROOT}/usr/lib")
set(NDK_TOOLCHAIN_VERSION		"${COMPILER_VERSION}")

CMAKE_FORCE_CXX_COMPILER(		"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-g++${TOOL_OS_SUFFIX}" GNU)
CMAKE_FORCE_C_COMPILER(			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-gcc${TOOL_OS_SUFFIX}" GNU)
set(CMAKE_AR				"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-ar${TOOL_OS_SUFFIX}" CACHE PATH "archive")
set(CMAKE_ASM_COMPILER			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-gcc${TOOL_OS_SUFFIX}" CACHE PATH "assembler")
set(CMAKE_LINKER			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-ld${TOOL_OS_SUFFIX}" CACHE PATH "linker")
set(CMAKE_NM				"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-nm${TOOL_OS_SUFFIX}" CACHE PATH "nm")
set(CMAKE_OBJCOPY			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-objcopy${TOOL_OS_SUFFIX}" CACHE PATH "objcopy")
set(CMAKE_OBJDUMP			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump")
set(CMAKE_RANLIB			"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-ranlib${TOOL_OS_SUFFIX}" CACHE PATH "ranlib")
set(CMAKE_STRIP				"${TOOLCHAIN_BIN}/${TOOLCHAIN_NAME}-strip${TOOL_OS_SUFFIX}" CACHE PATH "strip")

set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS	"gas7;gas;s")
set(CMAKE_ASM_COMPILE_OBJECT		"${CMAKE_C_COMPILER} -c -x assembler-with-cpp -c -march=armv7-a <FLAGS> -o <OBJECT> <SOURCE>")
set(AS_NEON_OPTIONS			"-mfpu=neon")
set(AS_WMMX_OPTIONS			"-mwmmxt")

set(NDK_PROJECT_PATH			"${TMP_DIR}/ndk/project")
set(NDK_PROJECT_LIBRARIES		"${NDK_PROJECT_PATH}/app/src/main/jniLibs/armeabi")

set(SEPARATE_LIBRARIES			"${TOOLCHAIN_ROOT}lib/gcc/${TOOLCHAIN_NAME}/${COMPILER_VERSION}/libgcc.a -L${ANDROID_NDK})/${NDK_PLATFORM} -lc -lstdc++ -lm ${NDK_PROJECT_PATH}/modules/Fsk/build/outputs/native/debug/arm/lib/armeabi/libFsk.so -ldl -llog")
set(SEPARATE_DIR			"${NDK_PROJECT_LIBRARIES}")

set(FREETYPE_VERSION			"2.6")
set(FREETYPE_DIR			"${TMP_DIR}/freetype-${FREETYPE_VERSION}")
set(FREETYPE_PLATFORM_C_OPTIONS		"--sysroot=${SYSROOT}")

set(TOOLCHAIN_LIB_PATH			"${TOOLCHAIN_ROOT}/lib/gcc/${TOOLCHAIN_NAME}/${COMPILER_VERSION}")
set(SEPARATE_LINK_OPTIONS		"-nostdlib -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--fix-cortex-a8 -Wl,-rpath-link=${NDK_PLATFORM}")
if(NOT RELEASE)
	set(SEPARATE_LINK_OPTIONS "${SEPARATE_LINK_OPTIONS} -g")
endif()
set(SEPARATE_LIBRARIES			"${TOOLCHAIN_ROOT}/lib/gcc/${TOOLCHAIN_NAME}/${COMPILER_VERSION}/libgcc.a -L${SYSROOT}/usr/lib -L${TMP_DIR} -lc -lstdc++ -lm -ldl -llog -landroid")


set(CMAKE_C_CREATE_SHARED_LIBRARY	"<CMAKE_C_COMPILER> -Wl,-soname,<TARGET_SONAME> <OBJECTS> ${SEPARATE_LINK_OPTIONS} <LINK_LIBRARIES> ${SEPARATE_LIBRARIES} -o <TARGET>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY	"<CMAKE_CXX_COMPILER> -Wl,-soname,<TARGET_SONAME> <OBJECTS> ${SEPARATE_LINK_OPTIONS} <LINK_LIBRARIES> ${SEPARATE_LIBRARIES} -o <TARGET>")

set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> cq <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> cq <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")

macro(BUILD)
	set(oneValueArgs NAME XSDEBUG)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	file(MAKE_DIRECTORY ${FREETYPE_DIR})

	add_subdirectory(${F_HOME}/libraries/freetype/xs6 FreeType)

	add_library(fsk ${SOURCES} ${OBJECTS} ${TARGET_OBJECTS})
	add_dependencies(fsk FreeType)

	add_custom_target(
		jet
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NDK_PROJECT_PATH}/app/src/main/res/raw
		COMMAND ${CMAKE_COMMAND} -E remove ${NDK_PROJECT_PATH}/app/src/main/res/raw/kinoma.jet
		COMMAND zip -8qrn .jpg:.png:.m4a ${NDK_PROJECT_PATH}/app/src/main/res/raw/kinoma.jet "*"
		DEPENDS FskManifest.xsa
		WORKING_DIRECTORY ${APP_DIR}
		)

	add_custom_target(ndk ALL
		COMMENT "Building NDK Parts"
		COMMAND sh gradlew -q assembleArm$<CONFIG>
		COMMAND ${CMAKE_COMMAND} -E copy ${NDK_PROJECT_PATH}/modules/Fsk/build/outputs/native/$<LOWER_CASE:$<CONFIG>>/arm/lib/armeabi/libFsk.so ${TMP_DIR}
		WORKING_DIRECTORY ${NDK_PROJECT_PATH}
		DEPENDS fsk jet
		)

	add_custom_target(apk ALL
		COMMENT "Creating the APK"
		COMMAND sh gradlew -q assemble$<CONFIG>
		COMMAND ${CMAKE_COMMAND} -E echo copy ${NDK_PROJECT_PATH}/app/build/outputs/apk/app-$<LOWER_CASE:$<CONFIG>>.apk ${BIN_DIR}/${APK_NAME}
		COMMAND ${CMAKE_COMMAND} -E copy ${NDK_PROJECT_PATH}/app/build/outputs/apk/app-$<LOWER_CASE:$<CONFIG>>.apk ${BIN_DIR}/${APK_NAME}
		WORKING_DIRECTORY ${NDK_PROJECT_PATH}
		DEPENDS ndk ${SHARED}
		)

	add_custom_target(message ALL
		COMMAND ${CMAKE_COMMAND} -E echo  "--------------------------------------------------------------------------------"
		COMMAND ${CMAKE_COMMAND} -E echo  "-   Install: adb -d install -r ${BIN_DIR}/${APK_NAME}"
		COMMAND ${CMAKE_COMMAND} -E echo  "- Uninstall: adb -d uninstall ${KPR_NAMESPACE}"
		COMMAND ${CMAKE_COMMAND} -E echo  "-     Start: adb shell am start -n ${KPR_NAMESPACE}/${KPR_NAMESPACE}.KinomaPlay"
		COMMAND ${CMAKE_COMMAND} -E echo  "-      Stop: adb shell am force-stop ${KPR_NAMESPACE}"
		COMMAND ${CMAKE_COMMAND} -E echo  "-     Debug: cd ${NDK_PROJECT_PATH} \\; export NDK_PROJECT_PATH=${NDK_PROJECT_PATH} \\; ${ANDROID_NDK}/ndk-gdb --verbose --force --start"
		COMMAND ${CMAKE_COMMAND} -E echo  "--------------------------------------------------------------------------------"
		DEPENDS apk
		)

endmacro()
