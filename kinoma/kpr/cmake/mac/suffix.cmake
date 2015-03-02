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
if (${CMAKE_GENERATOR} STREQUAL "Xcode")
	set_target_properties(${KPR_BINARY_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${BUILD_TMP}/Info.plist)
endif ()

add_custom_command(
	TARGET ${KPR_BINARY_NAME}
	POST_BUILD
 	COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_PATH}
 	COMMAND ${CMAKE_COMMAND} -E make_directory ${LANG_PATH}
 	COMMAND ${CMAKE_COMMAND} -E copy ${ICON_SRC} ${ICON_DEST}
 	COMMAND ${CMAKE_COMMAND} -E copy_directory ${NIB_SRC} ${NIB_DEST}
 	COMMAND ${CMAKE_COMMAND} -E copy ${PLIST_SRC} ${PLIST_DEST}
)

file(WRITE ${PKG_INFO} "APPLTINY")
