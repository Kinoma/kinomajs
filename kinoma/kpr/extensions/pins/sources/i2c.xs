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
	<target name="USEI2C">
		<patch prototype="PINS.prototypes">
            <object name="i2c" c="xs_i2c" script="true">
                <undefined name="bus"/>
                <number name="clock"/>
                <number name="sda"/>
                <number name="address"/>
                <function name="init" c="xs_i2c_init"/>
                <function name="readBlock" params="count, format" c="xs_i2c_readBlock"/>
                <function name="readByte" params="" c="xs_i2c_readByte"/>
                <function name="readByteDataSMB" params="" c="xs_i2c_readByteDataSMB"/>
                <function name="readWordDataSMB" params="" c="xs_i2c_readWordDataSMB"/>
                <function name="readBlockDataSMB" params="reg, count, format" c="xs_i2c_readBlockDataSMB"/>
                <function name="writeByte" params="byte" c="xs_i2c_writeByte"/>
                <function name="writeBlock" params="data" c="xs_i2c_writeBlock"/>
                <function name="writeByteDataSMB" params="reg, byte" c="xs_i2c_writeByteDataSMB"/>
                <function name="writeWordDataSMB" params="reg, word" c="xs_i2c_writeWordDataSMB"/>
                <function name="writeBlockDataSMB" params="reg, data" c="xs_i2c_writeBlockDataSMB"/>
                <function name="writeQuickSMB" params="byte" c="xs_i2c_writeQuickSMB"/>
                <function name="processCallSMB" params="reg, value" c="xs_i2c_processCallSMB"/>
				<function name="RDWR" params="messages" c="xs_i2c_RDWR"/>
                <function name="close" c="xs_i2c_close"/>
            </object>
		</patch>
		<patch prototype="PINS.constructors">
			<function name="I2C" params="it" prototype="PINS.prototypes.i2c">
                if (!("address" in it))
                    it = it.sandbox;

                this.address = it.address;
                if ("bus" in it)
                    this.bus = it.bus;
                else {
                    this.clock = it.clock;
                    this.sda = it.sda;
                }
			</function>
		</patch>
    </target>
</package>
