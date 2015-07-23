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
	var result = Object.create(coapPins);
	result.Pins = Pins;
	result.settings = settings;
	result.configure();
	return result;
}

var coapPins = {
	Pins: null,
	settings: null,
	coap: null,
	id: (Math.random() * 10000) | 0,
	configure: function() {
		this.coap = new CoAP.Client;
	},
	invoke: function(path, requestObject, callback) {
		if (typeof requestObject == "function") {
			callback = requestObject;
			requestObject = undefined;
		}

		var request = this.coap.createRequest(this.settings.url + "/invoke?path=" + path);
		request.confirmable = true;
		request.onResponse = function(response) {
			var contentFormat = response.contentFormat.split(';')[0];
			var result = response.payload;		//@@ test with binary payload
			if ("application/json" == contentFormat)
				result = JSON.parse(result);
			if (callback)
				callback.call(null, result);
		};
		this.coap.send(request);
	},
	repeat: function(path, requestObject, condition, callback) {		//@@ remove requestObject?
		var coap = this.coap;

		if (typeof condition == "function") {
			callback = condition;
			condition = requestObject;
			requestObject = undefined;
		}

		condition = (typeof condition == "number") ? ("interval=" + condition) : ("timer=" + condition);

		var url = this.settings.url + "/repeat?path=" + path + "&" + condition;
		var request = coap.createRequest(url);
		request.token = this.id++;
		request.observe = true;
		request.confirmable = true;
		request.onResponse = function(response) {
			var observe;

			response.options.some(function(value) {
				if ("Observe" == value[0]) {
					observe = value[1];
					return true;
				}
			});

			if (undefined === observe)
				throw new Error;
			
			if (2 === observe)
				return; // observe request received
			
			var contentFormat = response.contentFormat.split(';')[0];
			var result = response.payload;		//@@ test with binary payload
			if ("application/json" == contentFormat)
				result = JSON.parse(result);
			if (callback)
				callback.call(null, result);
		}
		coap.send(request);
		
		return {
			close: function() {
				var request = coap.createRequest(url);
				request.confirmable = true;
				request.addOption("Observe", 1);
				coap.send(request);
			}
		}
	},
	close: function(callback) {
		// cancel any repeats
		this.coap = null;
	}
};
