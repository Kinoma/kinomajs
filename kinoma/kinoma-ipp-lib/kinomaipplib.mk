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


<platform name="MacOSX">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/stublib/"/>
</platform>

<platform name="Windows">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/stublib/"/>
</platform>

<platform name="Linux">
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/include/"/>
<input name="$(F_HOME)/kinoma/intel/ipp/ia32/stublib/"/>
</platform>

<platform name="arm-linux-xscale">
<input name="$(F_HOME)/kinoma/intel/ipp/ixp/include/"/>
<input name="$(F_HOME)/../../../opt/intel/ipp/5.2/ixp/lib/le/"/>
</platform>

<input name="$(F_HOME)/kinoma/kinoma-avc/sources/ipp_lib/"/>
<input name="$(F_HOME)/kinoma/intel/codec/h264_dec/include/"/>
<input name="$(F_HOME)/kinoma/intel/codec/mpeg4_dec/include/"/>
<input name="$(F_HOME)/kinoma/intel/codec/common/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/umc/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm/include/"/>
<input name="$(F_HOME)/kinoma/intel/core/vm_plus/include/"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>  
  
<platform name="Windows">
<wrap name="../intel/core/vm/src/vm_debug_win32.c"/>
<wrap name="../intel/core/vm/src/vm_event_win32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_win32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_win32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_win32.c"/>
</platform>

<platform name="MacOSX,Linux,arm-linux">  
<wrap name="../intel/core/vm/src/vm_debug_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_event_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_mutex_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_sys_info_linux32.c"/>
<wrap name="../intel/core/vm/src/vm_thread_linux32.c"/>
</platform>

<!--shared-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_lib.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_deblocking.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_memory.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_ipp_math.c"/>
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->

<!--MP4V-->
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_dct.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_GMC.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_interpolation.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_motion.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_obmc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_com_quant.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_dec_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mpeg4_flv.c"/>

<platform name="arm-linux">
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_bitstream-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_deblocking-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_interpolate-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_avc_dec_reconstruction-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_ipp_memory-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_dct-arm.gas.s"/>
<wrap name="../kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_motion-sym.s"/>
</platform>  
  
<!--H264-->  
<wrap name="../kinoma-ipp-lib/kinoma_avc_dummy.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_bitstream.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_deblocking.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_interpolate.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_reconstruction.cpp"/>
<wrap name="../kinoma-ipp-lib/kinoma_avc_dec_reconstruction_HP.cpp"/>

<!--AAC-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_audio_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_aac_com_fft.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_aac_dec_vlc.c"/>

<!--MP3-->
<wrap name="../kinoma-ipp-lib/kinoma_ipp_audio_vlc.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mp3_dec_PQMF.c"/>
<wrap name="../kinoma-ipp-lib/kinoma_mp3_dec_vlc.c"/>


<platform name="Windows">
<common>
C_OPTIONS = /D "_WIN32_WINNT=0x0400"  \
            /D "__INTEL_IPP__=1"      \
            /D "__KINOMA_IPP__=1"     \
            /D "KINOMA_MP4V=1"        \
            /D "KINOMA_MP4V_DEBLOCKING" \
            /D "KINOMA_AVC=1"         \
            /D "KINOMA_MP3=1"         \
            /D "KINOMA_AAC=1"         \
            /D "BUF_PRINTF_NO=1"      \
            /D _KINOMA_LOSSY_OPT_     \
            $(C_OPTIONS)        
</common>
</platform>

<platform name="MacOSX">
<common>
C_OPTIONS += -DLINUX32=1 -DOSX32=1 -D__KINOMA_IPP__=1 -DKINOMA_AVC=1 -DKINOMA_MP4V=1 -DKINOMA_MP4V_DEBLOCKING=1 -D_KINOMA_LOSSY_OPT_=1
LINKER = g++
</common>
</platform>
  
<platform name="Linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1 -DLINUX32=1 -D__KINOMA_IPP__=1 -DKINOMA_AVC=1 -DKINOMA_MP4V=1 -DKINOMA_MP4V_DEBLOCKING=1 -D_KINOMA_LOSSY_OPT_=1
LINKER = g++
</common>
</platform>
  
<platform name="arm-linux">
<common>
C_OPTIONS += -DTARGET_OS_LINUX=1 -DLINUX32=1 -D__KINOMA_IPP__=1 -DKINOMA_AVC=1 -DKINOMA_MP4V=1 -DKINOMA_MP4V_DEBLOCKING=1 -D_KINOMA_LOSSY_OPT_=1 -D__KINOMA_IPP_ARM_V5__=1
LINKER = $(deviceLINK)


OBJECTS += $(TMP_DIR)/kinoma_mpeg4_com_motion-arm.gas.s.o

<!--
$(TMP_DIR)/kinoma_mpeg4_com_motion-arm.gas.s.o: $(F_HOME)/kinoma/kinoma-ipp-lib/arm_asm/kinoma_mpeg4_com_motion-arm.gas.s
arm-linux-gcc -c -o $@ $<
-->
  
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

<platform name="arm-linux-xscale">
<common>
LIBRARIES = \
	/opt/intel/ipp/5.2/ixp/lib/le/libippacs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippchsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippdcsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippscs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippssx.a   \
	/opt/intel/ipp/5.2/ixp/lib/le/libippacsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippcore.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippis2.a   \
	/opt/intel/ipp/5.2/ixp/lib/le/libippscsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippvcs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippccs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippcvs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippisx.a   \
	/opt/intel/ipp/5.2/ixp/lib/le/libippsrs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippvcsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippccsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippcvsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippjs2.a   \
	/opt/intel/ipp/5.2/ixp/lib/le/libippsrsx.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippchs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippdcs2.a  \
	/opt/intel/ipp/5.2/ixp/lib/le/libippjsx.a   \
	/opt/intel/ipp/5.2/ixp/lib/le/libippss2.a
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>