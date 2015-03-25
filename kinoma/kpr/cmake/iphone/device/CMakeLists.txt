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
if (NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -miphoneos-version-min=${SDK_MIN}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -miphoneos-version-min=${SDK_MIN}")
endif ()

set(IOS_ARCH armv7 armv7s CACHE STRING "Supported architecture" FORCE)
set(CMAKE_OSX_ARCHITECTURES ${IOS_ARCH} CACHE STRING "Build architecture for iOS" FORCE)
set(ENTITLEMENTS "${BUILD_TMP}/entitlements.plist")
execute_process(COMMAND xcrun -f codesign_allocate
	OUTPUT_VARIABLE CODESIGN_ALLOCATE OUTPUT_STRIP_TRAILING_WHITESPACE)
if (EXISTS ${ENTITLEMENTS})
	set(ENTITLEMENTS_COMMAND --entitlements ${ENTITLEMENTS})
endif ()
set(CODESIGN env CODESIGN_ALLOCATE=${CODESIGN_ALLOCATE} codesign -f -s "${CODESIGN_ID}" ${ENTITLEMENTS_COMMAND} ${BIN_DIR})
set(PACKAGE xcrun -sdk iphoneos PackageApplication ${BUILD_BIN}/${MANIFEST_NAME}.app -o ${BUILD_BIN}/${MANIFEST_NAME}.ipa)
