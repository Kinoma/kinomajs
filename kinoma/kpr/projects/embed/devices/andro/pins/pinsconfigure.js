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

exports.configure = function() {
	trace("pinsconfigure: configure\n");
}
exports.close = function() {
	trace("pinsconfigure: close\n");
}
exports.get = function() {
	trace("pinsconfigure: get\n");

    var result = {
        leftVoltage: 3.3,
        rightVoltage: 3.3,
        leftPins: [],
        rightPins: [],
        back: new Array(50)
    };

    for (var i = 4; i < 12; i++)
        result.leftPins[11 - i] = 0;

    for (var i = 4; i < 12; i++)
        result.rightPins[11 - i] = 0;

    return result;
}
exports.set = function(parameters) {
	trace("pinsconfigure: set\n");
}

exports.hibernate = function() {
	trace("pinsconfigure: hibernate\n");
}

exports.wake = function() {
	trace("pinsconfigure: wake\n");
}
