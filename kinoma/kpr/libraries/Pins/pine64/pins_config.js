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
 
var Pins = require("pins");


exports.configurationToMux = function(config)
{
	var mux = {
			leftPins: makeDisconnectedHeader(8),
			rightPins: makeDisconnectedHeader(8),
			back: makeBackHeader(),
			leftVoltage: ("leftVoltage" in config) ? config.leftVoltage : undefined,
			rightVoltage: ("rightVoltage" in config) ? config.rightVoltage : undefined
	};

	for (var bll in config) {
		if ((bll == "leftVoltage") || (bll == "rightVoltage"))
			continue;
		if (!("pins" in config[bll]))
			continue;
		var pins = config[bll].pins;
		for (var pin in pins) {
			pin = pins[pin];
			switch (pin.type) {
				case "Digital":
					setPin(mux, pin.pin, ("output" == pin.direction) ? Pins.digitalOut : Pins.digitalIn);
					break;
				case "A2D":
				case "Analog":
					setPin(mux, pin.pin, Pins.analogIn);
					break;
				case "I2C":
					setPin(mux, pin.sda, Pins.i2cData);
					setPin(mux, pin.clock, Pins.i2cClock);
					break;
				case "Serial":
					if ("rx" in pin) setPin(mux, pin.rx, Pins.serialRx);
					if ("tx" in pin) setPin(mux, pin.tx, Pins.serialTx);
					break;
				case "PWM":
					setPin(mux, pin.pin, Pins.pwm);
					break;
				case "Power":
					if ((5 != pin.voltage) && (3.3 != pin.voltage)) throw new Error("unsupported voltage " + pin.voltage)
					setPin(mux, pin.pin, (5 == pin.voltage) ? Pins.power5V : Pins.power3_3V);
					break;
				case "Ground":
					setPin(mux, pin.pin, Pins.ground);
					break;
				case "Audio":
					break;
				default:
					throw new Error("Unrecognized pin type " + pin.type);
					break;
			}
		}
	}

	if (undefined === mux.leftVoltage)
		mux.leftVoltage = 3.3;
	if (undefined === mux.rightVoltage)
		mux.rightVoltage = 3.3;

	return mux;
}

function setPin(mux, pin, type)
{
/*
	if ((51 <= pin) && (pin <= 58)) {
		validatePinAssignment(pin, mux.leftPins[pin - 51], type);
		if ((Pins.power3_3V == type) || (Pins.power5V == type)) {
			var leftVoltage = (Pins.power3_3V == type) ? 3.3 : 5;
			if (undefined === mux.leftVoltage)
				mux.leftVoltage = leftVoltage;
			else if (mux.leftVoltage !== leftVoltage)
				throw new Error("Cannot assign two different voltages to pins in left header");
			type = Pins.power3_3V;
		}
		mux.leftPins[pin - 51] = type;
		switch (pin - 51) {		// mirror front left pins to back
			case 0:	mux.back[37] = type; break;
			case 1:	mux.back[36] = type; break;
			case 2:	mux.back[39] = type; break;
			case 3:	mux.back[40] = type; break;
			case 4:	mux.back[43] = type; break;
			case 5:	mux.back[42] = type; break;
			case 6:	mux.back[47] = type; break;
			case 7:	mux.back[46] = type; break;
		}
	}
	else
	if ((59 <= pin) && (pin <= 66)) {
		validatePinAssignment(pin, mux.rightPins[pin - 59], type);
		if ((Pins.power3_3V == type) || (Pins.power5V == type)) {
			var rightVoltage = (Pins.power3_3V == type) ? 3.3 : 5;
			if (undefined === mux.rightVoltage)
				mux.rightVoltage = rightVoltage;
			else if (mux.rightVoltage !== rightVoltage)
				throw new Error("Cannot assign two different voltages to pins in right header");
			type = Pins.power3_3V;
		}
		mux.rightPins[pin - 59] = type;
	}
	else
	if ((1 <= pin) && (pin <= 50)) {
		validatePinAssignment(pin, mux.back[pin - 1], type);
		mux.back[pin - 1] = type;
		switch (pin - 51) {		// mirror back pins to front left
			case 37: mux.leftPins[0] = type; break;
			case 36: mux.leftPins[1] = type; break;
			case 39: mux.leftPins[2] = type; break;
			case 40: mux.leftPins[3] = type; break;
			case 43: mux.leftPins[4] = type; break;
			case 42: mux.leftPins[5] = type; break;
			case 47: mux.leftPins[6] = type; break;
			case 46: mux.leftPins[7] = type; break;
		}
	}
*/
}

function validatePinAssignment(pin, from, to)
{
/*
	if (from == to)
		return;

	if ((from == Pins.digitalUnconfigured) &&
		((to == Pins.digitalIn) || (to == Pins.digitalOut)))
		return;

	if (from == Pins.disconnected)
		return;

	throw new Error("Cannot configure pin " + pin + ", already assigned.");
*/
}

/* currently unused

function normalizeFrontConfig(config)
{
	if (!config) config = new Array;
	config = config.map(function(pin) {
		switch (pin) {
			case Pins.power5V:
			case Pins.power3_3V:
				return Pins.power3_3V;
			case Pins.ground:
			case Pins.analogIn:
			case Pins.digitalIn:
			case Pins.digitalOut:
			case Pins.i2cClock:
			case Pins.i2cData:
			case Pins.pwm:
				return pin;
			default:
				return Pins.disconnected; 
		}
	}, exports);

	while (config.length < 8)
		config.push(Pins.disconnected);

	return config;
}
*/

function makeBackHeader(count)
{
	var header = makeDisconnectedHeader(41);
	
	for (var i = 1; i <= 40; i++)
		header[i] = Pins.digitalUnconfigured;

	header[1] = header[17] = Pins.power3_3V;
	header[2] = header[4] = Pins.power5V;
	header[6] = header[9] =
	header[14] = header[20] =
	header[25] = header[30] =
	header[34] = header[39] = Pins.ground;

	header[8] = Pins.serialTx;
	header[10] = Pins.serialRx;

	header[3] = Pins.i2cData;
	header[5] = Pins.i2cClock;

	header.shift();
	
	return header;
}

function makeDisconnectedHeader(count)
{
	var header = new Array(count);
	for (var i = 0; i < count; i++)
		header[i] = Pins.disconnected;
	return header;
}

exports.getFixed = function(directions)
{	
	if (undefined == directions)
		directions = new Array(40);
    var fixedPins = [
		{ pin: 1, type: "Power", voltage: 3.3 },
		{ pin: 2, type: "Power", voltage: 5 },
		{ pin: 227, type: "I2CData", sda: 3, bus: 1 },
		{ pin: 4, type: "Power", voltage: 5 },
		{ pin: 226, type: "I2CClock", clock: 5, bus: 1 },
		{ pin: 6, type: "Ground" },
		{ pin: 362, type: "Digital", direction: directions[7] },
		{ pin: 32, type: "UartTX" },
		{ pin: 9, type: "Ground" },
		{ pin: 33, type: "UartRX" },
		{ pin: 71, type: "Digital", direction: directions[11] },
		{ pin: 72, type: "Digital", direction: directions[12] },
		{ pin: 233, type: "Digital", direction: directions[13] },
		{ pin: 14, type: "Ground" },
		{ pin: 76, type: "Digital", direction: directions[15] },
		{ pin: 77, type: "Digital", direction: directions[16] },
		{ pin: 17, type: "Power", voltage: 3.3 },
		{ pin: 78, type: "Digital", direction: directions[18] },
		{ pin: 64, type: "Digital", direction: directions[19] },
		{ pin: 20, type: "Ground" },
		{ pin: 65, type: "Digital", direction: directions[21] },
		{ pin: 79, type: "Digital", direction: directions[22] },
		{ pin: 66, type: "Digital", direction: directions[23] },
		{ pin: 67, type: "Digital", direction: directions[24] },
		{ pin: 25, type: "Ground" },
		{ pin: 231, type: "Digital", direction: directions[26] },
		{ pin: 361, type: "Digital", direction: directions[27] },
		{ pin: 360, type: "Digital", direction: directions[28] },
		{ pin: 229, type: "Digital", direction: directions[29] },
		{ pin: 30, type: "Ground" },
		{ pin: 230, type: "Digital", direction: directions[31] },
		{ pin: 68, type: "Digital", direction: directions[32] },
		{ pin: 69, type: "Digital", direction: directions[33] },
		{ pin: 34, type: "Ground" },
		{ pin: 73, type: "Digital", direction: directions[35] },
		{ pin: 70, type: "Digital", direction: directions[36] },
		{ pin: 80, type: "Digital", direction: directions[37] },
		{ pin: 74, type: "Digital", direction: directions[38] },
		{ pin: 39, type: "Ground" },
		{ pin: 75, type: "Digital", direction: directions[40] },
	];
    return fixedPins;
}

exports.hasMuxablePins = function()
{
	return false;
}
