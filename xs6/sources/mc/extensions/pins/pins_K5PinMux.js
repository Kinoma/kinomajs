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

exports.POWER = 1;
exports.GROUND = 2; 

exports.pins = {
	OEdigital: {type: "Digital", direction: "output"},
	STRdigital: {type: "Digital", direction: "output"},
	D14digital: {type: "Digital", direction: "output"},
	D58digital: {type: "Digital", direction: "output"},
	CPdigital: {type: "Digital", direction: "output"},
	
	OEanalog: {type: "Digital", direction: "output"},
	STRanalog: {type: "Digital", direction: "output"},
	D14analog: {type: "Digital", direction: "output"},
	D58analog: {type: "Digital", direction: "output"},
	CPanalog: {type: "Digital", direction: "output"},
}

exports.PowerGroundinit = function(){
	trace("PowerGround: init\n");
	this.OEdigital.init();
	this.STRdigital.init();
	this.D14digital.init();
	this.D58digital.init();
	this.CPdigital.init();
	this.OEanalog.init();
	this.STRanalog.init();
	this.D14analog.init();
	this.D58analog.init();
	this.CPanalog.init();



	//disable output
	this.PowerGroundSetOutput.call(this, 0);
	
	//park clock high
	this.PowerGroundSetClock.call(this, 1);
	
	//strobe down
	this.STRdigital.write(0);
	this.STRanalog.write(0);
}

exports.PowerGroundSetOutput = function(value){
	this.OEdigital.write(value);
	this.OEanalog.write(value);
}

exports.PowerGroundSetClock = function(value){
	this.CPdigital.write(value);
	this.CPanalog.write(value);
}

exports.PowerGroundPulseStrobe = function(){
	this.STRdigital.write(1);
	this.STRanalog.write(1);
	sensorUtils.mdelay(1);
	// System.sleep(1);
	this.STRdigital.write(0);
	this.STRanalog.write(0);
}

exports.PowerGroundset = function(digital14, digital58, analog14, analog58){
	// trace("doing set: \n");
	// trace("digital14: " + digital14.toString(2) + "\n");
	// trace("digital58: " + digital58.toString(2) + "\n");
	// trace("analog14: " + analog14.toString(2) + "\n");
	// trace("analog58: " + analog58.toString(2) + "\n");
	
	//disable output
	this.PowerGroundSetOutput.call(this, 0);	
	
	for (var i = 0; i < 8; i++){
		this.PowerGroundSetClock.call(this, 0);
		
		this.D14digital.write( (digital14 & 0x80) ? 1 : 0 );
		this.D58digital.write( (digital58 & 0x80) ? 1 : 0 );
		this.D14analog.write( (analog14 & 0x80) ? 1 : 0 );
		this.D58analog.write( (analog58 & 0x80) ? 1 : 0 );
		
		this.PowerGroundSetClock.call(this, 1);
		
		digital14 = digital14 << 1;
		digital58 = digital58 << 1;
		analog14 = analog14 << 1;
		analog58 = analog58 << 1;
	}
	
	this.PowerGroundPulseStrobe.call(this);
	
	this.PowerGroundSetOutput.call(this, 1);
}


exports.configure = function(parameters){
	this.PowerGroundinit.call(this);
}



exports.set = function(parameters){
	var d14 = 0x0F;
	var d58 = 0x0F;
	var a14 = 0x0F;
	var a58 = 0x0F;
			
	if ("digital" in parameters){
		for (var i = 0; i < 8; i++){
			var pin = parameters.digital[i];
			
			switch (pin){
				case this.POWER:
					trace("pin " + i + " is power.");
					if (i < 4){
						d14 &= ~(1 << i);
						trace(" d14 is now " + d14.toString(2) + "\n");
					}else{
						d58 &= ~(1 << (i - 4));
						trace(" d58 is now " + d58.toString(2) + "\n");
					}				
					break;
				case this.GROUND:
					if (i < 4){
						d14 |= (1 << (i + 4));
					}else{
						d58 |= (1 << i);
					}				
					break;			
			}
		}	
	}	
	
	if ("analog" in parameters){
		for (var i = 0; i < 8; i++){
			// pin number on case is different from the board. 
			// here we remap the user's configuration to real pin numbering on board
			// 1 2 3 4 5 6 7 8  (case numbering)
			// 8 7 6 5 4 3 2 1	(board numbering)
			var pin = parameters.analog[7-i];
			
			switch (pin){
				case exports.POWER:
					if (i < 4){
						a14 &= ~(1 << i);
					}else{
						a58 &= ~(1 << (i - 4));
					}				
					break;
				case exports.GROUND:
					if (i < 4){
						a14 |= (1 << (i + 4));
					}else{
						a58 |= (1 << i);
					}				
					break;			
			}
		}
	}
	
	this.PowerGroundset.call(this, d14, d58, a14, a58);

	/*  Need some delay for power and ground to settle and become stable.
		This help resolve the issue that serial camara VC0706 does not work on K5.
		Tried differnent values such as 500ms, 200ms, 250ms and 300ms. 
		300ms is the minimum to make the serial camera work. 
	*/
	if ('delay' in parameters)
		sensorUtils.mdelay(parameters.delay); // JIG should specify its delay here
	else
		sensorUtils.mdelay(300); // if delay is not specified, default is 300 ms to be sensor safe
}

exports.close = function(){
	trace("PowerGround: close\n");
	this.OEdigital.close();
	this.STRdigital.close();
	this.D14digital.close();
	this.D58digital.close();
	this.CPdigital.close();
	this.OEanalog.close();
	this.STRanalog.close();
	this.D14analog.close();
	this.D58analog.close();
	this.CPanalog.close();
}
