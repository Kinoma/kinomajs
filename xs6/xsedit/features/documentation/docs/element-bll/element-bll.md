<!-- Version: 160701-CR / Primary author: Lizzie Prader / Last reviewed: TBD 

This document describes how to program Kinoma Element’s hardware pins, and serves as a handy reference for the pin types supported by Kinoma Element.
-->

#Programming with Hardware Pins for Kinoma Element

This document describes how to program Kinoma Element's hardware pins, and serves as a handy reference to the pin types supported by Kinoma Element.

##About the Pins

Kinoma Element has 16 hardware pins. These pins implement Digital Input/Output, Analog Input, Pulse Width Modulation (PWM), Serial Input/Output, and I<sup>2</sup>C. 

The pins are divided into left and right groups of eight pins each. These pins are user-configurable, using either the Front Pins app in Kinoma Code or the Pins module in an application. All pins operate at 3.3 volts.

<!--From CR to Kouis: In Figure 1, please make font of top two lines smaller (more like size of other callouts). Also, here and in the separate Element Pin Map PDF, change "CLock" (twice) to "Clock"-->

**Figure 1.** Kinoma Element Pins  

![Pin map](img/pin-map.png)

##Introducing BLL Modules

All hardware pin programming is done in JavaScript. The code for each hardware module (sensor, LED, button, motor, and so on) is contained in a JavaScript module called a *BLL.* BLLs communicate directly with hardware pins. The application communicates with the BLL using the Pins module.

> **Note:** "BLL" stands for Blinking Light Library, but a BLL is not limited to blinking an LED; a BLL can be used to interact with all kinds of hardware modules.

<!--From CR re last sentence of the following: One of the `bll_x.js` files--for the as yet unsupported SPI--appears not to have been updated for Kinoma Element; why not leave out the part below about the file names (as in pins.doc, and since it's pretty obvious)? (If you prefer keeping it in, I'll change pins.doc to include it as well.)-->

Developers either implement their own BLL to support the hardware modules they have connected to Kinoma Element or use an existing BLL implementation. Sample BLL implementations for common hardware modules are available in our collection of [sample apps](https://github.com/Kinoma/KPR-examples) on GitHub. In addition, the Pins module includes built-in BLLs for each hardware protocol, and you can also find their [source code](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc/extensions/pins) on GitHub; they are the files with names of the form `bll_x.js`, where `x` is the name of the relevant hardware protocol (for example, `bll_Analog.js` and `bll_PWM.js`).

## Application Programming with Pins

Applications first configure the BLLs they will use, and then issue single or repeating commands to the BLL. The Pins module is used to send configuration and commands from the application to BLLs. (See also the document [*Using the Pins Module to Interact with Sensors on Kinoma Element*](../element-pins-module/).)

The call to `Pins.configure` lists the BLLs the application uses together with the pins that each BLL uses to communicate to its hardware module.

The following code configures the hardware pins to work with BLLs named `buttonBLL` and `ledBLL`. The BLL code is stored in files named `buttonBLL.js` and `ledBLL.js`. The second argument of `Pins.configure` is a callback function that is invoked whether or not the configuration is successful.

```
Pins.configure({
	greenButton: {
		require: "buttonBLL",
		pins: {
			button: { pin: 1 },
			power: { pin: 2 },
			ground: { pin: 3 },
		}
	},
	redLED: {
		require: "ledBLL",
		pins: {
			led: { pin: 8 },
			ground: { pin: 9 },
		}
	}
}, success => {
	if (success)
		trace("Configured pins.\n");
	else
		trace("Failed to configure pins.\n");
});
```

Applications assign a unique name to each hardware module in the configuration. In the preceding example, the button BLL is named `greenButton` and the LED BLL is named `redLED`. The application uses these names to send commands to the corresponding hardware module.

<!--From CR: I changed "Hardware Pins Service" to "application" below; OK? (You removed the earlier intro to Hardware Pins Service that appears in the Pins version of this doc, and you deleted all other references to it--except the one that used to appear below.)-->

The `require` and `pins` properties are defined by the application. The value of the `require` property is the name of the BLL file corresponding to the hardware module. The names of the properties inside the `pins` objects (in this example, `led`, `button`, `power`, and `ground`) are defined by each individual BLL.

While not evident from the application code, the button BLL uses a Digital Input pin, and the LED BLL uses a Digital Output pin. By convention, the application code specifies the pin numbering and the BLL defines the type of pins (Digital Input, Digital Output, PWM, Serial, and so on) that are used. The next section covers how to specify the pin type.

The LED BLL is bound to pins 8 and 9 and the button BLL to pins 1 through 3, as defined in the `pins` object.

Once the pins are configured, an application can invoke BLL functions using the `Pins.invoke` function.

```
Pins.invoke("/redLED/write", 1);  // Turn LED on

Pins.invoke("/redLED/write", 0);  // Turn LED off
```

Applications retrieve values from BLLs in a similar way: calling `Pins.invoke` with the name of the target hardware module and the name of the command in the path. Also passed in is a callback function, which will be called with the value returned.

```
Pins.invoke("/greenButton/get", value => {
	trace("Button value: " + value + "\n");
});
```
	
> **Note:** The format of the value returned from the BLL is determined by the author of the BLL. In the preceding example, the type of the returned value is `Number`, but it may be any valid JavaScript object. 

Applications establish repeated polling of a hardware module at a specified interval using the `Pins.repeat` function. The following example polls the button at 50-millisecond intervals.

```
Pins.repeat("/greenButton/get", 50, value => {
	trace("Button value: " + value + "\n");
});
```

If you would like to take a closer look at the Pins module for Kinoma Element, you can find its [source code](https://github.com/Kinoma/kinomajs/tree/master/xs6/sources/mc/extensions/pins)on GitHub.

## BLL Programming with Pins

As mentioned earlier, a BLL is a JavaScript module that communicates directly with hardware pins. The BLL is configured by the application, which communicates with the BLL exclusively using the Pins module.

As illustrated in the simple example below, all BLLs must export the following:

* A `pins` object that defines the types of pins it uses 

* A `configure` function that initializes each of the objects by calling the object's `init` function

* A `close` function that is called automatically when the host application exits and that typically closes the objects used to communicate with pins

You can define and export additional functions required for working with the module to interact with the sensors.

Here are the `pins` exports corresponding to `ledBLL` and `buttonBLL` (respectively) as introduced earlier:

```
exports.pins = {
	led: { type: "Digital", direction: "output" },
	ground: { type: "Ground" }
};

exports.pins = {
	button: { type: "Digital", direction: "input" }
	power: { type: "Power" },
	ground: { type: "Ground" }
};
```

When `Pins.configure` is called, the configuration data provided by the application (highlighted in <span class="app-defined">`green`</span> below) is merged with the properties of the `pins` object exported by the BLL (highlighted in <span class="bll-defined">`blue`</span>), to arrive at the following complete configurations:

<pre><code>redLED_configuration = {
	<span class="bll-defined">led</span>: { <span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 8, <span class="bll-defined">direction</span>: "output" },
	<span class="bll-defined">ground</span>: { <span class="bll-defined">type</span>: "Ground", <span class="app-defined">pin</span>: 9 }
}

greenButton_configuration = {
	<span class="bll-defined">button</span>: { <span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 1, <span class="bll-defined">direction</span>: "input" },
	<span class="bll-defined">power</span>: { <span class="bll-defined">type</span>: "Power", <span class="app-defined">pin</span>: 2 },
	<span class="bll-defined">ground</span>: { <span class="bll-defined">type</span>: "Ground", <span class="app-defined">pin</span>: 3 }
}</code></pre>
	
<!--From CR: Update the following (and maybe also Hardware Pins Reference later) to reflect the addition of power and ground pins in the example?-->

The appropriate BLLs are then loaded using the `require` function, and JavaScript objects to communicate with the hardware pins are instantiated. In this example, two Digital pin objects are instantiated: one configured as an input and the other as an output. The input is bound to pin 1 and the output to pin 8. These JavaScript objects are assigned to the module using the property names given by the BLL (`led` and `button` in this example).

Once the objects are instantiated and bound, the `configure` function of each BLL is called. The `configure` function must initialize each of the objects by calling the object's `init` function, and it can do any additional work required by the BLL. Note that Power and Ground pins do not need to be initialized.

```
exports.configure = function() {
	this.led.init();
}

exports.configure = function() {
	this.button.init();
}
```
	
The BLL also defines a `close` function, which is called automatically when the host application exits. The `close` function typically closes the objects used to communicate with pins, as follows:

```
exports.close = function() {
	this.led.close();
}

exports.close = function() {
	this.button.close();
}
```
	
The BLL author may define additional functions required for working with the module. Here are sample `read` and `write` functions for the `led` and `button` objects:

```
exports.read = function() {
	return this.button.read();
}

exports.write = function(value) {
	this.led.write(value ? 1 : 0);
}
```

## Hardware Pins Reference 

<!--From CR: Each subsection heading is meant to show the exact pin type, hence my changing a couple of them back to that. (I've tried to clarify this below.)-->

This section describes the pins data format that an application uses to configure a BLL. For each of the pin types that are programmatically supported by Kinoma Element--`Digital`, `Analog`, `PWM`, `Serial`, and `I2C`--the following reference details are provided:

* **Pins Object** -- The full pin configuration data, specified in the `pins` object with properties that define the type of pin and the pin number used. Data in <span class="bll-defined">`blue`</span> is defined by the BLL; data in <span class="app-defined">`green`</span>, by the application. 

<!--From CR re the following: FYI, now using "Functions" and "Values" as in Element ref doc.-->

* **Functions**, **Values** -- The functions and value properties supported by the pin type.

The object for each pin type has an `init` function that reserves any hardware resources required by the object, and a `close` function to release the resources used by the object. The object will not operate properly until the `init` function is called.

>**Note:** This reference section refers to chunks, but use of chunks has been deprecated.


###Digital

####Pins Object

<pre><code>{<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 2, <span class="bll-defined">direction</span>: "input"};
{<span class="bll-defined">type</span>: "Digital", <span class="app-defined">pin</span>: 3, <span class="bll-defined">direction</span>: "output"};</code></pre>

####Functions

#####`digital.read()`
Returns 0 or 1, or `undefined` if the read fails. This function is used to retrieve either the value of an input pin or the value being sent by an output pin.

#####`digital.write(value)`
`value` must be 0 or 1. This property can be read or written to change the direction.

####Values

<!--From CR: The current convention in Kinoma docs is not to put string values in quote marks in body text. The context should make it clear that they're strings, hence my adding that clarification below and elsewhere.-->

#####`digital.direction`
Value is the string `input` or `output`.


###Analog

####Pins Object

<pre><code>{<span class="bll-defined">type</span>: "Analog", <span class="app-defined">pin</span>: 3};</code></pre>

Analog is available only on pins 1 through 8.

####Functions

#####`analog.read()`
Returns a floating-point value from 0 to 1.0


###PWM

####Pins Object

<pre><code>{<span class="bll-defined">type</span>: "PWM", <span class="app-defined">pin</span>: 9};</code></pre>

PWM is available only on pins 9 through 16. All PWMs can be active simultaneously.

####Functions

#####`pwm.write(value)`
Sets the duty cycle (percentage of time spent "high") to `value` (a floating-point value from 0 to 1.0). The frequency is 50 Hz (period 20 ms).

#####`pwm.write(value, period)`

Sets the PWM output pulse width to `value` and the period (cycle duration, or 1/frequency) to `period`, both specified in milliseconds. Pulse width and period can also be specified as an array, as in `pwm.write([width, period])`. The minimum pulse width is .00002 ms (20 ns). The maximum frequency is 5 kHz--that is, a period of .2 ms, or 200 µs. Very long periods (greater than one hour) are supported.

#####Examples

<!--From CR: The above says the max frequency is .2 ms, so how can it be set to 5 ms in the last two examples below?-->

```
this.pwm.write(0.5);       // 50% duty cycle
this.pwm.write(0.5, 5);    // 0.5 ms pulse width, 5 ms period (10% @ 200 Hz)
this.pwm.write([0.5, 5]);  // Same as preceding example
```

###Serial

####Pins Object

<pre><code>{<span class="bll-defined">type</span>: "Serial", <span class="app-defined">rx</span>: 1, <span class="app-defined">tx</span>: 2, <span class="bll-defined">baud</span>: 38400};</code></pre>

Serial RX and TX pins must be used in adjacent pairs; the options are 1-2, 5-6, and 12-11. 

If the serial device is read-only or write-only, the unused pin property, `rx` or `tx`, may be excluded from the pin configuration data.

####Functions

#####`serial.read(format)`
Reads data from the serial input. Pass one of the following strings for the `format` parameter:

- `String` to return a string. The data read into a string must be valid UTF-8 data, otherwise an exception is thrown.

- `ArrayBuffer` to return an array buffer.

<!--

- `Chunk` to return the data in a chunk.

- `String` to return a string. The data read into a string must be valid UTF-8 data, otherwise an exception is thrown.

- `Array` to return an array of integer character codes from 0 to 255. 

- `charCode` to return a single character code as an integer.

All formats except `charCode` return all data that is immediately available on the input.
-->

#####`serial.read(format, maximumBytes)`
Same as `serial.read(format)` but reads no more than `maximumBytes`. This call does not block, so only immediately available bytes are returned. <!--This function is not supported for the `charCode` format, which returns either 1 or 0 bytes.-->

#####`serial.read(format, maximumBytes, msToWait)`
Same as `serial.read(format, maximumBytes)` but waits up to `msToWait` milliseconds for `maximumBytes` to arrive

#####`serial.write(value)`
Writes the specified value to the output. `value` can be a number, string, array, or array buffer.

<!--
#####`serial.write(value...)`
Writes the specified value(s) to the output. Each value can be a chunk, a string, an array of integer character codes from 0 to 255, or a single integer character code. 

#####Example
This example writes a string followed by a carriage return-line feed and `null`.

```
var crlf = new Chunk(2);
crlf[0] = 13;
crlf[1] = 10;
this.serial.write("Hello, world.", [crlf, 0]);
```
-->

###I2C

####Pins Object
<pre><code>{<span class="bll-defined">type</span>: "I2C", <span class="app-defined">sda</span>: 13, <span class="app-defined">clock</span>: 14, <span class="bll-defined">address</span>: 0x36};</code></pre>

I<sup>2</sup>C pins must be used in adjacent pairs; the options are 13-14 and 15-16.

<!--From CR: Should the second sentence below also be added here in the Create version of this doc?-->

The `address` property is the I<sup>2</sup>C slave address. The data sheet for some I<sup>2</sup>C devices specifies different slave addresses for reading and writing, differing only in the least significant bit; if this is the case, use the most significant seven bits for the slave address here.

####Functions

<!--From CR: Why comment out the ("Linux: xx)" part at the end of each of the following, as opposed to either deleting it (or leaving it in, as in the Create version of this doc)?-->

#####`i2c.readByte()`
Reads one byte

<!--(Linux: read)-->

#####`i2c.readBlock(count, format)`
Reads the number of bytes specified by `count`. Pass a string for the `format` parameter: `Chunk` to return the data in a chunk, or `Array` to return an array of integer character codes from 0 to 255.

<!--(Linux: read)-->

#####`i2c.readByteDataSMB(register)`
Reads one byte from the specified register

<!--(Linux: i2c_smbus_read_byte_data)-->

#####`i2c.readWordDataSMB(register)`
Reads two bytes from the specified register

<!--(Linux: i2c_smbus_read_word_data)-->

#####`i2c.readBlockDataSMB(register, count, format)`
Reads `count` bytes starting at the specified register. Pass a string for the `format` parameter: `Chunk` to return the data in a chunk, or `Array` to return an array of integer character codes from 0 to 255.

<!--(Linux: i2c_smbus_read_i2c_block_data)-->

#####`i2c.writeByte(value)`
Writes one byte.

<!--(Linux: write)-->

#####`i2c.writeBlock(value...)`
Writes the specified value(s), which are treated in the same way as by `serial.write`.

<!--(Linux: write)-->

#####`i2c.writeByteDataSMB(register, value)`
Writes one byte to the specified register

<!--(Linux: i2c_smbus_write_byte_data)-->

#####`i2c.writeWordDataSMB(register, value)`
Writes two bytes to the specified register

<!--(Linux: i2c_smbus_write_word_data)-->

#####`i2c.writeBlockDataSMB(register, value...)`
Writes up to 32 bytes starting at the specified register. The value(s) are treated in the same way as `serial.write`.

<!--(Linux: i2c_smbus_write_i2c_block_data)-->

<!-- untested: -->

#####`i2c.writeQuickSMB(value)`
Sends the low bit of `value` using the I<sup>2</sup>C `write_quick` command

<!--(Linux: i2c_smbus_write_quick)-->

#####`i2c.processCallSMB(register, value)`
Writes two bytes to specified register; after the write completes, reads two bytes and returns the resulting data word.
