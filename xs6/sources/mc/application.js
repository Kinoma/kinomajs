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
import Connection from "wifi";
import Environment from "env";
import Files from "files";
import wdt from "watchdogtimer";
import TimeInterval from "timeinterval";
import {setInterval, clearInterval, setTimeout, clearTimeout} from "timer";
import console from "console";

// for the compatilibity. Also overwrite the platform dependent functions
System._global.setInterval = setInterval;
System._global.clearInterval = clearInterval;
System._global.setTimeout = setTimeout;
System._global.clearTimeout = clearTimeout;
System._global.console = console;

console.log("starting (fw: " + new Date(System.timestamp * 1000) + ")");

System.addPath(Files.applicationDirectory + "/");
if (System.device == "host")
	System.addPath(Files.nativeApplicationDirectory + "/");	// to enable to load apps from the native file system
var env = new Environment();
var path = env.get("PATH");
if (path)
	System.addPath.apply(null, path.split(":"));

if (!env.get("FW_VER"))
	env.set("FW_VER", "0.90");

if ("PINS" in System._global)	// @@ if there's a better way to know...
	env.set("ELEMENT_SHELL", "1");
else
	env.unset("ELEMENT_SHELL");

if (System.device == "K5" || System.device == "MW300") {
	if (System.device == "K5") {
		var LED_PIN = 38;
		var MFI_PINS = {scl: 17, sda: 18};
	}
	else {
		var LED_PIN = 41;
		var MFI_PINS = {scl: 19, sda: 18};
	}
	var GPIOPin = require.weak("pinmux");
	GPIOPin.pinmux([[LED_PIN, GPIOPin.GPIO_OUT]]);
	var led = function(on) {
		GPIOPin.write(LED_PIN, !on);
	};
}
else {
	var led = function(on) {};
	var MFI_PINS = {};
}

System.configuration = {
	MFi_pins: MFI_PINS,
	timeSyncURL: "http://service.cloud.kinoma.com/includes/time-stamp.inc",
};

var application = {
	_applications: undefined,
	add: function(obj) {
		if (!obj)
			return;
		if (!this._applications)
			this._applications = [];
		this._applications.push(obj);
	},
	remove: function(obj) {
		var i = this._applications.indexOf(obj);
		if (i >= 0)
			this._applications.splice(i, 1);
	},
	_call: function(f) {
		var mode = Connection.mode;
		this._applications.forEach(function(o) {
			if (f in o) {
				try {
					o[f](mode);
				} catch(e) {
					console.log("application: " + f + ": caught an exception");
				}
			}
		});
	},
	start: function() {
		this._call("start");
	},
	stop: function() {
		this._call("stop");
	},
	run: function() {
		(require.weak("CLI")).prompt();
		var status = System.run();
		var config = Connection.config;
		return {status: status,
			save: config.save,
			config: config.state};
	},
};

var synctime = {
	start: function(mode) {
		if (mode & Connection.STA)
			setTimeout(this._start, 0);	// run in the main loop
	},
	_start: function() {
		var url = env.get("TIME_SERVER");
		if (url === null)
			url = System.configuration.timeSyncURL;
		else if (url === "")
			return;	// don't do anything
		var HTTPClient = require.weak("HTTPClient");
		var t1, t2;
		var req = new HTTPClient(url);
		req.onHeaders = function() {
			t2 = System.time;
			var date = this.getHeader("Date");
			if (date) {
				var t = Date.parse(date);
				t += 500;		// + 0.5 sec
				t += (t2 - t1) / 2;	// + the roundtrip time / 2
				System.time = t;
				console.log("setting RTC: " + Date(t));
			}
			application.remove(synctime);
		};
		req.start();
		t1 = System.time;
		synctime._req = req;
	},
	stop: function(mode) {
		this._req = undefined;
		System.gc();
	},
};

function main()
{
	led(1);
	Connection.init();		// initialize the network drivers
	System._init_rng(Connection.mac);	// initialize the rng
	application.add(require.weak("inetd"));
	application.add(require.weak("launcher"));
	application.add(synctime);
	// the main loop
	var ev = {save: false, config: Connection.NORMAL, status: 0};
	var toggle = 0, fallback = false, timer;
	wdt.start(90);
	do {
		switch (Connection.status) {
		case Connection.INITIALIZED:
		case Connection.DISCONNECTED:
			Connection.connect(ev.config);
			break;
		case Connection.CONNECTED:
			console.log("connected: " + Connection.ip);
			fallback = false;
			if (Connection.mode == Connection.STA) {
				led(1);		// solid on
				if (ev.save)
					Connection.save();
			}
			else {
				// provisioning mode
				led(0);
				timer = new TimeInterval(function() {
					led(toggle++ & 1);
				}, 500);	// slow blinking
				timer.start();
			}
			application.start();
			ev = application.run();		// the event loop
			application.stop();		// there should be nothing left to stop but just in case
			Connection.disconnect();
			if (timer) {
				timer.close();
				timer = undefined;
			}
			break;
		case Connection.ERROR:
			if (fallback) {
				// still error... give up
				ev.status = -1;
			}
			else {
				// try the provisioning mode
				Connection.connect(Connection.FALLBACK);
				fallback = true;
			}
			break;
		default:
			System.sleep(100);	// fast blink
			led(toggle++ & 1);
			break;
		}
		wdt.strobe();	// the timer is not running yet so wdt needs to be reset explicitly
	} while (ev.status == 0);
	wdt.stop();
	led(0);		// turn off
	Connection.fin();
	if (ev.status > 0)
		System.reboot(true);
	else {
		console.log("good night");
		System.shutdown(1);	// deep sleep
	}
	// never reach here
	System.fin();
	return ev.status;
}

function jig()
{
	console.log("=== JIG mode ===");
	led(0);
	Connection.init();	// need for socket
	System._init_rng("");
	var timer = new TimeInterval(function() {
		led(!(toggle++ & 5));
	}, 300), toggle = 0;
	timer.start();
	(require.weak("CLI")).prompt();
	var status = System.run();
	timer.close();
	led(0);
	Connection.fin();
	if (status > 0)
		System.reboot(true);
	else {
		console.log("good night");
		System.shutdown(1);	// deep sleep
	}
	return status;
}

var status = (function() {
	if (System.device == "K5") {
		var mac = env.get("MAC");
		if (mac) {
			try{
				Connection.mac = mac;	// must set the mac before initializing the network drivers	
			}
			catch(error){
				console.log("set mac address failed.");
				env.unset("MAC");
				env.save();
				return jig();
			}
			
			return main();
		}
		else {
			return jig();
		}
	}
	else
		return main();
})();

export default status;
