import {
	model,
} from "shell/main";

Handler.Bind("/network/interface/add", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		model.interfaces[query.name] = query;
		shell.distribute("onNetworkInterfacesChanged");
	}
});

Handler.Bind("/network/interface/remove", class extends Behavior {
	onInvoke(handler, message) {
		var query = parseQuery(message.query);
		delete model.interfaces[query.name];
		shell.distribute("onNetworkInterfacesChanged");
	}
});

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
	GRAYS,
	
	glyphsSkin,
	buttonSkin,
	buttonStyle,
	buttonsSkin,
	paneBodySkin,
	paneBorderSkin,
	paneHeaderSkin,
	paneHeaderStyle,
	noCodeSkin,
} from "shell/assets";	


const preferenceHeaderSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });
const preferenceLineSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });

const preferenceFirstNameStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:GRAYS[85], horizontal:"left" });
const preferenceSecondNameStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:GRAYS[85], horizontal:"left" });
const preferenceThirdNameStyle = new Style({ font:NORMAL_FONT, size:12, color:GRAYS[85], horizontal:"left" });
const preferenceValueStyle = new Style({ font:NORMAL_FONT, size:12, color:GRAYS[85], horizontal:"left", left:2 });

export var fieldScrollerSkin = new Skin({ fill: [ "white","white" ], stroke:GRAYS[6], borders: { left:1, right:1, bottom:1, top:1 } });

export const fieldLabelSkin = new Skin({ fill: [ "transparent","transparent",PASTEL_GRAY,PASTEL_ORANGE ] });
export const fieldLabelStyle = new Style({ font:NORMAL_FONT, size:12, color:GRAYS[85], horizontal:"right", left:10 })

const preferenceButtonStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:GRAYS[85] });
const preferenceCommentStyle = new Style({ font:LIGHT_FONT, size:12, color:GRAYS[85], horizontal:"left", left:10 });

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


class PreferencesColumnBehavior extends Behavior {
	onCreate(column) {
		let preferences = {
			items: [
				{
					Template: PreferencesTable,
					expanded: true,
					name: "BREAK",
					items: [
						{
							Template: ToggleLine,
							comment: "Break when the debuggee starts",
							name: "On Start",
							get value() {
								return model.breakOnStart;
							},
							set value(it) {
								model.toggleBreakOnStart(it);
							},
						},
						{
							Template: ToggleLine,
							comment: "Break when the debuggee throws exceptions",
							name: "On Exceptions",
							get value() {
								return model.breakOnExceptions;
							},
							set value(it) {
								model.toggleBreakOnExceptions(it);
							},
						},
					],
				},
				{
					Template: PreferencesTable,
					expanded: true,
					name: "NETWORK",
					items: [
						{
							Template: InterfacesLine,
							name: "Interfaces",
						},
						{
							Sheet: DebuggerPortSheet,
							Template: DebuggerPortLine,
							name: "Port Number",
							get value() {
								return model.port;
							},
							set value(it) {
								it = parseInt(it);
								if (isNaN(it))
									return;
								if (it < 1024)
									it = 1024;
								else if (it > 65535)
									it = 65535;
								if (model.port != it) {
									model.port = it	
									shell.distribute("onPortChanged");
								}
							},
						},
					],
				},
			]
		}
		preferences.items.forEach(item => column.add(new item.Template(item)));
	}
}

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

class ToggleLineBehavior extends LineBehavior {
	changeState(line) {
		super.changeState(line);
		line.last.visible = (this.flags & 1) ? true : false;
	}
	onTap() {
	}
}

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

var EditablePreferenceDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: EditablePreferenceDialogBehavior,
	contents: [
		$.Template($, {}),
	],
}));

export var PreferencesView = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, clip:true,
	contents: [
		Container($, {
			left:0, right:0, top:26, bottom:0, skin:noCodeSkin,
			contents: [
				Content($, { left:0, right:0, top:0, height:1, skin:paneBorderSkin, }),
				Scroller($, {
					left:0, right:0, top:1, bottom:0, clip:true, active:true, 
					Behavior:ScrollerBehavior,
					contents: [
						Column($, {
							left:0, right:0, top:0,
							Behavior:PreferencesColumnBehavior,
						}),
						HorizontalScrollbar($, {}),
						VerticalScrollbar($, {}),
					],
				}),
			],
		}),
		Line($, {
			left:0, right:0, top:0, height:26, skin:paneHeaderSkin, 
			contents: [
				Content($, { width:8 }),
				Label($, { left:0, right:0, style:paneHeaderStyle, string:"PREFERENCES" }),
				Content($, { 
					width:30, height:30, skin:buttonsSkin, variant:6, state:1, active:true, 
					Behavior: class extends ButtonBehavior {
						onTap(button) {
							button.bubble("doCloseFile");
						}
					},
				}),
			],
		}),
	],
}));


var PreferencesTable = Column.template(function($) { return {
	left:0, right:0,
	Behavior: PreferencesTableBehavior,
	contents: [
		Line($, {
			left:0, right:0, height:27, skin:preferenceHeaderSkin, active:true,
			Behavior: HeaderBehavior,
			contents: [
				Content($, { width:0 }),
				Content($, { width:26, top:3, skin:glyphsSkin, state:$.expanded ? 3 : 1, variant:0 }),
				Label($, { left:0, right:0, style:preferenceFirstNameStyle, string:$.name }),
			],
		}),
	],
}});

var InterfacesLine = Line.template($ => ({
	left:0, right:0, height:26, skin:preferenceLineSkin,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Label($, { 
			left:0, right:0, style:preferenceValueStyle, 
			Behavior: class extends Behavior {
				onCreate(label) {
					this.onNetworkInterfacesChanged(label);
				}
				onNetworkInterfacesChanged(label) {
					let string = "localhost";
					for (let name in model.interfaces)
						string += ", " + model.interfaces[name].ip;
					label.string = string;	
				}
			},
		}),
	],
}));

var DebuggerPortLine = Line.template($ => ({
	left:0, right:0, height:26, skin:preferenceLineSkin, active:true,
	Behavior: EditablePreferenceLineBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		Container($, {
			width:52, height:26,
			contents: [
				Scroller($, {
					left:0, right:0, top:2, bottom:2, skin:fieldScrollerSkin, clip:true,
					contents: [
						Label($, { 
							left:0, right:0, top:2, bottom:2, style:preferenceValueStyle, string:$.value,
							Behavior: class extends Behavior {
								onPortChanged(label) {
									label.string = model.port;
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
			width:52, height:26,
			contents: [
				Scroller($, {
					left:0, right:0, top:2, bottom:2, skin:fieldScrollerSkin, clip:true, active:true,
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
			anchor:"BUTTON", width:60, skin:buttonSkin, active:true, Behavior:ButtonBehavior, name:"onEnter",
			contents: [
				Label($, { left:0, right:0, style:buttonStyle, string:"Set" }),
			],
		}),
	],
}));

var ToggleLine = Line.template(function($) { return {
	left:0, right:0, height:26, skin:preferenceLineSkin, active:true,
	Behavior: ToggleLineBehavior,
	contents: [
		Content($, { width:50 }),
		Label($, { width:180, style:preferenceSecondNameStyle, string:$.name }),
		ToggleButton($, { }),
		Label($, { left:0, right:0, style:preferenceCommentStyle, string:$.comment, visible:false }),
	],
}});

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
var ToggleButton = Container.template($ => ({
	width:50, height:30, active:true,
	Behavior: ToggleButtonBehavior,
	contents: [
		Content($, { left:0, right:0, height:30, skin:toggleBarSkin }),
		Content($, { left:0, width:30, height:30, skin:toggleButtonSkin }),
	],
}));
