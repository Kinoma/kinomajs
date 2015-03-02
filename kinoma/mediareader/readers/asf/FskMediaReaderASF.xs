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
<package>
	<import href="FskCore.xs" link="dynamic"/>
	
	<target name="KPR_CONFIG != true">
	<patch prototype="Media">
		<object name="asx">
			<function name="parse" params="chunk" c="xs_asx_parse"/>
			<function name="strip" params="text" c="xs_asx_strip"/>
			
			<object name="list" prototype="Array.prototype">
				<boolean name="visible"/>

				<string name="title"/>
				<string name="author"/>
				<string name="copyright"/>
			</object>
			<object name="entry">
				<null name="uri"/>

				<string name="title"/>
				<string name="author"/>
				<string name="copyright"/>

				<undefined name="start"/>
				<undefined name="duration"/>
				<boolean name="skip" value="true"/>
			</object>
		</object>
	</patch>
	</target>
</package>