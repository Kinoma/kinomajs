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
		digital: {type: "Digital"},
	},
	configure(configuration, group) {
		if ("input" == this.digital.direction) {
			this.pinsSimulator = shell.delegate("addSimulatorPart", {
				header : { 
					label : "Digital Input",
					name : "Pin " + this.digital.pin,
					iconVariant : PinsSimulators.SENSOR_BUTTON 
				},
				axes : [
					new PinsSimulators.DigitalInputAxisDescription(
						{
							valueLabel : "Button",
							valueID : "value"
						}
					),
				]
			});
		}
		else if ("output" == this.digital.direction) {
			this.pinsSimulator = shell.delegate("addSimulatorPart", {
				header : { 
					label : "Digital Output",
					name : "Pin " + this.digital.pin,
					iconVariant : PinsSimulators.SENSOR_LED
				},
				axes : [
					new PinsSimulators.DigitalOutputAxisDescription(
						{
							valueLabel : "LED",
							valueID : "value"
						}
					),
				]
			});
		}
		else
			throw new Error("Bad Digital direction " + this.digital.direction);
	},
	read() {
		return this.pinsSimulator.delegate("getValue").value;
	},
	write(value) {
		this.pinsSimulator.delegate("setValue", "value", value);
	},
	setDirection(direction) {
		var saveValue = this.pinsSimulator.delegate("getValue");
		this.digital.direction = direction;
		this.close();									// rebuild simulator
		this.configure();
		this.pinsSimulator.delegate("setValue", "value", saveValue.value);
		return direction;
	},
	getDirection() {
		return this.digital.getDirection();
	},
	close(...args) {
		shell.delegate("removeSimulatorPart", this.pinsSimulator);
	},
	get direction() {
		return this.getDirection();
	},
	set direction(direction) {
		return this.setDirection(direction);
	},
	metadata: {
		sources: [
			{
				name: "read",
				result: { type: "Boolean", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
			},
		],
		sinks: [
			{
				name: "write",
				params: { type: "Boolean", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
			},
		]
	}
};
