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
<package script="true">
	<import href="kpr.xs" link="dynamic"/>

	<patch prototype="KPR">
		<object name="browser" prototype="KPR.content">
			<function name="get canBack" c="KPR_browser_get_canBack"/>
			<function name="get canForward" c="KPR_browser_get_canForward"/>
			<function name="get url" c="KPR_browser_get_url"/>
			
			<function name="set url" params="it" c="KPR_browser_set_url"/>
			
			<function name="load" params="url" c="KPR_browser_load"/>
			<function name="reload" params="" c="KPR_browser_reload"/>
			<function name="back" params="" c="KPR_browser_back"/>
			<function name="forward" params="" c="KPR_browser_forward"/>
			<function name="evaluate" params="snipet" c="KPR_browser_evaluate"/>
		</object>
	</patch>
	
	<function name="Browser" params="coordinates, url" prototype="KPR.browser" c="KPR_Browser"/>
</package>