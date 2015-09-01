<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<makefile>

<include name="/makefiles/xsFskDefaults.mk"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>
<input name="$(F_HOME)/tools/device_scan/"/>

<wrap name="sources/FskAndroidJavaCommon.c"/>
<wrap name="sources/FskAndroidJavaDecoderExtension.c"/>
<wrap name="sources/FskAndroidVideoDecoder.c"/>
<wrap name="sources/FskAndroidAudioDecoder.c"/>

<platform name="android">
<common>
LIBRARIES += \
		-L$(NDK_PROJECT_LIBRARIES)	\
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

<include name="/makefiles/xsLibrary.mk"/>
</makefile>
