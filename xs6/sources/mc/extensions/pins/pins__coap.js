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
import {Server} from "coap";
import Pins from "pins";
import GPIOPin from "pinmux";
import System from "system";
import LED from "board_led";

function parseQuery(session) {
	const queries = {};
	for (let query of session.queries) {
		let parts = query.split('=');
		if (parts.length == 1) {
			queries[query] = true;
		} else {
			queries[parts[0]] = parts.slice(1).join('=');
		}
	}
	return queries;
}

export default {
	init(port, configuration) {
		this.repeats = [];

		this.coap = new Server();
		this.coap.configuration = configuration;

		this.coap.bind('/invoke', session => {
			let requestObject;
			if (session.payload) {
				requestObject = JSON.parse(String.fromArrayBuffer(session.payload));
			}

			session.autoAck = false;

			Pins.invoke(parseQuery(session).path, requestObject, result => {
				const response = session.createResponse();
				if (result !== undefined) {
					if (result instanceof ArrayBuffer)
						response.setPayload(result, "application/octet-stream");
					else
						response.setPayload(JSON.stringify(result), "application/json");
				}
				session.send(response);
			});
		});

		this.coap.bind('/repeat', session => {
			var observe;
			session.options.some(function(value) {
				if ("Observe" == value[0]) {
					observe = value[1];
					return true;
				}
			});

			var response = session.createResponse();
			if (session.observe) {			// start repeat
				session.acceptObserve();
				response.setCode(2, 5);
				session.send(response);
			}
			else if (session.observeDeregister) {		// end repeat
				this.repeats.some(function(item, index) {
					if ((item.path !== session.path) || (item.query !== session.query) || (item.remoteIP != session.remoteIP) || (item.remotePort != session.remotePort)) return false;
					this.repeats.splice(index, 1);
					item.close();
					item.session.closed = true;
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

			var query = parseQuery(session);
			var repeat = Pins.repeat(query.path, ("interval" in query) ? parseInt(query.interval) : query.timer, function(result) {
				if (session.closed) return;		//@@ observe somehow terminated. should remove from list.

				var response = session.createResponse();
				response.confirmable = false;
				response.setCode(2, 5);
				if (result !== undefined) {
					if (result instanceof ArrayBuffer)
						response.setPayload(result, "application/octet-stream");
					else
						response.setPayload(JSON.stringify(result), "application/json");
				}

				session.send(response);
			});
			repeat.session = session;
			repeat.path = session.path;
			repeat.query = session.query;
			repeat.remoteIP = session.remoteIP;
			repeat.remotePort = session.remotePort;
			this.repeats.push(repeat);
		});

		this.coap.bind('/hello', session => {
			session.send(session.createResponse('WORLD'));
		});

		this.coap.start(port);

		this.led = new LED({onColor: [0, 1, 1], offColor: [1, 0, 1]});
		this.led.run();		// ready to receive a request
	},

	close() {
		this.coap.stop(); // ideally we should close the repeats here
		delete this.coap;

		this.led.stop();
		LED.resume();
	}
};
