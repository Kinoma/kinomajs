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
import {
	model,
} from "shell/main";

import { 
	DeviceConfig, 
	updateCredentials
} from "features/devices/devices";

import { 
	UpdateDialog,
	closeDialog,
	openDialog,
} from "features/devices/viewer";

const PATH_MAX = 127;

export default class Element extends DeviceConfig {
	constructor(devices, discovery) {
		super(devices, discovery);
	}
	checkProject(project) {
		let valid = super.checkProject(project);
		if (valid)
			valid = this.checkProjectPathLength(project);
		return valid;
	}
	checkProjectPathLength(project) {
		let length = this.getProjectMaxPathLength(project.url, "", 0) + 1;
		let valid = length < PATH_MAX - 3 - 1;  // 3 for /k0, 1 for (js)b
		if (!valid) {
			system.alert({
				type:"stop", 
				prompt:"Relative path too long {" + length + " characters)!",
				info:"Please keep the path length inferior to " + PATH_MAX + " characters.",
				buttons:["OK"]
			});
		}
		return valid
	}
	getPathByteLength(path) {
		let encoded = encodeURIComponent(path);
		let length = path.length;
		if (encoded.length == length)
			return length;
		let match = encoded.match(/%[89ABab]/g);
		return length + (match ? match.length : 0);
	}
	getProjectMaxPathLength(root, path, length) {
		let url = root + path;
		let info = Files.getInfo(url);
		if (info) {
			if (info.type == Files.directoryType) {
				var iterator = new Files.Iterator(url);
				info = iterator.getNext();
				while (info) {
					let temp = path + info.path;
					let tempLength = this.getPathByteLength(temp);
					if (info.type == Files.directoryType)
						length = this.getProjectMaxPathLength(root, temp + "/", length);
					else if (length < tempLength)
						length = tempLength;
					info = iterator.getNext();
				}
			}
		}
		return length;
	}
	get helperID() {
		return "xsedit.element.kinoma.marvell.com";
	}
	get helperProject() {
		return {
			source: false,
			id: this.helperID,
			url: mergeURI(this.constructor.url, "./helper/"),
			query: {
				title: "Kinoma Code Helper",
				xsedit: this.debugHost + ":" + model.devicesFeature.wsServer.port,
			}
		};
	}
	get name() {
		return this.description.hostname;
	}
	set name(name) {
		this.description.hostname = name;
	}
	get softwareVersion() {
		if ("fwVersion" in this.description)
			return this.description.fwVersion;
		else
			return super.softwareVersion;
	}
	get softwareUpdateTarget() {
		if (model.elementSoftwareUpdatePreRelease)
			return getEnvironmentVariable("ELEMENT_FIRMWARE_TEST");
		else
			return getEnvironmentVariable("ELEMENT_FIRMWARE_TARGET")
	}
	get systemVersion() {
		if ("osVersion" in this.description)
			return this.description.osVersion;
		else
			return super.systemVersion;
	}
	get systemUpdateTarget() {
	}
	get toolURL() {
		return mergeURI(this.url, "/app/");
	}
	getUpdateInfo() {
		var message = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update-info?target="
			+ getEnvironmentVariable("ELEMENT_FIRMWARE_TARGET") + ","
			+ getEnvironmentVariable("ELEMENT_FIRMWARE_TEST"));
		message.setRequestHeader("Accept", "application/json");
		message.setRequestCertificate(updateCredentials);
		return message.invoke(Message.JSON);
	}
	isSimulator() {
		return this.systemVersion.indexOf("WM/") != 0;
	}
	newPingMessage() {
		debugger
	}
	newSetupMessage(path, query) {
		if (query)
			path = path + "?" + serializeQuery(query);
		let message = new Message(mergeURI(this.url, path));
		message.setRequestHeader("Connection", "Close");
		return message;
	}
	// HELPER
	blinkLight(config) {
		return this.wsRequest({
			handler: "blinkLight",
			config,
		});
	}
	networkConfigure(config) {
		config.save = true;
		return this.wsRequest({
			handler: "networkConfigure",
			config
		});
	}
	networkScan(active) {
		// nothing to do for Element
	}
	pinExplorerStart(container) {
		return this.wsRequest({
			handler: "pinExplorerStart",
		});
	}
	pinExplorerStop(container) {
		return this.wsRequest({
			handler: "pinExplorerStop",
		});
	}
	pinsShare(shared) {
		return this.wsRequest({
			handler: "pinsShare",
			shared,
			ip: this.currentIP,
		});
	}
	setName(name) {
		this.name = name;
		shell.distribute("onDeviceSelected", this);
		return this.wsRequest({
			handler: "setName",
			name
		});
	}
	// SETUP
	getNetworkSSID() {
		if (model.SSID)
			return Promise.resolve(model.SSID);
		else {
			let message = this.newSetupMessage("/ssid");
			return message.invoke(Message.JSON).then(ssid => {
				if (ssid)
					model.SSID = ssid;
				else
					debugger
				return Promise.resolve(model.SSID);
			});
		}
	}
	// UPDATE
	updateSoftware(data) {
		let message = new Message("/updateElement?uuid=" + this.uuid + "&target=" + this.softwareUpdateTarget);
		message.setRequestHeader("Connection", "Close");
		return message.invoke();
	}
	updateSoftwareStatus() {
		return Promise.resolve(this.softwareStatus);
	}
	updateSoftwareError(data) {
		super.updateSoftwareError(data);
		this.updateAlert(data, false, "Please try again later.");
	}
	updateSoftwareFinished(data) {
		super.updateSoftwareFinished(data);
		model.devicesFeature.onDeviceDown(this);
		this.updateAlert(data, true, "The device is now restarting.");
	}
}

Handler.Bind("/updateElement", class extends Behavior {
	onInvoke(handler, request) {
		let query = parseQuery(request.query);
		this.device = model.devicesFeature.findDeviceByUUID(query.uuid);
		this.device.quitApplication();

		this.state = 0;
		let message = this.device.newSetupMessage("/download", { target:query.target, test:false });
		this.success = false;
		this.buffer = "";
		this.device.softwareStatus.updating = true;
		handler.download(message, null);
		this.dialog = new UpdateDialog(this);
		this.title = "Kinoma Software";
		this.TITLE.string = "Updating \"" + this.device.name + "\" to " + this.title + " " + this.device.softwareUpdateVersion + "...",
		this.SUBTITLE.string = "Downloading...";
		this.INFO.string = "Please keep the device connected and powered during the update.";
		openDialog(this.dialog);
	}
	fromChunkToString(chunk) {
		if (!chunk) return "";

		var chars = [];
		for (var i = 0, len = chunk.length; i < len; i++) {
			chars.push(chunk.peek(i));
		}
		return String.fromCharCode.apply(String, chars);
	}
	parseArray() {
		let buffer = this.buffer;
		let json;
		let start = buffer.indexOf("[");
		let stop = buffer.indexOf("]") + 1;
		if ((start >= 0) && (stop > 0)) {
			json = JSON.parse(buffer.slice(start, stop));
			this.buffer = buffer.slice(stop);
		}
		return json;
	}
	parseJSON() {
		let buffer = this.buffer;
		let json;
		let start = buffer.indexOf("{");
		let stop = buffer.indexOf("}") + 1;
		if ((start >= 0) && (stop > 0)) {
			json = JSON.parse(buffer.slice(start, stop));
			this.buffer = buffer.slice(stop);
		}
		return json;
	}
	parseOffset(file) {
		let buffer = this.buffer;
		let json = this.parseJSON();
		if (json) {
			if (json[file.name] == "http error")
				return -1;
		}
		let start = buffer.indexOf("{");
		if (start >= 0) {
			this.buffer = buffer.slice(start);
			buffer = buffer.slice(0, start);
		}
		else
			this.buffer = "";
		let length = file.tag.length;
		let index = buffer.indexOf(file.tag);
		if (index >= 0) {
			return file.size;
		}
		let offset = file.offset;
		if (buffer.length <= length) {
			offset = parseInt(buffer);
		}
		else {
			for (let i = offset.toString().length; i <= length; i++) {
				let number = parseInt(buffer.slice(-i));
				if (offset < number) {
					offset = number;
					break;
				}
			}
		}
		return offset;
	}
	onReceive(handler, message, chunk) {
		let file, info
		this.buffer += this.fromChunkToString(chunk);
		switch (this.state) {
			case 0:
				this.files = this.parseArray();
				this.filesIndex = 0;
				this.state = 1;
				break;
			case 1:
				let name = this.files[this.filesIndex];
				info = this.parseJSON();
				file = {
					name,
					size:info[name],
					offset:0,
					tag:info[name].toString()
				};
				this.files[this.filesIndex] = file;
				this.state = 2;
				this.SUBTITLE.string = "Downloading " + name;
				this.COUNT.string = (this.filesIndex + 1) + "/" + this.files.length;
				this.PROGRESS.delegate("onValueChanged");
				break;
			case 2:
				file = this.files[this.filesIndex];
				let offset = this.parseOffset(file);
				if (offset < 0) {
					this.SUBTITLE.string = "Download failed";
					this.success = false;
				}
				else {
					file.offset = offset;
					this.SUBTITLE.string = "Downloading " + file.name;
					this.PROGRESS.delegate("onValueChanged", file.offset, file.size);
					if (file.offset == file.size)
						this.state = 3;
				}
				break;
			case 3:
				file = this.files[this.filesIndex];
				info = this.parseJSON();
				if (info[file.name] == "OK") {
					this.SUBTITLE.string = "Downloaded " + file.name;
					this.filesIndex++;
					if (this.filesIndex < this.files.length)
						this.state = 1;
					else
						this.success = true;
				}
				else
					this.success = false;
				break;
			default:
				break;
		}
	}
	onComplete(handler, message) {
		closeDialog(this.dialog);
		if (this.success)
			this.device.updateSoftwareFinished(this);
		else
			this.device.updateSoftwareError(this);
	}
});

Element.iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
Element.id = "com.marvell.kinoma.launcher.element";
Element.product = "Kinoma Element";
Element.tag = "Element";
Element.templateTag = "elementSample";
Element.url = this.uri;

import {
	BLLExplorerTile,
	BLLExplorerView 
} from "features/devices/apps/bllexplorer/bllexplorer";

import {
	PinExplorerTile,
	PinExplorerView 
} from "features/devices/apps/pinexplorer/pinexplorer";

import {
	SettingsTile,
	SettingsView 
} from "features/devices/apps/settings/settings";

import {
	SidePinsTile,
	SidePinsView 
} from "features/devices/apps/sidepins/sidepins";

import {
	WifiTest,
	WifiTile,
	WifiView 
} from "features/devices/apps/wifi/wifi";

import {
	BlinkLightTest,
	BlinkLightTile,
	BlinkLightView 
} from "features/devices/apps/blinklight/blinklight";

Element.apps = {
	wifi: {
		Test: WifiTest,
		Tile: WifiTile,
		View: WifiView
	},
	settings: {
		Tile: SettingsTile,
		View: SettingsView
	},
	sidepins: {
		Tile: SidePinsTile,
		View: SidePinsView
	},
	pinexplorer: {
		Tile: PinExplorerTile,
		View: PinExplorerView
	},
	blinklight: {
		Test: BlinkLightTest,
		Tile: BlinkLightTile,
		View: BlinkLightView
	},
//	bllexplorer: {
//		Tile: BLLExplorerTile,
//		View: BLLExplorerView
//	},
}
Element.serial = {
	description: {
		vendor: 0x1286,
		product: 0x8080,
		name: "MARVELL MC200 VCOM ",
	},
	settings: {
		name: "Kinoma Element",
		baud: 115200,
		bits: 8,
		parity: "N",
		stop: 1,
	},
	shortcuts: [
	]
}

Element.preferences = {
	discoveryFlag: true,
	serialFlag: true,
}


