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
	this.i2c = PINS.create( { type: "I2C", sda: this.sda, clock: this.clock, address: address } );
	this.i2c.init();
}