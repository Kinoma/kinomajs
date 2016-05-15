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
import {setInterval, clearInterval, setTimeout, clearTimeout} from "timer";
import console from "console";
import LED from "board_led";

const FW_VERSION = "2.0.0";

// for the compatilibity. Also overwrite the platform dependent functions
System._global.setInterval = setInterval;
System._global.clearInterval = clearInterval;
System._global.setTimeout = setTimeout;
System._global.clearTimeout = clearTimeout;
System._global.console = console;
System._global.sensorUtils = {
	mdelay(ms) {
		System.sleep(ms);
	},
	delay(s) {
		System.sleep(s * 1000);
	},
};

// define connection mode
const MODEBITS = 8;
System.connection = {
	STA: 0x01,
	UAP: 0x02,
	SAFEMODE: 0x04,
	ERROR: -1,
	MODEBITS: MODEBITS,
	MODEMASK: ((1 << MODEBITS) - 1),
	WAC: 0x1 << MODEBITS,
	SINGLEUSER: 0x2 << MODEBITS,
	FWUPDATE: 0x4 << MODEBITS,
	JIGMODE: 0x8 << MODEBITS,
};

// set up the onboard LEDs and the power button
var led = new LED({onColor: [1, 1, 1], offColor: [0, 0, 0], default: true});

var powerbutton = {};
if (System.config.wakeupButtons) {
	let POWER_BUTTON = System.config.wakeupButtons[0];
	powerbutton = {
		start: function() {
			let GPIOPin = require.weak("pinmux");
			GPIOPin.event(POWER_BUTTON, GPIOPin.RISING_EDGE|GPIOPin.FALLING_EDGE, val => {
				if (val == 0)	// the button is being pressed
					return;
				let wdt = require.weak("watchdogtimer");
				led.stop();
				wdt.stop();
				wdt.shutdown(5);		// in case it gets stuck..
				System.shutdown(0);
			});
		},
		stop: function() {
			let GPIOPin = require.weak("pinmux");
			GPIOPin.event(POWER_BUTTON);
		},
	}
}

var application = {
	_applications: [],
	add: function(obj) {
		if (obj)
			this._applications.push(obj);
	},
	remove: function(obj) {
		var i = this._applications.indexOf(obj);
		if (i >= 0)
			this._applications.splice(i, 1);
	},
	_call: function(call, mode) {
		this._applications.forEach(obj => {
			if (call in obj) {
				try {
					obj[call](mode, application);
				} catch(e) {
					console.log("-stderr", "application: caught an exception");
				}
			}
		});
	},
	start: function(mode) {
		let wdt = require.weak("watchdogtimer");
		wdt.stop();
		wdt.start(45);
		// run the modules in the event loop
		System.sched(() => this._call("start", mode));
	},
	stop: function(mode) {
		let wdt = require.weak("watchdogtimer");
		wdt.strobe();
		this._call("stop", mode);
		wdt.stop();
		wdt.start(10, false);
	},
	get connection() {
		return require.weak("wifi");
	},
	run(mode) {
		var start = !(mode & (System.connection.SAFEMODE | System.connection.JIGMODE | System.connection.SINGLEUSER));
		if (start)
			this.start(mode);
		setTimeout(() => (require.weak("CLI")).prompt(), 1 /* make sure it appears after the services has started */);
		return System.run(start ? () => this.stop(mode) : undefined);
	},
};

function main(mode)
{
	led.on(1);
	{	// open block so Files can be garbage collected
		let Files = require.weak("files");

		if (!(mode & System.connection.SINGLEUSER)) {
			// check the file system
			let iter = Files.VolumeIterator(), fserr = [];
			console.enable = 0;
			for (let vol of iter) {
				let err = Files.fsck(vol.name, true);
				if (err != 0)
					fserr.push(vol.name + ": " + (err < 0 ? "corrupted" : "recovered"));
			}
			console.enable = console.LOGFILE;
			fserr.map(e => console.log("-stderr", e));
		}

		// set up the application paths
		System.addPath(Files.applicationDirectory + "/", Files.documentsDirectory + "/");
		if (System.device == "host")
			System.addPath(Files.nativeApplicationDirectory + "/");    // to enable to load apps from the native file system

		try {
			System.config = require.weak("config");
			if (Files.getInfo(Files.applicationDirectory + "/rc.js") || Files.getInfo(Files.documentsDirectory + "/rc.js"))
				require.weak("rc");
		} catch (e) {
			// silently ignore the error
		}
	}
	console.log(`starting... FW version: ${System.get("FW_VER")} (${new Date(System.timestamp * 1000)})`);

	application.add(require.weak("inetd"));
	application.add(require.weak("synctime"));
	application.add(powerbutton);
	application.add({
		start: mode => (require.weak("launcher")).start(mode),
		stop: mode => (require.weak("launcher")).stop(mode)
	});
	application.add({
		start: mode => {console.enable |= console.XSBUG},
		stop: mode => {console.enable &= ~console.XSBUG},
	});

	// the main loop
	var status, conn = 0;
	do {
		conn = application.connection.run(conn | mode);
		if (conn == System.connection.ERROR)
			// fatal error! cannot continue...
			break;
		switch (conn) {
		case System.connection.STA:
			led.run({onColor: [0, 1, 0]});	// green
 			break;
		case System.connection.UAP:
			led.run({onColor: [1, 1, 1]});	// white
			break;
		case System.connection.SAFEMODE:
			led.run({onColor: [1, 0, 0]});	// red
			break;
		}
		status = application.run(conn | mode);		// the event loop
		application.connection.disconnect();
	} while (status == 0);
	application.connection.fin();
	led.stop();		// turn off
	if (status > 0)
		System.reboot(true);
	else {
		console.log("good night");
		System.shutdown(1);	// deep sleep
	}
	// never reach here
	System.fin();
}

function jig(error)
{
	console.log("=== JIG mode ===");
	application.connection.run(System.connection.JIGMODE);
	led.run(error ? {onColor: [1, 0, 0]} : {interval: 300, pattern: 5});
	var status = application.run(System.connection.JIGMODE);	// try to keep running even in the case of the fatal error
	application.connection.fin();
	led.stop();
	if (status > 0)
		System.reboot(true);
	else {
		console.log("good night");
		System.shutdown(1);	// deep sleep
	}
}

(function() {
	try {
		var bootMode = 0;
		var mac;
		{
			let Environment = require.weak("env");
			let env = new Environment();
			let path = env.get("PATH");
			if (path)
				System.addPath.apply(null, path.split(":"));
			bootMode = env.get("BOOT_MODE") || 0;
			if (bootMode) {
				bootMode <<= System.connection.MODEBITS;
				env.set("BOOT_MODE");
				env.save();
			}
			let ver = env.get("FW_VER");
			if (!ver || System.versionCompare(ver, FW_VERSION) < 0)
				env.set("FW_VER", FW_VERSION);
			mac = env.get("MAC");
		}

		if (System.device == "K5") {
			if (mac || !System.config.usbConsole) {
				if (mac)
					application.connection.mac = mac;	// must set the mac before initializing the network drivers
				return main(bootMode);
			}
			else
				return jig();
		}
		else
			return main(bootMode);
	} catch (e) {
		console.log("-stderr", "falling back to the JIG mode");
		let wdt = require.weak("watchdogtimer");
		wdt.stop();
		return jig(true);
	};
})();

export default application;
