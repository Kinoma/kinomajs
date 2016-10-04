//@module
/*
  Copyright 2011-2014 Marvell Semiconductor, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

class AbstractTransition extends Transition {
	constructor(duration) {
		if (!duration) duration = 500;
		super(duration);
	}
	onBegin(container, formerContent, currentContent, formerData, currentData) {
		var options = (undefined != currentData) ? currentData : formerData;
		if (this.addCurrentContent) {
			if (false == this.addCurrentBehind)
			container.add( currentContent );
		else
			container.insert( currentContent, container.first );
		}
		this.formerLayer = new Layer({alpha: false});
		this.currentLayer = new Layer({alpha: false});
		this.formerLayer.attach( formerContent );
		this.currentLayer.attach( currentContent );   
		this.onBeginTransition( container, formerContent, currentContent, formerData, currentData );
	}
	onStep(fraction) {
		if ( this.easeType != "linear" ) fraction = Math[this.easeType]( fraction );
		this.onStepTransition( fraction );
	}
	onEnd(container, formerContent, currentContent) {
		this.currentLayer.detach();
        this.formerLayer.detach();
        if ( this.removeFormerContent )
        	container.remove( formerContent);  
        this.onEndTransition( container, formerContent, currentContent );
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {}
	onStepTransition(fraction) {}
	onEndTransition(container, formerContent, currentContent) {}
}


class CrossFade extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 1500;
		super(this.duration);
		this.easeType = { value: 'linear', writable: true };
	}
	onStepTransition(fraction) {
		this.currentLayer.opacity = fraction;
	}
}

class Push extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 500;
		super(this.duration);
		this.direction = data.direction || "left";
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		this.formerFromX = this.formerToX = this.currentFromX = this.currentToX = 0;
		this.formerFromY = this.formerToY = this.currentFromY = this.currentToY = 0;
		switch (this.direction) {
			case "left":
				this.formerFromX = 0;
				this.formerToX = - formerContent.width;
				this.currentLayer.translation = { x : currentContent.width, y : 0 };
				this.currentFromX = currentContent.width;
				this.currentToX = 0;
				break;
			case "right":
				this.formerFromX = 0;
				this.formerToX = formerContent.width;
				this.currentLayer.translation = { x : -currentContent.width, y : 0 };
				this.currentFromX = -currentContent.width;
				this.currentToX = 0;
				break
			case "up":
				this.formerFromY = 0;
				this.formerToY = -formerContent.height;
				this.currentLayer.translation = { x : 0, y : currentContent.height };
				this.currentFromY = currentContent.height;
				this.currentToY = 0;
				break
			case "down":
				this.formerFromY = 0;
				this.formerToY = formerContent.height;
				this.currentLayer.translation = { x : 0, y : -currentContent.height };
				this.currentFromY = -currentContent.height;
				this.currentToY = 0;
				break
		}
	}
	onStepTransition(fraction) {
		var formerX = lerp( this.formerFromX, this.formerToX, fraction );
		var formerY = lerp( this.formerFromY, this.formerToY, fraction );
		this.formerLayer.translation = { x : formerX, y : formerY }
		var currentX = lerp( this.currentFromX, this.currentToX, fraction );
		var currentY = lerp( this.currentFromY, this.currentToY, fraction );
		this.currentLayer.translation = { x : currentX, y : currentY }
	}
}


class Flip extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 500;
		super(this.duration);
		this.direction = data.direction || "left";	
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		var formerLayer = this.formerLayer;
		formerLayer.origin = { y: formerLayer.height / 2 };
		this.formerStarts = [
			{ x: 0, y: 0 },
			{ x: 1, y: 0 },
			{ x: 1, y: 1 },
			{ x: 0, y: 1 },
		];
		this.formerSteps = [
			{ x: 0, y: 0 },
			{ x: 1, y: 0 },
			{ x: 1, y: 1 },
			{ x: 0, y: 1 },
		];
		var currentLayer = this.currentLayer;
		currentLayer.origin = { y: currentLayer.height / 2 };
		this.currentStops = [
			{ x: 0, y: 0 },
			{ x: 1, y: 0 },
			{ x: 1, y: 1 },
			{ x: 0, y: 1 },
		];
		this.currentSteps = [
			{ x: 0, y: 0 },
			{ x: 1, y: 0 },
			{ x: 1, y: 1 },
			{ x: 0, y: 1 },
		];
		switch ( this.direction ) {
			case "left":
				this.formerStops = [
					{ x: 0.49, y: 0.1 },
					{ x: 0.51, y: -0.1 },
					{ x: 0.51, y: 1.1 },
					{ x: 0.49, y: 0.9 },
				];
				this.currentStarts = [
					{ x: 0.49, y: -0.1 },
					{ x: 0.51, y: 0.1 },
					{ x: 0.51, y: 0.9 },
					{ x: 0.49, y: 1.1 },
				];
				break;
			case "right":		
				this.formerStops = [
					{ x: 0.49, y: -0.1 },
					{ x: 0.51, y: 0.1 },
					{ x: 0.51, y: 0.9 },
					{ x: 0.49, y: 1.1 },
				];
				this.currentStarts = [
					{ x: 0.49, y: 0.1 },
					{ x: 0.51, y: -0.1 },
					{ x: 0.51, y: 1.1 },
					{ x: 0.49, y: 0.9 },
				];
				break;
			case "up":		
				this.currentStarts = [
					{ x: -0.2, y: 0.49 },
					{ x: 1.2, y: 0.49 },
					{ x: 0.8, y: 0.51 },
					{ x: 0.2, y: 0.51 },
				];
				this.formerStops = [
					{ x: 0.2, y: 0.49 },
					{ x: 0.8, y: 0.49 },
					{ x: 1.2, y: 0.51 },
					{ x: -0.2, y: 0.51 },
				];
				break;
			case "down":		
				this.formerStops = [
					{ x: -0.2, y: 0.49 },
					{ x: 1.2, y: 0.49 },
					{ x: 0.8, y: 0.51 },
					{ x: 0.2, y: 0.51 },
				];			
				this.currentStarts = [
					{ x: 0.2, y: 0.49 },
					{ x: 0.8, y: 0.49 },
					{ x: 1.2, y: 0.51 },
					{ x: -0.2, y: 0.51 },
				];
				break;
		}
	}
	onStepTransition(fraction) {
		if (fraction <= 0.5) {
			fraction *= 2;
			var layer = this.formerLayer;
			var starts = this.formerStarts;
			var stops = this.formerStops;
			var steps = this.formerSteps;
			for (var i = 0; i < 4; i++) {
				var start = starts[i];
				var stop = stops[i];
				var step = steps[i];
				step.x = (start.x * (1 - fraction)) + (stop.x * fraction);
				step.y = (start.y * (1 - fraction)) + (stop.y * fraction);
			}
			layer.opacity = 1;
			layer.corners = steps;
			this.currentLayer.opacity = 0;
		}
		else {
			fraction = 2 * (fraction - 0.5);
			var layer = this.currentLayer;
			var starts = this.currentStarts;
			var stops = this.currentStops;
			var steps = this.currentSteps;
			for (var i = 0; i < 4; i++) {
				var start = starts[i];
				var stop = stops[i];
				var step = steps[i];
				step.x = (start.x * (1 - fraction)) + (stop.x * fraction);
				step.y = (start.y * (1 - fraction)) + (stop.y * fraction);
			}
			layer.opacity = 1;
			layer.corners = steps;
			this.formerLayer.opacity = 0;
		}
	}
}

class Reveal extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 350;
		super(this.duration);
		this.direction = data.direction || "up";		
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		this.currentFromX = this.currentToX = 0;
		this.currentFromY = this.currentToY = 0;
		
		this.effect = new Effect();
		
		switch (this.direction) {
			case "left":
				this.currentLayer.translation = { x : currentContent.width, y : 0 };
				this.currentFromX = currentContent.width;
				break;
			case "right":
				this.currentLayer.translation = { x : -currentContent.width, y : 0 };
				this.currentFromX = -currentContent.width;
				break;
			case "up":
				this.currentLayer.translation = { x : 0, y : currentContent.height };
				this.currentFromY = currentContent.height;
				break;
			case "down":
				this.currentLayer.translation = { x : 0, y : -currentContent.height };
				this.currentFromY = -currentContent.height;
				break;
		}
	}
	onStepTransition(fraction) {
		var currentX = lerp( this.currentFromX, this.currentToX, fraction );
		var currentY = lerp( this.currentFromY, this.currentToY, fraction );
		this.currentLayer.translation = { x : currentX, y : currentY };
		
		var effect = new Effect();
		effect.colorize("black", fraction / 2);
		this.formerLayer.effect = effect;		
	}
}



class Hide extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 350;
		super(this.duration);
		this.direction = data.direction || "down";
		this.easeType = data.easeType || "quadEaseIn";
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		this.formerFromX = this.formerToX = 0;
		this.formerFromY = this.formerToY = 0;
		
		switch (this.direction) {
			case "left":
				this.formerToX = -formerContent.width;
				break;
			case "right":
				this.formerToX = formerContent.width;
				break;
			case "up":
				this.formerToY = -formerContent.height;
				break;
			case "down":
				this.formerToY = formerContent.height;
				break;
		}
	}
	onStepTransition(fraction) {
		var formerX = lerp( this.formerFromX, this.formerToX, fraction );
		var formerY = lerp( this.formerFromY, this.formerToY, fraction );
		this.formerLayer.translation = { x : formerX, y : formerY }

		var effect = new Effect();
		effect.colorize("black", 0.5 - (fraction / 2));
		this.currentLayer.effect = effect;
	}
}

class TimeTravel extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 500;
		super(this.duration);
		this.direction = data.direction || "forward";
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		if (this.hasOwnProperty("xOrigin") && this.hasOwnProperty("yOrigin")) {
			this.formerLayer.origin = { x : this.xOrigin, y : this.yOrigin };
			this.currentLayer.origin = { x : this.xOrigin, y : this.yOrigin };
		}
		else {
			this.formerLayer.origin = { x : this.formerLayer.width / 2, y : this.formerLayer.height / 2 };
			this.currentLayer.origin = { x : this.currentLayer.width / 2, y : this.currentLayer.height / 2 };
		}
		switch (this.direction) {
			case "forward":
				this.fromFormerScale = 1.0;
				this.toFormerScale = 2.0;
				this.fromCurrentScale = 0.5;
				this.toCurrentScale = 1.0;
				break;
			case "back":
				this.fromFormerScale = 1.0;
				this.toFormerScale = 0.5;
				this.fromCurrentScale = 2.0;
				this.toCurrentScale = 1.0;
				break;
		}
		this.onStepTransition( 0 );
	}
	onStepTransition(fraction) {
		var formerScale = lerp( this.fromFormerScale, this.toFormerScale, fraction );
		this.formerLayer.scale = { x : formerScale, y : formerScale };
		this.formerLayer.opacity = 1 - fraction;
		var currentScale = lerp( this.fromCurrentScale, this.toCurrentScale, fraction );
		this.currentLayer.scale = { x : currentScale, y : currentScale };
		this.currentLayer.opacity = fraction;
	}
}


class ZoomAndSlide extends AbstractTransition {
	constructor(data) {
		this.duration = data.duration || 500;
		super(this.duration);
		this.direction = data.direction || "forward";
	}
	onBeginTransition(container, formerContent, currentContent, formerData, currentData) {
		this.formerLayer.origin = { x : this.formerLayer.width / 2, y : this.formerLayer.height / 2 };
		this.currentLayer.origin = { x : this.currentLayer.width / 2, y : this.currentLayer.height / 2 };
		this.currentaFromX = currentContent.width;
		this.formerFromY = this.formerToY = this.currentFromY = this.currentToY = 0;
		switch (this.direction) {
			case "forward":
				this.fromFormerScale = 1.0;
				this.toFormerScale = 0.8;
				this.currentLayer.translation = { x : currentContent.width, y : 0 };
				this.currentFromX = currentContent.width;
				this.currentToX = 0;
				break;
			case "back":
				this.fromCurrentScale = 0.8;
				this.toCurrentScale = 1.0;
				this.formerLayer.translation = { x : 0, y : 0 };
				this.formerFromX = 0;
				this.formerToX = currentContent.width;
				break;
		}
	}
	onStepTransition(fraction) {
		switch (this.direction) {
			case "forward":
				var formerScale = lerp( this.fromFormerScale, this.toFormerScale, fraction);
				this.formerLayer.scale = { x : formerScale, y : formerScale };
				var currentX = lerp( this.currentFromX, this.currentToX, fraction );
				this.currentLayer.translation = { x : currentX, y : 0 };
				break;
			case "back":
				var currentScale = lerp( this.fromCurrentScale, this.toCurrentScale, fraction);
				this.currentLayer.scale = { x : currentScale, y : currentScale };
				var formerX = lerp( this.formerFromX, this.formerToX, fraction );
				this.formerLayer.translation = { x : formerX, y : 0 };
				break;
		}
	}
}

var lerp = function(from, to, fraction) {
	return from + fraction * (to - from);
}