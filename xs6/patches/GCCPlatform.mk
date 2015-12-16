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
	<c option="-Wall"/>
	<c option="-Wextra"/>
	<c option="-Wno-missing-field-initializers"/>
	<c option="-Wno-unused-function"/>
	<c option="-Wno-unused-parameter"/>

	<version name="debug">
		<c option="-DmxDebug=1"/>
		<c option="-g"/>
		<c option="-O0"/>
	</version>

	<version name="release">
		<c option="-O3"/>
	</version>
</makefile>
