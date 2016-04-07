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

/**
 * Kinoma LowPAN Framework: Kinoma Bluetooth Stack
 * Bluetooth v4.2 - Public BLL API
 */

var GAP = require("./bluetooth/core/gap");
var UART = require("./bluetooth/transport/uart");
var BTUtils = require("./bluetooth/core/btutils");
var UUID = BTUtils.UUID;
var BluetoothAddress = BTUtils.BluetoothAddress;

var Utils = require("./common/utils");
var Logger = Utils.Logger;
var Buffers = require("./common/buffers");
var ByteBuffer = Buffers.ByteBuffer;

var GATT = GAP.GATT;
var ATT = GATT.ATT;

const DEFAULT_MTU = 158;
const LOGGING_ENABLED = false;

var logger = new Logger("BGAPI");
logger.loggingLevel = Logger.Level.INFO;

// ----------------------------------------------------------------------------------
// Pins Configuration
// ----------------------------------------------------------------------------------

var _notification = null;
var _repeat = null;

function pollTransport() {
	let responses = UART.receive();
	for (var i = 0; i < responses.length; i++) {
		GAP.HCI.transportReceived(responses[i]);
	}
}

exports.configure = function () {
	Utils.Logger.setOutputEnabled(LOGGING_ENABLED);
	this.notification = _notification = PINS.create({
		type: "Notification"
	});
	this.serial = UART.open();
	_repeat = PINS.repeat("serial", this, pollTransport);
	GAP.activate(UART, _gapApplication, _storage);
};

exports.close = function () {
	logger.debug("Closing BLL");
	_notification.close();
	_notification = null;
	let closed = false;
	_gapApplication.close(() => {
		logger.debug("Shutdown");
		GAP.HCI.Controller.reset({
			commandComplete: (opcode, response) => {
				_repeat.close();
				UART.close();
				closed = true;
			}
		});
	});
	while (!closed) {
		pollTransport();
	}
	logger.info("Exit");
};

function doNotification(notification) {
	if (_notification == null) {
		return;
	}
	if (notification.notification == "gap/discover") {
		/* Suppress Logging... */
		logger.trace("Notification: " + JSON.stringify(notification));
	} else {
		logger.debug("Notification: " + JSON.stringify(notification));
	}
	_notification.invoke(notification);
}

// ----------------------------------------------------------------------------------
// GAP Configuration
// ----------------------------------------------------------------------------------

class BondingStorage {
	constructor() {
		this._bondings = new Array();
	}
	/* GAP Callback: Bonding Storage */
	allocateBond() {
		let dummy = {};
		for (let i = 0; i < this._bondings.length; i++) {
			if (this._bondings[i] == null) {
				this._bondings[i] = dummy;
				return i;
			}
		}
		return this._bondings.push(dummy) - 1;
	}
	/* GAP Callback: Bonding Storage */
	storeBond(index, info) {
		if (index < 0 || this._bondings.length <= index) {
			return false;
		}
		this._bondings[index] = info;
	}
	/* GAP Callback: Bonding Storage */
	getBond(index) {
		if (index < 0 || this._bondings.length <= index) {
			return null;
		}
		return this._bondings[index];
	}
	/* GAP Callback: Bonding Storage */
	getBonds() {
		let results = new Array();
		for (let i = 0; i < this._bondings.length; i++) {
			if (this._bondings[i] != null) {
				results.push(this._bondings[i]);
			}
		}
		return results;
	}
	/* GAP Callback: Bonding Storage */
	findBondIndexByAddress(address) {
		for (let i = 0; i < this._bondings.length; i++) {
			if (this._bondings[i] != null) {
				let address = this._bondings[i].address;
				if (address != null && address.equals(address)) {
					return i;
				}
			}
		}
		return -1;
	}
}

class GAPApplication {
	constructor() {
		this._contexts = new Array();
		this._closing = false;
		this._closeCallback = null;
	}
	close(callback = null) {
		this._closing = true;
		this._closeCallback = callback;
		logger.debug("Disconnect all bearers...");
		let num = 0;
		this.forEachContext(context => {
			context.disconnect(0x15);
			num++;
		});
		if (num == 0) {
			logger.debug("No bearers are active");
			if (callback != null) {
				callback();
			}
		} else {
			logger.debug("" + num + " disconnetion is pending.");
		}
	}
	registerContext(context) {
		for (let i = 0; i < this._contexts.length; i++) {
			if (this._contexts[i] == null) {
				this._contexts[i] = context;
				return i;
			}
		}
		return this._contexts.push(context) - 1;
	}
	unregisterContext(index) {
		this._contexts[index] = null;
		if (this._closing) {
			let num = 0;
			this.forEachContext(context => num++);
			if (num == 0) {
				logger.debug("All bearers are disconnected.");
				if (this._closeCallback != null) {
					this._closeCallback();
				}
			}
		}
	}
	forEachContext(func) {
		for (let i = 0; i < this._contexts.length; i++) {
			let context = this._contexts[i];
			if (context != null && !context.isDisconnected()) {
				func(context);
			}
		}
	}
	getContext(index) {
		let context = this._contexts[index];
		if (context == null || context.isDisconnected()) {
			throw "Bearer has been disconnected";
		}
		return context;
	}
	/* GAP Callback */
	gapReady() {
		logger.info("GAP Ready");
		doNotification({
			notification: "system/reset"
		});
	}
	/* GAP Callback */
	gapConnected(context, rpa) {
		let index = this.registerContext(context);
		context.delegate = new GAPContextDelegate(this, index);
		context.bearer.delegate = new ATTBearerDelegate(index);
		if (!context.peripheral) {
			GATT.exchangeMTU(context.bearer);
		}
		let link = context._connectionManager.getHCILink();
		let hciInfo = link._info;	// FIXME
		doNotification({
			notification: "gap/connect",
			connection: index,
			address: link.remoteAddress.toString(),
			addressType: link.remoteAddress.isRandom() ? "random" : "public",
			rpa: rpa != null ? rpa.toString() : null,
			interval: hciInfo.connParameters.interval,
			timeout: hciInfo.connParameters.supervisionTimeout,
			latency: hciInfo.connParameters.latency,
		//	bond: this.findBondIndexByAddress(link.remoteAddress)
		});
	}
	/* GAP Callback */
	gapDiscovered(discoveredList) {
		for (let discovered of discoveredList) {
			let {rssi, remoteAddress, structures, type} = discovered;
			doNotification({
				notification: "gap/discover",
				rssi: rssi,
				address: remoteAddress.toString(),
				addressType: remoteAddress.isRandom() ? "random" : "public",
				resolvable: remoteAddress.isResolvable(),
				data: parseADFromStructures(structures),
				type: type
			});
		}
	}
	/* GAP Callback */
	privacyEnabled(privateAddress) {
		doNotification({
			notification: "gap/privacy/complete",
			rpa: privateAddress.toString()
		});
	}
}

class GAPContextDelegate {
	constructor(gapApp, index) {
		this._gapApp = gapApp;
		this._index = index;
	}
	passkeyRequested(input) {
		doNotification({
			notification: "sm/passkey",
			connection: this._index,
			input
		});
	}
	encryptionCompleted(div, pairingInfo) {
		doNotification({
			notification: "sm/encryption/complete",
			connection: this._index,
			bond: div,
			pairing: pairingInfo != null
		});
	}
	encryptionFailed(status) {
		let msg = "Encryption Failed: status=" + Utils.toHexString(status);
		logger.error(msg);
		throw new Error(msg);
	}
	disconnected() {
		logger.debug("Disconnected: index=" + this._index);
		this._gapApp.unregisterContext(this._index);
		doNotification({
			notification: "gap/disconnect",
			connection: this._index
		});
	}
}

class ATTBearerDelegate {
	constructor(index) {
		this._index = index;
	}
	indicationReceived(opcode, indication) {
		doNotification({
			notification: "gatt/characteristic/indicate",
			connection: this._index,
			characteristic: indication.handle,
			value: Array.from(indication.value)
		});
	}
	notificationReceived(opcode, notification) {
		doNotification({
			notification: "gatt/characteristic/notify",
			connection: this._index,
			characteristic: notification.handle,
			value: Array.from(notification.value)
		});
	}
}

exports.enableLogging = function (params) {
	Utils.Logger.setOutputEnabled(params.enabled);
	if ("bll" in params) {
		logger.loggingLevel = Logger.Level[params.bll];
	}
	if ("gap" in params) {
		GAP.setLoggingLevel(Logger.Level[params.gap]);
	}
	if ("hci" in params) {
		GAP.HCI.setLoggingLevel(Logger.Level[params.hci]);
	}
	if ("l2cap" in params) {
		GAP.L2CAP.setLoggingLevel(Logger.Level[params.l2cap]);
	}
	if ("gatt" in params) {
		GATT.setLoggingLevel(Logger.Level[params.gatt]);
	}
	if ("att" in params) {
		ATT.setLoggingLevel(Logger.Level[params.att]);
	}
	if ("sm" in params) {
		GAP.SM.setLoggingLevel(Logger.Level[params.sm]);
	}
};

// ----------------------------------------------------------------------------------
// API - GAP
// ----------------------------------------------------------------------------------

var _gapApplication = new GAPApplication();
var _storage = new BondingStorage();

/* Consts from GAP */
var SCAN_FAST_INTERVAL = 0x03C0;		// TGAP(scan_fast_interval)		30ms to 60ms	(set to 600ms per Wi-Fi team, AJC)
var SCAN_FAST_WINDOW = 0x0040;			// TGAP(scan_fast_window)		30ms			(set to 40ms per Wi-Fi team, AJC)
var MIN_INITIAL_CONN_INTERVAL = 0x18;	// TGAP(initial_conn_interval)	30ms to 50ms
var MAX_INITIAL_CONN_INTERVAL = 0x28;	// TGAP(initial_conn_interval)	30ms to 50ms
var MIN_ADV_FAST_INTERVAL1 = 0x30;		// TGAP(adv_fast_interval1)		30ms to 60ms
var MAX_ADV_FAST_INTERVAL1 = 0x60;		// TGAP(adv_fast_interval1)		30ms to 60ms

exports.gapStartScanning = function (params) {
	if (params === undefined || params == null) {
		params = {};
	}
	var active = ("active" in params) ? params.active : true;
	var interval = ("interval" in params) ? params.interval : SCAN_FAST_INTERVAL;
	var window = ("window" in params) ? params.window : SCAN_FAST_WINDOW;
	var duplicates = ("duplicates" in params) ? params.duplicates : "allow";

	logger.debug("gapStartScanning");
	GAP.startScanning(interval, window, !(duplicates == "allow"));
};

exports.gapStopScanning = function () {
	logger.debug("gapStopScanning");
	GAP.stopScanning();
};

exports.gapConnect = function (params) {
	let address = params.address;
	let random = (params.addressType == "random");
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_INITIAL_CONN_INTERVAL;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_INITIAL_CONN_INTERVAL;
	var supervisionTimeout = ("timeout" in params) ? params.timeout : 3200;
	var latency = ("latency" in params) ? params.latency : 0;

	logger.debug("gapConnect");
	var peerAddress = BluetoothAddress.getByString(address, true, random);

	GAP.stopScanning();		// XXX: Make sure it stops scanning
	GAP.directConnection(peerAddress, {
		intervalMin: intervalMin,
		intervalMax: intervalMax,
		latency: latency,
		supervisionTimeout: supervisionTimeout,
		minimumCELength: 0,
		maximumCELength: 0
	});
};

exports.gapSetScanResponseData = function (params) {
	logger.debug("gapSetScanResponseData");
	GAP.setScanResponseData(serializeAD(params));
};

exports.gapStartAdvertising = function (params) {
	if (params === undefined || params == null) {
		params = {};
	}
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_ADV_FAST_INTERVAL1;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_ADV_FAST_INTERVAL1;
	var structures = ("data" in params) ? serializeAD(params.data) : null;

	logger.debug("gapStartAdvertising");
	GAP.setDiscoverableMode(GAP.DiscoverableMode.GENERAL);
	GAP.setConnectableMode(GAP.ConnectableMode.UNDIRECTED);
	GAP.startAdvertising(intervalMin, intervalMax, structures);
};

exports.gapStopAdvertising = function () {
	logger.debug("gapStopAdvertising");
	GAP.stopAdvertising();
};

exports.gapUpdateConnection = function (params) {
	var index = params.connection;
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_INITIAL_CONN_INTERVAL;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_INITIAL_CONN_INTERVAL;
	var supervisionTimeout = ("timeout" in params) ? params.timeout : 3200;
	var latency = ("latency" in params) ? params.latency : 0;

	logger.debug("gapUpdateConnection");

	var bearer = _gapApplication.getContext(index).bearer;
	GAP.updateConnectionParameter(bearer, {
		intervalMin: intervalMin,
		intervalMax: intervalMax,
		latency: latency,
		supervisionTimeout: supervisionTimeout,
		minimumCELength: 0,
		maximumCELength: 0
	});
};

exports.gapDisconnect = function (params) {
	var index = params.connection;

	logger.debug("gapDisconnect");

	var context = _gapApplication.getContext(index);
	context.disconnect(0x13);
};

exports.gapEnablePrivacy = function () {
	logger.debug("gapEnablePrivacy");
	GAP.enablePrivacyFeature(true);
};

// ----------------------------------------------------------------------------------
// API - SM
// ----------------------------------------------------------------------------------

exports.smStartEncryption = function (params) {
	var index = params.connection;

	logger.debug("smStartEncryption");

	var security = _gapApplication.getContext(index).security;
	security.startEncryption();
};

exports.smSetSecurityParameter = function (params) {
	var index = params.connection;

	logger.debug("smSetSecurityParameter");

	var security = _gapApplication.getContext(index).security;
	if ("bonding" in params) {
		security.parameter.bonding = params.bonding;
	}
	if ("mitm" in params) {
		security.parameter.mitm = params.mitm;
	}
	if ("display" in params) {
		security.parameter.display = params.display;
	}
	if ("keyboard" in params) {
		security.parameter.keyboard = params.keyboard;
	}
};

exports.smPasskeyEntry = function (params) {
	var index = params.connection;

	logger.debug("smPasskeyEntry");

	var security = _gapApplication.getContext(index).security;
	security.passkeyEntry(params.passkey);
};

// ----------------------------------------------------------------------------------
// API - HCI
// ----------------------------------------------------------------------------------

exports.hciReadRSSI = function (params) {
	var index = params.connection;

	logger.debug("hciReadRSSI");

	var context = _gapApplication.getContext(index);
	GAP.HCI.Status.readRSSI(
		context._connectionManager.getHCILink().handle,	// FIXME
		{
			commandComplete: function (opcode, response) {
				var status = response.getInt8();
				if (status != 0) {
					throw new Error("readRSSI failed: " + Utils.toHexString(status));
				}
				var hciHandle = response.getInt16();	// XXX: Should check
				var rssi = Utils.toSignedByte(response.getInt8());
				doNotification({
					notification: "gap/rssi",
					connection: index,
					rssi: rssi
				});
			}
		}
	);
};

/* FIXME: Compatibility Alias */
exports.gapReadRSSI = exports.hciReadRSSI;

// ----------------------------------------------------------------------------------
// API - GATT
// ----------------------------------------------------------------------------------

function procedureComplete(connHandle, handle) {
	doNotification({
		notification: "gatt/request/complete",
		connection: connHandle
	});
}

function procedureCompleteWithError(procedure, errorCode, handle) {
	var msg = procedure + " failed: " + "ATT(" + Utils.toHexString(errorCode) + ")";
	logger.error(msg);
	throw new Error(msg);
}

exports.gattDiscoverAllPrimaryServices = function (params) {
	var index = params.connection;

	logger.debug("gattDiscoverAllPrimaryServices");

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.discoverAllPrimaryServices(bearer, {
		primaryServicesDiscovered: function (services) {
			for (var c = 0; c < services.length; c++) {
				var service = services[c];
				doNotification({
					notification: "gatt/service",
					connection: index,
					start: service.start,
					end: service.end,
					uuid: service.uuid.toString()
				});
			}
			return true;
		},
		procedureComplete: function (handle) {
			procedureComplete(index, handle);
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattDiscoverAllPrimaryServices", errorCode, handle);
		}
	});
};

exports.gattFindIncludedServices = function (params) {
	var index = params.connection;
	var start = params.start;
	var end = params.end;

	logger.debug("gattFindIncludedServices: "
		+ "start=" + Utils.toHexString(start, 2)
		+ ", end=" + Utils.toHexString(end, 2));

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.findIncludedServices(bearer, start, end, {
		includedServicesDiscovered: function (includes) {
			for (var i = 0; i < includes.length; i++) {
				var service = includes[i];
				doNotification({
					notification: "gatt/service",
					connection: index,
					start: service.start,
					end: service.end,
					uuid: service.uuid.toString()
				});
			}
			return true;
		},
		procedureComplete: function (handle) {
			procedureComplete(index, handle);
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattFindIncludedServices", errorCode, handle);
		}
	});
};

exports.gattDiscoverAllCharacteristics = function (params) {
	var index = params.connection;
	var start = params.start;
	var end = params.end;

	logger.debug("gattDiscoverAllCharacteristics: "
		+ "start=" + Utils.toHexString(start, 2)
		+ ", end=" + Utils.toHexString(end, 2));

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.discoverAllCharacteristics(bearer, start, end, {
		characteristicsDiscovered: function (characteristics) {
			for (var i = 0; i < characteristics.length; i++) {
				var characteristic = characteristics[i];
				doNotification({
					notification: "gatt/characteristic",
					connection: index,
					properties: parseCharacteristicProperties(characteristic.properties),
					characteristic: characteristic.handle,
					uuid: characteristic.uuid.toString()
				});
			}
			return true;
		},
		procedureComplete: function (handle) {
			procedureComplete(index, handle);
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattDiscoverAllCharacteristics", errorCode, handle);
		}
	});
};

exports.gattDiscoverAllCharacteristicDescriptors = function (params) {
	var index = params.connection;
	var start = params.start;
	var end = params.end;

	logger.debug("gattDiscoverAllCharacteristicDescriptors: "
		+ "start=" + Utils.toHexString(start, 2)
		+ ", end=" + Utils.toHexString(end, 2));

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.discoverAllCharacteristicDescriptors(bearer, start, end, {
		characteristicDescriptorsDiscovered: function (descriptors) {
			for (var c = 0; c < descriptors.length; c++) {
				var descriptor = descriptors[c];
				doNotification({
					notification: "gatt/descriptor",
					connection: index,
					descriptor: descriptor.handle,
					uuid: descriptor.uuid.toString()
				});
			}
			return true;
		},
		procedureComplete: function (handle) {
			procedureComplete(index, handle);
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattDiscoverAllCharacteristicDescriptors", errorCode, handle);
		}
	});
};

exports.gattReadCharacteristicValue = function (params) {
	var index = params.connection;
	var handle = params.characteristic;

	logger.debug("gattReadCharacteristicValue: handle=" + Utils.toHexString(handle, 2));

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.readCharacteristicValue(bearer, handle, {
		procedureComplete: function (handle, value) {
			doNotification({
				notification: "gatt/characteristic/value",
				connection: index,
				characteristic: handle,
				value: Array.from(value)
			});
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattReadCharacteristicValue", errorCode, handle);
		}
	});
};

exports.gattWriteWithoutResponse = function (params) {
	var index = params.connection;
	var handle = params.characteristic;
	var value = params.value;

	logger.debug("gattWriteWithoutResponse: handle=" + Utils.toHexString(handle, 2));

	var bearer = _gapApplication.getContext(index).bearer;

	GATT.writeWithoutResponse(bearer, handle, new Uint8Array(value));
};

exports.gattWriteCharacteristicValue = function (params) {
	var index = params.connection;
	var handle = params.characteristic;
	var value = params.value;

	logger.debug("gattWriteCharacteristicValue: handle=" + Utils.toHexString(handle, 2));

	var bearer = _gapApplication.getContext(index).bearer;
	GATT.writeCharacteristicValue(bearer, handle, new Uint8Array(value), {
		procedureComplete: function (handle, value) {
			procedureComplete(index, handle);
		},
		procedureCompleteWithError: function (errorCode, handle) {
			procedureCompleteWithError("gattWriteCharacteristicValue", errorCode, handle);
		}
	});
};

function toCharacteristicProperties(params) {
	var properties = 0;
	for (var i = 0; i < params.length; i++) {
		switch (params[i]) {
		case "broadcast":
			properties |= GATT.Properties.BROADCAST;
			break;
		case "read":
			properties |= GATT.Properties.READ;
			break;
		case "writeWithoutResponse":
			properties |= GATT.Properties.WRITE_WO_RESP;
			break;
		case "write":
			properties |= GATT.Properties.WRITE;
			break;
		case "notify":
			properties |= GATT.Properties.NOTIFY;
			break;
		case "indicate":
			properties |= GATT.Properties.INDICATE;
			break;
		}
	}
	return properties;
}

function parseCharacteristicProperties(properties) {
	var parsed = [];
	if ((properties & GATT.Properties.BROADCAST) > 0) {
		parsed.push("broadcast");
	}
	if ((properties & GATT.Properties.READ) > 0) {
		parsed.push("read");
	}
	if ((properties & GATT.Properties.WRITE_WO_RESP) > 0) {
		parsed.push("writeWithoutResponse");
	}
	if ((properties & GATT.Properties.WRITE) > 0) {
		parsed.push("write");
	}
	if ((properties & GATT.Properties.NOTIFY) > 0) {
		parsed.push("notify");
	}
	if ((properties & GATT.Properties.INDICATE) > 0) {
		parsed.push("indicate");
	}
	return parsed;
}

exports.gattAddServices = function (params) {
	logger.debug("gattAddServices");
	var services = [];
	for (var s = 0; s < params.services.length; s++) {
		var serviceTmp = params.services[s];
		logger.debug("Service#" + s + ": " + serviceTmp.uuid);
		var service = new GATT.Service(
			UUID.getByString(serviceTmp.uuid),
			serviceTmp.hasOwnProperty("primary") ? serviceTmp.primary : true);
		if (serviceTmp.hasOwnProperty("includes")) {
			for (var i = 0; i < serviceTmp.includes.length; i++) {
				var uuid = serviceTmp.includes[i];
				service.addInclude(new GATT.Include(UUID.getByString(uuid)));
			}
		}
		for (var c = 0; c < serviceTmp.characteristics.length; c++) {
			var characteristicTmp = serviceTmp.characteristics[c];
			logger.debug("Characteristic#" + c + ": " + characteristicTmp.uuid);
			var characteristic = new GATT.Characteristic(
				UUID.getByString(characteristicTmp.uuid),
				toCharacteristicProperties(characteristicTmp.properties));
			if (characteristicTmp.hasOwnProperty("description")) {
				characteristic.description = characteristicTmp.description;
			}
			if (characteristicTmp.hasOwnProperty("value")) {
				/* XXX: We override the formats if the default values is a known type. */
				if ((typeof characteristicTmp.value) == "string") {
					characteristicTmp.formats = ["utf8s"];
				} else if ((typeof characteristicTmp.value) == "boolean") {
					characteristicTmp.formats = ["boolean"];
				} else if ((typeof characteristicTmp.value) == "number") {
					characteristicTmp.formats = ["sint32"];
				}
				characteristic.value = characteristicTmp.value;
			}
			if (characteristicTmp.hasOwnProperty("formats")) {
				for (var f = 0; f < characteristicTmp.formats.length; f++) {
					var format = GATT.getFormatByName(characteristicTmp.formats[f]);
					if (format > 0) {
						characteristic.addFormat({format: format});
					}
				}
			}
			characteristic.onValueRead = (c) => {
				logger.debug("Characteristic value on read: handle="
					+ Utils.toHexString(c.handle, 2));
			};
			characteristic.onValueWrite = (c, value) => {
				logger.debug("Characteristic value on write: handle="
					+ Utils.toHexString(c.handle, 2));
				doNotification({
					notification: "gatt/local/write",
					service: service.uuid,
					characteristic: c.uuid,
					value: Array.from(value)
				});
			};
			service.addCharacteristic(characteristic);
		}
		services.push(service);
	}
	var result = GAP.deployServices(services);
	logger.debug("Service added: " + JSON.stringify(result));

	doNotification({
		notification: "gatt/services/add",
		services: toAddServiceResults(services)
	});
};

function toAddServiceResults(services) {
	var results = [];
	for (var s = 0; s < services.length; s++) {
		var service = services[s];
		var result = {
			uuid: service.uuid.toString(),
			characteristics: []
		};
		for (var c = 0; c < service.characteristics.length; c++) {
			var characteristic = service.characteristics[c];
			result.characteristics.push({
				uuid: characteristic.uuid.toString(),
				handle: characteristic.handle
			});
		}
		results.push(result);
	}
	return results;
}

exports.gattWriteLocal = function (params) {
	var serviceUUID = UUID.getByString(params.service);
	var characteristicUUID = UUID.getByString(params.characteristic);
	var value = params.value;

	logger.debug("gattWriteLocal");

	/* Find characteristic */
	var service = GAP.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	var characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	/* Update characteristic value */
	characteristic.value = value;

	/* Auto Notify/Indication */
	_gapApplication.forEachContext(context => {
		var bearer = context.bearer;
		var config = characteristic.getClientConfiguration(bearer);
		if ((config & GATT.ClientConfiguration.NOTIFICATION) > 0) {
			GATT.notifyValue(bearer, characteristic.handle, characteristic._readValue());	// FIXME
		} else if ((config & GATT.ClientConfiguration.INDICATION) > 0) {
			GATT.indicateValue(bearer, characteristic.handle, characteristic._readValue(), {	// FIXME
				procedureComplete: function (handle) {
					logger.info("Indicated for handle=" + Utils.toHexString(handle, 2));
				},
				procedureCompleteWithError: function (errorCode, handle) {
					logger.error("Indicate error for handle=" + Utils.toHexString(handle, 2)
						+ ", errorCode=" + Utils.toHexString(errorCode));
				}
			});
		}
	});
};

exports.gattReadLocal = function (params) {
	var serviceUUID = UUID.getByString(params.service);
	var characteristicUUID = UUID.getByString(params.characteristic);

	logger.debug("gattReadLocal");

	/* Find characteristic */
	var service = GAP.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	var characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	doNotification({
		notification: "gatt/local/read",
		service: serviceUUID,
		characteristic: characteristicUUID,
		value: Array.from(characteristic.value)
	});
};

// ----------------------------------------------------------------------------------
// GAP Advertising Data
// ----------------------------------------------------------------------------------

var Packet = require("bluetooth/bgcompat/packet").Packet;

/* BG/Tessel Compatibility */
function parseADFromStructures(structures) {
	let bgStructures = [];
	for (let structure of structures) {
		structure.data = Array.from(structure.data);
		let bg = new Packet(structure.type, structure.data, "LE");
		bgStructures.push({
			data: bg.data,
			raw: structure.data,
			type: bg.type,
			flag: structure.type
		});
	}
	return bgStructures;
}

function serializeAD(params) {
	let structures = new Array();
	for (let key in params) {
		if (params.hasOwnProperty(key) && AdvertisingDataSerializer.hasOwnProperty(key)) {
			let serializer = AdvertisingDataSerializer[key];
			let data = serializer.serialize(params[key]);
			structures.push({
				type: serializer.type,
				data
			});
		}
	}
	return structures;
}

function serializeUUIDList(data) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
	for (let uuidStr of data) {
		let uuid = UUID.getByString(uuidStr);
		if (uuid.isUUID16()) {
			buffer.putInt16(uuid.toUUID16());
		} else {
			buffer.putByteArray(uuid.toUUID128());
		}
	}
	buffer.flip();
	return buffer.getByteArray();
}

function serializeServiceData({uuid, data = null}) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
	if (uuid.isUUID16()) {
		buffer.putInt16(uuid.toUUID16());
	} else {
		buffer.putByteArray(uuid.toUUID128());
	}
	if (data != null) {
		buffer.putByteArray(data);
	}
	buffer.flip();
	return buffer.getByteArray();
}

function serializeUInt8(data) {
	return [data & 0xFF];
}

function serializeUInt16(data) {
	return Utils.toByteArray(data, 2, true);
}

function serializeAddress(data) {
	return BluetoothAddress.getByString(data).getRawArray();
}

var AdvertisingDataSerializer = {
	["incompleteUUID16List"]: {
		type: GAP.ADType.INCOMPLETE_UUID16_LIST,
		serialize: serializeUUIDList
	},
	["completeUUID16List"]: {
		type: GAP.ADType.COMPLETE_UUID16_LIST,
		serialize: serializeUUIDList
	},
	["incompleteUUID128List"]: {
		type: GAP.ADType.INCOMPLETE_UUID128_LIST,
		serialize: serializeUUIDList
	},
	["completeUUID128List"]: {
		type: GAP.ADType.COMPLETE_UUID128_LIST,
		serialize: serializeUUIDList
	},
	["shortName"]: {
		type: GAP.ADType.SHORTENED_LOCAL_NAME,
		serialize: BTUtils.toCharArray
	},
	["completeName"]: {
		type: GAP.ADType.COMPLETE_LOCAL_NAME,
		serialize: BTUtils.toCharArray
	},
	["flags"]: {
		type: GAP.ADType.FLAGS,
		serialize: (data) => {
			let flags = 0;
			for (let flag of data) {
				flags |= flag;
			}
			return [flags];
		}
	},
	["manufacturerSpecific"]: {
		type: GAP.ADType.MANUFACTURER_SPECIFIC_DATA,
		serialize: ({identifier, data /* = null // FIXME: KINOMA STUDIO WILL CRASH!!  */}) => {
			let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
			buffer.putInt16(identifier);
			if (data != undefined && data != null) {
				buffer.putByteArray(data);
			}
			buffer.flip();
			return buffer.getByteArray();
		}
	},
	["txPowerLevel"]: {
		type: GAP.ADType.TX_POWER_LEVEL,
		serialize: serializeUInt8
	},
	["connectionInterval"]: {
		type: GAP.ADType.SLAVE_CONNECTION_INTERVAL_RANGE,
		serialize: ({intervalMin, intervalMax}) => {
			let buffer = ByteBuffer.allocateUint8Array(4, true);
			buffer.putInt16(intervalMin);
			buffer.putInt16(intervalMax);
			return buffer.array;
		}
	},
	["solicitationUUID16List"]: {
		type: GAP.ADType.SOLICITATION_UUID16_LIST,
		serialize: serializeUUIDList
	},
	["solicitationUUID128List"]: {
		type: GAP.ADType.SOLICITATION_UUID128_LIST,
		serialize: serializeUUIDList
	},
	["serviceDataUUID16"]: {
		type: GAP.ADType.SERVICE_DATA_UUID16,
		serialize: serializeServiceData
	},
	["serviceDataUUID128"]: {
		type: GAP.ADType.SERVICE_DATA_UUID128,
		serialize: serializeServiceData
	},
	["appearance"]: {
		type: GAP.ADType.APPEARANCE,
		serialize: serializeUInt16
	},
	["publicAddress"]: {
		type: GAP.ADType.PUBLIC_TARGET_ADDRESS,
		serialize: serializeAddress
	},
	["randomAddress"]: {
		type: GAP.ADType.RANDOM_TARGET_ADDRESS,
		serialize: serializeAddress
	},
	["advertisingInterval"]: {
		type: GAP.ADType.ADVERTISING_INTERVAL,
		serialize: serializeUInt16
	},
/*
	["deviceAddress"]: {
		type: GAP.ADType.LE_BLUETOOTH_DEVICE_ADDRESS,
		serialize: null			// TODO
	},
*/
	["role"]: {
		type: GAP.ADType.LE_ROLE,
		serialize: serializeUInt8
	},
	["uri"]: {
		type: GAP.ADType.URI,
		serialize: BTUtils.toCharArray
	}
};
