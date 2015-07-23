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
	<target name="USESERIAL">
		<patch prototype="PINS.prototypes">
			<object name="serial" c="xs_serial">
				<number name="rx"/>
				<number name="tx"/>
				<string name="path"/>
				<number name="baud"/>
                <function name="init" c="xs_serial_init"/>
                <function name="read" params="format, count" c="xs_serial_read"/>   <!-- count is optional maximum read size -->
                <function name="write" params="value" c="xs_serial_write"/>
                <function name="close" c="xs_serial_close"/>
                <function name="repeat" params="" c="xs_serial_repeat" />
			</object>
		</patch>
		<patch prototype="PINS.constructors">
			<function name="Serial" params="it" prototype="PINS.prototypes.serial">
                if (!("rx" in it) && !("tx" in it) && !("path" in it))
                    it = it.sandbox;
				this.rx = it.rx;
				this.tx = it.tx;
				this.path = it.path;
				this.baud = it.baud;
			</function>
		</patch>
    </target> 
</package>
