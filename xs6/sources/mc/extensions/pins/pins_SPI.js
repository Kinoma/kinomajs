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
import GPIOPin from "pinmux";
import pinmap from "map";

export default class SPI {
	constructor(pinconf) {
		// we are interested in only "txd", "rxd" and "baud"
		this.tx = pinmap(pinconf.tx);
		this.rx = pinmap(pinconf.rx);
		this.clk = pinmap(pinconf.clk);
		this.cpha = this.cpol = 0;
		this.cs = this.freq = -1;
		if("cs" in pinconf)
			this.cs = pinmap(pinconf.cs);
		
		if("freq" in pinconf)
			this.freq = pinconf.freq;
		

		if("cpha" in pinconf)
			this.cpha = pinconf.cpha;
		

		if("cpol" in pinconf)
			this.cpol = pinconf.cpol;
	};
	init() {
		this.ssp = GPIOPin.ssp({tx: this.tx, rx: this.rx, clk: this.clk, cs: this.cs, freq: this.freq, cpha: this.cpha, cpol: this.cpol});
	};
	activate(){
		return this.ssp.activate();
	};
	deactivate(){
		return this.ssp.deactivate();
	};
	readByte(reg){
		return this.ssp.readChar(reg);
	};
	readWord(reg){
		return this.ssp.readWord(reg);
	};
	readBlock(reg, size, type) {
		return this.ssp.read(reg, size);
	};
	writeByte(reg, val){
		// no DMA
		return this.ssp.write(reg, val, 0, 1);
	};
	writeWord(reg, val){
		// no DMA
		return this.ssp.write(reg, val, 0, 2);
	};
	writeBlock(reg, val, dma, size) {
		return this.ssp.write(reg, val, dma, size);
		
	};
	close() {
		if(this.ssp){
			this.ssp.close();
			delete this.ssp;
		}
		GPIOPin.close(this.tx);
		GPIOPin.close(this.rx);
	};
};

