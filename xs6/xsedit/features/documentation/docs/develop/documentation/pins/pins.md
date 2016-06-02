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
<!--
Primary author: Peter Hoddie
Last reviewed: October 2015 (but the wrong version)

Intro to this document for the Kinoma Documentation page:

This document describes how to program Kinoma Create’s hardware pins, and serves as a handy reference to the pin types supported by Kinoma Create.
-->

# Programming with Hardware Pins

This document describes how to program Kinoma Create’s hardware pins, and serves as a handy reference to the pin types supported by Kinoma Create.


##About the Pins

<!--From CR re the following: What is "SPI"? It never comes up again in this document. 

Note that I later add mention of audio--as I understand it, a hardware protocol that (strictly speaking) no pins "implement," hence its not being listed here. 

Also note that most other docs (including Getting Started with Hardware) call A2D "Analog In"; can/should this be made more consistent?
-->

Kinoma Create has 66 hardware pins. These pins implement Digital Input, Digital Output, Analog-to-Digital Input (A2D), Pulse Width Modulation (PWM), I<sup>2</sup>C, Serial Input/Output, and SPI.

There are 16 pins on the front-facing header, divided into left and right groups of eight pins each (see Figure 1). These pins are user-configurable using the Front Pins app on Kinoma Create. The left and right groups are independently configurable to operate at either 3.3 or 5 volts.

**Figure 1.** Front-Header Pins  

![](http://kinoma.com/develop/documentation/pins/img/create-front-pins.png)

<!--From CR: below I've taken a stab at resolving the following issues: The pin type Audio is described later but audio is not listed/introduced here; likewise, something should be said about A2D not mapping to any specific pins.-->

As shown in Figure 2, there are another 50 pins on the back header. The majority of the back pins are fixed-function, with eight of them (pins 51 to 58) mirroring the pin configuration on the front-left pins. The back pins operate at 3.3 volts, except as noted. Four sets of the back pins map to supported protocols, as indicated. Note that:

- A2D does not map to any particular pins but rather can be supported by any of the front pins when configured. 

- Kinoma Create also supports audio, which is treated programmatically as a pin type but does not map to any pins; the microphone on the device reads audio input and the speaker returns audio output. 

<!--From CR re the following figure: I suggest making the labels consistent (matching the following) in all extant pin diagrams/maps (including the PDF, if that remains a separate doc). That would mean, e.g., changing 
Serial Tx and Rx in Getting Started with Hardware to UART TX and RX (though actually I prefer the former, capitalized as Serial TX and RX). I also suggest adding the legend to all such diagrams.-->

**Figure 2.** Back-Header Pins  

![](http://kinoma.com/develop/documentation/pins/img/create-back-pins.png)

## Introducing BLL Modules

All hardware pin programming is done in JavaScript. The non-blocking style of JavaScript programming used in HTML5 client development and `node.js` server development is applied to hardware pins by KinomaJS. KinomaJS separates application code from code that interacts with hardware pins by running the hardware pins code in a separate thread inside its own JavaScript virtual machine, called the *Hardware Pins Service.* The code for each hardware module (sensor, LED, button, motor, and so on) is contained in a JavaScript module called a *BLL.* The application communicates with the BLL using KinomaJS messages.

> **Note:** "BLL" stands for Blinking Light Library, but a BLL is not limited to blinking an LED; a BLL can be used to interact with all kinds of hardware modules.

Developers either implement their own BLL to support the hardware modules they have connected to Kinoma Create or use an existing BLL implementation. Sample BLL implementations for common hardware modules are available on the Kinoma Create website.

## Application Programming with Pins

Applications first configure the BLLs they will use, and then issue single or repeating commands to the BLL. KinomaJS messages are used to send configuration and commands from the application to BLLs via the Hardware Pins Service.

The configuration message lists each BLL the application uses together with the pins that each BLL uses to communicate to its hardware module.

The following code configures the hardware pins to work with BLLs named `buttonBLL` and `ledBLL`. The BLL code is stored in files named `buttonBLL.js` and `ledBLL.js`.

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

Applications assign a unique name to each hardware module in the configuration. In the preceding example, the button BLL is named `greenButton` and the LED BLL is named `redLED`. The application uses these names to send messages to the corresponding hardware module.

> **Note:** In this pins configuration data example, the names `greenButton` and `redLED` are defined by the application. The `require` and `pins` properties are defined by the Hardware Pins Service. The value of the `require` property is the name of the BLL file corresponding to the hardware module. The names of the properties inside the `pins` objects (that is, `led` and `button`) are defined by each individual BLL.

While not evident from the application code, the button BLL uses a Digital Input pin, and the LED BLL uses a Digital Output pin. The BLL implementation determines the type of pins (Digital Input, Digital Output, I<sup>2</sup>C, Serial, and so on) that it uses. How the pin type is determined is explained in the next section.

The LED BLL is bound to pin 21 and the button BLL to pin 23, as defined in the `pins` object.

Once the pins are configured, an application sends KinomaJS messages to the module. The following commands turn the LED on and off.

	application.invoke(new MessageWithObject("pins:/redLED/write", 1));
	application.invoke(new MessageWithObject("pins:/redLED/write", 0));

Applications retrieve values from BLLs in a similar way: invoking a message with the name of the target hardware module and the name of the command in the path. The `onComplete` function on the behavior of the invoking object will be called with the value shown below.

	application.invoke(new MessageWithObject("pins:/greenButton/get"), Message.JSON)

	function onComplete(container, message) {
		trace(" Button " + message.requestObject + "\n");
	}
	
> **Note:** The format of the value returned from the BLL is determined by the author of the BLL. The value  is always contained in `message.requestObject` and is always a valid JavaScript object, which includes simple atomic types such as `Number`, `Boolean`, and `String`. In the preceding example, the type of the returned value is `Number`.

<!--From CR: Is my separation of the JavaScript code from the XML handler below OK?-->

Applications establish repeated polling of a hardware module at a specified interval using the `repeat` query parameter. The following example polls the button at 50-millisecond intervals.

	application.invoke(new MessageWithObject("pins:/greenButton/get?
		repeat=on&interval=50&callback=/gotButton"));
	
The result is sent in a message to the application’s `gotButton` handler.

<div class="xmlcode"></div>

	<handler path="/gotButton">
		<behavior>
			<method id="onInvoke" params="container, message">
				trace(" Button " + message.requestObject + "\n");
			</method>
		</behavior>
	</handler>

To stop a repeat, send a message with `repeat=off` with the same `callback` and `interval` query parameters, as follows:

	application.invoke(new MessageWithObject("pins:/greenButton/get?
		repeat=off&interval=50&callback=/gotButton"));

Some pin types, such as Audio, support an interrupt-style callback, which uses less CPU time and provides lower latencies than periodic polling. To enable interrupt-style callbacks in place of periodic polling, specify the unique name of the hardware module from the configuration as the value of the `timer` query parameter.

<!--From CR re the following code: In the currently published version, "microphone" and "audio" are in the special green and blue highlighting explained *later*. I assume (especially since not explained till later) that the omission of the highlighting here is intentional, but please verify.-->

	var message = new MessageWithObject("pins:configure", {
		microphone: {
			require: "recordingBLL",
			pins: {
				audio: {sampleRate: 8000, channels: 1}
			}
	}});
	
	application.invoke(message);

	application.invoke(new MessageWithObject("pins:/microphone/read?repeat=on&timer=audio&callback=/gotAudio"));

When a message to send to the `pins:` service is being created, the `referrer` header must be set to the application URL. This is done automatically by `new MessageWithObject`, so applications that use the `MessageWithObject` constructor do not need to set the referrer. An application also can use `new Message` to create the message, as follows:

	var message = new Message("pins:...");
	message.requestObject = { parameters };
	message.setRequestHeader("referrer", "xkpr://" + application.id);

## BLL Programming with Pins

As mentioned earlier, a BLL is a JavaScript module that communicates directly with hardware pins. The BLL is configured by the application, which communicates with the BLL exclusively using KinomaJS messages.

The BLL module exports a `pins` object that defines the type of pins it uses. Here is the `pins` export corresponding to `ledBLL` as introduced earlier:

<!--From CR re the following code: In the currently published version, `type` is highlighted in aqua here and in the next bit of code (and everywhere else in this document, for that matter), though I'm not sure why; OK that it's black here?-->

	exports.pins = {
		led: {type: "Digital", direction: "output"}
	};

Here is the `pins` export corresponding to `buttonBLL` as introduced earlier:

	exports.pins = {
		button: {type: "Digital", direction: "input"}
	};

<!--From CR: Kouis has highlighted BLL-defined data in blue and application-defined data in green based on the old/existing Programming with Hardware Pins documentation, though there are some differences (e.g., `type` is aqua in the current version). I've commented only on differences that don't seem right to me; please carefully review all highlighting.--> 

The Hardware Pins Service merges the pin configuration data provided by the application (highlighted in <span class="app-defined">green</span> below) with the properties of the `pins` object exported by the BLL (highlighted in <span class="bll-defined">blue</span>), to arrive at the following complete configurations:

<pre><code>redLED_configuration = {
	<span class="bll-defined">led</span>: {<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 21, <span class="bll-defined">direction</span>: "output"}
}

greenButton_configuration = {
	<span class="bll-defined">button</span>: {<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 23, <span class="bll-defined">direction</span>: "input"}
}</code></pre>
	
The Hardware Pins Service then loads the appropriate BLL using the `require` function, and instantiates JavaScript objects to communicate with the hardware pins. In this example, it instantiates two Digital pin objects, one configured as an input and the other as an output. The input is bound to pin 23 and the output to pin 21. These JavaScript objects are assigned to the module using the property names given by the BLL (`led` and `button` in this example).

Once the objects are instantiated and bound, the Hardware Pins Service invokes the `configure` function of each BLL. The `configure` function must initialize each of the objects by calling the object's `init` function, and it can do any additional work required by the BLL.

	exports.configure = function() {
		this.led.init();
	}

	exports.configure = function() {
		this.button.init();
	}
	
The BLL also defines a `close` function, which is called automatically when the host application exits. The `close` function typically closes the objects used to communicate with pins, as follows:

	exports.close = function() {
		this.led.close();
	}

	exports.close = function() {
		this.button.close();
	}
	
The `configure` and `close` functions are the only functions defined by the Hardware Pins Service. The BLL author defines the additional functions required to work with the module. Here are the `read` and `write` functions for the `led` and `button` objects:

	exports.read = function() {
		return this.button.read();
	}

	exports.write = function(value) {
		this.led.write(value ? 1 : 0);
	}
	
<!--From CR: Is my addition of the descriptive term "property" after `requestObject` correct below?-->

The value returned by a BLL function is returned to the application as the `requestObject` property of the message that invoked the function. If no value is returned, the `requestObject` value is `undefined`.

In some situations a BLL needs to instantiate a hardware pin dynamically, instead of having it automatically instantiated before the BLL’s `configure` function is called. A BLL can use the `PINS.create` function to instantiate a hardware pin at any time by passing the following configuration:

<!--From CR re the following code: I'd expect the special blue/green highlighting to be applied here. Intentional omission?-->

	var digitalOut = PINS.create({type: "Digital", pin: 23, direction: "output"});
	var i2c = PINS.create({type: "I2C", sda: 27, clock: 29, address: 0x39});

## Hardware Pins Reference

This section describes the pins data format that an application uses to configure a BLL. The configuration data defines the type of each pin and the pin number used. For each type, the reference section begins by showing the full pin configuration data. Data in <span class="bll-defined">blue</span> is defined by the BLL; data in <span class="app-defined">green</span>, by the application. The API calls supported for each pin type are then described.

> **Note:** The object for each pin type has an `init` function that reserves any hardware resources required by the object, and a `close` function to release the resources used by the object. The object will not operate properly until the `init` function is called.

<!--From CR: Eventually the sections below may link to the Samples pages currently linked to in Getting Started with Hardware, which are named exactly like the pin types except for:

A2D -> Analog In Samples    (Can/should these correlate more closely?)
I2C -> I<sup>2</sup>C Samples
-->


### Digital

<pre><code>{<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 2, <span class="bll-defined">direction</span>: "input"};
{<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 3, <span class="bll-defined">direction</span>: "output"};</code></pre>

`digital.read()` -- Returns 0 or 1, or `undefined` if the read fails. This function is used to retrieve either the value of an input pin or the value being sent by an output pin.

<!--From CR: Should the following say "must be 0 or 1"?-->

`digital.write(value)` -- `value` is 0 or 1.

<!--From CR re the following: I think it could/should be clearer that the values stated below are strings. In a different context, Mike Jennings is proposing/pushing for always showing string values with the quote marks around them; if we don't do that (which would affect **many** documents), I'll clarify here which params are strings (e.g., "Value is the string ...")--or should I take a stab at changing the format to be more standard, which would show and describe each param separately (along with its type)?-->

`digital.direction` -- Value is `input` or `output`. Note that `direction` is a data property, not a function.

### A2D

<pre><code>{<span class="bll-defined">type</span>: "A2D", <span class="app-defined">pin</span>: 3};</code></pre>

`a2d.read()` -- Returns a floating-point value from 0 to 1.0

### PWM

<pre><code>{<span class="bll-defined">type</span>: "PWM", <span class="app-defined">pin</span>: 54};</code></pre>

	
`pwm.write(value[, period])` -- Without *period*, sets the duty cycle to *value* (a floating-point value between 0 and 1.0). 

When the *period* argument is specified, sets the PWM output pulse width to *value* and the period (cycle duration, or 1/*frequency*) to *period*. Pulse width and period are specified in milliseconds. Pulse width and period can also be specified as an array, for example `pwm.write([width, period])`

Kinoma Create front panel PWMs default to a period of 20ms (50Hz) when a single duty cycle parameter is passed. Specified pulse widths are rounded to multiples of 128µs.  The maximum period is 32.64ms (30.64Hz).

Kinoma Create rear header PWMs only support parameters specifying duty cycle. The PWM frequency of the rear header PWMs is fixed at 12.7KHz (78770ns/cycle) and specified pulse widths are rounded to 1023 multiples of 77ns).


### Serial

<pre><code>{<span class="bll-defined">type</span>: "Serial", <span class="app-defined">rx</span>: 33, <span class="app-defined">tx</span>: 31, <span class="bll-defined">baud</span>: 38400};</code></pre>

`serial.read(format)` -- Reads data from the serial input. Pass `Chunk` for the `format` parameter to return the data in a chunk, `String` to return a string, `Array` to return an array of integer character codes between 0 and 255, and `charCode` to return a single character code as an integer. Note that the data read in a string must be valid UTF-8 data; if it is not, an exception is thrown. All formats except `charCode` return all the data that is immediately available in the input.

`serial.read(format, maximumBytes)` -- Same as `serial.read(format)` but reads no more than `maximumBytes`. This call does not block, so only immediately available bytes are returned. This function is not supported for the `charCode` format, which returns either 1 or 0 bytes.

`serial.read(format, maximumBytes, msToWait)` -- Same as `serial.read(format, maximumBytes)` but waits up to `msToWait` milliseconds for `maximumBytes` to arrive.

<!--From CR: Is my rewrite of what can be written to the output below OK? To me it makes more sense than expressing the formats as they would be specified in the call to `serial.read`.-->

`serial.write(value1, value2, ...)` -- Writes all arguments to the output. Each value can be a chunk, a string, an array of integer character codes between 0 and 255, or a single integer character code. The following example writes a string followed by a carriage return-line feed and `null`.

	var crlf = new Chunk(2);
	crlf[0] = 13;
	crlf[1] = 10;
	serial.write("Hello, world.", [crlf, 0]);

> **Note:** If the serial device is read-only or write-only, the corresponding pin, `rx` or `tx`, may be excluded from the pin configuration data.

### I2C

<pre><code>{<span class="bll-defined">type</span>: "I2C", <span class="app-defined">sda</span>: 27, <span class="app-defined">clock</span>: 29, <span class="bll-defined">address</span>: 0x36};</code></pre>

> **Note:** The `address` property is the I<sup>2</sup>C slave address.  The data sheet for some I²C devices specifies different slave addresses for read and write, differing only in the least significant bit. If this is the case, use the most significant 7 bits for the slave address here.

`i2c.readByte()` -- Reads one byte <!--(Linux: `read`)-->

`i2c.readBlock(count, format)` -- Reads the number of bytes specified by `count`. Pass `Chunk` for the `format` parameter to return the data in a chunk, or `Array` to return an array of integer character codes between 0 and 255. <!--(Linux: `read`)-->

`i2c.readByteDataSMB(register)` -- Reads one byte from the specified register <!--(Linux: `i2c_smbus_read_byte_data`)-->

`i2c.readWordDataSMB(register)` -- Reads two bytes from the specified register <!--(Linux: `i2c_smbus_read_word_data`)-->

`i2c.readBlockDataSMB(register, count, format)` -- Reads `count` bytes starting at the specified register. Pass `Chunk` for the `format` parameter to return the data in a chunk, or `Array` to return an array of integer character codes between 0 and 255. <!--(Linux: `i2c_smbus_read_i2c_block_data`)-->

`i2c.writeByte(value)` -- Writes one byte <!--(Linux: `write`)-->

`i2c.writeBlock(value, ...)` -- Writes the values provided. The value(s) are treated in the same way as by `serial.write`. <!--(Linux: `write`)-->

`i2c.writeByteDataSMB(register, value)` -- Writes one byte to the specified register <!--(Linux: `i2c_smbus_write_byte_data`)-->

`i2c.writeWordDataSMB(register, value)` -- Writes two bytes to specified register <!--(Linux: `i2c_smbus_write_word_data`)-->

`i2c.writeBlockDataSMB(register, value, ...)` -- Writes up to 32 bytes starting at the specified register. The value(s) are treated in the same way as by `serial.write`. <!--(Linux: `i2c_smbus_write_i2c_block_data)`-->

`i2c.writeQuickSMB(value)` -- Sends the low bit of `value` using the I<sup>2</sup>C `write_quick` command <!--(Linux: `i2c_smbus_write_quick`)-->

`i2c.processCallSMB(register, value)` -- Writes two bytes to the specified register and, after the write completes, reads two bytes and returns the resulting data word

> **Note:** Kinoma Create has up to three separate I<sup>2</sup>C buses. The primary I<sup>2</sup>C bus corresponds to pins 27 and 29 on the back pins connector. The front-left and front-right pin headers can be configured to have I<sup>2</sup>C pins using the Front Pins app. When configured to have I<sup>2</sup>C pins, each front pin header is a separate I<sup>2</sup>C bus. Having multiple I<sup>2</sup>C buses is very convenient when multiple components in a project share the same I<sup>2</sup>C slave address.

### Audio

<pre><code>{<span class="bll-defined">type</span>: "Audio", <span class="app-defined">sampleRate</span>: 8000, <span class="app-defined">channels</span>: 1, <span class="bll-defined">direction</span>: "input"};</code></pre>

`audio.read()` -- Reads all available audio input and returns it in a chunk

`audio.write(chunk)` -- Writes `chunk` to the queue for audio output

`audio.start()` -- Begins playback (output) or recording (input)

`audio.stop()` -- Ends playback (output) or recording (input)

`audio.setVolume(value)` -- Sets the volume level of an audio output to a value from 0 to 1.0

## Wait Functions

<!--From CR: This used to be a subsection of the Hardware Pins Reference section, but it (a) doesn't fit with what that section's intro says it describes, and (b) isn't strictly reference, as it begins with an overview. As a quick fix, I just moved it out into its own section.-->

The Hardware Pins Service provides functions to wait a specified period of time. These functions are useful because many hardware components require a period of time to elapse between certain operations. The wait functions are synchronous; that is, they block. Because Kinoma Create runs BLLs in their own thread, these wait functions do not block the application driving the user interface; however, they do block other messages pending with the BLL.

> **Note:** The wait functions run in Linux user space. This means that the exact amount of time that the function waits may be longer than requested. This is not usually significant, because Kinoma Create has more than enough CPU power for most projects.

`sensorUtils.delay(seconds)` -- Waits the specified number of seconds. The `seconds` argument is interpreted as an integer, so any fractional portion is ignored.

`sensorUtils.mdelay(milliseconds)` -- Waits the specified number of milliseconds (thousandths of a second). The `milliseconds` argument is interpreted as an integer, so any fractional portion is ignored.

`sensorUtils.udelay(microseconds)` -- Waits the specified number of microseconds (millionths of a second). The `microseconds` argument is interpreted as an integer, so any fractional portion is ignored. A microsecond is a very short period of time, making this function considerably less precise than `sensorUtils.delay` and `sensorUtils.mdelay`.
