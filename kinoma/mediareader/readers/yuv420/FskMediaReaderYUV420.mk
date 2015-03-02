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
<input name="../../sources/"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>
<input name="$(F_HOME)/kinoma/mediareader/readers/camera-android/sources"/>

<wrap name="FskMediaReaderYUV420Extension.c"/>
<wrap name="FskMediaReaderYUV420.c"/>
<wrap name="kinoma_utilities_buf_print.c"/>
<!--wrap name="jsmn.c"/-->
  
<platform name="android-cmake">
<common>
set(LIBRARY_OUTPUT_PATH ${F_HOME}/build/android/inNDK/Play/project/libs/armeabi)

include_directories(${BUILD_TMP})

link_directories(${NDK_LIBS_PATH})

set(EXTENSION_LINK_LIBS -lFsk)

set(TARGET_DEPENDENCIES FskLib)
</common>
</platform>    
    
<include name="/makefiles/xsLibrary.mk"/>

</makefile>