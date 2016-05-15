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

<input name="$(F_HOME)/libraries/QTReader"/>

<platform name="MacOSX">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="iPhone">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="Windows">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="Linux">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="android">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<input name="$(F_HOME)/kinoma/intel/codec/mpeg4_dec/include/"/>
<input name="$(F_HOME)/kinoma/intel/codec/common/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>
<input name="$(F_HOME)/kinoma/intel/codec/h264_dec/include/"/>

<wrap name="sources/kinomamp4ippextension.c"/>
<wrap name="sources/kinomamp4ippdecode.cpp"/>

<wrap name="../intel/codec/mpeg4_dec/src/mp4decvop.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4decvopb.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4decvopi.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4decvopp.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4decvops.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4parse.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4stream.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/mp4tbl.c"/>
<wrap name="../intel/codec/mpeg4_dec/src/umc_mpeg4_video_decoder.cpp"/>

<wrap name="../intel/core/umc/src/umc_video_decoder.cpp"/>
<wrap name="../intel/core/umc/src/umc_video_data.cpp"/>
<wrap name="../intel/core/umc/src/umc_media_data.cpp"/>

<!--buildstyle name="nonembed"--> 
  
<!--platform name="Windows">
<wrap name="../intel/core/vm/src/vm_debug_win32.c"/>
<wrap name="../intel/core/vm/src/vm_event_win32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_win32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_win32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_win32.c"/>
</platform>

<platform name="MacOSX">
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>

<platform name="iPhone">
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>

<platform name="Linux">
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>

<platform name="android">
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>


<!--shared-->
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_dct.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_GMC.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_interpolation.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_motion.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_obmc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_quant.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_dec_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_flv.c"/>

<wrap name="../kinoma-ipp-lib/kinoma_ipp_lib_mp4_dec.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_math.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_performance.c"/>
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->

<!--use h264 deblocking-->
<wrap name="../kinoma-ipp-lib/kinoma_deblocking.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_deblocking.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_deblocking_table.cpp"/>
  
<!--/buildstyle-->

<platform name="Windows">
<common>
C_OPTIONS = /D "_WIN32_WINNT=0x0400"        \
            /D "__INTEL_IPP__NO=1"            \
            /D "__KINOMA_IPP__=1"           \
            /D "KINOMA_MP4V=1"              \
            /D "KINOMA_MP4V_DEBLOCKING"     \
            /D "_KINOMA_LOSSY_OPT_=1"       \
            /D "DROP_FIELD=1"               \
            /D "DROP_GMC=1"                 \
            /D "DROP_SPRITE=1"              \
            /D "DROP_COLOR_CONVERSION"      \
            /D "DROP_MULTI_THREAD=1"        \
            /D "BUF_PRINTF_NO"              \
            /D "DROP_MBAFF"                 \
            $(C_OPTIONS)
</common>
</platform>

<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1                  \
            -DOSX32=1                     \
            -D__KINOMA_IPP__=1            \
            -DKINOMA_MP4V=1               \
            -DKINOMA_MP4V_DEBLOCKING=1    \
            -D_KINOMA_LOSSY_OPT_=1        \
            -DDROP_FIELD=1                \
            -DDROP_GMC=1                  \
            -DDROP_SPRITE=1               \
            -DDROP_MULTI_THREAD=1         \
            -DDROP_COLOR_CONVERSION=1     \
            -DDROP_MBAFF=1

            
LINKER = g++
</common>
</platform>

<platform name="iPhone">
<common>
C_OPTIONS += -DLINUX32=1                  \
            -DOSX32=1                     \
            -D__KINOMA_IPP__=1            \
            -DKINOMA_MP4V=1               \
            -DKINOMA_MP4V_DEBLOCKING=1    \
            -D_KINOMA_LOSSY_OPT_=1        \
            -DDROP_FIELD=1                \
            -DDROP_GMC=1                  \
            -DDROP_SPRITE=1               \
            -DDROP_MULTI_THREAD=1         \
            -DDROP_COLOR_CONVERSION=1     \
            -DDROP_MBAFF=1


LINKER = g++
</common>
</platform>

<platform name="Linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1        \
  -DLINUX32=1                 \
  -D__KINOMA_IPP__=1          \
  -DKINOMA_MP4V=1             \
  -DKINOMA_MP4V_DEBLOCKING=1  \
  -D_KINOMA_LOSSY_OPT_=1      \
  -DDROP_FIELD=1              \
  -DDROP_GMC=1                \
  -DDROP_SPRITE=1             \
  -DDROP_MULTI_THREAD=1       \
  -DDROP_COLOR_CONVERSION=1   \
  -DDROP_MBAFF=1

LINKER = g++
</common>
</platform>

<platform name="android">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1        \
  -DLINUX32=1                 \
  -D__KINOMA_IPP__=1          \
  -D__KINOMA_IPP_ARM_V5__=1    \
  -DKINOMA_MP4V=1             \
  -DKINOMA_MP4V_DEBLOCKING=1  \
  -D_KINOMA_LOSSY_OPT_=1      \
  -DDROP_FIELD=1              \
  -DDROP_GMC=1                \
  -DDROP_SPRITE=1             \
  -DDROP_MULTI_THREAD=1       \
  -DDROP_COLOR_CONVERSION=1	\
  -DBUF_PRINTF_NO              \
  -DDROP_MBAFF                \
  -DSUPPORT_H263_ONLY		\
	-fno-rtti
LINKER = $(deviceLINK)

OBJECTS += $(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_mpeg4_com_motion-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_mpeg4_com_dct_v6-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_mpeg4_com_dct-arm.gas.o

$(TMP_DIR)/kinoma_ipp_memory-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas -o $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.o

$(TMP_DIR)/kinoma_mpeg4_com_dct-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_dct-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_dct-arm.gas -o $(TMP_DIR)/kinoma_mpeg4_com_dct-arm.gas.o

$(TMP_DIR)/kinoma_mpeg4_com_dct_v6-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_dct_v6-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_dct_v6-arm.gas -o $(TMP_DIR)/kinoma_mpeg4_com_dct_v6-arm.gas.o

$(TMP_DIR)/kinoma_mpeg4_com_motion-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_motion-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_motion-arm.gas -o $(TMP_DIR)/kinoma_mpeg4_com_motion-arm.gas.o

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
