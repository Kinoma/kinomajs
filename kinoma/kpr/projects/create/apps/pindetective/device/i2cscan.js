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

    var result = [];
    var bus = 0;

    if (parameters && ("bus" in parameters))
        bus = parseInt(parameters.bus)

    for (i = 0x03; i < 0x77; i++) {
        var i2c = undefined;

        try {
			if (bus == 0 && i >= 0x20 && i <= 0x21){ //skip the PICs, which apparently no longer tolerate being probed
				result.push(i);
				continue;
			}

            i2c = PINS.create({type: "I2C", address: i, bus: bus});
            i2c.init();

            if ( ((0x30 <= i) && (i <= 0x37)) || ((0x50 <= i) && (i <= 0x5F)) || (0 != bus) )
				i2c.readByteDataSMB(0);
			else
				i2c.writeQuickSMB(0);

            result.push(i);
        }
        catch (e) {
        	//trace("\n CATCH: " + e);
        }
        finally {
        	//trace("\n FINALLY");
            if (i2c)
                i2c.close();
        }
    }
    return result;
}
