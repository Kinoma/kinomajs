/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
import {pinmap, GPIO_MASK} from "pins/map";

const pinsdir = "pins/";

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
		Pins.pinmux.fill(0);
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
	share(conf, discover) {
		if (!conf) {
			// stop the servers and services
			var mdns = require.weak("mdns");
			mdns.removeService("_kinoma_pins._tcp.local");
			mdns.stop();

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
			mdns.start("local", System.hostname);
			var svc = mdns.newService("_kinoma_pins._tcp.local", 9999 /* this won't be used */, discover.name);
			var txt = {
				bll: (function(c) {var a = []; for (var i in c) a.push(i); return a;})(this.configuration).toString(),
				uuid: discover.uuid || require.weak("uuid").getUUID(),
			};
			this.handlers.forEach(function(e) {
				txt["_" + e._protocol] = e._protocol + "://*:" + e._port + "/";
			});
			svc.configure(txt);
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
		
		GPIOPin.enable_leds();
		GPIOPin.led(0, 0);	/* turn off the lights */
		GPIOPin.led(1, 0);
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
				f(this.configuration);
			};
			break;
		case "getPinMux":
			cb = function() {
				f(this.pinmux);
			};
			break;
		case "setPinMux":
			// copy the parameter into the pinmux arrays
			for (var i = 0; i < this.pinmux.length && i < f.length; i++)
				this.pinmux[i] = f[i];
			if (this.modules.k5mux)
				this.modules.k5mux.set({analog: this.pinmux.slice(0, 8), digital: this.pinmux.slice(8, 16)});
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
			if (callback && Pins.timers[callback]) {
				clearInterval(Pins.timers[callback]);
				delete Pins.timers[callback];
			}
		}
		else if (typeof ti == "number") {
			var that = this;
			var timer = setInterval(function() {
				var res = that.modules[a[1]][a[2]]();
				if (res !== undefined)
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
		Pins.share();	// stop mdns
		Launcher.remove(this);
	},
	configurePins(conf) {
		for (var i in conf) {
			var desc = conf[i];
			var require = desc.require || desc.type;
			var mod = this.load(desc);
			Pins.modules[i] = mod;
			conf[i] = { pins: mod.pins, require };
		}
		if (Pins.modules.k5mux && Pins.pinmux_dirty) {
			// set GPIO IN for all power and ground pins
			var mux = [], powerground = this.pinmux;
			for (var i = 0; i < powerground.length; i++) {
				if (powerground[i])
					mux.push([pinmap(powerground[i]), GPIOPin.GPIO_IN]);
			}
			GPIOPin.pinmux(mux);
			// power up before configure other pins
			this.modules.k5mux.set({analog: this.pinmux.slice(0, 8), digital: this.pinmux.slice(8, 16)});
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

// just to make the trivial object global for the compatibility..........
function for_bll() @ "for_bll";
for_bll();

if ((new Environment()).get("ELEMENT_SHELL")) {
	Pins.close = function() {
		for (var i in this.timers)
			clearInterval(this.timers[i]);
		this.timers = {};
		PINS.close();
	};
	Pins.configure = function(configuration, callback) {
		this.configuration = configuration;
		PINS.configure(configuration, callback);
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

if (System.device == "K5") {
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
