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

<input name="$(F_HOME)/libraries/QTReader"/>

<platform name="MacOSX,mac-cmake,iPhone">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/stublib/"/>
</platform>

<platform name="Windows">
<input name="$(F_HOME)/libraries/DirectX"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/stublib/"/>
</platform>

<platform name="Linux">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/lib/"/>
</platform>

<platform name="android">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

  
<input name="$(F_HOME)/kinoma/intel/codec/mp3_dec_int/include/"/>
<input name="$(F_HOME)/kinoma/intel/codec/common/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>


<include name="/makefiles/xsFskDefaults.mk"/>

<wrap name="sources/kinomamp3ippextension.c"/>
<wrap name="sources/kinomamp3ippdecode.cpp"/>

<wrap name="../intel/codec/mp3_dec_int/src/mp3dec_api_int.c"/>
<wrap name="../intel/codec/mp3_dec_int/src/mp3dec_int.cpp"/>
<wrap name="../intel/codec/mp3_dec_int/src/mp3dec_layer1_int.c"/>
<wrap name="../intel/codec/mp3_dec_int/src/mp3dec_layer2_int.c"/>
<wrap name="../intel/codec/mp3_dec_int/src/mp3dec_layer3_int.c"/>

<wrap name="../intel/codec/common/src/bstream.c"/>
<wrap name="../intel/codec/common/src/mp3dec_alloc_tab.c"/>
<wrap name="../intel/codec/common/src/mp3dec_api.c"/>
<wrap name="../intel/codec/common/src/mp3dec_common.c"/>
<wrap name="../intel/codec/common/src/mp3dec_huftabs.c"/>
<wrap name="../intel/codec/common/src/mp3dec_layer1.c"/>
<wrap name="../intel/codec/common/src/mp3dec_layer2.c"/>
<wrap name="../intel/codec/common/src/mp3dec_layer3.c"/>

<wrap name="../intel/core/umc/src/umc_audio_codec.cpp"/>
<wrap name="../intel/core/umc/src/umc_media_data.cpp"/>

<!--buildstyle name="nonembed"-->
  
<!--platform name="Windows">
<wrap name="../intel/core/vm/src/vm_debug_win32.c"/>
<wrap name="../intel/core/vm/src/vm_trace_log_win32.c"/>
</platform>

<platform name="MacOSX,mac-cmake,Linux,iPhone,android">
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_trace_log_linux32.c"/>
</platform>-->  
  
<platform name="android">
<!--
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s"/>
-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
</platform>

<platform name="iPhone">
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas"/>
</platform>

<!--shared-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_audio_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mp3_dec_PQMF.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mp3_dec_vlc.c"/>

<wrap name="../kinoma-ipp-lib/kinoma_ipp_lib_mp3_dec.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_math.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mp3_dec_huff.c"/>
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->
<!--/buildstyle-->

<platform name="MacOSX,mac-cmake,Solaris,iPhone,Windows">
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
</platform>
  
<platform name="Windows">
<common>
C_OPTIONS =  /D "_WIN32_WINNT=0x0400" /D "__INTEL_IPP__NO=1" /D "__KINOMA_IPP__=1" /D "KINOMA_MP3=1" /D "KINOMA_FAST_HUFFMAN=1" $(C_OPTIONS)
</common>
</platform>

<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1 -DOSX32=1 -D__KINOMA_IPP__=1 -DKINOMA_MP3=1
LINKER = g++
</common>
</platform>

<platform name="mac-cmake">
<common>
add_definitions(-DLINUX32=1)
add_definitions(-DOSX32=1)
add_definitions(-D__KINOMA_IPP__=1)
add_definitions(-DKINOMA_MP3=1)
</common>
</platform>

<platform name="Linux">
<common>
C_OPTIONS += -O2 -DTARGET_OS_LINUX=1 -DLINUX32=1 -D__KINOMA_IPP__=1 -DKINOMA_MP3=1 -D_GNU_SOURCE
LINKER = g++
</common>
</platform>

<platform name="android">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1 \
	-DLINUX32=1 			\
	-D__KINOMA_IPP__=1 		\
	-DKINOMA_MP3=1 			\
	-D_GNU_SOURCE 			\
	-D__KINOMA_IPP_ARM_V5__	\
	-DKINOMA_FAST_HUFFMAN=1	\
	-D__INTEL_IPP__NONONO=1	\
	-fno-rtti

LINKER = $(deviceLINK)

OBJECTS += $(TMP_DIR)/kinoma_ipp_memory-gas.o

$(TMP_DIR)/kinoma_ipp_memory-gas.o: ../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s -o $(TMP_DIR)/kinoma_ipp_memory-gas.o

</common>
</platform>

<platform name="iPhone">
<common>
C_OPTIONS += -DTARGET_OS_IPHONE=1 \
	-DLINUX32=1 			\
	-D__KINOMA_IPP__=1 		\
	-DKINOMA_MP3=1 			\
	-D_GNU_SOURCE 			\
	-D__KINOMA_IPP_ARM_V5__	\
	-DKINOMA_FAST_HUFFMAN=1	\
	-D__INTEL_IPP__NONONO=1

LINKER = g++
</common>
</platform>


<platform name="Windows">
<common>
LIBRARIES = $(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippac.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippcc.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippch.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippcore.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippcv.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippdc.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippi.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippj.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippm.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ipps.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippsc.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippsr.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippvc.lib \
			$(F_HOME)/kinoma/intel/ipp/ia32/stublib/ippvm.lib
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>
</makefile>
