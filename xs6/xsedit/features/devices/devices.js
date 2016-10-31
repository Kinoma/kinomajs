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
// MODEL

import {
	model,
} from "shell/main";

import {
	Feature
} from "shell/feature";

import {
	Viewer
} from "shell/viewer";

import tool from "shell/tool";

const ssdpDeviceType = "urn:schemas-kinoma-com:device:shell:1";
const mdnsServiceType = "_kinoma_setup._tcp.";

export const updateCredentials = {
//	certificates: Files.readText(mergeURI(shell.url, "../../certs/xsedit.cert.pem")),
	policies: "allowOrphan",
//	key: Files.readText(mergeURI(shell.url, "../../certs/xsedit.priv.pem"))
}

let wsIndex = 0;

export default class extends Feature {
	constructor(model) {
		super(model, "Devices");
		this.Template = DevicePane;
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
		this.viewers = [
			new DeviceViewer(this),
		];
		
		this.Configs = [];
		this.DiscoveryConfigs = [];
		this.SerialConfigs = [];
		this.SimulatorConfigs = [];
		if (Files.exists(mergeURI(model.simulatorsURI, CreateSimulatorConfig.defaultURL)))
			this.SimulatorConfigs.push(CreateSimulatorConfig);
		if (Files.exists(mergeURI(model.simulatorsURI, ElementSimulatorConfig.defaultURL)))
			this.SimulatorConfigs.push(ElementSimulatorConfig);
		if (Files.exists(mergeURI(model.simulatorsURI, EmbedSimulatorConfig.defaultURL)))
			this.SimulatorConfigs.push(EmbedSimulatorConfig);
		
		this.PlatformConfigs = [
//			MacOSPlatformConfig,
//			iOSPlatformConfig,
//			AndroidPlatformConfig,
		];
		
		this.networkInterface = [];
		let url = mergeURI(shell.url, "../features/devices/configs/");
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					let Config = require(mergeURI(url, info.path + "/" + info.path));
					this.Configs.push(Config);
					if (!Config.id.endsWith(".factory"))
						this.DiscoveryConfigs.push(Config);
					if ("serial" in Config)
						this.SerialConfigs.push(Config);
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		this.DiscoveryConfigs.sort((a, b) => a.product.compare(b.product));
		this.SerialConfigs.sort((a, b) => a.product.compare(b.product));
		
		this.server = new HTTP.Server({ port: 9999 });
		this.server.behavior = this;
		this.server.start();
		this.devices = this.SimulatorConfigs.concat(this.PlatformConfigs).map(Config => new Config);
		this.authorizations = {};
		this.currentDevice = this.devices[0];
		this.discoveryFlags = {};
		this.filteredDevices = [];
		this.platformFlags = {};
		this.serialFlags = {};
		this.simulatorFiles = {};
		this.simulatorFlags = {};
		this.ssdp = new SSDP.Client(ssdpDeviceType);
		this.ssdp.behavior = this;
		this.ssdp.start();
//		this.mdns = new Zeroconf.Browser(mdnsServiceType);
//		this.mdns.behavior = this;
//		this.mdns.start();
		this.updates = {};
		this.ssdpHelpers = [];
 		this.wsServerStart();
 		
		this.serialNotifier = new Serial.Notifier();
		this.serialNotifier.behavior = this;
		this.serialNotifier.start();
	}
	createDevice(discovery) {
		if (!this.findDeviceByUUID(discovery.uuid)) {
			for (let Config of this.Configs) {
				if (Config.id == discovery.description.id)
					return new Config(this, discovery);
			}
		}
	}
	filterDevices(device) {
		let filteredDevices = this.filteredDevices = this.devices.filter(device => {
			let id = device.constructor.id;
			if (device instanceof PlatformConfig)
				return this.platformFlags[id];
			if (device instanceof SimulatorConfig)
				return this.simulatorFlags[id];
			if (device instanceof SerialConfig)
				return this.serialFlags[device.configID];
			if (device.simulatorConfig)
				return this.simulatorFlags[device.simulatorConfig.constructor.id];
			if (id.endsWith(".factory"))
				id = id.slice(0, -8);
			return this.discoveryFlags[id];
		});
		shell.distribute("onDevicesChanged", filteredDevices);
		if (!device || (filteredDevices.indexOf(device) < 0)) {
			device = this.currentDevice;
			if (!device || (filteredDevices.indexOf(device) < 0)) {
				if (filteredDevices.length)
					device = filteredDevices[0];
				else
					device = null;
			}
		}
		if (this.currentDevice != device)
			this.selectDevice(device);
	}
	findDeviceByAddress(address) {
		let ip = address.slice(0, address.lastIndexOf(":"));
		return this.devices.find(device => device.hasIP(ip));
	}
	findDeviceByAddressAndTag(address, tag) {
		let ip = address.slice(0, address.lastIndexOf(":"));
		tag = (tag == "CreateShell") ? "Create" : (tag == "ElementShell") ? "Element" : null
		return this.devices.find(device => device.hasIP(ip) && (!tag || (device.constructor.tag == tag)));
	}
	findDeviceByName(name) {
		return this.devices.find(device => device.zeroconf && (device.name == name));
	}
	findDeviceByUUID(uuid) {
		return this.devices.find(device => device.uuid == uuid);
	}
	idle(now) {
		// ping
	}
	networkInterfaceIndexToName(index) {
		let name = this.networkInterface[index];
		if (!name)
			name = this.networkInterface[index] = system.networkInterfaceIndexToName(index);
		return name;
	}
	notify() {
		this.dirty = !((model.currentFeature == model.devicesFeature)
			|| (model.currentFeature == model.filesFeature)
			|| (model.currentFeature == model.debugFeature)
			|| (model.currentFeature == model.samplesFeature));
		shell.distribute("onFeatureChanged", this);
	}
	read(json) {
		if ("authorizations" in json)
			this.authorizations = json.authorizations;
		if ("discoveryFlags" in json)
			this.discoveryFlags = json.discoveryFlags;
		this.DiscoveryConfigs.forEach(Config => {
			let id = Config.id;
			if (!(id in this.discoveryFlags))
				 this.discoveryFlags[id] = Config.preferences.discoveryFlag;
		});
		if ("platformFlags" in json)
			this.platformFlags = json.platformFlags;
		this.PlatformConfigs.forEach(Config => {
			let id = Config.id;
			if (!(id in this.platformFlags))
				 this.platformFlags[id] = Config.preferences.platformFlag;
		});
		if ("serialFlags" in json)
			this.serialFlags = json.serialFlags;
		this.SerialConfigs.forEach(Config => {
			let id = Config.id;
			if (!(id in this.serialFlags))
				 this.serialFlags[id] = Config.preferences.serialFlag;
		});
		if ("simulatorFiles" in json)
			this.simulatorFiles = json.simulatorFiles;
		if ("simulatorFlags" in json)
			this.simulatorFlags = json.simulatorFlags;
		this.SimulatorConfigs.forEach(Config => {
			let id = Config.id;
			if (!(id in this.simulatorFiles))
				 this.simulatorFiles[id] = Config.defaultURL;
			if (!(id in this.simulatorFlags))
				 this.simulatorFlags[id] = Config.preferences.simulatorFlag;
		});
		this.devices.forEach(device => device.read(json));
		this.filterDevices();
	}
	selectDevice(device) {
		this.currentDevice = device;
		if (device) {
			device.authorize();
			if ("uuid" in device)
				model.deviceUUID = device.uuid;
		}
		shell.distribute("onDeviceSelected", device);
		model.debugFeature.selectMachineByDevice(device);
	}
	write(json) {
		json.authorizations = this.authorizations;
		json.discoveryFlags = this.discoveryFlags;
		json.platformFlags = this.platformFlags;
		json.serialFlags = this.serialFlags;
		json.simulatorFiles = this.simulatorFiles;
		json.simulatorFlags = this.simulatorFlags;
		this.devices.forEach(device => device.write(json));
	}
	
	onDeviceDown(device, interfaceName) {
		if (!device) return;
		trace("onDeviceDown " + device.name + " " + interfaceName + "\n");
		let devices = this.devices;
		if (device.removeInterface(interfaceName)) {
			let index = devices.findIndex(it => it.uuid == device.uuid);
			if (index < 0) return
			if (device.softwareStatus.updating || device.systemStatus.updating) return;
			let simulator;
			if (device.local) {
				if (device.constructor.tag == "Create")
					simulator = new CreateSimulatorConfig();
				else if (device.constructor.tag == "Element")
					simulator = new ElementSimulatorConfig();
				else
					simulator = new EmbedSimulatorConfig();
			}
			if (simulator) {
				let currentIndex = devices.indexOf(this.currentDevice);
				devices[index] = simulator;
				this.filterDevices((currentIndex == index) ? simulator : null);
			}
			else {
				devices.splice(index, 1);
				this.filterDevices(null);
			}
			if (!interfaceName)
				this.ssdp.remove(device.uuid);
			this.notify();
		}
	}
	onDeviceUp(device, interfaceName) {
		if (!device) return;
		trace("onDeviceUp " + device.name + " " + interfaceName + "\n");
		let devices = this.devices;
		if (device.addInterface(interfaceName)) {
			device.local = model.interfaces[interfaceName].ip == device.ip;
			let currentIndex = devices.indexOf(this.currentDevice);
			let index = -1;
			if (device.local) {
				device.ip = undefined;
				if (device.constructor.tag == "Create")
					index = devices.findIndex(device => (device.constructor.tag == "CreateShell") && device.constructor.preferences.simulatorFlag);
				else if (device.constructor.tag == "Element")
					index = devices.findIndex(device => (device.constructor.tag == "ElementShell") && device.constructor.preferences.simulatorFlag);
				else
					index = devices.findIndex(device => (device.constructor.tag == "EmbedShell") && device.constructor.preferences.simulatorFlag);
				if (index >= 0) {
					device.simulatorConfig = devices[index];
					devices[index] = device;
				}
				else {
					index = devices.length;
					devices.push(device);
				}
				this.filterDevices((index >= 0) ? device : null);
			}
			else {
				devices.push(device);
				this.filterDevices((model.deviceUUID == device.uuid) ? device : null);
			}
			this.notify();
		}
	}
	onHTTPTaskError(uuid) {
		trace("onHTTPTaskError " + uuid + "\n");
		let device = this.findDeviceByUUID(uuid);
		if (!device) return;
		system.alert({ 
			type:"stop",
			prompt:"Device \"" + device.name + "\" does not seem to be available anymore.",
			info:"Do you want to remove it from the list?",
			buttons:["OK", "Cancel"]
		}, ok => {
			if (ok) {
				this.ssdp.remove(uuid);
			}
		});
	}
	onSSDPServerUp(discovery) {
		trace("onSSDPServerUp " + JSON.stringify(discovery) + "\n");
		let device = this.findDeviceByUUID(discovery.uuid);
		if (discovery.type == ssdpDeviceType) {
			if (device)
				this.onDeviceUp(device, discovery.interfaceName);
			else {
				let message = new Message(mergeURI(discovery.url, "/description"));
				message.setRequestHeader("Connection", "Close");
				message.invoke(Message.JSON).then(json => {
					if (!json) {
						debugger
						return;
					}
					if (("uuid" in json) && (json.uuid != discovery.uuid)) {
						// Create and Element studio server in Simulator have same url
						return;
					}
					device = this.findDeviceByUUID(discovery.uuid);
					if (device)
						this.onDeviceUp(device, discovery.interfaceName);
					else {
						discovery.description = json;
						let split = parseURI(discovery.url).authority.split(":");
						discovery.ip = split[0];
						discovery.port = split[1];
						if (discovery.description.id == "com.marvell.kinoma.launcher.element") {
							if (discovery.description.xsedit) {
								discovery.url = "http://" + discovery.ip + ":8081";
								discovery.port = 8081;
								if (!discovery.description.hostname) {
									this.onSSDPServerUp(discovery);
									return;
								}
							}
							else if ("studio" in discovery.description)
								discovery.description.id = discovery.description.id + ".factory"; // upgrade factory devices
						}
						if (discovery)
							this.onDeviceUp(this.createDevice(discovery), discovery.interfaceName);
					}
				},
				message => {
					debugger
				});
			}
		}
	}
	onSSDPServerDown(discovery) {
		trace("onSSDPServerDown " + JSON.stringify(discovery) + "\n");
		if (discovery.type == ssdpDeviceType) {
			let device = this.findDeviceByUUID(discovery.uuid);
			this.onDeviceDown(device, discovery.interfaceName);
		}
	}
	onZeroconfServiceUp(service) {
		trace("onZeroconfServiceUp " + JSON.stringify(service) + " " + this.networkInterfaceIndexToName(service.interfaceIndex) + "\n");
		if (service.type == mdnsServiceType) {
			let interfaceName = this.networkInterfaceIndexToName(service.interfaceIndex);
			let uuid = service.txt.uuid;
			let device = this.findDeviceByUUID(uuid);
			if (device) {
				device.zeroconf = true;
				this.onDeviceUp(device, interfaceName);
			}
			else {
				let url = "http://" + service.ip + ":" + service.port;
				let message = new Message(mergeURI(url, "/description"));
				message.setRequestHeader("Connection", "Close");
				let promise = message.invoke(Message.JSON);
				promise.then(json => {
					if (!json) {
						debugger
						return;
					}
					device = this.findDeviceByUUID(uuid);
					if (device) {
						device.zeroconf = true;
						this.onDeviceUp(device, interfaceName);
					}
					else {
						service.description = json;
						service.url = url;
						service.uuid = uuid;
						this.onDeviceUp(this.createDevice(service), interfaceName);
					}
				},
				message => {
					debugger
				});
			}
		}
	}
	onZeroconfServiceDown(service) {
		trace("onZeroconfServiceDown " + JSON.stringify(service) + " " + this.networkInterfaceIndexToName(service.interfaceIndex) + "\n");
		if (service.type == mdnsServiceType) {
			this.onDeviceDown(this.findDeviceByName(service.name), this.networkInterfaceIndexToName(service.interfaceIndex));
		}
	}
	
	onSerialRegistered(discovery) {
		trace("onSerialRegistered: " + JSON.stringify(discovery) + "\n");
		for (let Config of this.Configs) {
			if ("serial" in Config) {
				let description = Config.serial.description;
				if ((discovery.vendor == description.vendor) && (discovery.product == description.product)) {
					let devices = this.devices;
					let device = new SerialConfig(this, discovery, Config);
					devices.push(device);
					this.filterDevices();
					this.notify();
				}
			}
		}
		
	}
	onSerialUnregistered(discovery) {
		trace("onSerialUnregistered: " + JSON.stringify(discovery) + "\n");
		let devices = this.devices;
		let currentIndex = devices.indexOf(this.currentDevice);
		let index = devices.findIndex(device => ("discovery" in device) && (device.discovery.path == discovery.path));
		if (index < 0) return
		let device = devices[index];
		device.close();
		devices.splice(index, 1);
		this.filterDevices();
	}
	
	onInvoke(hanlder, message) {
		message.status = 404;
		if (message.path == "/xsedit") {
			let query = parseQuery(message.query);
			let device = this.devices.find(device => {
				if (!device.running) return false;
				let ips = device.ip ? [ device.ip ] : device.interfaces.map(name => model.interfaces[name].ip);
				return (ips.find(ip => (message.remoteIP == ip))) && (device.helperID == query.id);
			});
			if (device) {
				message.setResponseHeader("Content-Type", "application/text");
				message.responseText = device.debugHost + ":" + model.devicesFeature.wsServer.port;
				message.status = 200;
			}
		}
	}
	// websocket
	wsServerStart() {
		let context = this;
		let wsServer = this.wsServer = new WebSocketServer();
		wsServer.context = context;
		wsServer.connections = [];
		wsServer.onlaunch = function() {
//  			trace("WSS: Launched\n");
		}
		wsServer.ondisconnecting = function(ws, options) {
//			trace("WSS: ondisconnecting\n");
		}
		wsServer.ondisconnect = function(ws, options) {
//			trace("WSS: ondisconnect\n");
			this.connections = this.connections.filter(item => item !== ws);
		}
		wsServer.onerror = function(e) {
//			trace("WSS: onerror\n");
			debugger
		}
		wsServer.onconnect = function(ws, options) {
			this.open = true;
//			trace("WSS: onconnect\n");
			this.connections.push(ws);
			ws.context = wsServer.context;
			ws.device = undefined;
			ws.queue = [];

			ws.onopen = function() {
				this.wsIndex = wsIndex++;
//  				trace("WS: onopen " + this.wsIndex + "\n");
			}
			ws.onclose = function() {
//  				trace("WS: onclose " + this.wsIndex + " " + this.readyState + "\n");
				if ("device" in this)
					this.device.wsClose();
			}
			ws.onerror = function(error) {
				if ("device" in this)
					context.onHTTPTaskError(this.device.uuid);
//  				trace("WS: onerror " + this.wsIndex + " -> " + error.code + " " + error.message + "\n");
			}
			ws.onmessage = function(message) {
//  				trace("WS: onmessage " + this.wsIndex + ": " + message.data + "\n");
				if (this.readyState != WebSocket.OPEN) return;
				if (!this.device) {
					let device = this.device = this.context.findDeviceByUUID(message.data);
					device.ws = this;
					this.device = device;
					shell.distribute("onDeviceHelperUp");
				}
				else {
					let json = message.data.length ? JSON.parse(message.data) : undefined;
					let current = this.queue.shift();
					if (json) {
						switch (json.status) {
						case 200:
						case 204:
							current.resolve(json.content);
							break;
						default:
							current.reject(json);
							break;
						}
					}
					else
						current.reject();
					if (this.queue.length) {
						current = this.queue[0];
//  						trace("wsSend from queue: " + JSON.stringify(json) + "\n");
						this.send(JSON.stringify(current.json));
					}
				}
			}
		};
	}
	wsServerStop() {
		if ("wsServer" in this) {
//  			trace("WSS: Stop\n");
			this.wsServer.connections.forEach(ws => {
				if ("device" in ws)
					ws.device.wsClose();
				else
					ws.close();
			});
			delete this.wsServer;
		}
	}
}

class DeviceViewer extends Viewer {
	constructor(feature) {
		super(feature);
		this.scheme = "device";
	}
	accept(parts, url) {
		return parts.scheme == this.scheme;
	}
	create(parts, url) {
		let uuid = parts.authority;
		let device = this.feature.findDeviceByUUID(uuid);
		if (device) {
			let app = device.constructor.apps[parts.name];
			return new app.View({ device });
		}
		return new ErrorView({ url, error:"Device not found!" });
	}
};

export class DeviceConfig {
	constructor(devices, discovery) {
		this.uuid = discovery.uuid;
//		this.url = discovery.url;
//		this.toolURL = this.url;
		this.ip = discovery.ip;
		this.port = discovery.port;
		this.local = false;
		this.description = discovery.description;
		this.selected = false;
		this.update = null;
		this.currentProject = null;
		this.projectID = null;
		this.helperURL = null;
		this.softwareStatus = {
			"updating": false,
		};
		this.systemStatus = {
			"updating": false,
		};
		this.authorization = (discovery.uuid in devices.authorizations) ? devices.authorizations[discovery.uuid] : encodeBase64("kinoma:kinoma");
		this.setupDeviceUpdateInfo(devices);
		this.zeroconf = "host" in discovery;
		
		this.authorized = !this.description.locked;
		this.authorizing = false;
		this.machineCount = 0;
		this.running = true;
		this.simulatorConfig = null;
		this.ButtonTemplate = DeviceMenuButton;
		this.ItemTemplate = DeviceMenuItemLine;
		this.interfaces = [ ];
	}
	addInterface(name) {
		let interfaces = this.interfaces;
		let length = interfaces.length;
		if (interfaces.indexOf(name) < 0)
			interfaces.push(name);
		trace("addInterface: " + this.uuid + " " + name + " -> " + interfaces.length + "\n");
		return !length && (interfaces.length == 1);
	}
	removeInterface(name) {
		let interfaces = this.interfaces;
		let index = interfaces.indexOf(name);
		if (index >= 0)
			interfaces.splice(index, 1);
		trace("removeInterface: " + this.uuid + " " + name + " -> " + interfaces.length + "\n");
		return interfaces.length == 0;
	}
	authorize() {
		if (!this.authorized) {
			this.authorizing = true;
			let message = this.newPingMessage();
			message.invoke(Message.TEXT).then(text => {
				this.authorized = message.status != 401;
				this.authorizing = false;
				shell.distribute("onDeviceSelected", this);
			});
		}
	}
	checkProject(project) {
		return true;
	}
	newStudioMessage(path, query) {
		if (query)
			path = path + "?" + serializeQuery(query);
		let message = new Message(mergeURI(this.toolURL, path));
		message.setRequestHeader("Connection", "Close");
		if (this.authorization != null)
			message.setRequestHeader("Authorization", "Basic " + this.authorization);
		return message;
	}
//	newXSEditMessage(path, query) {
//		if (!this.helperURL) return;
//		if (query)
//			path = path + "?" + serializeQuery(query);
//		let message = new Message(mergeURI(this.helperURL, path));
//		message.setRequestHeader("Connection", "Close");
//		if (this.authorization != null)
//			message.setRequestHeader("Authorization", "Basic " + this.authorization);
//		return message;
//	}
	get currentIP() {
		return this.ip ? this.ip : model.interfaces[this.interfaces[0]].ip;
	}
	get helperID() {
		debugger
	}
	get helperProject() {
	}
	getUpdateInfo() {
	}
	get debugHost() {
		let iface = model.interfaces[this.interfaces[0]];
		if (iface)
			return iface.ip;
	}
	get name() {
		return this.description.name;
	}
	set name(name) {
		this.description.name = name;
	}
	get product() {
		if (this.isSimulator())
			return this.constructor.product + " Simulator";
		else
			return this.constructor.product;
	}
	get softwareVersion() {
		return this.description.version;
	}
	get softwareUpdateTarget() {
	}
	get softwareUpdateVersion() {
		let target = this.softwareUpdateTarget;
		return (this.update && (target in this.update)) ? this.update[target][0].version : undefined;
	}
	get softwareUpdate() {
		let current = this.softwareVersion;
		let update = this.softwareUpdateVersion;
		let c = current[0];
		let isDigit = (c >= '0' && c <= '9');
		return (current && update && isDigit) ? current != update : false;
	}
	get systemVersion() {
		return this.description.firmware;
	}
	get systemUpdateTarget() {
	}
	get systemUpdateVersion() {
		let target = this.systemUpdateTarget;
		return (target && this.update && (target in this.update)) ? this.update[target][0].version : undefined;
	}
	get systemUpdate() {
		let current = this.systemVersion;
		let update = this.systemUpdateVersion;
		let c = current[0];
		let isDigit = (c >= '0' && c <= '9');
		return (current && update && isDigit) ? current != update : false;
	}
	get toolURL() {
		return this.url;
	}
	get type() {
		debugger
	}
	get url() {
		return "http://" + (this.ip ? this.ip : model.interfaces[this.interfaces[0]].ip) + ":" + this.port + "/";
	}
	hasIP(ip) {
		return this.ip ? this.ip == ip : this.interfaces.findIndex(name => (name in model.interfaces) && (model.interfaces[name].ip == ip)) >= 0;
	}
	isSimulator() {
		return false;
	}
	read(json) {
	}
	run(project) {
		this.currentProject
	}
	setupDeviceUpdateInfo(feature) {
		let configName = this.constructor.tag;
		if (configName in feature.updates) {
			this.setUpdateInfo(feature.updates[configName]);
		}
		else {
			let update = this.getUpdateInfo();
			if (update) {
				update.then(result => {
					feature.updates[configName] = result;
					for (let device of feature.devices) {
						if (configName == device.constructor.tag) {
							device.setUpdateInfo(result);
							if (this == device)
								shell.distribute("onDeviceChanged", device);
						}
					}
				});
			}
		}
	}
	setUpdateInfo(update) {
		this.update = update;
	}
	// UPDATES
	updateAlert(data, success, info) {
		system.alert({
			type:"stop",
			prompt:"The update of \"" + this.name + "\" to " + data.title + " " + this.softwareUpdateVersion + (success ? " succeeded!" : " failed!"),
			info,
			buttons:["OK"]
		}, ok => {
			if (ok) {
			}
		});
	}
	updateSoftware() {
		debugger
	}
	updateSoftwareError(data) {
		this.softwareStatus = {
			"updating": false,
		};
	}
	updateSoftwareFinished(data) {
		this.softwareStatus = {
			"updating": false,
		};
	}
	updateSystem() {
		debugger
	}
	updateSystemError(data) {
		this.systemStatus = {
			"updating": false,
		};
	}
	updateSystemFinished(data) {
		this.systemStatus = {
			"updating": false,
		};
	}
	write(json) {
	}
	// websocket
	wsClose() {
		if (this.ws) {
			let ws = this.ws;
			ws.queue.forEach(promise => {
// 				trace("WC " + ws.wsIndex + " " + ws.readyState + ": cancel " + JSON.stringify(promise.json) + "\n");
				promise.reject({ status:500, reason:"Internal Server Error" });
			});
			shell.distribute("onDeviceHelperDown", this);
			this.ws.close();
			delete ws.device;
			delete this.ws;
		}
	}
	wsRequest(json) {
		let ws = this.ws;
		if (!ws || (ws.readyState != WebSocket.OPEN))
			return new Promise((resolve, reject) => reject({ status:500, reason:"Internal Server Error" }));
		return new Promise((resolve, reject) => {
			let length = ws.queue.push({ resolve, reject, json });
			if (length == 1) {
// 				trace("wsSend " + ws.wsIndex + ": " + JSON.stringify(json) + "\n");
				ws.send(JSON.stringify(json));
			}
		});
	}
	getNetworkLevel() {
		if (this.ws)
			return this.wsRequest({
				handler: "getNetworkLevel",
			}).then(level => {
				this.level = level;
				return Promise.resolve(this.level);
			});
		else
			return Promise.resolve(this.level);
	}
	getNetworkList() {
		return this.wsRequest({
			handler: "getNetworkList",
		});
	}	
	getNetworkMAC() {
		if (this.MAC)
			return Promise.resolve(this.MAC);
		else {
			return this.wsRequest({
				handler: "getNetworkMAC",
			}).then(mac => {
				this.MAC = mac;
				return Promise.resolve(this.MAC);
			});
		}
	}
	getNetworkSSID() {
		return Promise.resolve(model.SSID);
	}
	getTimezone() {
		return this.wsRequest({
			handler: "getTimezone",
		});
	}
	quitApplication() {
		tool.abort(this, model.debugFeature);
	}
	setName(name) {
		this.name = name;
		return this.wsRequest({
			handler: "setName",
			name
		});
	}
	setTimezone(timezone) {
		return this.wsRequest({
			handler: "setTimezone",
			timezone: timezone
		});
	}
	updateSoftwareStatus() {
		return this.wsRequest({
			handler: "updateSoftwareStatus",
		});
	}
	updateSystemStatus() {
		return this.wsRequest({
			handler: "updateSystemStatus",
		});
	}
	getLogicalToPhysicalMapJson() {
		return this.wsRequest({
			handler: "getLogicalToPhysicalMapJson",
		});
	}
}

class PlatformConfig {
	constructor() {
		this.authorized = true;
		this.authorizing = false;
		this.running = true;
		this.ip = "";
		this.local = true;
		this.machineCount = 0;
		this.ButtonTemplate = PlatformMenuButton;
		this.ItemTemplate = PlatformMenuItemLine;
		this.uuid = system.getUUID(this.constructor.tag);
		this.options = {
			debug: true,
			instrumentation: true,
			memory: false,
			xs: false,
			platform:"mac",
		};
		this.output = {
			directory:mergeURI(Files.documentsDirectory, "Kinoma%20Code/Applications/"),
		};
	}
	authorize() {
	}
	get name() {
		return this.constructor.product;
	}
	hasIP(ip) {
		return false;
	}
	read(json) {
		let id = this.constructor.id;
		if (id in json) {
			let preferences = json[id];
			if ("options" in preferences)
				this.options = preferences.options;
			if ("output" in preferences)
				this.output = preferences.output;
		}
	}
	write(json) {
		json[this.constructor.id] = {
			options: this.options,
			output: this.output,
		}
	}
}

import {
	MacOSTile,
	MacOSView,
} from "features/devices/apps/platforms/macOS";

class MacOSPlatformConfig extends PlatformConfig {
}

MacOSPlatformConfig.iconSkin = new Skin({ texture:new Texture("./assets/macOS.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
MacOSPlatformConfig.id = "com.marvell.kinoma.platform.mac";
MacOSPlatformConfig.product = "Mac OS";
MacOSPlatformConfig.tag = "mac";
MacOSPlatformConfig.apps = {
	macOS: {
		Tile: MacOSTile,
		View: MacOSView
	},
}
MacOSPlatformConfig.preferences = {
	platformFlag: false,
}

import {
	iOSTile,
	iOSView,
} from "features/devices/apps/platforms/iOS";

class iOSPlatformConfig extends PlatformConfig {
}

iOSPlatformConfig.iconSkin = new Skin({ texture:new Texture("./assets/iOS.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
iOSPlatformConfig.id = "com.marvell.kinoma.platform.ios";
iOSPlatformConfig.product = "iOS";
iOSPlatformConfig.tag = "ios";
iOSPlatformConfig.apps = {
	iOS: {
		Tile: iOSTile,
		View: iOSView
	},
}
iOSPlatformConfig.preferences = {
	platformFlag: false,
}

import {
	AndroidTile,
	AndroidView,
} from "features/devices/apps/platforms/android";

class AndroidPlatformConfig extends PlatformConfig {
}

AndroidPlatformConfig.iconSkin = new Skin({ texture:new Texture("./assets/android.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
AndroidPlatformConfig.id = "com.marvell.kinoma.platform.android";
AndroidPlatformConfig.product = "Android";
AndroidPlatformConfig.tag = "android";
AndroidPlatformConfig.apps = {
	Android: {
		Tile: AndroidTile,
		View: AndroidView
	},
}
AndroidPlatformConfig.preferences = {
	platformFlag: false,
}

class SerialConfig {
	constructor(feature, discovery, Config) {
		this.authorized = true;
		this.authorizing = false;
		this.discovery = discovery;
		this.iconSkin = Config.iconSkin;
		this.configID = Config.id;
		this.ip = "";
		this.machineCount = 0;
		this.running = false;
		this.settings = Config.serial.settings;
		this.shortcuts =  Config.serial.shortcuts;
		this.ButtonTemplate = SerialMenuButton;
		this.ItemTemplate = SerialMenuItemLine;
		this.serialDevice = null;
	}
	authorize() {
	}
	close() {
		let serialDevice = this.serialDevice;
		if (serialDevice) {
			serialDevice.close();
			this.serialDevice = null;
		}
	}
	execute(command) {
		let serialDevice = this.serialDevice;
		if (serialDevice)
			serialDevice.write(command + "\n");
	}
	open() {
		let serialDevice = this.serialDevice;
		if (!serialDevice) {
			let settings = this.settings;
			serialDevice = this.serialDevice = new Serial.Device(this.discovery.path);
			serialDevice.behavior = this;
			serialDevice.open(settings.baud, settings.bits, settings.parity, settings.stop);
		}
	}
	get name() {
		return this.discovery.path;
	}
	get product() {
		return this.settings.name; 
	}
	hasIP(ip) {
		return false;
	}
	onSerialData(string) {
		model.doLogRaw(shell, string);
	}
	read(json) {
	}
	write(json) {
	}
}

class SimulatorConfig {
	constructor() {
		this.authorized = true;
		this.authorizing = false;
		this.ip = "";
		this.machineCount = 0;
		this.name = this.constructor.product + " Simulator";
		this.running = false;
		this.ButtonTemplate = SimulatorMenuButton;
		this.ItemTemplate = SimulatorMenuItemLine;
	}
	authorize() {
	}
	hasIP(ip) {
		return false;
	}
	launch() {
		let constructor = this.constructor;
		let id = constructor.id;
		let url = mergeURI(model.simulatorsURI, model.devicesFeature.simulatorFiles[id]);
		if (!url || !Files.exists(url)) {
			system.alert({ 
				type:"stop", 
				prompt:"Simulator not found.", 
				info:"Do you want to use the default simulator?", 
				buttons:["Locate", "Cancel", "Use Default"]
			}, ok => {
				if (ok === undefined)
					return;
				if (ok) {
					var dictionary = { message:"Locate " + constructor.defaultURL, prompt:"Open", url:mergeURI(shell.url, "../../../../../") };
					system.openFile(dictionary, url => { 
						if (url) {
							model.devicesFeature.simulatorFiles[id] = url;
							shell.distribute("onSimulatorFilesChanged", id, url);
							launchURI(url);
						}
					});
				}
				else {
					let url = constructor.defaultURL;
					model.devicesFeature.simulatorFiles[id] = url;
					shell.distribute("onSimulatorFilesChanged", id, url);
					launchURI(mergeURI(model.simulatorsURI, url));
				}
			});
		}
		else
			launchURI(url);
	}
	read(json) {
	}
	write(json) {
	}
	static get defaultURL() {
		if (system.platform == "mac")
			return this.tag + ".app";
		else if (system.platform == "win")
			return this.tag + "/" + this.tag + ".exe";
		else
			return this.tag + "/" + this.tag;
	}
}

class CreateSimulatorConfig extends SimulatorConfig {
	constructor() {
		super();
	}
}

CreateSimulatorConfig.iconSkin = new Skin({ texture:new Texture("./assets/create-simulator.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
CreateSimulatorConfig.id = "com.marvell.kinoma.launcher.create";
CreateSimulatorConfig.preferences = { simulatorFlag: true }
CreateSimulatorConfig.product = "Kinoma Create";
CreateSimulatorConfig.tag = "CreateShell";

class ElementSimulatorConfig extends SimulatorConfig {
	constructor() {
		super();
	}
}

ElementSimulatorConfig.iconSkin = new Skin({ texture:new Texture("./assets/element-simulator.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
ElementSimulatorConfig.id = "com.marvell.kinoma.launcher.element";
ElementSimulatorConfig.preferences = { simulatorFlag: (system.platform == "mac") }
ElementSimulatorConfig.product = "Kinoma Element";
ElementSimulatorConfig.tag = "ElementShell";

class EmbedSimulatorConfig extends SimulatorConfig {
	constructor() {
		super();
	}
}

EmbedSimulatorConfig.iconSkin = new Skin({ texture:new Texture("./assets/embed-simulator.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
EmbedSimulatorConfig.id = "com.marvell.kinoma.launcher.embed";
EmbedSimulatorConfig.preferences = { simulatorFlag: true }
EmbedSimulatorConfig.product = "Kinoma Embed";
EmbedSimulatorConfig.tag = "EmbedShell";

// ASSETS

import { 
	dialogBoxSkin,
	menuIconSkin,
} from "common/menu";

import {
	BLACK,
	FIXED_FONT,
	LIGHT_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	WHITE,
	featureEmptyStyle,
	grayBorderSkin,
	grayHeaderSkin,
	orangeHeaderSkin,
	menuHeaderStyle,
	menuLineSkin,
	menuLineStyle,
	tableHeaderStyle,
	blackButtonSkin,
	blackButtonStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "shell/assets";

import {
	fieldHintStyle,
	fieldLabelSkin,
	fieldLabelStyle,
	fieldScrollerSkin,
	deviceAddressSkin,
	deviceDebugSkin,
} from "features/devices/assets";

var menuHeaderTitleStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"left" })
var menuLineTitleStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"left" })

var menuHeaderAddressStyle = new Style({ font:LIGHT_FONT, size:12, color:WHITE, horizontal:"right", right:5 })
var menuLineAddressStyle = new Style({ font:LIGHT_FONT, size:12, color:BLACK, horizontal:"right", right:5 })
var menuHeaderVariantStyle = new Style({ font:NORMAL_FONT, size:12, color:WHITE, horizontal:"left" })
var menuLineVariantStyle = new Style({ font:NORMAL_FONT, size:12, color:BLACK, horizontal:"left" })

// var menuHeaderAddressStyle = new Style({ font:NORMAL_FONT, size:12, color:WHITE, horizontal:"left" })
// var menuLineAddressStyle = new Style({ font:NORMAL_FONT, size:12, color:BLACK, horizontal:"left", right:10 })
// var menuHeaderVariantStyle = new Style({ font:LIGHT_FONT, size:12, color:WHITE, horizontal:"left" })
// var menuLineVariantStyle = new Style({ font:LIGHT_FONT, size:12, color:BLACK, horizontal:"left" })

var textSkin = new Skin({ fill:["transparent", "transparent", "#e0e0e0", "#cbe1fa"] })
var textStyle = new Style({ 
	font:FIXED_FONT,
	size:12, 
	horizontal:"left",
	left:8, right:8,
	color: [ "black", "#103ffb", "#b22821", "#008d32" ]
})

// BEHAVIORS

import { 
	MenuButtonBehavior,
	MenuItemBehavior,
} from "common/menu";

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import { 
	CodeBehavior, 
	CodeScrollerBehavior, 
	LineBehavior,
} from "shell/behaviors";

import { 
	FeaturePaneBehavior,
} from "shell/feature";


class DevicePaneBehavior extends FeaturePaneBehavior {
	onDeviceSelected(container, device) {
		let content;
		if (device) {
			if (device instanceof SerialConfig)
				content = new SerialDeviceContainer(device);
			else if (device.authorized && device.running)
				content = new DeviceScroller(device);
			else
				content = new NoDevicesContainer();
		}
		else
			content = new NoDevicesContainer();
		container.replace(container.last, content);
	}
	onDisplaying(container) {
		let data = this.data;
		container.distribute("onDeviceSelected", data.currentDevice);
		container.distribute("onMachinesChanged", model.debugFeature.machines, model.debugFeature.debuggees);
		container.distribute("onMachineSelected", model.debugFeature.currentMachine);
		container.distribute("onURLChanged", data.model.url);
	}
};

class DeviceHeaderBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDeviceSelected(container, device) {
		let content, tools;
		if (device) {
			if (device instanceof SerialConfig)
				tools = new DeviceCommandLine(device);
			else if (device.authorized)
				tools = new DebugToolsHeader(model.debugFeature);
			else if (device.authorizing)
				tools = new DeviceConnectingLine(this.data);
			else
				tools = new DevicePasswordLine(this.data);
		}
		else
			tools = new DebugToolsHeader(model.debugFeature);
		container.replace(container.last, tools);
	}
	onEnter(container) {
		let data = this.data;
		let device = data.currentDevice;
		device.authorization = encodeBase64("kinoma:" + data.PASSWORD.string);
		data.authorizations[device.uuid] = device.authorization;
		device.authorize();
		shell.distribute("onDeviceSelected", device);
	}
}

class DeviceCommandLineBehavior extends Behavior {
	onCreate(container, device) {
		this.device = device;
	}
	onDisplayed(container) {
		this.device.open();
	}
	onEnter(container) {
		let device = this.device;
		let label = container.first.first;
		device.execute(label.string);
		label.string = "";
		label.next.visible = true;
		return true;
	}
	onKeyDown(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 3) || (c == 13));
			return true;
		return false;
	}
	onKeyUp(container, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 3) || (c == 13)) {
			this.onEnter(container);
			return true;
		}
		return false;
	}
	onUndisplayed(container) {
		this.device.close();
	}
}

class DeviceMenuHeaderBehavior extends LineBehavior {
	onDescribeMenu(container) {
		let data = this.data;
		return {
			ItemTemplate: DeviceMenuItemLine,
			items: data.filteredDevices,
			selection: data.filteredDevices.findIndex(device => data.currentDevice == device),
			context: shell,
		};
	}
	onDisplaying(container) {
		let data = this.data;
		this.arrow = true;
		this.onDevicesChanged(container, data.filteredDevices);
	}
	onDevicesChanged(container, devices) {
 		container.active = devices.length > 1;
 		container.last.visible = this.arrow && container.active;
	}
	onDeviceSelected(container, device) {
		if (device)
			container.replace(container.first, new device.ButtonTemplate(device));
		else
			container.replace(container.first, new NoDevicesButton(null));
	}
	onLaunch(container, device) {
 		device.launch();
	}
	onMenuCanceled(container, selection) {
 		container.last.visible = this.arrow = true;
	}
	onMenuSelected(container, selection) {
 		this.selecting = false;
		container.last.visible = this.arrow = true;
		this.data.selectDevice(selection);
	}
//	onMenuEmpty(container) {
//		container.bubble("doSearch");
//	}
	onTap(container) {
		let data = this.onDescribeMenu();
		data.button = container;
		data.context.add(new DeviceMenuDialog(data));
 		container.last.visible = this.arrow = false;
	}
}

const tileWidth = 120;
const tileFiller = 10;

class DeviceLayoutBehavior extends Behavior {
	onCreate(layout, device) {
		let constructor = device.constructor;
		let apps = constructor.apps;
		for (let name in apps)
			if (!("Test" in apps[name]) || (apps[name].Test(device)))
			layout.add(new apps[name].Tile({ device, url:"device://" + device.uuid + "/" + name }));
		model.filesFeature.projects.items.forEach(project => {
			if (device instanceof PlatformConfig) {
				if (project.standalone && (device.constructor.tag in project.standalone.platforms))
					layout.add(new ProjectTile({ device, project }));
			}
			else if (project[constructor.tag])
				layout.add(new ProjectTile({ device, project }));
		});
	}
	onMeasureHorizontally(layout) {
		let total = layout.container.container.container.width - 20;
		let c = this.tileRowCount = Math.floor((total + tileFiller) / (tileWidth + tileFiller));
		let width = this.tileWidth = Math.floor((total + tileFiller) / c) - tileFiller;
		let content = layout.first;
		while (content) {
			let coordinates = content.coordinates;
			coordinates.width = width;
			content.coordinates = coordinates;
			content = content.next;
		}
		return total;
	}
	onMeasureVertically(layout) {
		let c = this.tileRowCount;
		let width = this.tileWidth;
		let xs = new Array(c).fill();
		let ys = new Array(c).fill();
		let coordinates = {left:0, width, top:0, height: 0};
		for (let i = 0; i < c; i++) {
			xs[i] = (i * (width + tileFiller));
			ys[i] = 0;
		}
		let content = layout.first;
		while (content) {
			let min = 0x7FFFFFFF;
			let j = 0;
			for (let i = 0; i < c; i++) {
				let y = ys[i];
				if (y < min) {
					min = y;
					j = i;
				}
			}
			min += tileFiller;
			coordinates.left = xs[j];
			coordinates.top = min;
			coordinates.height = content.measure().height;
			content.coordinates = coordinates;
			ys[j] = min + coordinates.height;
			content = content.next;
		}
		var max = 0;
		for (var i = 0; i < c; i++) {
			var y = ys[i];
			if (y > max)
				max = y;
		}
		return max;
	}
}

import { 
	ButtonBehavior, 
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

// TEMPLATES

import {
	DebugToolsHeader,
} from "features/debug/debug"

export var DevicePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	Behavior: DevicePaneBehavior,
	contents: [
		DeviceHeader($, { }),
		Scroller($, {
			left:10, right:0, top:90, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				NoDevicesContainer($, {}),
				VerticalScrollbar($, { right:0 }),
			]
		}),
	],
}));

export var DeviceHeader = Container.template($ => ({
	left:0, right:0, top:0, height:90,
	Behavior: DeviceHeaderBehavior,
	contents: [
		DeviceMenuHeader(model.devicesFeature, { }),
		DebugToolsHeader(model.debugFeature, { }),
	],
}));

export var DeviceMenuHeader = Container.template($ => ({
	left:10, right:10, top:0, height:60, skin:grayHeaderSkin, active:true,
	Behavior: DeviceMenuHeaderBehavior,
	contents: [
		Content($, {}),
 		Content($, { width:20, right:5, height:20, bottom:5, skin:menuIconSkin }),
	]
}));

var NoDevicesButton = Line.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Picture($, {
					width:60, height:60, url:"../devices/assets/searching.png",
					Behavior: class extends Behavior {
						onCreate(picture) {
							picture.opacity = 0.66;
							picture.origin = { x:30, y:30 };
						}
						onDisplayed(picture) {
							picture.start();
						}
						onTimeChanged(picture) {
							var rotation = picture.rotation;
							rotation += 1;
							if (rotation > 360) rotation = 0;
							picture.rotation = rotation;
						}
					},
				}),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Label($, { left:0, right:0, style:tableHeaderStyle, string:"DEVICES" }),
				Label($, { left:0, right:0, style:menuHeaderVariantStyle, string:model.SSID }),
			],
		}),
	],
}));

var DeviceMenuButton = Line.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin, state:1  }),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { style:menuHeaderTitleStyle, string:$.name, }),
						Label($, { left:0, right:0, style:menuHeaderAddressStyle, string:$.ip ? $.ip : model.interfaces[$.interfaces[0]].ip, }),
						Content($, { width:20, height:20, skin:deviceAddressSkin, state:0, variant:$.simulatorConfig ? 1 : 0 }),
						Content($, { width:5 }),
					],
				}),
				Label($, { left:0, right:0, style:menuHeaderVariantStyle, string:$.product, }),
			],
		}),
	]
}));

var PlatformMenuButton = Line.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin, state:1  }),
			],
		}),
		Content($, { width:10 }),
		Label($, { left:0, right:0, style:menuHeaderVariantStyle, string:$.constructor.product }),
	]
}));

var SerialMenuButton = Line.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.iconSkin, state:1  }),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { left:0, right:0, style:menuHeaderTitleStyle, string:$.name, }),
						Content($, { width:20, height:20, skin:deviceAddressSkin, state:0, variant:2 }),
						Content($, { width:5 }),
					],
				}),
				Label($, { left:0, right:0, style:menuHeaderVariantStyle, string:$.product, }),
			],
		}),
	]
}));

var SimulatorMenuButton = Line.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:grayHeaderSkin,
	contents: [
		Container($, {
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin, state:1  }),
			],
		}),
		Content($, { width:10 }),
		Label($, { left:0, right:0, style:menuHeaderVariantStyle, string:$.name }),
		Container($, {
			width:80, skin:whiteButtonSkin, active:true, name:"onLaunch", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Launch" }),
			],
		}),
	]
}));

var DeviceMenuDialog = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior: class extends Behavior {
		onTouchEnded(container, id, x, y, ticks) {
			var layout = container.first;
			if (!layout.hit(x, y))
				layout.behavior.onCancel(layout);
		}
	},
	contents: [
		Layout($, {
			left:$.button.x - $.context.x,
			width:$.button.width,
			top:$.button.y - $.context.y + $.button.height, 
			skin: dialogBoxSkin,
			Behavior: class extends Behavior {
				onCancel(layout) {
					let data = this.data;
					let context = data.context;
					data.context.remove(this.data.context.last);
					this.data.button.delegate("onMenuCanceled", item);
				}
				onCreate(layout, data) {
					this.data = data;
					if (data.selection >= 0) {
						let column = layout.first.first;
						let content = column.content(data.selection);
						column.remove(content);
					}
				}
				onMeasureHorizontally(layout, width) {
					return width;
				}
				onMeasureVertically(layout, height) {
					let data = this.data;
					let context = data.context;
					let delta = (context.y + context.height) - (data.button.y + data.button.height) - 20;
					let size = layout.first.first.measure();
					return Math.min(size.height, delta);
				}
				onSelected(layout, item) {
					this.onCancel(layout, item);
					this.data.button.delegate("onMenuSelected", item);
				}
			},
			contents: [
				Scroller($, {
					left:0, right:0, top:0, bottom:0, clip: true, active:true,
					Behavior: ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							contents: $.items.map(item => {
								return new item.ItemTemplate(item);
							}),
						})
					]
				}),
			],
		}),
	],
}));

var DeviceMenuItemLine = Line.template($ => ({
	left:0, right:0, height:60, skin: menuLineSkin, active:true,
	Behavior: MenuItemBehavior,
	contents: [
		Container($, { 
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin }),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { style:menuLineTitleStyle, string:$.name}),
						Label($, { left:0, right:0, style:menuLineAddressStyle, string:$.ip ? $.ip : model.interfaces[$.interfaces[0]].ip}),
						Content($, { width:20, height:20, skin:deviceAddressSkin, state:1, variant:$.simulatorConfig ? 1 : 0 }),
						Content($, { width:5 }),
					],
				}),
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { left:0, right:0, height:20, style:menuLineVariantStyle, string:$.product}),
						Content($, { width:20, height:20, skin:deviceDebugSkin, state:1, visible:$.machineCount > 0 }),
						Content($, { width:5 }),
					],
				}),
			],
		}),
	]
}));

var PlatformMenuItemLine = Line.template($ => ({
	left:0, right:0, height:60, skin: menuLineSkin, active:true,
	Behavior: MenuItemBehavior,
	contents: [
		Container($, { 
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin }),
			],
		}),
		Content($, { width:10 }),
		Label($, { style:menuLineVariantStyle, string:$.constructor.product }),
	]
}));

var SerialMenuItemLine = Line.template($ => ({
	left:0, right:0, height:60, skin: menuLineSkin, active:true,
	Behavior: MenuItemBehavior,
	contents: [
		Container($, { 
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.iconSkin }),
			],
		}),
		Content($, { width:10 }),
		Column($, {
			left:0, right:0, height:40,
			contents: [
				Line($, {
					left:0, right:0, height:20,
					contents: [
						Label($, { left:0, right:0, style:menuLineTitleStyle, string:$.name}),
						Content($, { width:20, height:20, skin:deviceAddressSkin, state:1, variant:2 }),
						Content($, { width:5 }),
					],
				}),
				Label($, { left:0, right:0, style:menuLineVariantStyle, string:$.product, }),
			],
		}),
	]
}));

var SimulatorMenuItemLine = Line.template($ => ({
	left:0, right:0, height:60, skin: menuLineSkin, active:true,
	Behavior: MenuItemBehavior,
	contents: [
		Container($, { 
			width:60, height:60,
			contents: [
				Content($, { width:60, height:60, skin:$.constructor.iconSkin }),
			],
		}),
		Content($, { width:10 }),
		Label($, { style:menuLineVariantStyle, string:$.name }),
	]
}));

var DevicePasswordLine = Line.template($ => ({
	left:10, right:10, top:60, height:30, skin:grayBorderSkin,
	contents: [
		Scroller($, {
			left:5, right:5, top:5, bottom:5, skin: fieldScrollerSkin, clip:true, active:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"PASSWORD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:fieldLabelStyle, hidden:true, editable:true,
					Behavior: class extends FieldLabelBehavior {
						onDisplaying(label) {
							label.focus();
						}
						onEdited(label) {
							label.next.visible = label.string.length == 0;
						}
					},
				}),
				Label($, { left:0, top:2, bottom:2, style:fieldHintStyle, string:"Password" }),
			],
		}),
		Container($, {
			width:80, skin:blackButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Connect" }),
			],
		}),
	],
}));

var DeviceConnectingLine = Line.template($ => ({
	left:10, right:10, top:60, height:30, skin:grayBorderSkin,
	contents: [
		Label($, { left:0, right:0, style:fieldHintStyle, string:"Connecting..." }),
	],
}));

var DeviceCommandLine = Line.template($ => ({
	left:10, right:10, top:60, height:30, skin:grayBorderSkin,
	Behavior: DeviceCommandLineBehavior,
	contents: [
		Scroller($, {
			left:5, right:5, top:5, bottom:5, skin: fieldScrollerSkin, clip:true, active:true,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:fieldLabelStyle, editable:true,
					Behavior: class extends FieldLabelBehavior {
						onDisplaying(label) {
							label.focus();
						}
						onEdited(label) {
							label.next.visible = label.string.length == 0;
						}
					},
				}),
				Label($, { left:0, top:2, bottom:2, style:fieldHintStyle, string:"Command" }),
			],
		}),
		Container($, {
			width:80, skin:blackButtonSkin, active:true, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Run" }),
			],
		}),
	],
}));

var DeviceScroller = Container.template($ => ({
	left:10, right:0, top:90, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
	contents: [
		Layout($, { left:0, top:0, Behavior: DeviceLayoutBehavior }),
		VerticalScrollbar($, { right:0 }),
	]
}));

var NoDevicesContainer = Container.template($ => ({
	left:10, right:0, top:90, bottom:0,
	contents: [
		Text($, { left:0, right:0, style:featureEmptyStyle, string:"Simulator not running.\nUse \"Launch\" to start the simulator." }),
	]
}));

var SerialDeviceContainer = Container.template($ => ({
	left:10, right:0, top:90, bottom:0,
	contents: [
		Text($, { left:0, right:0, style:featureEmptyStyle, string:"" }),
	]
}));

import {
	ProjectTile
} from "tiles";

import {
	ErrorView
} from "shell/viewer";



