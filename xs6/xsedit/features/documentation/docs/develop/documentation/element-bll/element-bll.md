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
#BLL Programming for Kinoma Element
Kinoma Element has 16 hardware pins. These pins implement Digital Input/Output, Analog Input, Pulse Width Modulation (PWM), I2C, and Serial Input/Output. All pins operate at 3.3 volts.

![Pin map](img/pin-map.png)

##BLLs
A BLL is a JavaScript module that communicates directly with hardware pins. All BLLs should export three objects:

1. A pins object which defines the type of pins it uses
2. A `configure` function which initializes each of the objects by calling their `init` function
2. A `close` function which is called automatically when the host application exits and is typically used to close the objects used to communicate with pins

In addition, you can define and export additional functions required to work with the module to interact with the sensors.


###Simple Example
Say you have the following code in a BLL called led.js:

```
exports.pins = {
	led: {type: "Digital", direction: "output"}
};
	
exports.configure = function() {
	this.led.init();
	this.led.write(1);
}
	
exports.close = function() {
	this.led.write(0);
	this.led.close();
}
```

To use this BLL in your application, you use the [pins module](./pins-module-element.md) and call `Pins.configure`. This will call the exported `configure` function in the BLL, which calls its `init` function and turns the light on. When the application is closed, the `close` function will be called, turning the light off.
	
If you wanted to toggle the light on and off from your application file, you could add a `write` function:

```
exports.write = function(value) {
	this.led.write(value ? 1 : 0);
}
```

You would then call `Pins.invoke` with an argument of 0 or 1 from your application file.

The remaining sections of this document cover the pins object and other available functions/properties for all the supported hardware protocols.  The other functions listed can be used in custom functions once a pin's `init` function has been called. Some examples are provided for each protocol.

***

##Digital
###Pins object
`{type: "Digital", direction: "input"}`

or

`{type: "Digital", direction: "output"}`

For example, you might have

```
exports.pins = {
	button: {type: "Digital", direction: "input"}
};	
```	
	
or 

```
exports.pins = {
	led: {type: "Digital", direction: "output"}
};
```
	
###Other functions and properties
**digital.read()** — Returns 0 or 1, or undefined if the read fails. This is used both to retrieve the value of an input pin, or the value being output on an output pin.

**digital.write(value)** — Value must be either 0 or 1

**digital.direction** — Value is either “input” or “output”. Note direction is a property, not a function.

In the simple example provided earlier, the `digital.write` function was used in the custom, exported `write` function:

```
exports.write = function(value) {
	this.led.write(value ? 1 : 0);
}
```

***

##Analog

###Pins object

`{ type: "Analog" }`
> Note that Analog only works on pins 1-8.

For example, you might have

```
exports.pins = {
	potentiometer: { type: "Analog" }
};
```

###Other functions
**analog.read()** — Returns a floating point value from 0 to 1.0

Like the digital example, you might define a `read` function and export it so it can be called from your application:

```
exports.read = function() {
	return this.potentiometer.read();
}
```

***
##PWM

###Pins object
`{type: "PWM"};`
> Note that PWM only works on pins 9-16.

###Other functions and properties
**pwm.write(value)** — Sets the PWM frequency to the floating point value provided (between 0 and 1.0). If the parameter is an array, the first element is the duty cycle and the second element is the period. If tehre is only one element in the array, the period will be twice the duty cycle.

```
this.pwm.write(0.5);
this.pwm.write(0.5, 5);
```

***

##Serial

###Pins object
 `{type: "Serial", baud: 9600};`

> Note that RX/TX pins must be used in adjacent pairs. The three options are 1/2, 5/6, and 12/11. In addition if the serial device is read-only or write-only, the corresponding pin, rx or tx, may be excluded from the pin configuration data.

For example, you might have:

```
exports.pins = {
	display: {type: "Serial", baud: 9600}
};
```

###Other functions and properties
**serial.read(type)** — Reads data from the serial input. Pass "Chunk" for the type parameter to return the data in a Chunk, "String" to return a string, "Array" to return an array of integer character codes between 0 and 255, and “charCode” to return a single character code as an integer. Note that the data returned in a String must be valid UTF-8 data; if the data read is not valid UTF-8 an exception is thrown. All types of read, except charCode return all data that is immediately available on the input.

**serial.read(type, maximumBytes)** — Same as serial.read(type) but no more than maximumBytes are read. This call does not block, so only immediately available bytes are returned. This is not supported for type charCode which returns either 1 or 0 bytes.

**serial.read(type, maximumBytes, msToWait)** — Same as serial.read(type, maximumBytes) but waits up to msToWait milliseconds for maximumBytes to arrive.

**serial.write(value, …)** — Writes all arguments to the output. Values can be of type Chunk, String, Array of character codes, or numbers (character codes from 0 to 255). The following writes a message, followed by a carriage return, line feed, and null.

```
var crlf = new Chunk(2);
crlf[0] = 13;
crlf[1] = 10;
serial.write("Hello, world.", [crlf, 0]);
```

***

##I²C

###Pins object
`{type: "I2C", address: 0x36};`
> Note: The address property is the I²C slave address.
> Also note that SDA/SCL pins must be used in adjacent pairs. The two options are 13/14 and 15/16.

###Other functions and properties

**i2c.readByte()** — Reads one byte (Linux: read)

**i2c.readBlock(count, format)** - reads the number of bytes specified by count. Pass “Chunk” for the type parameter to return the data in a Chunk, or “Array” to return an array of integer character codes between 0 and 255. (Linux: read)

**i2c.readByteDataSMB(register)** — Reads 1 byte from specified register (Linux: i2c\_smbus\_read\_byte\_data)

**i2c.readWordDataSMB(register)** — Reads 2 bytes from specified register (Linux: i2c\_smbus\_read\_word\_data)

**i2c.readBlockDataSMB(register, count, type)** — Reads count bytes starting at the specified register. Pass "Chunk" for the type parameter to return the data in a Chunk, or "Array" to return an array of integer character codes between 0 and 255. (Linux: i2c\_smbus\_read\_i2c\_block\_data)

**i2c.writeByte(value)** — Writes one byte (Linux: write)

**i2c.writeBlock(value...)** - writes the values provided. The value(s) are treated in the same way as serial.write(). (Linux: write)

**i2c.writeByteDataSMB(register, value)** — Writes 1 byte to specified register (Linux: i2c\_smbus\_write\_byte\_data)

**i2c.writeWordDataSMB(register, value)** — Writes 2 bytes to specified register (Linux: i2c\_smbus\_write\_word\_data)

**i2c.writeBlockDataSMB(register, value...)** — Writes up to 32 bytes starting at the specified register. The value(s) are treated in the same way as serial.write(). (Linux: i2c\_smbus\_write\_i2c\_block\_data)

<!-- untested -->
**i2c.writeQuickSMB(value)** — Sends the low bit of value using the I2C write_quick command (Linux: i2c\_smbus\_write\_quick)

**i2c.processCallSMB(register, value)** — Writes 2 bytes to specified register. After the write completes, reads 2 bytes, and returns the resulting data word.

