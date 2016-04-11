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

let PinsSimulators = require("PinsSimulators");

export default {
	pins: {
		analog: {type: "Analog"},
	},
	configure(configuration, group) {
		this.pinsSimulator = shell.delegate("addSimulatorPart", {
			header : { 
				label : group,
				name : "Analog In. Pin " + this.analog.pin,
				iconVariant : PinsSimulators.SENSOR_MODULE
			},
			axes : [
				new PinsSimulators.FloatAxisDescription(
					{
						ioType : "input",
						dataType : "float",
						valueLabel : "Percent",
						valueID : "value",
						minValue : 0,
						maxValue : 1,
						value : 0.5,
						speed : 1,
						defaultControl: PinsSimulators.SLIDER
					}
				)
			]
		});
	},
	read() {
		return this.pinsSimulator.delegate("getValue").value;
	},
	close(...args) {
		shell.delegate("removeSimulatorPart", this.pinsSimulator);
	},
	metadata: {
		sources: [
			{
				name: "read",
				result: { type: "Number", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
			},
		]
	}
};

