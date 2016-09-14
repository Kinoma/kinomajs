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
	NORMAL_FONT,
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
	gSelectedPWMControlIndex,
	PWMControlsButton,
} from "controls";

import {
	SquareWaveformControl,
	SineWaveformControl,
	TriangleWaveformControl,
	CanvasSliderBehavior,
} from "features/devices/waveforms";

// import { HorizontalSlider, HorizontalSliderBehavior, horizontalSliderBarSkin, horizontalSliderButtonSkin } from "common/slider";
// import { HorizontalSwitch, HorizontalSwitchBehavior, horizontalSwitchSkin, horizontalSwitchStyle } from "common/switch";

/***
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
***/

var pinLabelStyle = new Style({font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"left" });
var pinNumberStyle = new Style({font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"center", left:4, right:4 });
var pinDescriptionStyle = new Style({font:SEMIBOLD_FONT, size:14, color:WHITE, horizontal:"right" });
var levelPopupPointerSkin = new Skin({ texture:new Texture("./assets/value-inspector-pointer.png", 1), x:0, y:0, width:17, height:9 });
var levelPopupStyle = new Style({font:"bold", size:18, color:BLACK, horizontal:"center", vertical:"middle"});
var levelPopupSkin = new Skin({ texture:new Texture("./assets/level-popup.png", 1), x:0, y:0, width:30, height:30, tiles: { left:10, right:10 } });

var Separator = Content.template($ => ({ left:0, right:0, bottom:0, height:1, skin:blackSkin }));

var PinLabel = Label.template($ => ({ style:pinLabelStyle, string:$.type }));
var PinDirectionLabel = Label.template($ => ({ left:10, style:pinLabelStyle, string:$.direction, Behavior:PinDirectionLabelBehavior }));
var PinVoltageLabel = Label.template($ => ({ left:10, style:pinLabelStyle, string:$.voltage }));
var PinNumberLabel = Label.template($ => ({ right:10, style:pinNumberStyle, skin:whiteSkin, width:24, string:$.index }));
var PinDescriptionLabel = Label.template($ => ({ style:pinDescriptionStyle }));

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

export class PinHeaderBehavior extends HeaderBehavior {
	changeArrowState(line, state) {
		line.first.next.next.next.state = state;
	}
	onDisplaying(line) {
		this.addDeviceSpecificPinDescriptions(line);
	}
	addDeviceSpecificPinDescriptions(line) {
		let deviceTag = this.data.device.constructor.tag;
		let pinNumber = this.data.info.pin;
		let text = "";
		switch(deviceTag) {
			case "Create":
				if (pinNumber <= 50)
					text = "Back";
				else if (pinNumber <= 58)
					text = "Front - Left";
				else if (pinNumber <= 66)
					text = "Front - Right";
			break
			case "Element":
				if (pinNumber <= 8)
					text = "Left";
				else if (pinNumber <= 16)
					text = "Right";
			break
		}
		line.add( new PinDescriptionLabel(this.data, { right:10, string:text }) );
	}
};

var GenericPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: PinHeaderBehavior,
	contents: [
		Content($, { width:8 }),
		PinNumberLabel($),
		Content($, { width:-10 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		Content($, { left:0, right:0 }),
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
	Behavior: PinHeaderBehavior,
	contents: [
		Content($, { width:8 }),
		PinNumberLabel($),
		Content($, { width:-10 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		PinVoltageLabel($),
		Content($, { left:0, right:0 }),
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

class PinDirectionLabelBehavior extends Behavior {
	onDigitalDirectionChanged(label, newDirection) {
		label.string = newDirection;
	}
}

var DigitalPinHeader = Line.template($ => ({
	left:0, right:0, height:30, skin:greenHeaderSkin, active:$.canExpand(),
	Behavior: PinHeaderBehavior,
	contents: [
		Content($, { width:8 }),
		PinNumberLabel($),
		Content($, { width:-10 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		PinDirectionLabel($),
		Content($, { left:0, right:0 }),
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
					switch(this.data.controlIndex) {
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
	Behavior: PinHeaderBehavior,
	contents: [
		Content($, { width:8 }),
		PinNumberLabel($),
		Content($, { width:-10 }),
		Content($, { width:30, height:30, skin:fileGlyphsSkin, visible:$.canExpand(), state:$.expanded ? 3 : 1, variant:1 }),
		PinLabel($),
		Content($, { left:0, right:0 }),
	],
}));

class PWMPinExplorerBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		container.delegate("setPWMMode", this.data.mode);
	}
	setPWMMode(container, mode) {
		let ledPane = this.data.PWMLEDPane;
		let motorPane = this.data.PWMMotorPane;
		if (mode == "led") {
			motorPane.visible = motorPane.active = false;
			motorPane.stop();
			ledPane.visible = ledPane.active = true;
			ledPane.start();
		}
		else {
			ledPane.visible = ledPane.active = false;
			ledPane.stop();
			motorPane.visible = motorPane.active = true;
			motorPane.start();
		}
		this.data.mode = mode;
	}
	onUpdate(container) {
		if (this.data.mode == "led")
			this.data.PWMLEDPane.delegate("onUpdate");
		else
			this.data.PWMMotorPane.delegate("onUpdate");
	}
}

var PWMPinExplorer = Container.template($ => ({
	left:0, right:0, height:205, skin:whiteSkin,
	Behavior: PWMPinExplorerBehavior,
	contents: [
		Container($, {left:0, top:0, right:0, bottom:0,
			contents: [
				PWMLEDPane($, {anchor:"PWMLEDPane", left:0, top:0, right:0, bottom:0 }),
				PWMMotorPane($, {anchor:"PWMMotorPane", left:0, top:0, right:0, bottom:0 }),
			]
		}),
		MotorLEDSwitch($, { left:20, top:0 } ),
	],
}));

class PWMLEDPaneBehavior extends Behavior {
	onCreate(column, data) {
		this.data = data;
	}
	onTimeChanged(container) {
		if ("LED_GRAPH" in this.data) {
			let graph = this.data.LED_GRAPH;
			graph.delegate("onUpdate", this.data.value);
		}
	}
}

var PWMLEDPane = Container.template($ => ({
	Behavior:PWMLEDPaneBehavior,
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
							controlContainer.add( new PWMSlider(this.data, { left:10, right:10, top:60, height:40 }) );
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
		SampleGraphContainer($, { anchor: "LED_GRAPH" }),
		SampleGraphProbeContainer($),
	],
}));

class PWMMotorPaneBehavior extends Behavior {
	onCreate(column, data) {
		this.data = data;
	}
	onTimeChanged(container) {
		let data = this.data;
		if ("PERIOD_GRAPH" in data && (undefined != gPeriodValue)) {
			let periodGraph = data.PERIOD_GRAPH;
			let periodSlider = data.PWM_PERIOD_SLIDER;
			let fraction = periodSlider.delegate("getFractionalValue");
			periodGraph.delegate("onUpdate", fraction);
		}
		if ("DUTY_CYCLE_GRAPH" in data && (undefined != gDutyCycleValue)) {
			let dutyCycleGraph = data.DUTY_CYCLE_GRAPH;
			let dutyCycleSlider = data.PWM_DUTY_CYCLE_SLIDER;
			let fraction = dutyCycleSlider.delegate("getFractionalValue");
			dutyCycleGraph.delegate("onUpdate", fraction);
		}
	}
}

var PWMMotorPane = Container.template($ => ({
	Behavior:PWMMotorPaneBehavior,
	left:0, right:0, top:0, bottom:0,
	contents: [
		Layout($, {
			left:0, top:40, bottom:0, right:0,
			Behavior:SplitHorizontalLayoutBehavior,
			contents: [
				PWMPeriodColumn($, {}),
				PWMDutyCycleColumn($, {}),
			]
		}),
	],
}));

    
class AnalogOutputSliderBehavior extends CanvasSliderBehavior {
	onCreate(canvas, data) {
		if (false == "min" in data)	data.min = 0;
		if (false == "max" in data) data.max = 1;
		if (false == "value" in data) data.value = 0;
		CanvasSliderBehavior.prototype.onCreate.call(this, canvas, data);
		this.data = data;
		this.pinNumber = data.pinNumber;
	}
	onValueChanged(canvas) {
		CanvasSliderBehavior.prototype.onValueChanged.call(this, canvas);
		this.assertValue(canvas);
	}
	assertValue(canvas) {
		var value = this.data.value;
		this.data.write(value);
	}
}

class PWMSliderBehavior extends AnalogOutputSliderBehavior {
	onCreate(canvas, data) {
		AnalogOutputSliderBehavior.prototype.onCreate.call(this, canvas, data);
		data.deleteWaveGenerator();
	}
	onDisplaying(canvas) {
		AnalogOutputSliderBehavior.prototype.onDisplaying.call(this, canvas);
		canvas.start();
	}
	onTimeChanged(canvas) {
//		this.assertValue(canvas);
	}
}

export var PWMSlider = Container.template($ => ({
	contents: [
		Canvas($, { left:0, top:0, right:0, bottom:0, active:true, Behavior:PWMSliderBehavior })
	],
}));
   
var gPeriodValue = 30;

class PWMPeriodSliderBehavior extends CanvasSliderBehavior {
	onCreate(canvas, data) {
		this.data = data;
		var sliderData = { min: 1, max: 30, value: gPeriodValue, parentData: data };
		CanvasSliderBehavior.prototype.onCreate.call(this, canvas, sliderData);
	}
	onDisplaying(canvas) {
		canvas.delegate("setValue", gPeriodValue);
		CanvasSliderBehavior.prototype.onDisplaying.call(this, canvas);		// calls onValueChanged()
	}
	onValueChanged(canvas) {
		this.data.value = gPeriodValue = Math.round(this.data.value);
		CanvasSliderBehavior.prototype.onValueChanged.call(this, canvas);

		var parentData = this.data.parentData;
		parentData.PERIOD_LABEL.string = "Period: " + this.data.value.toString() + "ms";

		parentData.PWM_DUTY_CYCLE_SLIDER.delegate("onPeriodChanged", gPeriodValue);
	}
}

var PWMPeriodSlider = Container.template($ => ({
	left:0, top:0, width:240, height:40,
	contents: [
		Canvas($, { anchor:"PWM_PERIOD_SLIDER", left:0, top:0, right:0, bottom:0, active:true, Behavior:PWMPeriodSliderBehavior })
	],
}));

var gDutyCycleValue = 30;

class PWMDutyCycleSliderBehavior extends CanvasSliderBehavior {
	onCreate(canvas, data) {
		this.data = data;
		this.afterOnDisplaying = false;
		var sliderData = { min: 0, max: 30, value: gDutyCycleValue, parentData: data };
		CanvasSliderBehavior.prototype.onCreate.call(this, canvas, sliderData);
	}
	onPeriodChanged(canvas, period) {
		this.data.max = period;

		var parentData = this.data.parentData;
		if (this.data.value > period) {
			gDutyCycleValue = period;
			canvas.delegate("setValue", period);
		}

		if (this.afterOnDisplaying)
			canvas.delegate("onValueChanged");

		parentData.DUTY_CYCLE_MAX_LABEL.string = period;

		parentData.writeDutyCyclePeriod(gDutyCycleValue, period);
	}
	onDisplaying(canvas) {
		canvas.delegate("setValue", gDutyCycleValue);
		CanvasSliderBehavior.prototype.onDisplaying.call(this, canvas);
		this.afterOnDisplaying = true;
	}
	onValueChanged(canvas) {
		CanvasSliderBehavior.prototype.onValueChanged.call(this, canvas);
		var parentData = this.data.parentData;
		gDutyCycleValue = this.data.value;
		parentData.DUTY_CYCLE_LABEL.string = "Pulse Width: " + (Math.round(gDutyCycleValue * 100) / 100).toFixed(2).toString() + "ms";
		parentData.writeDutyCyclePeriod(gDutyCycleValue, gPeriodValue);
	}
}

var PWMDutyCycleSlider = Container.template($ => ({
	left:0, top:0, width:240, height:40,
	contents: [
		Canvas($, { anchor: "PWM_DUTY_CYCLE_SLIDER", left:0, top:0, right:0, bottom:0, active:true, Behavior:PWMDutyCycleSliderBehavior })
	],
}));

const leftSliderLabelStyle = new Style({ font:NORMAL_FONT, size:20, color:BLACK, horizontal:"left" });
const centerSliderLabelStyle = new Style({ font:NORMAL_FONT, size:20, color:BLACK, horizontal:"center" });
const rightSliderLabelStyle = new Style({ font:NORMAL_FONT, size:20, color:BLACK, horizontal:"right" });

export var PWMPeriodColumn = Column.template($ => ({
	left:0, top:42, right:0,
	contents: [
		Label($, { anchor:"PERIOD_LABEL", top:0, left:24, right:0, string:"Period: 30ms", style:leftSliderLabelStyle }),
		PWMPeriodSlider($, { left:10, top:-4, right:10 }),
		Container($, { 
			left:0, top:-4, right:0,
			contents: [
				Label($, { top:0, left:20, string:"1", style:leftSliderLabelStyle }),
				Label($, { top:0, right:15, string:"30", style:rightSliderLabelStyle }),
			]
		}),
		Container($, {
			left:20, right:0, top:0, bottom:0,
			contents: [
				SampleGraphContainer($, { anchor: "PERIOD_GRAPH" }),
				SampleGraphProbeContainer($),
			]
		}),
	],
}));

const dutyCycleValueLabelStyle = new Style({ font:NORMAL_FONT, size:20, color:BLACK, horizontal:"center,middle" });

export var PWMDutyCycleColumn = Column.template($ => ({
	left:0, top:0, right:0,
	contents: [
		Label($, { anchor:"DUTY_CYCLE_LABEL", top:0, left:24, right:0, string:"Pulse Width", style:leftSliderLabelStyle }),
		PWMDutyCycleSlider($, { left:10, top:-4, right:10 }),
		Container($, { 
			left:0, top:-4, right:0,
			contents: [
				Label($, { top:0, left:20, string:"0", style:leftSliderLabelStyle }),
				Label($, { anchor:"DUTY_CYCLE_MAX_LABEL", top:0, right:15, string:"30", style:rightSliderLabelStyle }),
			]
		}),
		Container($, {
			left:20, right:0, top:0, bottom:0,
			contents: [
				SampleGraphContainer($, { anchor: "DUTY_CYCLE_GRAPH" }),
				SampleGraphProbeContainer($),
			]
		}),
	],
}));


export class SplitHorizontalLayoutBehavior extends Behavior {
	onMeasureVertically(layout, width) {	
		var leftWidth = Math.floor(layout.width / 2);
		var leftContainer = layout.first;
		leftContainer.coordinates = { left:0, top:0, bottom:0, width:leftWidth };
		var rightWidth = layout.width - leftWidth;
		var rightContainer = layout.first.next;
		rightContainer.coordinates = { left:leftWidth, top:0, bottom:0, width:rightWidth }
	}
};

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
	constructor(device, pins, pin, info) {
		super(device, pins, pin, info);
		this.controlIndex = 0;
	}
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
		let interval = 100;
		if (this.isReadable)
			this.repeat = this.pins.repeat(this.readPath, interval, value => this.onRepeat(value));
		this.container = container;
		container.time = 0;
		container.start();
		container.container.distribute("onDigitalOutSelected");
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
	constructor(device, pins, pin, info) {
		super(device, pins, pin, info);
		this.mode = "led";				// "led" | "motor"
		this.period = undefined;
		this.dutyCycle = undefined;
	}
	get type() {
		return "PWM";
	}
	get hasMotorMode() {
		switch (this.device.constructor.tag) {
			case "Create":
				return this.isFront;
			break;
			case "Element":
				return true;
			break;
		}
	}
	get pwmMode() {
		return this.mode;
	}
	set pwmMode(mode) {
		if (false == this.hasMotorMode)
			return
		this.mode = mode;
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
	get writeDutyCyclePeriodPath() {
		return "/" + this.pin + "/writeDutyCyclePeriod";
	}
	get explorer() {
		return PWMPinExplorer;
	}
/*
	onRepeat(value) {
		this.value = value;
		this.container.delegate("onUpdate");
		if (this.waveGenerator != null)
			this.waveGenerator.onTimeChanged(this.container.time);
	}
*/
	start(container) {
		this.container = container;
		container.time = 0;
		container.start();
	}
	stop(container) {
		container.stop();
	}
	write(value) {
		if (this.mode == "led")
			this.pins.invoke(this.writePath, value);
	}
	writeDutyCyclePeriod(dutyCycle, period) {
		if (this.mode == "motor") {
			this.pins.invoke(this.writeDutyCyclePeriodPath, { dutyCycle:dutyCycle, period:period });
		}
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
			if ((pin != "leftVoltage") && (pin != "rightVoltage")) {
				let probe = PinManager.createProbe(device, pins, pin, configuration[pin].pins);
				if (probe) {
					probes.push(probe);
				}
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
