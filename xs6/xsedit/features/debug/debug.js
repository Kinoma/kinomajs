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

var defaultIconSkin = new Skin({ texture:new Texture("./assets/defaultIcon.png", 1), x:0, y:0, width:60, height:60 });
var xs6IconSkin = new Skin({ texture:new Texture("./assets/xs6Icon.png", 1), x:0, y:0, width:60, height:60 });

export default class extends Feature {
	constructor(model) {
		super(model, "Debugger");
		this.Template = DebugPane;
		this.iconSkin = new Skin({ texture:new Texture("./icon.png", 2), x:0, y:0, width:60, height:60, states:60 });
		this.machines = [];
		this.debuggees = [];
		this.currentMachine = null;
		this.sortingRegexps = [
			/(\[)([0-9]+)(\])/,
			/(\(\.)([0-9]+)(\))/,
			/(\(\.\.)([0-9]+)(\))/,
			/(arg\()([0-9]+)(\))/,
			/(var\()([0-9]+)(\))/,
		];
		this.sortingZeros = "0000000000";
	}

	canAbort() {
		let machine = this.currentMachine;
		return machine ? true : false;
	}
	canClearAllBreakpoints() {
		let breakpoints = this.model.breakpoints.items;
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
		let device = machine.device;
		if (device) {
			this.doDebugCommand("logout");
			tool.abort(device, this);
		}
		else
			this.doDebugCommand("abort");
	}
	doClearAllBreakpoints() {
		this.model.breakpoints.items = [];
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
	
	close() {
		this.machines.forEach(machine => {
			if (machine.broken)
				this.debug.abort(machine.address);
		});
		this.debug.close();
		delete this.debug;
		delete this.port;
		delete this.host;
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
		let breakpoints = this.model.breakpoints.items;
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
	idle(now) {
		let data = this.data;
		let currentMachine = this.currentMachine;
		let machines = this.machines;
		let c = machines.length;
		for (let i = 0; i < c; i++) {
			let machine = machines[i];
			if ((!machine.broken) && (!machine.running) && (machine.timeout <= now)){
				machine.views.forEach((view, index) => {
					view.empty();
				});
				machine.running = true;
				this.notify();
				if (machine == currentMachine)
					shell.distribute("onMachineSelected", machine);
			}
		}
	}
	open() {
		this.host = "localhost";
		this.port = model.debugPort;
		this.debug = new KPR.Debug({ port:this.port });
  		this.debug.behavior = this;
	}
	notify() {
		let flag = false;
		if (!this.container) {
			let machines = this.machines;
			let c = machines.length;
			for (let i = 0; i < c; i++) {
				if (machines[i].broken) {
					flag = true;
					break;
				}
			}	
		}
		this.dirty = flag;
		shell.distribute("onFeatureChanged", this);
	}
	selectMachine(machine) {
		if (machine && machine.device && !machine.visible) {
			let device = machine.device;
			machine = this.machines.find(machine => (machine.device == device) && (machine.visible));
		}
		this.currentMachine = machine
		shell.distribute("onMachineSelected", machine);
	}
	selectMachineByDevice(device) {
		this.selectMachine(this.machines.find(machine => machine.device == device));
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
	sortFiles(items) {
		if (items) {
			items.forEach(item => this.sortFiles(item.items));
			items.sort((a, b) => a.name.compare(b.name));
		}
	}
	spreadFiles(view, lines) {
		let homePath = Files.toPath(model.home);
		let homeLength = homePath.length;
		let projectsPath = Files.toPath(model.projectsDirectory);
		let projectsLength = projectsPath.length;
		let samplesPath = Files.toPath(model.samplesDirectory);
		let samplesLength = samplesPath.length;
		
		let homeFolder = view.homeFolder;
		if (!homeFolder)
			homeFolder = view.homeFolder = { Template:FolderTable, depth:1, name:"KinomaJS", expanded:true, items:[] }
		let projectsFolder = view.projectsFolder;
		if (!projectsFolder)
			projectsFolder = view.projectsFolder = { Template:FolderTable, depth:1, name:"Projects", expanded:true, items:[] }
		let samplesFolder = view.samplesFolder;
		if (!samplesFolder)
			samplesFolder = view.samplesFolder = { Template:FolderTable, depth:1, name:"Samples", expanded:true, items:[] }
		
		let homeItems = homeFolder.items;
		let projectsItems = projectsFolder.items;
		let samplesItems = samplesFolder.items;
		
		lines.forEach(line => {
			let path = line.path, items = null;
			if (path.startsWith(homePath)) {
				path = path.slice(homeLength);
				items = homeItems;
			}
			else if (path.startsWith(projectsPath)) {
				path = path.slice(projectsLength);
				items = projectsItems;
			}
			else if (path.startsWith(samplesPath)) {
				path = path.slice(samplesLength);
				items = samplesItems;
			}
			if (items) {
				let names = path.split("/");
				let i = 0, j = names.length - 1;
				while (i < j)  {
					let name = names[i];
					let folder = items.find(item => item.name == name);
					if (!folder) {
						folder = { Template:FolderTable, depth:i + 2, name, expanded:false, items:[] },
						items.push(folder);
					}
					items = folder.items;
					i++;
				}
				let name = names[i];
				let file = items.find(item => item.name == name);
				if (!file) {
					file = { Template:FileLine, depth:i + 2, name, url:Files.toURI(line.path) },
					items.push(file);
				}
			}
		});
		let items = [];
		if (homeItems.length) {
			this.sortFiles(homeItems);
			items.push(homeFolder);
		}
		if (projectsItems.length) {
			this.sortFiles(projectsItems);
			items.push(projectsFolder);
		}
		if (samplesItems.length) {
			this.sortFiles(samplesItems);
			items.push(samplesFolder);
		}
		return items;
	}
	
	onMachineFileChanged(address, path, at) {
		trace("onMachineFileChanged " + path + " " + at + "\n");
		let machine = this.findMachine(address);
		if (this.currentMachine == machine) {
			if (path)
				shell.delegate("doOpenURL", Files.toURI(path), at);
		}
	}
	onMachineRegistered(address) {
		trace("onMachineRegistered " + address + "\n");
		this.machines.unshift(new Machine(address));
		shell.distribute("onMachinesChanged", this.machines, this.debuggees);
		if (this.currentMachine == null)
			this.selectMachine(this.machines[0]);
	}
	onMachineTitleChanged(address, title, tag) {
		trace("onMachineTitleChanged " + address + " " + title + "\n");
		let machine = this.findMachine(address);
		if (!machine) return;
		let kind = "";
		if (title.startsWith("pins @ ")) {
			title = title.slice(7);
			kind = "pins @ ";
		}
		let device = model.devicesFeature.findDeviceByAddressAndTag(address, tag);
		if (device) {
			machine.device = device;
			device.machineCount++;
			machine.iconSkin = device.constructor.iconSkin;
			machine.tag = device.name;
			machine.visible = (title != "SSL");
			if (title.startsWith("xkpr://"))
				title = title.slice(7);
			let project = model.filesFeature.findProjectByID(title);
			if (project)
				title = project.title;
			else if (device.helperID == title)
				title = "Kinoma Code Helper";
		}
		else {
			machine.iconSkin = (tag == "XS6") ? xs6IconSkin : defaultIconSkin;
			machine.tag = tag;
			machine.visible = true;
			if (title.startsWith("xkpr://"))
				title = "application";
		}
		machine.title = kind + title;
		this.debug.addBreakpoints(address, this.model.breakpoints.items.map(item => ({ path:Files.toPath(item.url), line:item.line })));
		this.debuggees = this.machines.filter((machine, pos) => this.machines.findIndex(target => target.tag == machine.tag) == pos);
		shell.distribute("onMachinesChanged", this.machines, this.debuggees);
		if (!this.currentMachine || (this.currentMachine == machine))
			this.selectMachine(machine);
	}
	onMachineUnregistered(address) {
		trace("onMachineUnregistered " + address + "\n");
		let index = this.findMachineIndex(address);
		if (index < 0) return;
		let machine = this.machines.splice(index, 1)[0];
		if (machine.device)
			machine.device.machineCount--;
		this.debuggees = this.machines.filter((machine, pos) => this.machines.findIndex(target => target.tag == machine.tag) == pos);
		shell.distribute("onMachinesChanged", this.machines, this.debuggees);
		if (this.currentMachine == machine)
			this.selectMachine(this.machines.find(target => target.tag == machine.tag));
		this.notify();
	}
	onMachineViewChanged(address, viewIndex, lines) {
		let machine = this.findMachine(address);
		if (!machine) return;
		let view = machine.views[viewIndex];
		view.lines = lines;
		// trace("onMachineViewChanged " + machine.address + " " + viewIndex + " " + lines.length + " " + machine.broken + "\n");
		if (model.home && (viewIndex == mxFilesView)) {
			view.lines = this.spreadFiles(view, lines);
		}
		else
			view.lines = lines;
		if (viewIndex == mxFramesView) {
			machine.broken = true;
			machine.localsView.expanded = true;
			view.expanded = true;
			view.lineIndex = 0;
			this.selectMachine(machine);
			this.notify();
		}
		else if ((viewIndex == mxLocalsView) || (viewIndex == mxGrammarsView) || (viewIndex == mxGlobalsView)) {
			this.sortLines(view);
		}
		if (this.currentMachine == machine)
			shell.distribute("onMachineViewChanged", viewIndex);
	}
	onMachineViewPrint(address, viewIndex, line) {
		let machine = this.findMachine(address);
		if (!machine) return;
		model.doLog(shell, line);
	}
}

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

const mxFramesView = 0;
const mxLocalsView = 1;
const mxGlobalsView = 2;
const mxFilesView = 3;
const mxBreakpointsView = 4;
const mxGrammarsView = 5;
const mxFileView = 6;
const mxLogView = 7;

class Machine {
	constructor(address) {
		this.address = address;
		this.broken = false;
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
			new View("LOG"),
		];
		this.localsView.exceptions = {
			"(return)":"0",
			"new.target":"1",
			"(function)":"2",
			"this":"3",
		};
		this.device = null;
		this.ip = address.slice(0, address.lastIndexOf(":"));
		this.iconSkin = null;
		this.tag = "";
		this.title = "";
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

// ASSETS

import { 
	menuIconSkin,
} from "common/menu";

import {
	BLACK,
	BUTTON_DISABLED,
	BUTTON_ENABLED,
	LIGHT_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	WHITE,
	blackMenuSkin,
	featureEmptyStyle,
	fileGlyphsSkin,
	grayBorderSkin,
	grayHeaderSkin,
	grayFooterSkin,
	grayLineSkin,
	greenHeaderSkin,
	greenFooterSkin,
	greenLineSkin,
	menuHeaderStyle,
	menuLineSkin,
	menuLineStyle,
	orangeHeaderSkin,
	orangeFooterSkin,
	orangeLineSkin,
	tableHeaderStyle,
	tableLineStyle,
	whiteButtonSkin,
	whiteButtonStyle,
} from "shell/assets";

var menuHeaderAddressStyle = new Style({ font:LIGHT_FONT, size:12, color:WHITE, horizontal:"right", right:10 })
var menuLineTitleStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"left" })
var menuLineAddressStyle = new Style({ font:LIGHT_FONT, size:12, color:BLACK, horizontal:"right", right:10 })

var menuHeaderVariantStyle = new Style({ font:NORMAL_FONT, size:12, color:WHITE, horizontal:"left" })
var menuLineVariantStyle = new Style({ font:NORMAL_FONT, size:12, color:BLACK, horizontal:"left" })

var debugButtonsTexture = new Texture("assets/debugButtons.png", 2);
var debugButtonsSkin = new Skin({ texture: debugButtonsTexture, x:0, y:0, width:30, height:30, variants:30, states:30 });

var lineHeight = 16;
var lineFlagsSkin = fileGlyphsSkin;
var fileStyle = new Style({ font:NORMAL_FONT, size:12, color:BLACK, horizontal:"left" });
var nameStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:BLACK });
var valueStyle = new Style({ font:LIGHT_FONT, size:12, color:BLACK });

const machineButtonStyle = new Style({ font:SEMIBOLD_FONT, size:14, horizontal:"left", left:5, right:5, color:[BUTTON_ENABLED, BUTTON_ENABLED, BUTTON_ENABLED, WHITE ] });
const machineItemStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"left", left:5, right:5});

// BEHAVIORS

import { 
	ButtonBehavior, 
} from "common/control";

import {
	bulletSkin,
	MenuButtonBehavior,
	MenuItemBehavior,
} from "common/menu";

import {
	ScrollerBehavior,
	VerticalScrollbar,
} from "common/scrollbar";

import { 
	HolderColumnBehavior,
	HolderContainerBehavior,
	LineBehavior,
	HeaderBehavior,
	TableBehavior,
} from "shell/behaviors";

import { 
	FeaturePaneBehavior,
} from "shell/feature";

class DebugPaneBehavior extends FeaturePaneBehavior {
	onDisplaying(container) {
		let data = this.data;
		container.distribute("onDeviceSelected", model.devicesFeature.currentDevice);
		container.distribute("onMachinesChanged", data.machines, data.debuggees);
		container.distribute("onMachineSelected", data.currentMachine);
		container.distribute("onBreakpointsChanged");
	}
	onMachineSelected(container, machine) {
		let scroller = container.first;
		let column = scroller.first;
		let data = this.data;
		scroller.empty(2);
		column.empty(1);
		if (machine) {
			if (model.home)
				column.add(FileTable(data, {}));
			column.add(CallTable(data, {}));
			column.add(DebugTable(data, { Behavior:LocalsTableBehavior }));
			column.add(DebugTable(data, { Behavior:ModulesTableBehavior }));
			column.add(DebugTable(data, { Behavior:GlobalsTableBehavior }));
		}
		else {
			scroller.add(new NoDebuggeesContainer());
			scroller.last.delegate("onScrolled", scroller);
		}
	}
};

class MachineButtonBehavior extends MenuButtonBehavior {
	onDescribeMenu(button) {
		let data = this.data;
		let machine = data.currentMachine;
		let machines = data.machines.filter(item => ((machine.tag == item.tag) && item.visible));
		return {
			ItemTemplate:MachineItemLine,
			items:machines,
			horizontal:"fit",
			vertical:"selection",
			selection:machines.indexOf(machine),
			context: shell,
		};
	}
	onMachinesChanged(button, machines) {
		let machine = this.data.currentMachine;
		button.active = machine ? this.data.machines.filter(item => ((machine.tag == item.tag) && item.visible)).length > 1 : false;
	}
	onMachineSelected(button, machine) {
		button.visible = machine && machine.visible;
		button.active = button.visible ? this.data.machines.filter(item => ((machine.tag == item.tag) && item.visible)).length > 1 : false;
		button.last.string = button.visible ? machine.title : "";
	}
	onMenuSelected(button, selection) {
		let data = this.data;
		data.selectMachine(selection);
	}
};

class DebugButtonBehavior extends ButtonBehavior {
	onCreate(container, data) {
		super.onCreate(container, data);
		this.can = "can" + container.name;
		this.do = "do" + container.name;
	}
	onMachineSelected(container, machine) {
		container.active = this.data[this.can]();
	}
	onTap(container) {
		this.data[this.do]();
	}
};

class BreakpointTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			for (let item of data.items)
				column.add(new BreakpointLine(item));
		}
		else {
			header.behavior.expand(header, false);
		}
		column.add(new BreakpointFooter(data));
	}
	hold(column) {
		return BreakpointHeader(this.data, {left:0, right:0, top:0, height:column.first.height});
	}
	onCreate(column, data) {
		this.data = data;
		if (data.expanded) {
			for (let item of data.items)
				column.add(new BreakpointLine(item));
		}
		column.add(new BreakpointFooter(data));
	}
	onBreakpointsChanged(column, data) {
		var data = this.data;
		if (data.expanded) {
			column.empty(1);
			for (let item of data.items) {
				column.add(new BreakpointLine(item));
			}
			column.add(new BreakpointFooter(data));
		}
	}
}

class BreakpointHeaderBehavior extends HeaderBehavior {
	reveal(line, revealIt) {
		line.last.visible = revealIt;
	}
};

class BreakpointLineBehavior extends LineBehavior {
	onTap(line) {
		let data = this.data;
		shell.delegate("doOpenURL", data.url, data.line);
	}
};

class DebugTableBehavior extends TableBehavior {
	addLines(column) {
		let header = column.first;
		let view = this.view;
		column.empty(1);
		if (view && view.expanded) {
			header.behavior.expand(header, true);
			view.lines.forEach(data => column.add(new (this.lineTemplate)(data)));
			if (view.lineIndex >= 0) {
				let line = column.content(view.lineIndex + 1);
				line.behavior.select(line, true);
			}
		}
		else
			header.behavior.expand(header, false);
		column.add(new this.footerTemplate(data));
		model.onHover(shell);
	}
	hold(column) {
		let header = column.first;
		let result = DebugHeader(this.data, {left:0, right:0, top:0, height:header.height, skin:header.skin});
		let view = this.view;
		result.behavior.expand(result, view && view.expanded);
		result.last.string = header.last.string;
		return result;
	}
	onCreate(column, data) {
		let machine = data.currentMachine;
		let view = machine ? machine.views[this.viewIndex] : null;
		this.data = data;
		this.machine = machine;
		this.view = view;	
		this.addLines(column);
	}
	onMachineSelected(column, machine) {
		this.machine = machine;
		if (machine) {
			this.view = machine.views[this.viewIndex];
			this.addLines(column);
			column.visible = true;
		}
		else {
			this.view = null;
			column.empty(1);
			column.visible = false;
		}
	}
	onMachineViewChanged(column, viewIndex) {
		if (this.viewIndex == viewIndex)
			this.addLines(column);
	}
	toggle(column) {
		var view = this.view;
		if (view)
			view.expanded = !view.expanded;
		this.addLines(column);
	}
	trigger(column, line) {
	}
};

class DebugHeaderBehavior extends HeaderBehavior {
};

class DebugLineBehavior extends LineBehavior {
	onTap(line) {
		let behavior = line.container.behavior;
		let data = this.data;
		behavior.data.doDebugToggle(behavior.viewIndex, data.name, data.value);
	}
};

class FileTableBehavior extends DebugTableBehavior {
	addLines(column) {
		let header = column.first;
		let view = this.view;
		column.empty(1);
		if (view && view.expanded) {
			header.behavior.expand(header, true);
			view.lines.forEach(item => column.add(new item.Template(item)));
		}
		else
			header.behavior.expand(header, false);
		column.add(new FileFooter(data));
		model.onHover(shell);
	}
	onCreate(column, data) {
		this.viewIndex = mxFilesView;
		super.onCreate(column, data);
		column.HEADER.last.string = "FILES";
	}
}

class FileHeaderBehavior extends HeaderBehavior {
};

import { 
	FileLineBehavior,
} from "features/files/files";

class FolderTableBehavior extends TableBehavior {
	expand(column, expandIt) {
		var data = this.data;
		var header = column.first;
		data.expanded = expandIt;
		column.empty(1);
		if (expandIt) {
			header.behavior.expand(header, true);
			data.items.forEach(item => column.add(new item.Template(item)));
		}
		else {
			header.behavior.expand(header, false);
		}
	}
	onCreate(column, data) {
		this.data = data;
		this.expand(column, data.expanded);
	}
};

class FolderHeaderBehavior extends HeaderBehavior {
};


class CallTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = CallFooter;
		this.lineTemplate = CallLine;
		this.viewIndex = mxFramesView;
		super.onCreate(column, data);
		column.HEADER.last.string = "CALLS";
	}
	trigger(column, line) {
		this.view.lineIndex = line.index - 1;
		let content = column.first.next;
		while (content) {
			if (content.behavior)
				content.behavior.select(content, content == line);
			content = content.next;
		}
	}
};

class CallHeaderBehavior extends HeaderBehavior {
};

class CallLineBehavior extends LineBehavior {
	onTap(line) {
		let column = line.container;
		let behavior = column.behavior;
		let data = this.data;
		behavior.data.doDebugFile(behavior.viewIndex, data.path, data.line, data.value);
		behavior.trigger(column, line);
	}
};

class LocalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxLocalsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "LOCALS";
	}
};

class ModulesTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxGrammarsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "MODULES";
	}
};

class GlobalsTableBehavior extends DebugTableBehavior {
	onCreate(column, data) {
		this.footerTemplate = DebugFooter;
		this.lineTemplate = DebugLine;
		this.viewIndex = mxGlobalsView;
		super.onCreate(column, data);
		column.HEADER.last.string = "GLOBALS";
	}
};

// TEMPLATES

import {
	DeviceHeader,
} from "features/devices/devices"

var DebugPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, Behavior: DebugPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:100, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Column($, {
					left:10, right:10, top:0, clip:true, Behavior:HolderColumnBehavior, 
					contents: [
						BreakpointTable($.model.breakpoints, {}),
					]
				}),
				VerticalScrollbar($, {}),
			]
		}),
		Container($, {
			left:10, right:10, top:100, height:40, clip:true, Behavior:HolderContainerBehavior,
		}),
		DeviceHeader(model.devicesFeature, { }),
	]
}));

export var DebugToolsHeader = Line.template($ => ({
	left:10, right:10, top:60, height:30, skin:grayBorderSkin,
	contents: [
		DebugToolButton($, { name:"Abort", variant:0 }),
		DebugToolButton($, { name:"Go", variant:1 }),
		DebugToolButton($, { name:"Step", variant:2 }),
		DebugToolButton($, { name:"StepIn", variant:3 }),
		DebugToolButton($, { name:"StepOut", variant:4 }),
		MachineButton($),
	],
}));

var MachineItemLine = Line.template($ => ({
	left:0, right:0, height:20, skin:menuLineSkin, active:true, Behavior:MenuItemBehavior,
	contents: [
		Content($, { width:20, height:20, skin:bulletSkin, visible:false }),
		Label($, { left:0, height:20, style:machineItemStyle, string:$.title }),
	],
}));

var MachineButton = Line.template($ => ({
	left:10, right:5, height:20, skin:blackMenuSkin, visible:false,
	Behavior: MachineButtonBehavior,
	contents: [
		Content($, { width:20, height:20, skin:bulletSkin, visible:false, variant:1 }),
		Label($, { left:0, right:20, style:machineButtonStyle }),
	],
}));

var DebugToolButton = Content.template($ => ({
	width:30, height:30, skin:debugButtonsSkin, active:false, Behavior: DebugButtonBehavior,
}));

var BreakpointTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:BreakpointTableBehavior,
	contents: [
		BreakpointHeader($, { name:"HEADER" }),
	],
}));

var BreakpointHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:greenHeaderSkin, active:true,
	Behavior: BreakpointHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:1, state:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"BREAKPOINTS" }),
		Container($, {
			width:80, skin:whiteButtonSkin, active:true, visible:false, 
			Behavior: class extends ButtonBehavior {
				onBreakpointsChanged(button) {
					button.active = model.debugFeature.canClearAllBreakpoints();
				}
				onTap(button) {
					model.debugFeature.doClearAllBreakpoints();
				}
			},
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Clear All" }),
			],
		}),
	],
}});

var BreakpointFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:greenFooterSkin,
}});

var BreakpointLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:greenLineSkin, active:true, 
	Behavior:BreakpointLineBehavior,
	contents: [
		Content($, { width:lineHeight, }),
		Label($, { style:nameStyle, string:$.name }),
		Label($, { style:valueStyle, string:" (" + $.line + ")" }),
	]
}});


var FileTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:FileTableBehavior,
	contents: [
		FileHeader($, { name:"HEADER" }),
	],
}));

var FileHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:greenHeaderSkin, active:true,
	Behavior: FileHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:1, state:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"FILES" }),
	],
}});

var FileLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:greenLineSkin, active:true, 
	Behavior:FileLineBehavior,
	contents: [
		Content($, { width:($.depth - 1) * 20 }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, variant:4, visible:false }),
		Label($, { style:fileStyle, string:$.name }),
	]
}});

var FolderTable = Column.template(function($) { return {
	left:0, right:0, active:true,
	Behavior: FolderTableBehavior,
	contents: [
		FolderHeader($, {}),
	],
}});

var FolderHeader = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:greenLineSkin, active:true,
	Behavior: FolderHeaderBehavior,
	contents: [
		Content($, { width:($.depth - 1) * 20 }),
		Content($, { width:20, height:20, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:0}),
		Label($, { style:nameStyle, string:$.name }),
	],
}});

var FileFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:greenFooterSkin,
}});

var CallTable = Column.template($ => ({
	left:0, right:0, active:true, 
	Behavior:CallTableBehavior,
	contents: [
		CallHeader($, { name:"HEADER" }),
	],
}));

var CallHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:orangeHeaderSkin, active:true,
	Behavior: CallHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle, string:"CALLS" }),
	],
}});

var CallFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:orangeFooterSkin,
}});


var CallLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:orangeLineSkin, active:true, 
	Behavior:CallLineBehavior,
	contents: [
		Content($, { width:lineHeight, }),
		Label($, { style:nameStyle, string:$.name }),
	]
}});

var DebugTable = Column.template($ => ({
	left:0, right:0, active:true,
	contents: [
		DebugHeader($, { name:"HEADER" }),
	],
}));

var DebugHeader = Line.template(function($) { return {
	left:0, right:0, height:30, skin:grayHeaderSkin, active:true,
	Behavior: DebugHeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, variant:1 }),
		Label($, { left:0, right:0, style:tableHeaderStyle }),
	],
}});

var DebugFooter = Line.template(function($) { return {
	left:0, right:0, height:10, skin:grayFooterSkin,
}});

var DebugLine = Line.template(function($) { return {
	left:0, right:0, height:lineHeight, skin:grayLineSkin, active:true, 
	Behavior:DebugLineBehavior,
	contents: [
		Content($, { left:$.column*lineHeight, width:lineHeight, top:0, height:lineHeight, skin:lineFlagsSkin, state:$.state }),
		Label($, { style:nameStyle, string:$.name }),
		Label($, { style:valueStyle, string:$.state == 0 ? (" = " + $.value) : "" }),
	]
}});

var NoDebuggeesContainer = Container.template($ => ({ 
	left:0, right:0, top:0, bottom:0,
	Behavior: class extends Behavior {
		onScrolled(container) {
			let scroller = container.container;
			let text = container.last;
			let size = scroller.height;
			let range = scroller.first.height;
			let height = text.height;
			if (height > size - range) {
				text.y = 0;
				text.visible = false;
			}
			else {
				text.y = scroller.y + ((size + range - height) >> 1);
				text.visible = true;
			}
		}
	},
	contents: [
		Content($, {}), // scrollbar
		Text($, { left:0, right:0, top:0, style:featureEmptyStyle, string:"No debuggees!" }),
	],
}));

