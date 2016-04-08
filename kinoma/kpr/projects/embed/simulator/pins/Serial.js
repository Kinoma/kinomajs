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
export default {
	pins: {
		serial: {type: "Serial"},
	},
	configure(...args) {
		return this.serial.init(...args);
	},
	read(o) {
		if (typeof o == "string")
			return this.serial.read(String);
		else
			return this.serial.read(o.type, o.maximumBytes, o.msToWait);
	},
	write(o) {
		return this.serial.write(o);	// string??
	},
	close(...args) {
		return this.serial.close(...args);
	},
};
