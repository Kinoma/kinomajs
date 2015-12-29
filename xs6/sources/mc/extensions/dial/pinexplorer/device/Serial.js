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
	serial: { type: "Serial", tx: 31, rx: 33, baud: 9600 }
};

exports.close = function() {
	this.serial.close();
}

exports.configure = function() {
	this.serial.init();
}

exports.readString = function() {
	return this.serial.read("String");
}

exports.readAsIntegerArray = function() {
	var array = this.serial.read("Array");
	if (array.length > 0) {
		trace("\n read serial array of length: " + array.length);
		for (var i=0; i < array.length; i++) {
			trace("\n int[" + i + "]: " + array[i]);
		}
	}
	return array;
}

exports.setBaud = function(baud) {
	this.serial.close();
	this.serial = Pins.create( { type: "Serial", tx: 31, rx: 33, baud: baud } );
}

exports.write = function(a) {
//	trace("\n attempt serial write using apply with args length: " + a.length);

//	this.serial.write.apply(this, a);
	
	switch (a.length) {
		case 0:
		break;
		case 1:
			this.serial.write(a[0]);
		break;
		case 2:
			this.serial.write(a[0], a[1]);
		break;
		case 3:
			this.serial.write(a[0], a[1], a[2]);
		break;
		case 4:
			this.serial.write(a[0], a[1], a[2], a[3]);
		break;
		case 5:
			this.serial.write(a[0], a[1], a[2], a[3], a[4]);
		break;
		case 6:
			this.serial.write(a[0], a[1], a[2], a[3], a[4], a[5]);
		break;
		case 7:
			this.serial.write(a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
		break;
		case 8:
			this.serial.write(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
		break;
		case 9:
			this.serial.write(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
		break;
		case 10:
			this.serial.write(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
		break;
	}
}
