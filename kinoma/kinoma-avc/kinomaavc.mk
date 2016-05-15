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

<input name="$(F_HOME)/kinoma/kinoma-avc/sources/ipp_lib/"/>
<input name="$(F_HOME)/kinoma/intel/codec/h264_dec/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
	<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm_plus/include/"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<wrap name="kinomaavcextension.c"/>
<wrap name="kinomaavcdecode.cpp"/>

<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_bitstream.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_conversion.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_decode_pic.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_defs_yuv.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_slice_decoder.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_slice_decoder_decode_pic.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_slice_store.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_tables.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_deblocking.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_deblocking_table.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_decode_mb.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_decode_mb_types.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_mblevel_calc.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_reconstruct_mb.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_store.cpp"/>


<!--	dropped features
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_bitstream_cabac.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_ipplevel.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_sei.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_slice_decoder_mt.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_dec_tables_cabac.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_hp.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_hp_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_mt.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_mt_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_mt_hp.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cabac_mt_hp_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_hp.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_hp_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_mt.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_mt_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_mt_hp.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_cavlc_mt_hp_field.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_deblocking_mbaff.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_deblocking_mt.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_decode_mb_cabac.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_decode_mb_types_cabac.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_mt.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_reconstruct_mb_aff.cpp"/>
<wrap name="../intel/codec/h264_dec/src/umc_h264_segment_decoder_reconstruct_mb_field.cpp"/>

-->

<wrap name="../intel/core/umc/src/umc_video_decoder.cpp"/>
<wrap name="../intel/core/umc/src/umc_media_data.cpp"/>
<!--wrap name="../intel/core/vm_plus/src/umc_event.cpp"/-->

<!--buildstyle name="nonembed"-->

<!--platform name="Windows">
<wrap name="../intel/core/vm/src/vm_thread_win32.c"/>
<wrap name="../intel/core/vm/src/vm_event_win32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_win32.c"/>
<wrap name="../intel/core/vm/src/vm_debug_win32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_win32.c"/>
</platform>

<platform name="MacOSX,Linux,iPhone">
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>-->

<platform name="iphone/device">
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_reconstruction-arm.gas"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_interpolate-arm.gas"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas"/>
</platform>

<platform name="android">
<!--
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_interpolate-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_reconstruction-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas.s"/>
-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
</platform>
  
<!--shared-->
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_bitstream.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_deblocking.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_interpolate.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_reconstruction.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_reconstruction_HP.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_strip_epb.cpp"/>
  
<wrap name="../kinoma-ipp-lib/kinoma_ipp_lib_avc_dec.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dummy.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_performance.c"/>
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->

<platform name="MacOSX,Solaris,Windows,iPhone">
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
</platform>

<!--/buildstyle-->

<platform name="Windows">
<common>
C_OPTIONS = /D "_WIN32_WINNT=0x0400"  \
            /D "__INTEL_IPP__NO=1"      \
            /D "__KINOMA_IPP__=1"     \
            /D "KINOMA_AVC=1"         \
            /D "DO_TIMING_NO=1" \
            /D MAX_NUM_SEQ_PARAM_SETS=8 \
            /D MAX_NUM_PIC_PARAM_SETS=16  \
            /D MAX_SLICE_NUM=16 \
            /D MAX_NUM_REF_FRAMES=8 \
            /D DROP_MBAFF \
            /D DROP_FIELD \
            /D DROP_HIGH_PROFILE \
            /D DROP_MULTI_THREAD \
            /D DROP_CABAC \
            /D DROP_C \
            /D DROP_COLOR_CONVERSION \
            /D DROP_NON_VCL \
            /D DROP_SEI \
            /D DROP_VUI \
            /D DROP_ADAPTIVE_REF_MARKING \
            /D _KINOMA_LOSSY_OPT_ \
            /D SPEEDUP_HACK_NO \
            /D BUF_PRINTF_NO \
            $(C_OPTIONS)
</common>
</platform>

  
<!--buildstyle name="nonembed"-->

<!--/buildstyle-->

<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1				        \
			-DOSX32=1					                \
			-D__KINOMA_IPP__=1			          \
			-DKINOMA_AVC=1				            \
    -DMAX_NUM_SEQ_PARAM_SETS=8 \
    -DMAX_NUM_PIC_PARAM_SETS=16  \
    -DMAX_SLICE_NUM=16 \
    -DMAX_NUM_REF_FRAMES=8 \
            -DDROP_MBAFF_NO=1				    \
            -DDROP_FIELD_NO=1				    \
            -DDROP_HIGH_PROFILE_NO=1		\
            -DDROP_MULTI_THREAD_NO=1    \
            -DDROP_MBAFF                \
            -DDROP_FIELD                \
            -DDROP_HIGH_PROFILE         \
            -DDROP_MULTI_THREAD         \
            -DDROP_CABAC                \
            -DDROP_C                    \
            -DDROP_COLOR_CONVERSION     \
            -DDROP_NON_VCL              \
            -DDROP_SEI                  \
            -DDROP_VUI                  \
            -DDROP_ADAPTIVE_REF_MARKING \
            -D_KINOMA_LOSSY_OPT_        \
            -DSPEEDUP_HACK_NO           \
            -DBUF_PRINTF_NO             
            
LINKER = g++
</common>
</platform>

<platform name="iPhone">
<common>
C_OPTIONS += -DLINUX32=1				        \
			-DOSX32=1					                \
			-D__KINOMA_IPP__=1			          \
			-DKINOMA_AVC=1				            \
		-DMAX_NUM_SEQ_PARAM_SETS=8 \
		-DMAX_NUM_PIC_PARAM_SETS=16  \
		-DMAX_SLICE_NUM=16 \
		-DMAX_NUM_REF_FRAMES=8 \
            -DDROP_MBAFF_NO=1				    \
            -DDROP_FIELD_NO=1				    \
            -DDROP_HIGH_PROFILE_NO=1		\
            -DDROP_MULTI_THREAD_NO=1    \
            -DDROP_MBAFF                \
            -DDROP_FIELD                \
            -DDROP_HIGH_PROFILE         \
            -DDROP_MULTI_THREAD         \
            -DDROP_CABAC                \
            -DDROP_C                    \
            -DDROP_COLOR_CONVERSION     \
            -DDROP_NON_VCL              \
            -DDROP_SEI                  \
            -DDROP_VUI                  \
            -DDROP_ADAPTIVE_REF_MARKING \
            -D_KINOMA_LOSSY_OPT_        \
            -DSPEEDUP_HACK_NO           \
            -DBUF_PRINTF_NO             \
            -D__KINOMA_IPP_ARM_V5__=1

LINKER = g++
</common>
</platform>

<platform name="Linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1		    \
			-DLINUX32=1					              \
			-D__KINOMA_IPP__=1			          \
			-DKINOMA_AVC=1				            \
            -DDROP_MBAFF_NO=1				    \
            -DDROP_FIELD_NO=1				    \
            -DDROP_HIGH_PROFILE_NO=1		\
            -DDROP_MULTI_THREAD_NO=1    \
            -DDROP_MBAFF                \
            -DDROP_FIELD                \
            -DDROP_HIGH_PROFILE         \
            -DDROP_MULTI_THREAD         \
            -DDROP_CABAC                \
            -DDROP_C                    \
            -DDROP_COLOR_CONVERSION     \
            -DDROP_NON_VCL              \
            -DDROP_SEI                  \
            -DDROP_VUI                  \
            -DDROP_ADAPTIVE_REF_MARKING \
            -D_KINOMA_LOSSY_OPT_        \
            -DSPEEDUP_HACK_NO           \
            -DBUF_PRINTF_NO             

LIBRARIES += -lstdc++

LINKER = g++
</common>
</platform>
  
<platform name="android">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1		    \
  		-D__INTEL_IPP__NO=1 	\
		-DLINUX32=1					              \
  		-D__KINOMA_IPP_ARM_V5__=1				\
		-D__KINOMA_IPP__=1			          \
		-DDO_TIMING_NO=1	 \
		-DKINOMA_AVC=1				            \
  		-DMAX_NUM_SEQ_PARAM_SETS=8 \
  		-DMAX_NUM_PIC_PARAM_SETS=16  \
  		-DMAX_SLICE_NUM=16 \
  		-DMAX_NUM_REF_FRAMES=8 \
  		-DDROP_MBAFF \
  		-DDROP_FIELD \
  		-DDROP_HIGH_PROFILE \
  		-DDROP_MULTI_THREAD \
  		-DDROP_CABAC \
  		-DDROP_C \
  		-DDROP_COLOR_CONVERSION \
  		-DDROP_NON_VCL \
  		-DDROP_SEI \
  		-DDROP_VUI \
  		-DDROP_ADAPTIVE_REF_MARKING \
  		-D_KINOMA_LOSSY_OPT_ \
  		-DSPEEDUP_HACK_NO \
		-DBUF_PRINTF_NO	\
		-fno-rtti

LINKER = $(deviceLINK)

OBJECTS += $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_avc_dec_reconstruction-arm.gas.s.o
OBJECTS += $(TMP_DIR)/kinoma_avc_dec_interpolate-arm.gas.s.o
OBJECTS += $(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.s.o
OBJECTS += $(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.s.o

$(TMP_DIR)/kinoma_ipp_memory-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas -o $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_deblocking-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_interpolate-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_interpolate-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_interpolate-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_interpolate-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_reconstruction-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_reconstruction-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_reconstruction-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_reconstruction-arm.gas.o

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
