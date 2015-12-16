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
	var result = Object.create(httpPins);
	result.Pins = Pins;
	result.settings = settings;
	result.configure();
	return result;
}

var httpPins = {
	Pins: null,
	settings: null,
	httpServer: null,
	configure: function() {
		this.httpServer = new HTTP.Server({id: "PinShared", port: ("port" in this.settings) ? this.settings.port : undefined});
		this.httpServer.behavior = new ServerBehavior;
		this.httpServer.behavior.Pins = this.Pins;
		this.httpServer.behavior.repeats = [];
		this.httpServer.start();
	},
	close: function(callback) {
		this.httpServer.stop();
		delete this.httpServer;
	},
	get port() {
		return this.httpServer.port;
	},
	get url() {
		return "http://*:" + this.httpServer.port + "/";
	}
};

var ServerBehavior = Behavior.template({
	onComplete: function(handler, message, result) {
		handler.message.status = message.status;
		if (undefined !== result) {
		if ((typeof result == "object") && (result instanceof Chunk)) {
			handler.message.setResponseHeader("Content-Type", "application/octet-stream");
			handler.message.responseChunk = result;
		}
		else {
			handler.message.setResponseHeader("Content-Type", "application/json");
			handler.message.responseText = JSON.stringify(result);
		}
		}
	},
	onInvoke: function(handler, message) {
		var query = parseQuery(message.query);
		if ("repeat" in query) {		//@@ security hole - check that callback is http (otherwise callback accesses xkpr: etc)
			if ("on" == query.repeat) {
				query.cb = query.callback.replace("*", message.remoteIP);
				query.repeat = this.Pins.repeat(query.path, ("interval" in query) ? parseInt(query.interval) : query.timer, function(result) {
					var message = new Message(query.cb);
					if ((typeof result == "object") && (result instanceof Chunk)) {
						message.setRequestHeader("Content-Type", "application/octet-stream");
						message.requestChunk = result;
					}
					else {
						message.setRequestHeader("Content-Type", "application/json");
						message.requestText = JSON.stringify(result);
					}
					handler.invoke(message);
				});
				this.repeats.push(query);
			}
			else {
				this.repeats.every(function(item, index) {
					if ((item.path !== query.path) || (item.callback !== query.callback)) return true;
					item.repeat.close();
					this.repeats.splice(index, 1);
				}, this);
			}
		}
		else {
			if (!("path" in query))
				message.status = 500;
			else {
				var body = message.requestText;
				handler.invoke(this.Pins.toMessage(query.path, body ? JSON.parse(body) : undefined), Message.JSON);
			}
		}
	},
});
