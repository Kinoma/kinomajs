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

// Pins should be a constructor... or at least contain some:
//		Local & remote pins (different URL scheme)
//
// Ability to specify referrer (allow pins in multiple threads - obscure, but useful)
//
//
// Share over multiple outputs:
//		web page
//		HTTP
//		WebSockets
//		CoAP
//

exports.disconnected = 0;
exports.power3_3V = 1;
exports.ground = 2;
exports.analogIn = 3;
exports.digitalIn = 4;
exports.digitalOut = 5;
exports.i2cClock = 6;
exports.i2cData = 7;
exports.serialRx = 8;
exports.serialTx = 9;
exports.pwm = 10;
exports.power5V = 11;

exports.digitalUnconfigured = 128;

var requests = [];
var repeatID = (Math.random() * 5000) | 0;

exports.remember = function(request)
{
	requests.push(request);
}
exports.forget = function(request)
{
	requests.splice(requests.indexOf(requests), 1);
}

exports.configure = function(configuration, mux, callback)
{
	if (typeof mux == "function") {
		callback = mux;
		mux = undefined;
	}

	if (mux && configuration)
		throw new Error("Can only configure pin mix or BLL configuration, not both");

	var container = new Container({behavior: Behavior({
		onComplete: function(handler, message, result) {
			switch (this.state) {
				case 0:
					container.invoke(new MessageWithObject("pins:preconfigure", configuration), Message.JSON);
					break;
				case 1:
					this.configuration = result;
					container.invoke(exports.toMessage("getPinMux"), Message.JSON);
					break;
				case 2:
					if (!mux)
						mux = configurationToMux(this.configuration);
					var current = result;
					if (equalPins(current.leftPins, mux.leftPins) &&
						equalPins(current.rightPins, mux.rightPins) &&
						(normalizeVoltage(current.leftVoltage) == mux.leftVoltage) &&
						(normalizeVoltage(current.rightVoltage) == mux.rightVoltage)) {
	
						if (configuration)
							container.invoke(new MessageWithObject("pins:configure", configuration), Message.JSON);
						else {
							exports.forget(container);
							callback.call(null, true);
						}
						break;
					}

					var random = (Math.random() * 100000) | 0;
					var nameOK = "/pins/temp/ok_" + random;
					var nameCancel = "/pins/temp/cancel_" + random;

					Handler.bind(nameOK, Behavior({
						onInvoke: function(handler, message) {
							Handler.remove(Handler.get(nameOK));
							Handler.remove(Handler.get(nameCancel));
							exports.forget(container);
							if (configuration)
								container.invoke(new MessageWithObject("pins:configure", configuration), Message.JSON);
							else
								container.invoke(exports.toMessage("setPinMux", mux), Message.JSON);
						}
					}));
					Handler.bind(nameCancel, Behavior({
						onInvoke: function(handler, message) {
							Handler.remove(Handler.get(nameOK));
							Handler.remove(Handler.get(nameCancel));
							exports.forget(container);
							callback.call(null, false);
						}
					}));

					// have shell show settings dialog to re-mux
					container.invoke(new MessageWithObject("xkpr://shell/settings/pinmux/dialog?ok=" + nameOK + "&cancel=" + nameCancel, mux));
					break;
				case 3:
					exports.forget(container);
					callback.call(null, true);
					break;
			}
			this.state += 1;
		}
	})});

	exports.remember(container);
	container.behavior.state = mux ? 1 : 0;
	container.behavior.onComplete();
}

exports.invoke = function(path, requestObject, callback)
{
	var container = new Container({behavior: Behavior({
		onComplete: function(handler, message, result) {
			exports.forget(container);
			if (callback)
				callback.call(null, result);
		}
	})});
	exports.remember(container);

	if (typeof requestObject == "function") {
		callback = requestObject;
		requestObject = undefined;
	}

	container.invoke(this.toMessage(path, requestObject), Message.JSON);
}

exports.repeat = function(path, requestObject, condition, callback)		//@@ remove requestObject
{
	var container = new Container({behavior: Behavior({})});
	exports.remember(container);

	if (typeof condition == "function") {
		callback = condition;
		condition = requestObject;
		requestObject = undefined;
	}

	var id = ++repeatID;
	var name = "/pins/temp/repeat_" + id;
	Handler.bind(name, Behavior({
		onInvoke: function(handler, message) {
			callback.call(null, message.requestObject);
		}
	}));

	condition = (typeof condition == "number") ? ("interval=" + condition) : ("timer=" + condition);
	container.invoke(new MessageWithObject("pins:" + path + "?repeat=on&id=" + id + "&" + condition + "&callback=" + name, requestObject));

	return {
		close: function() {
			container.invoke(new MessageWithObject("pins:" + path + "?repeat=off&id=" + id + "&" + condition + "&callback=" + name));
			exports.forget(container);
			Handler.remove(Handler.get(name));
		}
	};
}

exports.when = function(target, path, callback)
{
	return this.repeat("/" + target + "/_WHEN_", path, callback);
}

exports.close = function() {
	var container = new Container({behavior: Behavior({})});
	container.invoke(new MessageWithObject("pins:close"));
}


exports.connect = function(connectionDescription, settings)
{
	if (typeof connectionDescription == "string") {
		if (settings) {
			var protocol = connectionDescription;
		} else {
			var url = connectionDescription;
			settings = {url: url};
			var protocol = url.substring(0, url.indexOf(":"));	
		}
	} else {
		var url = connectionDescription.connections[0];
		var protocol = url.substring(0, url.indexOf(":"));		//@@ choose preferred protocol!	
		settings = connectionDescription;
		settings.url = url;
	}
	return require("pins_connect_" + protocol).instantiate(exports, settings);
}

var shared;
var zeroconfShare;

exports.share = function(shares, advertise)
{
	if (!advertise) advertise = {};
	if (!("uuid" in advertise)) advertise.uuid = application.uuid;

	if (!(shares instanceof Array))
		shares = shares ? [shares] : [];

	if (shared && shared.length) {
		shared = [];
		if (zeroconfShare) {
			zeroconfShare.stop();
			zeroconfShare = undefined;
		}
	}

	for (var i = 0; i < shares.length; i++) {
		var share = shares[i];
		if (typeof share == "string")
			share = shares[i] = {type: share};
		var module = require("pins_share_" + share.type);
		share.instance = module.instantiate(exports, share);
	}

	if (("zeroconf" in advertise) && advertise.zeroconf) {
		exports.invoke("configuration", function(configuration) {
			var bll = [];
			for (var c in configuration) {
				if (-1 == bll.indexOf(configuration[c].require))
					bll.push(configuration[c].require);
			}
			var txt = {bll: bll.join(","), uuid: advertise.uuid};
			for (var i = 0; i < shares.length; i++) {
				var share = shares[i];
				if (!("url" in share.instance)) continue
				txt["_" + share.type] = share.instance.url;
			}

//@@ port is random below. can it be 0?
		    try {
		        zeroconfShare = new Zeroconf.Advertisement("_kinoma_pins._tcp.", ("name" in advertise) ? advertise.name : "Shared Pins", 8080, txt);
		        zeroconfShare.behavior = new Behavior;
		        zeroconfShare.start();
		    }
		    catch (e) {
		        trace("Zeroconf mdns responder not found. Please see http://www.kinoma.com/0conf for troubleshooting information.\n");
		    }
		});
	}

	shared = shares;
	return shares;
}

exports.discover = function(bllNames, onFound, onLost)
{
	if (typeof bllNames == "function") {
		onLost = onFound;
		onFound = bllNames;
		bllNames = undefined;
	} else if (typeof bllNames == "string") {
		bllNames = [bllNames];
	}
	this.services = [];
	this.zeroconfBrowse = new Zeroconf.Browser("_kinoma_pins._tcp.");
	this.zeroconfBrowse.behavior = Behavior(zeroconfBrowse);
	this.zeroconfBrowse.behavior.discover = this;
	this.bllNames = bllNames;
	this.onFound = onFound ? onFound : function() {};
	this.onLost = onLost ? onLost : function() {};
	this.zeroconfBrowse.start();
	
}

exports.discover.prototype = {
	close: function() {
		this.zeroconfBrowse.stop();
		delete this.services;
	}
}

exports.toMessage = function(path, object) {
	var message;

	switch (path) {
		case "metadata":
			message = new Message("pins:metadata");
			message.setRequestHeader("referrer", "xkpr://" + (application ? application.id : shell.id));
			break;

		case "getPinMux":
			message = new Message("pins:/pinmux/get");
			message.setRequestHeader("referrer", "xkpr://shell");
			break;

		case "setPinMux":
			message = new Message("pins:/pinmux/set");
			message.requestObject = object;
			message.setRequestHeader("referrer", "xkpr://shell");
			break;
		
		default:
			message = new MessageWithObject("pins:" + path, object);
			break;
	}

	return message;
}

zeroconfBrowse = {
/*
	this funciton isn't correct. it really wants to know the IP address of the lost service... since a service can exist at more than one. Alain?
*/
	onZeroconfServiceDown: function(service) {
		var i = findConnectionDescription(this.discover.services, undefined, service.name);
		if (undefined === i) return;

		var connectionDescription = this.discover.services[i];
		this.discover.services.splice(i, 1);
		this.discover.onLost.call(this.discover, connectionDescription);
	},
	onZeroconfServiceUp: function(service) {
		try {
			var connectionDescription;
			var i = findConnectionDescription(this.discover.services, service.txt.uuid);
			if (undefined === i) {
				  connectionDescription = {
					name: service.name,
					ip: service.ip,
					id: service.txt.uuid,
					bll: service.txt.bll.split(","),
					connections: []
				}
				this.discover.services.push(connectionDescription);
			}
			else {
				connectionDescription = this.discover.services[i];
			}
			for (var property in service.txt) {
				if (property.charAt(0) != "_") continue;
				var url = service.txt[property].replace("*", service.ip);
				if (-1 == connectionDescription.connections.indexOf(url))
					connectionDescription.connections.push(url);
			}
			if (undefined === i) {
				if (!(undefined === this.discover.bllNames)) {
					for (var i in this.discover.bllNames) {
						if (connectionDescription.bll.indexOf(this.discover.bllNames[i]) > -1) {
							this.discover.onFound.call(this.discover, connectionDescription);
							break;
						}
					}
				} else {
					this.discover.onFound.call(this.discover, connectionDescription);
				}
			}
		}
		catch (e) {
		}
	}
}

function findConnectionDescription(services, uuid, name) {
	for (var i = 0; i < services.length; i++) {
		var connectionDescription = services[i];
		if ((uuid && (services[i].id == uuid)) || (name && (services[i].name == name)))
			return i;
	}
}

function configurationToMux(config)
{
	var mux = {
			leftPins: makeDisconnectedHeader(8),
			rightPins: makeDisconnectedHeader(8),
			back: makeBackHeader(),
			leftVoltage: ("leftVoltage" in config) ? config.leftVoltage : undefined,
			rightVoltage: ("rightVoltage" in config) ? config.rightVoltage : undefined
	};

	for (var bll in config) {
		if (!("pins" in config[bll]))
			continue;
		var pins = config[bll].pins;
		for (var pin in pins) {
			pin = pins[pin];
			switch (pin.type) {
				case "Digital":
					setPin(mux, pin.pin, ("output" == pin.direction) ? exports.digitalOut : exports.digitalIn);
					break;
				case "A2D":
				case "Analog":
					setPin(mux, pin.pin, exports.analogIn);
					break;
				case "I2C":
					setPin(mux, pin.sda, exports.i2cData);
					setPin(mux, pin.clock, exports.i2cClock);
					break;
				case "Serial":
					if ("rx" in pin) setPin(mux, pin.rx, exports.serialRx);
					if ("tx" in pin) setPin(mux, pin.tx, exports.serialTx);
					break;
				case "PWM":
					setPin(mux, pin.pin, exports.pwm);
					break;
				case "Power":
					if ((5 != pin.voltage) && (3.3 != pin.voltage)) throw new Error("unsupported voltage " + pin.voltage)
					setPin(mux, pin.pin, (5 == pin.voltage) ? exports.power5V : exports.power3_3V);
					break;
				case "Ground":
					setPin(mux, pin.pin, exports.ground);
					break;
				case "Audio":
					break;
				default:
					throw new Error("Unrecognized pin type " + pin.type);
					break;
			}
		}
	}

	if (undefined === mux.leftVoltage)
		mux.leftVoltage = 3.3;
	if (undefined === mux.rightVoltage)
		mux.rightVoltage = 3.3;

	return mux;
}

function setPin(mux, pin, type)
{
	if ((51 <= pin) && (pin <= 58)) {
		validatePinAssignment(pin, mux.leftPins[pin - 51], type);
		if ((exports.power3_3V == type) || (exports.power5V == type)) {
			var leftVoltage = (exports.power3_3V == type) ? 3.3 : 5;
			if (undefined === mux.leftVoltage)
				mux.leftVoltage = leftVoltage;
			else if (mux.leftVoltage !== leftVoltage)
				throw new Error("Cannot assign two different voltages to pins in left header");
			type = exports.power3_3V;
		}
		mux.leftPins[pin - 51] = type;
		switch (pin - 51) {		// mirror front left pins to back
			case 0:	mux.back[37] = type; break;
			case 1:	mux.back[36] = type; break;
			case 2:	mux.back[39] = type; break;
			case 3:	mux.back[40] = type; break;
			case 4:	mux.back[43] = type; break;
			case 5:	mux.back[42] = type; break;
			case 6:	mux.back[47] = type; break;
			case 7:	mux.back[46] = type; break;
		}
	}
	else
	if ((59 <= pin) && (pin <= 66)) {
		validatePinAssignment(pin, mux.rightPins[pin - 59], type);
		if ((exports.power3_3V == type) || (exports.power5V == type)) {
			var rightVoltage = (exports.power3_3V == type) ? 3.3 : 5;
			if (undefined === mux.rightVoltage)
				mux.rightVoltage = rightVoltage;
			else if (mux.rightVoltage !== rightVoltage)
				throw new Error("Cannot assign two different voltages to pins in right header");
			type = exports.power3_3V;
		}
		mux.rightPins[pin - 59] = type;
	}
	else
	if ((1 <= pin) && (pin <= 50)) {
		validatePinAssignment(pin, mux.back[pin - 1], type);
		mux.back[pin - 1] = type;
		switch (pin - 51) {		// mirror back pins to front left
			case 37: mux.leftPins[0] = type; break;
			case 36: mux.leftPins[1] = type; break;
			case 39: mux.leftPins[2] = type; break;
			case 40: mux.leftPins[3] = type; break;
			case 43: mux.leftPins[4] = type; break;
			case 42: mux.leftPins[5] = type; break;
			case 47: mux.leftPins[6] = type; break;
			case 46: mux.leftPins[7] = type; break;
		}
	}
}

function validatePinAssignment(pin, from, to)
{
	if (from == to)
		return;

	if ((from == exports.digitalUnconfigured) &&
		((to == exports.digitalIn) || (to == exports.digitalOut)))
		return;

	if (from == exports.disconnected)
		return;

	throw new Error("Cannot configure pin " + pin + ", already assigned.");
}

function equalPins(a, b)
{
	for (var i = 0; i < 8; i++)
		if (a[i] != b[i])
			return false;
	return true;	
}

function normalizeFrontConfig(config)
{
	if (!config) config = new Array;
	config = config.map(function(pin) {
		switch (pin) {
			case this.power5V:
			case this.power3_3V:
				return this.power3_3V;
			case this.ground:
			case this.analogIn:
			case this.digitalIn:
			case this.digitalOut:
			case this.i2cClock:
			case this.i2cData:
			case this.pwm:
				return pin;
			default:
				return this.disconnected; 
		}
	}, exports);

	while (config.length < 8)
		config.push(exports.disconnected);

	return config;
}

function normalizeVoltage(voltage)
{
	switch (voltage) {
		case 5:
			return 5;

		default:
		case 3.3:
			return 3.3;
	}
}

function makeDisconnectedHeader(count)
{
	var header = new Array(count);
	for (var i = 0; i < count; i++)
		header[i] = exports.disconnected;
	return header;
}

function makeBackHeader(count)
{
	var header = makeDisconnectedHeader(51);
	
	for (var i = 1; i <= 24; i++)
		header[i] = exports.digitalUnconfigured;

	header[1] = header[2] =
	header[13] = header[14] =
	header[26] = header[32] =
	header[35] = header[36] =
	header[41] = header[42] =
	header[45] = header[46] = exports.ground;

	header[25] = exports.power3_3V;
	header[49] = exports.power3_3V;
	header[50] = exports.power5V;

	header[28] = header[30] = header[34] = exports.pwm;

	header[31] = exports.serialTx;
	header[33] = exports.serialRx;

	header[27] = exports.i2cData;
	header[29] = exports.i2cClock;

	header.shift();
	
	return header;
}
