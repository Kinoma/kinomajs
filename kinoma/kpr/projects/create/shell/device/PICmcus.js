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
    leftPIC: {type: "I2C", bus: 0, address: 0x20},
    rightPIC: {type: "I2C", bus: 0, address: 0x21},
}

exports.configure = function() {
	this.leftPIC.init();
	this.rightPIC.init();
}

exports.close = function() {
    this.leftPIC.close();
    this.rightPIC.close();
}

exports.getVersion = function() {
	var left = -1;
	var right = -1;
	try{
		left = this.leftPIC.readByteDataSMB(0x01);	    
	}catch (e) {
		left = -1;		
	}
	
	try{
		right = this.rightPIC.readByteDataSMB(0x01);
	}catch (e){
		right = -1;
	}
	
    return {left: left, right: right, inSimulator: false};	
}
