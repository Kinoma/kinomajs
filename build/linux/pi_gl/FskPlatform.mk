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
	<input name="$(F_HOME)/libraries/zlib"/>
	<input name="$(F_HOME)/libraries/QTReader"/>

	<input name="$(F_HOME)/build/linux/kpl"/>
	<input name="$(F_HOME)/core/kpl"/>
	<input name="$(F_HOME)/libraries/libjpeg"/>

	<input name="$(PI_SYSROOT)/opt/vc/include/"/>
	<input name="$(PI_SYSROOT)/opt/vc/include/interface/vcos/pthreads/"/>
	<input name="$(PI_SYSROOT)/opt/vc/include/interface/vmcs_host/linux/"/>

	<header name="FskPlatform.h"/>
	
	<source name="KplScreenLinuxPiGl.c"/>
	<source name="KplInputLinuxDebian.c"/>
    <source name="FskGLBlit.c"/>
    <source name="FskGLEffects.c"/>
    <source name="FskGLContext.c"/>
    <source name="KplGL.c"/>

	<source name="KplAudioLinuxALSA.c"/>
	<source name="FskFixedMath-arm.gas"/>
	<source name="yuv420torgb565le-arm.gas"/>
	<source name="FskYUV420Copy-arm.gas"/>
	<!--
	<source name="yuv420torgb-arm-v4-v5.gas"/>
	<header name="yuv420torgb-arm-s.h"/>
	<source name="FskYUV420iCopy-arm.gas"/>
	-->

	<c option="--sysroot=${PI_SYSROOT}" />
	
	<c option="-DBAD_FTRUNCATE=1"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-D __PI2_GL__=1"/>
	<c option="-DRASPBERRY_PI=1"/>
	<c option="-DFSK_APPLICATION_$(FSK_APPLICATION)=1"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-I${F_HOME}/libraries/libtess2/Include/ "/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-DFSK_TEXT_FREETYPE=1"/>
	<c option="-DKPL=1"/>
	<c option="-DKPR_CONFIG=1"/>
	<c option="-DUSE_ASYNC_RESOLVER=1 "/>
	<c option="-DUSE_FRAMEBUFFER_VECTORS=1"/>
	<c option="-DUSE_POLL=1"/>
	<c option="-DUSE_POSIX_CLOCK=1"/>
    <c option="-DLINUX"/>
	<c option="-D_FILE_OFFSET_BITS=64"/>
	<c option="-D_LARGEFILE64_SOURCE"/>
	<c option="-D_LARGEFILE_SOURCE"/>
	<c option="-DFSK_OPENGLES_KPL=1"/>
	<c option="-D_REENTRANT"/>
	<c option="-D__FSK_LAYER__=1"/>
	<c option="-fsigned-char"/>
	<c option="-Wall"/>
	<c option="-Wno-multichar"/>

	<library name="--sysroot=${PI_SYSROOT}" />

	<library name="-Wl,-rpath,.,-rpath,'$ORIGIN/lib'"/>
	<library name="-L${PI_SYSROOT}/opt/vc/lib/"/>
	<library name="-lGLESv2"/>
	<library name="-lasound"/>
	<library name="-lEGL"/>
	<library name="-lbcm_host"/>
	<library name="-lvcos"/>
	<library name="-lvchiq_arm"/>
	<library name="-lm"/>
	<library name="-pthread"/>
	<library name="-ldl"/>
	<library name="-lresolv"/>
	<library name="-Wl,-z,muldefs"/>

	<version name="debug">
		<c option="-DmxDebug"/>
		<c option="-g"/>
	</version>
	
	<version name="release">
		<c option="-g"/>
		<c option="-O2"/>
	</version>
</makefile>
