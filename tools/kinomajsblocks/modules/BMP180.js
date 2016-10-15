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
export const pins = {
	bmp: {type: "I2C", address: 0x77},
	// accs: {type: "I2C", address: 0x53},
};

export function configure(configuration) {
	if('bmp' in configuration.pins){
		this.bmp.init();
		this.configureBMP(configuration);
	}
	else this.bmp = undefined;

}

export function read16(i2c,reg, sign) {
	var msb = i2c.readByteDataSMB(reg);
	var lsb = i2c.readByteDataSMB(reg + 1);
	var v = (msb << 8) | lsb;
	return sign && v > 32767 ? v - 65536 + 1 : v;
};

export function read24(i2c,reg, sign) {
	var msb = i2c.readByteDataSMB(reg);
	var lsb = i2c.readByteDataSMB(reg + 1);
	var xlsb = i2c.readByteDataSMB(reg + 2);
	var v = (msb << 16) | (lsb << 8) | xlsb;
	return sign ? v : toUnsigned(v);
};

export function toUnsigned(v){
	return (v >>> 0);
};

export function configureBMP(configuration){
	if('oss' in configuration.pins.bmp)
		this.bmp.oss = configuration.pins.bmp['oss'];
	else
		this.bmp.oss = 0;

	var id = this.bmp.readByteDataSMB(0xd0);
	if (id != 0x55) {
		trace("unknown ID: " + id + "\n");
		return;
	}

	this.bmp.params = {};
	var params = this.bmp.params;
	var read16 = this.read16;
	var i2c = this.bmp;

	params.ac1 = read16(i2c, 0xaa, true);
	params.ac2 = read16(i2c, 0xac, true);
	params.ac3 = read16(i2c, 0xae, true);
	params.ac4 = read16(i2c, 0xb0);
	params.ac5 = read16(i2c, 0xb2);
	params.ac6 = read16(i2c, 0xb4);
	params.b1 = read16(i2c, 0xb6, true);
	params.b2 = read16(i2c, 0xb8, true);
	params.mb = read16(i2c, 0xba, true);
	params.mc = read16(i2c, 0xbc, true);
	params.md = read16(i2c, 0xbe, true);
	trace("ac1 = " + params.ac1 + "\n");
	trace("ac2 = " + params.ac2 + "\n");
	trace("ac3 = " + params.ac3 + "\n");
	trace("ac4 = " + params.ac4 + "\n");
	trace("ac5 = " + params.ac5 + "\n");
	trace("ac6 = " + params.ac6 + "\n");
	trace("b1 = " + params.b1 + "\n");
	trace("b2 = " + params.b2 + "\n");
	trace("mb = " + params.mb + "\n");
	trace("mc = " + params.mc + "\n");
	trace("md = " + params.md + "\n");

}



export function close() {
	if (this.bmp) {
		this.bmp.close();
		this.bmp = undefined;
	}
}

export function delay(oss){
	switch(oss){
		case 0:
			return 5;	//4.5ms
		case 1:
			return 8;	//7.5ms
		case 2:
			return 14;	//13.5ms
		case 3:
			return 26;	//25.5ms
	}
	return 5;	//default?
}

exports.read = function () {
	var read16 = this.read16;
	var read24 = this.read24;
	var toUnsigned = this.toUnsigned;
	var i2c = this.bmp;
	var oss = this.bmp.oss;		//0;
	var delay = this.delay(oss);
	var params = this.bmp.params;

	i2c.writeByteDataSMB(0xf4, 0x2e);	// temperature
	sensorUtils.mdelay(5);	// actually 4.5 ms
	// System.sleep(5);
	var ut = read16(i2c, 0xf6);
	// trace('ut = ' + ut + '\n');

	i2c.writeByteDataSMB(0xf4, 0x34 + (oss << 6));
	sensorUtils.mdelay(5);	// actually 4.5 ms
	// sensorUtils.mdelay(delay);
	// System.sleep(5);
	var up = read24(i2c, 0xf6, true) >> (8-oss);
	// trace('up = ' + up + '\n');

	//temperature
	var x1 = ((ut - params.ac6) * params.ac5) >> 15;
	var x2 = (params.mc << 11) / (x1 + params.md);
	var b5 = x1 + x2;
	var t = (b5 + 8) >> 4;	// in 0.1C

	//pressure
	var b6 = b5 - 4000;
	// trace('b6 = ' + b6 + '\n');
	x1 = (params.b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (params.ac2 * b6) >> 11;
	var x3 = x1 + x2;
	// trace('x1 = ' + x1 + '\n');
	// trace('x2 = ' + x2 + '\n');
	// trace('x3 = ' + x3 + '\n');

	var b3 = ((((params.ac1 << 2) + x3) << oss) + 2) >> 2;
	// trace('b3 = ' + b3 + '\n');

	x1 = (params.ac3 * b6) >> 13;
	x2 = (params.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = (x1 + x2 + 2) >> 2;

	// trace('x1 = ' + x1 + '\n');
	// trace('x2 = ' + x2 + '\n');
	// trace('x3 = ' + x3 + '\n');


	var b4 = (params.ac4 * (toUnsigned(x3 + 32758))) >> 15;
	var b7 = (toUnsigned(up) - b3) * (50000 >> oss);


	// trace('b4 = ' + b4 + '\n');
	// trace('b7 = ' + b7 + '\n');

	var p;
	if (b7 < 0x80000000)
		p = (b7 * 2) / b4;
	else
		p = (b7 / b4) * 2;
	// trace('p = ' + p + '\n');
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	// trace('x1 = ' + x1 + '\n');
	// trace('x2 = ' + x2 + '\n');
	// trace('x3 = ' + x3 + '\n');

	p = p + ((x1 + x2 + 3791) >> 4);
	a = (p/100.0 - 1013.25) * 8.43;	//altitude above sea level, unit: meter

	// trace("temp = " + t / 10 + "." + t % 10 + "\n");
	// trace("pressure = " + p + " Pa, " + (p * 0.01) + " hPa" + "\n");
	sensorUtils.mdelay(1000);
	// System.sleep(1000);
	return{'temperature': t, 'pressure': p, 'altitude': a};
}
