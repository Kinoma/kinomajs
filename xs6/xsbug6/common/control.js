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

export class FieldDeleterBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
		this.onStateChanged(content);
	}
	onStateChanged(content) {
		content.state = content.active ? 1 : 0;
	}
	onTouchBegan(content, id, x, y, ticks) {
		content.state = 2;
		content.captureTouch(id, x, y, ticks);
	}
	onTouchCancelled(content, id, x, y, ticks) {
		content.state = 1;
	}
	onTouchEnded(content, id, x, y, ticks) {
		content.state = 1;
		var label = content.previous.first;
		label.string = "";
		label.focus();
		label.behavior.onEdited(label);
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
