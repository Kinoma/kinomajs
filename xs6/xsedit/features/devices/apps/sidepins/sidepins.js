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
import Pins from "pins";

// ASSETS

import {
	whiteButtonSkin,
	whiteButtonStyle,
} from "common/assets";

import {
	ButtonBehavior
} from "common/control";

import {
	DropDialogBehavior,
	dropSideSkin,
	dropSkin,
	menuLineSkin,
	menuLineStyle,
	popupCheckSkin,
} from "common/menu";

import { 
	model,
} from "shell/main";

import {
	BLACK,
	DARK_GRAY,
	PASTEL_GRAY,
	PASTEL_GREEN,
	SEMIBOLD_FONT,
	TRANSPARENT,
	WHITE,
	greenBodySkin,
	greenHeaderSkin,
} from "shell/assets";

import { 
	LineBehavior,
} from "shell/behaviors";

import {
	greenTileSkin, 
	tileStyle,
	TileBehavior,
	tileSelectionSkin,
} from "features/devices/tiles";

import {
	HelperBehavior,
} from "features/devices/behaviors";

const appTitle = "Side Pins";

const pinLabelWidth = 40;
const pinLabelHeight = 20;
const pinSize = 40;
const pinBorder = 6;
const pinContent = pinSize-pinBorder;
const pinLabel = ["Nul", "V+", "GND", "Anl", "Dln", "DOut", "SCL", "SDA", "SRx", "STx", "PWM"];
const pinTitle = ["Disconnected", "Power", "Ground", "Analog", "Digital Input", "Digital Output", "I2C Clock", "I2C Data", "Serial Rx", "Serial Tx", "PWM"];

const bodySkin = new Skin({ fill:WHITE, stroke:PASTEL_GREEN, borders:{ left:1, right:1, top:1, bottom:1 } });

const elementSkin = new Skin({ texture:new Texture("./element.png", 1), x:0, y:0, width:90, height:90,
	tiles: { left:30, right: 30, top:30, bottom: 30 },
});
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:100, height:40, aspect:"fit" });
const pinButtonSkin = new Skin({ fill: [TRANSPARENT, PASTEL_GREEN, PASTEL_GREEN, PASTEL_GREEN] });
const pinBackSkin = new Skin({fill:DARK_GRAY});
const pinSkins = [
	new Skin({ fill:WHITE }),
	new Skin({ fill:"#ff0000" }),
	new Skin({ fill:BLACK }),
	new Skin({ fill:"#fe8f18" }),
	new Skin({ fill:"#039d27" }),
	new Skin({ fill:"#0074d7" }),
	new Skin({ fill:"#ae41f3" }),
	new Skin({ fill:"#fc1cf7" }),
	new Skin({ fill:"#00fffd" }),
	new Skin({ fill:"#ffff41" }),
	new Skin({ fill:"#999999" }),
];
const pinStyle = new Style({ font:SEMIBOLD_FONT, size:20, color:[BLACK, WHITE], horizontal:"center" });
const leftPinLabelStyle = new Style({ font:SEMIBOLD_FONT, size:13, color:BLACK, horizontal:"right", vertical:"middle" });
const rightPinLabelStyle = new Style({ font:SEMIBOLD_FONT, size:13, color:BLACK, horizontal:"left", vertical:"middle" });

const V = 1;
const GND = 2;
const ANL = 4;
const DIN = 8;
const DOUT = 16;
const SCL = 32;
const SDA = 64;
const SRX = 128;
const STX = 256;
const PWM = 512;
const NUL = 1024;

const availableBits = [
	NUL,
	V,
	GND,
	ANL,
	DIN,
	DOUT,
	SCL,
	SDA,
	SRX,
	STX,
	PWM,
];
const availablePins = [
	0,

	NUL | V | GND | DIN | DOUT | ANL | SRX,
	NUL | V | GND | DIN | DOUT | ANL | STX,
	NUL | V | GND | DIN | DOUT | ANL,
	NUL | V | GND | DIN | DOUT | ANL,
	NUL | V | GND | DIN | DOUT | ANL | SRX,
	NUL | V | GND | DIN | DOUT | ANL | STX,
	NUL | V | GND | DIN | DOUT | ANL,
	NUL | V | GND | DIN | DOUT | ANL,

	NUL | V | GND | DIN | DOUT | PWM,
	NUL | V | GND | DIN | DOUT | PWM,
	NUL | V | GND | DIN | DOUT | PWM | STX,
	NUL | V | GND | DIN | DOUT | PWM | SRX,
	NUL | V | GND | DIN | DOUT | PWM | SDA,
	NUL | V | GND | DIN | DOUT | PWM | SCL,
	NUL | V | GND | DIN | DOUT | PWM | SDA,
	NUL | V | GND | DIN | DOUT | PWM | SCL,
];

// BEHAVIORS

class SidePinsViewBehavior extends HelperBehavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDeviceHelperUp(container) {
		super.onDeviceHelperUp(container);
		var data = this.data;
		var device = model.devicesFeature.currentDevice;
		device.pinsShare(true).then(url => {
			data.pins = Pins.connect(url);
			data.pins.invoke("getPinMux", pinmux => this.onGetPinMux(pinmux));
		});
	}
	onUndisplayed(container) {
		var data = this.data;
		var device = model.devicesFeature.currentDevice;
		if (device.ws)
			device.pinsShare(false);
		if (data.pins)
			data.pins.close();
	}
	doUpdateApplyButton(button) {
		let data = this.data;
		let pinmux = this.data.pinmux;
		if (!pinmux) return;
		let apply = this.data.BUTTON;
		apply.active = apply.visible = false;
		for (let i = 0, c = data.leftPins.length; i < c; i++) {
			if ((data.leftPins[i] != pinmux.leftPins[i]) || (data.rightPins[i] != pinmux.rightPins[i])) {
				apply.active = apply.visible = true;
				return
			}
		}
		if ((data.leftVoltage != pinmux.leftVoltage) || (data.rightVoltage != pinmux.rightVoltage)) {
			apply.active = apply.visible = true;
			return
		}
	}
	onEnter(button) {
		let data = this.data;
		let pinmux = data.pinmux;
		let apply = this.data.BUTTON;
		apply.active = apply.visible = false;
		data.leftPins = pinmux.leftPins.slice(0);
		data.leftVoltage = pinmux.leftVoltage;
		data.rightPins = pinmux.rightPins.slice(0);
		data.rightVoltage = pinmux.rightVoltage;
		trace("setPinMux " + JSON.stringify(pinmux) + "\n");
		data.pins.invoke("setPinMux", pinmux);
	}
	onGetPinMux(pinmux) {
		trace("onGetPinMux " + JSON.stringify(pinmux) + "\n");
		if (!this.data.PINS.container) return;
		let data = this.data;
		data.pinmux = pinmux;
		data.leftPins = pinmux.leftPins.slice(0);
		data.leftVoltage = pinmux.leftVoltage;
		data.rightPins = pinmux.rightPins.slice(0);
		data.rightVoltage = pinmux.rightVoltage;
 		var pins = new PinsContainer(data);
 		this.data.PINS.empty();
 		this.data.PINS.add(pins);
	}
};

class PinButtonBehavior extends LineBehavior {
	onDialogClosed(line, item) {
		this.dialog = null;
		shell.focus();
	}
	isPinType(type, value) {
		return availableBits[type] == value;
	}
	onSelected(button, data) {
		var pins = this.data.pins;
		var index = this.data.index;
		var pin = this.data.number;
		var type = data.index;
		if (pins[index] != type) {
			let other;
			let otherType;
			if (this.isPinType(type, SRX)) {
				other = (pin == 12) ? button.previous : button.next;
				otherType = type + 1;
			}
			else if (this.isPinType(pins[index], SRX)) {
				other = (pin == 12) ? button.previous : button.next;
				otherType = 0;
			}
			else if (this.isPinType(type, STX)) {
				other = (pin == 11) ? button.next : button.previous;
				otherType = type - 1;
			}
			else if (this.isPinType(pins[index], STX)) {
				other = (pin == 11) ? button.next : button.previous;
				otherType = 0;
			}
			else if (this.isPinType(type, SCL)) {
				other = button.previous;
				otherType = type + 1;
			}
			else if (this.isPinType(pins[index], SCL)) {
				other = button.previous;
				otherType = 0;
			}
			else if (this.isPinType(type, SDA)) {
				other = button.next;
				otherType = type - 1;
			}
			else if (this.isPinType(pins[index], SDA)) {
				other = button.next;
				otherType = 0;
			}
			if (other) {
				other.delegate("updatePin", otherType);
			}
			this.updatePin(button, type);
			button.bubble("doUpdateApplyButton", data);
		}
	}
	onTap(button) {
		this.changeState(button, 1);
		let data = {
			Template:PinTypeMenu,
			button:button,
			data:this.data,
			variant:0,
			context:button,
		};
		this.dialog = new PinsDialog(data);
		shell.add(this.dialog);
		button.focus();
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
	updatePin(button, type) {
		var pins = this.data.pins;
		var index = this.data.index;
		this.data.type = type;
		pins[index] = type;
		var label = button.first;
		label.string = pinLabel[type];
		label = button.last.first;
		label.skin = pinSkins[type];
		label.state = type ? 1 : 0;
	}
};

class PinTypeItemBehavior extends ButtonBehavior {
	onTap(line) {
		line.bubble("onSelected", this.data);
	}
};


// height:(pinSize+pinBorder)*pinTitle.length+pinBorder

class PinsDialogBehavior extends DropDialogBehavior {	
	onMeasureVertically(layout, height) {
		let data = this.data;
		let side = data.data.side;
		let button = data.button;
		let column = button.container;
		let scroller = layout.first;
		let pointer = scroller.next;
		let size = scroller.first.measure();
		let scrollerHeight = (pinSize+pinBorder)*scroller.first.length;
		let delta = shell.height - button.y;
		let width = 250;
		let offset = pinBorder - 1;
		let left = side ? (column.x - width - pointer.width + offset) : (column.x + column.width + pointer.width - offset);
		scroller.coordinates = { left, width, top:button.y - 5, height:Math.min(scrollerHeight, delta) }
		pointer.coordinates = { left:side ? (column.x - pointer.width + offset) : (column.x + column.width - offset), width:pointer.width, top:button.y, height:pointer.height }
		return height;
	}
	onSelected(layout, data) {
		this.onClose(layout);
		this.data.context.delegate("onSelected", data);
	}
};

// TEMPLATES

import {
	ScrollerBehavior,
	HorizontalScrollbar,
	VerticalScrollbar,
} from "common/scrollbar";

import {
	AppViewHeader,
} from "features/devices/viewer";

import {
	SpinnerContent,
	ValueBehavior,
} from "features/devices/behaviors";

export const SidePinsView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:bodySkin, clip:true,
	Behavior: SidePinsViewBehavior,
	contents:[
		AppViewHeader({ skin:greenHeaderSkin, title:appTitle, device:$.device }),
		Container($, {
			left:1, right:1, top:60, bottom:1, anchor:"PINS",
			contents: [
				SpinnerContent($, { anchor:"SPINNER" })
			]
		}),
	],
}));

export const SidePinsTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

var PinsContainer = Column.template($ => ({
	top:0,
	contents: [
		Scroller($, {
			left:1, right:1, top:20, bottom:0, clip:true, active:true,
			Behavior:ScrollerBehavior,
			contents: [
				Column($, {
					width:480, top:0, skin:elementSkin,
					contents: [
						Container($, {
							left:2, right:2, height:480,
							contents: [
								LeftPinsColumn({pins: $.pinmux.leftPins, side:0}),
								Container($, {
									width:480-2*(pinLabelWidth+pinSize+pinBorder),
									contents: [
										Container($, {
											anchor:"BUTTON", width:80, skin:whiteButtonSkin, active:false, visible:false, name:"onEnter", Behavior: ButtonBehavior,
											contents: [
												Label($, { left:0, right:0, style:whiteButtonStyle, string:"Apply" }),
											],
										}),
									]
								}),
								RightPinsColumn({pins: $.pinmux.rightPins, side:1}),
							]
						}),
					]
				}),
				HorizontalScrollbar($, { bottom:-10 }),
				VerticalScrollbar($, { right:-10 }),
			]
		}),
	]
}));

var LeftPinsColumn = Column.template($ => ({
	left:0,
	contents: [
		Content($, { left:pinLabelWidth+pinBorder, right:0, height:pinBorder/2, skin:pinBackSkin }),
		$.pins.map((type, index) => new LeftPinButton({type, index, number:1+index, label:pinLabel[type], pins:$.pins, side:$.side})),
		Content($, { left:pinLabelWidth+pinBorder, right:0, height:pinBorder/2, skin:pinBackSkin }),
	],
}));

var RightPinsColumn = Column.template($ => ({
	right:0,
	contents: [
		Content($, { left:0, right:pinLabelWidth+pinBorder, height:pinBorder/2, skin:pinBackSkin }),
		$.pins.map((type, index) => new RightPinButton({type, index:index, number:9+index, label:pinLabel[type], pins:$.pins, side:$.side})),
		Content($, { left:0, right:pinLabelWidth+pinBorder, height:pinBorder/2, skin:pinBackSkin }),
	],
}));

var PinsDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PinsDialogBehavior,
	contents: [
		Scroller($, {
			skin:dropSkin, state:$.variant, clip:true, active:true,
			Behavior: ScrollerBehavior,
			contents: [
				$.Template($, {}),
			]
		}),
		Content($, {left:50, width:20, top:0, height:40, skin:dropSideSkin, variant:$.data.side}),
	],
}));

var LeftPinButton = Container.template($ => ({
	left:0, height:pinSize, skin:pinButtonSkin, active:true,
	Behavior: PinButtonBehavior,
	contents: [
		Label($, { left:2, width:pinLabelWidth-2, top:0, height:pinSize, style:leftPinLabelStyle, string:$.label }),
		Container($, {
			left:pinLabelWidth+pinBorder, width:pinSize+pinBorder, height:pinSize, skin:pinBackSkin,
			contents: [
				Label($, { width:pinContent, height:pinContent, skin:pinSkins[$.type], style:pinStyle, string:$.number, state:$.type ? 1 : 0 }),			
			]
		}),
	]
}));

var RightPinButton = Container.template($ => ({
	left:0, height:pinSize, skin:pinButtonSkin, active:true,
	Behavior: PinButtonBehavior,
	contents: [
		Label($, { left:pinSize+pinBorder+pinBorder, width:pinLabelWidth-2, top:0, height:pinSize, style:rightPinLabelStyle, string:$.label }),
		Container($, {
			left:0, width:pinSize+pinBorder, height:pinSize, skin:pinBackSkin,
			contents: [
				Label($, { width:pinContent, height:pinContent, skin:pinSkins[$.type], style:pinStyle, string:$.number, state:$.type ? 1 : 0 }),			
			]
		}),
	]
}));

var PinTypeMenu = Column.template($ => ({
	left:0, right:0, top:0,
	contents: [
		pinTitle.map(function(title, index) {
			if (availablePins[$.data.number] & availableBits[index])
				return new PinTypeItem({index, number:$.data.number, title, selected:$.data.type == index})
		}),
	]
}));

var PinTypeItem = Line.template($ => ({
	left:0, right:0, top:0, height:pinSize+pinBorder, skin:menuLineSkin, style:pinStyle, active:true,
	Behavior: PinTypeItemBehavior,
	contents: [
		Content($, { width:30, height:30, skin:popupCheckSkin, visible:$.selected }),
		Container($, {
			left:pinBorder, width:pinSize, height:pinSize,
			contents: [
				Label($, { width:pinContent, height:pinContent, skin:pinSkins[$.index], string:$.number, state:$.index ? 1 : 0 }),
			]
		}),
		Label($, { left:pinBorder, right:0, style:menuLineStyle, string:$.title}),
	]
}));
