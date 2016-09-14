export class ButtonBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
		var content = container.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		this.onStateChanged(container);
	}
	onMouseEntered(container, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		this.changeState(container, 2);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, container.active ? 1 : 0);
	}
	onStateChanged(container) {
		this.changeState(container, container.active ? 1 : 0);
	}
	onTap(container) {
		let name = container.name;
		if (name)
			container.bubble(name, this.data);
		else
			debugger;
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.changeState(container, 3);
		container.captureTouch(id, x, y, ticks);
	}
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.changeState(container, 2);
			this.onTap(container);
		}
		else {
			this.changeState(container, 1);
		}
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 3 : 2);
	}
};

export class FieldLabelBehavior extends Behavior {
	filterKey(label, key) {
		if (!key) return null;
		var c = key.charCodeAt(0);
		if ((Event.FunctionKeyPlay <= c) && (c <= Event.FunctionKeyPower))
			return null;
		if (c > 0x000F0000)
			return null;
		if ((1 == c) || (4 == c) || (9 == c) || (11 == c) || (12 == c) || (25 == c) || (27 == c))
			return null;
		if ((label instanceof Label) && ((3 == c) || (13 == c)))
			return null;
		if (label.editable)
			return c;
		if (label.selectable && (((28 <= c) && (c <= 31))))
			return c;
		return null;
	}
	insertKey(label, key) {
		label.insert(key);
	}
	onCreate(label, data) {
		this.data = data;
		this.acceleration = 0.01;
		this.anchor = {
			from:0,
			to:0,
		};
		this.dx = 0;
		this.dy = 0;
		this.touchCount = 0;
		this.touchTicks = 0;
	}
	onEdited(label) {
	}
	onFocused(label) {
		label.select(0, label.length)
	}
	onKeyDown(label, key, repeat, ticks) {
		var charCode = this.filterKey(label, key);
		if (!charCode) return false
		var edited = false;
		switch (charCode) {
		case 2: /* delete selection */
			label.insert();
			edited = true;
			break;
		case 8: /* backspace */
			if (label.selectionLength == 0)
				label.select(label.selectionOffset - 1, 1)
			label.insert()
			edited = true;
			break;
		case 28: /* left */
			if (shiftKey) {
				label.select(label.selectionOffset - 1, label.selectionLength + 1);
			}
			else {
				if (label.selectionLength == 0)
					label.select(label.selectionOffset - 1, 0);
				else
					label.select(label.selectionOffset, 0);
			}
			break;
		case 29: /* right */
			if (shiftKey)
				label.select(label.selectionOffset, label.selectionLength + 1);
			else {
				if (label.selectionLength == 0)
					label.select(label.selectionOffset + 1, 0);
				else
					label.select(label.selectionOffset + label.selectionLength, 0);
			}
			break;
		case 30: /* up */
			return false;
		case 31: /* down */
			return false;
		case 127: /* delete */
			if (label.selectionLength == 0)
				label.select(label.selectionOffset, 1)
			label.insert()
			edited = true;
			break;
		default:
			this.insertKey(label, key);
			edited = true;
		}
		this.onReveal(label);
		if (edited)
			this.onEdited(label);
		return true;
	}
	onKeyUp(label, key, repeat, ticks) {
		var charCode = this.filterKey(label, key);
		if (!charCode) return false;
		return true;
	}
	onReveal(label) {
		label.container.reveal(label.selectionBounds);
	}
	onTimeChanged(label) {
		var scroller = label.container;
		var t = Date.now();
		scroller.scrollBy(this.dx * this.acceleration * (t - this.tx), 0);
		this.onTouchSelect(label, this.x, this.y);
	}
	onTouchBegan(label, id, x, y, ticks) {
		label.captureTouch(id, x, y, ticks);
		this.onTouchBeganMode(label, id, x, y, ticks);
	}
	onTouchBeganMode(label, id, x, y, ticks, mode) {
		label.focus();
		var anchor = this.anchor;
		var bounds = label.bounds;
		var offset = label.hitOffset(x - bounds.x, y - bounds.y);
		var from, to;
		if (shiftKey) {
			if (offset < label.selectionOffset) {
				from = offset;
				to = anchor.from = anchor.to = label.selectionOffset + label.selectionLength;
			}
			else {
				from = anchor.from = anchor.to = label.selectionOffset;
				to = offset;
			}
		}
		else
			from = to = anchor.from = anchor.to = offset;
		label.select(from, to - from);
		this.x = x;
		this.y = y;
	}
	onTouchEnded(label, id, x, y, ticks) {
		label.stop();
	}
	onTouchMoved(label, id, x, y, ticks) {
		if ((this.x != x) || (this.y != y))
			this.onTouchSelect(label, x, y);
		this.x = x;
		this.y = y;
		var scroller = label.container;
		var dx = 0, dy = 0;
		if (x < scroller.x) 
			dx = -1;
		else if (scroller.x + scroller.width < x) 
			dx = 1;
		if (this.dx != dx) {
			this.dx = dx;
			this.tx = Date.now();
		}
		if (dx)
			label.start();
		else
			label.stop();
	}
	onTouchSelect(label, x, y) {
		var anchor = this.anchor;
		var bounds = label.bounds;
		var offset = label.hitOffset(x - bounds.x, y - bounds.y);
		if (anchor.from < offset)
			label.select(anchor.from, offset - anchor.from);
		else
			label.select(offset, anchor.to - offset);
	}
	onUnfocused(label) {
	}
};

export class FieldScrollerBehavior extends Behavior {
	onStateChanged(container) {
		container.state = container.active ? 1 : 0;
	}
	onTouchBegan(scroller, id, x, y, ticks) {
		scroller.captureTouch(id, x, y, ticks);
		var label = scroller.first;
		label.behavior.onTouchBeganMode(label, id, x, y, ticks);
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		var label = scroller.first;
		label.behavior.onTouchMoved(label, id, x, y, ticks);
	}
	onTouchEnded(scroller, id, x, y, ticks) {
		var label = scroller.first;
		label.behavior.onTouchEnded(label, id, x, y, ticks);
	}
};

export class PopupDialogBehavior extends Behavior {	
	onCreate(layout, data) {
		this.data = data;
		let index = data.items.findIndex(item => item == data.value);
		let column = layout.first.first;
		if ((0 <= index) && (index < column.length)) {
			this.selection = index;
			column.content(index).first.visible = true;
		}
	}
	onDisplayed(layout) {
		shell.behavior.onHover();
	}
	onMeasureHorizontally(layout, width) {
		return width;
	}
	onMeasureVertically(layout, height) {
		let data = this.data;
		let button = data.button;
		let scroller = layout.first;
		let column = scroller.first;
		let index = this.selection;
		let step = column.first.measure().height;
		let y = button.y + ((button.height - step) >> 1);
		let listTop = y - (index * step);
		scroller.coordinates = { left:button.x - step, width:button.width + step, top:0, bottom:0 };
		scroller.tracking = true;
		scroller.scrollTo(0, 0 - listTop);
		return height;
	}
	onSelected(layout, item) {
		shell.remove(shell.last);
		this.data.button.bubble("onMenuSelected", item);
	}
	onTouchEnded(layout, id, x, y, ticks) {
		var column = layout.first.first;
		if (!column.hit(x, y))
			this.onSelected(layout, null);
	}
};

export class PopupScrollerBehavior extends Behavior {	
	onTouchScrolled(scroller, touched, dx, dy) {
		var content = scroller.first;
		var size = scroller.height;
		var range = content.height;
		if (dy < 0) {
			var offset = (content.y + range + content.skin.margins.bottom) - (scroller.y + size);
			if (offset > 0)
				scroller.scrollBy(0, Math.min(-dy, offset));
		}
		else if (dy > 0) {
			var offset = scroller.y - content.y + content.skin.margins.top;
			if (offset > 0)
				scroller.scrollBy(0, -Math.min(dy, offset));
		}
	}
};

export class PopupShadowBehavior extends Behavior {	
	onCreate(shadow, delta) {
		this.delta = delta;
		this.acceleration = 0.01;
	}
	onMouseEntered(shadow, x, y) {
		this.now = Date.now();
		shadow.start();
	}
	onMouseExited(shadow, x, y) {
		shadow.stop();
	}
	onTimeChanged(code) {
		var scroller = code.container;
		var now = Date.now();
		scroller.scrollBy(0, this.delta * this.acceleration * (now - this.now));
	}
	onScrolled(shadow) {
		var scroller = shadow.container;
		var content = scroller.first;
		var size = scroller.height;
		var range = content.height;
		if (this.delta > 0) {
			var offset = (content.y + range + content.skin.margins.bottom) - (scroller.y + size);
			if (offset > 0) {
				shadow.active = shadow.visible = true;
			}
			else {
				shadow.active = shadow.visible = false;
				shadow.stop();
			}
		}
		else {
			var offset = scroller.y - content.y + content.skin.margins.top;
			if (offset > 0) {
				shadow.active = shadow.visible = true;
			}
			else {
				shadow.active = shadow.visible = false;
				shadow.stop();
			}
		}
	}
}

export class PopupItemBehavior extends ButtonBehavior {
	onTap(line) {
		line.bubble("onSelected", this.data);
	}
}

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

export class HorizontalScrollbarBehavior extends ScrollbarBehavior {
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

export class VerticalScrollbarBehavior extends ScrollbarBehavior {
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
