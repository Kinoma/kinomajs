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
	PopupDialog,
	PopupItem,
	popupArrowsSkin,
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
	whiteButtonSkin,
	whiteButtonStyle,
	grayButtonSkin,
	grayButtonStyle
} from "common/assets";

import {
	ButtonBehavior,
	FieldLabelBehavior,
	FieldScrollerBehavior
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

import { 
	fieldScrollerSkin,
	fieldLabelSkin,
	blackSkin,
	whiteSkin,
} from "features/devices/assets";

import {
	gPinFilterInfo
} from "pinexplorer";

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
		var initialValue = data.value;
		var switchData = this.data = { explorerData:data, sizeInset:18, value:initialValue };
		super.onCreate(container, switchData);
		this.onValueChanged(container, initialValue)
	}
	onValueChanged(container, value) {
		super.onValueChanged(container, value);
		this.data.explorerData.value = value;
		this.data.explorerData.write(this.data.value);
	}
	onDigitalOutSelected(container) {		
		this.onValueChanged(container, this.data.value);
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
		this.data.explorerData.start(this.data.explorerData.container);
		container.container.distribute("onDigitalOutSelected");
		container.container.container.distribute("onDigitalDirectionChanged", toggledDirection);
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

		let pinNumber = this.data.info.pin;
		if (false == this.data.device.pwmPinHasMotorMode(pinNumber)) {
			container.active = false;
			container.visible = false;
			return
		}
		
		var selection = this.data.pwmMode;
		var info = data.info;
		var pin = info.pin;
		var switchData = this.data = { explorerData:data, sizeInset:9, value:(selection == "motor") ? 1 : 0, pinNumber:pin };
		SwitchButtonBehavior.prototype.onCreate.call(this, container, switchData);
	}
	onValueChanged(container, value) {
		SwitchButtonBehavior.prototype.onValueChanged.call(this, container, value);
		var selection = this.data.explorerData.pwmMode;
		var toggledSelection = (selection == "motor") ? "led" : "motor";
		container.container.delegate("setPWMMode", toggledSelection);		
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
		if (content.state == 1)
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

const DigitalOutControlTypeMenu = Column.template($ => ({
	left:0, right:0, top:0, height:(gearSize+pinBorder)*digitalOutControlTitle.length+pinBorder,
	contents: [
		digitalOutControlTitle.map((title, index) => new DigitalOutControlItem({index, title, selected:$.data.controlIndex == index})),
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
		if (this.data.controlIndex != data.index) {
			this.data.controlIndex = data.index;
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

// Filter Buttons

const pinFilterStyle = new Style({ font:SEMIBOLD_FONT, size:12, color:[BLACK, WHITE], horizontal:"right" });

class FilterButtonLineBehavior extends Behavior {
	onCreate(line, data) {
		this.data = data;
	}
	onFilterButtonChanged(line) {
		this.data.PROBES.delegate("onUpdate");
	}
	onAllFilterButtonChanged(line) {
		let aButton = line.first.next;		// skip the label
		while (aButton) {
			let type = aButton.behavior.data.type;
			if (type == "pin")
				aButton.delegate("changeState", 2);
			aButton = aButton.next;
		}
		this.data.PROBES.delegate("onUpdate");
	}
	onNoneFilterButtonChanged(line) {
		let aButton = line.first.next;		// skip the label
		while (aButton) {
			let type = aButton.behavior.data.type;
			if (type == "pin")
				aButton.delegate("changeState", 0);
			aButton = aButton.next;
		}
		this.data.PROBES.delegate("onUpdate");
	}
	filterProbes(line, probes) {
		if (undefined == probes)
			return undefined;
				
		let filteredTypes = [];
//		let aButton = line.first;
		let aButton = line.first.next; 		// skip the label column
		while (aButton) {
			if (aButton.state == 0)
				filteredTypes.push(aButton.behavior.data.name);
			aButton = aButton.next;
		}
		
		let filteredProbes = [];
		probes.forEach(probe => {
			var isFiltered = false;
			filteredTypes.forEach(type => {
				if (type == probe.info.type)
					isFiltered = true;
			});
			if (false == isFiltered)
				filteredProbes.push(probe);
		});
		
		filteredProbes = this.applyDeviceSpecificFiltering(line, filteredProbes);

		return filteredProbes;
	}
	
	applyDeviceSpecificFiltering(line, filteredProbes) {
		let device = this.data.device;
		let deviceFilterInfo = gPinFilterInfo.get(device);
		if (undefined == deviceFilterInfo)
			return;
		let deviceButtons = deviceFilterInfo.buttons;
		let createFilteredProbes = [];
		let aButton = line.first;
		while (aButton) {
			deviceButtons.forEach(deviceButton => {
				if (deviceButton.name == aButton.name) {
					if (aButton.state != 0) {
						filteredProbes.forEach(probe => {
							let logicalPinNum = probe.info.pin;
							let physicalPinNum = undefined;
							if (undefined != device.logicalToPhysicalMap)
								physicalPinNum = device.logicalToPhysicalMap.get(logicalPinNum);
							if (undefined == physicalPinNum)
								physicalPinNum = logicalPinNum;
							if ((physicalPinNum >= deviceButton.startPin) && (physicalPinNum <= deviceButton.endPin))
								createFilteredProbes.push(probe);
						});
					}
				}
			});
			aButton = aButton.next;
		}
		return createFilteredProbes;
	}

	appendAllNoneButtons(line) {
		line.add( new NoneAllButton({ name:"All", message:"onAllFilterButtonChanged", type:"all" }, { left:20 } ));
		line.add( new NoneAllButton({ name:"None", message:"onNoneFilterButtonChanged", type:"none" }, { left:10 } ));
	}

	appendDeviceSpecificButtons(line) {
		let device = this.data.device;
		let deviceFilterInfo = gPinFilterInfo.get(device);
		let deviceButtons = deviceFilterInfo.buttons;
		deviceButtons.forEach(deviceButton => {
			line.add( new FilterButton( { name:deviceButton.name, message:"onFilterButtonChanged", type:"custom" }, { left:10, name:deviceButton.name } ));
		});
	}
};

export const FilterButtonLine = Line.template($ => ({
	clip:true,
	Behavior: FilterButtonLineBehavior,
	contents: [
		Column($, {
			left:10,
			contents: [
				Label($, { top:0, right:0, style:pinFilterStyle, string:"Show"}),
				Label($, { top:-3, right:0, style:pinFilterStyle, string:"Pins"}),
			]
		}),
		FilterButton({ name:"Analog", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
		FilterButton({ name:"Digital", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
		FilterButton({ name:"PWM", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
		FilterButton({ name:"I2C", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
//		FilterButton({ name:"Serial", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
		FilterButton({ name:"Power", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
		FilterButton({ name:"Ground", message:"onFilterButtonChanged", type:"pin" }, { left:10 }),
	],
}));

const roundedFilterButtonTexture = new Texture("assets/rounded-filter-button.png", 1);
const roundedFilterButtonSkin = new Skin({ texture:roundedFilterButtonTexture, x:0, y:0, width:30, height:24, states:24, tiles: { left:10, right:10 } });

const rectangleFilterButtonTexture = new Texture("assets/rectangle-filter-button-check.png", 1);
const rectangleFilterButtonSkin = new Skin({ texture:rectangleFilterButtonTexture, x:0, y:0, width:44, height:24, states:24, tiles: { left:24, right:10 } });


class FilterButtonBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		this.changeState(container, 2);		// change to data.isSelected later ***
		this.wasToggled = false;
	}
	changeState(container, state) {
		container.state = state;
		var content = container.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.beginTrigger(container);
	}
	onTouchEnded(container, id, x, y, ticks) {
		this.endTrigger(container);
	}
	beginTrigger(container) {
		container.state = (this.saveState == 2 ? 0 : 2)
		this.saveState = container.state;
		this.changeState(container, container.state);
		container.container.delegate(this.data.message);
		this.wasToggled = true;
		if ((this.data.type == "custom") && (container.state == 0))
			this.ensureOneCustomButtonIsEnabled(container);
	}
	endTrigger(container) {
		this.wasToggled = false;
	}
	onMouseEntered(container, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		this.saveState = container.state;
		this.changeState(container, 1);
	}
	onMouseExited(container, x, y) {
		if (false == this.wasToggled) {
			container.state = this.saveState;
			this.changeState(container, container.state);
		}
	}
	ensureOneCustomButtonIsEnabled(container) {
		let customButtons = [];
		let line = container.container;
		let allMuted = true;
		let aButton = line.first.next; 	// skip label
		while (aButton) {
			if (aButton.behavior.data.type == "custom") {
				customButtons.push(aButton);
				if (aButton.state != 0)
					allMuted = false;
			}
			aButton = aButton.next;
		}
		if (allMuted) {
			for (var i=0, c=customButtons.length; i<c; i++) {
				let aButton = customButtons[i];
				if (aButton !== container) {
					aButton.delegate("beginTrigger");
					aButton.delegate("endTrigger");
					return;
				}
			}
		}
	}
};

export const filterButtonStyle = new Style({ font:"Open Sans Semibold", size:14, color:[ WHITE, BLACK, BLACK, WHITE ] });

export const FilterButton = Line.template($ => ({
	skin:rectangleFilterButtonSkin, active:true, Behavior: FilterButtonBehavior,
	contents: [
		Container($, { width:24 }),
		Label($, { style:filterButtonStyle, string:$.name }),
		Container($, { width:10 }),
	],
}));

class NoneAllButtonBehavior extends ButtonBehavior {
	onTap(container) {
		container.container.delegate(this.data.message);
	}
};

export const NoneAllButton = Line.template($ => ({
	skin:roundedFilterButtonSkin, active:true, Behavior: NoneAllButtonBehavior,
	contents: [
		Container($, { width:10 }),
		Label($, { style:whiteButtonStyle, string:$.name }),
		Container($, { width:10 }),
	],
}));

// Read / Write Switch Button

var readWriteButtonTexture = new Texture("assets/read-write-toggle-switch.png", 1);
var readWriteSwitchBarSkin = new Skin({ texture: readWriteButtonTexture, x:100, y:0, width:60, height:40, 
	tiles: { left:20, right: 20 },
	states: 40
});
var readWriteSwitchButtonSkin = new Skin({ texture: readWriteButtonTexture, x:160, y:0, width:40, height:40, 
	states: 40
});
var readWriteSwitchTextSkin = new Skin({ texture: readWriteButtonTexture, x:200, y:0, width:60, height:40, 
	states: 40
});

export class I2CReadWriteSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		var switchData = { sizeInset : 9, value : data.isWrite ? 1 : 0, i2cPin:data };
		super.onCreate(container, switchData);
	}
	onValueChanged(container, value) {
		super.onValueChanged(container, value);
		if (this.data.value == 1) {
			this.data.i2cPin.READ_WRITE_BUTTON_LABEL.string = "Write Pins";
			this.data.i2cPin.isWrite = true;
		}
		else {
			this.data.i2cPin.READ_WRITE_BUTTON_LABEL.string = "Read Pins";
			this.data.i2cPin.isWrite = false;
		}
		container.container.container.distribute("onI2CMessageTypeChanged");
	}
};

export var I2CReadWriteSwitch = Container.template($ => ({
	width:100, height:40, active:true,
	Behavior: I2CReadWriteSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:readWriteSwitchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:readWriteSwitchBarSkin, state:1 }),
		Container($, { left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, { left:0,
					contents: [
						Content($, { skin:readWriteSwitchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:readWriteSwitchButtonSkin, state:0 }),
						Content($, { skin:readWriteSwitchTextSkin, state:0 }),
					]
				}),
			],
		}),
	],
}));

// Byte / Word Switch Button

var byteWordButtonTexture = new Texture("assets/size-toggle-switch.png", 1);
var byteWordSwitchBarSkin = new Skin({ texture: byteWordButtonTexture, x:100, y:0, width:60, height:40, 
	tiles: { left:20, right: 20 },
	states: 40
});
var byteWordSwitchButtonSkin = new Skin({ texture: byteWordButtonTexture, x:160, y:0, width:40, height:40, 
	states: 40
});
var byteWordSwitchTextSkin = new Skin({ texture: byteWordButtonTexture, x:200, y:0, width:60, height:40, 
	states: 40
});

export class I2CByteWordSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		var switchData = { sizeInset : 9, value : data.isWord ? 1 : 0, i2cPin:data };
		super.onCreate(container, switchData);
	}
	onValueChanged(container, value) {
		super.onValueChanged(container, value);
		if (this.data.value == 1)
			this.data.i2cPin.isWord = true;
		else
			this.data.i2cPin.isWord = false;
		container.container.container.distribute("onI2CMessageTypeChanged");
	}
};

export var I2CByteWordSwitch = Container.template($ => ({
	width:100, height:40, active:true,
	Behavior: I2CByteWordSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:byteWordSwitchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:byteWordSwitchBarSkin, state:1 }),
		Container($, { left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, { left:0,
					contents: [
						Content($, { skin:byteWordSwitchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:byteWordSwitchButtonSkin, state:0 }),
						Content($, { skin:byteWordSwitchTextSkin, state:0 }),
					]
				}),
			],
		}),
	],
}));

// Scan / Manual Switch Button

var scanManualButtonTexture = new Texture("assets/scan-manual-toggle-switch.png", 1);
var scanManualSwitchBarSkin = new Skin({ texture: scanManualButtonTexture, x:100, y:0, width:60, height:40, 
	tiles: { left:20, right: 20 },
	states: 40
});
var scanManualSwitchButtonSkin = new Skin({ texture: scanManualButtonTexture, x:160, y:0, width:40, height:40, 
	states: 40
});
var scanManualSwitchTextSkin = new Skin({ texture: scanManualButtonTexture, x:200, y:0, width:80, height:40, 
	states: 40
});

export class ScanManualSwitchBehavior extends SwitchButtonBehavior {
	onCreate(container, data) {
		data.sizeInset = 9;
		super.onCreate(container, data);
	}
	onValueChanged(container, value) {
		super.onValueChanged(container, value);
		application.distribute("onRebuildAddressEditor");
	}
};

export var ScanManualSwitch = Container.template($ => ({
	width:120, height:40, active:true,
	Behavior: ScanManualSwitchBehavior,
	contents: [
		Content($, { left:0, right:0, top:0, bottom:0, skin:scanManualSwitchBarSkin, state:0 }),
		Content($, { left:0, width:40, top:0, bottom:0, skin:scanManualSwitchBarSkin, state:1 }),
		Container($, { left:9, right:9, top:0, bottom:0, clip:true,
			contents: [
				Line($, { left:0,
					contents: [
						Content($, { skin:scanManualSwitchTextSkin, state:1 }),
						Content($, { width:40, top:0, bottom:0, skin:scanManualSwitchButtonSkin, state:0 }),
						Content($, { skin:scanManualSwitchTextSkin, state:0 }),
					]
				}),
			],
		}),
	],
}));

// Read Write Pins Button

const readWritePinsTexture2 = new Texture("assets/generic-3-part-2.png", 1);
const readWritePinsSkin2 = new Skin({ texture:readWritePinsTexture2, x:0, y:0, width:25, height:25, states:25,
	tiles: { left:5, right:5, top:5, bottom:5 },
});

const readWritePinsButtonStyle = new Style({ font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"middle", vertical:"center" });

class I2CReadWritePinsButtonBehavior extends Behavior {
	onCreate(container, data) {
		this.i2cPin = data;
	}
	changeState(container, state) {
		container.state = state;
		var content = container.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.state = 1;
	}
	onTouchEnded(container, id, x, y, ticks) {
		container.state = 0;
		this.sendI2CMessage(container);
	}
	bindBehaviorCallback(container, functionName) {
		return function(param) {
			container.delegate(functionName, param);
		}
	}
	onI2CMessageTypeChanged(container) {
		var i2cPin = this.i2cPin;
		container.active = 	
				(i2cPin.addressString.length == 2) && (i2cPin.registerString.length == 2)
			&& 	((i2cPin.isWrite && (false == i2cPin.isWord) && (i2cPin.writByteValueString.length == 2))
			||	(i2cPin.isWrite && (true == i2cPin.isWord) && (i2cPin.writWordValueString.length == 4)));
	}
	sendI2CMessage(container) {
		if (! container.active)
			return;
			
		var i2cPin = this.i2cPin;
		if (i2cPin.isWrite) {
			if (i2cPin.isWord)					// write word
				i2cPin.writeWordDataSMB(i2cPin.register, i2cPin.writeWordValue);
			else								// write byte
				i2cPin.writeByteDataSMB(i2cPin.register, i2cPin.writeByteValue);
		}
		else {
			if (i2cPin.isWord) {				// read word
				var callback = this.bindBehaviorCallback(container, "onReadWordDataSMB");
				i2cPin.readWordDataSMB(i2cPin.register, callback);
			}
			else {								// read byte
				var callback = this.bindBehaviorCallback(container, "onReadByteDataSMB");
				i2cPin.readByteDataSMB(i2cPin.register, callback);
			}
		}
	}
	onReadWordDataSMB(container, value) {
		this.i2cPin.readWordResult = value;
		container.container.container.distribute("onDataPropertyChanged");
	}
	onReadByteDataSMB(container, value) {
		this.i2cPin.readByteResult = value;
		container.container.container.distribute("onDataPropertyChanged");
	}
};

export var I2CReadWritePinsButton = Container.template($ => ({
	active:"true", height:"25", width:"100", skin:readWritePinsSkin2,
	Behavior: I2CReadWritePinsButtonBehavior,
	contents: [
		Label($, { 
			anchor:"READ_WRITE_BUTTON_LABEL", style:readWritePinsButtonStyle,
			Behavior: class extends Behavior {
				onCreate(label, data) {
					if (data.isWrite)
						label.string = "Write Pins";
					else
						label.string = "Read Pins";
				}
			}
		}),
	]
}));

// I2C READ ONLY / EDiTABLE, WORD / BYTE LABELS

const i2CReadOnlyValueStyle = new Style({ font:SEMIBOLD_FONT, size:24, color:BLACK, horizontal:"left", vertical:"middle" });

const i2CValueStyle = new Style({ font:SEMIBOLD_FONT, size:24, color:BLACK, horizontal:"left", vertical:"middle" });

var decimalToByteHexString = function(number) {
	if (number < 0)
		number = 0xFFFFFFFF + number + 1;
	number = number.toString(16).toUpperCase();
	if (number.length == 1)
		number = "0" + number;
	return number;
}	
	
var decimalToWordHexString = function(number) {
	if (number < 0)
		number = 0xFFFFFFFF + number + 1;
	number = number.toString(16).toUpperCase();
	while (number.length < 4)
		number = "0" + number;
	return number;
}	
	
class ByteLabelBehavior extends Behavior {
	onCreate(label, data) {
		this.data = data;
		this.updateLabel(label);
	}
	updateLabel(label) {
		let value = this.data.readByteResult;
		if (value == -1)
			label.string = "None";
		else
			label.string = decimalToByteHexString(value);
	}
	onDataPropertyChanged(label, propertyName, value) {
		this.updateLabel(label);
	}
};

class WordLabelBehavior extends Behavior {
	onCreate(label, data) {
		this.data = data;
		this.updateLabel(label);
	}
	updateLabel(label) {
		let value = this.data.readWordResult;
		if (value == -1)
			label.string = "None";
		else
			label.string = decimalToWordHexString(value);
	}
	onDataPropertyChanged(label, propertyName, value) {
		this.updateLabel(label);
	}
};

export var I2CByteLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Container($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			contents: [
				Label($, {
					anchor:"BYTE_VALUE", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"00",
					Behavior: ByteLabelBehavior,
				}),
			],
		}),
	]
}));

export var I2CWordLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Container($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			contents: [
				Label($, {
					anchor:"WORD_VALUE", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"0000",
					Behavior: WordLabelBehavior,
				}),
			],
		}),
	]
}));

var hexLabelStyle = new Style({ font: "Open Sans Semibold", size:16, color:BLACK, horizontal:"left" })
export const hexFieldLabelStyle = new Style({ font: "Open Sans", size:16, color:BLACK, horizontal:"right", left:7 })

class HexFieldLabelBehavior extends FieldLabelBehavior {
	insertTheKey(label, key) {
		let theKey = key;
		switch (key) {
			case "a":
			case "A":
				theKey = "A";
				break;
			case "b":
			case "B":
				theKey = "B";
				break;
			case "c":
			case "C":
				theKey = "C";
				break;
			case "d":
			case "D":
				theKey = "D";
				break;
			case "e":
			case "E":
				theKey = "E";
				break;
			case "f":
			case "F":
				theKey = "F";
				break;
			case "0": case "1": case "2": case "3": case "4": case "5": case "6": case "7": case "8": case "9": 
				break
			default:
				var c = theKey.charCodeAt(0);
				if ((c >= 32) && (c <= 126))
					return;

		}
		label.insert(theKey);
	}
}

class ByteFieldLabelBehavior extends HexFieldLabelBehavior {
	onCreate(label, data) {
		super.onCreate(label, data);
		this.i2cPin = data;
	}
	insertKey(label, key) {
		if ((label.string.length < 2) || (label.selectionLength > 0))
			this.insertTheKey(label, key);
	}
	onEdited(label) {
		this.i2cPin.writeByteValueString = label.string;
		this.i2cPin.writeByteValue = parseInt(label.string, 16);
	}
}

export var I2CEditableByteLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Scroller($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"BYTE_FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"00",
					Behavior: ByteFieldLabelBehavior,
				}),
			],
		}),
	]
}));

class WordFieldLabelBehavior extends HexFieldLabelBehavior {
	onCreate(label, data) {
		super.onCreate(label, data);
		this.i2cPin = data;
	}
	insertKey(label, key) {
		if ((label.string.length < 4) || (label.selectionLength > 0))
			this.insertTheKey(label, key);
	}
	onEdited(label) {
		this.i2cPin.writeWordString = label.string;
		this.i2cPin.writeWordValue = parseInt(label.string, 16);
	}
}

export var I2CEditableWordLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Scroller($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"WORD_FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"0000",
					Behavior: WordFieldLabelBehavior,
				}),
			],
		}),
	]
}));

class RegisterFieldLabelBehavior extends HexFieldLabelBehavior {
	onCreate(label, data) {
		super.onCreate(label, data);
		this.i2cPin = data;
	}
	insertKey(label, key) {
		if ((label.string.length < 2) || (label.selectionLength > 0))
			this.insertTheKey(label, key);
	}
	onEdited(label) {
		this.i2cPin.registerString = label.string;
		this.i2cPin.register = parseInt(label.string, 16);
	}
}

export var I2CRegisterLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Scroller($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"REGISTER_FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"00",
					Behavior: RegisterFieldLabelBehavior,
				}),
			],
		}),
	]
}));

class AddressFieldLabelBehavior extends HexFieldLabelBehavior {
	onCreate(label, data) {
		super.onCreate(label, data);
		this.i2cPin = data;
	}
	insertKey(label, key) {
		if ((label.string.length < 2) || (label.selectionLength > 0))
			this.insertTheKey(label, key);
	}
	onEdited(label) {
		this.i2cPin.addressString = label.string;
		if (label.string.length == 2) {
			let address = this.i2cPin.address = parseInt(label.string, 16);
			this.i2cPin.setAddress(address);
		}
	}
}

export var I2CAddressLabel = Line.template($ => ({
	left:0, top:0, right:0, bottom:0, skin:blackSkin,
	contents: [
		Label($, { left:0, bottom:0, top:0, width:22, style:hexLabelStyle, skin:whiteSkin, string:"0x" }),
		Scroller($, {
			left:1, right:1, top:1, bottom:1, clip:true, active:true, skin:whiteSkin,
			Behavior: FieldScrollerBehavior,
			contents: [
				Label($, {
					anchor:"ADDRESS_FIELD", left: 0, top:2, bottom:2, skin:fieldLabelSkin, style:hexFieldLabelStyle, editable:true, string:"00",
					Behavior: AddressFieldLabelBehavior,
				}),
			],
		}),
	]
}));

/***

const settingNameStyle = new Style({ font:NORMAL_FONT, size:14, color:BLACK, horizontal:"left", left:10 });
const settingLineSkin = new Skin({ fill: [WHITE, PASTEL_ORANGE, PASTEL_ORANGE, PASTEL_ORANGE], stroke:PASTEL_ORANGE, borders:{ bottom:1 } });

class StartupAppSetting {
	constructor($) {
		this.device = $.device;
		this.title = "Startup App";
	}
	load(line) {
		let device = this.device;
		device.getStartupApp().then(json => {
			this.value = json;
			return device.getStartupAppList();
		}).then(json => {
			this.items = json.map(item => {
				let result = new StartupAppItem(item);
				if (this.value.id == result.id)
					this.value = result;
				return result;
			});
			if (!line.container) return;
			line.container.replace(line, new PopupSettingLine(this));
		});
	}
	put() {
		let message = new Message(mergeURI(this.device.url, "settings/startup-app"));
		message.requestText = JSON.stringify(this.value);
		message.method = "PUT"
		message.invoke();
	}
}

class SettingBehavior extends LineBehavior {
	onFocused(line) {
		this.select(line, true);
	}
	onUnfocused(line) {
		this.select(line, false);
	}
}

class PopupSettingBehavior extends SettingBehavior {
	changeState(line) {
		super.changeState(line);
		let arrow =	line.first.next.first.next;
		arrow.visible = arrow.next.visible = this.flags != 0;
	}
	onCreate(line, data) {
		super.onCreate(line, data);
		data.LABEL.string = data.value;
	}
	onMenuSelected(line, item) {
		let data = this.data;
		if (item) {
			data.value = item;
			data.put();
			data.LABEL.string = data.value;
		}
		shell.focus();
	}
	onTap(line) {
		this.changeState(line, 1);
		let data = this.data;
		let description = {
			ItemTemplate:PopupItem,
			button:line.first.next,
			items:data.items,
			value:data.value,
			context:shell,
		};
		shell.add(new PopupDialog(description));
		line.focus();
	}
}

var PopupSettingLine = Line.template($ => ({
	left:0, right:0, height:40, skin:settingLineSkin, active:true,
	Behavior: PopupSettingBehavior,
	contents: [
		Label($, { width:160, style:settingNameStyle, string:$.title }),
		Container($, { left:0, right:0, skin:whiteButtonSkin, state:1,
			contents: [
				Label($, { left:0, right:0, anchor:"LABEL", style:settingValueStyle }),
				Content($, { right:5, top:5, skin:popupArrowsSkin, state:0, visible:false }),
				Content($, { right:5, bottom:5, skin:popupArrowsSkin, state:1, visible:false }),
			]
		}),
		Content($, { width:160 }),
	],
}));

***/