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
set(CMAKE_USER_MAKE_RULES_OVERRIDE
	${F_HOME}/xs6/cmake/overrides/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
	${F_HOME}/xs6/cmake/overrides/cxx_flag_overrides.cmake)

macro(BUILD)
	add_custom_target(copy_resources DEPENDS ${WIN_COPY_FILES})
	list(APPEND SOURCES ${RESOURCE})
	add_executable(${APP_NAME} WIN32 ${SOURCES} ${FskPlatform_SOURCES})
	target_link_libraries(${APP_NAME} ${LIBRARIES} ${OBJECTS})
	add_dependencies(${APP_NAME} copy_resources)

	add_custom_command(
		TARGET ${APP_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME}> ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_DIR}/ $<TARGET_FILE_DIR:${APP_NAME}>
		)
endmacro()
