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
set(CMAKE_MODULE_PATH ${CONFIG_BASEDIR}/${TARGET_PLATFORM}/CMake-Modules)

enable_language(ASM-gas7)

set(BIN_DIR ${BUILD_BIN}/${MANIFEST_NAME}.app)
set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR ${BUILD_BIN})

if (${IOS_PLATFORM} MATCHES "OS")
	include(${CONFIG_BASEDIR}/${TARGET_PLATFORM}/device/CMakeLists.txt)
else ()
	include(${CONFIG_BASEDIR}/${TARGET_PLATFORM}/simulator/CMakeLists.txt)
endif ()

set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
set(CMAKE_XCODE_ATTRIBUTE_CURRENT_ARCH ${IOS_ARCH})
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
set(SDKROOT ${CMAKE_IOS_SDK_ROOT} CACHE STRING "Default CMake SDKROOT")

if (${CMAKE_GENERATOR} STREQUAL "Xcode")
	set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${SDK_MIN})
endif ()

foreach (ARCH ${IOS_ARCH})
	set(CMAKE_ASM-gas7_FLAGS "${CMAKE_ASM-gas7_FLAGS} -arch ${ARCH}")
endforeach ()

set(ENTITLEMENTS_LINK_FLAGS "-Xlinker -sectcreate -Xlinker __TEXT -Xlinker __entitlements -Xlinker ${BUILD_TMP}/entitlements.plist")

local_find_library(AddressBook)
local_find_library(AssetsLibrary)
local_find_library(AudioToolbox)
local_find_library(AVFoundation)
local_find_library(CoreAudio)
local_find_library(CoreData)
local_find_library(CoreGraphics)
local_find_library(CoreLocation)
local_find_library(CoreMedia)
local_find_library(CoreTelephony)
local_find_library(CoreText)
local_find_library(CoreVideo)
local_find_library(Foundation)
local_find_library(MediaPlayer)
local_find_library(OpenGLES)
local_find_library(QuartzCore)
local_find_library(SystemConfiguration)
local_find_library(UIKit)

list(APPEND EXTRA_LIBS -ObjC -lresolv -lstdc++ ${ENTITLEMENTS_LINK_FLAGS})
