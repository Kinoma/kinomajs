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
	greenButtonSkin,
	greenButtonStyle,
} from "common/assets";

import {
	ButtonBehavior
} from "common/control";

import {
	DropDialogBehavior,
	dropArrowSkin,
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

const appTitle = "Front Pins";

const pinLabelHeight = 20;
const pinSize = 40;
const pinBorder = 6;
const pinContent = pinSize-pinBorder;
const pinLabel = ["Nul", "V+", "GND", "Anl", "Dln", "DOut", "SCL", "SDA", "SRx", "STx", "PWM"];
const pinTitle = ["Disconnected", "Power", "Ground", "Analog", "Digital Input", "Digital Output", "I2C Clock", "I2C Data", "Serial Rx", "Serial Tx", "PWM"];

const switchButtonTexture = new Texture("./voltage-toggle-switch.png", 1);
const switchBarSkin = new Skin({ texture:switchButtonTexture, x:100, y:0, width:60, height:40, states:40,
	tiles: { left:20, right:20 },
});
const switchButtonSkin = new Skin({ texture:switchButtonTexture, x:160, y:0, width:40, height:40, states:40 });
const switchTextSkin = new Skin({ texture:switchButtonTexture, x:200, y:0, width:40, height:40, states:40 });


const bodySkin = new Skin({ texture:new Texture("./slot-bay.png", 1), x:0, y:0, width:90, height:90,
	tiles: { left:30, right: 30, top:30, bottom: 30 },
});
const iconSkin = new Skin({ texture:new Texture("./icon.png", 1), x:0, y:0, width:100, height:40, aspect:"fit" });
const pinButtonSkin = new Skin({ fill: [WHITE, PASTEL_GRAY, PASTEL_GRAY, PASTEL_GRAY], stroke:PASTEL_GREEN, borders:{ bottom:1 } });
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
const pinLabelStyle = new Style({ font:SEMIBOLD_FONT, size:13, color:BLACK, horizontal:"center" });

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

	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,

	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
	NUL | V | GND | DIN | DOUT | ANL | SCL | SDA | PWM,
];

// BEHAVIORS

class FrontPinsViewBehavior extends HelperBehavior {
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
		data.pins.invoke("setPinMux", pinmux);
	}
	onGetPinMux(pinmux) {
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
	onVoltageChanged(button, data) {
		let pinmux = this.data.pinmux;
		pinmux[data.accessor] = data.value ? 5 : 3.3;
		this.doUpdateApplyButton(button);
	}
};

class PinButtonBehavior extends LineBehavior {
	isPinType(type, value) {
		return availableBits[type] == value;
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		shell.focus();
	}
	onSelected(button, data) {
		var pins = this.data.pins;
		var index = this.data.index;
		var pin = this.data.number;
		var type = data.index;
		if (pins[index] != type) {
			let other, otherType;
			if (this.isPinType(type, SCL) || this.isPinType(type, SDA)) {
				// only one I2C per front header
				let otherIndex = pins.findIndex(it => type == it);
				if (otherIndex >= 0) {
					other = button.container.peek(otherIndex + 1);
					otherType = 0;
				}
			}
			else if (this.isPinType(type, PWM)) {
				// only three PWM per front header
				let count = 0;
				pins.forEach(it => {
					if (type == it) count++;
				});
				if (count >= 3) {
					let otherIndex = pins.findIndex(it => type == it);
					other = button.container.peek(otherIndex + 1);
					otherType = 0;
				}
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

class PinsDialogBehavior extends DropDialogBehavior {	
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button.container;
		let scroller = layout.first;
		let pointer = scroller.next;
		let size = scroller.first.measure();
		let scrollerHeight = (pinSize+pinBorder)*scroller.first.length;
		let delta = shell.height - button.y - button.height - pointer.height;
		scroller.coordinates = { left:button.x - 5, width:button.width + 10, top:button.y + 80, height:Math.min(scrollerHeight, delta) }
		pointer.coordinates = { left:button.x + (pointer.width * data.data.index) + (pinBorder/2), width:pointer.width, top:button.y + pinContent + 1, height:pointer.height }
		return height;
	}
	onSelected(layout, data) {
		this.onClose(layout);
		this.data.context.delegate("onSelected", data);
	}
};

class SwitchButtonBehavior extends Behavior {
	changeOffset(container, offset) {
		var label = container.last;
		var line = label.first;
		var button = line.first.next;
		var bar = label.previous;
		var background = bar.previous;
		if (offset < 0)
			offset = 0;
		else if (offset > this.size)
			offset = this.size;
		this.offset = offset;
		bar.width = button.width + Math.round(this.size - offset);
		line.x = label.x - Math.round(offset);
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		var label = container.last;
		var line = label.first;
		var button = line.first.next;
		var bar = label.previous;
		var background = bar.previous;
		this.half = background.width >> 1;
		this.size = background.width - button.width;
		line.first.coordinates = line.last.coordinates = { width: this.size - 9 };   
		this.changeOffset(container, (this.data.value > 0) ? 0 : this.size);
	}
	onFinished(container) {
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
		this.capturing = false;
		this.delta = this.offset + x;
		container.last.first.first.next.state = 1;
	}
	onTouchCancelled(container, id, x, y, ticks) {
		container.last.first.first.next.state = 0;
	}
	onTouchEnded(container, id, x, y, ticks) {
		var offset = this.offset;
		var size =  this.size;
		var delta = size >> 1;
		if (this.capturing) {
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
			else if (x < this.half)
				delta = 0 - offset;
			else
				delta = size - offset;
		}
		if (delta) {
			this.anchor = offset;
			this.delta = delta;
			container.duration = 250 * Math.abs(delta) / size;
			container.time = 0;
			container.start();
		}
		var newValue = ((this.offset + delta) == 0) ? 1 : 0;
		if (this.data.value != newValue) {
			this.data.value = newValue;
			this.onValueChanged(container, this.data.value);
		}
				
		container.last.first.first.next.state = 0;
	}
	onTouchMoved(container, id, x, y, ticks) {
		if (this.capturing) {
			this.changeOffset(container, this.delta - x);
		}
		else if (Math.abs(x - this.anchor) >= 8) {
			this.capturing = true;
			container.captureTouch(id, x, y, ticks);
			this.changeOffset(container, this.delta - x);
		}
	}
	onValueChanged(container, value) {
		this.changeOffset(container, value ? 0 : this.size);
		container.bubble("onVoltageChanged", this.data);
	}
}

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

export const FrontPinsView = Container.template($ => ({
	left:0, right:10, top:0, bottom:10, skin:greenBodySkin, clip:true,
	Behavior: FrontPinsViewBehavior,
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

export const FrontPinsTile = Container.template($ => ({
	left:0, top:0, height:iconSkin.height+50, skin:greenTileSkin, style:tileStyle,
	Behavior: TileBehavior,
	contents: [
		Content($, { center:0, top:10, skin:iconSkin }),
		Label($, { left:5, right:5, top:iconSkin.height+15, string:appTitle }),
		Content($, { right:0, top:0, width:20, height:20, skin:tileSelectionSkin }),
	]
}));

const PinsContainer = Column.template($ => ({
	top:0,
	contents: [
		Scroller($, {
			left:1, right:1, top:0, bottom:0, clip:true, active:true, Behavior:ScrollerBehavior,
			contents: [
				Container($, {
					width:pinSize*20, top:0, height:180, skin:bodySkin,
					contents: [
						Column($, {
							left:40,
							contents: [
								VoltageSwitchButton({value: $.pinmux.leftVoltage == 5 ? 1 : 0, accessor:"leftVoltage"}, {top:20}),
 								PinsLine({pins: $.pinmux.leftPins, side:0}, {top:20}),
							]
						}),
						Column($, {
							right:40,
							contents: [
								VoltageSwitchButton({value: $.pinmux.rightVoltage == 5 ? 1 : 0, accessor:"rightVoltage"}, {top:20}),
 								PinsLine({pins: $.pinmux.rightPins, side:1}, {top:20}),
							]
						}),
					]
				}),
				HorizontalScrollbar($, { bottom:-10 }),
				VerticalScrollbar($, { right:-10 }),
			]
		}),
		Container($, {
			anchor:"BUTTON", width:80, top:40, skin:greenButtonSkin, active:false, visible:false, name:"onEnter", Behavior: ButtonBehavior,
			contents: [
				Label($, { left:0, right:0, style:greenButtonStyle, string:"Apply" }),
			],
		}),
	]
}));

const PinsLine = Line.template($ => ({
	bottom:40,
	contents: [
		Content($, { top:pinLabelHeight, bottom:0, width:pinBorder/2, skin:pinBackSkin }),
		$.pins.map((type, index) => new PinButton({type, index, number:($.side?59:51) + index, label:pinLabel[type], pins:$.pins})),
		Content($, { top:pinLabelHeight, bottom:0, width:pinBorder/2, skin:pinBackSkin }),
	],
}));

const PinsDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PinsDialogBehavior,
	contents: [
		Scroller($, {
			top:80, skin:dropSkin, state:$.variant, clip:true, active:true,
			Behavior: ScrollerBehavior,
			contents: [
				$.Template($, {}),
			]
		}),
		Content($, {left:50, width:pinSize, top:0, height:71, skin:dropArrowSkin}),
	],
}));

const PinButton = Container.template($ => ({
	left:0, width:pinSize, skin:pinButtonSkin, active:true,
	Behavior: PinButtonBehavior,
	contents: [
		Label($, { left:0, right:0, top:0, height:pinLabelHeight, style:pinLabelStyle, string:$.label }),
		Container($, {
			top:pinLabelHeight, bottom:0, width:pinSize, height:pinSize+pinBorder, skin:pinBackSkin,
			contents: [
				Label($, { width:pinContent, height:pinContent, skin:pinSkins[$.type], style:pinStyle, string:$.number, state:$.type ? 1 : 0 }),			
			]
		}),
	]
}));

const PinTypeMenu = Column.template($ => ({
	left:0, right:0, top:0,
	contents: [
		pinTitle.map(function(title, index) {
			if (availablePins[$.data.number - 50] & availableBits[index])
				return new PinTypeItem({index, number:$.data.number, title, selected:$.data.type == index})
		}),
	]
}));

const PinTypeItem = Line.template($ => ({
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

const VoltageSwitchButton = Container.template($ => ({
	width:100, height:40, active:true,
	Behavior:SwitchButtonBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:switchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:switchBarSkin, state:1 }),
		Container($, {
			left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, {
					left:0,
					contents: [
						Content($, { skin:switchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:switchButtonSkin, state:0 }),
						Content($, { skin:switchTextSkin, state:0 }),
					]
				}),
			]
		}),
	]
}));
