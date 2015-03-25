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
<input name="../../sources/"/>
<input name="sdk/include/"/>
<input name="sdk/include/FLAC/"/>
<input name="sdk/include/share/"/>
<input name="sdk/include/share/grabbag/"/>
<input name="sdk/src/libFLAC/"/>
<input name="sdk/src/libFLAC/include/"/>
<input name="sdk/src/libFLAC/include/private/"/>
<input name="sdk/src/libFLAC/include/protected/"/>

<wrap name="FskMediaReaderFLACExtension.c"/>
<wrap name="FskMediaReaderFLAC.c"/>

<wrap name="sdk/src/libFLAC/bitmath.c"/>
<wrap name="sdk/src/libFLAC/bitreader.c"/>
<wrap name="sdk/src/libFLAC/cpu.c"/>
<wrap name="sdk/src/libFLAC/crc.c"/>
<wrap name="sdk/src/libFLAC/fixed.c"/>
<wrap name="sdk/src/libFLAC/float.c"/>
<wrap name="sdk/src/libFLAC/format.c"/>
<wrap name="sdk/src/libFLAC/FLAC_lpc.c"/>
<wrap name="sdk/src/libFLAC/md5.c"/>
<wrap name="sdk/src/libFLAC/memory.c"/>
<wrap name="sdk/src/libFLAC/metadata_object.c"/>
<wrap name="sdk/src/libFLAC/stream_decoder.c"/>
<wrap name="sdk/src/libFLAC/window.c"/>

<platform name="Windows">
<common>
C_OPTIONS = /D FLAC__INTEGER_ONLY_LIBRARY /D FLAC__LPC_UNROLLED_FILTER_LOOPS /D FLAC__NO_DLL /D VERSION=\"1.2.0\" $(C_OPTIONS)
</common>
</platform>
	  
<platform name="MacOSX,iPhone">
<common>
C_OPTIONS = \
	-D__FSK_LAYER__=1 \
	-DFLAC__INTEGER_ONLY_LIBRARY \
	-DFLAC__LPC_UNROLLED_FILTER_LOOPS \
	-DFLAC__NO_DLL \
	-DVERSION=\"1.2.0\"
</common>
</platform>

<platform name="mac-cmake">
<common>
add_definitions(-D__FSK_LAYER__=1)
add_definitions(-DFLAC__INTEGER_ONLY_LIBRARY)
add_definitions(-DFLAC__LPC_UNROLLED_FILTER_LOOPS)
add_definitions(-DFLAC__NO_DLL)
add_definitions(-DVERSION=\"1.2.0\")
</common>
</platform>

<platform name="Linux">
<common>
C_OPTIONS = \
	-D__FSK_LAYER__=1 \
	-DFLAC__INTEGER_ONLY_LIBRARY \
	-DFLAC__LPC_UNROLLED_FILTER_LOOPS \
	-DFLAC__NO_DLL \
	-DVERSION=\"1.2.0\"
</common>
</platform>

<platform name="arm-linux">
<common>
C_OPTIONS = \
	-D__FSK_LAYER__=1 \
	-DFLAC__INTEGER_ONLY_LIBRARY \
	-DFLAC__LPC_UNROLLED_FILTER_LOOPS \
	-DFLAC__NO_DLL \
	-DVERSION=\"1.2.0\"
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>