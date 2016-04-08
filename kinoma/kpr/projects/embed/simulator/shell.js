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
	EmbedShellBehavior,
} from "embedShell";

import {
	blackSkin,
	graySkin,
	kinomaSkin,
	whiteSkin,
} from "embedHome";

import PinsSimulators from "PinsSimulators6";

var smallTitleStyle = new Style({ font:"bold", size:16, color:"white", horizontal:"left" });
var largeTitleStyle = new Style({ font:"bold", size:24, color:"white", horizontal:"left" });

class ShellBehavior extends EmbedShellBehavior {
/* APPLE MENU */
	canAbout() {
		return true;
	}
	canQuit() {
		return true; 
	}
	doAbout() {
		shell.alert("about", "Kinoma Simulator", "Copyright © 2012 Marvell. All rights reserved. Kinoma is a registered trademark of Kinoma, Inc."); 
	}
	doQuit() {
		shell.quit(); 
	}
/* DEVICE MENU */
	canDevice(shell, item) {
		item.check = this.currentDevice == this.devices[item.value];
		return true;
	}
	doDevice(shell, item) {
		this.goHome();
		shell.share(false);
		this.currentDevice = this.devices[item.value];
		shell.share({ ssdp:true });
		this.writePreferences();
		shell.distribute("onDeviceChanged");
		this.startup();
	}
/* SCREEN MENU */
	canScreen(shell, item) {
		item.check = this.currentScreen == this.screens[item.value];
		return true;
	}
	doScreen(shell, item) {
		this.currentScreen = this.screens[item.value];
		let hostContainer = this.getHostContainer();
		let host = hostContainer.first;
		hostContainer.remove(host);
		
		let container = this.window.last;
		container.replace(container.first, new ScreenContainer(this.currentScreen));
		
		hostContainer = this.getHostContainer();
		hostContainer.add(host);
		if (host.adapt)
			host.adapt();
	}
/* APPLICATION MENU */
	canBreakApplication() {
		return this.host ? true : false; 
	}
	canCloseApplication() {
		return this.host ? true : false; 
	}
	canPurgeApplication() {
		return this.host ? true : false; 
	}
	doBreakApplication() {
		this.host.debugger(); 
	}
	doCloseApplication() {
		this.goHome(); 
	}
	doPurgeApplication() {
		this.host.purge(); 
	}
/* HELP MENU */
	canSupport() {
		return true;
	}
	doSupport() {
		launchURI("http://developer.kinoma.com"); 
	}
	
/* EVENTS */
	onDefaults() {
		super.onDefaults();
		this.loadDevices();
		this.loadScreens();
	}
	onOpen() {
		this.devices.forEach(device => device.description.name = this.name);
		this.window = new Window(this);
		shell.add(this.window);
		shell.updateMenus();
		super.onOpen();
	}
	onNameChanged() {
		this.devices.forEach(device => device.description.name = this.name);
	}
	
/* APPLICATIONS */
	getHostContainer() {
		return this.window.last.first.first;
	}
	
/* DEVICES */
	loadDevices() {
        let name = this.name;
        let uuid = shell.uuid;
        let version = getEnvironmentVariable("CORE_VERSION");
		let url = mergeURI(shell.url, "./devices/");
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		this.devices = [];
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					let jsonURL = mergeURI(url, info.path + "/description.json");
					let iconURL = mergeURI(url, info.path + "/icon.png");
					let pictureURL = mergeURI(url, info.path + "/picture.png");
					let descriptionURL = mergeURI(url, info.path + "/dd.xml");
					if (Files.exists(jsonURL) && Files.exists(iconURL)) {
						let description = JSON.parse(Files.readText(jsonURL));
						let texture = new Texture(iconURL);
						let iconSkin = new Skin({ texture, x:0, y:0, width:texture.width, height:texture.height }); 
						description.firmware = model.firmware;
						description.name = name;
						description.uuid = uuid;
						description.version = version;
						this.devices.push({ description, descriptionURL, iconSkin, iconURL, pictureURL });
					}
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		this.currentDevice = this.devices[0];
		let deviceMenu = shell.menus[0];
		this.devices.forEach((device, i) => {
			deviceMenu.items.push({ title:device.description.title, key:"", command:"Device", value:i });
		});
	}

/* PINS */
	addSimulatorPart(shell, data) {
		var scroller = shell.first.first.first;
		return scroller.delegate("addPart", data);
	}
	removeSimulatorPart(shell, container) {
		var scroller = shell.first.first.first;
		scroller.partsContainer.remove(container);
	}

/* SCREENS */
	loadScreens() {
		let url = mergeURI(shell.url, "./screens/");
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		this.screens = [];
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					let jsonURL = mergeURI(url, info.path + "/" + info.path + ".json");
					let pngURL = mergeURI(url, info.path + "/" + info.path + ".png");
					if (Files.exists(jsonURL) && Files.exists(pngURL)) {
						let screen = JSON.parse(Files.readText(jsonURL));
						let texture = new Texture(pngURL);
						screen.skin = new Skin({ texture, x:0, y:0, width:texture.width, height:texture.height }); 
						this.screens.push(screen);
					}
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		this.currentScreen = this.screens[0];
		let screenMenu = shell.menus[1];
		this.screens.forEach((screen, i) => {
			screenMenu.items.push({ title:screen.title, key:i.toString(), command:"Screen", value:i });
		});
	}
	
/* PREFERENCES */
	readPreferencesObject(preferences) {
		super.readPreferencesObject(preferences);
		if ("windowState" in preferences)
			shell.windowState = preferences.windowState;
		if ("deviceIndex" in preferences)
			this.currentDevice = this.devices[preferences.deviceIndex];
		if ("screenIndex" in preferences)
			this.currentScreen = this.screens[preferences.screenIndex];
	}
	writePreferencesObject(preferences) {
		super.writePreferencesObject(preferences);
		preferences.windowState = shell.windowState;
		preferences.deviceIndex = this.devices.indexOf(this.currentDevice);
		preferences.screenIndex = this.screens.indexOf(this.currentScreen);
	}
};

var Window = Line.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, { 
			width:360, top:0, bottom:0, skin:whiteSkin, clip:true,
			contents: [
				PinsSimulators.PartsScroller($, { top:60 }),
				DeviceHeader($.currentDevice, {}),
			] 
		}),
		Container($, { 
			left:0, right:0, top:0, bottom:0, skin:graySkin, clip:true,
			contents: [
				ScreenContainer($.currentScreen, {}),
			] 
		}),
	],
}));

var DeviceHeader = Line.template($ => ({
	left:0, right:0, top:0, height:60, skin:kinomaSkin,
	Behavior: class extends Behavior {
		onDeviceChanged(container) {
			let device = model.currentDevice;
			container.first.skin = device.iconSkin;
			container.last.first.string = device.description.title;
		}
	},
	contents: [ 
		Content($, { left:10, width:60, height:60, skin:$.iconSkin }),
		Column($, {
			left:10, right:0,
			contents:[
				Label($, { left:0, right:0, style:largeTitleStyle, string:$.description.title }),
				Label($, { left:0, right:0, style:smallTitleStyle, string:"Hardware Pins Simulators" }),
			]
		}),
	] 
}));

var ScreenContainer = Container.template($ => ({
	width:$.skin.width, height:$.skin.height, skin:$.skin,
	contents: [
		Container($, {
			left:$.x, width:$.width, top:$.y, height:$.height, skin:blackSkin,
			contents: [
			]
		}),
	] 
}));

shell.menus = [
	{ 
		title: "Device", 
		items: [
		]
	},
	{ 
		title: "Screen",
		items: [
		],
	},
	{ 
		title: "Application", 
		items: [
			{ title: "Break", key: "B", command: "BreakApplication"},
			{ title: "Close", key: "W", command: "CloseApplication"},
			{ title: "Purge", key: "", command: "PurgeApplication"},
		]
	},
	{
		title: "Help",
		items: [
			{ title: "Kinoma Developer", command: "Support" },
			null,
			{ title: "About Kinoma Simulator", command: "About"},
		],
	},
];
shell.behavior = new ShellBehavior(shell, {});

// HANDLERS

Handler.Bind("/", class extends Behavior {
	onInvoke(handler, message) {
		let text = Files.readText(model.currentDevice.descriptionURL);
		text = text.replace("[friendlyName]", model.name);
		text = text.replace("[udn]", shell.uuid);
		message.responseText = text;
		message.setResponseHeader("Content-Type", "text/xml; charset=\"utf-8\"");
		message.status = 200;
	}
});

Handler.Bind("/description", class extends Behavior {
	onInvoke(handler, message) {
		message.responseText = JSON.stringify(model.currentDevice.description);
		message.status = 200;
	}
});

Handler.Bind("/description/icon", class extends Behavior {
	onInvoke(handler, message) {
		message.responsePath = model.currentDevice.iconURL;
		message.status = 200;
	}
});

Handler.Bind("/description/picture", class extends Behavior {
	onInvoke(handler, message) {
		message.responsePath = model.currentDevice.pictureURL;
		message.status = 200;
	}
});

