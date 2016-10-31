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

import Pins from "pins";

export default {
	pins: {
		i2c: {type: "I2C"},
	},
	configure(...args) {
		return this.i2c.init(...args);
	},
	readByte(...args) {
		return this.i2c.readByte(...args);
	},
	readBlock(...args) {
		return this.i2c.readBlock(...args);
	},
	readByteDataSMB(...args) {
		return this.i2c.readByteDataSMB(...args);
	},
	readBlockDataSMB(...args) {
		return this.i2c.readBlockDataSMB(...args);
	},
	writeByte(...args) {
		return this.i2c.writeByte(...args);
	},
	writeBlock(...args) {
		return this.i2c.writeBlock(...args);
	},
	writeWordDataSMB(...args) {
		return this.i2c.writeWordDataSMB(...args);
	},
	writeBlockDataSMB(...args) {
		return this.i2c.writeBlockDataSMB(...args);
	},
	writeQuickSMB(...args) {
		return this.i2c.writeQuickSMB(...args);
	},
	processCallSMB(...args) {
		return this.i2c.processCallSMB(...args);
	},
	close(...args) {
		return this.i2c.close(...args);
	},
	setAddress(address) {
		this.i2c.close();
		this.i2c = Pins.createPin( { type: "I2C", sda: this.sda, clock: this.clock, address: address } );
		this.i2c.init();
	}
};
