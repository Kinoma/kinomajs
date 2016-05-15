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
		ssp: {type: "SPI"},
	},
	configure(...args) {
		return this.ssp.init(...args);
	},
	activate(){
		return this.ssp.activate();
	},
	deactivate(){
		return this.ssp.deactivate();
	},
	readByte(...args) {
		return this.ssp.readByte(...args);
	},
	readBlock(...args) {
		return this.ssp.read(...args);
	},
	readWord(...args) {
		return this.ssp.readWord(...args);
	},
	writeByte(...args) {
		return this.ssp.writeByte(...args);
	},
	writeBlock(...args) {
		return this.ssp.writeBlock(...args);
	},
	writeWord(...args) {
		return this.ssp.writeWordDataSMB(...args);
	},
	close(...args) {
		return this.ssp.close(...args);
	},
};
