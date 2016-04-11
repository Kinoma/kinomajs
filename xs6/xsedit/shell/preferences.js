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
		return new this.Template(model);
	}
};

// ASSETS

import {
	BOLD_FONT,
	NORMAL_FONT,
	SEMIBOLD_FONT,
	BLACK,
	WHITE,

	LIGHT_ORANGE,
	PASTEL_ORANGE,
	ORANGE,
	
	PASTEL_GRAY,
	
	grayHeaderSkin,
	orangeBorderSkin,
	orangeHeaderSkin,
	tableHeaderStyle,
	whiteButtonsSkin,
} from "shell/assets";	

import {
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";	

const preferenceBodySkin = new Skin({ fill:PASTEL_GRAY })
const preferenceLineSkin = new Skin({ fill: [WHITE, PASTEL_ORANGE, LIGHT_ORANGE, ORANGE], stroke:PASTEL_GRAY, borders:{ bottom:1 } });
const preferenceNameStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:[BLACK, BLACK, BLACK, WHITE], horizontal:"left", left:10 });
const preferenceValueStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:2 });
export var fieldScrollerSkin = new Skin({ fill: [ "white","white" ] });

export const fieldLabelSkin = new Skin({ fill: [ "transparent","transparent",PASTEL_GRAY,PASTEL_ORANGE ] });
export const fieldLabelStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"right", left:10 })

// BEAHVIORS

import {
	ButtonBehavior,
	FieldLabelBehavior, 
	FieldScrollerBehavior, 
} from "common/control";

import {
	LineBehavior,
} from "shell/behaviors";

class PreferencesViewBehavior extends Behavior {
	onLocateHome(container) {
		debugger
	}
};

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
	getValue(line) {
		debugger
	}
	onDialogClosed(line, data) {
		this.dialog = null;
		if (data.ok)
			this.setValue(line, data.value);
	}
	onTap(line) {
		let data = {
			Template:DebugPortPreferenceSheet,
			button:line,
			ok:false,
			value:this.getValue(line),
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
	setValue(line, value) {
		debugger
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

class CreateSimulatorPreferenceLineBehavior extends LocatePreferenceLineBehavior {
	onEnter(line) {
		var dictionary = { message:"Locate Kinoma Create Simulator", prompt:"Open", url:shell.url };
		system.openFile(dictionary, url => { 
			if (url) {
				this.data.createSimulator = url;
				shell.distribute("onCreateSimulatorChanged", url);
			}
		});
	}
};

class ElementSimulatorPreferenceLineBehavior extends LocatePreferenceLineBehavior {
	onEnter(line) {
		var dictionary = { message:"Locate Kinoma Element Simulator", prompt:"Open", url:shell.url };
		system.openFile(dictionary, url => { 
			if (url) {
				this.data.elementSimulator = url;
				shell.distribute("onElementSimulatorChanged", url);
			}
		});
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
	Behavior: PreferencesViewBehavior,
	contents: [
		Container($, {
			left:0, right:10, top:30, bottom:10, skin:preferenceBodySkin,
			contents: [
				Scroller($, {
					left:1, right:1, top:0, bottom:1, clip:true, active:true, 
					Behavior:ScrollerBehavior,
					contents: [
						Column($, {
							width:640, top:0,
							contents: [
								Content($, { height:10 }),
								DebugPortPreferenceLine($, {}),
								Content($, { height:5 }),
								HomePreferenceLine($, {}),
								Content($, { height:5 }),
								CreateSimulatorPreferenceLine($, {}),
								Content($, { height:5 }),
								ElementSimulatorPreferenceLine($, {}),
							]
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


var DebugPortPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: DebugPortPreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Debugger Port" }),
		Container($, {
			left:0, right:0, height:40,
			contents: [
				Scroller($, {
					left:3, right:3, top:5, bottom:5, skin:fieldScrollerSkin, clip:true,
					contents: [
						Label($, { 
							left:0, right:0, style:preferenceValueStyle, string:$.debugPort,
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
		Content($, { width:160 }),
	],
}));

var DebugPortPreferenceSheet = Line.template($ => ({
	skin:preferenceLineSkin, state:2,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Debugger Port" }),
		Container($, {
			left:0, right:0, height:40,
			contents: [
				Scroller($, {
					left:3, right:3, top:5, bottom:5, skin:fieldScrollerSkin, clip:true, active:true,
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
		Content($, { width:75 }),
		Container($, {
			anchor:"BUTTON", width:80, skin:whiteButtonSkin, active:true, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:whiteButtonStyle, string:"Set" }),
			],
		}),
		Content($, { width:5 }),
	],
}));

var HomePreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: HomePreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"KinomaJS Folder" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.home),
			Behavior: class extends Behavior {
				onHomeChanged(label, home) {
					label.string = Files.toPath($.home);
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

var CreateSimulatorPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: CreateSimulatorPreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Create Simulator" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.createSimulator),
			Behavior: class extends Behavior {
				onCreateSimulatorChanged(label, home) {
					label.string = Files.toPath($.createSimulator);
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

var ElementSimulatorPreferenceLine = Line.template($ => ({
	left:0, right:0, height:40, skin:preferenceLineSkin, active:true,
	Behavior: ElementSimulatorPreferenceLineBehavior,
	contents: [
		Label($, { width:160, style:preferenceNameStyle, string:"Element Simulator" }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, string:Files.toPath($.elementSimulator),
			Behavior: class extends Behavior {
				onElementSimulatorChanged(label, home) {
					label.string = Files.toPath($.elementSimulator);
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
