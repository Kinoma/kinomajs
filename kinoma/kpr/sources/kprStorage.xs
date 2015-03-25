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
	<object name="localStorage">
		<function name="clear" params="key" c="KPR_localStorage_clear"/>
		<function name="getItem" params="key" c="KPR_localStorage_getItem"/>
		<function name="get length" params="key" c="KPR_localStorage_getLength"/>
		<function name="key" params="index" c="KPR_localStorage_key"/>
		<function name="removeItem" params="key" c="KPR_localStorage_removeItem"/>
		<function name="setItem" params="key, value" c="KPR_localStorage_setItem"/>
	</object>
</package>