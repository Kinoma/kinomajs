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
import wdt from "watchdogtimer";
import LED from "board_led";

const SCAN_TIME = 3500;
const MIN_SCAN = 2;

var Wifi = {
	init() @ "xs_wm_init",
	fin() @ "xs_wm_fin",
	start() @ "xs_wm_start",
	set dhcpName(name) @ "xs_wm_setDHCP",
	_connect(params) @ "xs_wm_connect",
	disconnect() @ "xs_wm_disconnect",
	close() @ "xs_wm_close",
	get ip() @ "xs_wm_getIP",
	get mac() @ "xs_wm_getMAC",
	setmac(addr) @ "xs_wm_setMAC",
	get rssi() @ "xs_wm_getRSSI",
	scan(rescan) @ "xs_wm_scan",
	get status() @ "xs_wm_get_status",
	set status(s) @ "xs_wm_set_status",
	get mode() @ "xs_wm_get_mode",
	get config() @ "xs_wm_get_config",
	get ssid() @ "xs_wm_get_ssid",
	getInterfaces() @ "xs_wm_getInterfaces",
	stat() @ "xs_wm_stats_sockets",
	load() @ "xs_wm_load",

	set mac(addr) {
		if (typeof addr == "string") {
			var Arith = require.weak("arith");
			this.setmac((new Arith.Integer("0x" + addr)).toChunk(6));
		}
		else
			this.setmac(addr);
	},
	connect(config) {
		if (!config)
			config = this.NORMAL;
		else if (typeof config == 'object' && config.encryptedPassword) {
			let Crypt = require.weak("crypt");
			let Configuration = require.weak("config");
			let Files = require.weak("files");
			let Bin = require.weak("bin");
			let key = Crypt.PKCS8.decrypt(Files.read("srvkey.pk8"), Configuration.pk8Password);
			let pk = new Crypt.PKCS1_5(key, true);
			let plain = pk.decrypt(Bin.decode(config.encryptedPassword));
			config.password = String.fromArrayBuffer(plain);
		}
		this._connect(config);
	},

	notify(state) {
		switch (state) {
		case this.FATAL:
			console.log("-stderr", "wifi: fatal error. shutdown...");
			System.shutdown();
			break;
		case this.ERROR:
		case this.DISCONNECT:
			console.log("wifi: disconnected.");
			// try to reconnect
			this.close();
			break;
		default:
			console.log("wifi: notification: " + state);
			break;
		}
	},
	onScanComplete() {
		if (this.scanCallback) {
			this.scanCallback(this.scan());
			delete this.scanCallback;
		}
	},
	run(desiredMode) {
		var led = new LED({onColor: [1, 1, 0], offColor: [0, 0, 0]});
		led.on(1);

		var makeupKey = function() {
			let Configuration = require.weak("config");
			var mac = Wifi.mac;
			var mask = Configuration.envKey;
			if (mask) {
				var len = mask.length;
				var key = new Uint8Array(len);
				for (var i = 0; i < len; i++)
					key[i] = mask.charCodeAt(i) ^ mac.charCodeAt(i % mac.length);
				System._init_key(key.buffer);
			}
			else
				System._init_key(ArrayBuffer.fromString(mac));
		};

		if (System.config.wakeupButtons) {
			let POWER_BUTTON = System.config.wakeupButtons[0];
			var powerButtonPressed = function() {
				let GPIOPin = require.weak("pinmux");
				GPIOPin.pinmux([[POWER_BUTTON, GPIOPin.GPIO_IN]]);
				return GPIOPin.read(POWER_BUTTON) == 0;
			};
		}
		else {
			var powerButtonPressed = function() {return false;};
		}

		var aps, apiter;
		var getNextAP = function(aplist) {
			if (!apiter) {
				if (!aps)
					aps = new Environment("wifi", true, true);		// access points
				apiter = aps[Symbol.iterator]();
			}
			var conn;
			do {
				var ap = apiter.next();
				if (ap.done)
					return undefined;
				conn = JSON.parse(aps.get(ap.value));
			} while (!aplist.has(conn.bssid));
			return conn;
		};
		var status = 0, toggle = 0, fallback = false, aplist = undefined, scantime, nscanned;
		if (!(desiredMode & System.connection.JIGMODE)) {
			wdt.stop();
			wdt.start(10, false);
		}
		do {
			switch (this.status) {
			case this.NOTRUNNING:
				this.init();
				break;
			case this.UNINITIALIZED:
				this.start();
				break;
			case this.INITIALIZED:
				if (desiredMode & System.connection.JIGMODE)
					return System.connection.JIGMODE;
				makeupKey();
				if (!System.hostname)
					System.hostname = "Kinoma Element-" + this.mac;	// here's where the default hostname is manufactured, which appears as SSID in the provisioning mode and in the prompt on console
				this.dhcpName = System.hostname;
				// fall thru
			case this.SCANNED:
				if (aplist && ((new Date()).getTime() < scantime || nscanned++ < MIN_SCAN)) {
					let scanned = this.scan();
					scanned.forEach(e => aplist.set(e.bssid, e));
					this.scan(true);
					break;
				}
				// fall thru
			case this.DISCONNECTED:
				let conn = undefined;
				if (!fallback && (desiredMode & (System.connection.STA | System.connection.UAP)))
					conn = this.config.state;
				if (!conn || conn == this.NORMAL) {
					if (!aplist) {
						this.scan(true);
						scantime = (new Date()).getTime() + SCAN_TIME;
						aplist = new Map();
						nscanned = 0;
						break;
					}
					conn = getNextAP(aplist);
					if (!conn)
						conn = this.FALLBACK;
				}
				switch (conn) {
				case this.STA:
					console.log(`connecting: ${this.config.ssid}...`);
					break;
				case this.FALLBACK:
					console.log(`connecting: UAP...`);
					break;
				default:
					console.log(`connecting: ${conn.ssid}...`);
					break;
				}
				toggle = -1;
				this.connect(conn);
				break;
			case this.CONNECTED:
				console.log(`connected: ${this.ip} [${this.mac}]`);
				if (this.mode & this.STA) {		// station mode
					var conf = this.config;
					if (conf.save) {
						conf.save = false;	// turn off the save flag to avoid saving it again
						if (!aps)
							aps = new Environment("wifi", true, true);		// access points
						aps.set(conf.bssid, JSON.stringify(conf), 0);
					}
				}
				status = this.mode & this.STA ? System.connection.STA : System.connection.UAP;
				break;
			case this.ERROR:
				if (this.config.state == this.FALLBACK)
					status = System.connection.SAFEMODE;	// return
				else {
					fallback = true;
					this.status = this.DISCONNECTED;	// retry
				}
				break;
			case this.FATAL:
				console.log("-stderr", "wifi: fatal!");
				status = System.connection.ERROR;
				break;
			default:	/* pending */
				if (powerButtonPressed())
					status = System.connection.SAFEMODE;
				else {
					System.sleep(100);
					if (toggle >= 0)
						led.on(toggle++ & 1);
					else
						led.on(1);
				}
				break;
			}
			wdt.strobe();	// the wdt is not running yet so kick the dog explicitly
		} while (status == 0);
		wdt.stop();
		LED.resume();
		return status;
	},
};

Wifi.load();

export default Wifi;
