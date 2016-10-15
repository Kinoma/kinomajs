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

export const pins = {
	range: {type: "Analog", supplyVoltage: 3.3, voltsPerInch: 0.0064}
};

export function configure(configuration) {
	this.voltsPerInch = configuration.pins.range.voltsPerInch;
	this.supplyVoltage = configuration.pins.range.supplyVoltage;
	this.range.init();
}

export function read() {
    var measured = this.range.read();
    var range = (measured * this.supplyVoltage) / this.voltsPerInch;
    trace("range : " + range + "\n" );
    return range;
}

export function close() {
	this.range.close();
}
