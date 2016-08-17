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
export class ScrollerBehavior extends Behavior {
	onMouseEntered(scroller, x, y) {
	return
		var scrollbar = scroller.first.next;
		while (scrollbar) {
			scrollbar.visible = true;
			scrollbar = scrollbar.next;
		}
	}
	onMouseExited(scroller, x, y) {
	return
		var scrollbar = scroller.first.next;
		while (scrollbar) {
			scrollbar.visible = false;
			scrollbar = scrollbar.next;
		}
	}
	onTouchScrolled(scroller, touched, dx, dy) {
		scroller.scrollBy(-dx, -dy);
	}
};

var WHITE = "#ffffff";
var PASTEL_GRAY = "#f7f7f7";
var HOVER_GRAY = "#e0e0e0";
var CLICK_GRAY = "#c7c7c7";
var BORDER_GRAY = "#aaaaaa";
var TRANSPARENT = "transparent";

var horizontalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, PASTEL_GRAY, PASTEL_GRAY ],
	stroke: [ TRANSPARENT, HOVER_GRAY, HOVER_GRAY ],
	borders: { top:1 },
});
var verticalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, PASTEL_GRAY, PASTEL_GRAY ],
	stroke: [ TRANSPARENT, HOVER_GRAY, HOVER_GRAY ],
	borders: { left:1 },
});
var scrollbarThumbSkin = new Skin({
	fill: [ "#00e0e0e0", "#FFe0e0e0", HOVER_GRAY, CLICK_GRAY ],
});

class ScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		scrollbar.duration = 2000;
		this.former = 0;
	}
	onFinished(scrollbar) {
		scrollbar.first.state = 0;
	}
	onMouseEntered(scrollbar, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		var thumb = scrollbar.first;
		scrollbar.state = 1;
		thumb.state = 2;
		scrollbar.stop();
	}
	onMouseExited(scrollbar, x, y) {
		var thumb = scrollbar.first;
		scrollbar.state = 0;
		thumb.state = 1;
		scrollbar.time = 0;
		scrollbar.start();
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		let current = scroller.scroll[this.direction];
		if (this.former != current) {
			this.former = current;
			var thumb = scrollbar.first;
			if (thumb.state <= 1) {
				thumb.state = 1;
				scrollbar.time = 0;
				scrollbar.start();
			}
		}
	}
	onTimeChanged(scrollbar) {
		let fraction = scrollbar.fraction;
		let state = (fraction > 0.5) ? 1 - Math.quadEaseOut(2 * (fraction - 0.5)) : 1;
		scrollbar.first.state = state;
	}
};

class HorizontalScrollbarBehavior extends ScrollbarBehavior {
	onCreate(scrollbar) {
		super.onCreate(scrollbar);
		this.direction = "x";
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		super.onScrolled(scrollbar, scroller);
		var thumb = scrollbar.first;
		var size = scroller.width;
		var range = scroller.first.width;
		if (size < range) {
			var width = scrollbar.width;
			thumb.x = scrollbar.x + Math.round(scroller.scroll.x * width / range);
			thumb.width = Math.round(width * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.width = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.x;
		let thumbHeight = thumb.width;
		if ((x < thumbY) || ((thumbY + thumbHeight) <= x))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - x;
		this.min = scrollbar.x;
		this.max = scrollbar.x + scrollbar.width - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		x += this.anchor;
		if (x < this.min)
			x = this.min;
		else if (x > this.max)
			x = this.max;
		thumb.x = x;

		scroller.scrollTo((x - this.min) * (scroller.first.width / scrollbar.width), scroller.scroll.y);
	}
};

export var HorizontalScrollbar = Container.template($ => ({
	left:0, right:0, height:10, bottom:0, skin: horizontalScrollbarSkin, active:true, Behavior:HorizontalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, height:9, bottom:0, skin: scrollbarThumbSkin }),
	],
}));

class VerticalScrollbarBehavior extends ScrollbarBehavior {
	onCreate(scrollbar) {
		super.onCreate(scrollbar);
		this.direction = "y";
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		super.onScrolled(scrollbar, scroller);
		var thumb = scrollbar.first;
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.height = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 3;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.y;
		let thumbHeight = thumb.height;
		if ((y < thumbY) || ((thumbY + thumbHeight) <= y))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - y;
		this.min = scrollbar.y;
		this.max = scrollbar.y + scrollbar.height - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		y += this.anchor;
		if (y < this.min)
			y = this.min;
		else if (y > this.max)
			y = this.max;
		thumb.y = y;

		scroller.scrollTo(scroller.scroll.x, (y - this.min) * (scroller.first.height / scrollbar.height));
	}
};

export var VerticalScrollbar = Container.template($ => ({
	width:10, right:0, top:0, bottom:0, skin: verticalScrollbarSkin, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:0, width:9, top:0, height:0, skin: scrollbarThumbSkin }),
	],
}));
