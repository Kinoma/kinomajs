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

exports.pins = {
	i2c: {type: "I2C"}
}

exports.configure = function()
{
	this.i2c.init();
}

exports.close = function()
{
	this.i2c.close();
}

exports.readByte = function()
{
	return this.i2c.readByte();
}

exports.readBlock = function(param)
{
	return this.i2c.readBlock(param.count, param.format);
}

exports.readByteDataSMB = function(register)
{
	return this.i2c.readByteDataSMB(register);
}

exports.readWordDataSMB = function(register)
{
	return this.i2c.readWordDataSMB(register);
}

exports.readBlockDataSMB = function(param)
{
	return this.i2c.readWordDataSMB(param.register, param.count, param.type);
}

exports.writeByte = function(value)
{
	return this.i2c.writeByte(value);
}

exports.writeBlock = function(param)
{
	this.i2c.writeBlock(param);
}

exports.writeByteDataSMB = function(param)
{
	return this.i2c.writeByteDataSMB(param.register, param.value);
}

exports.writeWordDataSMB = function(param)
{
	return this.i2c.writeWordDataSMB(param.register, param.value);
}

exports.writeBlockDataSMB = function(param)
{
	this.i2c.writeBlockDataSMB(param.register, param.value);
}

exports.writeQuickSMB = function(value)
{
	this.i2c.writeQuickSMB(value);
}

exports.processCallSMB = function(param)
{
	return this.i2c.processCallSMB(param.register, param.value);
}

exports.setAddress = function(address) {
	this.i2c.close();
	this.i2c = PINS.create( { type: "I2C", sda: this.sda, clock: this.clock, address: address } );
	this.i2c.init();
}
