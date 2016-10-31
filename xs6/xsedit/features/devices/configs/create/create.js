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

export default class Create extends DeviceConfig {
	constructor(devices, discovery) {
		super(devices, discovery);
		if (!("firmware" in this.description))
			this.description.firmware = getEnvironmentVariable("OS") + " " + getEnvironmentVariable("OSVersion");
	}
	get helperID() {
		return "xsedit.create.kinoma.marvell.com";
	}
	get helperProject() {
		return {
			id: this.helperID,
			url: mergeURI(this.constructor.url, "./helper/"),
			query: {
				id: this.helperID,
				title: "Kinoma Code Helper",
			}
		};
	}
	get softwareUpdateTarget() {
		if (model.elementSoftwareUpdatePreRelease)
			return getEnvironmentVariable("CREATE_SOFTWARE_TEST");
		else
			return getEnvironmentVariable("CREATE_SOFTWARE_TARGET")
	}
	get systemUpdateTarget() {
		return getEnvironmentVariable("CREATE_FIRMWARE_TARGET");
	}
	getUpdateInfo() {
		var message = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update-info?target="
			+ getEnvironmentVariable("CREATE_SOFTWARE_TARGET") + ","
			+ getEnvironmentVariable("CREATE_SOFTWARE_TEST") + ","
			+ getEnvironmentVariable("CREATE_FIRMWARE_TARGET"));
		message.setRequestHeader("Accept", "application/json");
		message.setRequestCertificate(updateCredentials);
		return message.invoke(Message.JSON);
	}
	isSimulator() {
		return this.systemVersion.indexOf(getEnvironmentVariable("OS")) == 0;
	}
	// HELPER
	clearApps() {
		return this.wsRequest({
			handler: "clearApps",
		});
	}
	clearAppsPrefs() {
		return this.wsRequest({
			handler: "clearAppsPrefs",
		});
	}
	clearCookies() {
		return this.wsRequest({
			handler: "clearCookies",
		});
	}
	clearHTTPCache() {
		return this.wsRequest({
			handler: "clearHTTPCache",
		});
	}
	clearKnownNetworks() {
		return this.wsRequest({
			handler: "clearKnownNetworks",
		});
	}
	getStartupApp() {
		return this.wsRequest({
			handler: "getStartupApp",
		});
	}
	getStartupAppList() {
		return this.wsRequest({
			handler: "getStartupAppList",
		});
	}
	networkScan(active) {
		return this.wsRequest({
			handler: "networkScan",
			active,
		});
	}
	pinExplorerStart(container) {
		return this.wsRequest({
			handler: "pinExplorerStart",
			ip: this.currentIP,
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
	// STUDIO
	getNetworkSSID() {
		if (model.SSID)
			return Promise.resolve(model.SSID);
		else {
			let message = this.newStudioMessage("/network/status");
			return message.invoke(Message.JSON).then(status => {
				if (status && "ssid" in status)
					model.SSID = status.ssid;
				return Promise.resolve(model.SSID);
			});
		}
	}
	networkConfigure(config) {
		this.wsClose();
		let delay = new Message("/network/configure?" + serializeQuery({uuid:this.uuid}));
		delay.requestText = JSON.stringify(config);
		return delay.invoke().then(message => {
			model.devicesFeature.onDeviceDown(this);
		});
	}
	newPingMessage() {
		return this.newStudioMessage("/info");
	}
	// UPDATE
	updateSoftware(data) {
		data.duration = 1000;
		this.updateDialog = new UpdateDialog(data);
		data.TITLE.string = "Updating \"" + this.name + "\" to " + data.title + " " + this.softwareUpdateVersion + "...",
		data.SUBTITLE.string = "Downloading Update";
		data.INFO.string = "Please keep the device connected and powered during the update.";
		openDialog(this.updateDialog);
		let target = this.softwareUpdateTarget;
		return this.wsRequest({
			handler: "updateSoftware",
			update: this.update[target][0].info,
		});
	}
	updateSoftwareError(data) {
		super.updateSoftwareError(data);
		closeDialog(this.updateDialog);
		delete this.updateDialog;
		this.updateAlert(data, false, "Please try again later.");
	}
	updateSoftwareFinished(data) {
		super.updateSoftwareFinished(data);
		this.wsClose();
		model.devicesFeature.onDeviceDown(this);
		closeDialog(this.updateDialog);
		delete this.updateDialog;
		this.updateAlert(data, true, "The device is now restarting.");
	}
	updateSoftwareStatus() {
		return this.wsRequest({
			handler: "updateSoftwareStatus",
		});
	}
	updateSystem(data) {
		data.duration = 1000;
		this.updateDialog = new UpdateDialog(data);
		data.TITLE.string = "Updating \"" + this.name + "\" to " + data.title + " " + this.systemUpdateVersion + "...",
		data.SUBTITLE.string = "Downloading Update";
		data.INFO.string = "Please keep the device connected and powered during the update.";
		openDialog(this.updateDialog);
		let target = this.systemUpdateTarget;
		return this.wsRequest({
			handler: "updateSystem",
			update: this.update[target][0].info,
		});
	}
	updateSystemError(data) {
		super.updateSystemError(data);
		closeDialog(this.updateDialog);
		delete this.updateDialog;
		this.updateAlert(data, false, "Please try again later.");
	}
	updateSystemFinished(data) {
		super.updateSystemFinished(data);
		this.wsClose();
		model.devicesFeature.onDeviceDown(this);
		closeDialog(this.updateDialog);
		delete this.updateDialog;
		this.updateAlert(data, true, "The device is now restarting.");
	}
	updateSystemStatus() {
		return this.wsRequest({
			handler: "updateSystemStatus",
		});
	}
	getPinFilteringInfo() {
		let pinFilteringInfo = 
		{
			buttons: [
				{ name:"Front", startPin:51, endPin:66 },
				{ name:"Back", startPin:1, endPin:50 }
			],
			locations: [
				{ name:"Front - Left", startPin:51, endPin:58 },
				{ name:"Front - Right", startPin:59, endPin:66 },
				{ name:"Back", startPin:1, endPin:50 }
			]
		}
		return pinFilteringInfo;
	}
	pwmPinHasMotorMode(logicalPinNumber) {
		// logical and physical same on create
		let physicalPinNumber = logicalPinNumber
		return (physicalPinNumber > 50);
	}
}

Handler.Bind("/network/configure", class extends Behavior {
	onInvoke(handler, message) {
		handler.wait(3000);
	}
	onComplete(handler, message) {
		let query = parseQuery(handler.message.query);
		let device = model.devicesFeature.findDeviceByUUID(query.uuid);
		let config = JSON.parse(handler.message.requestText);
		let configure = device.newStudioMessage("/network/configure", config);
		configure.invoke();
	}
});


Create.iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:60, height:60, states:60, variants:60 });
Create.id = "com.marvell.kinoma.launcher.create";
Create.product = "Kinoma Create";
Create.tag = "Create";
Create.templateTag = "createSample";
Create.url = this.uri;

import {
	BLLExplorerTile,
	BLLExplorerView 
} from "features/devices/apps/bllexplorer/bllexplorer";

import {
	FrontPinsTile,
	FrontPinsView 
} from "features/devices/apps/frontpins/frontpins";

import {
	PinExplorerTile,
	PinExplorerView 
} from "features/devices/apps/pinexplorer/pinexplorer";

import {
	SettingsTile,
	SettingsView 
} from "features/devices/apps/settings/settings";

import {
	WifiTest,
	WifiTile,
	WifiView 
} from "features/devices/apps/wifi/wifi";

Create.apps = {
	wifi: {
		Test: WifiTest,
		Tile: WifiTile,
		View: WifiView
	},
	settings: {
		Tile: SettingsTile,
		View: SettingsView
	},
	frontpins: {
		Tile: FrontPinsTile,
		View: FrontPinsView
	},
	pinexplorer: {
		Tile: PinExplorerTile,
		View: PinExplorerView
	},
//	bllexplorer: {
//		Tile: BLLExplorerTile,
//		View: BLLExplorerView
//	},
}

Create.serial = {
	description: {
		vendor: 0x0403,
		product: 0x6015,
		name: "FT230X Basic UART",
	},
	settings: {
		name: "Kinoma Create",
		baud: 115200,
		bits: 8,
		parity: "N",
		stop: 1,
	},
	shortcuts: [
	]
}

Create.preferences = {
	discoveryFlag: true,
	serialFlag: true,
}
