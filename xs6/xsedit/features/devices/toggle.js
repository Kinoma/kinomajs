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

export class SwitchButtonBehavior extends Behavior {
	changeOffset(container, offset) {
		var label = container.last;
		var line = label.first;
		var button = line.first.next;
		var bar = label.previous;
		var background = bar.previous;
		if (offset < 0)
			offset = 0;
		else if (offset > this.size)
			offset = this.size;
		this.offset = offset;
		bar.width = button.width + Math.round(this.size - offset);
		line.x = label.x - Math.round(offset);
	}
	onCreate(container, data) {
		this.data = data;
	}
	onDisplaying(container) {
		var label = container.last;
		var line = label.first;
		var button = line.first.next;
		var bar = label.previous;
		var background = bar.previous;
		this.half = background.width >> 1;
		this.size = background.width - button.width;
		line.first.coordinates = line.last.coordinates = { width: this.size - this.data.sizeInset };
		this.changeOffset(container, (this.data.value > 0) ? 0 : this.size);
	}
	onFinished(container) {
	}

	onTimeChanged(container) {
		this.changeOffset(container, this.anchor + Math.round(this.delta * container.fraction));
	}
	onTouchBegan(container, id, x, y, ticks) {
		if (container.running) {
			container.stop();
			container.time = container.duration;
		}
		this.anchor = x;
		this.capturing = false;
		this.delta = this.offset + x;
		container.last.first.first.next.state = 1;
	}
	onTouchCancelled(container, id, x, y, ticks) {
		container.last.first.first.next.state = 0;
	}
	onTouchEnded(container, id, x, y, ticks) {
		var offset = this.offset;
		var size =  this.size;
		var delta = size >> 1;
		if (this.capturing) {
			if (offset < delta)
				delta = 0 - offset;
			else 
				delta = size - offset;
		}
		else {
			if (offset == 0)
				delta = size;
			else if (offset == size)
				delta = 0 - size;
			else if (x < this.half)
				delta = 0 - offset;
			else
				delta = size - offset;
		}
		if (delta) {
			this.anchor = offset;
			this.delta = delta;
			container.duration = 250 * Math.abs(delta) / size;
			container.time = 0;
			container.start();
		}
		var newValue = ((this.offset + delta) == 0) ? 1 : 0;
		if (this.data.value != newValue) {
			this.data.value = newValue;
			this.onValueChanged(container, this.data.value);
		}
				
		container.last.first.first.next.state = 0;
	}
	onTouchMoved(container, id, x, y, ticks) {
		if (this.capturing) {
			this.changeOffset(container, this.delta - x);
		}
		else if (Math.abs(x - this.anchor) >= 8) {
			this.capturing = true;
			container.captureTouch(id, x, y, ticks);
			this.changeOffset(container, this.delta - x);
		}
	}
	onValueChanged(container, value) {
		this.changeOffset(container, value ? 0 : this.size);
	}
};

