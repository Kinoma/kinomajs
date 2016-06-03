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

exports.pins = {
    i2c: { type: "I2C" }
};

exports.configure = function(configuration) {	

	this.value = undefined;
	this.sda = -1;
	this.clock = -1;
	this.address = -1;
	
	for (var inputName in configuration.pins) {
		var input = configuration.pins[inputName];
		if (input.type == "I2C") {
			this.sda = input.sda;
			this.clock = input.clock;
			this.address = input.address;
			break;
		}
	}

	this.pinsSimulator = addSimulatorPart(this.sda, this.clock, this.address);
}

var addSimulatorPart = function(sda, clock, address) {
	var pinsSimulator = shell.delegate("addSimulatorPart", {
		id : 'I2C',
		header : { 
			label : "I2C Part", 
			name : "I2C sda " + sda + " clk " + clock + " address: " + address,
			iconVariant : PinsSimulators.SENSOR_MODULE 
		},
		axes : [
		]
	});
	return pinsSimulator;
}

exports.close = function() {
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
}

exports.setAddress = function(address) {
	this.address = address;
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
	this.i2c.close();
	this.i2c = PINS.create({type: "I2C", sda: this.sda, clock: this.clock, address: address});
	this.i2c.init();
	this.pinsSimulator = addSimulatorPart(this.sda, this.clock, this.address);
}

exports.readByteDataSMB = function(params) {
trace("\n simulator bll readByteDataSMB register: " + params.register + " address: " + this.address);
	return 256;
}

exports.readWordDataSMB = function(params) {
trace("\n simulator bll readWordDataSMB register: " + params.registe + " address: " + this.address);
	return 512;
}

exports.writeByteDataSMB = function(params) {
trace("\n simulator bll writeByteDataSMB byteData: " + params.byteData + " register: " + params.register + " address: " + this.address);
}

exports.writeWordDataSMB = function(params) {
trace("\n simulator bll writeWordDataSMB wordData: " + params.wordData + " register: " + params.register + " address: " + this.address);
}

/*
var metadata = exports.metadata = {
	sinks: [
		{
			name: "writeByteDataSMB",
			params: [
				{ type: "Number", name: "value", decimalPlaces: 0 }
			]
		},
		{
			name: "writeWordDataSMB",
			params: [
				{ type: "Number", name: "value", decimalPlaces: 0 }
			]
		},
	],
	sources: [
		{
			name: "readByteDataSMB",
			result: [
				{ type: "Number", name: "value", decimalPlaces: 0 }
			],
			repeat: 50
		},
		{
			name: "readWordDataSMB",
			result: [
				{ type: "Number", name: "value", decimalPlaces: 0 }
			],
			repeat: 50
		},
	]
};
*/

