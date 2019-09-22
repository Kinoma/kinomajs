# Building Your Own BLLs

As discussed in the document [*Using the Pins Module to Interact with Sensors on Kinoma Create*](../../libraries/Pins/create-pins-module/create-pins-module.md), a BLL in KinomaJS is a module that defines the configuration and available methods of a piece of hardware, and there are some BLLs built into the Pins module. The built-in BLLs provide basic functions for all pin types--Digital, Analog, PWM, I<sup>2</sup>C, Serial, Power, and Ground. They enable you to easily communicate with hardware, and they work well for simple scenarios. If you simply want to turn a light on, for example, you can use the built-in Digital BLL rather than create your own.

```
Pins.configure({
	ground: {pin: 51, type: "Ground"},
	led: {pin: 52, type: "Digital", direction: "output"},
	power: {pin: 59, type: "Power", voltage: 3.3 },
}, function(success) {
	if (success) Pins.invoke("/led/write", 1);
});
```
	
However, as the functionality of the hardware gets more complex, working with the built-in BLLs can get tedious. Creating a custom BLL is a much cleaner way to incorporate additional hardware details and logic such as register numbers, call order rules, or conversions.

This tutorial will lead you step-by-step through example cases where you would want to write your own sensor module. In addition, a [collection of sample BLLs](https://github.com/lprader/SampleBLLs) for specific sensors are available, along with many [sample projects](https://github.com/Kinoma/KPR-examples).


## From Built-In to Custom BLL

In this example, of an analog distance sensor, we need to convert the raw value to the measurement value after reading the analog hardware value. 

Our main program might start out using the built-in BLL for convenience, as shown here:

```
var supplyVoltage = 3.3;
var voltsPerInch = 0.009766;
Pins.configure({
    gnd: {pin: 51, type: "Ground"},
    pwr: {pin: 53, type: "Power", voltage: supplyVoltage },
    dxSensor: {pin: 52, type: "Analog"},  // Uses built-in analog BLL
}, success => {
    if (success) {
    	Pins.repeat("/dxSensor/read", 100, result => {
    		let dx = result * supplyVoltage / voltsPerInch
    		trace("Distance: " + dx.toFixed(2) + "\n");
    	});
    }
});
```

Eventually, however, burdening the main application with hardware-specific functions becomes cumbersome. At this point we have outgrown the built-in BLL.

### Creating a Separate Module

It is time to break the code out into a module. The next step, then, is to create a module--named, for example, `dxSensor.js`. This will be a custom BLL. 

The main program needs to change as follows to require the module and specify the required pins in the call to `Pins.configure`:

```
var supplyVoltage = 3.3;

Pins.configure({
    gnd: {pin: 51, type: "Ground"},  // pwr and gnd are unchanged
    pwr: {pin: 53, type: "Power", voltage: supplyVoltage },
    dxSensor: {
    	require: "dxSensor",  // Omit the .js extension
    	pins: {
    		range: { pin: 52, supplyVoltage: supplyVoltage },
    	}
    }
}, success => {
...
```

### Objects to Export

The module `dxSensor.js` needs the following few basic exports, which are common to BLLs in general.

####`pins`

Properties in the exported `pins` object are merged with those of the corresponding `pins` object from `Pins.configure`.

```
exports.pins = {
	range: {type: "Analog", supplyVoltage: 5.0, voltsPerInch: 0.009766}
};
```

No matter when or where this BLL is used, we want the pin named `range` to have a `type` of `"Analog"`, so it makes sense to simply specify it here. This is also true of the voltage multiplier. The program then only has to specify which pin to configure. Where the properties are identical between `exports.pins` and `Pins.configure`, those specified in `Pins.configure` take precedence.  In this case, the main program is overriding the default `supplyVoltage` value in the BLL.

#### `configure`

When `Pins.configure` is used, the `configure` function will be called.  

```
exports.configure = function( configuration ) {
	this.voltsPerInch = configuration.pins.range.voltsPerInch;
	this.supplyVoltage = configuration.pins.range.supplyVoltage;
	this.range.init();
}
```

The `pins` object is initialized by calling its `init` method. The `this` keyword is used to specify the instance of the pin being defined in the pin configuration.

####`close`

When the application quits or the `pins` object is closed, the `close` function is called. At a minimum it should look like this:

```
exports.close = function() {
	this.range.close();
}
```
####`read`

The built-in analog BLL exports a `read` function that is used in `Pins.repeat` in the program. This BLL has one too, but we have moved our calculation into it.

```
exports.read = function() {
    var measured = this.range.read();
    var range = (measured * this.supplyVoltage) / this.voltsPerInch;
    return range;
}
```

This enables us to eliminate all the conversion from our program (and any other program that uses this BLL) and use the result directly. 

### Tying Up

Here is the new `Pins.repeat` from the main program:

```
Pins.repeat("/dxSensor/read", 100, result => {
	trace("Distance: " + result.toFixed(2) + "\n");
});
```

This example is relatively trivial; custom BLLs become far more useful when you are configuring more complex sensors that use protocols like I<sup>2</sup>C or serial.

>**Note:** Custom BLLs actually run on the hardware pins thread, which is separate from the main thread. This can benefit the overall performance of your project when used correctly, but it is recommended that you do not use expensive methods inside a BLL if timing matters. Calling `trace` inside your module repeatedly, for example, is not recommended, and will slow your hardware performance by a few milliseconds every iteration. 


## I<sup>2</sup>C Color Sensor

The TCS34725 I<sup>2</sup>C color sensor reads and registers reflected light in RGB form. From Adafruit's [data sheet](https://www.adafruit.com/datasheets/TCS34725.pdf) and [example code](https://github.com/adafruit/Adafruit_TCS34725) for this sensor, we can glean the information necessary to write our module for it. 

From the data sheet, we can see that the I<sup>2</sup>C Vbus address we want is `0x29`, so that is what we set in our configuration. 

Two important settings described in the data sheet are gain and integration time, both of which affect the resolution and sensitivity of the RGB color reading. In our module, we have created methods to set these.

The `writeByteDataSMB` method is used to write the specified values. From the data sheet, we can see that every command is seven bits, preceded by one high command bit. Thus, `COMMAND` is `0x80` or `1000 0000`. Additionally, the allowable gains are 1X, 4X, 16X, and 60X. The allowable integration time range is 2.4-614.4 ms. The calculation for integration time follows from the spec.

```
exports.setGain = function( gain ) {
    var value;
    switch ( gain ) {
        case 1: value = 0; break;
        case 4: value = 1; break;
        case 16: value = 2; break;
        case 60: value = 3; break;
        default: throw "Invalid gain " + gain;
    }
	this.rgb.writeByteDataSMB(COMMAND | CONTROL, value);
}
exports.setIntegrationTime = function( time ) {
    if ( ( time < 2.4 ) || ( time > 614.4 ) )
        throw "Invalid integrationIime " + time;

    var value = Math.round( 256 - ( time / 2.4 ) );
	this.rgb.writeByteDataSMB( COMMAND | ATIME, value );
    return value;
}
```

The default configuration accounts for the gain/integration time as well as the built-in LED. This configuration is arbitrary, and the settable values can be reconfigured from the main file. 

```
exports.pins = {
    rgb: { type: "I2C", address: 0x29, gain: 16, integrationTime: 153.6 },
    led: { type: "Digital", direction: "output", value: 1 }
};
```

In the `configure` function, we have added an ID check to make sure that the IC is the right type. The data sheet specifies a read-only ID associated with this exact sensor type. When we send the command to the power-on register, a slight delay is observed before the RGBC service is powered on; this is to ensure the delivery of the command to the RGBC service.


```
exports.configure = function(configuration) {
	this.rgb.init();
	var id = this.rgb.readByteDataSMB( COMMAND | ID );
	if (0x44 != id)
		throw "colorSensor - cannot find device - got ID " + id;
	this.rgb.writeByteDataSMB( COMMAND | ENABLE, ENABLE_PON );
	sensorUtils.mdelay( 3 );
	this.rgb.writeByteDataSMB( COMMAND | ENABLE, ENABLE_PON | ENABLE_AEN );

    this.setGain( configuration.pins.rgb.gain );
    this.setIntegrationTime( configuration.pins.rgb.integrationTime );

    if ( "led" in this ) {
        this.led.init();
        this.setLED( configuration.pins.led.value );
    }
}
```

The `getColor` function then returns an interpreted value from 0 to 255 for R/G/B. Here we assume perfect balance between the RGB color channels and the clear channel, which is just a simplification (see this [data sheet](http://ams.com/eng/Products/Light-Sensors/Color-Sensor/TCS34725)).

```
exports.getColor = function() {
	var r = this.rgb.readWordDataSMB( COMMAND | RDATAL );
	var g = this.rgb.readWordDataSMB( COMMAND | GDATAL );
	var b = this.rgb.readWordDataSMB( COMMAND | BDATAL );
	var c = this.rgb.readWordDataSMB( COMMAND | CDATAL );

	return {
		raw: { r: r, g: g, b: b, c: c },
		r: Math.round( ( r / c ) * 255 ),
		g: Math.round( ( g / c ) * 255 ),
		b: Math.round( ( b / c ) * 255 )
	};
}
```

<!--From CR: This tutorial is currently Create-specific yet mentions Kinoma Element below; this anomaly is being overlooked because the project will soon no longer be categorized as a KinomaJS tutorial. -->

This completes our module for using the TCS34725 I<sup>2</sup>C color sensor. You can download a sample project for Kinoma Create [here](https://github.com/Kinoma/KPR-examples/tree/master/i2c-color-sensor) and a sample project for Kinoma Element [here](https://github.com/Kinoma/KPR-examples/tree/master/element-colors).