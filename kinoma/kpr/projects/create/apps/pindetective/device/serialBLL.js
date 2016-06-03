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
	this.serial = PINS.create( { type: "Serial", tx: 31, rx: 33, baud: baud } );
	this.serial.init();
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
