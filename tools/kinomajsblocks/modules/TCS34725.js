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

let COMMAND = 0x80;
let ID = 0x12;
let ATIME = 0X1;
let CONTROL = 0XF;
let CDATAL = 0x14;
let RDATAL = 0x16;
let GDATAL = 0x18;
let BDATAL = 0x1A;
let ENABLE = 0;
let ENABLE_PON = 0x01;
let ENABLE_AEN = 0x02;

export const pins = {
    rgb: { type: "I2C", address: 0x29, gain: 16, integrationTime: 153.6 },
    led: { type: "Digital", direction: "output", value: 1 }
};

export function configure(configuration) {
	this.rgb.init();
	let id = this.rgb.readByteDataSMB( COMMAND | ID );
	if (0x44 != id)
		throw "colorSensor - cannot find device - got ID " + id;
	this.rgb.writeByteDataSMB( COMMAND | ENABLE, ENABLE_PON );
	sensorUtils.mdelay( 3 );
	this.rgb.writeByteDataSMB( COMMAND | ENABLE, ENABLE_PON | ENABLE_AEN );

    this.setGain( configuration.pins.rgb.gain );
    this.setIntegrationTime( configuration.pins.rgb.integrationTime );

    if ( "led" in this ) {
        this.led.init();
        this.setLED( configuration.pins.led.value );
    }
}

export function close() {
	this.rgb.close();
    if ( "led" in this )
        this.led.close();
}

export function getColor() {

	let r = this.rgb.readWordDataSMB( COMMAND | RDATAL );
	let g = this.rgb.readWordDataSMB( COMMAND | GDATAL );
	let b = this.rgb.readWordDataSMB( COMMAND | BDATAL );
	let c = this.rgb.readWordDataSMB( COMMAND | CDATAL );

	return '#' + Math.round( ( r / c ) * 255 ).toString(16) + Math.round( ( g / c ) * 255 ).toString(16) + Math.round( ( b / c ) * 255 ).toString(16);
}

export function setGain( gain ) {
    let value;
    switch ( gain ) {
        case 1: value = 0; break;
        case 4: value = 1; break;
        case 16: value = 2; break;
        case 60: value = 3; break;
        default: throw "Invalid gain " + gain;
    }
	this.rgb.writeByteDataSMB(COMMAND | CONTROL, value);
}

export function setIntegrationTime( time ) {
    if ( ( time < 2.4 ) || ( time > 614.4 ) )
        throw "Invalid integrationIime " + time;

    let value = Math.round( 256 - ( time / 2.4 ) );
	this.rgb.writeByteDataSMB( COMMAND | ATIME, value );
    return value;
}

export function setLED( value ) {
    this.led.write( value ? 1 : 0 );
}
