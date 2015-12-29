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
import GPIOPin from "pinmux";
import pinmap from "map";

export default class Digital {
	constructor(pinconf) {
		// we are interested in only "pin"
		this.pin = pinmap(pinconf.pin);
		this._direction = pinconf.direction || whateverconf.direction;
	};
	init() {
		GPIOPin.pinmux([[this.pin, this._direction == "input" ? GPIOPin.GPIO_IN : GPIOPin.GPIO_OUT]]);
	};
	read() {
		return GPIOPin.read(this.pin);
	};
	write(val) {
		return GPIOPin.write(this.pin, val);
	};
	setDirection(direction) {
		GPIOPin.pinmux([[this.pin, direction == "input" ? GPIOPin.GPIO_IN : GPIOPin.GPIO_OUT]]);
		this._direction = direction;
	};
	getDirection() {
		return this._direction;
	};
	repeat(cb) {
		GPIOPin.event(this.pin, GPIOPin.FALLING_EDGE | GPIOPin.RISING_EDGE, cb);
	};
	close() {
		GPIOPin.close(this.pin);
	};
	get direction() {
		return this.getDirection();
	};
	set direction(direction) {
		this.setDirection(direction);
	};
};
