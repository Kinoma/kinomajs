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
project(xslib)

if (WIN32)
	set(CMAKE_C_FLAGS "${BASE_CMAKE_C_FLAGS} /c /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /nologo /Zp1")
	set(CMAKE_C_FLAGS_DEBUG "/D _DEBUG /D mxDebug /D mxProfile /Fp$(TMP_DIR) /MTd /Od /W3 /Z7")
	set(CMAKE_C_FLAGS_RELEASE "/D NDEBUG /Fp$(TMP_DIR) /MT /O2 /W0")
else ()
	set(CMAKE_C_FLAGS "${BASE_CMAKE_C_FLAGS} -fno-common")
	set(CMAKE_C_FLAGS_DEBUG "-DmxDebug=1 -DmxProfile=1 -g -O1 -Wall -fstrict-aliasing -fstrict-overflow")
	set(CMAKE_C_FLAGS_RELEASE "-O3")
endif ()

file(GLOB sources ${XSLIB_DIR}/*.h)
list(APPEND sources ${XSLIB_DIR}/xsPlatform.c)
list(APPEND sources ${XSLIB_DIR}/xsAll.c)
list(APPEND sources ${XSLIB_DIR}/xsDebug.c)
list(APPEND sources ${XSLIB_DIR}/xsMemory.c)
list(APPEND sources ${XSLIB_DIR}/xsSymbol.c)
list(APPEND sources ${XSLIB_DIR}/xsScript.c)
list(APPEND sources ${XSLIB_DIR}/xsRun.c)
list(APPEND sources ${XSLIB_DIR}/xsType.c)
list(APPEND sources ${XSLIB_DIR}/xsProperty.c)
list(APPEND sources ${XSLIB_DIR}/xsGlobal.c)
list(APPEND sources ${XSLIB_DIR}/xsObject.c)
list(APPEND sources ${XSLIB_DIR}/xsFunction.c)
list(APPEND sources ${XSLIB_DIR}/xsArray.c)
list(APPEND sources ${XSLIB_DIR}/xsString.c)
list(APPEND sources ${XSLIB_DIR}/xsBoolean.c)
list(APPEND sources ${XSLIB_DIR}/xsNumber.c)
list(APPEND sources ${XSLIB_DIR}/xsMath.c)
list(APPEND sources ${XSLIB_DIR}/xsDate.c)
list(APPEND sources ${XSLIB_DIR}/xsRegExp.c)
list(APPEND sources ${XSLIB_DIR}/xsError.c)
list(APPEND sources ${XSLIB_DIR}/xsChunk.c)
list(APPEND sources ${XSLIB_DIR}/xsAPI.c)
list(APPEND sources ${XSLIB_DIR}/xsMarkup.c)
list(APPEND sources ${XSLIB_DIR}/xsGrammar.c)
list(APPEND sources ${XSLIB_DIR}/xsInfoSet.c)
list(APPEND sources ${XSLIB_DIR}/xs_dtoa.c)
list(APPEND sources ${XSLIB_DIR}/xs_pcre.c)
list(APPEND sources ${XSLIB_DIR}/xsProfile.c)
list(APPEND sources ${XSLIB_DIR}/xsMarshall.c)

add_library(xs STATIC ${sources})
install(TARGETS xs
	RUNTIME DESTINATION ${BIN_DIR}
	LIBRARY DESTINATION ${BIN_DIR}
	ARCHIVE DESTINATION ${BIN_DIR}
)

# vim: set ft=cmake:
