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

<input name="$(F_HOME)/libraries/expat"/>
<input name="$(F_HOME)/extensions/http/sources/"/>
<input name="$(F_HOME)/kinoma/kpr/"/>
<input name="$(F_HOME)/kinoma/kpr/sources/"/>
<input name="$(F_HOME)/kinoma/mediatranscoder/sources/"/>
<header name="kpr.h"/>

<platform name="android">
	<header name="kprLibraryServer.h"/>
	<header name="kprLibrary.h"/>
	<source name="kprLibraryServer.c"/>
	<source name="kprLibraryAndroid.c"/>
	<source name="kprProxyServer.c"/>
</platform>

<platform name="mac">
	<input name="sqlite"/>
	<header name="sqlite3.h"/>
	<header name="kprLibraryServer.h"/>
	<header name="kprLibrary.h"/>
	<source name="sqlite3.c"/>
	<source name="kprSQLite.c"/>
	<source name="kprLibraryServer.c"/>
	<source name="kprLibrarySQLite.c"/>
	<source name="kprProxyServer.c"/>
</platform>

<platform name="iphone">
	<source name="kprLibraryiOS.m"/>
	<source name="kprLibraryServer.c"/>
	<source name="kprProxyServer.c"/>
</platform>

</makefile>