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

var GPIO = {
	configure(pins) {
		var mux = [];
		for (var i in pins) {
			var pin = pins[i];
			mux.push([pinmap(pin.pin), pin.direction == "input" ? GPIOPin.GPIO_IN : GPIOPin.GPIO_OUT]);
		}
		GPIOPin.pinmux(mux);
		this.mux = mux;
	},
	read(...param) {
		return GPIOPin.read(...param);
	},
	write(...param) {
		return GPIOPin.write(...param);
	},
	close() {
		this.mux.forEach(function(e) {
			if (e[1] == GPIOPin.GPIO_IN || e[1] == GPIOPin.GPIO_OUT)
				GPIOPin.close(e[0]);
		});
	},
};
export default GPIO;
