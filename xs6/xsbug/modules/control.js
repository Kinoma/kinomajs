/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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

export var ButtonBehavior = Behavior.template({
	changeState(container, state) {
		container.state = state;
		var content = container.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	},
	onCreate(container, data) {
		this.data = data;
	},
	onDisplaying(container) {
		this.onStateChanged(container);
	},
	onStateChanged(container) {
		this.changeState(container, container.active ? 1 : 0);
	},
	onTap(container) {
		var data = this.data;
		if (data && ("action" in data))
			container.invoke(new Message(data.action));
	},
	onTouchBegan(container, id, x, y, ticks) {
		this.changeState(container, 2);
		container.captureTouch(id, x, y, ticks);
	},
	onTouchEnded(container, id, x, y, ticks) {
		if (container.hit(x, y)) {
			this.changeState(container, 1);
			this.onTap(container);
		}
	},
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 2 : 1);
	},
});

export var FieldDeleterBehavior = Behavior.template({
	onCreate(content, data) {
		this.data = data;
		this.onStateChanged(content);
	},
	onStateChanged(content) {
		content.state = content.active ? 1 : 0;
	},
	onTouchBegan(content, id, x, y, ticks) {
		content.state = 2;
		content.captureTouch(id, x, y, ticks);
	},
	onTouchCancelled(content, id, x, y, ticks) {
		content.state = 1;
	},
	onTouchEnded(content, id, x, y, ticks) {
		content.state = 1;
		var label = content.previous.first;
		label.string = "";
		label.focus();
		label.behavior.onEdited(label);
	},
});

export var FieldLabelBehavior = Behavior.template({
	onCreate(label, data) {
		this.data = data;
	},
	onDisplayed(label) {
		this.onEdited(label);
	},
	onEdited(label) {
	},
	onFocused(label) {
		label.select(0, label.length)
	},
	onKeyDown(label, key, repeat, ticks) {
		if (key) {
			var code = key.charCodeAt(0);
			var edited = false;
			switch (code) {
			case 1: /* home */
				label.select(0, 0);
				break;
			case 2: /* delete selection */
				label.insert();
				edited = true;
				break;
			case 3: /* enter */
				return false;
			case 4: /* end */
				label.select(label.length, 0);
				break;
			case 5: /* help */
				return false;
			case 8: /* backspace */
				if (label.selectionLength == 0)
					label.select(label.selectionOffset - 1, 1)
				label.insert()
				edited = true;
				break;
			case 9: /* tab */
				return false;
			case 11: /* page up */
				return false;
			case 12: /* page down */
				return false;
			case 13: /* return */
				if (label instanceof Text) {
					label.insert(key);
					edited = true;
				}
				else
					return false;
				break;
			case 27: /* escape */
				return false;
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
				if ((Event.FunctionKeyPlay <= code) && (code <= Event.FunctionKeyPower))
					return false;
				if (code > 0x000F0000)
					return false;
				label.insert(key);
				edited = true;
			}
		}
		else {
			label.insert()
			edited = true;
		}
		this.onReveal(label);
		if (edited)
			this.onEdited(label);
		return true;
	},
	onKeyUp(label, key, repeat, ticks) {
		if (!key) return false
		var code = key.charCodeAt(0);
		var edited = false;
		switch (code) {
		case 3: /* enter */
		case 5: /* help */
		case 9: /* tab */
		case 11: /* page up */
		case 12: /* page down */
		case 27: /* escape */
		case 30: /* up */
		case 31: /* down */
			return false;
		case 13: /* return */
			return label instanceof Text;
		default:
			if ((Event.FunctionKeyPlay <= code) && (code <= Event.FunctionKeyPower))
				return false;
			return code <= 0x000F0000;
		}
	},
	onReveal(label) {
		label.container.reveal(label.selectionBounds);
	},
	onTouchBegan(label, id, x, y, ticks) {
		this.position = label.position;
		var offset = label.hitOffset(x - this.position.x, y - this.position.y);
		if (shiftKey) {
			if (offset < label.selectionOffset)
				this.anchor = label.selectionOffset + label.selectionLength;
			else
				this.anchor = label.selectionOffset;
		}
		else
			this.anchor = offset;
		this.onTouchMoved(label, id, x, y, ticks);
	},
	onTouchCancelled(label, id, x, y, ticks) {
	},
	onTouchEnded(label, id, x, y, ticks) {
		this.onTouchMoved(label, id, x, y, ticks);
	},
	onTouchMoved(label, id, x, y, ticks) {
		this.offset = label.hitOffset(x - this.position.x, y - this.position.y);
		label.select(this.offset, 0);
		//if (this.anchor < this.offset)
		//	label.select(this.anchor, this.offset - this.anchor);
		//else
		//	label.select(this.offset, this.anchor - this.offset);
		trace(this.anchor + " " + this.offset + "\n");
	},
	onUnfocused(label) {
	},
});

export var FieldScrollerBehavior = Behavior.template({
	onTouchBegan(scroller, id, x, y, ticks) {
		var label = scroller.first;
		this.tracking = label.focused;
		if (this.tracking)
			label.behavior.onTouchBegan(label, id, x, y, ticks);
		else
			label.focus();
	},
	onTouchMoved(scroller, id, x, y, ticks) {
		var label = scroller.first;
		if (this.tracking)
			label.behavior.onTouchMoved(label, id, x, y, ticks);
	},
	onTouchEnded(scroller, id, x, y, ticks) {
		var label = scroller.first;
		if (this.tracking)
			label.behavior.onTouchEnded(label, id, x, y, ticks);
	},
});
