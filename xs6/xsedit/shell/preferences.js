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
	Viewer
} from "shell/viewer";


class DiscoveryItem {
	constructor(Config) {
		this.Template = ToggleLine;
		this.comment = "Discover " + Config.product + " devices on Wi-Fi";
		this.id = Config.id;
		this.name = Config.product;
	}
	get value() {
		return  model.devicesFeature.discoveryFlags[this.id] ? 1 : 0;
	}
	set value(it) {
		let devicesFeature = model.devicesFeature;
		devicesFeature.discoveryFlags[this.id] = it ? true : false;
		devicesFeature.filterDevices();
	}
}

class PlatformItem {
	constructor(Config) {
		this.Template = ToggleLine;
		this.comment = "Build standalone applications for " + Config.product;
		this.id = Config.id;
		this.name = Config.product;
	}
	get value() {
		return  model.devicesFeature.platformFlags[this.id] ? 1 : 0;
	}
	set value(it) {
		let devicesFeature = model.devicesFeature;
		devicesFeature.platformFlags[this.id] = it ? true : false;
		devicesFeature.filterDevices();
	}
}

class SerialItem {
	constructor(Config) {
		this.Template = ToggleLine;
		this.comment = "Control " + Config.product + " devices on USB";
		this.id = Config.id;
		this.name = Config.product;
	}
	get value() {
		return  model.devicesFeature.serialFlags[this.id] ? 1 : 0;
	}
	set value(it) {
		let devicesFeature = model.devicesFeature;
		devicesFeature.serialFlags[this.id] = it ? true : false;
		devicesFeature.filterDevices();
	}
}

class SimulatorItem {
	constructor(Config) {
		this.Template = SimulatorLine;
		this.defaultURL = Config.defaultURL;
		this.id = Config.id;
		this.name = Config.product;
	}
	get currentURL() {
		return model.devicesFeature.simulatorFiles[this.id];
	}
	set currentURL(it) {
		let id = this.id;
		model.devicesFeature.simulatorFiles[id] = it;
		shell.distribute("onSimulatorFilesChanged", id, it);
	}
	get value() {
		return  model.devicesFeature.simulatorFlags[this.id] ? 1 : 0;
	}
	set value(it) {
		let devicesFeature = model.devicesFeature;
		devicesFeature.simulatorFlags[this.id] = it ? true : false;
		devicesFeature.filterDevices();
	}
}

export class PreferencesViewer extends Viewer {
	constructor(feature) {
		super(feature);
		this.Template = PreferencesView;
		this.scheme = "preferences";
	}
	accept(parts, url) {
		return parts.scheme == this.scheme;
	}
	create(parts, url) {
		let devicesFeature = model.devicesFeature;
		let preferences = {
			items: [
				{
					Template: PreferencesTable,
					expanded: true,
					name: "DEVICES",
					items: [
						{
							Template: ToggleTable,
							expanded: false,
							comment: "Discover devices on Wi-Fi",
							name: "Discovery",
							value: 1,
							items: devicesFeature.DiscoveryConfigs.map(Config => new DiscoveryItem(Config)),
						},
//						{
//							Template: ToggleTable,
//							comment: "Build standalone applications",
//							expanded: false,
//							name: "Platforms",
//							items: devicesFeature.PlatformConfigs.map(Config => new PlatformItem(Config)),
//						},
						{
							Template: ToggleTable,
							comment: "Control devices on USB",
							expanded: false,
							name: "Serial Console",
							items: devicesFeature.SerialConfigs.map(Config => new SerialItem(Config)),
						},
						{
							Template: ToggleTable,
							comment: "Launch simulators",
							expanded: false,
							name: "Simulators",
							items: devicesFeature.SimulatorConfigs.map(Config => new SimulatorItem(Config)),
						},
						{
							Template: ToggleTable,
							comment: "Enable installation of beta software",
							expanded: false,
							name: "Beta Program",
							items:[
								{
									Template: ToggleLine,
									comment: "Enable Kinoma Create beta software updates",
									name: "Kinoma Create",
									get value() {
										return model.createSoftwareUpdatePreRelease;
									},
									set value(it) {
										model.createSoftwareUpdatePreRelease = it;
									},
								},
								{
									Template: ToggleLine,
									comment: "Enable Kinoma Element beta software updates",
									name: "Kinoma Element",
									get value() {
										return model.elementSoftwareUpdatePreRelease;
									},
									set value(it) {
										model.elementSoftwareUpdatePreRelease = it;
									},
								},
							],
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "PROJECTS",
					items: [
						{
							Template: LocateDirectoryLine,
							event: "onProjectsDirectoryChanged",
							message: "Locate the Projects Folder",
							name: "Projects Folder",
							get value() {
								return model.projectsDirectory;
							},
							set value(it) {
								model.projectsDirectory = it;
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "DEBUGGER",
					items: [
						{
							Template: LocateDirectoryLine,
							event: "onHomeChanged",
							message: "Locate kinomajs",
							name: "KinomaJS Folder",
							get value() {
								return model.home;
							},
							set value(it) {
								model.home = it;
							},
						},
						{
							Sheet: DebuggerPortSheet,
							Template: DebuggerPortLine,
							name: "Port Number",
							get value() {
								return model.debugPort;
							},
							set value(it) {
								it = parseInt(it);
								if (isNaN(it))
									return;
								if (it < 1024)
									it = 1024;
								else if (it > 65535)
									it = 65535;
								if (model.debugPort != it) {
									model.debugPort = it	
									shell.distribute("onDebugPortChanged", it);
								}
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "SAMPLES",
					items: [
						{
							Template: LocateDirectoryLine,
							event: "onSamplesDirectoryChanged",
							message: "Locate the Samples Folder",
							name: "Samples Folder",
							get value() {
								return model.samplesDirectory;
							},
							set value(it) {
								model.samplesDirectory = it;
							},
						},
					],
				},
			],
		};
		return new this.Template(preferences);
	}
};


// ASSETS

import {
	BOLD_FONT,
	LIGHT_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
	GRAY,
	WHITE,

	DARKEST_GRAY,
	LIGHT_GRAY,
	LIGHT_GREEN,
	LIGHT_ORANGE,
	PASTEL_ORANGE,
	ORANGE,
	
	PASTEL_GRAY,
	
	grayBorderSkin,
	grayHeaderSkin,
	grayLineSkin,
	orangeBorderSkin,
	orangeHeaderSkin,
	tableHeaderStyle,
	tableLineStyle,
	whiteButtonsSkin,
	
	fileGlyphsSkin,
	blackButtonSkin,
	blackButtonStyle,
} from "shell/assets";	

import {
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";	

const preferenceLineSkin = new Skin({ fill: [WHITE, PASTEL_GRAY, LIGHT_GRAY, PASTEL_GRAY] });

const preferenceFirstNameStyle = new Style({ font:BOLD_FONT, size:14, color:DARKEST_GRAY, horizontal:"left" });
const preferenceSecondNameStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:DARKEST_GRAY, horizontal:"left" });
const preferenceThirdNameStyle = new Style({ font:NORMAL_FONT, size:14, color:DARKEST_GRAY, horizontal:"left" });
const preferenceValueStyle = new Style({ font:NORMAL_FONT, size:14, color:DARKEST_GRAY, horizontal:"left", left:2 });

export var fieldScrollerSkin = new Skin({ fill: [ "white","white" ] });

export const fieldLabelSkin = new Skin({ fill: [ "transparent","transparent",PASTEL_GRAY,PASTEL_ORANGE ] });
export const fieldLabelStyle = new Style({ font:NORMAL_FONT, size:14, color:DARKEST_GRAY, horizontal:"right", left:10 })

const checkBoxSkin = new Skin({ texture:new Texture("../features/devices/apps/settings/assets/lighter-checkbox.png"), x:5, y:5, width:30, height:30, states:40 });

const preferenceButtonStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:DARKEST_GRAY });
const preferenceCommentStyle = new Style({ font:LIGHT_FONT, size:14, color:DARKEST_GRAY, horizontal:"left", left:10 });

// BEHAVIORS

import {
	ButtonBehavior,
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import {
	HolderColumnBehavior,
	HolderContainerBehavior,
	LineBehavior,
	HeaderBehavior,
	TableBehavior,
} from "shell/behaviors";

class PreferencesTableBehavior extends TableBehavior {
	addLines(table, expandIt) {
		this.data.items.forEach(item => table.add(new item.Template(item)));
	}
	expand(table, expandIt) {
		var data = this.data;
		var header = table.first;
		data.expanded = expandIt;
		if (expandIt) {
			header.behavior.expand(header, true);
			this.addLines(table);
		}
		else {
			header.behavior.expand(header, false);
			table.empty(1);
		}
	}
	onCreate(table, data) {
		super.onCreate(table, data);
		this.expand(table, data.expanded);
	}
}

class ToggleTableBehavior extends PreferencesTableBehavior {
	onDisplaying(table) {
		let data = this.data;
		data.value = data.items.reduce((value, item) => value + item.value, 0) / data.items.length;
	}
	onToggleChanged(table, data) {
		if (this.data == data) {
			data.items.forEach(item => {
				item.value = data.value;
				table.distribute("onValueChanged", item);
			});
		}
		else {
			data = this.data;
			data.value = data.items.reduce((value, item) => value + item.value, 0) / data.items.length;
			table.distribute("onValueChanged", data);
		}
		return true;
	}
}

class ToggleHeaderBehavior extends HeaderBehavior {
	changeState(line) {
		super.changeState(line);
		line.last.visible = (this.flags & 1) ? true : false;
	}
}

class ToggleLineBehavior extends LineBehavior {
	changeState(line) {
		super.changeState(line);
		line.last.visible = (this.flags & 1) ? true : false;
	}
	onTap() {
	}
}

class EditablePreferenceDialogBehavior extends Behavior {	
	onClose(layout, item) {
		let data = this.data;
		shell.remove(shell.last);
		shell.focus();
		shell.behavior.onHover();
		data.button.bubble("onDialogClosed", data);
	}
	onCreate(layout, data) {
		this.data = data;
	}
	onDisplayed(layout) {
		shell.behavior.onHover();
	}
	onEnter(layout) {
		let data = this.data;
		data.ok = true;
		data.value = data.FIELD.string;
		this.onClose(layout);
	}
	onEscape(layout) {
		this.onClose(layout);
	}
	onKeyDown(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		if ((c == 9) || (c== 25)) {
			return true;
		}
		if ((c == 3) || (c== 13)) {
			this.onEnter(dialog);
			return true;
		}
		if (c == 27) {
			this.onEscape(dialog);
			return true;
		}
		return false;
	}
	onKeyUp(dialog, key, repeat, ticks) {
		var c = key.charCodeAt(0);
		return (c == 3) || (c== 9) || (c== 13) || (c== 25) || (c== 27);
	}
	onMeasureHorizontally(layout, width) {
		return width;
	}
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let content = layout.first;
		content.coordinates = { left:button.x, width:button.width, top:button.y, height:button.height }
		return height;
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var content = layout.first;
		if (!content.hit(x, y))
			this.onClose(layout);
	}
};

class EditablePreferenceLineBehavior extends LineBehavior {
	onDialogClosed(line, data) {
		this.dialog = null;
		if (data.ok)
			this.data.value = data.value;
	}
	onTap(line) {
		let data = {
			Template:this.data.Sheet,
			button:line,
			name:this.data.name,
			ok:false,
			value:this.data.value,
		};
		this.dialog = new EditablePreferenceDialog(data);
		shell.add(this.dialog);
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
};

class DebugPortPreferenceLineBehavior extends EditablePreferenceLineBehavior {
	getValue(line) {
		return this.data.debugPort;
	}
	setValue(line, value) {
		value = parseInt(value);
		if (isNaN(value))
			return;
		if (value < 1024)
			value = 1024;
		else if (value > 65535)
			value = 65535;
		if (this.data.debugPort != value) {
			this.data.debugPort = value	
			shell.distribute("onDebugPortChanged", value);
		}
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalCenterScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

var EditablePreferenceDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: EditablePreferenceDialogBehavior,
	contents: [
		$.Template($, {}),
	],
}));

var PreferencesView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	contents: [
		Container($, {
			left:0, right:10, top:30, bottom:10, skin:grayBorderSkin,
			contents: [
				Scroller($, {
					left:1, right:1, top:0, bottom:1, clip:true, active:true, 
					Behavior:ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							contents:$.items.map(item => new item.Template(item)),
						}),
						HorizontalCenterScrollbar($, { left:-1, right:-1, bottom:-10 }),
						VerticalScrollbar($, { right:-10 }),
					],
				}),
			],
		}),
		Line($, {
			left:0, right:10, top:0, height:30, skin:grayHeaderSkin, active:true, 
			contents: [
				Content($, { width:10 }),
				Label($, { left:0, right:0, style:tableHeaderStyle, string:"PREFERENCES" }),
				Content($, { 
					width:30, height:30, skin:whiteButtonsSkin, variant:0, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doClose");
						}
					},
				}),
			],
		}),
	],
}));

export var PreferencesTable = Column.template(function($) { return {
	left:0, right:0,
	Behavior: PreferencesTableBehavior,
	contents: [
		Line($, {
			left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
			Behavior: HeaderBehavior,
			contents: [
				Content($, { width:0 }),
				Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
				Label($, { left:0, right:0, style:preferenceFirstNameStyle, string:$.name }),
			],
		}),
	],
}});

export var ToggleTable = Column.template(function($) { return {
	left:0, right:0,
	Behavior: ToggleTableBehavior,
	contents: [
		Line($, {
			left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
			Behavior: ToggleHeaderBehavior,
			contents: [
				Content($, { width:20 }),
				Content($, { width:30, height:30, skin:fileGlyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
				Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
				ToggleButton($, { }),
				Label($, { left:0, right:0, style:preferenceCommentStyle, string:$.comment, visible:false }),
			],
		}),
	],
}});

export var ToggleLine = Line.template(function($) { return {
	left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
	Behavior: ToggleLineBehavior,
	contents: [
		Content($, { width:40 }),
		Content($, { width:30 }),
		Label($, { width:160, style:preferenceThirdNameStyle, string:$.name }),
		ToggleButton($, { }),
		Label($, { left:0, right:0, style:preferenceCommentStyle, string:$.comment, visible:false }),
	],
}});

var SimulatorLine = Line.template(function($) { return {
	left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
	Behavior: class extends LineBehavior {
		changeState(line) {
			super.changeState(line);
			let locateButton = line.last.previous;
			let defaultButton = locateButton.previous;
			locateButton.visible = defaultButton.visible = (this.flags & 1) ? true : false;
			locateButton.width = defaultButton.width = (this.flags & 1) ? 80 : 0;
		}
		onTap() {
		}
	},
	contents: [
		Content($, { width:40 }),
		Content($, { width:30 }),
		Label($, { width:160, style:preferenceThirdNameStyle, string:$.name }),
		ToggleButton($, { }),
		Container($, {
			width:0, skin:blackButtonSkin, active:true, visible:false,
			Behavior: class extends ButtonBehavior {
				onTap(container) {
					let data = this.data;
					data.currentURL = data.defaultURL;
				}
			},
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Default" }),
			],
		}),
		Container($, {
			width:0, skin:blackButtonSkin, active:true, visible:false,
			Behavior: class extends ButtonBehavior {
				onTap(container) {
					let data = this.data;
					var dictionary = { message:"Locate " + data.defaultValue, prompt:"Open", url:mergeURI(shell.url, "../../../../../")  };
					system.openFile(dictionary, url => { 
						if (url)
							data.currentURL = url;
					});
				}
			},
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Locate" }),
			],
		}),
		Label($, { left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.currentURL),
			Behavior: class extends Behavior {
				onCreate(label, data) {
					this.data = data;
				}
				onSimulatorFilesChanged(label, id, it) {
					let data = this.data;
					if (data.id == id)
						label.string = Files.toPath(data.currentURL);
				}
			} 
		}),
	],
}});

var DebuggerPortLine = Line.template($ => ({
	left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
	Behavior: EditablePreferenceLineBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:60, height:30,
			contents: [
				Scroller($, {
					left:0, right:0, top:4, bottom:4, skin:fieldScrollerSkin, clip:true,
					contents: [
						Label($, { 
							left:0, right:0, top:2, bottom:2, style:preferenceValueStyle, string:$.value,
							Behavior: class extends Behavior {
								onDebugPortChanged(label, debugPort) {
									label.string = debugPort;
								}
							}
						}),
					],
				}),
			],
		}),
	],
}));

var DebuggerPortSheet = Line.template($ => ({
	skin:preferenceLineSkin, state:2,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:60, height:30,
			contents: [
				Scroller($, {
					left:0, right:0, top:4, bottom:4, skin:fieldScrollerSkin, clip:true, active:true,
					Behavior: FieldScrollerBehavior,
					contents: [
						Label($, {
							anchor:"FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:preferenceValueStyle, string:$.value, editable:true,
							Behavior: class extends FieldLabelBehavior {
								onDisplaying(label) {
									label.focus();
								}
							}
						}),
					],
				}),
			],
		}),
		Container($, {
			anchor:"BUTTON", width:80, skin:blackButtonSkin, active:true, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Set" }),
			],
		}),
	],
}));

export var LocateDirectoryLine = Line.template($ => ({
	left:0, right:0, height:30, skin:preferenceLineSkin, active:true,
	Behavior: class extends LineBehavior {
		changeState(line) {
			super.changeState(line);
			let locateButton = line.last.previous;
			locateButton.visible = (this.flags & 1) ? true : false;
			locateButton.width = (this.flags & 1) ? 80 : 0;
		}
		onTap() {
		}
	},
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:0, skin:blackButtonSkin, active:true, visible:false,
			Behavior: class extends ButtonBehavior {
				onTap(container) {
					var dictionary = { message:$.message, prompt:"Open", url:Files.documentsDirectory };
					system.openDirectory(dictionary, url => {
						if (url) {
							let data = this.data;
							data.value = url;
							shell.distribute(data.event, url);
						}
					});
				}
			},
			contents: [
				Label($, { left:0, right:0, style:blackButtonStyle, string:"Locate" }),
			],
		}),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.value),
			Behavior: class extends Behavior {
				[$.event](label, url) {
					label.string = Files.toPath(url);
				}
			}
		}),
	],
}));

const toggleTexture = new Texture("./assets/toggle.png", 2);
const toggleBarSkin = new Skin({ texture:toggleTexture, x:0, y:0, width:60, height:30, states:30,
	tiles: { left:20, right:20 },
});
const toggleButtonSkin = new Skin({ texture:toggleTexture, x:60, y:0, width:30, height:30, states:30 });

class ToggleButtonBehavior extends Behavior {
	changeOffset(container, offset) {
		var bar = container.first;
		var button = bar.next;
		if (offset < 0)
			offset = 0;
		else if (offset > this.size)
			offset = this.size;
		else
			offset = Math.round(offset);
		this.offset = offset;
		bar.state = this.offset / this.size;
		button.x = container.x + this.offset;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		var bar = container.first;
		var button = bar.next;
		this.size = bar.width - button.width;
		let data = this.data;
		this.changeOffset(container, (data.value == 0) ? 0 : (data.value == 1) ? this.size : this.size >> 1);
	}
	onTimeChanged(container) {
		this.changeOffset(container, this.anchor + Math.round(this.delta * container.fraction));
	}
	onTouchBegan(container, id, x, y, ticks) {
		if (container.running) {
			container.stop();
			container.time = container.duration;
		}
		this.anchor = x;
		this.moved = false;
		this.delta = this.offset;
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		var offset = this.offset;
		var size =  this.size;
		var delta = size >> 1;
		if (this.moved) {
			if (offset < delta)
				delta = 0 - offset;
			else 
				delta = size - offset;
		}
		else {
			if (offset == 0)
				delta = size;
			else if (offset == size)
				delta = 0 - size;
			else if (x > (container.x + (container.width >> 1)))
				delta = size - offset;
			else
				delta = 0 - offset;
		}
		if (delta) {
			this.anchor = offset;
			this.delta = delta;
			container.duration = 125 * Math.abs(delta) / size;
			container.time = 0;
			container.start();
		}
		var value = ((this.offset + delta) == 0) ? 0 : 1;
		if (this.data.value != value) {
			this.data.value = value;
			container.container.bubble("onToggleChanged", this.data);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.moved = Math.abs(x - this.anchor) >= 8;
		this.changeOffset(container, this.delta + x - this.anchor);
	}
	onValueChanged(container, data) {
		if (this.data == data) {
			let offset = (data.value == 0) ? 0 : (data.value == 1) ? this.size : this.size >> 1;
			if (this.offset != offset) {
				this.anchor = this.offset;
				this.delta = offset - this.offset;
				container.duration = 125 * Math.abs(this.delta) / this.size;
				container.time = 0;
				container.start();
			}
		}
	}
};

export var ToggleButton = Container.template($ => ({
	width:50, height:30, active:true,
	Behavior: ToggleButtonBehavior,
	contents: [
		Content($, { left:0, right:0, height:30, skin:toggleBarSkin }),
		Content($, { left:0, width:30, height:30, skin:toggleButtonSkin }),
	],
}));

/*


class LocatePreferenceLineBehavior extends LineBehavior {
	onMouseEntered(line, x, y) {
		line.state = 1;
		let button = line.last;
		button.active = button.visible = true;
	}
	onMouseExited(line, x, y) {
		line.state = 0;
		let button = line.last;
		button.active = button.visible = false;
	}
	onTouchBegan(line) {
	}
	onTouchEnded(line) {
	}
};

class HomePreferenceLineBehavior extends LocatePreferenceLineBehavior {
	onEnter(line) {
		var dictionary = { message:"Locate kinomajs", prompt:"Open", url:Files.documentsDirectory };
		system.openDirectory(dictionary, url => { 
			if (url) {
				this.data.home = url;
				shell.distribute("onHomeChanged", url);
			}
		});
	}
};

class SimulatorPreferenceLineBehavior extends LocatePreferenceLineBehavior {
	onMouseEntered(line, x, y) {
		super.onMouseEntered(line, x, y);
		let button = line.last.previous;
		button.active = button.visible = true;
	}
	onMouseExited(line, x, y) {
		super.onMouseExited(line, x, y);
		let button = line.last.previous;
		button.active = button.visible = false;
	}
};

class CreateSimulatorPreferenceLineBehavior extends SimulatorPreferenceLineBehavior {
	onDefault(line) {
		let url = "CreateShell.app";
		this.data.createSimulatorURI = url;
		shell.distribute("onCreateSimulatorChanged", url);
	}
	onEnter(line) {
		var dictionary = { message:"Locate Kinoma Create Simulator", prompt:"Open", url:mergeURI(shell.url, "../../../../../") };
		system.openFile(dictionary, url => { 
			if (url) {
				this.data.createSimulatorURI = url;
				shell.distribute("onCreateSimulatorChanged", url);
			}
		});
	}
};

class ElementSimulatorPreferenceLineBehavior extends SimulatorPreferenceLineBehavior {
	onDefault(line) {
		let url = "ElementShell.app";
		this.data.createSimulatorURI = url;
		shell.distribute("onElementSimulatorChanged", url);
	}
	onEnter(line) {
		var dictionary = { message:"Locate Kinoma Element Simulator", prompt:"Open", url:mergeURI(shell.url, "../../../../../") };
		system.openFile(dictionary, url => { 
			if (url) {
				this.data.elementSimulatorURI = url;
				shell.distribute("onElementSimulatorChanged", url);
			}
		});
	}
};

class CreateUpdatePreferenceLineBehavior extends LineBehavior {
	onDisplayed(container) {
		let button = container.first.next;
		button.state = model.createSoftwareUpdatePreRelease ? 2 : 1;
	}
	onTouchBegan(container, id, x, y, ticks) {
		let button = container.first.next;
		let state = button.state;
		button.state = (state == 1) ? 2 : 1;
	}
	onTouchCancelled(container, id, x, y, ticks) {
	}
	onTouchEnded(container, id, x, y, ticks) {
		let button = container.first.next;
		model.createSoftwareUpdatePreRelease = button.state == 2;
		shell.distribute("onDeviceChanged");
	}
};

class ElementUpdatePreferenceLineBehavior extends LineBehavior {
	onDisplayed(container) {
		let button = container.first.next;
		button.state = model.elementSoftwareUpdatePreRelease ? 2 : 1;
	}
	onTouchBegan(container, id, x, y, ticks) {
		let button = container.first.next;
		let state = button.state;
		button.state = (state == 1) ? 2 : 1;
	}
	onTouchCancelled(container, id, x, y, ticks) {
	}
	onTouchEnded(container, id, x, y, ticks) {
		let button = container.first.next;
		model.elementSoftwareUpdatePreRelease = button.state == 2;
		shell.distribute("onDeviceChanged");
	}
};

class SamplesDirectoryPreferenceLineBehavior extends LocatePreferenceLineBehavior {
	onEnter(line) {
		var dictionary = { message:"Locate Kinoma Code Samples Folder", prompt:"Open", url:Files.documentsDirectory };
		system.openDirectory(dictionary, url => { 
			if (url) {
				this.data.home = url;
				shell.distribute("onSamplesDirectoryChanged", url);
			}
		});
	}
};

var CreateSimulatorPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: CreateSimulatorPreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Create Simulator" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.createSimulatorURI),
			Behavior: class extends Behavior {
				onCreateSimulatorChanged(label, url) {
					label.string = Files.toPath(url);
				}
			}
		}),
		Container($, {
			anchor:"BUTTON", width:80, right:5, skin:whiteButtonSkin, active:false, visible:false, Behavior:ButtonBehavior, name:"onDefault",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Default" }),
			],
		}),
		Container($, {
			anchor:"BUTTON", width:80, right:5, skin:whiteButtonSkin, active:false, visible:false, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Locate" }),
			],
		}),
	],
}));

var ElementSimulatorPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: ElementSimulatorPreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Element Simulator" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.elementSimulatorURI),
			Behavior: class extends Behavior {
				onElementSimulatorChanged(label, url) {
					label.string = Files.toPath(url);
				}
			}
		}),
		Container($, {
			anchor:"BUTTON", width:80, right:5, skin:whiteButtonSkin, active:false, visible:false, Behavior:ButtonBehavior, name:"onDefault",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Default" }),
			],
		}),
		Container($, {
			anchor:"BUTTON", width:80, right:5, skin:whiteButtonSkin, active:false, visible:false, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Locate" }),
			],
		}),
	],
}));

var CreateUpdatePreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: CreateUpdatePreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Kinoma Create" }),
		Content($, { width:40, skin:checkBoxSkin, state:2 }),
		Label($, { style:preferenceValueStyle, string:"Enable pre-release updates" }),
	],
}));

var ElementUpdatePreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: ElementUpdatePreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Kinoma Element" }),
		Content($, { width:40, skin:checkBoxSkin, state:2 }),
		Label($, { style:preferenceValueStyle, string:"Enable pre-release updates" }),
	],
}));

var SamplesDirectoryPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: HomePreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Samples Folder" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.samplesDirectory),
			Behavior: class extends Behavior {
				onSamplesDirectoryChanged(label, samplesDirectory) {
					label.string = Files.toPath($.samplesDirectory);
				}
			}
		}),
		Container($, {
			anchor:"BUTTON", width:80, right:5, skin:whiteButtonSkin, active:false, visible:false, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Locate" }),
			],
		}),
	],
}));

*/
