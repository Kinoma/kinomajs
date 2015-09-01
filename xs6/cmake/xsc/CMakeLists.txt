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
PROJECT(XSC)

INCLUDE_DIRECTORIES(${XS6}/sources/tool)

IF(WIN32)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /nologo /Zp1") 
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /D mxDebug /Od /Z7")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /Fp$(TMP_DIR_RLS) /O2")
ELSEIF(UNIX)
	SET(CMAKE_C_FLAGS "-fno-common -DmxCompile=1")
	IF(APPLE)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
	ENDIF()
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
ENDIF()

LIST(APPEND SOURCES ${XS6}/sources/xs6Code.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Common.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Lexical.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Scope.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Script.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6SourceMap.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Syntaxical.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Tree.c)
LIST(APPEND SOURCES ${XS6}/sources/xs_dtoa.c)
LIST(APPEND SOURCES ${XS6}/sources/xsc6.c)

ADD_EXECUTABLE(xsc ${SOURCES})
IF(WIN32)
	TARGET_LINK_LIBRARIES(xsc ws2_32.lib comctl32.lib gdi32.lib)
ENDIF()
SET_TARGET_PROPERTIES(xsc PROPERTIES OUTPUT_NAME xsc6)
ADD_CUSTOM_COMMAND(
	TARGET xsc
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xsc> ${XS6_TOOLS_BIN}
	)
