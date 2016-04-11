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
import Pins from "pins";
import { WebSocketClient } from "websocket";

const PINMUX_DISCONNECTED = 0;
const PINMUX_POWER = 1;
const PINMUX_GROUND = 2;
const PINMUX_ANALOG = 3;
const PINMUX_DIGITAL_IN = 4;
const PINMUX_DIGITAL_OUT = 5;
const PINMUX_I2C_CLK = 6
const PINMUX_I2C_SDA = 7;
const PINMUX_SERIAL_RX = 8
const PINMUX_SERIAL_TX = 9;
const PINMUX_PWM = 10;

const TYPE_DISCONNECTED = "Disconnected";
const TYPE_POWER = "Power";
const TYPE_GROUND = "Ground";
const TYPE_ANALOG = "Analog";
const TYPE_DIGITAL = "Digital";
const TYPE_I2C = "I2C"
const TYPE_SERIAL = "Serial";
const TYPE_PWM = "PWM";

const HTTP_ERROR_505 = "Internal Server Error";

let handlers = {
	getNetworkLevel(helper, query) {
		let Wifi = require.weak("wifi");
		helper.wsResponse(Wifi.rssi);
	},
	getNetworkList(helper, query) {
		let Wifi = require.weak("wifi");
		helper.wsResponse(Wifi.scan(false));
		Wifi.scan(true);
	},
	getNetworkMAC(helper, query) {
		let Wifi = require.weak("wifi");
		let mac = Wifi.mac.toUpperCase();
		mac = mac.substr(0, 2) + ":"
			+ mac.substr(2, 2) + ":"
			+ mac.substr(4, 2) + ":"
			+ mac.substr(6, 2) + ":"
			+ mac.substr(8, 2) + ":"
			+ mac.substr(10, 2);
		helper.wsResponse(mac);
	},
	getTimezone(helper, query) {
		let timezone = System.timezone;
		helper.wsResponse({  "zone": timezone.timedifference / 60, "daylight-savings": timezone.dst ? true : false });
	},
	networkConfigure(helper, query) {
		if (query) {
			helper.wsResponse();
			let Wifi = require.weak("wifi");
			Wifi.connect(query.config);
		}
		else 
			helper.wsErrorResponse(505, HTTP_ERROR_505);
	},
	pinExplorerStart(helper, query) {
		Pins.invoke("getPinMux", pinmux => {
			let explorer = {};
			let leftPins = pinmux.leftPins;
			for (let pin = 0, c = leftPins.length; pin < c; pin++) {
				helper.pinExplorerAddPin(explorer, 1 + pin, leftPins[pin], pinmux.leftVoltage);
			}
			let rightPins = pinmux.rightPins;
			for (let pin = 0, c = rightPins.length; pin <= c; pin++) {
				helper.pinExplorerAddPin(explorer, 9 + pin, rightPins[pin], pinmux.rightVoltage);
			}
			Pins.configure(explorer, success => {
				let url = helper.pinsStartSharing(query.ip);
				if (url)
					helper.wsResponse(url)
				else
					helper.wsErrorResponse(505, HTTP_ERROR_505)
			}, error => {
				helper.pinsStopSharing();
				helper.wsErrorResponse(505, HTTP_ERROR_505);
			});
		});
	},
	pinExplorerStop(helper, query) {
		helper.pinsStopSharing();
		helper.wsResponse()
	},
	pinsShare(helper, query) {
		if (query.shared) {
			let url = helper.pinsStartSharing();
			if (url)
				helper.wsResponse(url)
			else
				helper.wsErrorResponse(505, HTTP_ERROR_505)
		}
		else {
			helper.pinsStopSharing();
			helper.wsResponse();
		}
	},
	setName(helper, query) {
		System.hostname = query.name;
		helper.wsResponse();
	},
	setTimezone(helper, query) {
		let timezone = { "timedifference": query.timezone.zone * 60, "dst": query.timezone["daylight-savings"] ? 60 : 0 };
		System.timezone = timezone;
		helper.wsResponse();
	},
	updateSoftwareStatus(helper, query) {
		helper.wsResponse(helper.softwareStatus);
	},
	updateSystemStatus(helper, query) {
		helper.wsResponse(helper.systemStatus);
	}
};

class XSEditHelper {
	constructor() {
		this.ws = undefined;
		this.xsedit = undefined;
		this.connectionTimeout = 20000; // 20 sec
		this.softwareStatus = {
			"updating": false,
		};
		this.systemStatus = {
			"updating": false,
		};
	}
	onLaunch(args) {
		let xs = require.weak("debug");
		xs.logout();
		this.wsConnect(args.xsedit);
	}
	onQuit() {
		this.wsDisconnect();
	}
	// pins
	pinExplorerAddPin(explorer, pin, type, voltage) {
		switch (type) {
			case PINMUX_DISCONNECTED:
			break;
			case PINMUX_POWER:
				explorer[TYPE_POWER + pin] = {
					pins: { power: { pin, type:TYPE_POWER } },
					require: TYPE_POWER,
				};					
			break;
			case PINMUX_GROUND:
				explorer[TYPE_GROUND + pin] = {
					pins: { ground: { pin, type:TYPE_GROUND } },
					require: TYPE_GROUND,
				};					
			break;
			case PINMUX_ANALOG:
				explorer[TYPE_ANALOG + pin] = {
					pins: { analog: { pin } },
					require: TYPE_ANALOG,
				};					
			break;
			case PINMUX_DIGITAL_IN:
				explorer[TYPE_DIGITAL + pin] = {
					pins: { digital: { pin, direction:"input" } },
					require: TYPE_DIGITAL,
				};					
			break;
			case PINMUX_DIGITAL_OUT:
				explorer[TYPE_DIGITAL + pin] = {
					pins: { digital: { pin, direction:"output" } },
					require: TYPE_DIGITAL,
				};					
			break;
			case PINMUX_I2C_CLK:
			break;
			case PINMUX_SERIAL_RX:
			break;
			case PINMUX_PWM:
				explorer[TYPE_PWM + pin] = {
					pins: { pwm: { pin } },
					require: TYPE_PWM,
				};					
			break;
		}
	}
	pinsStartSharing() {
		if (Pins.handlers.length == 0)
			Pins.share({type: "ws"}); // "ws" breaks xsedit on close
		var handlers = Pins.handlers;
		if (Pins.handlers.length > 0) {
			let Wifi = require.weak("wifi");
			let handler = Pins.handlers[0];
			let url = handler._protocol + '://' + Wifi.ip + ':' + handler._port + '/';
			return url;
		}
		return;
	}
	pinsStopSharing() {
		Pins.share();
		Pins.close();
	}
	// websocket
	wsConnect(xsedit) {
		let context = this;
		let url = "ws://" + xsedit;
		
		let helper = this;
		let ws = this.ws = new WebSocketClient(url);
		ws.onopen = function() {
// 			trace("WSC: onopen\n");
			try {
				this.helper = helper;
				var uuid = require.weak("uuid");
				this.send(uuid.get());
			}
			catch (e) {
				this.helper.wsErrorResponse(505, HTTP_ERROR_505);
			}
		}
		ws.onmessage = function(message) {
// 			trace("WSC: onmessage " + message.data + "\n");
			try {
				let json = JSON.parse(message.data);
				let helper = this.helper;
				let handler = ("handler" in json) ? json.handler : undefined;
				if (handler && (handler in handlers))
					handlers[handler](helper, json);
			}
			catch (e) {
				this.helper.wsErrorResponse(505, HTTP_ERROR_505);
			}
		}
		ws.onclose = function() {
// 			trace("WSC: onclose\n");
			if (this.helper) {
				this.helper.ws = undefined;
				this.helper = undefined;
			}
			let Launcher = require.weak("launcher");
			Launcher.quit();
		}
		ws.onerror = function(error) {
// 			trace("WSC: onerror " + error.code + " " + error.reason  + "\n");
		}
	}
	wsDisconnect() {
		if (this.ws) {
			this.ws.close();
			this.ws = undefined;
		}
	}
	wsResponse(content) {
		let json;
		if (content != undefined) {
// 			trace("response: " + JSON.stringify(content) + "\n");
			json = { status:200, content };
		}
		else {
// 			trace("response: -\n");
			json = { status:204 };
		}
		this.ws.send(JSON.stringify(json));
	}
	wsErrorResponse(status, reason) {
		let json = { status, reason };
		this.ws.send(JSON.stringify(json));
	}
}

var xsEditHelper = new XSEditHelper;
export default xsEditHelper;
