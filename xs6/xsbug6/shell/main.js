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

import DebugServer from "common/DebugServer";
import DesktopShellBehavior from "common/desktop";

import { 
	DividerLayoutBehavior,
	HorizontalDivider,
	VerticalDivider,
} from "common/divider";

import {
	noCodeSkin
} from "assets";

import {
	CodeView,
	ErrorView,
} from "code";

import { 
	ConsolePane,
} from "console";

import {
	DebugPane,
} from "debug";

import {
	FilePane,
} from "files";

import {
	PreferencesView,
} from "preferences";

import {
	TabsPane,
} from "tabs";


export const mxFramesView = 0;
export const mxLocalsView = 1;
export const mxGlobalsView = 2;
export const mxFilesView = 3;
export const mxBreakpointsView = 4;
export const mxGrammarsView = 5;
export const mxFileView = 6;
export const mxLogView = 7;

class Home {
	constructor(url) {
		let parts = parseURI(url.slice(0, -1));
		this.depth = 0;
		this.expanded = true;
		this.items = [];
		this.name = parts.name;
		this.url = url;
		this.initialize();
	}
	close() {
		if (this.folderNotifier)
			this.folderNotifier.close();
	}
	initialize() {
		let url = this.url.slice(0, 0 - this.name.length - 1);
		this.directoryNotifier = new Files.DirectoryNotifier(url, url => {
			this.onDirectoryChanged(url);
		});
	}
	onDirectoryChanged() {
		if (!Files.exists(this.url))
			model.doCloseDirectory(this.url);
	}
	toJSON() {
		return {
			url: this.url,
			name: this.name,
			expanded: this.expanded,
			depth: this.depth,
			items: this.items,
		};
	}
};

class Machine {
	constructor(address) {
		this.address = address;
		this.broken = false;
		this.once = false;
		this.running = true;
		this.timeout = 0;
		this.views = [
			new View("CALLS"),
			new View("LOCALS"),
			new View("GLOBALS"),
			new View("FILES"),
			new View("BREAKPOINTS"),
			new View("MODULES"),
			new View("FILE"),
			new View("CONSOLE"),
		];
		this.localsView.exceptions = {
			"(return)":"0",
			"new.target":"1",
			"(function)":"2",
			"this":"3",
		};
		this.ip = address.slice(0, address.lastIndexOf(":"));
		this.tag = "";
		this.title = "";
		
		this.url = "";
		this.at = 0;

		this.codeScroll = { x:0, y:0 };
		
		this.consoleScroll = { x:0, y:0 };
		this.debugScroll = { x:0, y:0 };
	}
	get closable() {
		return !this.broken;
	}
	get framesView() {
		return this.views[0];
	}
	get localsView() {
		return this.views[1];
	}
	get globalsView() {
		return this.views[2];
	}
	get filesView() {
		return this.views[3];
	}
	get breakpointsView() {
		return this.views[4];
	}
	get modulesView() {
		return this.views[5];
	}
	get fileView() {
		return this.views[6];
	}
	get logView() {
		return this.views[7];
	}
};

class View {
	constructor(title) {
		this.exceptions = null;
		this.expanded = false;
		this.lineIndex = -1;
		this.lines = [];
		this.path = null;
		this.title = title;
	}
	empty() {
		this.expanded = false;
		this.lineIndex = -1;
		this.lines = [];
	}
};

class DebugBehavior extends DesktopShellBehavior {
	onCreate(shell, data) {
		super.onCreate(shell, data);
		this.interfaces = {};
		this.port = 5002;
		this.machines = [];
		this.currentMachine = null;
		this.consoleLines = [];
		this.consoleScroll = { x:0, y:0 };
		this.breakOnStart = 0;
		this.breakOnExceptions = 1;
		this.sortingRegexps = [
			/(\[)([0-9]+)(\])/,
			/(\(\.)([0-9]+)(\))/,
			/(\(\.\.)([0-9]+)(\))/,
			/(arg\()([0-9]+)(\))/,
			/(var\()([0-9]+)(\))/,
		];
		this.sortingZeros = "0000000000";
	}
	closeServer() {
		this.machines.forEach(machine => this.debug.abort(machine.address));
		this.debug.close();
		delete this.debug;
	}
	doClearConsole(command) {
		let machine = this.currentMachine;
		if (machine)
			machine.logView.empty();
		else
			this.consoleLines = [];
	}
	doDebugCommand(command) {
		let machine = this.currentMachine;
		if (command != "abort") {
			machine.broken = false;
			machine.running = false;
			machine.timeout = Date.now() + 500;
		}
		this.debug[command](machine.address);
	}
	doDebugFile(viewIndex, path, line, value) {
		this.debug.file(this.currentMachine.address, viewIndex, path, line, value);
	}
	doDebugToggle(viewIndex, name, value) {
		this.debug.toggle(this.currentMachine.address, viewIndex, name, value);
	}
	doToggleBreakpoint(url, line) {
		let breakpoints = this.breakpoints.items;
		let path = Files.toPath(url);
		let index = breakpoints.findIndex(breakpoint => (breakpoint.url == url) && (breakpoint.line == line));
		if (index >= 0) {
			breakpoints.splice(index, 1);
			this.machines.forEach(machine => this.debug.removeBreakpoint(machine.address, path, line));
		}
		else {
			var parts = parseURI(url);
			breakpoints.push({ url, name:parts.name, line });
			breakpoints.sort(this.sortBreakpoints);
			this.machines.forEach(machine => this.debug.addBreakpoint(machine.address, path, line));
		}
		shell.distribute("onBreakpointsChanged");
	}
	findMachine(address) {
		return this.machines.find(machine => machine.address == address);
	}
	findMachineIndex(address) {
		return this.machines.findIndex(machine => machine.address == address);
	}
	openServer() {
		this.debug = new DebugServer({ port:this.port });
  		this.debug.behavior = this;
	}
	runMachine(machine) {
		machine.framesView.empty();
		machine.localsView.empty();
		machine.modulesView.empty();
		machine.globalsView.empty();
		machine.running = true;
		shell.distribute("onMachineChanged", machine);
		if (machine == this.currentMachine) {
			shell.distribute("onMachineViewChanged", mxFramesView);
			shell.distribute("onMachineViewChanged", mxLocalsView);
			shell.distribute("onMachineViewChanged", mxGrammarsView);
			shell.distribute("onMachineViewChanged", mxGlobalsView);
		}
	}
	selectMachine(machine) {
		if (this.currentMachine != machine) {
			shell.distribute("onMachineDeselected", this.currentMachine);
			let container = this.FEATURE;
			if (!this.currentMachine)
				container.replace(container.first, new DebugPane(this));
			else if (!machine)
				container.replace(container.first, new FilePane(this));
			this.currentMachine = machine
			shell.distribute("onMachineSelected", machine);
		}
	}
	sortBreakpoints(a, b) {
		let result = a.url.compare(b.url);
		if (!result)
			result = a.line - b.line;
		return result;
	}
	sortLines(view) {
		let former = { column:-1, parent:null, path:null };
		let exceptions = view.exceptions;
		let lines = view.lines;
		let zeros = this.sortingZeros;
		let regexps = this.sortingRegexps;
		let c = regexps.length;
		lines.forEach(line => {
			while (line.column <= former.column)
				former = former.parent;
			line.parent = former;
			let name = line.name;
			for (let i = 0; i < c; i++) {
				let results = regexps[i].exec(name);
				if (results) {
					let result = results[2];
					name = results[1] + zeros.slice(0, -result.length) + result + results[3];
					break;
				}
			}
			let path = former.path;
			if (path)
				line.path = path + "." + name;
			else if (exceptions && (name in exceptions))
				line.path = exceptions[name];
			else
				line.path = name;
			former = line;
		});
		lines.sort((a, b) => {
			return a.path.compare(b.path);
		});
	}
	toggleBreakOnExceptions(it) {
		this.breakOnExceptions = it;
		if (it)
			this.machines.forEach(machine => this.debug.addBreakpoint(machine.address, "exceptions", 0));
		else
			this.machines.forEach(machine => this.debug.removeBreakpoint(machine.address, "exceptions", 0));
	}
	toggleBreakOnStart(it) {
		this.breakOnStart = it;
	}
	onMachineBroken(address) {
		let machine = this.findMachine(address);
		machine.broken = true;
		shell.distribute("onMachineViewChanged", mxFramesView);
	}
	onMachineDone(address) {
		let machine = this.findMachine(address);
		if (machine.once) {
			machine.once = false;
			this.debug.addBreakpoints(address, this.breakpoints.items.map(item => ({ path:Files.toPath(item.url), line:item.line })), this.breakOnStart, this.breakOnExceptions);
		}
		else if (machine.broken) {
			machine.framesView.expanded = true;
			var lineIndex = machine.framesView.lineIndex;
			if ((lineIndex < 0) || (machine.framesView.lines.length <= lineIndex))
				machine.framesView.lineIndex = 0;
			machine.localsView.expanded = true;
			shell.distribute("onMachineChanged", machine);
			if (!this.currentMachine || !this.currentMachine.broken)
				this.selectMachine(machine);
		}
		else
			this.debug.go(address);
	}
	onMachineFileChanged(address, path, at) {
		// trace("onMachineFileChanged " + path + " " + at + "\n");
		let machine = this.findMachine(address);
		if (!machine) return;
		machine.url = (path) ? Files.toURI(path) : "";
		machine.at = at;
		if (this.currentMachine == machine) {
			if (machine.url)
				this.doOpenURL(machine.url, at);
		}
	}
	onMachineRegistered(address) {
		// trace("onMachineRegistered " + address + "\n");
		let machine = new Machine(address);
		this.machines.unshift(machine);
		shell.distribute("onMachinesChanged", this.machines);
	}
	onMachineTitleChanged(address, title, tag) {
		// trace("onMachineTitleChanged " + address + " " + title + "\n");
		let machine = this.findMachine(address);
		machine.tag = tag;
		machine.title = title;
		machine.visible = true;
		shell.distribute("onMachinesChanged", this.machines);
		machine.broken = false;
		machine.once = true;
		this.runMachine(machine);
	}
	onMachineUnregistered(address) {
		// trace("onMachineUnregistered " + address + "\n");
		let index = this.findMachineIndex(address);
		if (index < 0) return;
		let machine = this.machines.splice(index, 1)[0];
		shell.distribute("onMachinesChanged", this.machines);
		if (this.currentMachine == machine)
			this.selectMachine(this.machines.find(machine => machine.broken));
	}
	onMachineViewChanged(address, viewIndex, lines) {
		let machine = this.findMachine(address);
		if (!machine) return;
		let view = machine.views[viewIndex];
		view.lines = lines;
		// trace("onMachineViewChanged " + machine.address + " " + viewIndex + " " + lines.length + " " + machine.broken + "\n");
		view.lines = lines;
		if ((viewIndex == mxLocalsView) || (viewIndex == mxGrammarsView) || (viewIndex == mxGlobalsView))
			this.sortLines(view);
		if (this.currentMachine == machine)
			shell.distribute("onMachineViewChanged", viewIndex);
	}
	onMachineViewPrint(address, viewIndex, line) {
		let machine = this.findMachine(address);
		if (!machine) return;
		let view = machine.views[viewIndex];
		view.lines.push(line);
		this.consoleLines.push(line);
		if ((this.currentMachine == machine) || (this.currentMachine == null))
			model.doLog(shell, line);
	}
	onPortChanged(shell) {
		this.closeServer();
		this.openServer();
	}
	onTimeChanged() {
		let now = Date.now();
		let machines = this.machines;
		let c = machines.length;
		for (let i = 0; i < c; i++) {
			let machine = machines[i];
			if ((!machine.broken) && (!machine.running) && (machine.timeout <= now)) {
				this.runMachine(machine);
			}
		}
	}
};

class ShellBehavior extends DebugBehavior {

	checkForUpdate() {
// 		let message = new Message("https://auth.developer.cloud.kinoma.com/kinoma-device-update?target=KINOMA_CODE_MAC_RELEASE");
// 		message.setRequestHeader("Accept", "application/json");
// 		message.setRequestCertificate(updateCredentials);
// 		message.invoke(Message.JSON).then(result => {
// 			if (result && result.url && result.ver && (result.ver != getEnvironmentVariable("VERSION"))) {
// 				system.alert({ 
// 					type:"note", 
// 					prompt:"A new version of Kinoma Code is available!",
// 					info:"Kinoma Code " + result.ver + " is now available. Would you like to download it now?",
// 					buttons:["Download", "Cancel"]
// 				}, ok => {
// 					if (ok === undefined)
// 						return;
// 					if (ok)
// 						launchURI(result.url);
// 				});
// 			}
// 		});
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
				this.doOpenDirectoryCallback(url);
			else
				this.doOpenFileCallback(url);
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
		shell.alert("about", "xsbug", "Copyright © 2016 Marvell. All rights reserved. Kinoma is a registered trademark of Kinoma, Inc."); 
	}
	doPreferences() {
		this.doOpenURL("preferences://");
	}

/* FILE MENU */
	canOpenFile() {
		return true;
	}
	canOpenDirectory() {
		return true;
	}
	canCloseFile() {
		return this.url ? true : false;
	}
	canCloseFiles() {
		return this.url ? true : false;
	}
	canQuit() {
		return true;
	}
	doOpenDirectory() {
		system.openDirectory({ prompt:"Open Folder", url:Files.documentsDirectory }, url => { if (url) this.doOpenDirectoryCallback(url); });
	}
	doOpenDirectoryCallback(url) {
		let items = this.homes.items;
		let home = items.find(item => item.url == url);
		if (!home) {
			home = new Home(url);
			items.push(home);
			items.sort((a, b) => a.name.compare(b.name));
			shell.distribute("onHomesChanged");
		}
		return home;
	}
	doOpenFile() {
		system.openFile({ prompt:"Open File", url:Files.documentsDirectory }, url => { if (url) this.doOpenFileCallback(url); });
	}
	doOpenFileCallback(url) {
		if (url.endsWith(".js") || url.endsWith(".json") || url.endsWith(".xml") || url.endsWith(".xs"))
			this.doOpenURL(url);
	}
	doOpenURL(url, at) {
		//trace("### doOpenURL " + url + "\n");
		if (this.url != url) {
			let items = this.history.items;
			let index = items.findIndex(item => item.url == url);
			if (index >= 0)
				items.splice(index, 1);
			if (this.url && this.url.startsWith("file://")) {
				let parts = parseURI(this.url);
				items.unshift({ url:this.url, name:parts.name });
				if (items.length > 32)
					items.length = 32;
			}
			shell.distribute("onHistoryChanged");
			this.url = url;
			this.at = at;
			this.doOpenView();
			shell.distribute("onURLChanged", url);
		}
		else if (this.at != at) {
			this.at = at;
			shell.distribute("doSelectLine", at);
		}
	}
	doOpenView() {
		let Template = ErrorView;
		let url = this.url;
		if (url) {
			if (url == "preferences://")
				Template = PreferencesView;
			else if (Files.exists(url))
				Template = CodeView;
			else {
				for (let mapping of this.mappings) {
					if (url.startsWith(mapping.remote)) {
						url = mapping.locale.concat(url.slice(mapping.remote.length));
						if (Files.exists(url)) {
							this.url = url;
							Template = CodeView;
						}
					}
				}
			}
		}
		else
			Template = NoCodePane;
		this.MAIN.replace(this.MAIN.first, new Template(this));
	}
	doCloseDirectory(url) {
		let items = this.homes.items;
		let index = items.findIndex(item => item.url == url);
		if (index >= 0) {
			items[index].close();
			items.splice(index, 1);
			shell.distribute("onHomesChanged");
		}
	}
	doCloseFile() {
		this.doCloseURL(this.url);
	}
	doCloseFiles() {
		this.doCloseURL(null);
	}
	doCloseURL(url) {
		trace("### doCloseURL " + url + "\n");
		let items = this.history.items;
		if (url) {
			if (this.url == url) {
				if (items.length) {
					let item = items.shift();
					url = this.url = item.url;
					this.at = undefined;
					shell.distribute("onHistoryChanged");
				}
				else {
					url = this.url = undefined;
					this.at = undefined;
				}
				this.doOpenView();
				shell.distribute("onURLChanged", url);
			}
		}
		else {
			items.length = 0;
			this.url = undefined;
			this.at = undefined;
			this.doOpenView();
			shell.distribute("onURLChanged", url);
		}
	}
	doQuit(shell) {
		shell.quit();
	}
	doUpdateFile(url) {
		if (this.url == url) {
			let at = this.at;
			this.doCloseURL(this.url);
			this.doOpenURL(url, at);
		}
	}

/* DEBUG MENU */
	canAbort() {
		let machine = this.currentMachine;
		return machine ? true : false;
	}
	canBreak() {
		let machine = this.currentMachine;
		return machine && !machine.broken;
	}
	canClearAllBreakpoints() {
		let breakpoints = this.breakpoints.items;
		return breakpoints.length > 0;
	}
	canGo() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStep() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStepIn() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	canStepOut() {
		let machine = this.currentMachine;
		return machine && machine.broken;
	}
	
	doAbort() {
		let machine = this.currentMachine;
		this.doDebugCommand("abort");
	}
	doBreak() {
		this.doDebugCommand("step");
	}
	doClearAllBreakpoints() {
		this.breakpoints.items = [];
		this.machines.forEach(machine => this.debug.resetBreakpoints(machine.address));
		shell.distribute("onBreakpointsChanged");
	}
	doGo() {
		this.doDebugCommand("go");
	}
	doStep() {
		this.doDebugCommand("step");
	}
	doStepIn() {
		this.doDebugCommand("stepIn");
	}
	doStepOut() {
		this.doDebugCommand("stepOut");
	}

/* HELP MENU */
	canSupport() {
		return true;
	}
	doSupport() {
		launchURI("http://kinoma.com/develop/"); 
	}


	doLog(shell, text) {
		var console = this.CONSOLE.first;
		console.behavior.doLog(console, text);
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

	onCreate() {
		super.onCreate(shell, data);
  		shell.interval = 100;
  		shell.start();

		model = this;

		this.arrangement = true;
		this.featureDividerCurrent = 320;
		this.featureDividerStatus = true;
		this.HORIZONTAL_MAIN_DIVIDER = null;
		this.horizontalMainDividerCurrent = 240;
		this.horizontalMainDividerStatus = true;
		this.VERTICAL_MAIN_DIVIDER = null;
		this.verticalMainDividerCurrent = 240;
		this.verticalMainDividerStatus = true;
		
		this.findHint = "FIND";
		this.findMode = 1;
		this.findString = "";
		
		this.breakpoints = {
			expanded: true,
			items: [],
		};
		this.errors = {
			expanded: true,
			items: [],
		};
		this.history = {
			expanded: true,
			items: [],
		};
		this.homes = {
			expanded: true,
			items: [],
		};
		this.mappings = [];
		this.search = {
			expanded:false,
			findHint:"SEARCH",
			findMode:1,
			findString:"",
			items:[],
			message:null,
		}
		
		this.url = undefined;
		this.at = undefined;
		
		this.preferences = "xsbug.json";
		this.readPreferences();

		let window = new Window(this);
		shell.add(window);
		
		this.MAIN.add(new Content);
		this.CONSOLE.add(ConsolePane(this, {}));
		
		shell.updateMenus();
		shell.windowTitle = "xsbug";
		this.openServer();
		if (this.url)
			this.MAIN.replace(this.MAIN.first, new CodeView(this));
	}
	onQuit() {
		this.closeServer();
		this.writePreferences();
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
				if ("breakOnExceptions" in preferences)
					this.breakOnExceptions = preferences.breakOnExceptions;
				if ("breakOnStart" in preferences)
					this.breakOnStart = preferences.breakOnStart;
				if ("breakpoints" in preferences) {
					this.breakpoints.expanded = preferences.breakpoints.expanded;
					preferences.breakpoints.items.forEach(item => { 
						if (Files.exists(item.url))
							this.breakpoints.items.push(item);
					});
				}
				if ("history" in preferences) {
					this.history.expanded = preferences.history.expanded;
					preferences.history.items.forEach(item => { 
						if (Files.exists(item.url))
							this.history.items.push(item);
					});
				}
				if ("homes" in preferences) {
					preferences.homes.items.forEach(item => { 
						if (Files.exists(item.url)) {
							Object.setPrototypeOf(item, Home.prototype);
							item.initialize();
							this.homes.items.push(item);
						}
					});
				}
				if ("mappings" in preferences)
					this.mappings = preferences.mappings;
				if ("port" in preferences)
					this.port = preferences.port;
				if ("url" in preferences) {
					if (Files.exists(preferences.url))
						this.url = preferences.url;
				}
			}
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
				breakOnExceptions: this.breakOnExceptions,
				breakOnStart: this.breakOnStart,
				breakpoints: this.breakpoints,
				history: this.history,
				homes: this.homes,
				mappings: this.mappings,
				port: this.port,
				url: this.url,
			};
			Files.deleteFile(url);
			Files.writeText(url, JSON.stringify(preferences, null, 4));
		}
		catch(e) {
		}
	}
	
};

// TEMPLATES

var Window = Container.template($ => ({
	anchor:"WINDOW", left:0, right:0, top:0, bottom:0, skin:new Skin({ fill:"white" }),
	contents: [
		TabsPane($, {}),
		Layout($, {
			left:0, right:0, top:27, bottom:0, Behavior:DividerLayoutBehavior,
			contents: [
				Container($, { 
					anchor:"FEATURE", left:0, width:0, top:0, bottom:0,
					contents: [
						FilePane($, {}),
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
			contents: [
				NoCodePane($, {}),
			]
		}),
		Container($, { 
			anchor:"CONSOLE", left:0, right:0, height:0, bottom:0,
		}),
		HorizontalDivider($, {  
			anchor:"HORIZONTAL_MAIN_DIVIDER",  width:6, bottom:$.horizontalMainDividerStatus ? $.horizontalMainDividerCurrent - 3 : 23, 
			before:160, current:$.horizontalMainDividerCurrent, after:26, status:$.horizontalMainDividerStatus,
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

var NoCodePane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:noCodeSkin,
	contents: [
	],
}));

shell.menus = [
	{ 
		title: "File",
		items: [
			{ title: "Open File...", key: "O", command: "OpenFile" },
			{ title: "Open Folder...", key: "Shift+O", command: "OpenDirectory" },
			null,
			{ title: "Close", key: "W", command: "CloseFile" },
			{ title: "Close All", key: "Alt+W", command: "CloseFiles" },
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
			{ title: "Preferences", key: ",", command: "Preferences"},
		],
	},
	{ 
		title: "Debug",
		items: [
			{ title: "Kill", key: "K", command: "Abort" },
			{ title: "Break", key: "B", command: "Break" },
			null,
			{ title: "Run", key: "R", command: "Go" },
			{ title: "Step", key: "T", command: "Step" },
			{ title: "Step In", key: "I", command: "StepIn" },
			{ title: "Step Out", key: "O", command: "StepOut" },
			null,
			{ title: "Set Breakpoint", titles: ["Set Breakpoint", "Clear Breakpoint"], key: "Shift+B", command: "ToggleBreakpoint" },
			{ title: "Clear All Breakpoints", key: "Alt+B", command: "ClearAllBreakpoints" },
		],
	},
	{
		title: "Help",
		items: [
			{ title: "Kinoma Developer", command: "Support" },
			null,
			{ title: "About xsbug", command: "About"},
		],
	},
];
shell.behavior = new ShellBehavior(shell, {});
shell.acceptFiles = true;
export var model;
