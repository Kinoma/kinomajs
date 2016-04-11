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

import {
	BLACK,
	SEMIBOLD_FONT,
	WHITE,
	fileGlyphsSkin,
	grayFooterSkin,
	greenHeaderSkin,
} from "shell/assets";

import { 
	HeaderBehavior,
	TableBehavior,
} from "shell/behaviors";

import {
	blackSkin,
	blackStyle,
	whiteSkin,
} from "features/devices/assets";

import {
	SampleGraphProbeContainer,
	SampleGraphContainer,
} from "features/devices/graphic";

import {
	BigOnOffSwitch,
	InOutSwitch,
	DigitalOutControlsButton,
	MomentaryButton,
	MotorLEDSwitch,
	gSelectedDigitalOutControlIndex,
	
	gSelectedPWMControlIndex,
	PWMControlsButton,
} from "controls";

import {
	SquareWaveformControl,
	PWMSlider,
	SineWaveformControl,
	TriangleWaveformControl
} from "features/devices/waveforms";

// import { HorizontalSlider, HorizontalSliderBehavior, horizontalSliderBarSkin, horizontalSliderButtonSkin } from "common/slider";
// import { HorizontalSwitch, HorizontalSwitchBehavior, horizontalSwitchSkin, horizontalSwitchStyle } from "common/switch";

var gIndexToPin = {
	left: [
		[51,52,53,54,55,56,57,58],
		[42,43,44,45,46,47,48,49],
		[],
	],
	right: [
		[59,60,61,62,63,64,65,66],
		[0,1,2,3,18,19,20,21],
		[],
	],
};

var gIndexToMap = {
	left: [
		[38,37,40,39,44,43,48,47],
		[],
		[]
	],
	right: [
		[],
		[],
		[]
	],
};

var pinLabelStyle = new Style({font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"left" });
var pinNumberStyle = new Style({font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"center", left:4, right:4 });
var levelPopupPointerSkin = new Skin({ texture:new Texture("./assets/value-inspector-pointer.png", 1), x:0, y:0, width:17, height:9 });
var levelPopupStyle = new Style({font:"bold", size:18, color:BLACK, horizontal:"center", vertical:"middle"});
var levelPopupSkin = new Skin({ texture:new Texture("./assets/level-popup.png", 1), x:0, y:0, width:30, height:30, tiles: { left:10, right:10 } });

var Separator = Content.template($ => ({ left:0, right:0, bottom:0, height:1, skin:blackSkin }));

var PinLabel = Label.template($ => ({ style:pinLabelStyle, string:$.type }));
var PinDirectionLabel = Label.template($ => ({ left:10, style:pinLabelStyle, string:$.direction }));
var PinVoltageLabel = Label.template($ => ({ left:10, style:pinLabelStyle, string:$.voltage }));
var PinNumberLabel = Label.template($ => ({ right:10, style:pinNumberStyle, skin:whiteSkin, string:$.index }));

// var sliderTexture = new Texture("./assets/slider.png", 1);
// horizontalSliderBarSkin = new Skin({ texture: sliderTexture, x:10, y:0, width:60, height:40, tiles:{ left:10, right:10 }, states:40 });
// horizontalSliderButtonSkin = new Skin({ texture: sliderTexture, x:90, y:0, width:20, height:40, states:40 });
// export var Slider = Line.template($ => ({
// 	left:0, right:0, height:50,
// 	contents: [
// 		Label($, { width:120, top:0, bottom:0, string:$.label, style:nameStyle }),
// 		HorizontalSlider($, { 
// 			left:0, right:0,
// 			Behavior: class extends HorizontalSliderBehavior {
// 				onLayoutChanged(container) {
// 					super.onLayoutChanged(container);
// 					container.next.string = this.getValue(container) + this.data.unit;
// 				}
// 				onValueChanged(container) {
// // 					shell.behavior.controller[this.data.event](this.getValue(container));
// 				}
// 			},
// 		}),
// 		Label($, { width:60, top:0, bottom:0, style:valueStyle }),
// 	]
// }));
// 
// var switchTexture = new Texture("./assets/switch.png", 1);
// horizontalSwitchSkin = new Skin({ texture: switchTexture, x:0, y:0, width:80, height:40, tiles:{ left:2, right:2 }, states:40, variants:80 });
// horizontalSwitchStyle = new Style({ size:12 });
// export var Switch = Line.template($ => ({
// 	left:0, right:0, height:50,
// 	contents: [
// 		Label($, { width:140, top:0, bottom:0, string:"", style:blackStyle }),
// 		HorizontalSwitch($, { 
// 			left:0, right:0 , top:0, bottom:0,
// 			Behavior: class extends HorizontalSwitchBehavior {
// 				onCreate(container, $) {
// 					super.onCreate(container, $);
// 					this.data = $;
// 				}
// 				onValueChanged(container) {
// 					this.data.invoke(container, this.getValue(container));
// 				}
// 			},
// 		}),
// 		Content($, { width:80 }),
// 	]
// }));

class GenericPinBehavior extends TableBehavior {
	addLines(column) {
		let data = this.data;
		let header = column.first;
		column.empty(1);
		header.behavior.expand(header, data.expanded);
		if (data.expanded) {
			column.add(new data.explorer(data));
		}
		column.add(new this.footerTemplate(data));
	}
	onCreate(column, $) {
		this.data = $;
		this.footerTemplate = PinFooter;
		this.addLines(column);
	}
	onDisplayed(column) {
//		this.data.start(column);
	}
	onUndisplayed(column) {
		this.data.stop(column);
	}
	onUpdate(container) {
		if ("GRAPH" in this.data) {
			let graph = this.data.GRAPH;
			graph.delegate("onUpdate", this.data.value);
		}
	}
	onTimeChanged(container) {
		if (this.data.waveGenerator != null)
			this.data.value = this.data.waveGenerator.onTimeChanged(container.time);
		this.onUpdate(container);
	}
	toggle(column) {
		var data = this.data;
		if (data) {
			data.expanded = !data.expanded;
			if (data.expanded)
				data.start(column);
			else
				data.stop(column);
		}
		this.addLines(column);
	}
	trigger(column, line) {
		debugger
	}
}

var PinFooter = Container.template($ => ({
	left:0, right:0, height:10, skin:grayFooterSkin,
}));

// GENERIC PIN

var GenericPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		Content($, { left:0, right:0 }),
		PinNumberLabel($),
	],
}));

var GenericPinContainer = Column.template($ => ({
	left:0, right:0, active:false, clip:true, Behavior:GenericPinBehavior,
	contents: [
		$.header($),
	],
}));

// POWER PIN

var PowerPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		PinVoltageLabel($),
		Content($, { left:0, right:0 }),
		PinNumberLabel($),
	],
}));

var redSkin = new Skin({ fill:"red" });		//*
var blueSkin = new Skin({ fill:"blue" });

// ANALOG PIN

var AnalogPinExplorer = Container.template($ => ({
	left:0, right:0, height:105, skin:whiteSkin,
	contents: [
		SampleGraphContainer($, { anchor: "GRAPH" }),
		SampleGraphProbeContainer($),
	],
}));

// DIGITAL PIN

var DigitalPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		PinDirectionLabel($),
		Content($, { left:0, right:0 }),
		PinNumberLabel($),
	],
}));

class DigitalPinExplorerBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		container.delegate("setDirection", this.data.direction);
	}
	setDirection(container, direction) {
		let inputPane = this.data.DigitalInputPane;
		let outputPane = this.data.DigitalOutputPane;
		if (direction == "input") {
			inputPane.visible = inputPane.active = true;
			outputPane.visible = outputPane.active = false;
		}
		else {
			inputPane.visible = inputPane.active = false;
			outputPane.visible = outputPane.active = true;
		}
		this.data.setDirection(direction);
	}
	onUpdate(container) {
		if (this.data.direction == "input")
			this.data.DigitalInputPane.delegate("onUpdate");
	}
}

var DigitalPinExplorer = Container.template($ => ({
	left:0, right:0, height:155, skin:whiteSkin,
	Behavior: DigitalPinExplorerBehavior,
	contents: [
		Container($, {left:0, top:0, right:0, bottom:0,
			contents: [
				DigitalInputPinPane($, { anchor: "DigitalInputPane" }),
				DigitalOutputPinPane($, { anchor: "DigitalOutputPane" })
			]
		}),
		InOutSwitch($, { left:20, top:6 } ),
		SampleGraphContainer($, { anchor: "GRAPH" }),
		SampleGraphProbeContainer($),
	],
}));

var DigitalInputPinPane = Container.template($ => ({
	left:0, top:0, right:0, bottom:0,
}));

var DigitalOutputPinPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, { anchor: "CONTROL_CONTAINER", left:0, top:0, bottom:70, right:0,
			Behavior: class extends Behavior {
				onCreate(container, data) {
					this.data = data;
					this.CONTROL_TYPES = { momentaryButton:0, switchButton:1, digitalSquare:2 };
					this.rebuildControl(container);
				}
				rebuildControl(container) {
					this.data.CONTROL_CONTAINER.empty();
					this.data.deleteWaveGenerator();
					switch(gSelectedDigitalOutControlIndex) {
						case this.CONTROL_TYPES.momentaryButton:
							var button = new MomentaryButton(this.data);
							this.data.CONTROL_CONTAINER.add( button );
						break
						case this.CONTROL_TYPES.switchButton:
							this.data.CONTROL_CONTAINER.add( new BigOnOffSwitch(this.data) );
						break
						case this.CONTROL_TYPES.digitalSquare:
							var control = new SquareWaveformControl(this.data, { top:5 })
							this.data.CONTROL_CONTAINER.add(control);
						break
					}
				}
			},
			contents: [			
			]
		}),		
		DigitalOutControlsButton($, { top:0, right:20 } ),
	],
}));

// PWM PIN

var PWMPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: HeaderBehavior,
	contents: [
		Content($, { width:0 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		Content($, { left:0, right:0 }),
		PinNumberLabel($),
	],
}));

class PWMPinExplorerBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
	}
	onUpdate(container) {
		this.data.PWMPane.delegate("onUpdate");
	}
}

var PWMPinExplorer = Container.template($ => ({
	left:0, right:0, height:155, skin:whiteSkin,
	Behavior: PWMPinExplorerBehavior,
	contents: [
		PWMPane($, {anchor:"PWMPane", left:0, top:0, right:0, bottom:0 ,}),
//*		MotorLEDSwitch($, { left:20, top:6 } ),
		SampleGraphContainer($, { anchor: "GRAPH" }),
		SampleGraphProbeContainer($),
	],
}));

var PWMPane = Container.template($ => ({
	left:0, right:0, top:0, bottom:0,
	contents: [
		Container($, { anchor: "CONTROL_CONTAINER", left:0, top:0, bottom:70, right:0,
			Behavior: class extends Behavior {
				onCreate(container, data) {
					this.data = data;
					this.CONTROL_TYPES = { slider:0, sine:1, triangle:2, square:3 };
					this.rebuildControl(container);
				}
				rebuildControl(container) {
					let controlContainer = this.data.CONTROL_CONTAINER;
					controlContainer.empty();
					this.data.deleteWaveGenerator();
					switch(gSelectedPWMControlIndex) {
						case this.CONTROL_TYPES.slider:
							controlContainer.add( new PWMSlider(this.data, { left:140, right:140, top:20, height:40 }) );
						break
						case this.CONTROL_TYPES.sine:
							controlContainer.add( new SineWaveformControl(this.data) );
						break
						case this.CONTROL_TYPES.triangle:
							controlContainer.add( new TriangleWaveformControl(this.data) );
						break
						case this.CONTROL_TYPES.square:
							controlContainer.add( new SquareWaveformControl(this.data) );
						break
					}
				}
			},
			contents: [			
			]
		}),		
		PWMControlsButton($, { top:0, right:20 } ),
	],
}));


// PIN CLASSES

export class GenericPin {
	constructor(device, pins, pin, info) {
		this.device = device;
		this.pins = pins;
		this.pin = pin;
		this.info = info;
		this.expanded = false;
		this.hertzValue = 0.5;
		this.waveGenerator = null;
		this.minRange = 0;
		this.maxRange = 1;
	}
	canExpand() {
		return false;
	}
	expand(expandIt) {
		if (this.canExpand())
			this.expanded = expandIt;
	}
	get explorer() {
		debugger
	}
	get index() {
		return this.info.pin;
	}
	get isFront() {
		switch (this.device.constructor.tag) {
			case "Create":
				return (this.info.pin > 50);
			break;
			case "Element":
				return true;
			break;
		}
	}
	get isReadable() {
		return false;
	}
	get header() {
		return GenericPinHeader;
	}
	get template() {
		return GenericPinContainer;
	}
	get type() {
		return this.info.type;
	}
	start(container) {
	}
	stop(container) {
	}
	deleteWaveGenerator(container) {
		if (null != this.waveGenerator) {
			delete this.waveGenerator;
			this.waveGenerator = null;	
		}
	}
}

class PowerPin extends GenericPin {
	get header() {
		return PowerPinHeader;
	}
	get path() {
		return null;
	}
	get voltage() {
		let voltage = this.info.voltage;
		return (voltage ? voltage : "3.3") + "V";
	}
}

class GroundPin extends GenericPin {
	get path() {
		return null;
	}
}

class AnalogPin extends GenericPin {
	canExpand() {
		return true;
	}
	get isReadable() {
		return true;
	}
	get path() {
		return "/" + this.pin + "/read";
	}
	get readPath() {
		return "/" + this.pin + "/read";
	}
	get explorer() {
		return AnalogPinExplorer;
	}
	onRepeat(value) {
		this.value = value;
		this.container.delegate("onUpdate");
	}
	start(container) {
		this.repeat = this.pins.repeat(this.readPath, 100, value => this.onRepeat(value));
		this.container = container;
		container.time = 0;
		container.start();
	}
	stop(container) {
		if ("repeat" in this) {
			this.repeat.close();
			delete this.repeat;
		}
		container.stop();
	}
}

class DigitalPin extends GenericPin {
	canExpand() {
		return true;
	}
	get direction() {
		return this.info.direction;
	}
	get header() {
		return DigitalPinHeader;
	}
	get isReadable() {
		return this.info.direction == "input";
	}
	get readPath() {
		return "/" + this.pin + "/read";
	}
	get writePath() {
		return "/" + this.pin + "/write";
	}
	get setDirectionPath() {
		return "/" + this.pin + "/setDirection";
	}
	get explorer() {
		return DigitalPinExplorer;
	}
	onRepeat(value) {
		this.value = value;
		this.container.delegate("onUpdate");
		if (this.waveGenerator != null && false == this.isReadable)
			this.waveGenerator.onTimeChanged(this.container.time);
	}
	start(container) {
		let interval = 50;
		if (this.isReadable)
			this.repeat = this.pins.repeat(this.readPath, interval, value => this.onRepeat(value));
		this.container = container;
		container.time = 0;
		container.start();
	}
	stop(container) {
		if ("repeat" in this) {
			this.repeat.close();
			delete this.repeat;
		}
		container.stop();
	}
	write(value) {
		if (false == this.isReadable)
			this.pins.invoke(this.writePath, value);
	}
	setDirection(direction) {
		this.info.direction = direction;
		this.pins.invoke(this.setDirectionPath, direction);
	}
}

class I2CPin extends GenericPin {
	get index() {
		return this.info.sda;
	}
	get isFront() {
		return (this.info.sda > 50) && (this.info.clock > 50);
	}
	get type() {
		return "I2C Data";
	}
}

class SerialPin extends GenericPin {
	get index() {
		return this.info.rx;
	}
	get isFront() {
		return (this.info.rx > 50) && (this.info.tx > 50);
	}
	get type() {
		return "Serial";
	}
}

class PWMPin extends GenericPin {
	get type() {
		return "PWM";
	}
	canExpand() {
		return true;
	}
	get header() {
		return PWMPinHeader;
	}
	get isReadable() {
		return false;
	}
	get writePath() {
		return "/" + this.pin + "/write";
	}
	get explorer() {
		return PWMPinExplorer;
	}
	onRepeat(value) {
		this.value = value;
		this.container.delegate("onUpdate");
		if (this.waveGenerator != null)
			this.waveGenerator.onTimeChanged(this.container.time);
	}
	start(container) {
		this.container = container;
		container.time = 0;
		container.start();
	}
	stop(container) {
		container.stop();
	}
	write(value) {
		this.pins.invoke(this.writePath, value);
	}
}

var gPinClass = {
	disconnected: null,
	power: PowerPin,
	ground: GroundPin,
	analog: AnalogPin,
	digital: DigitalPin,
	i2c: I2CPin,
	serial: SerialPin,
	pwm: PWMPin,
};

// PIN MANAGER

export class PinManager {
	static createProbes(device, pins, configuration) {
		let probes = [];
		for (let pin in configuration) {
			let probe = PinManager.createProbe(device, pins, pin, configuration[pin].pins);
			if (probe) {
				probes.push(probe);
			}
		}
		probes.sort(function(a, b) {
			if (a.index < b.index)
				return -1;
			else if (a.index > b.index)
				return 1;
			return 0;
		});
		return probes;
	}
	static createProbe(device, pins, pin, info) {
		let keys = Object.keys(info);
		if (keys.length != 1) return;
		let key = keys[0];
		trace("createProbe " + key + "\n");
		return new gPinClass[key](device, pins, pin, info[key]);
	}
}
