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
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
#pkg_check_modules(GLIB2 REQUIRED glib-2.0)

# include_directories(${GTK3_INCLUDE_DIRS})
# link_directories(${GTK3_LIBRARY_DIRS})

#include_directories(${GLIB2_INCLUDE_DIRS})
#link_directories(${GLIB2_LIBRARY_DIRS})

#list(APPEND LIBRARIES ${GTK3_LIBRARIES} ${GLIB2_LIBRARIES})

set(FREETYPE_VERSION 2.6)
set(FREETYPE_DIR ${TMP_DIR}/freetype-${FREETYPE_VERSION})

macro(BUILD)
	set(oneValueArgs APPLICATION)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	file(MAKE_DIRECTORY ${FREETYPE_DIR})

	add_subdirectory(${F_HOME}/libraries/freetype/xs6 FreeType)

	add_executable(${KPR_APPLICATION} ${SOURCES} ${FskPlatform_SOURCES} ${TARGET_OBJECTS})
	add_dependencies(${KPR_APPLICATION} FreeType)
	target_include_directories(${KPR_APPLICATION}  PRIVATE ${C_INCLUDES})
	target_compile_definitions(${KPR_APPLICATION} PRIVATE ${C_DEFINITIONS})
	target_compile_options(${KPR_APPLICATION} PRIVATE ${C_OPTIONS})
	target_link_libraries(${KPR_APPLICATION} -Wl,--whole-archive -Wl,-Map,${TMP_DIR}/${KPR_APPLICATION}.map ${OBJECTS} ${LIBRARIES})

	if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		add_custom_command(
			TARGET ${KPR_APPLICATION}
			POST_BUILD
			COMMAND ${TOOL_PREFIX}strip $<TARGET_FILE:${KPR_APPLICATION}>
			)
	endif()

	add_custom_target(
		Assemble
		ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${KPR_APPLICATION}> ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${TMP_DIR}/app ${APP_DIR}
		DEPENDS ${KPR_APPLICATION} FskManifest.xsa
		)
endmacro()
# vim: ft=cmake
