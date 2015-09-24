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
	var result = Object.create(wsPins);
	result.Pins = Pins;
	result.settings = settings;
	result.configure();
	return result;
}

var wsPins = {
	Pins: null,
	settings: null,
	wss: null,
	open: false,
	configure: function() {
		var wss = this.wss = new WebSocketServer(("port" in this.settings) ? this.settings.port : undefined);
		wss.Pins = this.Pins;
		wss.onlaunch = function() {
			this.repeats = [];
		}
		wss.onconnect = function(ws, options) {
			wss.open = true;

			ws.onopen = function() {
			}
			ws.onclose = function() {
				//@@ needs to check for repeats on this instance only!!
				wss.repeats.every(function(item) {
					item.repeat.close();
				});
				wss.repeats.length = 0;
			}
			ws.onerror = function() {
//@@ report to client that this connection was dropped
				// onclose will be called next, so no clean-up here
			}
			ws.onmessage = function(e) {
				var request = JSON.parse(e.data);
				if ("repeat" in request) {
					if (request.repeat) {
						var repeat = wss.Pins.repeat(request.path, ("interval" in request) ? parseInt(request.interval) : request.timer, function(result) {
							if ((typeof result === "object") && (result instanceof Chunk)) {
								ws.send(JSON.stringify({inReplyTo: request.id, binary: true}));
								ws.send(result);
							}
							else
								ws.send(JSON.stringify({inReplyTo: request.id, body: result}));
						});
						wss.repeats.push({repeat: repeat, request: request});
					}
					else {
						wss.repeats.every(function(item, index) {
							if (item.request.id !== request.id) return true;		//@@ not enough to compare request.id - need something connection specific.
							item.repeat.close();
							wss.repeats.splice(index, 1);
						});
					}
				}
				else {
					wss.Pins.invoke(request.path, ("requestObject" in request) ? request.requestObject : undefined, function(result) {
						if ((typeof result === "object") && (result instanceof Chunk)) {
							ws.send(JSON.stringify({inReplyTo: request.id, binary: true}));
							ws.send(result);
						}
						else
							ws.send(JSON.stringify({inReplyTo: request.id, body: result}));
					});
				}
			}
		}
	},
	close: function(callback) {
		wss.open = false;		//@@ close all connections. cancel repeats.
	},
	get port() {
		return this.wss.port;
	},
	get url() {
		return "ws://*:" + this.wss.port + "/";
	}
};
