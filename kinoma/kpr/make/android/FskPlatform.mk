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
	<input name="$(F_HOME)/kinoma/kpr/patches"/>
	<input name="$(F_HOME)/core/base"/>
	<input name="$(F_HOME)/core/grammars"/>
	<input name="$(F_HOME)/core/graphics"/>
	<input name="$(F_HOME)/core/managers"/>
	<input name="$(F_HOME)/core/misc"/>
	<input name="$(F_HOME)/core/network"/>
	<input name="$(F_HOME)/core/scripting"/>
	<input name="$(F_HOME)/core/ui"/>
	<input name="$(F_HOME)/extensions/crypt/sources"/>
	<input name="$(F_HOME)/xs/includes"/>
	<input name="$(F_HOME)/libraries/QTReader"/>
	<input name="$(F_HOME)/libraries/libjpeg"/>
	<input name="$(F_HOME)/libraries/resolv"/>
	<input name="$(F_HOME)/xs/includes"/>
	<input name="$(F_HOME)/kinoma/mediareader"/>
	<input name="$(F_HOME)/kinoma/mediareader/sources"/>

	<header name="FskPlatform.h"/>

	<c option="-DKPR_CONFIG=1"/>

	<c option="-D__ARM_ARCH_5__"/>
	<c option="-D__ARM_ARCH_5T__"/>
	<c option="-D__ARM_ARCH_5E__"/>
	<c option="-D__ARM_ARCH_5TE__"/>
	<c option="-DANDROID"/>
	<c option="-DSK_RELEASE"/>
	<c option="-DNDEBUG"/>
	<configuration name="!cmake">
		<c option="-UDEBUG"/>
	</configuration>
	
	<c option="-march=armv5te"/>
	<c option="-mtune=xscale"/>
	<c option="-msoft-float"/>
	<c option="-mthumb-interwork"/>
	<c option="-fpic"/>
	<c option="-fno-exceptions"/>
	<c option="-ffunction-sections"/>
	<c option="-funwind-tables"/>
	<c option="-fstack-protector"/>
	<c option="-fmessage-length=0"/>
	<c option="-Wall"/>
	<c option="-Wno-unused"/>
	<c option="-Wno-multichar"/>
	<c option="-Wstrict-aliasing=2"/>
	<c option="-finline-functions"/>
	<c option="-finline-limit=300"/>
	<c option="-fno-inline-functions-called-once"/>
	<c option="-fgcse-after-reload"/>
	<c option="-frerun-cse-after-loop"/>
	<c option="-frename-registers"/>
	<c option="-fomit-frame-pointer"/>
	<c option="-fstrict-aliasing"/>
	<c option="-funswitch-loops"/>
	
	<c option="-DXS_SDK=0"/>
	<c option="-marm"/>
	<c option="-fno-short-enums"/>
	<c option="-DSUPPORT_NEON=1"/>
	<c option="-DSUPPORT_WMMX=1"/>
	<c option="-DANDROID=1"/>
	<c option="-DTRY_ANDROID_DD=1"/>
	<c option="--static"/>
	<c option="-DBAD_FTRUNCATE=1"/>
	<c option="-DUSE_POLL=1"/>
	<c option="-DFSK_MISALIGNED_READS=1"/>
	<c option="-DUSE_FRAMEBUFFER_VECTORS=1"/>
	<c option="-DUSE_POSIX_CLOCK=1"/>
	<c option="-DUSE_ASYNC_RESOLVER=1"/>
	<c option="-DFSK_APPLICATION=\&quot;k3remote\&quot;"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-DFSK_ZIP=1"/>
	<c option="-DFSK_TEXT_FREETYPE=1"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-D__FSK_LAYER__=1"/>
	<c option="-D_REENTRANT"/>
	<c option="-fsigned-char"/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-Wno-multichar"/>
	<c option="-Werror-implicit-function-declaration"/>
	<c option="-Wall"/>

	<version name="debug">
		<c option="-DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)"/>
		<c option="-DmxDebug"/>
		<c option="-g"/>
		<c option="-O0"/>
	</version>
	
	<version name="release">
		<c option="-DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)"/>
		<c option="-O2"/>
	</version>
</makefile>