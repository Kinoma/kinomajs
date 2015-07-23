//@module
//
//     Copyright (C) 2010-2015 Marvell International Ltd.
//     Copyright (C) 2002-2010 Kinoma, Inc.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//

exports.instantiate = function(Pins, settings)
{
	var result = Object.create(localPins);
	result.Pins = Pins;
	result.settings = settings;
	result.configure();
	return result;
}

var localPins = {
	Pins: null,
	settings: null,
	configure: function() {
	},
	invoke: function(path, requestObject, callback) {
		this.Pins.invoke(null, path, requestObject, callback);
	},
	repeat: function(path, requestObject, condition, callback) {
		this.Pins.repeat(path, requestObject, condition, callback);
	},
	close: function(callback) {
		this.Pins.close(callback);
	}
};
