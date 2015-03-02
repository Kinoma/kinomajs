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

<input name="../crypt"/>
<input name="../crypt/sources"/>

<include name="/makefiles/xsFskDefaults.mk"/>

<platform name="Linux,MacOSX,Solaris,iPhone,android,Windows">
<common>
XSC_OPTIONS = -t client -t clientAuth -t server
</common>
</platform>
<platform name="mac-cmake">
<common>
set(XSC_OPTIONS ${XSC_OPTIONS} -t client -t clientAuth -t server)
</common>
</platform>

<include name="/makefiles/xsBinary.mk"/>

<platform name="mac-cmake">
<common>
include_directories("${BUILD_TMP}/FskSSL")
include_directories("${BUILD_TMP}/Crypt")
</common>
</platform>

</makefile>