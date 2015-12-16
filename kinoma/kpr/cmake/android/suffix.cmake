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
add_custom_command(
	OUTPUT ${FSK_A_PATH}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUILD_TMP}/libfsk.a ${FSK_A_PATH}
	DEPENDS fsk
	)

add_custom_target(CopyFskLibrary DEPENDS ${FSK_A_PATH})

add_custom_target(
	ndk
	COMMAND ${CMAKE_COMMAND} -E remove ${FSK_SO_PATH} ${NDK_OBJ_PATH}/libFsk.so
	COMMAND env NDK_PROJECT_PATH=${NDK_PROJECT_PATH} KPR_TMP_DIR=${BUILD_TMP} SUPPORT_XS_DEBUG=${SUPPORT_XS_DEBUG} ${ANDROID_NDK}/ndk-build V=1
	DEPENDS CopyFskLibrary
	)

add_custom_command(
	OUTPUT ${NDK_PROJECT_PATH}/res/raw/kinoma.jet
	COMMAND ${CMAKE_COMMAND} -E make_directory ${NDK_PROJECT_PATH}/res/raw
	COMMAND zip -8qrn .jpg:.png:.m4a ${NDK_PROJECT_PATH}/res/raw/kinoma.jet "*"
	WORKING_DIRECTORY ${BIN_DIR}
	DEPENDS fsk
	)

add_custom_target(jet DEPENDS ${NDK_PROJECT_PATH}/res/raw/kinoma.jet)

add_custom_command(
	OUTPUT ${NDK_PROJECT_PATH}/bin/${KPR_APPLICATION}-${ANT_BUILD_TYPE}.apk
	COMMAND ant ${ANT_BUILD_TYPE}
	WORKING_DIRECTORY ${NDK_PROJECT_PATH}
	DEPENDS fsk jet ndk
	)

add_custom_target(apk DEPENDS ${NDK_PROJECT_PATH}/bin/${KPR_APPLICATION}-${ANT_BUILD_TYPE}.apk)

add_custom_command(
	OUTPUT ${BUILD_BIN}/${MANIFEST_NAME}.apk
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NDK_PROJECT_PATH}/bin/${KPR_APPLICATION}-${ANT_BUILD_TYPE}.apk ${BUILD_BIN}/${MANIFEST_NAME}.apk
	DEPENDS fsk apk
	)

add_custom_target(CopyApks ALL DEPENDS ${BUILD_BIN}/${MANIFEST_NAME}.apk)
