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

<wrap name="androidakvm.c"/>
<wrap name="androidfb.cpp"/>
<wrap name="androidAudio.cpp"/>
<wrap name="androidAudioIn.cpp"/>

<platform name="android">
<common>
OSS = $(F_HOME)/build/android/OSS/

C_OPTIONS += \
    -g \
    -DANDROID_VERSION=2   \
    -DTARGET_OS_ANDROID=1   \
    -DHAVE_ENDIAN_H=1   \
    -I$(F_HOME)tmp/include \
    -I$(F_HOME)tmp/android	\
    -I$(OSS)frameworks/base/include \
    -I$(OSS)frameworks/base/include/utils/ \
    -I$(OSS)frameworks/base/include/ui/ \
    -I$(OSS)hardware/libhardware/include/ \
    -I$(OSS)system/core/include/ \
    -I$(OSS)external/skia/include/  

LINKER = $(deviceLINK)
LIBRARIES += -L$(OSS)lib/ -lui -lmedia -llog

# LIBRARIES += -L$(OSS)lib/ -lui -lmedia
</common>
</platform>

<platform name="android-cmake">
<common>
set(LIBRARY_OUTPUT_PATH ${F_HOME}/build/android/inNDK/Play/project/libs/armeabi)

set(OSS ${F_HOME}/build/android/OSS)

add_definitions(-DANDROID_VERSION=2)
add_definitions(-DTARGET_OS_ANDROID=1)
add_definitions(-DHAVE_ENDIAN_H=1)

include_directories(${BUILD_TMP})
include_directories(${OSS}/frameworks/base/include)
include_directories(${OSS}/frameworks/base/include/utils)
include_directories(${OSS}/frameworks/base/include/ui)
include_directories(${OSS}/hardware/libhardware/include)
include_directories(${OSS}/system/core/include)
include_directories(${OSS}/external/skia/include  )

link_directories(${OSS}/lib)
link_directories(${NDK_LIBS_PATH})

set(EXTENSION_LINK_LIBS -lFsk -llog -lui -lmedia)

set(TARGET_DEPENDENCIES FskLib)
</common>
</platform>



<include name="/makefiles/xsLibrary.mk"/>

</makefile>