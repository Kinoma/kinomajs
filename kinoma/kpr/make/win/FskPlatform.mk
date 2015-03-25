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
	<input name="$(F_HOME)/kinoma/kpr/patches"/>
	<input name="$(F_HOME)/core/base"/>
	<input name="$(F_HOME)/core/graphics"/>
	<input name="$(F_HOME)/core/managers"/>
	<input name="$(F_HOME)/core/misc"/>
	<input name="$(F_HOME)/core/network"/>
	<input name="$(F_HOME)/core/ui"/>
	<input name="$(F_HOME)/extensions/crypt/sources"/>
	<input name="$(F_HOME)/xs/includes"/>
	<input name="$(F_HOME)/libraries/DirectX"/>
	<input name="$(F_HOME)/libraries/expat"/>
	<input name="$(F_HOME)/libraries/QTReader"/>
	<input name="$(F_HOME)/libraries/zlib"/>

	<header name="FskPlatform.h"/>

	<c option="/c"/>
	<c option="/EHsc"/>
	<c option="/nologo"/>
	<c option="/W3"/>

	<c option="/D__FSK_LAYER__"/>
	<c option="/D_CRT_SECURE_NO_DEPRECATE"/>
	<c option="/D_MBCS"/>
	<c option="/D_WINDOWS"/>
	<c option="/DCLOSED_SSL=1"/>
	<c option="/DFSK_APPLICATION=\&quot;PLAY\&quot;"/>
	<c option="/DFSK_APPLICATION_PLAY=1"/>
	<c option="/DFSK_EMBED=1"/>
	<c option="/DFSK_EXTENSION_EMBED=1"/>
	<c option="/DKPR_CONFIG=1"/>
	<c option="/DSONY=0"/>
	<c option="/DSUPPORT_DLLEXPORT=1"/>
	<c option="/DSUPPORT_XS_DEBUG=1"/>
	<c option="/DTARGET_OS_WIN32=1"/>
	<c option="/DWIN32"/>
	<c option="/DFSK_WINDOWS_DEFAULT_WINDOW_ICON_ID=IDI_ICON1"/>

	<library name="/nologo"/>
	<library name="/nodefaultlib:libmmt.lib"/>
	<library name="/nodefaultlib:libircmt.lib"/>
	<library name="/nodefaultlib:svml_disp.lib"/>

	<!-- Handled by CMake
	<library name="/incremental"/>
	<library name="/machine:I386"/>
	<library name="/implib:$(BUILD_TMP)\fsk.lib"/>
	<library name="/manifest"/>
	<library name="/manifestfile:$(BUILD_TMP)\fsk.manifest"/>
	-->

	<version name="debug">
		<c option="/DmxDebug=1"/>
		<c option="/D_DEBUG"/>
		<c option="/MTd"/>
		<c option="/Od"/>
		<c option="/RTC1"/>
		<c option="/ZI"/>
	
		<library name="/debug"/>
		<library name="/nodefaultlib:msvcrtd"/>
	</version>

	<version name="release">
		<c option="/DNDEBUG"/>
		<c option="/GL"/>
		<c option="/O2"/>
		<c option="/MT"/>
	
		<library name="/nodefaultlib:msvcrt"/>
	</version>
</makefile>
