//@module
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

exports.pins = {
	digital: {type: "Digital"}
}

PinsSimulators = require("PinsSimulators");

exports.configure = function(configuration, group)
{
	if ("input" == this.digital.direction) {
		this.pinsSimulator = shell.delegate("addSimulatorPart", {
			id : 'DigitalInput',
			header : { 
				label :  "Digital Input " + this.digital.pin, 
				name : "Pin " + this.digital.pin, 
				iconVariant : PinsSimulators.SENSOR_BUTTON 
			},
			axes : [
				new PinsSimulators.BooleanAxisDescription(
					{
						ioType : "input",
						dataType : "boolean",
						valueLabel : "Value",
						valueID : "value",
						minValue : 0,
						maxValue : 1,
						value : 0,                                                 
						speed : 1
					}
				),
			]
		});
	}
	else if ("output" == this.digital.direction) {
		this.pinsSimulator = shell.delegate("addSimulatorPart", {
			id : 'DigitalOutput',
			header : { 
				label : "Digital Output " + this.digital.pin, 
				name : "Pin " + this.digital.pin, 
				iconVariant : PinsSimulators.SENSOR_LED 
			},
			axes : [
				new PinsSimulators.BooleanAxisDescription(
					{
						ioType : "output",
						dataType : "boolean",
						valueLabel : "Value",
						valueID : "value",
						minValue : 0,
						maxValue : 1,
						value : 0
					}
				),
			]
		});
	}
	else
		throw new Error("Bad Digital direction " + this.digital.direction);
}

exports.close = function()
{
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}

exports.read = function()
{
	return this.pinsSimulator.delegate("getValue").value;
}

exports.write = function(value)
{
	this.pinsSimulator.delegate("setValue", "value", value);
}

exports.setDirection = function(direction)
{
	this.digital.direction = direction;
	this.close();									// rebuild simulator
	this.configure();
	return direction;
}

exports.metadata = {
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
};

