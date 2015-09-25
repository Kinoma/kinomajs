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
project(XSC)

include_directories(${XS6}/sources/tool)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /nologo /Zp1") 
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /D mxDebug /Od /Z7 /MTd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /O2 /MT")
elseif(UNIX)
	set(CMAKE_C_FLAGS "-fno-common -DmxCompile=1")
	if(APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
	endif()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
endif()

list(APPEND SOURCES ${XS6}/sources/xs6Code.c)
list(APPEND SOURCES ${XS6}/sources/xs6Common.c)
list(APPEND SOURCES ${XS6}/sources/xs6Lexical.c)
list(APPEND SOURCES ${XS6}/sources/xs6Scope.c)
list(APPEND SOURCES ${XS6}/sources/xs6Script.c)
list(APPEND SOURCES ${XS6}/sources/xs6SourceMap.c)
list(APPEND SOURCES ${XS6}/sources/xs6Syntaxical.c)
list(APPEND SOURCES ${XS6}/sources/xs6Tree.c)
list(APPEND SOURCES ${XS6}/sources/xs_dtoa.c)
list(APPEND SOURCES ${XS6}/sources/xsc6.c)

add_executable(xsc ${SOURCES})
if(WIN32)
	target_link_libraries(xsc ws2_32.lib comctl32.lib gdi32.lib)
endif()
set_target_properties(xsc PROPERTIES OUTPUT_NAME xsc6)
add_custom_command(
	TARGET xsc
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xsc> ${XS6_TOOLS_BIN}
	)
