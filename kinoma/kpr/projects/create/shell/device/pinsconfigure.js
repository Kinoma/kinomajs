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

// 1 == 3.3v, 0 == 5v

exports.configure = function() {
    this.leftVoltage = PINS.create({type: "Digital", pin: 1026, direction: "output"});
    this.rightVoltage = PINS.create({type: "Digital", pin: 1022, direction: "output"});
    this.leftPins = PINS.create({type: "I2C", address: 0x20, bus: 0});
    this.rightPins = PINS.create({type: "I2C", address: 0x21, bus: 0});

    this.leftVoltage.init();
    this.rightVoltage.init();
    this.leftPins.init();
    this.rightPins.init();
}
exports.close = function() {
    this.leftVoltage.close();
    this.rightVoltage.close();
    this.leftPins.close();
    this.rightPins.close();
}
exports.get = function() {
    var result = {
        leftVoltage: this.leftVoltage.read() ? 3.3 : 5,
        rightVoltage: this.rightVoltage.read() ? 3.3 : 5,
        leftPins: [],
        rightPins: [],
        back: new Array(50).fill(0)
    };

    for (var i = 4; i < 12; i++)
        result.leftPins[11 - i] = this.leftPins.readByteDataSMB(i);

    for (var i = 4; i < 12; i++)
        result.rightPins[11 - i] = this.rightPins.readByteDataSMB(i);

    readGPIO(result.back, 3, 12);
    readGPIO(result.back, 15, 24);

    return result;
}
exports.set = function(parameters) {
    /* voltage */
	parameters.leftVoltage = (5 == parameters.leftVoltage) ? 0 : 1;
	parameters.rightVoltage = (5 == parameters.rightVoltage) ? 0 : 1;
    if ((this.leftVoltage.read() != parameters.leftVoltage) ||
        (this.rightVoltage.read() != parameters.rightVoltage)) {
        this.leftVoltage.write(parameters.leftVoltage);
        this.rightVoltage.write(parameters.rightVoltage);

        // wait for PIC Controller to reboot
        sensorUtils.mdelay(200);
    }

    /* left */
    for (var pin = 0, pins = parameters.leftPins, length = pins.length; pin < length; pin++)
        this.leftPins.writeByteDataSMB(11 - pin, pins[pin]);

    this.leftPins.writeByteDataSMB(0x03, 1);
    while (1 == this.leftPins.readByteDataSMB(0x03))
        ;

    /* right */
    for (var pin = 0, pins = parameters.rightPins, length = pins.length; pin < length; pin++)
        this.rightPins.writeByteDataSMB(11 - pin, pins[pin]);

    this.rightPins.writeByteDataSMB(0x03, 1);
    while (1 == this.rightPins.readByteDataSMB(0x03))
        ;
}
exports.hibernate = function() {
    this.state = this.get();

    for (var pin = 0; pin < 8; pin++) {     // disconnect all pins
        this.leftPins.writeByteDataSMB(11 - pin, 0);
        this.rightPins.writeByteDataSMB(11 - pin, 0);
    }
}
exports.wake = function() {
    var i2c = PINS.create({type: "I2C", address: 0x70, bus: 0});
    i2c.init();
    i2c.writeByteDataSMB(0, 3)
    i2c.close();

    // wait for PIC Controller
    sensorUtils.mdelay(200);

    this.set(this.state);
    delete this.state;
}

function readGPIO(pins, start, stop)
{
    for (var i = start; i <= stop; i++) {
        var p = PINS.create({type: "Digital", pin: i, direction: "undefined"});
        p.init();
        pins[i] = p.direction;
        p.close();
    }
}
