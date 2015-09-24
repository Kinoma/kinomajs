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
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(XSR)

include_directories(${XS6}/includes)
include_directories(${XS6}/sources)
include_directories(${XS6}/sources/tool)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /D mxParse=1 /D mxRun=1 /nologo /Zp1")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /D mxDebug /Od /Z7")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /O2")
	set(LINK_OPTIONS ws2_32.lib comctl32.lib)
elseif(UNIX)
	if(APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
		find_library(CORE_SERVICES CoreServices)
		set(LINK_OPTIONS ${CORE_SERVICES})
	else()
		set(LINK_OPTIONS "-rdynamic -lm -ldl")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -DmxParse -DmxRun=1")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
endif()

set(CMAKE_XCODE_ADDRITBUTE_SYMROOT ${CMAKE_BINARY_DIR})

list(APPEND SOURCES ${XS6}/sources/tool/xs6Host.c)
list(APPEND SOURCES ${XS6}/sources/tool/xs6Platform.c)
list(APPEND SOURCES ${XS6}/sources/xs6API.c)
list(APPEND SOURCES ${XS6}/sources/xs6All.c)
list(APPEND SOURCES ${XS6}/sources/xs6Array.c)
list(APPEND SOURCES ${XS6}/sources/xs6Boolean.c)
list(APPEND SOURCES ${XS6}/sources/xs6Code.c)
list(APPEND SOURCES ${XS6}/sources/xs6Common.c)
list(APPEND SOURCES ${XS6}/sources/xs6DataView.c)
list(APPEND SOURCES ${XS6}/sources/xs6Date.c)
list(APPEND SOURCES ${XS6}/sources/xs6Debug.c)
list(APPEND SOURCES ${XS6}/sources/xs6Error.c)
list(APPEND SOURCES ${XS6}/sources/xs6Function.c)
list(APPEND SOURCES ${XS6}/sources/xs6Generator.c)
list(APPEND SOURCES ${XS6}/sources/xs6Global.c)
list(APPEND SOURCES ${XS6}/sources/xs6JSON.c)
list(APPEND SOURCES ${XS6}/sources/xs6Lexical.c)
list(APPEND SOURCES ${XS6}/sources/xs6MapSet.c)
list(APPEND SOURCES ${XS6}/sources/xs6Marshall.c)
list(APPEND SOURCES ${XS6}/sources/xs6Math.c)
list(APPEND SOURCES ${XS6}/sources/xs6Memory.c)
list(APPEND SOURCES ${XS6}/sources/xs6Module.c)
list(APPEND SOURCES ${XS6}/sources/xs6Number.c)
list(APPEND SOURCES ${XS6}/sources/xs6Object.c)
list(APPEND SOURCES ${XS6}/sources/xs6Profile.c)
list(APPEND SOURCES ${XS6}/sources/xs6Promise.c)
list(APPEND SOURCES ${XS6}/sources/xs6Property.c)
list(APPEND SOURCES ${XS6}/sources/xs6Proxy.c)
list(APPEND SOURCES ${XS6}/sources/xs6RegExp.c)
list(APPEND SOURCES ${XS6}/sources/xs6Run.c)
list(APPEND SOURCES ${XS6}/sources/xs6Scope.c)
list(APPEND SOURCES ${XS6}/sources/xs6Script.c)
list(APPEND SOURCES ${XS6}/sources/xs6SourceMap.c)
list(APPEND SOURCES ${XS6}/sources/xs6String.c)
list(APPEND SOURCES ${XS6}/sources/xs6Symbol.c)
list(APPEND SOURCES ${XS6}/sources/xs6Syntaxical.c)
list(APPEND SOURCES ${XS6}/sources/xs6Tree.c)
list(APPEND SOURCES ${XS6}/sources/xs6Type.c)
list(APPEND SOURCES ${XS6}/sources/xs_dtoa.c)
list(APPEND SOURCES ${XS6}/sources/xs_pcre.c)
list(APPEND SOURCES ${XS6}/sources/xsr6.c)

add_executable(xsr ${SOURCES})
if(LINK_OPTIONS)
	target_link_libraries(xsr ${LINK_OPTIONS})
endif()
set_target_properties(xsr PROPERTIES OUTPUT_NAME xsr6)
add_custom_command(
	TARGET xsr
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xsr> ${XS6_TOOLS_BIN}
	)
