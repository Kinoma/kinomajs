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
<package script="true">
	<import href="kpr.xs" link="dynamic"/>
	<patch prototype="KPR">
		<function name="parseMarkdown" params="string" c="KPR_parseMarkdown" script="true"/>
	</patch>
	<patch prototype="KPR.text">
		<function name="find" params="pattern, mode" c="KPR_text_find"/>
		<function name="formatMarkdown" params="string, options" c="KPR_text_formatMarkdown" script="true"/>
	</patch>
</package>
