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

<input name="$(F_HOME)/libraries/QTReader"/>
<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<platform name="Windows">
<input name="$(F_HOME)/../jb/sdk/QT704SDK/CIncludes/"/>
<input name="$(F_HOME)/../jb/sdk/QT704SDK/ComponentIncludes/"/>
</platform>

<include name="/makefiles/xsFskDefaults.mk"/>

<wrap name="sources/kinomaqtdecextension.c"/>
<wrap name="sources/kinomaqtdec.c"/>
<!-- -->
<!--wrap name="../kinoma-ipp-lib/kinoma_utilities.c"/-->

<platform name="Windows">
<common>
LIBRARIES = $(F_HOME)/../jb/sdk/QT704SDK/libraries/qtmlclient.lib	\
			$(F_HOME)/../jb/sdk/QT704SDK/libraries/CVClient.lib		
</common>
</platform>

<platform name="MacOSX">
<common>
LIBRARIES = -framework QuartzCore -framework QuickTime -framework Carbon
</common>
</platform>

<platform name="mac-cmake">
<common>
find_library(QUARTZCORE_LIBRARY QuartzCore)
mark_as_advanced(QUARTZCORE_LIBRARY)
set(EXTENSION_LINK_LIBS ${QUARTZCORE_LIBRARY})
</common>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>