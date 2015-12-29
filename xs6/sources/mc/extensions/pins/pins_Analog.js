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

export default class Analog {
	constructor(pinconf) {
		// we are interested in only "pin"
		this.pin = pinmap(pinconf.pin);
	};
	init() {
		this.a2d = GPIOPin.a2d({pin: this.pin});
	};
	read() {
		return this.a2d.read();
	};
	close() {
		this.a2d.close();
		GPIOPin.close(this.pin);
	};
};
