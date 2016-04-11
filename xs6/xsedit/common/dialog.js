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
	progressBarSkin,
	progressCombSkin,
} from "features/samples/templates";

class ProgressBarBehavior extends Behavior {
	onValueChanged(container, value, size) {
		var bar = container.first;
		var comb = container.last;
		if (!value || (value == -1)) {
			bar.visible = false;
			comb.visible = true;
		}
		else {
			bar.visible = true;
			comb.visible = false;
			bar.width = 12 + Math.round((container.width - 12) * value / size);
		}
	}
	onDisplaying(container) {
		this.onValueChanged(container);
	}
};

class HorizontalTickerBehavior extends Behavior {
	onDisplaying(scroller) {
		scroller.interval = 25;
		if (scroller.width < scroller.first.width)
			scroller.start();
		else
			scroller.stop();
	}
	onTimeChanged(scroller) {
		scroller.scrollBy(1, 0);
	}
}

export var ProgressBar = Container.template($ => ({
	left:0, right:0, height:20, Behavior:ProgressBarBehavior, skin:progressBarSkin,
	contents: [
		Content($, { left:0, width:0, height:20, skin:progressBarSkin, state:1, }),
		Scroller($, {
			left:6, right:6, clip:true, loop:true, Behavior:HorizontalTickerBehavior,
			contents: [
				Content($, { left:0, width:2048, height:20, skin:progressCombSkin, }),
			],
		}),
	],
}));

