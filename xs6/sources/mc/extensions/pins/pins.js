/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
import System from "system";
import Environment from "env";
import GPIOPin from "pinmux";
import Launcher from "launcher";
import {pinmap, GPIO_MASK, pinremap} from "pins/map";

const pinsdir = "pins/";

const PINMUX_DISCONNECTED = 0;
const PINMUX_POWER = 1;
const PINMUX_GROUND = 2;
const PINMUX_ANALOG = 3;
const PINMUX_DIGITAL_IN = 4;
const PINMUX_DIGITAL_OUT = 5;
const PINMUX_I2C_CLK = 6
const PINMUX_I2C_SDA = 7;
const PINMUX_SERIAL_RX = 8;
const PINMUX_SERIAL_TX = 9;
const PINMUX_PWM = 10;

const PINMUX_MAP = [
	GPIOPin.DISCONNECTED,
	GPIOPin.DISCONNECTED,	// power
	GPIOPin.DISCONNECTED,	// ground
	GPIOPin.A2D_IN,
	GPIOPin.GPIO_IN,
	GPIOPin.GPIO_OUT,
	GPIOPin.I2C_SCL,
	GPIOPin.I2C_SDA,
	GPIOPin.UART_RXD,
	GPIOPin.UART_TXD,
	GPIOPin.GPT_IO
];

function map_func(f) {
	return PIMUX_MAP.indexOf(f);
}

var Pins = {
	GPIO_MASK: GPIO_MASK,
	pinmux: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
	pinmux_dirty: false,

	port: 8900,
	handlers: [],
	timers: {},
	modules: {},
	https: {},

	configure(configuration, callback) {
		this.close();
		this.configuration = configuration;
		// reset power & ground
		this.pinmux.fill(0);
		// reset all other pinmux functions
		var mux = [];
		for (var i = 0; i < this.pinmux.length; i++)
			mux.push([pinmap(i + 1), PINMUX_MAP[this.pinmux[i]]]);
		GPIOPin.pinmux(mux);
		this.configurePins(configuration);
		if(callback){
			setTimeout(function() {
				callback(true);
			}, 0);	
		}
		Launcher.add(this, function(o) {
			o.close();
		});
	},
	discover(onFound, onLost){	// what are the parameters???
		let mdns = require.weak("mdns");
		if (!onFound && !onLost) {
			mdns.query("_kinoma_pins._tcp", null);	// stop the query
			return;
		}
		/*
		// connection description
		name: service.name,
		ip: service.ip,
		id: service.txt.uuid,
		bll: service.txt.bll.split(","),
		connections: []

		// service record
		fqst: kinomapins._tcp.local
		onFound: 
		fqst: kinomapins._tcp.local
		addr: 173.20.85.10 
		port: 3879
		keys: "bll=fakeSensor:uuid=000164EA-64EA-1001-EF6A-0011000001cc:_ws=ws://*:8900/:_http=http://*:8901/"
		name: test
		*/
		let connDes = function(res) {
			let conns = [];
			for (let key in res.keys) {
				let value = res.keys[key];
				if (key.startsWith("_")) {
					key = key.substring(1) + "://"
					if (value.startsWith(key)) {
						if (value.charAt(key.length) == '*')
							value = value.replace("*", res.addr);
						conns.push(value);
					}
				}
			}
			let des = {name: res.name, connections: conns};	// name must be there in any case
			if (res.addr && res.port)
				des.ip = res.addr + ":" + res.port;
			if (res.keys) {
				des.id = res.keys.uuid || "";
				des.bll = res.keys.bll || "";
			}
			return des;
		};
		mdns.query("_kinoma_pins._tcp", res => {
			switch (res.status) {
			case "found":
			case "update":
				if (onFound)
					onFound(connDes(res));
				break;
			case "lost":
				if (onLost)
					onLost(connDes(res));
				break;
			}
		});
	},
	connect(des, url){
		let protocol = undefined;
		if(typeof des == "string"){
			if(url) protocol = des;
			else url = des;
		}
		else
			url = des.connections[0]; // always pick the first one??
		if(!protocol) protocol = url.substring(0, url.indexOf(":"));
		try{
			let cons = require.weak(pinsdir + "_conn_" + protocol);
			return new cons(url);	
		}
		catch(error){
			trace("Pins.connect: unsupported protocol: " + protocol);
			return undefined;
		}
	},
	share(conf, discover) {
		if (!conf) {
			// stop the servers and services
			var mdns = require.weak("mdns");
			mdns.remove("_kinoma_pins._tcp");

			this.handlers.forEach(function(h) {
				h.close();
			});
			this.handlers = [];
			return;
		}
		if (conf instanceof Array) {
			conf.forEach(function(e) {
				this._share(e);
			}, this);
		}
		else
			this._share(conf);
		if (discover && discover.zeroconf) {
			var mdns = require.weak("mdns");
			var txt = {
				bll: (function(c) {var a = []; for (var i in c) a.push(i); return a;})(this.configuration).toString(),
				uuid: discover.uuid || require.weak("uuid").get(),
			};
			this.handlers.forEach(function(e) {
				txt["_" + e._protocol] = e._protocol + "://*:" + e._port + "/";
			});
			mdns.add("_kinoma_pins._tcp", discover.name, 9999 /* this won't be used */, txt);
		}
	},
	_share(conf) {
		if (typeof conf == "string")
			conf = {type: conf};
		var port = conf.port || this.port++;
		var handler = require.weak(pinsdir + "_" + conf.type);
		handler._protocol = conf.type;
		handler._port = port;
		handler.init(port, this.configuration);
		this.handlers.push(handler);
	},
	setPowerGround(pin, func) {
		if (pin >= 1 && pin <= this.pinmux.length)
			this.pinmux[pin - 1] = func;
		this.pinmux_dirty = true;
	},
	invoke(path, f, opt) {
		var cb;
		switch (path) {
		case "configuration":
			cb = function() {
				let conf;
				for (let i in this.configuration) {
					if (i != "k5mux") {
						if (!conf)
							conf = {};
						conf[i] = this.configuration[i];
					}
				}
				f(conf);
			};
			break;
		case "getPinMux":
			cb = function() {
				var pinmux = GPIOPin.getPinmux();
				var mux = Array(16).fill(0);
				for (var i in pinmux) {
					i = parseInt(i);
					var n = pinremap(i);	// convert the pin number
					if (n >= 1)
						mux[n - 1] = PINMUX_MAP.indexOf(pinmux[i]);	// conver the function number
				}
				// merge the power/ground config
				for (var i = 0; i < this.pinmux.length; i++) {
					switch (this.pinmux[i]) {
					case PINMUX_POWER: case PINMUX_GROUND: mux[i] = this.pinmux[i]; break;
					}
				}
				f({leftPins: mux.slice(0, 8), rightPins: mux.slice(8, 16), leftVoltage: 3.3, rightVoltage: 3.3});
			};
			break;
		case "setPinMux":
			this.close();
			this.pinmux.fill(0);
			for (let i = 0; i < f.leftPins.length; i++)
				this.pinmux[i] = f.leftPins[i];
			for (let i = 0; i < f.rightPins.length; i++)
				this.pinmux[i + 8] = f.rightPins[i];
			// set power / ground first
			if (this.modules.k5mux)
				this.configurePowerGround();
			// then other functions
			var mux = [];
			for (var i = 0; i < this.pinmux.length; i++)
				mux.push([pinmap(i + 1), PINMUX_MAP[this.pinmux[i]]]);
			GPIOPin.pinmux(mux);
			break;
		default:
			var a = path.split("/");
			if (a.length == 2) {
				cb = function() {
					var metadata = a[0];
					var module = a[1];
					var result;
					if (metadata == "metadata") {
						let modules = Pins.modules;
						let configuration = Pins.configuration;
						for (let i in configuration) {
							let probe = configuration[i];
							if (probe.require == module) {
								result = modules[i][metadata];
							}
						}
					}
					opt(result);
				};
			}
			else if (a.length > 2) {
				if (!this.modules[a[1]] || !this.modules[a[1]][a[2]]) {
					// throw new Error("Pins: " + a[1] + "." + a[2] + " not configured");
					trace("Pins: " + a[1] + "." + a[2] + " not configured\n");
					return;
				}
				if (typeof f == "function") {
					cb = function() {
						var res = this.modules[a[1]][a[2]]();
						f(res);
					};
				}
				else {
					var res = this.modules[a[1]][a[2]](f);
					if (opt)
						opt(res);
				}
			}
			else
				throw new Error("Pins: no command " + path);
			break;
		}
		if (cb) {
			var that = this;
			setTimeout(function() {
				cb.call(that)
			}, 0);
		}
	},
	repeat(path, ti, f, callback) {
		var a = path.split("/");
		if (ti === undefined) {
			let anID = callback || a[1];
			if ( anID && Pins.timers[anID]) {
				clearInterval(Pins.timers[anID]);
				delete Pins.timers[anID];
			}
		}
		else if (typeof ti == "number") {
			var that = this;
			var timer = setInterval(function() {
				var res = that.modules[a[1]][a[2]]();
				if (res !== undefined && f)
					f(res);
			}, ti);
			Pins.timers[callback || a[1]] = timer;
			return {
				close() {
					clearInterval(timer);
				}
			}
		}
		else {
			this.modules[a[1]][ti].repeat(f);	// needs to call the callback with the result of a[2]? but if it's a button it can be too late to read the value after the interrupt
		}
	},
	close() {
		for (var i in this.timers)
			clearInterval(this.timers[i]);
		this.timers = {};
		for (var i in this.modules) {
			if (i == "k5mux")
				continue;
			try{
				this.modules[i].close(); // error in user script may cause exception and result failure in unloading module
			}
			catch(error){
				trace("Error calling close function in module.\n");
			}
			delete this.modules[i];
		}
		Pins.discover();	// stop mdns monitoring
		Pins.share();	// stop mdns
		Launcher.remove(this);
	},
	configurePowerGround() {
		// set GPIO IN to all power and ground pins
		var mux = [], powerground = this.pinmux;
		for (var i = 0; i < powerground.length; i++) {
			switch (powerground[i]) {
			case PINMUX_POWER: case PINMUX_GROUND:
				mux.push([pinmap(i + 1), GPIOPin.GPIO_IN]);
				break;
			}
		}
		GPIOPin.pinmux(mux);
		this.modules.k5mux.set({analog: this.pinmux.slice(0, 8), digital: this.pinmux.slice(8, 16)});
	},
	configurePins(conf) {
		for (var i in conf) {
			var desc = conf[i];
			var require = desc.require || desc.type;
			var mod = this.load(desc);
			Pins.modules[i] = mod;
			conf[i] = { pins: mod.pins, require };	// @@ overwriting this.configuration!?!? Did I do this???
		}
		// power up before configure other pins
		if (this.modules.k5mux && this.pinmux_dirty) {
			this.configurePowerGround();
			this.pinmux_dirty = false;
		}
		// configure all pins at last
		for (var i in conf)
			Pins.modules[i].configure(Pins.modules[i]);
	},
	mergePin(modpin, confpin) {
		var r = {};
		for (var i in modpin)
			r[i] = modpin[i];
		if (confpin) {
			for (var i in confpin)
				r[i] = confpin[i];
		}
		return r;
	},
	createPin(pin){
		var pintype = require.weak(pinsdir + pin.type);
		return new pintype(pin);
	},
	load(conf) {
		var mod = require.weak(conf.require || conf.type);
		mod = Object.create(mod);
		mod.pins = this.mergePin(mod.pins);
		// merge conf.pins (or conf) into mod.pins
		if (conf.pins) {
			for (var i in conf.pins) {
				if (i in mod.pins)
					mod.pins[i] = this.mergePin(mod.pins[i], conf.pins[i]);
				else
					mod.pins[i] = conf.pins[i];
			}
		}
		else {
			for (var i in mod.pins) {
				if (mod.pins[i].type == conf.type) {
					mod.pins[i] = this.mergePin(mod.pins[i], conf);
					break;
				}
			}
		}
		for (var i in mod.pins)
			mod[i] = this.createPin(mod.pins[i]);
		return mod;
	},
};

if ("PINS" in System._global) {
	Pins.close = function() {
		for (var i in this.timers)
			clearInterval(this.timers[i]);
		this.timers = {};
		PINS.close();
		Launcher.remove(this);
	};
	Pins.configure = function(configuration, callback) {
		this.configuration = configuration;
		PINS.configure(configuration, callback);
		Launcher.add(this, function(o) {
			o.close();
		});
	},
	Pins.invoke = function(path, object, callback) {
		if (typeof object == "function") {
			callback = object;
			object = undefined;
		}
		PINS.invoke(path, object, callback);
	};
	Pins.repeat = function(path, object, condition, callback) {
		if (typeof condition == "function") {
			callback = condition;
			condition = object;
			object = undefined;
		}
		if (typeof condition != "number")
			condition = 50;
		var timers = this.timers;
		if (path in timers) {
			clearInterval(timers[path]);
			delete timers[path];
		}
		var timer = timers[path] = setInterval(() => {
			PINS.repeat(path, object, callback);
		}, condition);
		return {
			close() {
				clearInterval(timer);
				delete timers[path];
			}
		}
	};
}

if (System.config.powerGroundPinmux) {
	Pins.configure({
		k5mux: {
			require: "pins/K5PinMux",
			pins: {
				OEdigital: {pin: 17 | GPIO_MASK},
				STRdigital: {pin: 16 | GPIO_MASK},
				D14digital: {pin: 4 | GPIO_MASK},
				D58digital: {pin: 23 | GPIO_MASK},
				CPdigital: {pin: 34 | GPIO_MASK},

				OEanalog: {pin: 11 | GPIO_MASK},
				STRanalog: {pin: 12 | GPIO_MASK},
				D14analog: {pin: 13 | GPIO_MASK},
				D58analog: {pin: 15 | GPIO_MASK},
				CPanalog: {pin: 14 | GPIO_MASK},
			},
		}
	}, function(success) {
	});
}

export default Pins;
