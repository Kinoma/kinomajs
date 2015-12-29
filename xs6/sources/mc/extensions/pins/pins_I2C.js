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
import GPIOPin from "pinmux";
import pinmap from "map";

export default class I2C {
	constructor(pinconf) {
		// we are interested in only "clock", "sda" and "address"
		this.clock = pinmap(pinconf.clock);
		this.sda = pinmap(pinconf.sda);
		this.address = pinconf.address;
	};
	init() {
		this.i2c = GPIOPin.i2c({sda: this.sda, scl: this.clock, addr: this.address});
	};
	readByte(reg) {
		return this.i2c.readChar(reg);
	};
	readByteDataSMB(reg) {
		return this.i2c.readChar(reg);
		
	};
	readWordDataSMB(reg) {
		return this.i2c.readWord(reg, 2, true);	// LSB first
	};
	readBlockDataSMB(reg, size, type) {
		var buf = this.i2c.read(reg, size);
		if (type == 'Chunk')
			return buf;
		else
			return new Uint8Array(buf);	// @@ not an Array buf a TypedArray.. no one will notice the difference
	};
	readBlock(size, type) {
		return this.readBlockDataSMB(undefined, size, type);
	};
	writeByte(val) {
		this.i2c.write(null, val, 1);
	};
	writeBlock() {
		if (arguments.length > 1){
			var arr = [];
			for (var i = 0; i < arguments.length; i++)
				arr[i] = arguments[i];
			this.i2c.write(undefined, arr);
		}
		else{
			this.i2c.write(undefined, arguments[0]);
		} 	
	};
	writeByteDataSMB(reg, val) {
		this.i2c.write(reg, val, 1);
	};
	writeWordDataSMB(reg, val) {
		this.i2c.write(reg, [val >> 8, val]);
	};
	writeQuickSMB(val) {
		this.writeByte(val);
	};
	processCallSMB(reg, val) {
		this.writeWordDataSMB(reg, val);
		return this.readWordDataSMB(reg);
	};
	close() {
		if (this.i2c) {
			this.i2c.close();
			delete this.i2c;
		}
		GPIOPin.close(this.clock);
		GPIOPin.close(this.sda);
	};
};
