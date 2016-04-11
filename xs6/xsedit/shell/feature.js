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
// MODEL

export class Feature {
	constructor(model, title) {
		this.dirty = false;
		this.model = model;
		this.IconBehavior = FeatureIconBehavior;
		this.iconSkin = null;
		this.Template = null;
		this.title = title;
		this.viewers = [];
	}
	close() {
	}
	idle(now) {
	}
	open() {
	}
	read(json) {
	}
	write(json) {
	}
}

// ASSETS

import {
	GREEN,
	ORANGE,
} from "assets";

var featuresColumnSkin = new Skin({ fill:GREEN });
var featureTexture = new Texture("assets/feature.png", 2);
var featureArrowSkin = new Skin({ texture:featureTexture, x:0, y:0, width:10, height:20, variants:10 });
var featureBadgeSkin = new Skin({ texture:featureTexture, x:20, y:0, width:20, height:20, });
var featureBadgeStyle = new Style({ font:"Open Sans Bold", size:12, color:ORANGE });

// BEHAVIORS

class FeaturesColumnBehavior extends Behavior {
	onCreate(column, data) {
		this.data = data;
	}
	onDisplaying(column) {
		column.distribute("onFeatureSelected", this.data.currentFeature);
	}
};

export class FeatureIconBehavior extends Behavior {
	getBadge(container) {
		return this.badge;
	}
	setBadge(container, value) {
		let content = container.last;
		if (value <= 0) {
			content.visible = false;
			content.first.string = "";
		}
		else {
			content.visible = true;
			content.first.string = (value < 10) ? value : "•";
		}
		this.badge = value;
	}
	getBounce(container) {
		return container.running;
	}
	setBounce(container, flag) {
		if (flag)
			container.start();
		else
			container.stop();
	}
	onCreate(container, data) {
		this.data = data;
		container.duration = 400;
	}
	onFeatureChanged(container, data) {
		if (this.data == data) {
			this.data.dirty &= container.active;
			this.setBounce(container, this.data.dirty);
		}
	}
	onFeatureSelected(container, data) {
		let flag = this.data == data;
		container.active = !flag;
		let content = container.first;
		content.state = content.next.variant = flag ? 1 : 0;
		this.onFeatureChanged(container, this.data);
	}
	onFinished(container) {
		container.time = 0;
		container.start();
	}
	onTap(container) {
		container.bubble("doSelectFeature", this.data);
	}
	onTimeChanged(container) {
		let content = container.first;
		content.y = container.y + Math.round(4 * Math.sin(2 * Math.PI * container.fraction));
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		container.first.state = 1;
	}
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.onTap(container);
			container.last.state = 1;
		}
		else
			container.first.state = 0;
	}
	onTouchMoved(container, id, x, y, ticks) {
		container.first.state = container.hit(x, y) ? 1 : 0;
	}
};

export class FeaturePaneBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
};

// TEMPLATES

export var FeaturesColumn = Column.template(function($) { return {
	skin:featuresColumnSkin, Behavior:FeaturesColumnBehavior,
	contents: $.features.map(feature => new FeatureIcon(feature)),
}});

var FeatureIcon = Container.template(function($) { return {
	height:60, Behavior: $.IconBehavior,
	contents: [
		Content($, { top:0, skin:$.iconSkin }),
		Content($, { left:50, top:20, skin:featureArrowSkin }),
		Container($, {
			width:20, right:5, top:5, height:20, skin:featureBadgeSkin, visible:false,
			contents: [
				Label($, { width:20, height:20, style:featureBadgeStyle }),
			],
		
		}),
	]
}});
