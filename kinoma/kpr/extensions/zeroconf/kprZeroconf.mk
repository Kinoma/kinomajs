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

<input name="$(F_HOME)/kinoma/kpr/"/>
<input name="$(F_HOME)/kinoma/kpr/sources/"/>

<header name="kpr.h"/>
<source name="kprZeroconfAdvertisement.c"/>
<source name="kprZeroconfBrowser.c"/>
<source name="kprZeroconfCommon.c"/>

<platform name="android">
	<source name="kprZeroconfAndroid.c"/>
	<source name="kprZeroconfAndroid.java"/>
</platform>
<platform name="iphone">
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="mac">
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="linux">
	<library name="-ldns_sd"/>
	<source name="kprZeroconfApple.c"/>
</platform>
<platform name="win">
	<input name="$(F_HOME)/libraries/BonjourSDK/Include/"/>
	<source name="kprZeroconfApple.c"/>
	<library name="/NODEFAULTLIB:libcmt.lib"/>
	<library name="$(F_HOME)/libraries/BonjourSDK\Lib\Win32\dnssd.lib"/>
</platform>
</makefile>