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

/*
	to consider: allow app parameter passed to discover to be an array of app signatures
*/

var Pins = require("pins");

var onFound, onLost;
var devices = [];
var ssdpClient;
var appSignature;
var deviceContainer = new Container;
var appCheckContainer = new Container;

deviceContainer.behavior = Behavior({
	onComplete: function (handler, message, document) {
		var device = {
			url: message.getResponseHeader("Application-URL") + appSignature,
			name: document.getElementsByTagName("friendlyName").item(0).firstChild.value,
			id: document.getElementsByTagName("UDN").item(0).firstChild.value,
			additionalData: {}
		}
		var element = document.getElementsByTagName("icon").item(0);
		if (element)
			device.iconURL = mergeURI(message.url, element.getElement("url"));
		devices.push(device);
		appCheckContainer.invoke(new Message(device.url), Message.DOM);
	}
});

appCheckContainer.behavior = Behavior({
	onComplete: function (handler, message, document) {
		var url = message.url, index;
		devices.some(function(item, i) {
			if (item.url !== url) return;
			index = i;
			return true;
		});
		if (200 == message.status) {
			devices[index].additionalData = additionalData(document);
			onFound.call(null, devices[index]);
		}
		else
			devices.splice(index, 1);
	}
});

exports.discover = function(app, found, lost)
{
	if (!app) {
		if (ssdpClient) ssdpClient.stop();
		onFound = undefined;
		onLost = undefined;
		ssdp = undefined;
		devices = [];
		appSignature = undefined;
		return;
	}

	appSignature = app;
	onFound = found;
	onLost = lost;
	
	ssdpClient = new SSDP.Client;
	ssdpClient.addService("urn:dial-multiscreen-org:service:dial:1");
	ssdpClient.behavior = Behavior({
		onSSDPServerUp: function(server) {
			deviceContainer.invoke(new Message(server.url), Message.DOM);
		},
		onSSDPServerDown: function(server) {
			devices.some(function(item, i) {
				if (item.url !== server.url) return;
debugger
				if (onLost) onLost.call(null, item);
				devices.splice(i, 1);
				return true;
			});
		}
	});
	ssdpClient.start();
}

exports.launch = function(device, query, callback) {
	if (typeof query == "function") {
		callback = query;
		query = undefined;
	}

	var container = new Container;
	container.behavior = Behavior({
		onComplete: function(handler, message, document) {
			switch (state++) {
				case 0:
					if (2 != Math.floor(message.status / 100)) {
						Pins.forget(container);
						if (callback) callback.call(null, message.status, device);
					}
					else
						container.invoke(new Message(device.url), Message.DOM);
					break;
				case 1:
					Pins.forget(container);
					device.additionalData = additionalData(document);
					if (callback) callback.call(null, message.status, device);
					break;
				}
		}
	});
	Pins.remember(container);
	var message = new Message(device.url);
	message.method = "POST";
	if (query)
		message.requestText = serializeQuery(query);
	state = 0;
	container.invoke(message, Message.TEXT);
}


function additionalData(document)
{
	var element = document.getElementsByTagName("additionalData").item(0);
	var additionalData = {};
	if (element) {
		try {
			for (var i = 0, children = element.children; children && (i < children.length); i++)
				additionalData[children[i].tagName] = children[i].firstChild.value;
		}
		catch (e) {
		}
	}
	return additionalData;
}




