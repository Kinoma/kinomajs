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

/* Mandatory BLE stack modules */
const GAP = require("./bluetooth/core/gap");
const UART = require("./bluetooth/transport/uart");
const BTUtils = require("./bluetooth/core/btutils");
const UUID = BTUtils.UUID;
const BluetoothAddress = BTUtils.BluetoothAddress;

/* Mandatory utilities */
const Utils = require("./common/utils");
const Logger = Utils.Logger;
const Buffers = require("./common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

/* Marvell BT chip */
const DEFAULT_UART_DEVICE = "/dev/mbtchar0";
const DEFAULT_UART_BAUDRATE = 115200;

/* API consts */
const LOGGING_ENABLED = false;

var logger = Logger.getLogger("BLE");

/* Pins instances */
var _notification = null;
var _serial = null;
var _repeat = null;

/* Transport */
var _transport = null;

/* GAP instances */
var _gapApplication = null;
var _storage = null;
var _gap = null;

var ADVANCED_MODE = false;
var GATTAPI = null;

// ----------------------------------------------------------------------------------
// Pins Configuration
// ----------------------------------------------------------------------------------

function pollTransport() {
	let buffer = _serial.read("ArrayBuffer");
	if (buffer.byteLength == 0) {
		logger.trace("serial.read returns 0");
		return;
	}
	let responses = _transport.receive(new Uint8Array(buffer), 0, buffer.byteLength);
	for (let i = 0; i < responses.length; i++) {
		_gap.hci.transportReceived(responses[i]);
	}
}

exports.configure = function (data) {
	if (data.hasOwnProperty("mode")) {
		ADVANCED_MODE = (data.mode === "advanced");
	}
	if (ADVANCED_MODE) {
		logger.info("Advanced Mode API");
	}
	if (data.hasOwnProperty("logging")) {
		Logger.setOutputEnabled(data.logging);
		if ("loggers" in data) {
			for (let config of data.loggers) {
				let logger = Logger.getLogger(config.name);
				if (logger != null) {
					logger.loggingLevel = Logger.Level[config.level];
				}
			}
		}
	} else {
		Logger.setOutputEnabled(LOGGING_ENABLED);
	}
	this.notification = _notification = PINS.create({
		type: "Notification"
	});
	this.serial = _serial = PINS.create({
		type: "Serial",
		path: data.hasOwnProperty("device") ? data.device : DEFAULT_UART_DEVICE,
		baud: data.hasOwnProperty("baudrate") ? data.baudrate : DEFAULT_UART_BAUDRATE
	});
	_serial.init();
	_repeat = PINS.repeat("serial", this, pollTransport);
	_transport = new UART.Transport(_serial);
	_gapApplication = new GAPApplication();
	_storage = new BondingStorage();
	if (data.hasOwnProperty("bondings")) {
		logger.info("Load Bonding Data...");
		for (let i = 0; i < data.bondings.length; i++) {
			let bond = data.bondings[i];
			if (bond !== undefined && bond != null) {
				bond.address = getBluetoothAddressByJSON(bond.address);
				bond.keys.address = getBluetoothAddressByJSON(bond.keys.address);
				logger.debug("Bond#" + i + ": " + bond.address.toString());
				_storage.addBond(i, bond, false);
			}
		}
	}
	_gap = GAP.createLayer(_gapApplication, _storage);
	if (!ADVANCED_MODE) {
		GATTAPI = require("./bllgatt");
		GATTAPI.setCallback(doNotification);
		GATTAPI.setGAPApplication(_gapApplication);
		/* Export all API */
		for (let key in GATTAPI) {
			module.exports[key] = GATTAPI[key];
		}
	}
	_gap.init(_transport);
};

exports.close = function () {
	logger.debug("Closing BLL");
	_notification.close();
	_notification = null;
	let closed = false;
	_gapApplication.close(() => {
		logger.debug("Shutdown");
		_gap._hci.commands.controller.reset({
			commandComplete: (opcode, response) => {
				_repeat.close();
				_serial.close();
				closed = true;
			}
		});
	});
	while (!closed) {
		pollTransport();
	}
	logger.info("Exit");
};

function getBluetoothAddressByJSON(obj) {
	return obj != null ?
		BluetoothAddress.getByString(obj.address, true, obj.type != "public") :
		null;
}

function preMarshall(val) {
	switch (typeof val) {
	case "symbol":
		return val.toString();
	case "object":
		if (val instanceof Uint8Array) {
			return Array.from(val);
		} else if (val instanceof BluetoothAddress) {
			/* Assume the instance is LEBluetoothAddress */
			return {
				address: val.toString(),
				type: val.typeString
			};
		} else if (val instanceof Array) {
			let sa = new Array();
			val.forEach(e => sa.push(preMarshall(e)));
			return sa;
		} else {
			if (val == null) {
				return null;
			}
			let so = {};
			for (let key in val) {
				if (!val.hasOwnProperty(key)) {
					continue;
				}
				so[key] = preMarshall(val[key]);
			}
			return so;
		}
	default:
		return val;
	}
}

function doNotification(notification) {
	if (_notification == null) {
		return;
	}
	let sanitised = preMarshall(notification);

	if (notification.notification == "gap/discover") {
		/* Suppress Logging... */
		logger.trace("Notification: " + JSON.stringify(sanitised));
	} else {
		logger.debug("Notification: " + JSON.stringify(sanitised));
	}
	_notification.invoke(sanitised);
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
		for (let i = 0; i < this._bondings.length; i++) {
			if (this._bondings[i] == null) {
				return i;
			}
		}
		return this._bondings.push(null) - 1;
	}
	/* GAP Callback: Bonding Storage */
	storeBond(index, info) {
		if (index < 0 || this._bondings.length <= index) {
			return false;
		}
		this.addBond(index, info);
	}
	addBond(index, info, notify = true) {
		this._bondings[index] = info;
		if (notify) {
			doNotification({
				notification: "gap/bond/add",
				index,
				info
			});
		}
	}
	removeBond(index, notify = true) {
		if (index < 0 || this._bondings.length <= index) {
			return;
		}
		this._bondings[index] = null;
		if (notify) {
			doNotification({
				notification: "gap/bond/remove",
				index
			});
		}
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
			_gap.disconnect(context, 0x15);
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
		if (ADVANCED_MODE) {
			/* User will handle ATT externally */
			context.attConnection.onReceived = buffer => {
				doNotification({
					notification: "att/received",
					connection: index,
				//	buffer: buffer.getByteArray().buffer
					array: Array.from(buffer.getByteArray())
				});
			};
		} else {
			/* Keep ATT bearer instance */
			context.bearer = GATTAPI.onConnection(context, index);
		}
		let link = context._connectionManager.link;
		let hciInfo = link._info;	// FIXME
		doNotification({
			notification: "gap/connect",
			connection: index,
			peripheral: context.peripheral,
			address: link.remoteAddress.toString(),
			addressType: link.remoteAddress.typeString,
			rpa: rpa != null ? rpa.toString() : null,
			interval: hciInfo.connParameters.interval,
			timeout: hciInfo.connParameters.supervisionTimeout,
			latency: hciInfo.connParameters.latency,
			bond: _storage.findBondIndexByAddress(link.remoteAddress)
		});
	}
	/* GAP Callback */
	gapDiscovered(discoveredList) {
		for (let discovered of discoveredList) {
			let {rssi, remoteAddress, structures, eventType} = discovered;
			doNotification({
				notification: "gap/discover",
				rssi: rssi,
				address: remoteAddress.toString(),
				addressType: remoteAddress.isRandom() ? "random" : "public",
				resolvable: remoteAddress.isResolvable(),
				data: parseADFromStructures(structures),
				type: eventType
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
	encryptionCompleted(div, info) {
		doNotification({
			notification: "sm/encryption/complete",
			connection: this._index,
			bond: (info != null) ? div : -1
		});
	}
	encryptionFailed(status) {
		let msg = "Encryption Failed: status=" + Utils.toHexString(status);
		logger.error(msg);
		throw new Error(msg);
	}
	disconnected(reason) {
		logger.debug("Disconnected: index=" + this._index
			+ ", reason=" + Utils.toHexString(reason));
		this._gapApp.unregisterContext(this._index);
		doNotification({
			notification: "gap/disconnect",
			connection: this._index,
			reason
		});
	}
}

// ----------------------------------------------------------------------------------
// API - GAP
// ----------------------------------------------------------------------------------

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
	_gap.startScanning(interval, window, !(duplicates == "allow"));
};

exports.gapStopScanning = function () {
	logger.debug("gapStopScanning");
	_gap.stopScanning();
};

exports.gapConnect = function (params) {
	let address = params.address;
	let random = (params.addressType != "public");
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_INITIAL_CONN_INTERVAL;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_INITIAL_CONN_INTERVAL;
	var supervisionTimeout = ("timeout" in params) ? params.timeout : 3200;
	var latency = ("latency" in params) ? params.latency : 0;

	logger.debug("gapConnect");
	var peerAddress = BluetoothAddress.getByString(address, true, random);

	_gap.stopScanning();		// XXX: Make sure it stops scanning
	_gap.directConnection(peerAddress, {
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
	_gap.setScanResponseData(serializeAD(params));
};

exports.gapStartAdvertising = function (params) {
	if (params === undefined || params == null) {
		params = {};
	}
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_ADV_FAST_INTERVAL1;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_ADV_FAST_INTERVAL1;
	var structures = ("data" in params) ? serializeAD(params.data) : null;

	logger.debug("gapStartAdvertising");
	_gap.setDiscoverableMode(GAP.DiscoverableMode.GENERAL);
	_gap.setConnectableMode(GAP.ConnectableMode.UNDIRECTED);
	_gap.startAdvertising(intervalMin, intervalMax, structures);
};

exports.gapStopAdvertising = function () {
	logger.debug("gapStopAdvertising");
	_gap.stopAdvertising();
};

exports.gapUpdateConnection = function (params) {
	var index = params.connection;
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_INITIAL_CONN_INTERVAL;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_INITIAL_CONN_INTERVAL;
	var supervisionTimeout = ("timeout" in params) ? params.timeout : 3200;
	var latency = ("latency" in params) ? params.latency : 0;
	var l2cap = ("l2cap" in params) ? params.l2cap : false;

	logger.debug("gapUpdateConnection");

	var context = _gapApplication.getContext(index);
	context.updateConnectionParameter({
		intervalMin: intervalMin,
		intervalMax: intervalMax,
		latency: latency,
		supervisionTimeout: supervisionTimeout,
		minimumCELength: 0,
		maximumCELength: 0
	}, l2cap);
};

exports.gapDisconnect = function (params) {
	var index = params.connection;

	logger.debug("gapDisconnect");

	var context = _gapApplication.getContext(index);
	_gap.disconnect(context, 0x13);
};

exports.gapEnablePrivacy = function () {
	logger.debug("gapEnablePrivacy");
	_gap.enablePrivacyFeature(true);
};

exports.gapDeleteBond = function (params) {
	logger.debug("gapDeleteBond");
	_storage.removeBond(params.index);
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
	_gap._hci.commands.status.readRSSI(
		context.handle,
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
// API - L2CAP (ATT Channel) (Advanced Mode API)
// ----------------------------------------------------------------------------------

exports.attSendAttributePDU = function (params) {
	let index = params.connection;

	logger.debug("attSendAttributePDU");

	let context = _gapApplication.getContext(index);
	context.attConnection.sendAttributePDU(new Uint8Array(params.buffer));
};

// ----------------------------------------------------------------------------------
// GAP Advertising Data
// ----------------------------------------------------------------------------------

var Packet = require("bluetooth/bgcompat/packet").Packet;

/* BG/Tessel Compatibility */
function parseADFromStructures(structures) {
	let bgStructures = [];
	for (let structure of structures) {
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
