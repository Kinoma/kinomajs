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

exports.configure = function() {
}

exports.close = function() {
}

exports.scan = function(parameters) {
	var bus = 0;	
    if (parameters && ("bus" in parameters))
        bus = parseInt(parameters.bus)

	switch (bus) {
		case 0:
			return [0x20, 0x21, 0x70];
		break
		case 101:
			return [0x10];
		break
		case 102:
			return [0x20];
		break
	}
}