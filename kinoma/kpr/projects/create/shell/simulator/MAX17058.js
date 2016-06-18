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
    microUSBCharger: {type: "Digital", direction: "input", pin: 1027},
	miniPower: {type: "Digital", direction:"input", pin: 1018},
	miniCharger: {type: "Digital", direction:"input", pin: 1024}
}

exports.read = function() {
    return {
        vcell: 123456,
        soc: 24150,
        batteryLevel: 0.42,
        chargeState: 3,
        microUSBCharger: true,
		miniUSBCharger: false,
    }
}

exports.chargerDetect = function() {
    return 0;
}

exports.chargerDetectMini = function() {
	return 0;
}
