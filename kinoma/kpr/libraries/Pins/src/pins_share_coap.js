//@module
/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
	configure: function() {
		var coap = this.coap = new CoAP.Server();
		coap.Pins = this.Pins;
		coap.repeats = [];

		coap.bind("/invoke", function(session) {
			var response = session.createResponse();
			var requestObject = response.payload ? JSON.parse(response.payload) : undefined; //@@ binary request
			session.autoAck = false;
			coap.Pins.invoke(parseQuery(session.query).path, requestObject, function(result) {
				response.setCode(2, 5);
				if (result instanceof ArrayBuffer)
					response.setPayload(result, "application/octet-stream");
				else
					response.setPayload(JSON.stringify(result), "application/json");
				session.send(response);
			});
		});

		coap.bind("/repeat", function(session) {
			var observe;
			session.options.some(function(value) {
				if ("Observe" == value[0]) {
					observe = value[1];
					return true;
				}
			});

			var response = session.createResponse();
			if (0 === observe) {			// start repeat
				session.acceptObserve();
				response.setCode(2, 5);
				session.send(response);
			}
			else if (1 === observe) {		// end repeat
				coap.repeats.some(function(item, index) {
					if ((item.path !== session.path) || (item.query !== session.query) || (item.remoteIP != session.remoteIP) || (item.remotePort != session.remotePort)) return false;
					coap.repeats.splice(index, 1);
					item.close();
					return true;
				});

				response.setCode(2, 5);
				session.send(response);
				return;
			}
			else {	// unknown
				response.setCode(4, 4);		//@@
				session.send(response);
				return;
			}

			var query = parseQuery(session.query);
			var repeat = coap.Pins.repeat(query.path, ("interval" in query) ? parseInt(query.interval) : query.timer, function(result) {
				var session = coap.getSession(repeat.id);
				if (!session) return;		//@@ observe somehow terminated. should remove from list.
				var response = session.createResponse();
				response.confirmable = false;
				response.setCode(2, 5);
				if (result instanceof ArrayBuffer)
					response.setPayload(result, "application/octet-stream");
				else
					response.setPayload(JSON.stringify(result), "application/json");
				session.send(response);
			});
			repeat.id = session.id;
			repeat.path = session.path;
			repeat.query = session.query;
			repeat.remoteIP = session.remoteIP;
			repeat.remotePort = session.remotePort;
			coap.repeats.push(repeat);
		});

		coap.start(("port" in this.settings) ? settings.port : undefined);
	},
	close: function(callback) {
		coap.repeats.some(function(item) {
			item.close();
		});

		this.coap.stop();
		this.coap = null;
	},
	get port() {
		return this.coap.port;
	},
	get url() {
		return "coap://*:" + this.port;
	}
};
