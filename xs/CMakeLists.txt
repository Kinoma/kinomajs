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

if (${CMAKE_MAJOR_VERSION} GREATER 2)
	cmake_policy(SET CMP0037 OLD)
endif()

project(xs)

if (WIN32)
	set(CMAKE_COLOR_MAKEFILE OFF)
endif ()

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

file(TO_CMAKE_PATH ${F_HOME} F_HOME)
file(TO_CMAKE_PATH ${XS_HOME} XS_HOME)
file(TO_CMAKE_PATH ${BIN_DIR} BIN_DIR)
file(TO_CMAKE_PATH ${TMP_DIR} TMP_DIR)
set(CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
set(XSLIB_DIR ${XS_HOME}/sources/xslib)
set(GRAMMAR_DIR ${TMP_DIR}/grammars)
set(LIBTOMCRYPT ${F_HOME}/extensions/crypt/sources/libs/libtomcrypt-1.13)

set(BASE_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
set(BASE_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

include_directories(${LIBTOMCRYPT}/src/headers)
include_directories(${LIBTOMCRYPT}/src/ciphers)
include_directories(${LIBTOMCRYPT}/src/ciphers/aes)
include_directories(includes)
include_directories(${XSLIB_DIR})
include_directories(${GRAMMAR_DIR})

macro (XSC OUTPUT_VAR SOURCES OUTPUT_DIR)
	set(${OUTPUT_VAR})
	foreach (SOURCE ${SOURCES})
		get_filename_component(NAME ${SOURCE} NAME)
		get_filename_component(DIRECTORY ${SOURCE} PATH)
		file(GLOB XS_SOURCE "${DIRECTORY}/*.xs")
		add_custom_command(
			OUTPUT ${OUTPUT_DIR}/${NAME}.c ${OUTPUT_DIR}/${NAME}.h
			COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
			COMMAND xsc ${SOURCE} ${XSC_OPTIONS}
			DEPENDS xsc ${XS_SOURCE}
		)
		add_custom_target(${NAME} DEPENDS ${OUTPUT_DIR}/${NAME}.c ${OUTPUT_DIR}/${NAME}.h)
		list(APPEND ${OUTPUT_VAR} ${OUTPUT_DIR}/${NAME}.c ${OUTPUT_DIR}/${NAME}.h)
	endforeach ()
endmacro ()

add_subdirectory(${CMAKE_DIR}/kpr2js)
add_subdirectory(${CMAKE_DIR}/kprconfig)
add_subdirectory(${CMAKE_DIR}/xsc)
add_subdirectory(${CMAKE_DIR}/xslib)
