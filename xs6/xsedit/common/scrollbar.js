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
		var scrollbar = scroller.first.next;
		while (scrollbar) {
			scrollbar.first.visible = true;
			scrollbar = scrollbar.next;
		}
	}
	onMouseExited(scroller, x, y) {
		var scrollbar = scroller.first.next;
		while (scrollbar) {
			scrollbar.first.visible = false;
			scrollbar = scrollbar.next;
		}
	}
	onTouchScrolled(scroller, touched, dx, dy) {
		scroller.scrollBy(-dx, -dy);
	}
};

var scrollbarSkin = new Skin({ 
	fill:"transparent"
});
var scrollbarThumbSkin = new Skin({
	fill: [ "#e0e0e0", "#c0c0c0", "#909090" ],
});

class HorizontalScrollbarBehavior extends Behavior {
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		if (thumb.state == 2)
			return
		var size = scroller.width;
		var range = scroller.first.width;
		if (size < range) {
			var width = scrollbar.width;
			thumb.x = scrollbar.x + Math.round(scroller.scroll.x * width / range);
			thumb.width = Math.round(width * size / range);
		}
		else
			thumb.width = 0;
	}
	onMouseEntered(scrollbar, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		var thumb = scrollbar.first;
		thumb.state = 1;
	}
	onMouseExited(scrollbar, x, y) {
		var thumb = scrollbar.first;
		thumb.state = 0;
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
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
		thumb.state = 1;
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
	left:0, right:0, height:10, bottom:0, skin: scrollbarSkin, active:true, Behavior:HorizontalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, top:0, height:5, visible:false, skin: scrollbarThumbSkin }),
	],
}));

class HorizontalCenterScrollbarBehavior extends HorizontalScrollbarBehavior {
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		if (thumb.state == 2)
			return
		var size = scroller.width;
		var range = scroller.first.width;
		if (size < range) {
			var width = scrollbar.width;
			thumb.width = Math.round(width * size / range);
			thumb.x = scrollbar.x + ((width - thumb.width)  >> 1) + Math.round(scroller.scroll.x * width / range);
		}
		else
			thumb.width = 0;
	}
};

export var HorizontalCenterScrollbar = Container.template($ => ({
	left:0, right:0, height:10, bottom:0, skin: scrollbarSkin, active:true, Behavior:HorizontalCenterScrollbarBehavior,
	contents: [
		Content($, { left:0, width:0, top:0, height:5, visible:false, skin: scrollbarThumbSkin }),
	],
}));


class VerticalScrollbarBehavior extends Behavior {
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		if (thumb.state == 2)
			return
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
		}
		else
			thumb.height = 0;
	}
	onMouseEntered(scrollbar, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		var thumb = scrollbar.first;
		thumb.state = 1;
	}
	onMouseExited(scrollbar, x, y) {
		var thumb = scrollbar.first;
		thumb.state = 0;
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		thumb.state = 2;
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
		thumb.state = 1;
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
	width:10, right:0, top:0, bottom:0, skin: scrollbarSkin, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { left:0, width:5, top:0, height:0, visible:false, skin: scrollbarThumbSkin }),
	],
}));
