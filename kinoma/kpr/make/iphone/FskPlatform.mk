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
	<input name="$(F_HOME)/xs/includes"/>
	<input name="$(F_HOME)/core/base"/>
	<input name="$(F_HOME)/core/graphics"/>
	<input name="$(F_HOME)/core/managers"/>
	<input name="$(F_HOME)/core/misc"/>
	<input name="$(F_HOME)/core/network"/>
	<input name="$(F_HOME)/core/ui"/>
	<input name="$(F_HOME)/extensions/crypt/sources"/>
	<input name="$(F_HOME)/libraries/expat"/>
	<input name="$(F_HOME)/libraries/QTReader"/>

	<import name="GCCPlatform.mk" />
	
	<header name="FskPlatform.h"/>
	
	<c option="$(ARCH)"/>
	<c option="-fasm-blocks"/>
	<c option="-fobjc-abi-version=2"/>
	<c option="-fobjc-legacy-dispatch"/>
	<c option="-fexceptions"/>
	<c option="-fmessage-length=0"/>
	<c option="-fpascal-strings"/>
	<c option="-fvisibility=hidden"/>
	<c option="-isysroot $(SDKROOT)"/>
	<c option="$(VERSION_MIN)"/>
	
	<c option="-D__FSK_LAYER__=1"/>
	<c option="-DTARGET_API_IPHONE=1"/>
	<c option="-D_Bool=int"/>
	<c option="-Dmacintosh=1"/>
	<c option="-Diphone=1"/>
	<c option="-D_REENTRANT=1"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-DSUPPORT_DLLEXPORT=1"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-DFSK_APPLICATION=\&quot;PLAY\&quot;"/>
	<c option="-DFSK_APPLICATION_PLAY=1"/>
	<c option="-DKPR_CONFIG=1"/>
	
	<c option="&quot;-DIBOutlet=__attribute__((iboutlet))&quot;"/>
	<c option="&quot;-DIBOutletCollection(ClassName)=__attribute__((iboutletcollection(ClassName)))&quot;"/>
	<c option="&quot;-DIBAction=void)__attribute__((ibaction)&quot;"/>

	<!-- define FskJPEGDecode/FskJPEGEncode options temporarily here -->
	<c option="-DNO_GETENV"/>
	<c option="-DKINOMA_YUV2RGB565_IFAST_ARM_SUPPORTED"/>
	<c option="-DKINOMA_IDCT_IFAST_SUPPORTED"/>
	<c option="-DKINOMA_DCT_IFAST_SUPPORTED"/>
	<!-- only for the case of cmake ide. simulator is broken in cmake so it does not matter to define this here -->
	<c option="-DSUPPORT_NEON_IOS=1"/>
</makefile>