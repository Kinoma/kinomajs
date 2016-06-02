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
	BLACK,
	BOLD_FONT,
	SEMIBOLD_FONT,
	NORMAL_FONT,
	WHITE,
} from "shell/assets";

class WaveGenerator {
	constructor(pinData, outputName, rangeMin, rangeMax, minHertz, maxHertz, hertz) {
		this.pinData = pinData;
		this.outputName = outputName;
		this.setRange(rangeMin, rangeMax);
		this.setMinHertz(minHertz);
		this.setMaxHertz(maxHertz);
		this.setHertz(hertz);
		this.twoPI = 2 * Math.PI;
	}
	setRange(rangeMin, rangeMax) {
		this.rangeMin = rangeMin;
		this.rangeMax = rangeMax;
	}
	setMinHertz(minHertz) {
        this.minHertz = minHertz;
	}
	setMaxHertz(maxHertz) {
    	this.maxHertz = maxHertz;
	}
	setHertz(hertz) {
		let fraction = hertz / (this.maxHertz - this.minHertz);
		this.setHertzFraction(fraction);
	}
	setHertzFraction(hertzFraction) {
		if (hertzFraction > 1)	hertzFraction = 1;
		if (hertzFraction < 0)	hertzFraction = 0;
		this.hertzFactor = 1000 / this.lerp(this.minHertz, this.maxHertz, hertzFraction);
	}
	lerp(from, to, fraction) {
		return from + fraction * (to - from);
	}
	start(time) {
        this.startTime = time;
	}
	onTimeChanged(time) {
		let fraction = this.timeToFraction(time);
		let value = this.normalizedValueForFraction(fraction);
		this.setPinsValue(value);
		return value;
	}
	setPinsValue(value) {
	}
	timeToFraction(time) {
		let dt = time - this.startTime;
		dt = dt % this.hertzFactor;
		let fraction = dt / this.hertzFactor;
		return fraction;
	}
	applyRange(value) {
		let scale = this.rangeMax - this.rangeMin;
		let rangedValue = (scale * value) + this.rangeMin;
		return rangedValue;
	}
	normalizedValueForFraction(fraction) {		// subclasses override
	}
}

class PinWaveGenerator extends WaveGenerator {
	setPinsValue(value) {
		this.pinData.write(value);
	}
}

export class SineWaveGenerator extends PinWaveGenerator {
	normalizedValueForFraction(fraction) {
		let value = this.valueForFraction(fraction)
		value = (1 + value) / 2;
		value = this.applyRange(value);
		return value;
	}
	valueForFraction(fraction) {
		return Math.sin(fraction * this.twoPI);
	}
}

export class TriangleWaveGenerator extends PinWaveGenerator {
	normalizedValueForFraction(fraction) {
		let amplitude = 1;
		let normalizedValue = ((2 * amplitude) / Math.PI) * Math.asin( Math.sin( this.twoPI * fraction ) );
		normalizedValue = (1 + normalizedValue) / 2;
		normalizedValue = this.applyRange(normalizedValue);
		return normalizedValue;
	}
}

export class SquareWaveGenerator extends PinWaveGenerator {
	normalizedValueForFraction(fraction) {
		let normalizedValue = 2 * (2 * Math.floor(fraction) - Math.floor(2 * fraction) + 0.5);
		normalizedValue = (1 + normalizedValue) / 2;
		normalizedValue = this.applyRange(normalizedValue);
		return normalizedValue;
	}
}


class WaveformControlBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		this.pinNumber = data.pinNumber;
		if ("outputName" in data)
			this.outputName = data.outputName;
		else
			this.outputName = "PWM " + data.pinNumber;
		this.data.waveGenerator = this.createWaveGenerator(container);
		this.data.waveGenerator.start(0);
	}
	createWaveGenerator(container) {
	}
	onRangeChanged(container, minRange, maxRange) {
		this.data.waveGenerator.setRange(minRange, maxRange);
	}
	onHertzFractionChanged(container, hertzFraction) {
		this.data.waveGenerator.setHertzFraction(hertzFraction);
	}
}

var AnalogWaveformControl = Container.template($ => ({
	top:0, bottom:0, anchor:"WAVEFORM_CONTROL",
	Behavior: WaveformControlBehavior,
	contents: [
		Line($, {
			contents: [
				Column($, {
					contents: [
						RangeSliderLabel($, {}),
						RangeSlider($, {})
					]
				}),
				Column($, {
					contents: [
						HertzSliderLabel($, {}),
						HertzSlider({parentData:$}, {})
					]
				})
			]
		})
	],
}));

var DigitalWaveformControl = Column.template($ => ({
	left:0, right:0, top:0, anchor:"WAVEFORM_CONTROL",
	Behavior: WaveformControlBehavior,
	contents: [
		HertzSliderLabel($, {}),
		HertzSlider({parentData:$}, {}),
	],
}));

class SineWaveformControlBehavior extends WaveformControlBehavior {
	createWaveGenerator(container) {
		let generator = this.data.waveGenerator;
		if (null === generator)
			generator = new SineWaveGenerator(this.data, this.data.pin, 0, 1, 0.25, 4, 2);
		return generator;
	}
}

export var SineWaveformControl = AnalogWaveformControl.template($ => ({
	Behavior: SineWaveformControlBehavior,
}));

class TriangleWaveformControlBehavior extends WaveformControlBehavior {
	createWaveGenerator(container) {
		let generator = this.data.waveGenerator;
		if (null === generator)
			generator = new TriangleWaveGenerator(this.data, this.data.pin, 0, 1, 0.25, 4, 1);
		return generator;
	}
}

export var TriangleWaveformControl = AnalogWaveformControl.template($ => ({
	Behavior: TriangleWaveformControlBehavior,
}));

class SquareWaveformControlBehavior extends WaveformControlBehavior {
	createWaveGenerator(container) {
		let generator = this.data.waveGenerator;
		if (null === generator)
			generator = new SquareWaveGenerator(this.data, this.data.pin, 0, 1, 0.25, 4, 1);
		return generator;	
	}
}

export var SquareWaveformControl = DigitalWaveformControl.template($ => ({
	Behavior: SquareWaveformControlBehavior,
}));

function roundRect(ctx, x, y, width, height, radius, fill, stroke) {
	if (typeof stroke == undefined)
		stroke = true;
	if (typeof radius == undefined)
		radius = 5;
	ctx.beginPath();
	ctx.moveTo(x + radius, y);
	ctx.lineTo(x + width - radius, y);
	ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
	ctx.lineTo(x + width, y + height - radius);
	ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
	ctx.lineTo(x + radius, y + height);
	ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
	ctx.lineTo(x, y + radius);
	ctx.quadraticCurveTo(x, y, x + radius, y);
	ctx.closePath();
	if (stroke)
		ctx.stroke();
	if (fill)
		ctx.fill();
}
    
class CanvasRangedSliderBehavior extends Behavior {
	onCreate(canvas, data) {
		this.data = data;
		if (! ("min" in data))			data.min = 0;
		if (! ("max" in data))			data.max = 0;
		if (! ("lowerValue" in data))	data.lowerValue = 0;
		if (! ("upperValue" in data))	data.upperValue = 1;
	}
	getMax(canvas) {
		return this.data.max;
	}
	getMin(canvas) {
		return this.data.min;
	}
	onLayoutChanged(canvas) {
	}
	onDisplaying(canvas) {
		this.lowerThumbTouchBeganXOffset = 0;
		this.upperThumbTouchBeganXOffset = 0;
		this.knobHeight = canvas.height;
		this.knobWidth = this.knobHeight;
		this.sliderBarHeight = this.knobHeight * (16 / 38);
		this.halfKnobWidth = this.knobWidth / 2;
		this.insetWidth = this.knobWidth + 8;
		this.halfInsetWidth = this.insetWidth / 2;
		this.trackingWidth = canvas.width - this.insetWidth;
		this.tracking = "none";
		this.onLayoutChanged(canvas);
		this.onValueChanged(canvas);	
	}
	onTouchBegan(canvas, id, x, y, ticks) {
		canvas.captureTouch(id, x, y, ticks);
		let hitLowerThumb = this.hitTestLowerThumb(canvas, x, y);
		if (hitLowerThumb) {
			this.lowerThumbTouchBeganXOffset = this.getLowerThumbTouchBeganXOffset(canvas, x);
			this.tracking = "lower";
			this.onTouchMoved(canvas, id, x, y, ticks);
			return;
		}
		let hitUpperThumb = this.hitTestUpperThumb(canvas, x, y);
		if (hitUpperThumb) {
			this.upperThumbTouchBeganXOffset = this.getUpperThumbTouchBeganXOffset(canvas, x);
			this.tracking = "upper";
			this.onTouchMoved(canvas, id, x, y, ticks);
			return;
		}
		this.tracking = "none";
	}
	onTouchEnded(canvas, id, x, y, ticks) {
	}
	onTouchMoved(canvas, id, x, y, ticks) {
		if (this.tracking == "lower") {
			x -= this.lowerThumbTouchBeganXOffset;
			let size = this.trackingWidth;
			let offset = (x - canvas.x - this.halfInsetWidth);
			let upperOffset = this.getUpperOffset(canvas, this.trackingWidth);
			let lowerOffset = this.getLowerOffset(canvas, this.trackingWidth);
			if (offset > upperOffset)
				offset = upperOffset;
			if (offset != lowerOffset) {
				this.setLowerOffset(canvas, size, offset);
				this.onValueChanged(canvas);
			}
		}
		else if (this.tracking == "upper") {
			x -= this.upperThumbTouchBeganXOffset;
			let size = this.trackingWidth;
			let offset = (x - canvas.x - this.halfInsetWidth);
			let upperOffset = this.getUpperOffset(canvas, this.trackingWidth);
			let lowerOffset = this.getLowerOffset(canvas, this.trackingWidth);
			if (offset < lowerOffset)
				offset = lowerOffset;
			if (offset != upperOffset) {
				this.setUpperOffset(canvas, size, offset);
				this.onValueChanged(canvas);
			}
		}
	}
	setLowerOffset(canvas, size, offset) {
		var min = this.getMin(canvas);
		var max = this.getMax(canvas);
		var value = min + ((offset * (max - min)) / size);
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setLowerValue(canvas, value);		
	}
	setUpperOffset(canvas, size, offset) {
		var min = this.getMin(canvas);
		var max = this.getMax(canvas);
		var value = min + ((offset * (max - min)) / size);
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setUpperValue(canvas, value);			
	}
	setLowerValue(canvas, value) {
		this.data.lowerValue = value;	
	}
	setUpperValue(canvas, value) {
		this.data.upperValue = value;	
	}
	getLowerValue(canvas) {
		return this.data.lowerValue;	
	}
	getUpperValue(canvas) {
		return this.data.upperValue;	
	}
	getLowerOffset(canvas, size) {
		let min = this.getMin(canvas);
		let max = this.getMax(canvas);
		let value = this.getLowerValue(canvas);
		return Math.round(((value - min) * size) / (max - min));
	}
	getUpperOffset(canvas, size) {
		let min = this.getMin(canvas);
		let max = this.getMax(canvas);
		let value = this.getUpperValue(canvas);
		return Math.round(((value - min) * size) / (max - min));
	}
	hitTestLowerThumb(canvas, x, y) {
		x -= canvas.container.x;
		y -= canvas.container.y;
		let b = this.getLowerThumbBounds(canvas);
		return (x >= b.x - b.width/2 && x <= (b.x + b.width/2) && y >= b.y && y <= (b.y + b.height));
	}
	hitTestUpperThumb(canvas, x, y) {
		x -= canvas.container.x;
		y -= canvas.container.y;
		let b = this.getUpperThumbBounds(canvas);
		return (x >= (b.x + b.width/2 + 1) && x <= (b.x + 1.5 * b.width) && y >= b.y && y <= (b.y + b.height));	
	}
	getLowerThumbBounds(canvas) {
		let offset = this.getLowerOffset(canvas, this.trackingWidth);
		return {
			x : offset + 4, 
			y : 3, 
			width : this.knobWidth, 
			height : this.knobHeight - 6 
		}
	}
	getUpperThumbBounds(canvas) {
		let offset = this.getUpperOffset(canvas, this.trackingWidth);
		return {
			x : offset + 4, 
			y : 3, 
			width : this.knobWidth, 
			height : this.knobHeight - 6 
		}
	}
	getLowerThumbTouchBeganXOffset(canvas, x) {
		x -= canvas.container.x;
		let b = this.getLowerThumbBounds(canvas);
		let centerX = b.x + (b.width / 2);
		let deltaX = x - centerX;
		return deltaX;
	}
	getUpperThumbTouchBeganXOffset(canvas, x) {
		x -= canvas.container.x;
		let b = this.getUpperThumbBounds(canvas);
		let centerX = b.x + (b.width / 2);
		let deltaX = x - centerX;
		return deltaX;
	}
	onValueChanged(canvas) {
		let active = canvas.active;
		let trackingWidth = this.trackingWidth;
		let lowerOffset = this.getLowerOffset(canvas, trackingWidth);
		let upperOffset = this.getUpperOffset(canvas, trackingWidth);
	   
		let ctx = canvas.getContext("2d");
		ctx.clearRect(0, 0, canvas.width, canvas.height);
		ctx.strokeStyle = "black";
		let sliderBarHeight = this.sliderBarHeight;
		let sliderBarTop = (canvas.height / 2) - (sliderBarHeight / 2);
		ctx.fillStyle =  "#e6e6e6";
		ctx.fillRect(this.halfInsetWidth, sliderBarTop, canvas.width - this.insetWidth, sliderBarHeight);
		ctx.fillStyle =  "#8acc50";
					
		ctx.fillRect(this.halfInsetWidth + lowerOffset, sliderBarTop, (upperOffset - lowerOffset), sliderBarHeight);
		ctx.lineWidth = 1;
		ctx.strokeRect(this.halfInsetWidth, sliderBarTop, canvas.width - this.insetWidth, sliderBarHeight);
   
		ctx.fillStyle = "white";										// lower thumb  
		ctx.lineWidth = 2;	
		let b = this.getLowerThumbBounds(canvas);
		roundRect(ctx, b.x, b.y, b.width/2 - 1, b.height, 4, true, true);

		ctx.fillStyle = "white";										// upper thumb  
		ctx.lineWidth = 2;	
		b = this.getUpperThumbBounds(canvas);
		roundRect(ctx, b.x + b.width/2 + 1, b.y, b.width/2 - 1, b.height, 4, true, true);
	}
}

class RangeSliderBehavior extends CanvasRangedSliderBehavior {
	onCreate(canvas, data) {
		this.data = data;
/*		data.min = 0;
		data.max = 1;
		data.value = 0;
		
*/
		
		super.onCreate(canvas, data);
	}
	setLowerValue(canvas, value) {
		this.data.minRange = value;
	}
	setUpperValue(canvas, value) {
		this.data.maxRange = value;
	}
	getLowerValue(canvas) {
		return this.data.minRange;
	}
	getUpperValue(canvas) {
		return this.data.maxRange;
	}
	onValueChanged(canvas) {
		super.onValueChanged(canvas);
		let lowerValue = this.getLowerValue();
		let upperValue = this.getUpperValue();
		this.data.WAVEFORM_CONTROL.delegate("onRangeChanged", lowerValue, upperValue);	
	}
}

var RangeSlider = Container.template($ => ({
	left:0, bottom:0, width:135, height:40,
	contents: [
		Canvas($, { left:0, top:0, right:0, bottom:0, active:true, Behavior:RangeSliderBehavior })
	],
}));

class SliderBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	changeState(container, state) {
		container.last.state = state;
	}
	getMax(container) {
		return this.data.max;
	}
	getMin(container) {
		return this.data.min;
	}
	getOffset(container, size) {
		let min = this.getMin(container);
		let max = this.getMax(container);
		let value = this.getValue(container);
		return Math.round(((value - min) * size) / (max - min));
	}
	getValue(container) {
		return this.data.value;
	}
	getFractionalValue(container) {
		return this.getValue(container) / (this.getMax(container) - this.getMin(container));
	}
	onAdapt(container) {
		this.onLayoutChanged(container);
	}
	onDisplaying(container) {
		this.onLayoutChanged(container);
		this.onValueChanged(container);
	}
	onLayoutChanged(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.changeState(container, 1);
		this.onTouchMoved(container, id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		this.changeState(container, 0);
	}
	onTouchMoved(container, id, x, y, ticks) {
		debugger
	}
	onValueChanged(container) {
	}
	setOffset(container, size, offset) {
		let min = this.getMin(container);
		let max = this.getMax(container);
		let value = min + ((offset * (max - min)) / size);
		if (value < min) value = min;
		else if (value > max) value = max;
		this.setValue(container, value);
	}
	setValue(container, value) {
		this.data.value = value;
	}
}

export class CanvasSliderBehavior extends SliderBehavior {
	onDisplaying(canvas) {
		this.thumbTouchBeganXOffset = 0;
		this.knobHeight = canvas.height;
		this.knobWidth = this.knobHeight * (22 / 38);
		this.sliderBarHeight = this.knobHeight * (16 / 38);
		this.halfKnobWidth = this.knobWidth / 2;
		this.insetWidth = this.knobWidth + 8;
		this.halfInsetWidth = this.insetWidth / 2;
		this.trackingWidth = canvas.width - this.insetWidth;    
		super.onDisplaying(canvas);
	}
	onTouchBegan(canvas, id, x, y, ticks) {
		canvas.captureTouch(id, x, y, ticks);
		var hitThumb = this.hitTestThumb(canvas, x, y);
		if (hitThumb)
			this.thumbTouchBeganXOffset = this.getThumbTouchBeganXOffset(canvas, x);
		else {
			this.thumbTouchBeganXOffset = 0;
			this.onTouchMoved(canvas, id, x, y, ticks);
		}
	}
	onTouchEnded(canvas, id, x, y, ticks) {
	}
	onTouchMoved(canvas, id, x, y, ticks) {
		x -= this.thumbTouchBeganXOffset;
		let size = this.trackingWidth;
		let offset = (x - canvas.x - this.halfInsetWidth);
		this.setOffset(canvas, size, offset);
		this.onValueChanged(canvas);
	}
	getOffset(canvas, size) {
		let min = this.getMin(canvas);
		let max = this.getMax(canvas);
		let value = this.getValue(canvas);
		return Math.round(((value - min) * size) / (max - min));
	}
	hitTestThumb(canvas, x, y) {
		x -= canvas.container.x;
		y -= canvas.container.y;
		let b = this.getThumbBounds(canvas);
		return (x >= b.x && x <= (b.x + b.width) && y >= b.y && y <= (b.y + b.height));	
	}
	getThumbBounds(canvas) {
		let offset = this.getOffset(canvas, this.trackingWidth);
		return {
			x : offset + 4, 
			y : 3, 
			width : this.knobWidth, 
			height : this.knobHeight - 6 
		}
	}
	getThumbTouchBeganXOffset(canvas, x) {
		x -= canvas.container.x;
		let b = this.getThumbBounds(canvas);
		let centerX = b.x + (b.width / 2);
		let deltaX = x - centerX;
		return deltaX;	
	}
	onValueChanged(canvas) {
		let active = canvas.active;
		let trackingWidth = this.trackingWidth;
		let offset = this.getOffset(canvas, trackingWidth);
		let ctx = canvas.getContext("2d");
		ctx.clearRect(0, 0, canvas.width, canvas.height);
		ctx.strokeStyle = "black";
		let sliderBarHeight = this.sliderBarHeight;
		let sliderBarTop = (canvas.height / 2) - (sliderBarHeight / 2);
		ctx.fillStyle =  "#e6e6e6";
		ctx.fillRect(this.halfInsetWidth, sliderBarTop, canvas.width - this.insetWidth, sliderBarHeight);
		ctx.fillStyle =  "#8acc50";
		ctx.fillRect(this.halfInsetWidth, sliderBarTop, offset, sliderBarHeight);
		ctx.lineWidth = 1;
		ctx.strokeRect(this.halfInsetWidth, sliderBarTop, canvas.width - this.insetWidth, sliderBarHeight);
   
		ctx.fillStyle = "white";										// thumb  
		ctx.lineWidth = 2;	
		let b = this.getThumbBounds(canvas);
		roundRect(ctx, b.x, b.y, b.width, b.height, 4, true, true);
	}
}

class HertzSliderBehavior extends CanvasSliderBehavior {
	onCreate(canvas, data) {
		this.data = data;
		data.min = 0;
		data.max = 1;
		data.value = data.parentData.hertzValue;
		super.onCreate(canvas, data);
	}
	onValueChanged(canvas) {
		super.onValueChanged(canvas);
		let value = this.data.value;
		this.data.parentData.hertzValue = value;
		this.data.parentData.WAVEFORM_CONTROL.delegate("onHertzFractionChanged", value);	
	}
}

var HertzSlider = Container.template($ => ({
	width:135, height:40,
	contents: [
		Canvas($, { left:0, top:0, right:0, bottom:0, active:true, Behavior:HertzSliderBehavior })
	],
}));
    
const sliderLabelStyle = new Style({ font:SEMIBOLD_FONT, size:20, color:[BLACK, WHITE], horizontal:"center,middle" });

const RangeSliderLabel = Label.template($ => ({
	width:135, height:30, style:sliderLabelStyle, string:"Range"
}));

const HertzSliderLabel = Label.template($ => ({
	width:135, height:30, style:sliderLabelStyle, string:"Hertz"
}));


		
		
		

