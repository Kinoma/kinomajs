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

	<input name="$(F_HOME)/libraries/freetype/include"/>

	<input name="$(F_HOME)/build/linux/kpl"/>
	<input name="$(F_HOME)/core/kpl"/>
	<input name="$(F_HOME)/libraries/libjpeg"/>
	<input name="$(F_HOME)/libraries/zlib/arm"/>

	<input name="$(ANDRO_SYSROOT)/usr/include"/>

	<input name="$(F_HOME)/libraries/freetype/include"/>

	<header name="FskPlatform.h"/>
	
	<source name="KplAudioLinux.c"/>
	<source name="KplScreenLinuxNULL.c"/>

<!--
	<source name="yuv420torgb565le-arm.gas"/>
	<source name="yuv420torgb-arm-v4-v5.gas"/>
	<source name="yuv420torgb-arm-v6.gas"/>
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
-->

	<source name="FskFixedMath-arm.gas"/>

	<asm option="-x assembler-with-cpp"/>

    <c option="-march=armv5te"/>
    <c option="-mtune=xscale"/>
    <c option="-msoft-float"/>
    <c option="-mthumb-interwork"/>
    <c option="-fPIE"/>
    <c option="-fno-exceptions"/>
    <c option="-ffunction-sections"/>
    <c option="-funwind-tables"/>
    <c option="-fstack-protector"/>
    <c option="-fmessage-length=0"/>
    <c option="-finline-functions"/>
    <c option="-finline-limit=300"/>
    <c option="-fno-inline-functions-called-once"/>
    <c option="-fgcse-after-reload"/>
    <c option="-frerun-cse-after-loop"/>
    <c option="-frename-registers"/>
    <c option="-fomit-frame-pointer"/>
    <c option="-fstrict-aliasing"/>
    <c option="-funswitch-loops"/>

	<c option="--sysroot=${ANDRO_SYSROOT}"/>
	<c option="-DBAD_FTRUNCATE=1"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-DFSK_APPLICATION_$(FSK_APPLICATION)=1"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-DFSK_TEXT_FREETYPE=1"/>
	<c option="-DKPL=1"/>
	<c option="-DKPR_CONFIG=1"/>
	<c option="-DLINUX_PLATFORM=1"/>
	<c option="-DUSE_ASYNC_RESOLVER=1 "/>
	<c option="-DUSE_FRAMEBUFFER_VECTORS=1"/>
	<c option="-DUSE_POLL=1"/>
	<c option="-DUSE_POSIX_CLOCK=1"/>
	<c option="-DUSE_INOTIFY=1"/>
	<c option="-D_FILE_OFFSET_BITS=64"/>
	<!--c option="-D_LARGEFILE64_SOURCE"/-->
	<c option="-D_LARGEFILE_SOURCE"/>
	<c option="-D_REENTRANT"/>
	<c option="-D__FSK_LAYER__=1"/>
	<c option="-D__XSCALE__=1"/>
	<c option="-DUSE_WPACONFIG=1"/>
	<c option="-DLINUX=1"/>
	<c option="-fsigned-char"/>
	<c option="-DANDROID_PLATFORM=1"/>
	<c option="-DANDRO=1"/>
    <c option="-D__ARM_ARCH_5__"/>
    <c option="-D__ARM_ARCH_5T__"/>
    <c option="-D__ARM_ARCH_5E__"/>
    <c option="-D__ARM_ARCH_5TE__"/>
    <c option="-march=armv5te"/>

	<c option="-Wall"/>
	<c option="-Werror-implicit-function-declaration"/>
	<c option="-Wunused-value"/>
	<c option="-Wunused-function"/>
	<c option="-Wunused-variable"/>
	<c option="-Wno-multichar"/>
	
	<library name="-Wl,-z,muldefs"/>
	<library name="-L$(ANDRO_SYSROOT)/usr/lib"/>
	<library name="-lm"/>
	<library name="-ldl"/>
	<library name="-fPIE -pie"/>
<!--
	<library name="-lrt"/>
	<library name="-lpthread"/>
	<library name="-lresolv"/>
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
