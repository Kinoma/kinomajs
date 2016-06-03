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
    analogInput: { type: "A2D", direction: "input" }
};

exports.configure = function(configuration) {
	this.pinNumber = configuration.pins.analogInput.pin;
    this.analogInput.init();
}

exports.close = function() {
	this.analogInput.close();
}

exports.getValue = function() {
	var value = this.analogInput.read();
	return value;
}