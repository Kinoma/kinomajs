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
	DividerLayoutBehavior,
	HorizontalDivider,
	VerticalDivider,
} from "common/divider";

import { 
	ConsolePane,
} from "console";

import {
	Feature,
	FeaturesColumn,
} from "feature";


import tool from "tool";

import { 
	Viewer,
} from "viewer";

import { 
	PreferencesViewer,
} from "preferences";

import { 
	updateCredentials
} from "features/devices/devices";

const kNoCurrentDevice = "Kinoma Code";
const kCurrentDevice = "Kinoma Code - ";

Handler.Bind("/network/interface/add", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
//		trace("/network/interface/add: " + JSON.stringify(query) + "\n");
		model.interfaces[query.name] = query;
		let info = system.getWifiInfo();
		model.SSID = info ? info.SSID : "";
	}
});

Handler.Bind("/network/interface/remove", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
//		trace("/network/interface/remove: " + JSON.stringify(query) + "\n");
		delete model.interfaces[query.name];
	}
});

class ShellBehavior extends Behavior {
	checkForUpdate() {
		let message = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=KINOMA_CODE_MAC_RELEASE");
		message.setRequestHeader("Accept", "application/json");
		message.setRequestCertificate(updateCredentials);
		message.invoke(Message.JSON).then(result => {
			if (result && result.url && result.ver && (result.ver != getEnvironmentVariable("VERSION"))) {
				system.alert({ 
					type:"note", 
					prompt:"A new version of Kinoma Code is available!",
					info:"Kinoma Code " + result.ver + " is now available. Would you like to download it now?",
					buttons:["Download", "Cancel"]
				}, ok => {
					if (ok === undefined)
						return;
					if (ok)
						launchURI(result.url);
				});
			}
		});
	}
	deleteDirectory(url) {
		while (this.url && this.url.startsWith(url))
			this.doCloseURL(shell, this.url);
		for (;;) {
			let index = this.history.findIndex(item => item.url.startsWith(url));
			if (index < 0)
				break;
			this.history.splice(index, 1);
		}
		let project = this.filesFeature.findProjectByURI(url);
		if (project)
			project.close();
		Files.deleteDirectory(url, true);
	}

	addError(url, line, reason) {
		let errors = this.errors.items;
		let error = errors.find(error => (error.url == url) && (error.line == line));
		if (error)
			return;
		var parts = parseURI(url);
		errors.push({ url, line, reason, name:parts.name,  });
		errors.sort(this.sortErrors);
		shell.distribute("onErrorsChanged");
	}
	removeErrors() {
		this.errors.items = [];
		shell.distribute("onErrorsChanged");
	}
	sortErrors(a, b) {
		let result = a.url.compare(b.url);
		if (!result)
			result = a.line - b.line;
		return result;
	}
	
/* CLIPBOARD */
	getClipboard() {
		return this.clipboard;
	}
	hasClipboard() {
		return this.clipboard ? true : false;
	}
	setClipboard(text) {
		this.clipboard = text;
	}
	onActivated(shell, activateIt) {
		if (activateIt) {
			this.clipboard = system.getClipboardText();
			if (!this.eula)
				shell.add(new Eula(this));
		}
		else
			system.setClipboardText(this.clipboard);
	}

/* DRAG & DROP */
	onFilesDropped(shell) {
		this.onFilesOpen(shell, this.urls);
		this.urls = null;
	}
	onFilesEntered(shell, urls) {
		this.urls = urls;
	}
	onFilesExited(shell) {
		this.urls = null;
	}
	onFilesOpen(shell, urls) {
		for (let url of urls) { 
			if (url.endsWith("/"))
				this.filesFeature.doOpenDirectoryCallback(url);
			else
				this.filesFeature.doOpenFileCallback(url);
		}
	}

/* APP MENU */
	canAbout() {
		return true;
	}
	canPreferences() {
		return true;
	}
	doAbout() {
		About();
	}
	doPreferences() {
		this.doOpenURL(shell, "preferences://");
	}


/* FILE MENU */
	canNewFile() {
		return this.filesFeature.canNewFile();
	}
	canNewDirectory() {
		//return this.filesFeature.canNewDirectory();
		return this.samplesFeature.canNewProject();
	}
	canOpenFile() {
		return this.filesFeature.canOpenFile();
	}
	canOpenDirectory() {
		return this.filesFeature.canOpenDirectory();
	}
	canClose() {
		return this.filesFeature.canClose(this.url);
	}
	canCloseAll() {
		return this.filesFeature.canCloseAll();
	}
	canSaveAll() {
		return this.filesFeature.canSaveAll();
	}
	doNewDirectory() {
		//this.filesFeature.doNewDirectory();
		this.samplesFeature.doNewProject();
	}
	doNewFile() {
		this.filesFeature.doNewFile();
	}
	doOpenDirectory() {
		this.filesFeature.doOpenDirectory();
	}
	doOpenFile(shell) {
		this.filesFeature.doOpenFile();
	}
	doClose() {
		this.filesFeature.doClose(this.url);
	}
	doCloseAll() {
		this.filesFeature.doCloseAll();
	}
	doSaveAll(shell) {
		this.filesFeature.doSaveAll();
	}
	doQuit(shell) {
		this.filesFeature.doQuit();
	}

/* DEBUG MENU */
	canAbort() {
		return this.debugFeature.canAbort();
	}
	canGo() {
		if (this.debugFeature.canGo())
			return true;
		return this.canRun();
	}
	canRun() {
		let project = this.filesFeature.currentProject;
		let device = this.devicesFeature.currentDevice;
		return project && project.canRun(device, this.url);
	}
	canStep() {
		return this.debugFeature.canStep();
	}
	canStepIn() {
		return this.debugFeature.canStepIn();
	}
	canStepOut() {
		return this.debugFeature.canStepOut();
	}
	canClearAllBreakpoints() {
		return this.debugFeature.canClearAllBreakpoints();
	}

	doAbort() {
		this.debugFeature.doAbort();
	}
	doGo() {
		if (this.debugFeature.canGo())
			this.debugFeature.doGo();
		else
			this.doRun();
	}
	doRun() {
		var console = this.CONSOLE.first;
		console.behavior.doClear(console);
		this.filesFeature.doSaveAll();
		this.filesFeature.currentProject.doRun(tool, this.devicesFeature.currentDevice, this.url, this.debugFeature)
	}
	doStep() {
		this.debugFeature.doStep();
	}
	doStepIn() {
		this.debugFeature.doStepIn();
	}
	doStepOut() {
		this.debugFeature.doStepOut();
	}
	doClearAllBreakpoints() {
		return this.debugFeature.doClearAllBreakpoints();
	}

/* VIEW MENU */
	canView(shell, item) {
		item.check = this.currentFeature == this.features[item.value];
		return true;
	}
	doView(shell, item) {
		this.doSelectFeature(shell, this.features[item.value]);
	}
	

/* HELP MENU */
	canSupport() {
		return true;
	}
	doSupport() {
		launchURI("http://developer.kinoma.com"); 
	}


	doLog(shell, text) {
		var console = this.CONSOLE.first;
		console.behavior.doLog(console, text);
	}
	doLogRaw(shell, text) {
		var console = this.CONSOLE.first;
		console.behavior.doLogRaw(console, text);
	}

	doCloseURL(shell, url) {
		trace("### doCloseURL " + url + "\n");
		let history = this.history;
		if (url) {
			if (this.url == url) {
				if (history.length) {
					let item = history.shift();
					url = this.url = item.url;
					this.at = undefined;
				}
				else {
					url = this.url = undefined;
					this.at = undefined;
				}
				this.doOpenView(shell, Viewer.fromURI(url));
				shell.distribute("onURLChanged", url);
			}
		}
		else {
			history.length = 0;
			this.url = undefined;
			this.at = undefined;
			this.doOpenView(shell, Viewer.fromURI(url));
			shell.distribute("onURLChanged", url);
		}
	}
	doOpenURL(shell, url, at) {
		trace("### doOpenURL " + url + "\n");
		if (this.url != url) {
			let history = this.history;
			let index = history.findIndex(item => item.url == url);
			if (index >= 0)
				history.splice(index, 1);
			if (this.url && this.url.startsWith("file://")) {
				this.history.unshift({ url:this.url });
				if (history.length > 32)
					history.length = 32;
			}
			this.url = url;
			this.at = at;
			this.doOpenView(shell, Viewer.fromURI(url));
			shell.distribute("onURLChanged", url);
		}
		else if (this.at != at) {
			this.at = at;
			shell.distribute("doSelectLine", at);
		}
			
	}
	doOpenView(shell, current) {
		let former = this.MAIN.first;
		former.delegate("onScreenEnding");
		current.delegate("onScreenBeginning");
		this.MAIN.replace(former, current);
		former.delegate("onScreenEnded");
		current.delegate("onScreenBegan");
	}
	doSelectFeature(shell, feature) {
		if (this.currentFeature != feature) {
			this.currentFeature = feature;
			this.FEATURE.replace(this.FEATURE.first, new feature.Template(feature));
			shell.distribute("onFeatureSelected", feature);
		}
	}
	
	doToggleArrangement(container) {
		let formerMain = this.MAIN;
		let formerLayout = formerMain.container;
		let formerConsole = this.CONSOLE;
		let currentLayout, content;
		if (this.arrangement) {
			this.arrangement = false;
			content = this.HORIZONTAL_MAIN_DIVIDER;
			this.horizontalMainDividerCurrent = content.behavior.current;
			this.horizontalMainDividerStatus = content.behavior.status;
			this.HORIZONTAL_MAIN_DIVIDER = null;
			currentLayout = VerticalLayout(this, { width:formerLayout.width });
		}
		else {
			this.arrangement = true;
			content = this.VERTICAL_MAIN_DIVIDER;
			this.verticalMainDividerCurrent = content.behavior.current;
			this.verticalMainDividerStatus = content.behavior.status;
			this.VERTICAL_MAIN_DIVIDER = null;
			currentLayout = HorizontalLayout(this, { width:formerLayout.width });
		}
		formerLayout.container.replace(formerLayout, currentLayout);
		content = formerMain.first;
		formerMain.remove(content);
		this.MAIN.add(content);
		content = formerConsole.first;
		formerConsole.remove(content);
		this.CONSOLE.add(content);
		this.CONSOLE.distribute("onArrangementChanged", this.arrangement);
	}
	doToggleConsole(container) {
		let divider = this.arrangement ? this.HORIZONTAL_MAIN_DIVIDER : this.VERTICAL_MAIN_DIVIDER;
		divider.behavior.toggle(divider);
	}
	get cacheDirectory() {
		return mergeURI(Files.preferencesDirectory, "./Kinoma%20Code/");
	}
	onCreate() {
		super.onCreate(shell, data);
  		shell.interval = 100;
  		shell.start();

		model = this;
		this.clipboard = "";
		
		this.cursorShape = system.cursors.arrow;
		this.hoverContent = null;
		this.hoverX = -1;
		this.hoverY = -1;
		
		this.arrangement = true;
		this.featureDividerCurrent = 320;
		this.featureDividerStatus = true;
		this.HORIZONTAL_MAIN_DIVIDER = null;
		this.horizontalMainDividerCurrent = 240;
		this.horizontalMainDividerStatus = true;
		this.VERTICAL_MAIN_DIVIDER = null;
		this.verticalMainDividerCurrent = 240;
		this.verticalMainDividerStatus = true;
		
		this.findHint = "Find String";
		this.findMode = 1;
		this.findString = "";
		this.replaceString = "";
		this.resultOffset = -1;
		this.touched = false;
		
		this.templateData = {
			deviceSelection:0,
		};
		
		this.breakpoints = {
			expanded: true,
			items: [],
		};
		this.debugPort = 5003;
		this.deviceUUID = "";
		this.errors = {
			expanded: true,
			items: [],
		};
		this.home = "";
		this.simulatorsURI = mergeURI(shell.url, "../../simulators/");
		this.history = [
		];
		this.projectsDirectory = mergeURI(Files.documentsDirectory, "Kinoma%20Code/Projects/");
		this.projectsDomain = "project.kinoma.marvell.com";
		this.samplesDirectory = mergeURI(Files.documentsDirectory, "Kinoma%20Code/Samples/");
		this.url = undefined;
		this.at = undefined;
		this.eula = false;
		
		this.createSoftwareUpdatePreRelease = false;
		this.elementSoftwareUpdatePreRelease = false;
		
		this.SSID = "";
		this.interfaces = {};
		
		let url = mergeURI(shell.url, "../features/"), featureURL;
		let names = [ "devices", "files", "debug", "samples" ];
		this.features = names.map(name => {
			let featureURL = mergeURI(url, name + "/" + name);
			let featureConstructor = require(featureURL);
			return new featureConstructor(this, featureURL);
		});
		let iterator = new Files.Iterator(url);
		let info = iterator.getNext();
		while (info) {
			if (info.type == Files.directoryType) {
				try {
					if (names.indexOf(info.path) < 0) {
						let featureURL = mergeURI(url, info.path + "/" + info.path);
						let featureConstructor = require(featureURL);
						this.features.push(new featureConstructor(this, featureURL));
					}
				}
				catch(e) {
				}
			}
			info = iterator.getNext();
		}
		
		let viewMenu = shell.menus[3];
		this.features.forEach((feature, i) => {
			let viewers = feature.viewers;
			if (viewers) {
				Viewer.register(viewers);
			}
			viewMenu.items.push({ title:feature.title, key:i.toString(), command:"View", value:i });
		});
		viewMenu.items.push(null);
		Viewer.register([ new PreferencesViewer() ]);
		
		this.currentFeature = this.features[0];
		this.devicesFeature = this.features[0],
		this.filesFeature = this.features[1];
		this.debugFeature = this.features[2];
		this.samplesFeature = this.features[3];
		
		this.preferences = "Kinoma%20Code.json";
		this.readPreferences();
		this.features.forEach(feature => feature.open());

		let window = new Window(this);
		shell.add(window);
		
		this.MAIN.add(Viewer.fromURI(this.url));
		this.CONSOLE.add(ConsolePane(this, {}));
		
		shell.updateMenus();
		shell.windowTitle = kNoCurrentDevice;
		if (this.eula)
			this.checkForUpdate();
	}
	onDebugPortChanged(shell, port) {
		this.debugFeature.close();
		this.debugFeature.open();
	}
	onDeviceSelected(shell, device) {
		shell.windowTitle = device ? kCurrentDevice + device.name : kNoCurrentDevice;
	}
	onDisplaying() {
		if (this.url)
			this.doOpen(shell, this.url);
	}
	onHover() {
		this.onTouchMoved(shell, 0, this.hoverX, this.hoverY, 0);
	}
	onProjectsChanged(shell, project) {
		this.writePreferences();
	}
	onQuit() {
		this.features.forEach(feature => feature.close());
		this.writePreferences();
	}
	onTimeChanged() {
		let now = Date.now();
		this.features.forEach(feature => feature.idle(now));
	}
	onTouchBegan(shell, id, x, y, ticks) {
		this.onTouchMoved(shell, id, x, y, ticks);
		this.touched = true;
	}
	onTouchEnded(shell, id, x, y, ticks) {
		this.touched = false;
		this.onTouchMoved(shell, id, x, y, ticks);
	}
	onTouchMoved(shell, id, x, y, ticks) {
		if (this.touched)
			return;
		var content = shell.hit(x, y);
		if (this.hoverContent != content) {
// 			if (content)
// 				trace(content.constructor.tag + "\n");
// 			else
// 				trace("null\n");
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
	
	readPreferences() {
		try {
			var url = mergeURI(Files.preferencesDirectory, this.preferences);
			var preferences = {};
			if (Files.exists(url)) {
				preferences = JSON.parse(Files.readText(url));
				if ("windowState" in preferences)
					shell.windowState = preferences.windowState;
				if ("arrangement" in preferences)
					this.arrangement = preferences.arrangement;
				if ("featureDividerCurrent" in preferences)
					this.featureDividerCurrent = preferences.featureDividerCurrent;
				if ("featureDividerStatus" in preferences)
					this.featureDividerStatus = preferences.featureDividerStatus;
				if ("horizontalMainDividerCurrent" in preferences)
					this.horizontalMainDividerCurrent = preferences.horizontalMainDividerCurrent;
				if ("horizontalMainDividerStatus" in preferences)
					this.horizontalMainDividerStatus = preferences.horizontalMainDividerStatus;
				if ("verticalMainDividerCurrent" in preferences)
					this.verticalMainDividerCurrent = preferences.verticalMainDividerCurrent;
				if ("verticalMainDividerStatus" in preferences)
					this.verticalMainDividerStatus = preferences.verticalMainDividerStatus;
										
				if ("debugPort" in preferences)
					this.debugPort = preferences.debugPort;
				if ("deviceUUID" in preferences)
					this.deviceUUID = preferences.deviceUUID;
					
				if ("featureIndex" in preferences)
					this.currentFeature = this.features[preferences.featureIndex];
				if ("history" in preferences)
					this.history = preferences.history
				if ("home" in preferences)
					this.home = preferences.home
				if ("projectsDirectory" in preferences)
					this.projectsDirectory = preferences.projectsDirectory
				if ("projectsDomain" in preferences)
					this.projectsDomain = preferences.projectsDomain
				if ("samplesDirectory" in preferences)
					this.samplesDirectory = preferences.samplesDirectory
				if ("url" in preferences) {
					if (Files.exists(preferences.url))
						this.url = preferences.url;
				}
				if ("eula" in preferences)
					this.eula = preferences.eula
				if ("createSoftwareUpdatePreRelease" in preferences)
					this.createSoftwareUpdatePreRelease = preferences.createSoftwareUpdatePreRelease;
				if ("elementSoftwareUpdatePreRelease" in preferences)
					this.elementSoftwareUpdatePreRelease = preferences.elementSoftwareUpdatePreRelease
					
				if ("breakpoints" in preferences) {
					this.breakpoints.expanded = preferences.breakpoints.expanded;
					this.breakpoints.items = preferences.breakpoints.items;
				}
			}
			this.features.forEach(feature => feature.read(preferences));
		}
		catch(e) {
		}
	}
	writePreferences() {
		try {
			var url = mergeURI(Files.preferencesDirectory, this.preferences);
			var content;
			var preferences = {
				windowState: shell.windowState,
				arrangement: this.arrangement,
				featureDividerCurrent: this.FEATURE_DIVIDER.behavior.current,
				featureDividerStatus: this.FEATURE_DIVIDER.behavior.status,
				horizontalMainDividerCurrent: (content = this.HORIZONTAL_MAIN_DIVIDER) ? content.behavior.current : this.horizontalMainDividerCurrent,
				horizontalMainDividerStatus: (content = this.HORIZONTAL_MAIN_DIVIDER) ? content.behavior.status : this.horizontalMainDividerStatus,
				verticalMainDividerCurrent: (content = this.VERTICAL_MAIN_DIVIDER) ? content.behavior.current : this.verticalMainDividerCurrent,
				verticalMainDividerStatus: (content = this.VERTICAL_MAIN_DIVIDER) ? content.behavior.status : this.verticalMainDividerStatus,
				debugPort: this.debugPort,
				deviceUUID: this.deviceUUID,
				featureIndex: this.features.indexOf(this.currentFeature),
				breakpoints: this.breakpoints,
				history: this.history,
				home: this.home,
				projectsDirectory: this.projectsDirectory,
				projectsDomain: this.projectsDomain,
				samplesDirectory: this.samplesDirectory,
				url: this.url,
				eula: this.eula,
				createSoftwareUpdatePreRelease: this.createSoftwareUpdatePreRelease,
				elementSoftwareUpdatePreRelease: this.elementSoftwareUpdatePreRelease,
			};
			this.features.forEach(feature => feature.write(preferences));
			Files.deleteFile(url);
			trace("writing preferences " + url + "\n");
			Files.writeText(url, JSON.stringify(preferences, null, 4));
		}
		catch(e) {
		}
	}
};

// TEMPLATES

var Window = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:new Skin({ fill:"white" }),
	contents: [
		FeaturesColumn($, { left:0, width:60, top:0, bottom:0}),
		Layout($, {
			left:60, right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior,
			contents: [
				Container($, { 
					anchor:"FEATURE", left:0, width:0, top:0, bottom:0,
					contents: [
						($.currentFeature.container) ? $.currentFeature.container : new $.currentFeature.Template($.currentFeature)
					]
				}),
				($.arrangement) ? HorizontalLayout($, { width:0 }) : VerticalLayout($, { width:0 }),
				VerticalDivider($, { 
					anchor:"FEATURE_DIVIDER", left:$.featureDividerStatus ? $.featureDividerCurrent - 3 : 317, width:6, 
					before:320, current:$.featureDividerCurrent, after:320, status:$.featureDividerStatus,
				}),
			],
		}),
	],
}));

var HorizontalLayout = Layout.template($ => ({
	right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior,
	contents: [
		Container($, { 
			anchor:"MAIN", left:0, right:0, top:0, height:0,
		}),
		Container($, { 
			anchor:"CONSOLE", left:0, right:0, height:0, bottom:0,
		}),
		HorizontalDivider($, {  
			anchor:"HORIZONTAL_MAIN_DIVIDER",  width:6, bottom:$.horizontalMainDividerStatus ? $.horizontalMainDividerCurrent - 3 : 37, 
			before:160, current:$.horizontalMainDividerCurrent, after:40, status:$.horizontalMainDividerStatus,
		}),
	],
}));

var VerticalLayout = Layout.template($ => ({
	right:0, top:0, bottom:0, Behavior:DividerLayoutBehavior,
	contents: [
		Container($, { 
			anchor:"MAIN", left:0, width:0, top:0, bottom:0,
		}),
		Container($, { 
			anchor:"CONSOLE", width:0, right:0, top:0, bottom:0,
		}),
		VerticalDivider($, {  
			anchor:"VERTICAL_MAIN_DIVIDER", width:6, right:$.verticalMainDividerStatus ? $.verticalMainDividerCurrent - 3 : 157, 
			before:160, current:$.verticalMainDividerCurrent, after:240, status:$.verticalMainDividerStatus,
		}),
	],
}));

import {
	About,
	Eula,
} from "eula";

export var ModalContainer = Content.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	Behavior:ModalContainerBehavior,
}));

export class ModalContainerBehavior extends Behavior {
	canAbout() {
		return false;
	}
	canPreferences() {
		return false;
	}
	canNewFile() {
		return false;
	}
	canNewDirectory() {
		return false;
	}
	canOpenFile() {
		return false;
	}
	canOpenDirectory() {
		return false;
	}
	canClose() {
		return false;
	}
	canCloseAll() {
		return false;
	}
	canSaveAll() {
		return false;
	}
	canAbort() {
		return false;
	}
	canGo() {
		return false;
	}
	canRun() {
		return false;
	}
	canStep() {
		return false;
	}
	canStepIn() {
		return false;
	}
	canStepOut() {
		return false;
	}
	canClearAllBreakpoints() {
		return false;
	}
	canView() {
		return false;
	}
	onDisplaying(content) {
		content.focus();
	}
}


shell.menus = [
	{ 
		title: "File",
		items: [
			{ title: "New File...", key: "N", command: "NewFile" },
			{ title: "New Project...", key: "Shift+N", command: "NewDirectory" },
			null,
			{ title: "Open File...", key: "O", command: "OpenFile" },
			{ title: "Open Project...", key: "Shift+O", command: "OpenDirectory" },
			null,
			{ title: "Close", key: "W", command: "Close" },
			{ title: "Close All", key: "Alt+W", command: "CloseAll" },
			null,
			{ title: "Save", key: "S", command: "Save" },
			{ title: "Save All", key: "Alt+S", command: "SaveAll" },
			{ title: "Revert to Saved", command: "Revert" },
			null,
			{ title: "Exit", key: "Q", command: "Quit"},
		],
	},
	{ 
		title: "Edit",
		items: [
			{ title: "Undo", key: "Z", command: "Undo" },
			{ title: "Redo", key: "Shift+Z", command: "Redo" },
			null,
			{ title: "Cut", key: "X", command: "Cut" },
			{ title: "Copy", key: "C", command: "Copy" },
			{ title: "Paste", key: "V", command: "Paste" },
			{ title: "Clear", command: "Clear" },
			null,
			{ title: "Select All", key: "A", command: "SelectAll" },
			null,
			{ title: "Find", key: "F", command: "Find" },
			{ title: "Find Next", key: "G", command: "FindNext" },
			{ title: "Find Previous", key: "Shift+G", command: "FindPrevious" },
			{ title: "Find Selection", key: "E", command: "FindSelection" },
			null,
			{ title: "Replace", key: "Alt+F", command: "Replace" },
			{ title: "Replace Next", key: "Alt+G", command: "ReplaceNext" },
			{ title: "Replace Previous", key: "Alt+Shift+G", command: "ReplacePrevious" },
			null,
			{ title: "Preferences", key: ",", command: "Preferences"},
		],
	},
	{ 
		title: "Debug",
		items: [
			{ title: "Kill", key: "K", command: "Abort" },
			null,
			{ title: "Run", key: "R", command: "Go" },
			{ title: "Step", key: "T", command: "Step" },
			{ title: "Step In", key: "I", command: "StepIn" },
			{ title: "Step Out", key: "O", command: "StepOut" },
			null,
			{ title: "Set Breakpoint", titles: ["Set Breakpoint", "Clear Breakpoint"], key: "B", command: "ToggleBreakpoint" },
			{ title: "Clear All Breakpoints", key: "Alt+B", command: "ClearAllBreakpoints" },
		],
	},
	{ 
		title: "View",
		items: [
			
		],
	},
	{
		title: "Help",
		items: [
			{ title: "Kinoma Developer", command: "Support" },
			null,
			{ title: "About Kinoma Code", command: "About"},
		],
	},
];
shell.behavior = new ShellBehavior(shell, {});
shell.acceptFiles = true;
export var model;
