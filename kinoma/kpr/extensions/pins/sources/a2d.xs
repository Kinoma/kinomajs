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
	<target name="USEA2D">
		<patch prototype="PINS.prototypes">
			<object name="a2d" c="xs_a2d">
				<number name="pin"/>
				<function name="init" c="xs_a2d_init"/>
				<function name="read" c="xs_a2d_read"/>
				<function name="close" c="xs_a2d_close"/>
			</object>
		</patch>
		<patch prototype="PINS.constructors">
			<function name="A2D" params="it" prototype="PINS.prototypes.a2d">
                if (!("pin" in it))
                    it = it.sandbox;
				this.pin = it.pin;
			</function>
			<function name="Analog" params="it" prototype="PINS.prototypes.a2d">
                if (!("pin" in it))
                    it = it.sandbox;
				this.pin = it.pin;
			</function>
		</patch>
    </target>    
</package>