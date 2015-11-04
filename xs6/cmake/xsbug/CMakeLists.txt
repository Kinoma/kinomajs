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
project(XSBUG)

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/build)
set(KPRCONFIG_OPTIONS -x -t ${BUILD_DIR})

if(UNIX)
	if(APPLE)
		set(OUTPUT ${XS6_TOOLS_BIN}/xsbug.app/Contents/MacOS/fsk)
	else()
		if(NOT "${ARCHITECTURE}" STREQUAL "x86_64")
			file(MAKE_DIRECTORY ${XS6_TOOLS_BIN}/xsbug)
			list(APPEND KPRCONFIG_OPTIONS -b ${XS6_TOOLS_BIN}/xsbug)
			set(OPTIONS -p linux/gtk)
			set(OUTPUT ${XS6_TOOLS_BIN}/xsbug/Kpl)
		endif()
	endif()
elseif(WIN32)
	set(OUTPUT ${XS6_TOOLS_BIN}/xsbug/xsbug.exe)
endif()

if(NOT NO_BUILD)
	list(APPEND KPRCONFIG_OPTIONS -m)
endif()

set(KPRCONFIG_OPTIONS_RELEASE ${KPRCONFIG_OPTIONS})
set(KPRCONFIG_OPTIONS_DEBUG ${KPRCONFIG_OPTIONS} -d -i)

file(MAKE_DIRECTORY ${BUILD_DIR})

add_custom_target(
	xsbug
	ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}/xsbug
	COMMAND ${CMAKE_COMMAND} -E echo ${KPRCONFIG} ${OPTIONS} ${KPRCONFIG_OPTIONS_${BUILD_TYPE}} -o ${XS6} ${XS6}/xsbug/manifest.json
	COMMAND ${XS6_TOOLS_BIN}/xsr6 -a ${XS6_MODULES_DIR}/tools.xsa kprconfig ${OPTIONS} ${KPRCONFIG_OPTIONS_${BUILD_TYPE}} -o ${XS6} ${XS6}/xsbug/manifest.json
	COMMENT "Creating xsbug"
	DEPENDS tools xsc xsl xsr
	)
