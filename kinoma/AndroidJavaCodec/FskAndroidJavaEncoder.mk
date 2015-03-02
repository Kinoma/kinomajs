<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<makefile>

<include name="/makefiles/xsFskDefaults.mk"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>
<input name="$(F_HOME)/tools/device_scan/"/>

<wrap name="sources/FskAndroidJavaCommon.c"/>
<wrap name="sources/FskAndroidJavaEncoderExtension.c"/>
<wrap name="sources/FskAndroidVideoEncoder.c"/>

<platform name="android">
<common>
LIBRARIES += \
        -L$(NDK_PROJECT_LIBRARIES)  \
		-lKinomaLibG


C_OPTIONS += -DTARGET_OS_LINUX=1		    \
			-O3								\
			-Wall							\
			-mabi=aapcs-linux				\
			-fPIC							\
			-D_IPP_LINUX                    \
			-DFSK_JAVA_NAMESPACE=\"${FSK_JAVA_NAMESPACE}\" \
			-DCLASSNAME=\"${FSK_JAVA_NAMESPACE}/MediaCodecCore\"
</common>
</platform>

<platform name="android-cmake">
<common>
set(LIBRARY_OUTPUT_PATH ${F_HOME}/build/android/inNDK/Play/project/libs/armeabi)

add_definitions(-DTARGET_OS_LINUX=1)
add_definitions(-D_IPP_LINUX)
add_definitions(-DCLASSNAME="com/marvell/kinoma/kinomaplay/MediaCodecCore")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIC -mabi=aapcs-linux")

link_directories(${F_HOME}/build/android/inNDK/Play/project/libs/armeabi)
link_directories(${NDK_LIBS_PATH})

set(EXTENSION_LINK_LIBS -lFsk -lKinomaLibG)

set(TARGET_DEPENDENCIES FskLib)
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>
</makefile>