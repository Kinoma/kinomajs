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
	serial: {type: "Serial"}
}

exports.configure = function()
{
	this.serial.init();
}

exports.close = function()
{
	this.serial.close();
}

exports.read = function(param)
{
	if (typeof param == "string")
		return this.serial.read(param);

	if (!("maximumBytes" in param))
		throw new Error("maximumBytes not specified");

	if ("msToWait" in param)
		return this.serial.read(param.type, param.maximumBytes, param.msToWait);

	return this.serial.read(param.type, param.maximumBytes)
}

exports.write = function(param)
{
	this.serial.write(param);
}
