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
	<input name="$(F_HOME)/libraries/QTReader"/>

	<import name="GCCPlatform.mk" />

	<header name="FskPlatform.h"/>

	<c option="-mmacosx-version-min=${SDKVER}"/>
	<c option="-arch i386"/>
	<c option="-fasm-blocks"/>
	<c option="-fno-common"/>
	<c option="-fvisibility=hidden"/>

	<c option="-D__FSK_LAYER__=1"/>
	<c option="-D_Bool=int"/>
	<c option="-DCLOSED_SSL=1"/>
	<c option="-DFSK_APPLICATION=\&quot;PLAY\&quot;"/>
	<c option="-DFSK_APPLICATION_PLAY=1"/>
	<c option="-DFSK_EMBED=1"/>
	<c option="-DFSK_EXTENSION_EMBED=1"/>
	<c option="-DKPR_CONFIG=1"/>
	<c option="-Dmacintosh=1"/>
	<c option="-DSUPPORT_DLLEXPORT=1"/>
	<c option="-Wno-shift-negative-value"/>		<!-- This warning is for machines that do not use twos complement. -->
	<c option="-Wno-unknown-warning-option"/>	<!-- This ignores the above flag on compilers that do not recognize it. -->
</makefile>
