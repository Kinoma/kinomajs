#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
cmake_minimum_required(VERSION 2.8)
project(kpr2js)

set(XS_TARGET ${PROJECT_NAME})

set(CMAKE_C_FLAGS "${BASE_CMAKE_C_FLAGS} -fno-common -DFSKCONFIG_H=1 -DHAVE_MEMMOVE=1")
set(CMAKE_C_FLAGS_DEBUG "-DmxDebug=1 -g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O1")

set(XSC_BASE_OPTIONS -t ${XS_TARGET} -o ${GRAMMAR_DIR} -i ${F_HOME}/kinoma/kpr/extensions/templates)
set(XSC_OPTIONS_DEBUG ${XSC_BASE_OPTIONS} -d -v)
set(XSC_OPTIONS_RELEASE ${XSC_BASE_OPTIONS})
set(XSC_OPTIONS ${XSC_OPTIONS_${CMAKE_BUILD_TYPE}})

set(SRC_DIR ${XS_HOME}/sources/${PROJECT_NAME})
set(EXPAT_DIR ${F_HOME}/libraries/expat)

include_directories(${EXPAT_DIR})

file(GLOB sources ${XSLIB_DIR}/*.h)
list(APPEND sources ${SRC_DIR}/${PROJECT_NAME}.c)
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
list(APPEND sources ${XSLIB_DIR}/xsGrammar.c)
list(APPEND sources ${XSLIB_DIR}/xsInfoSet.c)
list(APPEND sources ${XSLIB_DIR}/xs_dtoa.c)
list(APPEND sources ${XSLIB_DIR}/xs_expat.c)
list(APPEND sources ${XSLIB_DIR}/xs_pcre.c)
list(APPEND sources ${XSLIB_DIR}/xsProfile.c)
list(APPEND sources ${XSLIB_DIR}/xsMarshall.c)
list(APPEND sources ${EXPAT_DIR}/xmlparse.c)
list(APPEND sources ${EXPAT_DIR}/xmlrole.c)
list(APPEND sources ${EXPAT_DIR}/xmltok.c)

XSC(xs_source ${SRC_DIR}/${PROJECT_NAME}.xs ${GRAMMAR_DIR})
set_source_files_properties(${xs_source} PROPERTIES GENERATED TRUE)
list(APPEND sources ${xs_source})

add_executable(${PROJECT_NAME} ${sources})
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	list(APPEND link_libraries -lc -ldl -lm)
endif ()
target_link_libraries(${PROJECT_NAME} ${link_libraries})

# vim: set ft=cmake:
