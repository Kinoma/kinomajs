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

export default class Serial {
	constructor(pinconf) {
		// we are interested in only "txd", "rxd" and "baud"
		this.tx = pinmap(pinconf.tx);
		this.rx = pinmap(pinconf.rx);
		this.baud = pinconf.baud;
	};
	init() {
		this.uart = GPIOPin.uart({tx: this.tx, rx: this.rx, baud: this.baud});
	};
	read(...param) {
		return this.uart.read(...param);
	};
	write(...param) {
		return this.uart.write(...param);
	};
	repeat(...param) {
		return this.uart.repeat(...param);
	};
	close() {
		this.uart.close();
		GPIOPin.close(this.tx);
		GPIOPin.close(this.rx);
	};
};
