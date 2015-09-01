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
PROJECT(XSBUG)

STRING(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

SET(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/build)
SET(KPRCONFIG_OPTIONS -x -t ${BUILD_DIR})

IF(UNIX)
	IF(APPLE)
		SET(OUTPUT ${XS6_TOOLS_BIN}/xsbug.app/Contents/MacOS/fsk)
	ELSE()
		IF(NOT "${ARCHITECTURE}" STREQUAL "x86_64")
			FILE(MAKE_DIRECTORY ${XS6_TOOLS_BIN}/xsbug)
			LIST(APPEND KPRCONFIG_OPTIONS -b ${XS6_TOOLS_BIN}/xsbug)
			SET(OPTIONS -p linux/gtk)
			SET(OUTPUT ${XS6_TOOLS_BIN}/xsbug/Kpl)
		ENDIF()
	ENDIF()
ELSEIF(WIN32)
	SET(OUTPUT ${XS6_TOOLS_BIN}/xsbug/xsbug.exe)
ENDIF()

IF(NOT NO_BUILD)
	LIST(APPEND KPRCONFIG_OPTIONS -m)
ENDIF()

SET(KPRCONFIG_OPTIONS_RELEASE ${KPRCONFIG_OPTIONS})
SET(KPRCONFIG_OPTIONS_DEBUG ${KPRCONFIG_OPTIONS} -d -i)

FILE(MAKE_DIRECTORY ${BUILD_DIR})

IF(OUTPUT)
	SET(PATH "$ENV{PATH}")
	ADD_CUSTOM_COMMAND(
		OUTPUT ${OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
		COMMAND ${KPRCONFIG} ${OPTIONS} ${KPRCONFIG_OPTIONS_${BUILD_TYPE}} -o ${XS6} ${XS6}/xsbug/manifest.json
		COMMENT "Creating xsbug"
		DEPENDS tools xsc xsl xsr
		)
	ADD_CUSTOM_TARGET(xsbug ALL DEPENDS ${OUTPUT})
ENDIF()
