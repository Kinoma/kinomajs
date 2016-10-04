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
	pwm: {type: "PWM"}
}

PinsSimulators = require("PinsSimulators");

exports.configure = function(configuration, group)
{
    this.pinsSimulator = shell.delegate("addSimulatorPart", {
        header : { 
            label : group,
			name : "PWM Pin " + this.pwm.pin,
            iconVariant : PinsSimulators.SENSOR_MODULE
        },
		axes : [
			new PinsSimulators.FloatAxisDescription(
				{
					ioType : "output",
					dataType : "float",
					valueLabel : "Value",
					valueID : "value",
					minValue : 0,
					maxValue : 1,
					value : 0.5,
					speed : 1,
                    defaultControl: PinsSimulators.SLIDER
				}
			),
			new PinsSimulators.FloatAxisDescription(
				{
					ioType : "output",
					dataType : "float",
					valueLabel : "Pulse Width",
					valueID : "dutyCycle",
					minValue : 0,
					maxValue : 30,
					value : 30,
					speed : 1,
                    defaultControl: PinsSimulators.SLIDER
				}
			),
			new PinsSimulators.FloatAxisDescription(
				{
					ioType : "output",
					dataType : "float",
					valueLabel : "Period",
					valueID : "period",
					minValue : 0,
					maxValue : 30,
					value : 30,
					speed : 1,
                    defaultControl: PinsSimulators.SLIDER
				}
			),
		]
    });
}

exports.close = function()
{
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}

exports.write = function(value)
{
	this.pinsSimulator.delegate("setValue", "value", value);
	this.pinsSimulator.delegate("setValue", "dutyCycle", 0);
	this.pinsSimulator.delegate("setValue", "period", 0
	);
}

exports.writeDutyCyclePeriod = function(params) {
	this.pinsSimulator.delegate("setValue", "value", 0);
	this.pinsSimulator.delegate("setValue", "dutyCycle", params.dutyCycle);
	this.pinsSimulator.delegate("setValue", "period", params.period);
}

exports.metadata = {
	sinks: [
		{
			name: "write",
			params: { type: "Number", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
		},
	]
};
