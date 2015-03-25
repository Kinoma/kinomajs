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
	<target name="USESPI">
		<object name="spi" script="true">
			<function name="init" params="pins, chipSelect, mode, speed, bitsPerWord" c="xs_spi_init" script="true"/>
			<function name="read" params="pins, chipSelect, length" c="xs_spi_read" script="true"/>
			<function name="write" params="pins, chipSelect, data" c="xs_spi_write" script="true"/>
			<function name="transfer" params="pins, chipSelect, data" c="xs_spi_transfer" script="true"/>
			<function name="close" params="pins, chipSelect" c="xs_spi_close" script="true"/>
		</object>
 		<patch prototype="PINS.prototypes">
			<object name="spi">
				<number name="pin"/>
				<number name="chipSelect"/>
				<number name="mode"/>
				<number name="speed"/>
				<number name="bitsPerWord"/>
				<function name="init">
					return spi.init([this.pin], this.chipSelect, this.mode, this.speed, this.bitsPerWord);
				</function>
				<function name="read">
					return spi.read([this.pin], this.chipSelect, length);
				</function>
				<function name="transfer" params="data">
					return spi.transfer([this.pin], this.chipSelect, data);
				</function>
				<function name="write" params="data">
					return spi.write([this.pin], this.chipSelect, data);
				</function>
				<function name="close">
					return spi.close([this.pin], this.chipSelect);
				</function>
			</object>
		</patch>
		<patch prototype="PINS.constructors">
			<function name="SPI" params="it" prototype="PINS.prototypes.spi">
				this.pin = it.pin;
				this.chipSelect = it.chipSelect;
				this.mode = it.mode;
				this.speed = it.speed;
				this.bitsPerWord = it.bitsPerWord;
			</function>
		</patch>
   </target>
</package>