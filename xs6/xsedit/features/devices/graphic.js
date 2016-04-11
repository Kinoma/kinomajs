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
	SEMIBOLD_FONT,
} from "shell/assets";

import {
	blackSkin,
} from "assets";

var levelPopupPointerSkin = new Skin({ texture:new Texture("./assets/value-inspector-pointer.png", 1), x:0, y:0, width:17, height:9 });
var levelPopupStyle = new Style({font:SEMIBOLD_FONT, size:14, color:BLACK, horizontal:"center", vertical:"middle"});
var levelPopupSkin = new Skin({ texture:new Texture("./assets/level-popup.png", 1), x:0, y:0, width:30, height:30, tiles: { left:10, right:10, top:10, bottom:10 } });

export var SampleGraphProbeContainer = Container.template($ => ({
	name:"levelProbe", left:0, right:0, top:0, bottom:0, visible:false,
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.data = data;
			this.min = ("min" in data) ? data.min : 0;
			this.max = ("max" in data) ? data.max : 1;
			this.isRanged = (this.min != 0) || (this.max != 0);
			this.probeOffset = ("probeOffset" in data) ? data.probeOffset : 0;
			this.lastValue = -1;
			this.enableAudioFeedback = false;
			if (this.enableAudioFeedback) {
				this.openSound = new Sound( mergeURI( application.url, "assets/sounds/probe-open.aac" ) );
				this.closeSound = new Sound( mergeURI( application.url, "assets/sounds/probe-close.aac" ) );
				this.measureSound = new Sound( mergeURI( application.url, "assets/sounds/probe-measure.aac" ) );
			}
		}
		onDisplaying(container) {
			var graph = container.container.graph;
			this.toLevelPopupY = graph.y - container.levelPopup.height - container.levelPopupPointer.height - this.probeOffset;
			container.levelPopupLine.height = this.levelPopupLineHeight = graph.height + 2 + this.probeOffset;
			if (this.enableAudioFeedback)
				Sound.volume = 0.3;
			this.duration = 200;
			this.fraction = 0;
			this.animationState = "none";		// "none", "show", or "hide"
		}
		onScrolled(container) {
			var graph = container.container.graph;
			this.toLevelPopupY = graph.y - container.levelPopup.height - container.levelPopupPointer.height - this.probeOffset;
		}
		onUndisplayed(container) {
			if (this.enableAudioFeedback)
				Sound.volume = 1.0;
		}
		setFingerPosition(container, x, y) {
			this.fingerX = x;
			this.fingerY = y;
		
			var parent = container.container;
			var levelPopup = container.levelPopup;
			this.updateLevelPopupBubble(container, parent, levelPopup, x);
			
			var levelPopupPointer = container.levelPopupPointer;
			this.updateLevelPopupPointer(container, parent, levelPopup, levelPopupPointer, x);
							
			var levelPopupLine = container.levelPopupLine;
			this.updateLevelPopupLine(container, parent, levelPopup, levelPopupPointer, levelPopupLine);
		}		
		updateLevelPopupBubble(container, parent, levelPopup, x) {
			var popupX = x - (levelPopup.width / 2);
			var pad = 4;
			var minX = parent.x + pad;
			var maxX = parent.x + (parent.width - levelPopup.width) - pad;
			if (popupX < minX)	
				popupX = minX;
			else if (popupX > maxX)	
				popupX = maxX;
			levelPopup.x = popupX;
			var popupY;
			switch (this.animationState) {
				case "none":
					popupY = this.toLevelPopupY;
					break;
				case "show":
					popupY = this.fromLevelPopupY + this.fraction * (this.toLevelPopupY - this.fromLevelPopupY);
					break;
				case "hide":
					popupY = this.fromLevelPopupY + (1 - this.fraction) * (this.toLevelPopupY - this.fromLevelPopupY);
					break; 
			}
			
			levelPopup.y = popupY;				
		}
		updateLevelPopupPointer(container, parent, levelPopup, levelPopupPointer, x) {
			let pad = 6;
			pad = pad + (levelPopupPointer.width / 2) + 2;
			let minX = parent.x + pad;
			let maxX = parent.x + (parent.width - levelPopupPointer.width);
			if (x < minX)
				x = minX;
			else if (x > maxX)
				x = maxX;
			levelPopupPointer.x = x - (levelPopupPointer.width / 2);
			levelPopupPointer.y = levelPopup.y + levelPopup.height - 2;
		}
		updateLevelPopupLine(container, parent, levelPopup, levelPopupPointer, levelPopupLine) {
			levelPopupLine.x = levelPopupPointer.x + (levelPopupPointer.width / 2);
			levelPopupLine.y = levelPopupPointer.y + levelPopupPointer.height - 1;
			
			switch (this.animationState) {
				case "none":
					levelPopupLine.height = this.levelPopupLineHeight;
					break;
				case "show":
					levelPopupLine.height = this.fraction * (this.targetEndLineY - levelPopupLine.y);
					break;
				case "hide":
					break; 
			}

		}
		setValue(container, value) {
			if (value != this.lastValue) {
				this.lastValue = value;
				var displayValue = value;
				if (this.isRanged) {
					displayValue = this.min + (value * (this.max - this.min));
				}
				container.levelPopup.label.string = displayValue.toString();
				if (this.enableAudioFeedback)
					this.measureSound.play();
			}
		}
		show(container) {
			if (this.enableAudioFeedback)
				this.openSound.play();
			var graph = container.container.graph;
			this.toLevelPopupY = graph.y - container.levelPopup.height - container.levelPopupPointer.height - this.probeOffset;
			this.fromLevelPopupY = this.fingerY;
			var levelPopup = container.levelPopup;
			var levelPopupPointer = container.levelPopupPointer;
			this.targetEndLineY = (levelPopup.y + levelPopup.height - 2) + levelPopupPointer.height - 1 + this.levelPopupLineHeight;
			container.visible = true;
			container.time = 0;
			this.animationState = "show";
			container.start();		
		}
		hide(container) {
			if (this.enableAudioFeedback)
				this.closeSound.play();
			container.visible = false;
		}
		onTimeChanged(container) {
			this.fraction = container.time / this.duration;
			if (this.fraction >= 1)
				this.fraction = 1;
			this.fraction = Math.quadEaseOut(this.fraction);
			if (this.fraction == 1) {
				this.fraction = 1;
				container.stop();
				this.animationState = "none";
			}
			this.setFingerPosition(container, this.fingerX, this.fingerY);
		}
	},
	contents:[
		Container($, {
			name:"levelPopup", skin:levelPopupSkin, left:0, top:0, width:40, height:20,
			contents:[
				Label($, { name:"label", style:levelPopupStyle }),
			]
		}),
		Content($, { name:"levelPopupPointer", skin:levelPopupPointerSkin, left:0, top:0 }),
		Content($, { name:"levelPopupLine", skin:blackSkin, left:0, top:0, width:2, height:54 }),
	]
}));

export var SampleGraphContainer = Port.template($ => ({
	name:"graph", left:10, right:10, bottom:10, height:60, active:true,
	Behavior: class extends Behavior {
		getSamples() {
			return this.frozen ? this.frozenSamples : this.samples;
		}
		onCreate(port, data) {
			this.data = data;
			this.numSamples = ("numSamples" in data) ? data.numSamples : 14;
			this.barColor = ("barColor" in data) ? data.barColor : '#4293da';
			this.currentBarColor = ("currentBarColor" in data) ? data.currentBarColor : '#63a842';
			this.frozen = false;
			this.samples = new Array(this.numSamples);
			this.frozenSamples = new Array(this.numSamples);
			for (let i = 0, c = this.numSamples; i < c; i++) {
				this.samples[i] = this.frozenSamples[i] = 0;
			}
		}
		onUpdate(port, fraction) {
			this.samples.shift();
			this.samples.push(fraction);
			port.invalidate();
		}
		drawSample(port, x, sampleWidth, fraction) {
			var sampleHeight = fraction * port.height;
			if (sampleHeight <= 1)
				sampleHeight = 1;
			var y = port.height - sampleHeight;
			port.fillColor(this.fillColor, x, y, sampleWidth, sampleHeight);
		}
		hitTestSample(port, x) {	
			x = x - port.x;
			var numSamples = this.numSamples;
			var xStep = Math.floor(port.width / numSamples);
			var hitSample = Math.floor(x / xStep);
			if (hitSample < 0)	
				hitSample = 0;
			if (hitSample > (numSamples - 1))		
				hitSample = numSamples - 1;
			return hitSample;
		}
		onDraw(port) {
			var samples = this.getSamples();
			var numSamples = this.numSamples;
			var xStep = Math.floor(port.width / numSamples);
			var sampleWidth = xStep - 2;
			var x = 0;
			for (var i=0; i < numSamples; i++) {
				if (i == (numSamples - 1))
					this.fillColor = this.currentBarColor;
				else
					this.fillColor = this.barColor;
				this.drawSample(port, x, sampleWidth, samples[i]);
				x += xStep;
			}	
		}

		onTouchBegan(port, id, x, y, ticks) {
			this.frozen = true;
			this.frozenSamples = this.samples.slice(0);		// clone array
			var hitSample = this.hitTestSample(port, x);
			var value = this.frozenSamples[hitSample].toFixed(2);
			var levelProbe = port.container.levelProbe;
			levelProbe.delegate("setFingerPosition", x, y);
			levelProbe.delegate("setValue", value);
			levelProbe.delegate("show");
		}
		onTouchMoved(port, id, x, y, ticks) {
			port.container.levelProbe.delegate("setFingerPosition", x, y);
			var hitSample = this.hitTestSample(port, x);
			var samples = this.getSamples();
			var value = samples[hitSample].toFixed(2);
			port.container.levelProbe.delegate("setValue", value);
		}
		onTouchEnded(port, id, x, y, ticks) {
			port.container.levelProbe.delegate("hide");
			this.frozen = false;
		}	
		onTouchCancelled(port, id, x, y, ticks) {
			this.onTouchEnded(port, id, x, y, ticks);
		}	

	}
}));
