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
	//trace("\n simulator bll setAddress address: " + address);
	this.address = address;
	shell.delegate("removeSimulatorPart", this.pinsSimulator);
	this.i2c.close();	
	this.i2c = PINS.create({type: "I2C", sda: this.sda, clock: this.clock, address: address});
	this.i2c.init();
	this.pinsSimulator = addSimulatorPart(this.sda, this.clock, this.address);
}

exports.readByteDataSMB = function(register) {
	//trace("\n simulator bll readByteDataSMB register: " + register);
	return 256;
}

exports.readWordDataSMB = function(register) {
	//trace("\n simulator bll readWordDataSMB register: " + register);
	return 512;
}

exports.writeByteDataSMB = function(params) {
	//trace("\n simulator bll writeByteDataSMB register: " + params.register + " value: " + params.value);
}

exports.writeWordDataSMB = function(params) {
	//trace("\n simulator bll writeWordDataSMB register: " + params.register + " value: " + params.value);
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

