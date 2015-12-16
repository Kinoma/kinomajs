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
<package>
	<import href="xsChunk.xs"/>
	<import href="xsGrammar.xs"/>
	<import href="FskStream.xs"/>
	<object name="FileSystem">
		<function name="getExtension" params="path"><![CDATA[
			var index = path.lastIndexOf(".")
			if (index < 0)
				return ""
			return (path.substring(index + 1)).toLowerCase()
		]]></function>
		<function name="getFileInfo" params="path">
			return Files.exists("file://" + path); // @@ FskSSL uses getFileInfo to know if a file exists
		</function>
		<function name="getMIMEType" params="path">
		</function>
	</object>
</package>
