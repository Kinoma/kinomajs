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
set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Default Architecture")
set(SDKVER "10.9")

macro(BUILD)
	set(oneValueArgs APPLICATION NAMESPACE YEAR)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_executable(${LOCAL_APPLICATION} MACOSX_BUNDLE ${SOURCES} ${FskPlatform_SOURCES} ${F_HOME}/xs6/patches/main.m)
	target_link_libraries(${LOCAL_APPLICATION} ${LIBRARIES} ${OBJECTS} -ObjC)
	target_include_directories(${LOCAL_APPLICATION} PUBLIC ${C_INCLUDES})
	target_compile_definitions(${LOCAL_APPLICATION} PUBLIC ${C_DEFINITIONS})
	target_compile_options(${LOCAL_APPLICATION} PUBLIC ${C_OPTIONS})

	set(MACOSX_BUNDLE_INFO_STRING "Kinoma Simulator 2.0.0 Copyright © ${LOCAL_YEAR} Marvell Semiconductor, Inc.")
	set(MACOSX_BUNDLE_ICON_FILE "fsk.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "${LOCAL_NAMESPACE}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "Kinoma Simulator 2.0.0")
	set(MACOSX_BUNDLE_BUNDLE_NAME "Kinoma Simulator")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0.0")

	add_custom_command(
		TARGET ${LOCAL_APPLICATION}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${ICNS} ${APP_DIR}/../Resources/fsk.icns
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${NIB} ${APP_DIR}/../Resources/English.lproj/fsk.nib
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_DIR}/../Resources ${BUILD_APP_DIR}/../Resources
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${LOCAL_APPLICATION}> ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_DIR} ${BUILD_APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${LOCAL_APPLICATION}>/../Info.plist ${APP_DIR}/../
		COMMAND ${CMAKE_COMMAND} -E echo "APPLTINY" > ${APP_DIR}/../PkgInfo
		)
endmacro()
