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
include (CMakeForceCompiler)

macro(FIND_TOOL VAR TOOL)
		find_program(XCRUN xcrun)
		if(XCRUN)
			execute_process(COMMAND ${XCRUN} -f ${TOOL} OUTPUT_VARIABLE TOOL_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(TOOL_PATH)
				set(${VAR} ${TOOL_PATH} CACHE PATH "Path to a program")
			else()
				find_program(${VAR} ${TOOL})
			endif()
		endif()
endmacro()

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
find_tool(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
if(CMAKE_UNAME)
	exec_program(uname ARGS -r OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_VERSION)
	string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\1" DARWIN_MAJOR_VERSION "${CMAKE_HOST_SYSTEM_VERSION}")
endif()
set(UNIX TRUE)
set(APPLE TRUE)
set(IOS TRUE)

set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")

find_tool(CMAKE_INSTALL_NAME_TOOL install_name_tool)
find_tool(CMAKE_AR libtool)
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -static -o <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -static -o <TARGET> <OBJECTS>")
find_tool(CMAKE_C_COMPILER clang)
find_tool(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_C_COMPILER_WORKS TRUE)

set(SDKROOT /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk)

set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "Force unset of the deployment target for iOS" FORCE)
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "8.0" CACHE STRING "Set iOS deployment target" FORCE)
set(CMAKE_OSX_SYSROOT ${SDKROOT} CACHE PATH "Sysroot used for iOS support")
set(CMAKE_FIND_ROOT_PATH ${SDKROOT} ${CMAKE_PREFIX_PATH} CACHE string  "iOS find search path root")
set(CMAKE_FIND_FRAMEWORK FIRST)

set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} -c -x assembler-with-cpp -arch armv7 -MMD -DSUPPORT_NEON_IOS=1 -o <OBJECT> <SOURCE>")
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILER_ID AppleClang)
set(CMAKE_OSX_ARCHITECTURES armv7 arm64 CACHE STRING "Build architecture of iOS")
set(CODESIGN_ALLOCATE /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/codesign_allocate)
set(CMAKE_FRAMEWORK_PATH ${SDKROOT}/System/Library/Frameworks)
set(VERSION_MIN -miphoneos-version-min=6.0)

if (CMAKE_GENERATOR STREQUAL "Xcode")
	SET(APP_TYPE MACOSX_BUNDLE)
endif ()

macro(BUILD)
	set(oneValueArgs APPLICATION IDENTITY IDENTIFIER HASH)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_executable(${LOCAL_APPLICATION} MACOSX_BUNDLE ${SOURCES} ${FskPlatform_SOURCES} ${F_HOME}/xs6/patches/main.m)
	target_link_libraries(${LOCAL_APPLICATION} ${LIBRARIES} ${OBJECTS} -ObjC)

	set(MACOSX_BUNDLE_INFO_PLIST ${TMP_DIR}/Info.plist)

	set_target_properties(${LOCAL_APPLICATION}
		PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${LOCAL_IDENTITY}"
		MACOSX_BUNDLE_GUI_IDENTIFIER ${LOCAL_IDENTIFIER}
		)

	add_custom_command(
		TARGET ${LOCAL_APPLICATION}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${TMP_DIR}/Info.plist ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${PROVISION} ${APP_DIR}/embedded.mobileprovision
		)

	if(CMAKE_CONFIGURATION_TYPES)
		add_custom_command(
			TARGET ${LOCAL_APPLICATION}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${LOCAL_APPLICATION}.app
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${LOCAL_APPLICATION}.app/${LOCAL_APPLICATION} ${APP_DIR}
			)
	else()
		add_custom_command(
			TARGET ${LOCAL_APPLICATION}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy  $<TARGET_FILE:${LOCAL_APPLICATION}> ${APP_DIR}
			COMMAND dsymutil $<TARGET_FILE:${LOCAL_APPLICATION}> -o $<TARGET_FILE:${LOCAL_APPLICATION}>.dSYM
			)
	endif()

	add_custom_command(
		TARGET ${LOCAL_APPLICATION}
		POST_BUILD
		COMMAND codesign -f -v -s ${LOCAL_HASH} --entitlements ${TMP_DIR}/Entitlements.plist ${TMP_DIR}/${CMAKE_CFG_INTDIR}/${LOCAL_APPLICATION}.app
		COMMAND codesign -f -v -s ${LOCAL_HASH} --entitlements ${TMP_DIR}/Entitlements.plist ${APP_DIR}
		COMMAND xcrun -sdk iphoneos PackageApplication ${APP_DIR} -o ${APP_IPA}
		VERBATIM
		)
endmacro()
