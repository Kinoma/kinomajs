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
if (${CMAKE_GENERATOR} STREQUAL "Xcode")
	set_target_properties(${KPR_BINARY_NAME} PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")

	if (${IOS_PLATFORM} STREQUAL "OS")
		set_target_properties(${KPR_BINARY_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODESIGN_ID}")
		set_target_properties(${KPR_BINARY_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${BUILD_TMP}/Info.plist)
		set_target_properties(${KPR_BINARY_NAME} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER ${MANIFEST_NAMESPACE})

		if (${IOS_PLATFORM} STREQUAL "OS")
			add_custom_command(
				TARGET ${KPR_BINARY_NAME}
				POST_BUILD
				COMMAND ${PACKAGE}
				WORKING_DIRECTORY ${BUILD_BIN}
			)
		endif ()
	else ()
		add_custom_command(
			TARGET ${KPR_BINARY_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_TMP}/Info.plist ${BIN_DIR}/Info.plist
		)
	endif ()
else ()
	add_custom_command(
		TARGET ${KPR_BINARY_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${MANIFEST_NAME}.app/Contents/MacOS/${MANIFEST_NAME} ${BIN_DIR}
		COMMAND ${CMAKE_COMMAND} -E echo ${CODESIGN}
		COMMAND ${CODESIGN}
		COMMAND ${CMAKE_COMMAND} -E echo ${PACKAGE}
		COMMAND ${PACKAGE}
		VERBATIM
	)
	add_custom_command(
		TARGET ${KPR_BINARY_NAME}
		POST_BUILD
		COMMAND dsymutil ${BIN_DIR}/${MANIFEST_NAME} -o ${BIN_DIR}.dSYM
	)
endif ()
