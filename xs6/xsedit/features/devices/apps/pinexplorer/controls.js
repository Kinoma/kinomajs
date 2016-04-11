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
	DropDialogBehavior,
	dropArrowSkin,
	dropSkin,
	menuLineSkin,
	menuLineStyle,
	popupCheckSkin,
} from "common/menu";

import {
	BLACK,
	DARK_GRAY,
	PASTEL_GRAY,
	PASTEL_GREEN,
	SEMIBOLD_FONT,
	WHITE,
} from "shell/assets";

import {
	ButtonBehavior
} from "common/control";

import { 
	LineBehavior,
} from "shell/behaviors";

import {
	ScrollerBehavior,
} from "common/scrollbar";

import { 
	SwitchButtonBehavior 
} from "features/devices/toggle";

const sliderIconTexture = new Texture("assets/slider-proxy.png", 1);
const sliderIconSkin = new Skin({ texture:sliderIconTexture, x:0, y:0, width:64, height:34 });

const sineWaveIconTexture = new Texture("assets/wave-proxy.png", 1);
const sineWaveIconSkin = new Skin({ texture:sineWaveIconTexture, x:0, y:0, width:64, height:34 });

const triangleWaveIconTexture = new Texture("assets/spike-proxy.png", 1);
const triangleWaveIconSkin = new Skin({ texture:triangleWaveIconTexture, x:0, y:0, width:64, height:34 });

const squareWaveIconTexture = new Texture("assets/flattop-proxy.png", 1);
const squareWaveIconSkin = new Skin({ texture:squareWaveIconTexture, x:0, y:0, width:64, height:34 });

const digitalSwitchIconTexture = new Texture("assets/proxy-toggle.png", 1);
const digitalSwitchIconSkin = new Skin({ texture:digitalSwitchIconTexture, x:0, y:0, width:64, height:34 });

const digitalMomentaryIconTexture = new Texture("assets/proxy-button.png", 1);
const digitalMomentaryIconSkin = new Skin({ texture:digitalMomentaryIconTexture, x:0, y:0, width:64, height:34 });

const gearTexture = new Texture("assets/settings-gear.png", 1);
const gearSkin = new Skin({ texture:gearTexture, x:0, y:0, width:40, height:40, states:40 });

const pinLabelHeight = 20;
const pinSize = 40;
const pinBorder = 6;
const pinContent = pinSize-pinBorder;
const pinLabel = ["Nul", "V+", "GND", "Anl", "Dln", "DOut", "SCL", "SDA", "SRx", "STx", "PWM"];
const pinTitle = ["Disconnected", "Power", "Ground", "Analog", "Digital Input", "Digital Output", "I2C Clock", "I2C Data", "Serial Rx", "Serial Tx", "PWM"];

const digitalOutControlTitle = ["Momentary Button", "Toggle Switch", "Square Wave"];
const iconWidth = 64;
const iconHeight = 34;
const gearSize = 40;


// BigOnOffSwitch

const onOffButtonTexture = new Texture("../../assets/on-off-toggle-switch.png", 1);
const onOffSwitchBarSkin = new Skin({ texture:onOffButtonTexture, x:200, y:0, width:120, height:80, states:80,
	tiles: { left:40, right:40 },
});
const onOffSwitchButtonSkin = new Skin({ texture:onOffButtonTexture, x:340, y:20, width:40, height:40, states:80 });
const onOffSwitchTextSkin = new Skin({ texture:onOffButtonTexture, x:400, y:0, width:120, height:80, states:80 });

class BigOnOffSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		var initialValue = 0;					//* read ?
		var switchData = this.data = { explorerData:data, sizeInset:18, value:initialValue };
		super.onCreate(container, switchData);
	}
	onValueChanged(container, value) {
		super.onValueChanged(container, value);
		this.data.explorerData.value = value;
		this.data.explorerData.write(this.data.value);
	}
};

export var BigOnOffSwitch = Container.template($ => ({
	width:200, height:80, active:true,
	Behavior: BigOnOffSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:onOffSwitchBarSkin, state:0 }),
		Content($, { left:0, width:80, top:0, bottom:0, skin:onOffSwitchBarSkin, state:1 }),
		Container($, { 
			left:18, right:18, top:0, bottom:0, clip:true,
			contents: [
				Line($, { 
					left:0,
					contents: [
						Content($, { skin:onOffSwitchTextSkin, state:1 }),
						Content($, { width:80, top:0, bottom:0, skin:onOffSwitchButtonSkin, state:0 }),
						Content($, { skin:onOffSwitchTextSkin, state:0 }),
					],
				}),
			],
		}),
	],
}));


// InOutSwitch

const inOutButtonTexture = new Texture("../../assets/in-out-toggle-switch.png", 1);
const inOutSwitchBarSkin = new Skin({ texture:inOutButtonTexture, x:100, y:0, width:60, height:40, states:40,
	tiles: { left:20, right:20 },
});
const inOutSwitchButtonSkin = new Skin({ texture:inOutButtonTexture, x:160, y:0, width:40, height:40, states:40 });
const inOutSwitchTextSkin = new Skin({ texture:inOutButtonTexture, x:200, y:0, width:60, height:40, states:40 });

class InOutSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		this.data = data;
		var info = data.info;
		var direction = info.direction;
		var pin = info.pin;
		var switchData = this.data = { explorerData:data, sizeInset:9, value:(direction == "input") ? 0 : 1, pinNumber:pin };
		SwitchButtonBehavior.prototype.onCreate.call(this, container, switchData);
	}
	onValueChanged(container, value) {
		SwitchButtonBehavior.prototype.onValueChanged.call(this, container, value);
		var direction = this.data.explorerData.info.direction;
		var toggledDirection = (direction == "input") ? "output" : "input";
		container.container.delegate("setDirection", toggledDirection);		
	}
};

export var InOutSwitch = Container.template($ => ({
	width:100, height:40, active:true,
	Behavior: InOutSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:inOutSwitchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:inOutSwitchBarSkin, state:1 }),
		Container($, {
			left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, { 
					left:0,
					contents: [
						Content($, { skin:inOutSwitchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:inOutSwitchButtonSkin, state:0 }),
						Content($, { skin:inOutSwitchTextSkin, state:0 }),
					],
				}),
			],
		}),
	],
}));

// MotorLEDSwitch

const motorLEDButtonTexture = new Texture("./assets/motor-led-toggle-switch.png", 1);
const motorLEDSwitchBarSkin = new Skin({ texture:motorLEDButtonTexture, x:100, y:0, width:60, height:40, states:40,
	tiles: { left:20, right:20 },
});
const motorLEDSwitchButtonSkin = new Skin({ texture:motorLEDButtonTexture, x:160, y:0, width:40, height:40, states:40 });
const motorLEDSwitchTextSkin = new Skin({ texture:motorLEDButtonTexture, x:200, y:0, width:60, height:40, states:40 });

class MotorLEDSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		this.data = data;
		var info = data.info;
		var selection = info.selection;
		var pin = info.pin;
		var switchData = this.data = { explorerData:data, sizeInset:9, value:(selection == "motor") ? 0 : 1, pinNumber:pin };
		SwitchButtonBehavior.prototype.onCreate.call(this, container, switchData);
	}
	onValueChanged(container, value) {
		SwitchButtonBehavior.prototype.onValueChanged.call(this, container, value);
		var selection = this.data.explorerData.info.selection;
		var toggledSelection = (selection == "motor") ? "led" : "motor";
		container.container.delegate("setSelection", toggledSelection);		
	}
};

export var MotorLEDSwitch = Container.template($ => ({
	width:100, height:40, active:true,
	Behavior: MotorLEDSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:motorLEDSwitchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:motorLEDSwitchBarSkin, state:1 }),
		Container($, {
			left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, { 
					left:0,
					contents: [
						Content($, { skin:motorLEDSwitchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:motorLEDSwitchButtonSkin, state:0 }),
						Content($, { skin:motorLEDSwitchTextSkin, state:0 }),
					],
				}),
			],
		}),
	],
}));

// Momentary Button

const momentaryButtonTexture = new Texture("assets/toggle-button-large.png", 1);
const momentaryButtonSkin = new Skin({ texture:momentaryButtonTexture, aspect:"stretch", x:0, y:0, width:80, height:80, states:80 });

class MomentaryButtonBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
		data.deleteWaveGenerator(content);
		content.start();
	}
	onTimeChanged(content) {
		this.assertValue(content);
	}
	onTouchBegan(content, id, x, y, ticks) {
		content.state = 1;
		this.assertValue(content);	
	}
	onTouchEnded(content, id, x, y, ticks) {
		content.state = 0;
		this.assertValue(content);
	}
	assertValue(content) {
		var value = (content.state == 1) ? 1 : 0;
		this.data.value = value;
		this.data.write(value);	
	}
};

export var MomentaryButton = Container.template($ => ({
	width:60, height:60, active:true, skin:momentaryButtonSkin,
	Behavior: MomentaryButtonBehavior,
}));

// Digital Output Control Popup Menu



const digitalOutControlSkins = [
	digitalMomentaryIconSkin,
	digitalSwitchIconSkin,
	squareWaveIconSkin
];

const itemStyle = new Style({ font:SEMIBOLD_FONT, size:20, color:[BLACK, WHITE], horizontal:"left" });

export var gSelectedDigitalOutControlIndex = 0;

const DigitalOutControlTypeMenu = Column.template($ => ({
	left:0, right:0, top:0, height:(gearSize+pinBorder)*digitalOutControlTitle.length+pinBorder,
	contents: [
		digitalOutControlTitle.map((title, index) => new DigitalOutControlItem({index, title, selected:gSelectedDigitalOutControlIndex == index})),
	]
}));

class DigitalOutControlItemBehavior extends ButtonBehavior {
	onTap(line) {
		line.bubble("onSelected", this.data);
	}
};

const DigitalOutControlItem = Line.template($ => ({
	left:0, right:0, top:0, height:gearSize+pinBorder, skin:menuLineSkin, style:itemStyle, active:true,
	Behavior: DigitalOutControlItemBehavior,
	contents: [
		Content($, { left:0, width:30, height:30, skin:popupCheckSkin, visible:$.selected }),
		Container($, { left:0, width:iconWidth, height:iconHeight, skin:digitalOutControlSkins[$.index], state:$.index ? 1 : 0 }),
		Label($, { left:6, right:0, style:menuLineStyle, string:$.title}),
	]
}));

class DigitalOutControlsButtonBehavior extends LineBehavior {
	onCreate(line, data) {
		this.data = data;
		super.onCreate(line, data);
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		shell.focus();
	}
	onSelected(button, data) {
		if (gSelectedDigitalOutControlIndex != data.index) {
			gSelectedDigitalOutControlIndex = data.index;
			this.data.CONTROL_CONTAINER.delegate("rebuildControl");
		}
	}
	onTap(button) {
		this.changeState(button, 1);
		let data = {
			Template:DigitalOutControlTypeMenu,
			button:button,
			data:this.data,
			variant:0,
			context:button,
		};
		this.dialog = new DigitalOutControlsDialog(data);
		shell.add(this.dialog);
		button.focus();
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
};

export const DigitalOutControlsButton = Container.template($ => ({
	width:gearSize, height:gearSize, active:true, skin:gearSkin,
	Behavior: DigitalOutControlsButtonBehavior,
}));

class DigitalOutControlsDialogBehavior extends DropDialogBehavior {	
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let scroller = layout.first;
		let pointer = scroller.next;
		let size = scroller.first.measure();
		let delta = shell.height - button.y - button.height - pointer.height;
		let pointerLeft = button.x + (button.width/2) - (pointer.width/2);
		pointer.coordinates = { left:pointerLeft, width:pointer.width, top:button.y, height:pointer.height }
		var menuWidth = 320;
		scroller.coordinates = { left:pointerLeft - menuWidth + (pointer.width/2) + (button.width/2) + 10, width:menuWidth, top:button.y + button.height + 5, height:Math.min(size.height, delta) }
		return height;
	}
	onSelected(layout, data) {
		this.onClose(layout);
		this.data.context.delegate("onSelected", data);
	}
};

const DigitalOutControlsDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: DigitalOutControlsDialogBehavior,
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


// PWM Control Popup Menu

const pwmControlSkins = [
	sliderIconSkin,
	sineWaveIconSkin,
	triangleWaveIconSkin,
	squareWaveIconSkin
];
const pwmControlTitle = ["Slider", "Sine Wave", "Triangle Wave", "Square Wave"];

export var gSelectedPWMControlIndex = 0;

const PWMControlTypeMenu = Column.template($ => ({
	left:0, right:0, top:0, height:(gearSize+pinBorder)*pwmControlTitle.length+pinBorder,
	contents: [
		pwmControlTitle.map((title, index) => new PWMControlItem({index, title, selected:gSelectedPWMControlIndex == index})),
	]
}));

class PWMControlItemBehavior extends ButtonBehavior {
	onTap(line) {
		line.bubble("onSelected", this.data);
	}
};

const PWMControlItem = Line.template($ => ({
	left:0, right:0, top:0, height:gearSize+pinBorder, skin:menuLineSkin, style:itemStyle, active:true,
	Behavior: PWMControlItemBehavior,
	contents: [
		Content($, { left:0, width:30, height:30, skin:popupCheckSkin, visible:$.selected }),
		Container($, { left:0, width:iconWidth, height:iconHeight, skin:pwmControlSkins[$.index], state:$.index ? 1 : 0 }),
		Label($, { left:6, right:0, style:menuLineStyle, string:$.title}),
	]
}));

class PWMControlsButtonBehavior extends LineBehavior {
	onCreate(line, data) {
		this.data = data;
		super.onCreate(line, data);
	}
	onDialogClosed(line, item) {
		this.dialog = null;
		shell.focus();
	}
	onSelected(button, data) {
		if (gSelectedPWMControlIndex != data.index) {
			gSelectedPWMControlIndex = data.index;
			this.data.CONTROL_CONTAINER.delegate("rebuildControl");
		}
	}
	onTap(button) {
		this.changeState(button, 1);
		let data = {
			Template:PWMControlTypeMenu,
			button:button,
			data:this.data,
			variant:0,
			context:button,
		};
		this.dialog = new PWMControlsDialog(data);
		shell.add(this.dialog);
		button.focus();
	}
	onUndisplayed(line) {
		if (this.dialog) {
			shell.remove(this.dialog);
			this.dialog = null;
		}
	}
};

export const PWMControlsButton = Container.template($ => ({
	width:gearSize, height:gearSize, active:true, skin:gearSkin,
	Behavior: PWMControlsButtonBehavior,
}));

class PWMControlsDialogBehavior extends DropDialogBehavior {	
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let scroller = layout.first;
		let pointer = scroller.next;
		let size = scroller.first.measure();
		let delta = shell.height - button.y - button.height - pointer.height;
		let pointerLeft = button.x + (button.width/2) - (pointer.width/2);
		pointer.coordinates = { left:pointerLeft, width:pointer.width, top:button.y, height:pointer.height }
		var menuWidth = 320;
		scroller.coordinates = { left:pointerLeft - menuWidth + (pointer.width/2) + (button.width/2) + 10, width:menuWidth, top:button.y + button.height + 5, height:Math.min(size.height, delta) }
		return height;
	}
	onSelected(layout, data) {
		this.onClose(layout);
		this.data.context.delegate("onSelected", data);
	}
};

const PWMControlsDialog = Layout.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true,
	Behavior: PWMControlsDialogBehavior,
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

