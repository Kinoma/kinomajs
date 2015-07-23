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

var gRepeats = [];

var id = (Math.random() * 4096) | 0;
var gHTTPServer;

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
	configure: function() {
		this.id = ++id;
	},
	invoke: function(path, requestObject, callback) {
		var Pins = this.Pins;

		var container = new Container({behavior: Behavior({
				onComplete: function(handler, message, result) {
					Pins.forget(container);
					if (callback && (200 == message.status)) {
						if ("application/json" == message.getResponseHeader("Content-Type"))
							result = JSON.parse(result);
						callback.call(null, result);
					}
				}
			})});
		Pins.remember(container);

		if (typeof requestObject == "function") {
			callback = requestObject;
			requestObject = undefined;
		}

		var message = new Message(makePath(this.settings, path));
		if (requestObject) {
			message.requestText = JSON.stringify(requestObject);
			message.setRequestHeader("Content-Type", "application/json");
			message.method = "PUT";
		}
		container.invoke(message, Message.CHUNK);
	},
	repeat: function(path, requestObject, condition, callback) {		//@@ remove requestObject?
		var container = new Container({behavior: Behavior({})});
		this.Pins.remember(container);

		if (typeof condition == "function") {
			callback = condition;
			condition = requestObject;
			requestObject = undefined;
		}

		var callbackPath = "/pins/callback/" + this.id + "/" + ((Math.random() * 100000) | 0);
		addRepeat(callbackPath, callback)

		condition = (typeof condition == "number") ? ("interval=" + condition) : ("timer=" + condition);
		var httpPath = makePath(this.settings, path, condition + "&callback=" + encodeURIComponent("http://*:" + gHTTPServer.port + callbackPath));
		var message = new Message(httpPath + "&repeat=on")
		container.invoke(message);

		return {
			close: function() {
				removeRepeat(callbackPath);
				container.invoke(new Message(httpPath + "&repeat=off"));
			}
		}
	},
	close: function() {
		var root = "/pins/callback/" + this.id + "/";
		gRepeats.some(function(repeat, index) {
			if (0 == repeat.path.indexOf(root)) {
				removeRepeat(repeat.path);
//@@ call server to turn off repeats too
			}
		});
	}
};

function makePath(settings, path, query)
{
	var url = settings.url;
	url += "?path=" + encodeURIComponent(path);
	if (query)
		url += "&" + query;
	return url;
}

function addRepeat(path, callback)
{
	gRepeats.push({path: path, callback: callback});
}

function removeRepeat(path)
{
	gRepeats.some(function(repeat, index) {
		if (repeat.path != path) return false;
		gRepeats.splice(index, 1);
		return true;
	});
}

var CallbackServerBehavior = Behavior.template({
	onInvoke: function(handler, message) {
		var path = message.path;
		gRepeats.some(function(repeat) {
			if (repeat.path != path) return false;
			repeat.callback.call(null, ("application/json" == message.getRequestHeader("Content-Type")) ? JSON.parse(message.requestText) : message.requestChunk);
			return true;
		}, this);
	}
});

gHTTPServer = new HTTP.Server({id: "PinConnect"});
gHTTPServer.behavior = new CallbackServerBehavior;
gHTTPServer.start();



