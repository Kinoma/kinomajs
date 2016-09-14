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

import {
	Configure
} from "configure";

import PinsSimulators from "PinsSimulators6";

system.cursors = { arrow: 1 }
shell.changeCursor = function() {}

var smallTitleStyle = new Style({ font:"bold", size:16, color:"white", horizontal:"left" });
var largeTitleStyle = new Style({ font:"bold", size:24, color:"white", horizontal:"left" });

class ShellBehavior extends EmbedShellBehavior {
/* HOVER */
	onHover() {
		this.onTouchMoved(shell, 0, this.hoverX, this.hoverY, 0);
	}
	onTouchBegan(shell, id, x, y, ticks) {
		this.onTouchMoved(shell, id, x, y, ticks);
		this.hoverFlag = false;
	}
	onTouchEnded(shell, id, x, y, ticks) {
		this.hoverFlag = true;
		this.onTouchMoved(shell, id, x, y, ticks);
	}
	onTouchMoved(shell, id, x, y, ticks) {
		if (this.hoverFlag) {
			var content = shell.hit(x, y);
			if (this.hoverContent != content) {
				this.cursorShape = 0;
				if (this.hoverContent)
					this.hoverContent.bubble("onMouseExited", x, y);
				this.hoverContent = content;
				if (this.hoverContent)
					this.hoverContent.bubble("onMouseEntered", x, y);
				shell.changeCursor(this.cursorShape);
			}
			else if (this.hoverContent)
				this.hoverContent.bubble("onMouseMoved", x, y);
			this.hoverX = x;
			this.hoverY = y;
		}
	}
	onTouchScrolled(shell, touched, dx, dy, ticks) {
		if (Math.abs(dx) > Math.abs(dy))
			dy = 0;
		else
			dx = 0;
		var content = this.hoverContent;
		while (content) {
			if (content instanceof Scroller)
				content.delegate("onTouchScrolled", touched, dx, dy, ticks);
			content = content.container;
		}
	}
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
		this.currentDevice.description.uuid = shell.uuid;
		system.device = this.currentDevice.description.title
		this.writePreferences();
		shell.distribute("onDeviceChanged");
		this.startup();
	}
/* SCREEN MENU */
	canConfigure() {
		return !shell.transitioning;
	}
	canScreen(shell, item) {
		item.check = this.currentScreen == this.screens[item.value];
		return true;
	}
	doConfigure() {
		Configure(this);
	}
	doScreen(shell, item) {
		this.selectScreen(this.screens[item.value]);
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
	onCreate() {
		this.cursorShape = system.cursors.arrow;
		this.hoverContent = null;
		this.hoverFlag = true;
		this.hoverX = -1;
		this.hoverY = -1;
		super.onCreate();
		this.currentDevice.description.uuid = shell.uuid;
	}
	onDefaults() {
		super.onDefaults();
		this.loadDevices();
		this.loadResources();
	}
	onOpen() {
		this.devices.forEach(device => device.description.name = this.name);
		this.loadScreen(this.currentScreen);
		this.window = new Window(this);
		shell.add(this.window);
		this.updateScreenMenu();
		shell.windowTitle = "EmbedShell";
		super.onOpen();
	}
	onNameChanged() {
		this.devices.forEach(device => device.description.name = this.name);
	}
	onQuit() {
		this.unloadScreen(this.currentScreen);
		super.onQuit();
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

/* PREFERENCES */
	readPreferencesObject(preferences) {
		super.readPreferencesObject(preferences);
		if ("windowState" in preferences)
			shell.windowState = preferences.windowState;
		if ("deviceIndex" in preferences)
			this.currentDevice = this.devices[preferences.deviceIndex];
		if (("screens" in preferences) && (preferences.screens.length == 10))
			this.screens = preferences.screens;
		if ("screenIndex" in preferences)
			this.currentScreen = this.screens[preferences.screenIndex];
		system.device = this.currentDevice.description.title
	}
	writePreferencesObject(preferences) {
		super.writePreferencesObject(preferences);
		preferences.windowState = shell.windowState;
		preferences.deviceIndex = this.devices.indexOf(this.currentDevice);
		preferences.screens = this.screens;
		preferences.screenIndex = this.screens.indexOf(this.currentScreen);
	}
	
/* RESOURCES */
	loadResources() {
		let url = mergeURI(shell.url, "./screens/");
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		let resources = this.resources = [];
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					let jsonURL = mergeURI(url, info.path + "/" + info.path + ".json");
					let pngURL = mergeURI(url, info.path + "/" + info.path + ".png");
					if (Files.exists(jsonURL) && Files.exists(pngURL)) {
						let resource = JSON.parse(Files.readText(jsonURL));
						resource.texture = new Texture(pngURL);
						resource.iconSkin = this.makeIconSkin(resource.texture); 
						resources.push(resource);
					}
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		let screens = this.screens = [];
		for (let kind = 0; kind < 10; kind++) {
			let resource = resources[kind];
			screens.push({
				width: Math.round(resource.width * resource.scale),
				height: Math.round(resource.height * resource.scale),
				scale: resource.scale,
				kind
			});
		}
		this.currentScreen = this.screens[0];
		this.screenMode = false;
	}
	makeIconSkin(texture) {
		var srcWidth = texture.width;
		var srcHeight = texture.height;
		var dstWidth;
		var dstHeight
		if (srcWidth > srcHeight) {
			dstWidth = 200;
			dstHeight = Math.round(200 * srcHeight / srcWidth);
		}
		else {
			dstWidth = Math.round(200 * srcWidth / srcHeight);
			dstHeight = 200;
		}
		var port = new Port({width: dstWidth, height: dstHeight});
		port.behavior = {
			onDraw: function(port) {
				port.drawImage(texture, 0, 0, dstWidth, dstHeight, 0, 0, srcWidth, srcHeight);
			}
		}
		return new Skin({ texture:new Texture(port, 2), x:0, y:0, width:dstWidth >> 1, height:dstHeight >> 1, aspect:"fit" }); 
	}
	
/* SCREEN */
	changeScreen() {
		this.unloadScreen(this.currentScreen);
		this.loadScreen(this.currentScreen);
		this.swapScreen(this.currentScreen);
		this.updateScreenMenu();
		shell.distribute("onScreenChanged");
	}
	changeScreenMode(shell, mode) {
		this.screenMode = mode;
		if (mode) {
			let screen = this.currentScreen;
			let resource = this.resources[screen.kind];
			if (mode) {
				screen.width = Math.round(resource.width * resource.scale);
				screen.height =  Math.round(resource.height * resource.scale);
				screen.scale = resource.scale;
			}
			else {
				screen.width =  Math.round(resource.height * resource.scale);
				screen.height =  Math.round(resource.width * resource.scale);
				screen.scale = resource.scale;
			}
			this.changeScreen();
		}
		shell.distribute("onScreenModeChanged", mode);
	}
	loadScreen(screen) {
		let resource = this.resources[screen.kind];
		let texture = resource.texture;
		let srcWidth = texture.width;
		let srcHeight = texture.height;
		let sx = screen.width / resource.width / screen.scale;
		let sy = screen.height / resource.height / screen.scale;
		let dstWidth = Math.round(srcWidth * sx);
		let dstHeight = Math.round(srcHeight * sy);
		screen.x = Math.round(resource.x * sx);
		screen.y = Math.round(resource.y * sy);
		screen.dx = Math.round(screen.width / screen.scale);
		screen.dy = Math.round(screen.height / screen.scale);
		var canvas = new Canvas({width: dstWidth, height: dstHeight});
		var ctx = canvas.getContext("2d");
		ctx.save();
		ctx.translate(dstWidth / 2, dstHeight / 2);
		ctx.scale(sx, sy);
		ctx.drawImage(texture, -srcWidth / 2, -srcHeight / 2, srcWidth, srcHeight);
		ctx.restore();
		screen.skin = new Skin({ texture:new Texture(canvas), x:0, y:0, width:dstWidth, height:dstHeight }); 
		
		this.screenMode = (screen.width == Math.round(resource.width * resource.scale))
				&& (screen.height ==  Math.round(resource.height * resource.scale))
				&& (screen.scale == resource.scale);
	}
	selectScreen(screen) {
		let mode = this.screenMode;
		this.unloadScreen(this.currentScreen);
		this.currentScreen = screen;
		this.loadScreen(screen);
		this.swapScreen(screen);
		shell.distribute("onScreenChanged");
		if (mode != this.screenMode)
			shell.distribute("onScreenModeChanged", this.screenMode);
	}
	swapScreen(screen) {
		let hostContainer = this.getHostContainer();
		let host = hostContainer.first;
		hostContainer.remove(host);
		
		let container = this.window.last;
		container.replace(container.first, new ScreenContainer(screen));
		
		hostContainer = this.getHostContainer();
		hostContainer.add(host);
		if (host.adapt)
			host.adapt();
	}
	unloadScreen(screen) {
		delete screen.skin;
		delete screen.x;
		delete screen.y;
		delete screen.dx;
		delete screen.dy;
	}
	updateScreenMenu() {
		let screenMenu = shell.menus[1];
		screenMenu.items = screenMenu.items.slice(0, 2);
		this.screens.forEach((screen, i) => {
			let title = screen.width + " x " + screen.height;
			if (screen.scale != "1")
				title += " ÷ " + screen.scale;
			screenMenu.items.push({ title, key:i.toString(), command:"Screen", value:i });
		});
		shell.updateMenus();
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
			left:$.x, width:$.dx, top:$.y, height:$.dy, skin:blackSkin,
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
			{ title: "Configure...", key: "C", command: "Configure"},
			null,
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

