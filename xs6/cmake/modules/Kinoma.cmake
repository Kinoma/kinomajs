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
# Simple Copy tool
macro(COPY)
	set(oneValueArgs SOURCE DESTINATION)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(SOURCE_NAME ${LOCAL_SOURCE} NAME)
	get_filename_component(DESTIANTION_NAME ${LOCAL_DESTINATION} NAME)
	get_filename_component(DIRECTORY ${LOCAL_DESTINATION} PATH)

	if ("${PLATFORM}" STREQUAL "win")
		add_custom_command(
				OUTPUT ${LOCAL_DESTINATION}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${DIRECTORY}
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LOCAL_SOURCE} ${LOCAL_DESTINATION}
				DEPENDS ${LOCAL_SOURCE}
				)
		list(APPEND WIN_COPY_FILES ${LOCAL_DESTINATION})
	else ()
		file(COPY ${LOCAL_SOURCE} DESTINATION ${DIRECTORY})
		if(NOT ${SOURCE_NAME} STREQUAL ${DESTIANTION_NAME})
			file(RENAME ${DIRECTORY}/${SOURCE_NAME} ${DIRECTORY}/${DESTIANTION_NAME})
		endif()
	endif()
endmacro()

# Finds a library or framework and appends to the list of variables to link agains
#
# LIBNAME: the name of the library or framework to find
# LIST: the list to append the library too once found
macro(LOCAL_FIND_LIBRARY)
	set(onevalueArgs LIBNAME LIST OPTION)
	set(options OPTIONAL EXIT)
	cmake_parse_arguments(LOCAL "${options}" "${onevalueArgs}" "${multiValueArgs}" ${ARGN})

	string(TOUPPER ${LOCAL_LIBNAME} LIB_NAME)
	find_library(${LIB_NAME} ${LOCAL_LIBNAME} ${LOCAL_OPTION})
	if(${${LIB_NAME}} STREQUAL ${LIB_NAME}-NOTFOUND)
	 	if(NOT ${LOCAL_EXIT})
			local_find_library(LIBNAME ${LOCAL_LIBNAME} LIST ${LOCAL_LIST} OPTION NO_CMAKE_FIND_ROOT_PATH EXIT true)
	 	else()
	 		message(FATAL_ERROR ": ${LOCAL_LIBNAME} not found")
	 	endif()
	else()
		if(APPLE)
			get_filename_component(EXTENSION ${${LIB_NAME}} EXT)
			if(${EXTENSION} STREQUAL ".framework")
				mark_as_advanced(${LIB_NAME})
			endif()
		endif()
		list(APPEND ${LOCAL_LIST} ${${LIB_NAME}})
	endif()
endmacro()

# Gets the current running platform
#
# VARIABLE: Output variable for platform name
macro(GET_PLATFORM VARIABLE)
	if(UNIX)
		if(APPLE)
			set(${VARIABLE} "mac")
		else()
			if(FORCE_32BIT)
				set(${VARIABLE} "linux/i686")
				set(CMAKE_C_FLAGS "-m32")
			else()
				execute_process(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
				set(${VARIABLE} "linux/${ARCHITECTURE}")
			endif()
		endif ()
	elseif(WIN32 OR CYGWIN)
		set(${VARIABLE} "win")
	endif()
endmacro()

macro(FIX_FLAGS)
	set(oneValueArgs FLAGS)
	set(options CXX)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	if (NOT CXX)
		string(REPLACE "-fno-rtti" "" ${LOCAL_FLAGS} "${${LOCAL_FLAGS}}")
	endif ()
endmacro()
