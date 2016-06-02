//@program
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
var Pins = require("pins");

// ----------------------------------------------------------------------------------
// Skins
// ----------------------------------------------------------------------------------
const blackSkin = new Skin({fill: "black"});
const iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:128, height:128, aspect:"fit" });

// ----------------------------------------------------------------------------------
// Main screen / UI elements
// ----------------------------------------------------------------------------------

const whiteStyle = new Style({ font:"Arial", size:24, color:"white", horizontal:"center" });

// Main UI container
var MainScreen = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: blackSkin, active:true,
	contents: [
		Content($, { center:0, middle:0, skin:iconSkin }),
// 		Label($, { anchor:"LABEL", bottom:10, style:whiteStyle, string:"IP" }),
	]
}});

Handler.Bind("/network/interface/remove", class extends Behavior {
	onInvoke(handler, message) {
		application.invoke(new Message("xkpr://shell/close?id=" + application.id));
	}
});

// ----------------------------------------------------------------------------------
// Application
// ----------------------------------------------------------------------------------

const PINMUX_DISCONNECTED = 0;
const PINMUX_POWER = 1;
const PINMUX_GROUND = 2;
const PINMUX_ANALOG = 3;
const PINMUX_DIGITAL_IN = 4;
const PINMUX_DIGITAL_OUT = 5;
const PINMUX_I2C_CLK = 6
const PINMUX_I2C_SDA = 7;
const PINMUX_PWM = 8;

const TYPE_DISCONNECTED = "Disconnected";
const TYPE_POWER = "Power";
const TYPE_GROUND = "Ground";
const TYPE_ANALOG = "Analog";
const TYPE_DIGITAL = "Digital";
const TYPE_I2C_CLK = "I2C Clock"
const TYPE_I2C_SDA = "I2C Data";
const TYPE_PWM = "PWM";

let handlers = {
	pinExplorerStart(helper, query) {
		Pins.invoke("getPinMux", pinmux => {
			trace(JSON.stringify(pinmux, null, " ") + "\n");
			let explorer = {};
			let leftPins = pinmux.leftPins;
			for (let pin = 0, c = leftPins.length; pin < c; pin++) {
				helper.pinExplorerAddPin(explorer, 51 + pin, leftPins[pin], pinmux.leftVoltage);
			}
			let rightPins = pinmux.rightPins;
			for (let pin = 0, c = rightPins.length; pin <= c; pin++) {
				helper.pinExplorerAddPin(explorer, 59 + pin, rightPins[pin], pinmux.rightVoltage);
			}
			trace(JSON.stringify(explorer, null, " ") + "\n");
			Pins.configure(explorer, success => {
				let url = helper.pinsStartSharing(query.ip);
				if (url)
					helper.wsResponse(url)
				else
					helper.wsErrorResponse(500, "Internal Server Error")
			}, error => {
				helper.pinsStopSharing();
				helper.wsErrorResponse(500, "Internal Server Error");
			});
		});
	},
	pinExplorerStop(helper, query) {
		helper.pinsStopSharing();
		helper.wsResponse()
	},
	pinsShare(helper, query) {
		if (query.shared) {
			if (!model.pins)
				model.pins = Pins.share({type: "ws"});
			var url = model.pins[0].instance.url;
			helper.wsResponse(url.replace("*", query.ip));
		}
		else {
			Pins.share();
    		delete model.pins;
			helper.wsResponse();
		}
	},
	clearApps(helper, query) {
		let message = new Message("xkpr://shell/settings/clear-apps");
		message.invoke();
		helper.wsResponse();
	},
	clearAppsPrefs(helper, query) {
		let message = new Message("xkpr://shell/settings/clear-apps-prefs");
		message.invoke();
		helper.wsResponse();
	},
	clearCookies(helper, query) {
		HTTP.Cookies.clear(); 
		K4.log("settings", "Clearing HTTP Cookies");
		helper.wsResponse();
	},
	clearHTTPCache(helper, query) {
		HTTP.Cache.clear(); 
		K4.log("settings", "Clearing HTTP Cache");
		helper.wsResponse();
	},
	clearKnownNetworks(helper, query) {
		let message = new Message("xkpr://shell/network/known");
		message.method = "DELETE";
		message.invoke();
		helper.wsResponse();
	},
	getNetworkLevel(helper, query) {
		let message = new Message("xkpr://shell/network/status");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json.signal_level);
		});
	},
	getNetworkList(helper, query) {
		let message = new Message("xkpr://shell/network/status");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse("scanned" in json ? json.scanned : []);
		});
	},
	getNetworkMAC(helper, query) {
		let message = new Message("xkpr://shell/network/connect/status");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json.address.toUpperCase());
		});
	},
	getStartupApp(helper, query) {
		let message = new Message("xkpr://shell/settings/startup-app");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	getStartupAppList(helper, query) {
		let message = new Message("xkpr://shell/settings/startup-app-list");
		return message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	getTimezone(helper, query) {
		let message = new Message("xkpr://shell/settings/timezone");
		message.invoke(Message.JSON).then(json => {
			helper.wsResponse(json);
		});
	},
	networkScan(helper, query) {
		let message = new Message(query.active ? "xkpr://shell/network/wifi/scan/start" : "xkpr://shell/network/wifi/scan/stop");
		message.invoke();
		helper.wsResponse();
	},
	setName(helper, query) {
		let message = new Message("xkpr://shell/settings/name");
		message.requestText = JSON.stringify(query.name);
		message.method = "PUT";
		message.invoke();
		helper.wsResponse();
	},
	setTimezone(helper, query) {
		let message = new Message("xkpr://shell/settings/timezone");
		message.requestText = JSON.stringify(query.timezone);
		message.method = "PUT";
		message.invoke();
		helper.wsResponse();
	},
	updateSoftware(helper, query) {
		model.doUpdateSoftware(query.update);
		helper.wsResponse();
	},
	updateSoftwareStatus(helper, query) {
		helper.wsResponse(helper.softwareStatus);
	},
};

let model = application.behavior = Behavior({
	onLaunch(application) {
		this.softwareStatus = {
			"updating": false,
		};
		this.systemStatus = {
			"updating": false,
		};
		this.data = {};
		application.add(new MainScreen(this.data));

		let connect = getEnvironmentVariable("debugger");
		let url = "http://" + connect.split(":")[0] + ":9999/xsedit?id=" + application.id;
// 		this.data.LABEL.string = url;
		let message = new Message(url);
		message.invoke(Message.TEXT).then(host => {
			this.xsedit = host;
			this.wsConnect(host);
		});
	},
	onQuit(application) {
		if (this.pins) {
    		Pins.share();
    		delete this.pins;
    	}
		this.wsDisconnect();
	},
	// software
	doUpdateSoftware(json) {
		let status = this.softwareStatus;
		let sourceURL = status.sourceURL = json.url;
		let targetURL = status.targetURL = mergeURI(Files.temporaryDirectory, "software");
		status.updating = true;
		status.message = "DOWNLOADING";
		status.md5 = json.md5;
		Files.deleteFile(targetURL);
		application.invoke(new Message("/downloadSoftware?" + serializeQuery({ sourceURL, targetURL })));
	},
	onSoftwareDownloadError() {
		let status = this.softwareStatus;
		status.message = "DOWNLOAD FAILED";
	},
	onSoftwareDownloadProgress(offset, size) {
		let status = this.softwareStatus;
		status.message = "DOWNLOADING " + offset + "/" + size;
	},
	onSoftwareDownloaded() {
		let status = this.softwareStatus;
		let path;
		if ("toPath" in Files)
			path = Files.toPath(status.targetURL);
		else
			path = status.targetURL.slice(7);
		status.message = "UPDATING";
// 		application.invoke(new Message("/installSoftware?" + serializeQuery({ path, md5: status.md5 })));
	},
	onSoftwareInstallError() {
		status.message = "UPDATE FAILED";
	},
	onSoftwareInstalled() {
		application.invoke(new Message("xkpr://shell/close?id=" + application.id));
		application.invoke(new Message("xkpr://shell/quitLauncher"));
	},
	// pins
	pinExplorerAddPin(explorer, pin, type, voltage) {
		switch (type) {
			case PINMUX_DISCONNECTED:
			break;
			case PINMUX_POWER:
				explorer[TYPE_POWER + pin] = {
					pins: { power: { pin, type:TYPE_POWER, voltage } },
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
			case PINMUX_I2C_SDA:
			break;
			case PINMUX_PWM:
			break;
		}
	},
	pinsStartSharing(ip) {
 		if (!model.pins)
 			model.pins = Pins.share({type: "ws"});
		var url = model.pins[0].instance.url;
 		return url.replace("*", ip);
	},
	pinsStopSharing() {
		Pins.share();
		Pins.close();
	},
	// websocket
	wsConnect(xsedit) {
		let context = this;
		let url = "ws://" + xsedit;
		
		let helper = this;
		let ws = this.ws = new WebSocket(url);
		ws.onopen = function() {
// 			trace("WSC: onopen\n");
			try {
				this.helper = helper;
				// var uuid = require.weak("uuid");
				let message = new Message(mergeURI(url, "xkpr://shell/description"));
				message.invoke(Message.JSON).then(json => {
					this.send(json.uuid);
				});
			}
			catch (e) {
				debugger;
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
				this.helper.wsErrorResponse(505, "Internal Server Error");
			}
		}
		ws.onclose = function() {
// 			trace("WSC: onclose\n");
			application.invoke(new Message("xkpr://shell/close?id=" + application.id));
			this.helper = undefined;
		}
		ws.onerror = function(error) {
// 			trace("WSC: onerror " + error.code + " " + error.reason  + "\n");
		}
	},
	wsDisconnect() {
		if (this.ws) {
// 			trace("WSC: close\n");
			this.ws.close();
			this.ws = undefined;
		}
	},
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
	},
	wsErrorResponse(status, reason) {
		let json = { status, reason };
		this.ws.send(JSON.stringify(json));
	}
});

// settings app

Handler.Bind("/downloadSoftware", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		handler.download(new Message(query.sourceURL), query.targetURL);
	}
	onProgress(handler, message, offset, size) {
// 		trace("PROGRESS " + offset + " " + size + "\n");
		model.onSoftwareDownloadProgress(offset, size);
	}
	onComplete(handler, message) {
		if (message.error != 0 || message.status != 200)
			model.onSoftwareDownloadError();
		else
			model.onSoftwareDownloaded();
	}
});

Handler.Bind("/installSoftware", class extends Behavior {
	onInvoke(handler, message) {
		Message.notify(new Message("/software/update/start"));
		var query = parseQuery(message.query);
		handler.invoke(new Message("xkpr://k4/doLauncherUpdate?" + serializeQuery({ path:query.path, md5:query.md5 })), Message.TEXT);
	}
	onComplete(handler, message) {
		Message.notify(new Message("/software/update/complete"));
		if (message.error != 0 || message.status != 200)
			model.onSoftwareInstallError();
		else
			model.onSoftwareInstalled();
	}
});

