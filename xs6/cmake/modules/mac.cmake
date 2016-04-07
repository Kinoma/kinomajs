#
#     Copyright (C) 2010-2016 Marvell International Ltd.
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

	string(REPLACE " " "_" APPLICATION ${LOCAL_APPLICATION})

	add_executable(${APPLICATION} MACOSX_BUNDLE ${SOURCES} ${FskPlatform_SOURCES} ${F_HOME}/xs6/patches/main.m)
	target_link_libraries(${APPLICATION} ${LIBRARIES} ${OBJECTS} -ObjC)
	target_include_directories(${APPLICATION} PUBLIC ${C_INCLUDES})
	target_compile_definitions(${APPLICATION} PUBLIC ${C_DEFINITIONS})
	target_compile_options(${APPLICATION} PUBLIC ${C_OPTIONS})

	set_target_properties(${APPLICATION} PROPERTIES OUTPUT_NAME "${LOCAL_APPLICATION}")

	set(MACOSX_BUNDLE_INFO_STRING "${LOCAL_APPLICATION} ${APP_VERSION} Copyright © ${LOCAL_YEAR} Marvell Semiconductor, Inc.")
	set(MACOSX_BUNDLE_ICON_FILE "fsk.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "${LOCAL_NAMESPACE}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${LOCAL_APPLICATION} ${APP_VERSION}")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${LOCAL_APPLICATION}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}")

	set(APP_RES_DIR ${APP_DIR}/../Resources)
	# set(BINARY_DIR


	add_custom_command(
		TARGET ${APPLICATION}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_RES_DIR}
		COMMAND ${CMAKE_COMMAND} -E rename ${APP_DIR}/FskManifest.xsa ${APP_RES_DIR}/FskManifest.xsa
		COMMAND ${CMAKE_COMMAND} -E copy ${ICNS} ${APP_RES_DIR}/fsk.icns
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${NIB} ${APP_RES_DIR}/English.lproj/fsk.nib
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_RES_DIR} ${BUILD_APP_DIR}/../Resources
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APPLICATION}> ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${APPLICATION}>/../Info.plist ${APP_DIR}/../
		COMMAND ${CMAKE_COMMAND} -E echo "APPLTINY" > ${APP_DIR}/../PkgInfo
		)
endmacro()
