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
import System from "system";
import TimeInterval from "timeinterval";

var LED_PINS = System.config.ledPins || [];

class LED {
	constructor(conf) {
		for (var i in conf) {
			this[i] = conf[i];
		}
		if (!this.onColor)
			this.onColor = [1, 1, 1];
		if (!this.offColor)
			this.offColor = [0, 0, 0];
		if (this.interval && this.pattern === undefined)
			this.pattern = 1;	// simple blink
		if (this.default)
			LED._default = this;
		// check if at least one LED should be in onColor
		if (!LED_PINS.some((e, i) => this.onColor[i])) {
			this.onColor[0] = 1;
			this.offColor[0] = 0;
		}
	};
	on(on) {
		let GPIOPin = require.weak("pinmux");
		GPIOPin.write(LED_PINS.map((e, i) => [e, on ? !this.onColor[i] : !this.offColor[i]]));
	};
	run(conf) {
		if (LED._default && LED._default != this)
			LED._default.stop();
		if (conf) {
			var interval = conf.interval;
			var pattern = conf.pattern;
			if (conf.onColor)
				this.onColor = conf.onColor;
			if (conf.offColor)
				this.offColor = conf.offColor;
			// check if at least one LED should be in onColor
			if (!LED_PINS.some((e, i) => this.onColor[i])) {
				this.onColor[0] = 1;
				this.offColor[0] = 0;
			}
		}
		else {
			var interval = this.interval;
			var pattern = this.pattern;
		}
		if (this.timer)
			delete this.timer;
		if (interval) {
			pattern = pattern || 1;
			var toggle = 0;
			this.timer = new TimeInterval(() => this.on(toggle++ & pattern), interval);
			this.timer.start();
		}
		else
			this.on(1);
	};
	stop(notresume) {
		if (this.timer) {
			this.timer.stop();
			delete this.timer;
		}
		this.on(0);
		if (!notresume)
			LED.resume();
	};
	static resume() {
		if (LED._default)
			LED._default.run();
	};
};

export default LED;
