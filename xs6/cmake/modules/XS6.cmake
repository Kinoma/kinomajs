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
include(CMakeParseArguments)
include(Kinoma)

macro(FIND_XS_TOOL VAR EXEC)
	get_platform(BUILD_PLATFORM)
	find_program(
		${VAR}
		${EXEC}
		PATHS
		${XS6}/bin/${BUILD_PLATFORM}/Release
		${XS6}/bin/${BUILD_PLATFORM}/Debug
		)
endmacro()

macro(XS2JS)
	set(oneValueArgs SOURCE DESTINATION)
	set(multiValueArgs OPTIONS)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)
	set(OUTPUT ${LOCAL_DESTINATION}/${NAME}.js)
	if(XS_BUILD)
		set(DEPENDS xsr)
	endif()

	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND ${XS2JS} ${LOCAL_SOURCE} ${LOCAL_OPTIONS} -p -o ${LOCAL_DESTINATION}
		DEPENDS ${LOCAL_SOURCE} ${DEPENDS}
		)
endmacro()

# Run XSC against a JS file
#
# SOURCE_FILE: The location of the .js file
# DESTINATION: The temp directory to put these files under
# XSC_OPTIONS: options to pass to xsc other than -o
# The following two options are used for xs6 tools and not needed for kprconfig
# SOURCE_DIR: Directory to search for source files
# SOURCE: Path under SOURCE_DIR to find the file without the .js
macro(XSC)
	set(oneValueArgs SOURCE_DIR SOURCE SOURCE_FILE DESTINATION)
	set(multiValueArgs OPTIONS)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	if(LOCAL_SOURCE_FILE)
		get_filename_component(NAME ${LOCAL_SOURCE_FILE} NAME_WE)
		set(SOURCE_FILE ${LOCAL_SOURCE_FILE})
		set(OUTDIR ${LOCAL_DESTINATION})
	else()
		get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)
		get_filename_component(BASE ${LOCAL_SOURCE} PATH)
		set(SOURCE_FILE ${LOCAL_SOURCE_DIR}/${LOCAL_SOURCE}.js)
		set(OUTDIR ${LOCAL_DESTINATION}/${BASE})
	endif()
	set(OUTPUT ${OUTDIR}/${NAME}.xsb)
	if(XS_BUILD)
		set(DEPENDS xsc)
	endif()
	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTDIR}
		COMMAND ${XSC} ${LOCAL_OPTIONS} ${SOURCE_FILE} -o ${OUTDIR}
		DEPENDS ${SOURCE_FILE} ${DEPENDS}
		)
endmacro()

# Use XSL to create an XSA file from XSB files
#
# NAME: The name to send to -a
# TMP: Location of .xsb files
# DESTINATION: The bin directory for the xsa
# SOURCES: A list of xsb files
# SRC_DIR: Move generated source files into another directory
macro(XSL)
	set(oneValueArgs NAME TMP DESTINATION SRC_DIR)
	set(multiValueArgs SOURCES XSC_OPTIONS DEPENDS COPY)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	set(OUTPUT ${LOCAL_DESTINATION}/${LOCAL_NAME}.xsa)
	if(XS_BUILD)
		list(APPEND LOCAL_DEPENDS xsl)
	endif()
	add_custom_command(
		OUTPUT ${OUTPUT} ${LOCAL_TMP}/${LOCAL_NAME}.xs.c ${LOCAL_TMP}/${LOCAL_NAME}.xs.h
		COMMAND ${CMAKE_COMMAND} -E make_directory ${LOCAL_DESTINATION}
		COMMAND ${XSL} -a ${LOCAL_NAME} -b ${LOCAL_TMP} -o ${LOCAL_DESTINATION} ${LOCAL_SOURCES}
		DEPENDS ${LOCAL_SOURCES} ${LOCAL_DEPENDS}
		)
	add_custom_target(
		${LOCAL_NAME}.xsa
		DEPENDS ${OUTPUT}
		)
	if(LOCAL_SRC_DIR)
		add_custom_command(
			TARGET ${LOCAL_NAME}.xsa
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${LOCAL_SRC_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LOCAL_TMP}/${LOCAL_NAME}.xs.c ${LOCAL_SRC_DIR}/${LOCAL_NAME}.xs.c
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LOCAL_TMP}/${LOCAL_NAME}.xs.h ${LOCAL_SRC_DIR}/${LOCAL_NAME}.xs.h
			COMMAND ${CMAKE_COMMAND} -E remove  ${LOCAL_TMP}/${LOCAL_NAME}.xs.c ${LOCAL_TMP}/${LOCAL_NAME}.xs.h
			)
	endif()
	if(LOCAL_COPY)
		foreach(DIR ${LOCAL_COPY})
			add_custom_command(
				TARGET ${LOCAL_NAME}.xsa
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory ${DIR}
				COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${DIR}
				)
		endforeach()
	endif()
endmacro()

macro(KPR2JS)
	set(oneValueArgs SOURCE DESTINATION)
	set(multiValueArgs DEPENDS)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)
	set(OUTPUT ${LOCAL_DESTINATION}/${NAME}.js)
	if(XS_BUILD)
		set(DEPENDS xsr tools)
	endif()
	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${LOCAL_DESTINATION}
		COMMAND ${KPR2JS} ${LOCAL_SOURCE} -o ${LOCAL_DESTINATION}
		DEPENDS ${LOCAL_SOURCE} ${LOCAL_DEPENDS} ${DEPENDS}
		)
endmacro()
