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

<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>
<input name="$(F_HOME)/kinoma/intel/codec/h264_dec/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm_plus/include/"/>


<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<wrap name="FskMediaReaderFLVExtension.c"/>
<wrap name="FskMediaReaderFLV.c"/>
<wrap name="FskMediaReader.c"/>

<wrap name="../../../kinoma-ipp-lib/kinoma_avc_header_parser.c"/>

<platform name="android">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1		    \
			-DLINUX32=1					    \
			-D__KINOMA_IPP_ARM_V5__=1		\
		-DKINOMA							\
		-D__KINOMA_IPP__=1					\
		-DKINOMA_AVC=1				        \
  		-DDROP_C							\
		-DDROP_HIGH_PROFILE					\
  		-DDROP_VUI							\
		-DDROP_FIELD						\
		-DDROP_MBAFF						\
  		-DDROP_CABAC						\
		-fno-rtti

LINKER = $(deviceLINK)

OBJECTS += $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o
OBJECTS += $(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.o


$(TMP_DIR)/kinoma_ipp_memory-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas -o $(TMP_DIR)/kinoma_ipp_memory-arm.gas.o

$(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas
	$(deviceAS) $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas -o $(TMP_DIR)/kinoma_avc_dec_bitstream-arm.gas.o

</common>
</platform>


<platform name="Linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1		    \
			-DLINUX32=1					    \
		-DKINOMA							\
		-D__KINOMA_IPP__=1					\
		-DKINOMA_AVC=1				        \
  		-DDROP_C							\
		-DDROP_HIGH_PROFILE					\
  		-DDROP_VUI							\
		-DDROP_FIELD						\
		-DDROP_MBAFF						\
  		-DDROP_CABAC						\
		-fno-rtti

LINKER = $(deviceLINK)

</common>
</platform>


<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1				\
			-DOSX32=1					\
		-DKINOMA						\
		-D__KINOMA_IPP__=1				\
		-DKINOMA_AVC=1				    \
  		-DDROP_C						\
		-DDROP_HIGH_PROFILE				\
  		-DDROP_VUI						\
		-DDROP_FIELD					\
		-DDROP_MBAFF					\
  		-DDROP_CABAC
            
LINKER = $(CXX)
</common>
</platform>

<platform name="Windows">
<common>
C_OPTIONS = /D "_WIN32_WINNT=0x0400"	\
		/D "__KINOMA_IPP__=1"		\
        /D "KINOMA_AVC=1"			\
        /D DROP_MBAFF				\
        /D DROP_FIELD				\
        /D DROP_HIGH_PROFILE		\
        /D DROP_MULTI_THREAD		\
        /D DROP_CABAC				\
        /D DROP_C					\
        /D DROP_VUI					\
            $(C_OPTIONS)
</common>
</platform>


<platform name="iPhone">
<common>
C_OPTIONS += -DLINUX32=1				\
			-DOSX32=1					\
		-DKINOMA						\
		-D__KINOMA_IPP__=1				\
		-DKINOMA_AVC=1				    \
		-DDROP_C						\
		-DDROP_HIGH_PROFILE				\
		-DDROP_VUI						\
		-DDROP_FIELD					\
		-DDROP_MBAFF					\
		-DDROP_CABAC

LINKER = g++
</common>
</platform>


<include name="/makefiles/xsLibrary.mk"/>

</makefile>
