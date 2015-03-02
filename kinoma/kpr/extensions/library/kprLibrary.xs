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
<package script="true">
	<import href="kpr.xs" link="dynamic"/>
	<object name="Library">
		<function name="cacheQuery" params="query" c="Library_cacheQuery"/>
		<function name="getURI" params="index" c="Library_getURI"/>
		<function name="sniffMIME" params="url, f" c="Library_sniffMIME"/>
	</object>
</package>
