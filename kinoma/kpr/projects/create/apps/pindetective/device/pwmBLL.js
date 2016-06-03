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
    pwm: { type: "PWM" },
};

exports.configure = function(configuration) {
	this.pinNumber = configuration.pins.pwm.pin;
	this.value = undefined;
    this.pwm.init();
}

exports.close = function() {
	this.pwm.close();
}

exports.write = function(value) {
	if (value != this.value) {
		this.value = value;
		this.pwm.write(value);
	}
}

exports.writeDutyCyclePeriod = function(params) {
	this.pwm.write(params.dutyCycle, params.period);
}