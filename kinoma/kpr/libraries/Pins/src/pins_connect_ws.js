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

exports.instantiate = function(Pins, settings, description, callback)
{
	var result = Object.create(wsPins);
	result.Pins = Pins;
	result.settings = settings;
	result.configure();
	result.callback = callback;
	return result;
}

var wsPins = {
	Pins: null,
	settings: null,
	ws: null,
	open: false,
	sendQueue: null,
	id: 0,
	configure: function() {
		this.ws = new WebSocket(this.settings.url);
		this.ws.connection = this;
		this.sendQueue = [];
		this.id = (Math.random() * 10000000) | 0;
		this.ws.onopen = function() {
			this.connection.open = true;
			this.waiting = false;
			this.connection.sendQueue.forEach(function(item) {
				this.send(JSON.stringify(item.message));
			}, this);
		}
		this.ws.onclose = function() {
		//	debugger
			var connection = this.connection;
			connection.open = false;
			if (connection.callback)
				connection.callback.call(null, "closed", connection);
		}
		this.ws.onerror = function() {
		//	debugger
			var connection = this.connection;
			if (connection.callback)
				connection.callback.call(null, "error", connection);
		}
		this.ws.onmessage = function(e) {
			if (this.waiting) {
				if (this.waiting.callback)
					this.waiting.callback.call(null, e.data);
				this.waiting = false;
				return;
			}
			var message = JSON.parse(e.data);
			for (var i = 0; i < this.connection.sendQueue.length; i++) {
				var item = this.connection.sendQueue[i];
				if (item.message.id != message.inReplyTo)
					continue;
				if (!("repeat" in item.message))
					this.connection.sendQueue.splice(i, 1);
				if (("binary" in message) && message.binary) {
					this.waiting = item;
					return;
				}
				if (item.callback)
					item.callback.call(null, ("body" in message) ? message.body : undefined);
				return;
			}
			trace("Unhandled pins_connect_websocket message " + JSON.stringify(messsage) + "\n");
		}
	},
	invoke: function(path, requestObject, callback) {
		if (typeof requestObject == "function") {
			callback = requestObject;
			requestObject = undefined;
		}

		var item = {callback: callback, message: {path: path, requestObject: requestObject, id: this.id++}};
		this.sendQueue.push(item);
		if (this.open)
			this.ws.send(JSON.stringify(item.message));
	},
	repeat: function(path, requestObject, condition, callback) {		//@@ remove requestObject?
		if (typeof condition == "function") {
			callback = condition;
			condition = requestObject;
			requestObject = undefined;
		}

		var item = {callback: callback, message: {path: path, requestObject: requestObject, repeat: true, id: this.id++}};
		if (typeof condition == "number")
			item.message.interval = condition;
		else
			item.message.timer = condition;
		this.sendQueue.push(item);
		if (this.open)
			this.ws.send(JSON.stringify(item.message));

		return {
			ws: this.ws,
			close: function() {
				item.message.repeat = false;
				this.ws.send(JSON.stringify(item.message));
			}
		};
	},
	close: function(callback) {
		// cancel any repeats
	}
};
