<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
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
	<input name="$(F_HOME)/core/base"/>
	<input name="$(F_HOME)/core/graphics"/>
	<input name="$(F_HOME)/core/managers"/>
	<input name="$(F_HOME)/core/misc"/>
	<input name="$(F_HOME)/core/network"/>
	<input name="$(F_HOME)/core/ui"/>
	<input name="$(F_HOME)/extensions/crypt/sources"/>
	<input name="$(F_HOME)/xs6/includes"/>
	<input name="$(F_HOME)/libraries/QTReader"/>
	<input name="$(F_HOME)/libraries/expat"/>

	<input name="$(F_HOME)/libraries/freetype/include"/>

	<input name="$(F_HOME)/build/linux/kpl"/>
	<input name="$(F_HOME)/core/kpl"/>
	<input name="$(F_HOME)/libraries/libjpeg"/>
	<input name="$(F_HOME)/libraries/zlib"/>
	<input name="$(F_HOME)/libraries/zlib/arm"/>

	<input name="$(NANOPIM1_SYSROOT)/usr/include"/>

	<input name="$(F_HOME)/libraries/freetype/include"/>

	<header name="FskPlatform.h"/>
	
	<source name="KplScreenLinuxNanoPiM1.c"/>
	<source name="KplInputLinuxDebian.c"/>
	<source name="KplAudioLinuxALSA.c"/>

	<source name="yuv420torgb-arm-v4-v5.gas"/>
	<source name="yuv420torgb-arm-v6.gas"/>
	<source name="yuv420torgb565le-arm.gas"/>
	<source name="yuv420torgb565le-arm.gas7"/>
	<header name="yuv420torgb-arm-s.h"/>
	
	<source name="FskYUV420Copy-arm.gas"/>
	<source name="FskYUV420iCopy-arm.gas"/>
	<source name="FskYUV420iCopy-arm-v6.gas"/>
	<source name="jidctfst2-arm.gas"/>
	<source name="jidctfst2-arm-v6.gas"/>
	<source name="jidctfst2-arm-v7.gas7"/>

	<source name="FskBlit-arm.gas7"/>
	<source name="AAScaleBitmap-arm.gas7"/>
	<source name="FskTransferAlphaBlit-arm.gas7"/>
	<source name="HVfilter-arm.gas7"/>

    <source name="inflate_fast_copy_neon.gas"/>
    <source name="adler32_neon.gas"/>

	<source name="FskFixedMath-arm.gas"/>

<!--
	<source name="FskGLBlit.c"/>
	<source name="FskGLEffects.c"/>
	<source name="FskGLContext.c"/>
    <source name="KplGL.c"/>
-->

	<asm option="-ftree-vectorize"/>
	<asm option="-x assembler-with-cpp"/>
	<asm option="-mfpu=neon"/>

	<c option="-DBAD_FTRUNCATE=1"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-DFSK_APPLICATION_$(FSK_APPLICATION)=1"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-DFSK_TEXT_FREETYPE=1"/>
	<c option="-DKPL=1"/>
	<c option="-DKPR_CONFIG=1"/>
<!--
	<c option="-DLINUX_PLATFORM=1"/>
-->
	<c option="-DUSE_ASYNC_RESOLVER=1 "/>
	<c option="-DUSE_FRAMEBUFFER_VECTORS=1"/>
	<c option="-DUSE_POLL=1"/>
	<c option="-DUSE_POSIX_CLOCK=1"/>
	<c option="-DUSE_INOTIFY=1"/>
	<c option="-D_FILE_OFFSET_BITS=64"/>
	<c option="-D_LARGEFILE64_SOURCE"/>
	<c option="-D_LARGEFILE_SOURCE"/>
	<c option="-D_REENTRANT"/>
	<c option="-D__FSK_LAYER__=1"/>
<!--
	<c option="-D__XSCALE__=1"/>
-->
	<c option="-DUSE_WPACONFIG=1"/>
	<c option="-DLINUX=1"/>
	<c option="-DKINOMA_IDICT_IFAST_SUPPORTED"/>
	<c option="-DKINOMA_YUV2RGB565_IFAST_ARM_SUPPORTED"/>
	<c option="-DYUV420i_RGB_ARM_IMPLEMENTATION=1"/>
	<c option="-DSUPPORT_NEON=1"/>
<!--
-->
<!--
	<c option="-DEGL_API_FB=1"/>
	<c option="-DFSK_OPENGLES_KPL=1"/>
-->
	<c option="-DTARGET_RT_ALSA=1"/>
	<c option="-fsigned-char"/>

	<c option="-Wall"/>
	<c option="-Werror-implicit-function-declaration"/>
	<c option="-Wunused-value"/>
	<c option="-Wunused-function"/>
	<c option="-Wunused-variable"/>
	<c option="-Wno-multichar"/>
	<c option="-mthumb"/>
	<c option="-march=armv7-a"/>
<!--
	<c option="-mfpu=vfpv3-fp16"/>
-->
	<c option="-mfpu=neon"/>
	<c option="-mtune=cortex-a9"/>
	<c option="-mfloat-abi=hard"/>
	<c option="-DDEVICE_NAME=NanoPi\ M1"/>

    <c option="--sysroot=$(NANOPIM1_SYSROOT)"/>
	
	<library name="-Wl,-rpath,$(NANOPIM1_SYSROOT)/usr/lib,-z,muldefs"/>
    <library name="-Wl,-rpath,.,-rpath,'$ORIGIN/lib'"/>
	<library name="-L$(NANOPIM1_SYSROOT)/usr/lib"/>
	<library name="-nodefaultlibs"/>
	<library name="-lm"/>
	<library name="-ldl"/>
	<library name="-lrt"/>
	<library name="-lpthread"/>
	<library name="-lresolv"/>
	<library name="-lgcc"/>
	<library name="-lc"/>
	<library name="-lasound"/>
<!--
	<library name="-lEGL"/>
	<library name="-lGLESv2"/>
-->

	<c option="-I$(F_HOME)/libraries/freetype/include"/>
	<version name="debug">
		<c option="-DmxDebug"/>
		<c option="-g"/>
	</version>
	
	<version name="release">
		<c option="-g"/>
		<c option="-O2"/>
	</version>
</makefile>
