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
export default class I2C @ "xs_i2c_destructor" {
	constructor(port, addr, fast) @ "xs_i2c_constructor";
	close() @ "xs_i2c_close";
	read(reg, n) @ "xs_i2c_read";
	readChar(reg) @ "xs_i2c_readChar";
	readWord(reg, n, lsbFirst) @ "xs_i2c_readWord";
	write(reg, data, n) @ "xs_i2c_write";
	setTimeout(txtout, rxtout) @ "xs_i2c_setTimeout";
	setClock(high, low) @ "xs_i2c_setClock";
};
