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
import GPIOPin from "pinmux";
import pinmap from "map";

const DEFAULT_PERIOD = 20;

export default class PWM {
	constructor(pinconf) {
		this.pin = pinmap(pinconf.pin);
	};
	init() {
		this.gpt = GPIOPin.gpt({pin: this.pin});
	};
	read() {
		// what to read?
	};
	write(params, period) {
		if (Array.isArray(params)) {
			let dutyCycle, period;
			dutyCycle = params[0];
			if (params.length > 1)
				period = params[1];
			else
				period = params[0] * 2;	// ??
			this.gpt.pwm(dutyCycle, period - dutyCycle);
		}
		else if (period){
			this.gpt.pwm(params, period - params);
		}
		else {
			this.gpt.pwm(params * DEFAULT_PERIOD, DEFAULT_PERIOD - (params * DEFAULT_PERIOD));
		}		
	};
	close() {
		this.gpt.close();
		GPIOPin.close(this.pin);
	}
};
