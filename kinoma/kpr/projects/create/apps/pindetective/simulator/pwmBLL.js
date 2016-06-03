//@module
/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

var PinsSimulators = require ("PinsSimulators");

var valueID = "value";

exports.configure = function(configuration) {	

	this.value = undefined;

	this.pinNumber = -1;
	for (var inputName in configuration.pins) {
		var input = configuration.pins[inputName];		
		if (input.type == "PWM") {
			this.pinNumber = input.pin;						// store the pinNumber for getValue
			break;
		}
	}

	this.pinsSimulator = shell.delegate("addSimulatorPart", {
		id : 'PWM',
		header : { 
			label : "PWM", 
			name : "Pin " + this.pinNumber, 
			iconVariant : PinsSimulators.SENSOR_GAUGE 
		},
		axes : [
			new PinsSimulators.BooleanAxisDescription(
				{
					ioType : "output",
					dataType : "float",
					valueLabel : "Value",
					valueID : valueID,
					minValue : 0,
					maxValue : 1,
					value : 0
				}
			),
		]
	});
}

exports.close = function() {
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}

exports.write = function(value) {
//	var value = json[valueID];
	if (value != this.value) {
		this.value = value;
		this.pinsSimulator.delegate("setValue", valueID, value);
	}
}

exports.writeDutyCyclePeriod = function(params) {
	trace("\n writeDutyCyclePeriod dutyCycle: " + params.dutyCycle + " period: " + params.period);
}

var metadata = exports.metadata = {
	sinks: [
		{
			name: "write",
			params: { type: "Number", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
		},
		{
			name: "writeDutyCyclePeriod",
			params: 
				{ type: "Object", name: "params", properties:
					[
						{ type: "Number", name: "dutyCycle", defaultValue: 0, min: 0, max: 30, decimalPlaces: 0 },
						{ type: "Number", name: "period", defaultValue: 0, min: 0, max: 30, decimalPlaces: 0 },
					]
				},
		},
	]
};
