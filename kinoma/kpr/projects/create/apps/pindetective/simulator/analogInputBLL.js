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

exports.configure = function(configuration) {	
	
	this.pinNumber = -1;						
	for (var inputName in configuration.pins) {
		var input = configuration.pins[inputName];
		if (input.type == "A2D") {
			this.pinNumber = input.pin;						// store the pinNumber for getValue
			break;
		}
	}
	
	this.pinsSimulator = shell.delegate("addSimulatorPart", {
		id : 'AnalogInput',
		header : { 
			label : "Analog Input", 
			name : "Pin " + this.pinNumber, 
			iconVariant : PinsSimulators.SENSOR_KNOB 
		},
		axes : [
			new PinsSimulators.FloatAxisDescription(
				{
					ioType : "input",
					dataType : "float",
					valueLabel : "Value",
					valueID : "value",
					minValue : 0,
					maxValue : 1,
					value : 0.5,                                                 
					speed : 1
				}
			),
		]
	});
}

exports.close = function() {
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}



exports.getValue = function() {
	var result = this.pinsSimulator.delegate("getValue");		// result.value has current value
	return result.value;
}


var metadata = exports.metadata = {
	sources: [
		{
			name: "getValue",
			result: { type: "Number", name: "value", defaultValue: 0, min: 0, max: 1, decimalPlaces: 3 },
		},
	]
};

/*
exports.getValue = function() {
	var result = this.pinsSimulator.delegate("getValue");		// result.value has current value
	var value = result.value;	
	var stringValue = value.toString();
	return { value: value, stringValue: stringValue };
}


var metadata = exports.metadata = {
	sources: [
		{
			name: "getValue",
			result:
				{ 
					type: "Object", name: "result", 
					properties: [
						{ type: "Number", name: "value", defaultValue: 0, min: 0, max: 200, decimalPlaces: 3 },
						{ type: "String", name: "stringValue", defaultValue: "default" },
					]
				},
		},
	]
};
*/





