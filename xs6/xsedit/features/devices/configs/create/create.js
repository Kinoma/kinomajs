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
	getUpdateInfo() {
		var software = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=" + getEnvironmentVariable("CREATE_SOFTWARE_TARGET"));
		software.setRequestHeader("Accept", "application/json");
		software.setRequestCertificate(updateCredentials);
		var system = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=" + getEnvironmentVariable("CREATE_FIRMWARE_TARGET"));
		system.setRequestHeader("Accept", "application/json");
		system.setRequestCertificate(updateCredentials);
		return Promise.all([
			software.invoke(Message.JSON).then(json => { return json }),
			system.invoke(Message.JSON).then(json => { return json }),
		]);
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
		return this.wsRequest({
			handler: "updateSoftware",
			update: this.update[0],
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
	updateSystem() {
		data.duration = 1000;
		this.updateDialog = new UpdateDialog(data);
		data.TITLE.string = "Updating \"" + this.name + "\" to " + data.title + " " + this.softwareUpdateVersion + "...",
		data.SUBTITLE.string = "Downloading Update";
		data.INFO.string = "Please keep the device connected and powered during the update.";
		openDialog(this.updateDialog);
		return this.wsRequest({
			handler: "updateSystem",
			update: this.update[1],
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
