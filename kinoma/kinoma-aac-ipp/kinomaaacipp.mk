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

<platform name="MacOSX,iPhone">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="Windows">
<input name="$(F_HOME)/libraries/DirectX"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="Linux">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<platform name="android">
<!--input name="$(F_HOME)/kinoma/intel/ipp/ixp/include/"/-->
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
</platform>

<input name="$(F_HOME)/kinoma/intel/codec/aac_dec_int/include/"/>
<input name="$(F_HOME)/kinoma/intel/codec/common/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<include name="/makefiles/xsFskDefaults.mk"/>

<wrap name="sources/kinomaaacippextension.c"/>
<wrap name="sources/kinomaaacippdecode.cpp"/>

<wrap name="../intel/codec/aac_dec_int/src/aac_dec_api_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/aac_dec_decoding_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/aac_dec_ltp_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/aac_dec_tns_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_decoder_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_dequant_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_filter_inv_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_filter_qmf_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_hf_adjust_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_hf_gen_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_math_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_noise_table_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/sbrdec_qmf_tables_int.c"/>
<wrap name="../intel/codec/aac_dec_int/src/umc_aac_decoder_int.cpp"/>

<wrap name="../intel/codec/common/src/aaccmn_adif.c"/>
<wrap name="../intel/codec/common/src/aaccmn_adts.c"/>
<wrap name="../intel/codec/common/src/aaccmn_chmap.c"/>
<wrap name="../intel/codec/common/src/aac_dec_api.c"/>
<wrap name="../intel/codec/common/src/aac_dec_huff_tables.c"/>
<wrap name="../intel/codec/common/src/aac_dec_stream_elements.c"/>
<!--wrap name="../intel/codec/common/src/aac_filterbank_fp.c"/-->
<wrap name="../intel/codec/common/src/aac_filterbank_int.c"/>
<wrap name="../intel/codec/common/src/aac_sfb_tables.c"/>
<!--wrap name="../intel/codec/common/src/aac_win_tables_fp.c"/-->
<wrap name="../intel/codec/common/src/aac_win_tables_int.c"/>
<wrap name="../intel/codec/common/src/bstream.c"/>
<wrap name="../intel/codec/common/src/mp4cmn_config.c"/>
<wrap name="../intel/codec/common/src/mp4cmn_const.c"/>
<wrap name="../intel/codec/common/src/mp4cmn_pce.c"/>
<wrap name="../intel/codec/common/src/sbrdec_env_dec.c"/>
<wrap name="../intel/codec/common/src/sbrdec_freq_tables.c"/>
<wrap name="../intel/codec/common/src/sbrdec_huff_tables.c"/>
<wrap name="../intel/codec/common/src/sbrdec_parser.c"/>
<wrap name="../intel/codec/common/src/sbrdec_power.c"/>
<wrap name="../intel/codec/common/src/sbrdec_reset.c"/>

<wrap name="../intel/core/umc/src/umc_audio_codec.cpp"/>
<wrap name="../intel/core/umc/src/umc_media_data.cpp"/>

<!--buildstyle name="nonembed"-->
<!--shared-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_audio_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_aac_com_fft.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_aac_dec_vlc.c"/>
  
<wrap name="../kinoma-ipp-lib/kinoma_ipp_lib_aac_dec.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_math.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_aac_dec_huff.c"/>
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->
<!--/buildstyle-->

<!--
<platform name="android">
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s"/>
</platform>
-->

<platform name="iPhone">
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas"/>
</platform>

<platform name="Windows">
<common>
C_OPTIONS =  /D "_WIN32_WINNT=0x0400" /D "__INTEL_IPP__NO=1" /D "__KINOMA_IPP__=1" /D "KINOMA_AAC=1" /D "KINOMA_DEBUG_NO=1" /D "KINOMA_FAST_HUFFMAN=1" $(C_OPTIONS)
</common>
</platform>

<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1 -DOSX32=1 -D__KINOMA_IPP__=1 -DKINOMA_AAC=1
LINKER = g++
</common>
</platform>

<platform name="Linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1 -DLINUX32=1 -D__KINOMA_IPP__=1 -DKINOMA_AAC=1 -DKINOMA_FAST_HUFFMAN=1
LINKER = g++
</common>
</platform>

<platform name="android">
<common>
C_OPTIONS +=  \
	-DTARGET_OS_LINUX=1		\
	-D__KINOMA_IPP_ARM_V5__=1	\
	-DLINUX32=1				\
	-D__KINOMA_IPP__=1			\
	-DKINOMA_AAC=1			\
  -DKINOMA_DEBUG_NO=1  \
	-fno-rtti

LINKER = $(deviceLINK)

OBJECTS += $(TMP_DIR)/kinoma_ipp_memory-gas.o

$(TMP_DIR)/kinoma_ipp_memory-gas.o: ../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-gas.s -o $(TMP_DIR)/kinoma_ipp_memory-gas.o

</common>
</platform>

<platform name="iPhone">
<common>
C_OPTIONS +=  \
	-DTARGET_OS_IPHONE=1		\
	-D__KINOMA_IPP_ARM_V5__=1	\
	-DLINUX32=1				\
	-D__KINOMA_IPP__=1			\
	-DKINOMA_AAC=1			\
  -DKINOMA_DEBUG_NO=1 

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
