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
		<object name="debug" c="KPR_debug">
			<function name="abort" c="KPR_debug_abort"/>
			<function name="addBreakpoint" params="address, path, line" c="KPR_debug_addBreakpoint"/>
			<function name="addBreakpoints" params="address, breakpoints" c="KPR_debug_addBreakpoints"/>
			<function name="close" c="KPR_debug_close"/>
			<function name="file" params="address, view, path, line" c="KPR_debug_file"/>
			<function name="get behavior" c="KPR_debug_get_behavior"/>
			<function name="get machines" c="KPR_debug_get_machines"/>
			<function name="go" c="KPR_debug_go"/>
			<function name="logout" c="KPR_debug_logout"/>
			<function name="set behavior" params="it" c="KPR_debug_set_behavior"/>
			<function name="removeBreakpoint" params="address, path, line" c="KPR_debug_removeBreakpoint"/>
			<function name="resetBreakpoints" params="address" c="KPR_debug_resetBreakpoints"/>
			<function name="step" c="KPR_debug_step"/>
			<function name="stepIn" c="KPR_debug_stepIn"/>
			<function name="stepOut" c="KPR_debug_stepOut"/>
			<function name="toggle" params="address, view, value" c="KPR_debug_toggle"/>
			<!-- ids for callbacks to the behavior -->
			<null name="onMachineRegistered"/> <!-- params="address" -->
			<null name="onMachineTitleChanged"/> <!-- params="address, title" -->
			<null name="onMachineUnregistered"/> <!-- params="address" -->
			<null name="onMachineViewChanged"/> <!-- params="address, view, lines" -->
			<null name="onMachineViewPrint"/> <!-- params="address, view, line" -->
		</object>
		<function name="Debug" params="dictionary" prototype="KPR.debug" c="KPR_Debug"/>
		<function name="gotoFront" c="KPR_gotoFront"/>
	</patch>
	<patch prototype="system">
		<function name="getenv" params="name" c="KPR_system_getenv"/>
	</patch>
	<patch prototype="Files">
		<function name="toPath" params="url" c="KPR_Files_toPath"/>
		<function name="toURI" params="path" c="KPR_Files_toURI"/>
	</patch>
</package>

