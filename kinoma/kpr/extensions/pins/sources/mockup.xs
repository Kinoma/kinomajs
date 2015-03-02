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
<package script="true">
	<target name="MOCKUP">
		<patch prototype="PINS.prototypes">
			<object name="a2d">
				<number name="pin"/>
			</object>
			<object name="digital">
				<number name="pin"/>
				<string name="direction"/>
                <function name="repeat" params="poller">
                	return 50;
                </function>
			</object>
			<object name="i2c">
				<number name="address"/>
				<number name="clock"/>
				<number name="sda"/>
			</object>
			<object name="pwm">
				<number name="pin"/>
			</object>
			<object name="serial">
				<number name="rx"/>
				<number name="tx"/>
				<number name="baud"/>
			</object>
			<object name="spi">
				<number name="pin"/>
				<number name="chipSelect"/>
				<number name="mode"/>
				<number name="speed"/>
				<number name="bitsPerWord"/>
			</object>
		</patch>
		
		<patch prototype="PINS.constructors">
			<function name="A2D" params="it" prototype="PINS.prototypes.a2d">
				this.pin = it.pin;
			</function>
			<function name="Digital" params="it" prototype="PINS.prototypes.digital">
				this.pin = it.pin;
				this.direction = it.direction;
			</function>
			<function name="I2C" params="it" prototype="PINS.prototypes.i2c">
				this.address = it.address;
				this.clock = it.clock;
				this.sda = it.sda;
			</function>
			<function name="PWM" params="it" prototype="PINS.prototypes.pwm">
				this.pin = it.pin;
			</function>
			<function name="Serial" params="it" prototype="PINS.prototypes.serial">
				this.rx = it.rx;
				this.tx = it.tx;
				this.baud = it.baud;
			</function>
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