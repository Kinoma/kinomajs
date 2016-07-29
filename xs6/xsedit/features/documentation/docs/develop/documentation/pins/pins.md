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
# Programming with Hardware Pins
Kinoma Create has 66 hardware pins. These pins implement Digital Input, Digital Output, Analog-to-Digital Input (A2D), Pulse Width Modulation (PWM), I2C, Serial Input/Output, and SPI.

There are 16 pins on the front facing header, divided into left and right groups of 8 pins each. These pins are user configurable using the Front Pins application on Kinoma Create. The left and right groups are independently configurable to operate at either 3.3 or 5 volts. There are another 50 pins on the back. The majority of the back pins are fixed function, with 8 of the back pins mirroring the pin configuration on the Front-Left pins. The back pins operate at 3.3 volts, except as noted.

***
## Front Pins
![](http://www.kinoma.com/create/img/front-pinmap.png)
***
## Back Pins
![](http://www.kinoma.com/create/img/back-pinmap.png)

**Note**: Front facing pins 51 to 58 are mirrored on the back pins. The mirrored pins are labeled “Front-Left” above.
***
## Introducing BLL Modules
All hardware pin programming is done in JavaScript. The non-blocking style of JavaScript programming used in HTML5 client development and node.js server development is applied to hardware pins by KinomaJS. KinomaJS separates application code from code that interacts with hardware pins by running the hardware pins code in a separate thread inside its own JavaScript virtual machine called the Hardware Pins Service. The code for each hardware module (sensor, LED, button, motor, etc) is contained in a JavaScript module called a BLL. The application communicates with the BLL using KinomaJS messages.

Developers either implement their own BLL to support the hardware modules they have connected to Kinoma Create or use a pre-existing BLL implementation. Sample BLL implementations for common hardware modules are available on the Kinoma Create website.

**Note**: BLL is an acronym for Blinking Light Library. A BLL is not limited to blinking an LED. A BLL can be used to interact with all kinds of hardware modules.
***
## Application Programming with Pins

Applications first configure the BLLs they will use, and then issue single or repeating commands to the BLL. KinomaJS messages are used to send configuration and commands from the application to BLLs via the Hardware Pins Service.

The configuration message lists each BLL the application uses together with the pins that each BLL uses to communicate to its hardware module.

The following code configures the hardware pins to work with BLLs named buttonBLL and ledBLL. The BLL code is stored in files named `buttonBLL.js` and `ledBLL.js`.

	var message = new MessageWithObject("pins:configure", {
		greenButton: {
			require: "buttonBLL",
			pins: {
				button: {pin: 23}
			}
		},
		redLED: {
			require: "ledBLL",
			pins: {
				led: {pin: 21}
			}
  		}
	});
	
	application.invoke(message);
	
Applications assign a unique name to each hardware module in the configuration. In the preceding example, the button BLL is named `greenButton` and the LED is named `redLED`. The application uses these names to send messages to the corresponding hardware module.

**Note**: In this pins configuration data example, the names `greenButton` and `redLED` are defined by the application. The `require` and `pins` properties are defined by the Hardware Pins Service. The value of the `require` property is the name of the BLL file corresponding to the hardware module. The names of the properties inside the `pins` objects (e.g. `led` and `button`) are defined by each individual BLL.

While not evident from the application code, the button BLL uses a Digital Input; the LED BLL, a Digital Output. The BLL implementation determines the type of pins (Digital Input, Digital Output, I2C, Serial, etc) that it uses. How the pin type is determined will be explained below in the section BLL Programming with Pins.

The LED BLL is bound to pin 21 and the button BLL to pin 23, as defined in the pins object.

Once configured, an application sends KinomaJS messages to the module. The following commands turn the LED on and off.

	application.invoke(new MessageWithObject("pins:/redLED/write", 1));
	application.invoke(new MessageWithObject("pins:/redLED/write", 0));

Applications retrieve values from BLLs in a similar way—invoking a message with the name of the target hardware module and the name of the command in the path. The `onComplete` function on the behavior of the invoking object will be called with the value:

	application.invoke(new MessageWithObject("pins:/greenButton/get"),Message.JSON)

	function onComplete(container, message) {
		trace(" Button " + message.requestObject + "\n");
	}
	
**Note**: The format of the value returned from the BLL is determined by the author of the BLL. `message.requestObject` always contains the value. The value is always a valid JavaScript object, which includes simple atomic types such as `Number`, `Boolean`, and `String`. In the above example, the returned value is a `Number`.

Applications establish repeated polling of a hardware module at a specified interval using the `repeat` query parameter. The following example polls the button at 50 millisecond intervals. The result is sent in a message to the application’s gotButton handler.

	application.invoke(new MessageWithObject("pins:/greenButton/get?repeat=on&interval=50&callback=/gotButton"));

	<handler path="/gotButton">
		<behavior>
			<method id="onInvoke" params="container, message">
				trace(" Button " + message.requestObject + "\n");
			</method>
		</behavior>
	</handler>
	
To stop a repeat, send a message with repeat=off with the same callback and interval query parameters:

	application.invoke(new MessageWithObject("pins:/greenButton/get?repeat=off&interval=50&callback=/gotButton"));
	
Some types of pin, such as Audio, support an interrupt style callback, which uses less CPU time and provides lower latencies than periodic polling. To enable interrupt style callbacks in place of periodic polling, specific the unique name of the hardware module from the configuration as the value of the timer query parameter.

	var message = new MessageWithObject("pins:configure", {
		microphone: {
			require: "recordingBLL",
			pins: {
				audio: {sampleRate: 8000, channels: 1}
			}
    	}
    });
	application.invoke(message);

	application.invoke(new MessageWithObject("pins:/microphone/read?repeat=on&timer=audio&callback=/gotAudio"));
	
**Note**: When creating a message to send to the `pins`: service, the `referrer` header must be set to the application URL. This is done automatically by `new MessageWithObject`, so applications that use the `MessageWithObject` constructor do not need to set the referrer. An application also can use `new Message` to create the message:

	var message = new Message("pins:...");
	message.requestObject = { parameters };
	message.setRequestHeader("referrer", "xkpr://" + application.id);
***
## BLL Programming with Pins
A BLL is a JavaScript module that communicates directly with hardware pins. The BLL is configured by the application which communicates with the BLL exclusively using KinomaJS messages.

The BLL module exports a pins object which defines the type of `pins` it uses. Here is the pins export corresponding to the `ledBLL` introduced earlier:

	exports.pins = {
		led: {type: "Digital", direction: "output"}
	};
	
Here is the pins export corresponding to the `buttonBLL` introduced earlier:

	exports.pins = {
		button: {type: "Digital", direction: "input"}
	};
	
The Hardware Pins Service merges the pin configuration data provided by the application with the pins object exported by the BLLs, to arrive at the following complete configurations:

	redLED_configuration = {
		led: {type: "Digital", pin: 21, direction: "output"}
	}

	greenButton_configuration = {
		button: {type: "Digital", pin: 23, direction: "input"}
	}
	
The Hardware Pins Service then loads the appropriate BLL using the `require` function, and instantiates JavaScript objects to communicate with the hardware pins. In this example, it instantiates two Digital pin objects, one configured as an input, the other as an output. The input is bound to pin 23 and the output to pin 21. These JavaScript objects are assigned to the module using the property names given by the BLL (`led` and `button` in this example).

Once the objects are instantiated and bound, the Hardware Pins Service invokes the `configure` function of each BLL. The `configure` function must initialize each of the objects by calling their `init` function. The `configure` function can do any additional work required by the BLL.

	exports.configure = function() {
		this.led.init();
	}

	exports.configure = function() {
		this.button.init();
	}
	
The BLL also defines a `close` function, which is called automatically when the host application exits. The `close` function typically closes the objects used to communicate with pins:

	exports.close = function() {
		this.led.close();
	}

	exports.close = function() {
		this.button.close();
	}
	
The `configure` and `close` functions are the only functions defined by the Hardware Pins Service. The BLL author defines the additional functions required to work with the module. Here are the `read` and `write` functions for the led and button objects:

	exports.read = function() {
		return this.button.read();
	}

	exports.write = function(value) {
		this.led.write(value ? 1 : 0);
	}
	
The value returned by a BLL function is returned to the application as the requestObject of the message that invoked the function. If no value is returned, the `requestObject` value is `undefined`.

In some situations a BLL needs to dynamically instantiate a hardware pin, instead of having it automatically instantiated before the BLL’s `configure` function is called. A BLL can use the `PINS.create` function to instantiate a hardware pin at any time by passing the configuration:

	var digitalOut = PINS.create({type: "Digital", pin: 23,direction: "output"});
	var i2c = PINS.create({type: "I2C", sda: 27, clock: 29,address: 0x39});
***                              
## Hardware Pins Reference
This reference section describes the pins data format used to configure a BLL by an application. The configuration data defines the type of each pin and the pin number used. For each type, the reference begins by showing the full pin configuration data. Data in BLUE is defined by the BLL; data in GREEN, by the application. The API calls supported for each pin type are then given.

**Note**: The object for each pin type has an `init` function which reserves any hardware resources required by the object, and a `close` function to release the resources used by the object. The object will not operate properly until the `init` function is called.

### Digital

	{type: "Digital", pin: 2, direction: "input"};
	{type: "Digital", pin: 3, direction: "output"};
	
`digital.read()` — Returns 0 or 1, or undefined if the read fails. This is used both to retrieve the value of an input pin, or the value being output on an output pin.

`digital.write(value)` — Value is either 0 or 1

`digital.direction` — Value is either “`input`” or “`output`”. Note `direction` is a property, not a function.

### A2D
	{type: "A2D", pin: 3};
`a2d.read()` — Returns a floating point value from 0 to 1.0

### PWM
	{type: "PWM", pin: 54};
`pwm.write(value[, period])` -- Without *period*, sets the duty cycle to *value* (a floating-point value between 0 and 1.0). 

When the *period* argument is specified, sets the PWM output pulse width to *value* and the period (cycle duration, or 1/*frequency*) to *period*. Pulse width and period are specified in milliseconds. Pulse width and period can also be specified as an array, for example `pwm.write([width, period])`

Kinoma Create front panel PWMs default to a period of 20ms (50Hz) when a single duty cycle parameter is passed. Specified pulse widths are rounded to multiples of 128µs.  The maximum period is 32.64ms (30.64Hz).

Kinoma Create rear header PWMs only support parameters specifying duty cycle. The PWM frequency of the rear header PWMs is fixed at 12.7KHz (78770ns/cycle) and specified pulse widths are rounded to 1023 multiples of 77ns).

### Serial
	{type: "Serial", rx: 33, tx: 31, baud: 38400};
	
`serial.read(type)` — Reads data from the serial input. Pass `"Chunk"` for the type parameter to return the data in a Chunk, `"String"` to return a string, `"Array"` to return an array of integer character codes between 0 and 255, and “charCode” to return a single character code as an integer. Note that the data returned in a String must be valid UTF-8 data; if the data read is not valid UTF-8 an exception is thrown. All types of read, except charCode return all data that is immediately available on the input.

`serial.read(type, maximumBytes)` — Same as serial.read(type) but no more than maximumBytes are read. This call does not block, so only immediately available bytes are returned. This is not supported for type charCode which returns either 1 or 0 bytes.

`serial.read(type, maximumBytes, msToWait)` — Same as serial.read(type, maximumBytes) but waits up to msToWait milliseconds for maximumBytes to arrive.

`serial.write(value, …)` — Writes all arguments to the output. Values can be of type Chunk, String, Array of character codes, or numbers (character codes from 0 to 255). The following writes a message, followed by a carriage return, line feed, and null.

	var crlf = new Chunk(2);
	crlf[0] = 13;
	crlf[1] = 10;
	serial.write("Hello, world.", [crlf, 0]);
	
**Note**: If the serial device is read-only or write-only, the corresponding pin, rx or tx, may be excluded from the pin configuration data.

### I2C
	{type: "I2C", sda: 27, clock: 29, address: 0x36};
**Note**: The address property is the I2C slave address.  The data sheet for some I²C devices specifies different slave addresses for read and write, differing only in the least significant bit. If this is the case, use the most significant 7 bits for the slave address here.

`i2c.readByte()` — Reads one byte <!--(Linux: read)-->

`i2c.readBlock(count, format)` - reads the number of bytes specified by count. Pass “Chunk” for the type parameter to return the data in a Chunk, or “Array” to return an array of integer character codes between 0 and 255. <!--(Linux: read)-->

`i2c.readByteDataSMB(register)` — Reads 1 byte from specified register <!--(Linux: i2c_smbus_read_byte_data)-->

`i2c.readWordDataSMB(register)` — Reads 2 bytes from specified register <!--(Linux: i2c_smbus_read_word_data)-->

`i2c.readBlockDataSMB(register, count, type)` — Reads count bytes starting at the specified register. Pass "Chunk" for the type parameter to return the data in a Chunk, or "Array" to return an array of integer character codes between 0 and 255. <!--(Linux: i2c_smbus_read_i2c_block_data)-->

`i2c.writeByte(value)` — Writes one byte <!--(Linux: write)-->

`i2c.writeBlock(value...)` - writes the values provided. The value(s) are treated in the same way as serial.write(). <!--(Linux: write)-->

`i2c.writeByteDataSMB(register, value)` — Writes 1 byte to specified register <!--(Linux: i2c_smbus_write_byte_data)-->

`i2c.writeWordDataSMB(register, value)` — Writes 2 bytes to specified register <!--(Linux: i2c_smbus_write_word_data)-->

`i2c.writeBlockDataSMB(register, value...)` — Writes up to 32 bytes starting at the specified register. The value(s) are treated in the same way as `serial.write()`. <!--(Linux: i2c_smbus_write_i2c_block_data)-->

`i2c.writeQuickSMB(value)` — Sends the low bit of value using the I2C `write_quick` command <!--(Linux: i2c_smbus_write_quick)-->

`i2c.processCallSMB(register, value)` — Writes 2 bytes to specified register. After the write completes, reads 2 bytes, and returns the resulting data word.

**Note**: Kinoma Create has up to three separate I2C buses. The primary I2C bus corresponds to pins 27 and 29 on the Back Pins connector. The Front-Left and Front-Right Pin headers can be configured to have I2C pins using the Front Pins app. When configured to have I2C pins, each Front Pin header is a separate I2C bus. Having multiple I2C buses is very convenient when you have more than one component in a project that share the same I2C slave address.

### Audio
	{type: "Audio", sampleRate: 8000, channels: 1, direction: "input"};
`audio.read()` — Reads all available audio input, and returns it in a Chunk

`audio.write(chunk)` — Writes chunk to queue for audio output

`audio.start()` — Begins playback (output) or recording (input)

`audio.stop()` — Ends playback (output) or recording (input)

`audio.setVolume(value)` — Sets the volume level of an audio output to a value from 0 to 1.0

### Wait functions
The Hardware Pins Service provides functions to wait a specified period of time. These functions are useful as many hardware components require a period of time to elapse between certain operations. The wait functions are synchronous, that is they block. Because Kinoma Create runs BLLs in their own thread, these wait functions do not block the application driving the user interface. However, they do block other messages pending with the BLL.

**Note**: The wait functions run in Linux user-space. This means that the exact amount of time that the function waits may be longer than requested. This is not usually significant, as Kinoma Create has more than enough CPU power for most projects.

`sensorUtils.delay(seconds)` — Waits the specified number of seconds. The `seconds` argument is interpreted as an integer, so any fractional portion is ignored.

`sensorUtils.mdelay(milliseconds)` — Waits the specified number of milliseconds (thousandths of a second). The `milliseconds` argument is interpreted as an integer, so any fractional portion is ignored.

`sensorUtils.udelay(microseconds)` — Waits the specified number of microseconds (millionths of a second). The `microseconds` argument is interpreted as an integer, so any fractional portion is ignored. A microsecond is a very short period of time, making this function considerably less precise than `sensorUtils.delay` and `sensorUtils.mdelay`.

***

Copyright © 2015 Marvell. All rights reserved.

Marvell and Kinoma are registered trademarks of Marvell. All other products and company names mentioned in this document may be trademarks of their respective owners.
