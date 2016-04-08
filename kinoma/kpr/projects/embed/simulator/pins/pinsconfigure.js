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

var gSettings;

exports.configure = function () {
	gSettings = {
        leftPins: [1,2,3,4,5,6,7,0],
        rightPins: [10,6,7,4,3,2,1,0],
	    back: createTestBackpinsDirections(),
        leftVoltage: 5,
        rightVoltage: 3.3
    }
}

exports.close = function() {
    gSettings = undefined;
}

exports.get = function(parameters) {
    return gSettings;
}

exports.set = function(parameters) {
    if ("leftPins" in parameters) {
		checkPins(parameters.leftPins);
		gSettings.leftPins = parameters.leftPins;
	}
	if ("rightPins" in parameters) {
		checkPins(parameters.rightPins);
		gSettings.rightPins = parameters.rightPins;
	}
	if ("leftVoltage" in parameters) {
		checkVoltage(parameters.leftVoltage);
		gSettings.leftVoltage = parameters.leftVoltage;
	}
	if ("rightVoltage" in parameters) {
		checkVoltage(parameters.rightVoltage);
		gSettings.rightVoltage = parameters.rightVoltage;
	}
}

exports.hibernate = function() {
}

exports.wake = function() {
}

function createTestBackpinsDirections() {
    var directions = new Array(50).fill(0);
	for (var pinNumber = 3; pinNumber <= 12; pinNumber++)
		directions[pinNumber] = "input";
	for (var pinNumber = 15; pinNumber <= 24; pinNumber++)
		directions[pinNumber] = "output";
	return directions;
}

function checkVoltage(voltage)
{
	if ((3.3 !== voltage) && (5 !== voltage))
		throw new Error("Invalid voltage: " + voltage);
}

function checkPins(pins)
{
	if (pins.length != 8)
		throw new Error("Must specify 8 pins. Specified " + pins.length + ".");

	for (var i = 0; i < 8; i++) {
		if (pins[i] != Math.round(pins[i]))
			throw new Error("Bad pin type " + pins[i]);
		if ((pins[i] < 0) || ((pins[i] > 9) && (pins[i] != 10)))	// PWM=10 is now allowed in front pins
			throw new Error("Bad pin type " + pins[i]);
	}
}
