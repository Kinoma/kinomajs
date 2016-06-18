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
import System from "system";

export default class SPI @ "xs_ssp_destructor" {
	constructor(port, cs, freq) @ "xs_ssp_constructor";
	_close() @ "xs_ssp_close";
	read(reg, n) @ "xs_ssp_read";
	readChar(reg)@"xs_ssp_readChar";
	readWord(reg, lsbFirst) @ "xs_ssp_readWord";
	write(reg, data, dma, n, offset, duplex) @ "xs_ssp_write";
	activate()@"xs_ssp_cs_activate";
	deactivate()@"xs_ssp_cs_deactivate";

	close() {
		if (this.timer) {
			clearInterval(this.timer);
			delete this.timer;
		}
		this._close();
	};
};
