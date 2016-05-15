<package>
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
<program><![CDATA[

const disconnected = 0;
const power3_3V = 1;
const ground = 2;
const analogIn = 3;
const digitalIn = 4;
const digitalOut = 5;
const i2cClock = 6;
const i2cData = 7;
const serialRx = 8;
const serialTx = 9;
const pwm = 10;
const power5V = 11;

class ElementHost @ "KPR_elementHost" {
	constructor(dictionary) @ "KPR_ElementHost";
	debugger() @ "KPR_elementHost_debugger";
	launch() @ "KPR_elementHost_launch";
	purge() @ "KPR_elementHost_purge";
	quit() @ "KPR_elementHost_quit";
	wake() @ "KPR_elementHost_wake";
};

class PinsBase {
	constructor(it) {
		this.pin = ("pin" in it) ? it.pin : -1;
	}
	close() {
	}
	init() {
	}
}

var PINS = {
	behaviors: {
	},
	constructors: {
		A2D: class extends PinsBase {
		},
		Analog: class extends PinsBase {
			read() {
				return 0;
			}
		},
		Digital: class extends PinsBase {
			constructor(it) {
				super(it);
				this.direction = it.direction;
			}
			getDirection() {
				return this.direction;
			}
			read() {
				return 1;
			}
			write(value) {
			}
		},
		Ground: class extends PinsBase {
		},
		I2C: class extends PinsBase {
			constructor(it) {
				super(it);
				this.address = it.address;
				if ("clock" in it)
					this.clock = it.clock;
				if ("sda" in it)
					this.sda = it.sda;
				if ("bus" in it)
					this.bus = it.bus;
			}
		},
		Power: class extends PinsBase {
			constructor(it) {
				super(it);
				this.voltage = ("voltage" in it) ? it.voltage : 3.3;
			}
		},
		PWM: class extends PinsBase {
		},
		Serial: class extends PinsBase {
			constructor(it) {
				super(it);
				this.rx = it.rx;
				this.tx = it.tx;
				this.baud = it.baud;
			}
		},
		SPI: class extends PinsBase {
			constructor(it) {
				super(it);
				this.chipSelect = it.chipSelect;
				this.mode = it.mode;
				this.speed = it.speed;
				this.bitsPerWord = it.bitsPerWord;
			}
		},
	},

	close() {
		var behaviors = this.behaviors;
		for (m in behaviors)
			behaviors[m].close();
		this.behaviors = {};
		shell.delegate("onPinsClose");
	},
	configure(configurations) {
		try {
			var constructors = this.constructors;
			var behaviors = {};
			for (var i in configurations) {
				var configuration = configurations[i];
				var defaults = undefined;
				if (!("require" in configuration)) {
					if (!("type" in configuration))
						throw new Error("!!!!require property missing in configuration " + i + ": " + JSON.stringify(configuration));
					var type = configuration.type;
					var config = {};
					config.require = type;
					config.pins = {};
					config.pins[type.toLowerCase()] = configurations[i];
					configurations[i] = config;
					configuration = config;
				}
				var prototype = require(configuration.require);
				var behavior = Object.create(prototype);
				var defaults;
				behavior.id = i;
				if ("pins" in behavior)
					defaults = behavior.pins;
				var pins = configuration.pins;
				for (var j in pins) {
					var pin = pins[j];
					if (defaults && (j in defaults))
						this.merge(defaults[j], pin);
					var type = pin.type;
					if (!type)
						throw new Error("Pin " + j + " missing type: " + JSON.stringify(pin));
					if (!(type in constructors))
						throw new Error("Unsupported pin type on pin " + j + ": " + JSON.stringify(pin));
					behavior[j] = new (constructors[type])(pin);
				}
				if ("configure" in behavior)
					behavior.configure(configuration, i);	// passing i for debugging / simulators
				behaviors[i] = behavior;
			}
			this.behaviors = behaviors;
			shell.delegate("onPinsConfigure", configurations);
			this.configurations = configurations;
			this.pinmux = null;
			return configurations;
		}
		catch(e) {
		}
	},
	invoke(path, object) {
		try {
			if (path == "getPinMux") {
				var result = this.pinmux ? this.pinmux : this.configurationToMux(this.configurations);;
				shell.delegate("onPinsInvoke", path, object, result);
				return result;
			}
			if (path == "metadata") {
				var result = {};
				for (var i in this.behaviors) {
					var behavior = this.behaviors[i];
					result[i] = behavior.metadata;
				}
				shell.delegate("onPinsInvoke", path, object, result);
				return result;
			}
			if (path == "setPinMux") {
				this.pinmux = object;
				return;
			}
			if (path == "configuration") {
				return this.configurations;
			}
			var behaviors = this.behaviors;
			var result;
			var a = path.split("/");
			if (a.length == 2) {
				var f = a[0];
				var m = a[1];
				if (f == "metadata") {
					if (m in behaviors) {
						var behavior = behaviors[m];
						if (f in behavior) {
							result = behavior[f];
							shell.delegate("onPinsInvoke", path, object, result);
						}
						else
							shell.delegate("onPinsError", path, object, "invalid metadata");
					}
					else
						shell.delegate("onPinsError", path, object, "invalid module");
				}
				else
					shell.delegate("onPinsError", path, object, "invalid path");
			}
			else if (a.length > 2) {
				var m = a[1];
				var f = a[2];
				if (m in behaviors) {
					var behavior = behaviors[m];
					if (f in behavior) {
						result = behavior[f](object);
						shell.delegate("onPinsInvoke", path, object, result);
					}
					else
						shell.delegate("onPinsError", path, object, "invalid function");
				}
				else
					shell.delegate("onPinsError", path, object, "invalid module");
			}
			else
				shell.delegate("onPinsError", path, object, "invalid path");
			return result;
		}
		catch(e) {
		}
	},
	merge(defaultPin, configurationPin) {
		for (var i in defaultPin) {
			if (!(i in configurationPin))
				configurationPin[i] = defaultPin[i];
		}
	},
	repeat(path, object) {
		try {
			var result;
			var a = path.split("/");
			if (a.length > 2) {
				var m = a[1];
				var f = a[2];
				var behaviors = this.behaviors;
				if (m in behaviors) {
					var behavior = behaviors[m];
					if (f in behavior) {
						result = behavior[f](object);
						if (result !== undefined)
							shell.delegate("onPinsRepeat", path, object, result);
					}
					else
						shell.delegate("onPinsError", path, object, "invalid function");
				}
				else
					shell.delegate("onPinsError", path, object, "invalid module");
			}
			else
				shell.delegate("onPinsError", path, object, "invalid path");
			return result;
		}
		catch(e) {
		}
	},
	setPin(mux, index, type) {
		if (index < 9)
			mux.leftPins[index - 1] = type;
		else
			mux.rightPins[index - 9] = type;
	},
	configurationToMux(configurations) {
		var mux = {
			leftPins: [ 0,0,0,0,0,0,0,0 ],
			rightPins: [ 0,0,0,0,0,0,0,0 ],
			leftVoltage: 3.3,
			rightVoltage: 3.3
		};

		for (var i in configurations) {
			var pins = configurations[i].pins;
			for (var j in pins) {
				var pin = pins[j];
				switch (pin.type) {
					case "A2D":
					case "Analog":
						this.setPin(mux, pin.pin, analogIn);
						break;
					case "Digital":
						this.setPin(mux, pin.pin, ("output" == pin.direction) ? digitalOut :digitalIn);
						break;
					case "I2C":
						this.setPin(mux, pin.sda, i2cData);
						this.setPin(mux, pin.clock, i2cClock);
						break;
					case "Serial":
						if ("rx" in pin) this.setPin(mux, pin.rx, serialRx);
						if ("tx" in pin) this.setPin(mux, pin.tx, serialTx);
						break;
					case "PWM":
						this.setPin(mux, pin.pin, pwm);
						break;
					case "Power":
						this.setPin(mux, pin.pin, (5 == pin.voltage) ? power5V : power3_3V);
						break;
					case "Ground":
						this.setPin(mux, pin.pin, ground);
						break;
					case "Audio":
						break;
					default:
						throw new Error("Unrecognized pin type " + pin.type);
						break;
				}
			}
		}

		if (undefined === mux.leftVoltage)
			mux.leftVoltage = 3.3;
		if (undefined === mux.rightVoltage)
			mux.rightVoltage = 3.3;

		return mux;
	}

}

]]></program></package>
