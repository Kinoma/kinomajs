//@module
/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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

var Pins = require("pins");

exports.pins = {
    i2c: { type: "I2C" }
};

exports.configure = function(configuration) {
	this.sda = configuration.pins.i2c.sda;
	this.clock = configuration.pins.i2c.clock;
	this.address = configuration.pins.i2c.address;
    this.i2c.init();
}

exports.close = function() {
	this.i2c.close();
}

exports.readByteDataSMB = function(params) {
	var register = params.register;
	var value = this.i2c.readByteDataSMB(register);
	return value;
}

exports.writeByteDataSMB = function(params) {
	var register = params.register;
	var byteData = params.byteData;
	this.i2c.writeByteDataSMB(register, byteData);
}

exports.readWordDataSMB = function(params) {
	var register = params.register;
	var value = this.i2c.readWordDataSMB(register);
	return value;
}

exports.writeWordDataSMB = function(params) {
	var register = params.register;
	var wordData = params.wordData;
	this.i2c.writeWordDataSMB(register, wordData);
}

exports.setAddress = function(address) {
	this.i2c.close();
	this.i2c = Pins.create( { type: "I2C", sda: this.sda, clock: this.clock, address: address } );
}

