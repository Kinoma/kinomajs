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
set(CMAKE_OSX_ARCHITECTURES i386)
set(CMAKE_XCODE_ATTRIBUTE_CURRENT_ARCH i386)

set(SDKVER 10.9 CACHE STRING "Mac SDK Version")

set(ICON_NAME fsk.icns)
set(NIB_NAME fsk.nib)
set(MAC_HOME ${F_HOME}/build/mac)
set(APP_PATH ${BUILD_BIN}/${MANIFEST_NAME}.app)
set(CONTENTS_PATH ${APP_PATH}/Contents)
set(BIN_DIR ${CONTENTS_PATH}/MacOS)
set(RESOURCE_PATH ${CONTENTS_PATH}/Resources)
set(ICON_SRC ${MAC_HOME}/${ICON_NAME})
set(ICON_DEST ${RESOURCE_PATH}/${MANIFEST_NAME}.icns)
set(LANG_PATH ${RESOURCE_PATH}/English.lproj)
set(NIB_SRC ${MAC_HOME}/${NIB_NAME})
set(NIB_DEST ${LANG_PATH}/${NIB_NAME})
set(PKG_INFO ${CONTENTS_PATH}/PkgInfo)
set(PLIST_SRC ${BUILD_TMP}/Info.plist)
set(PLIST_DEST ${CONTENTS_PATH}/Info.plist)

if (CMAKE_GENERATOR STREQUAL "Xcode")
	set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR ${BUILD_BIN})
else ()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN})
endif ()
