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
 * Bluetooth v4.2 - Public BLL API (Obsolete)
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

/* GATT related optional BLE stack modules */
const GATT = require("./bluetooth/core/gatt");
const ATT = GATT.ATT;
const GATTServer = require("./bluetooth/gatt/server");
const GATTClient = require("./bluetooth/gatt/client");

/* Marvell BT chip */
const DEFAULT_UART_DEVICE = "/dev/mbtchar0";
const DEFAULT_UART_BAUDRATE = 115200;

/* API consts */
const LOGGING_ENABLED = false;
const DEFAULT_MTU = 158;

let logger = Logger.getLogger("BLE");

/* Pins instances */
let _notification = null;
let _serial = null;
let _repeat = null;

/* Transport */
let _transport = null;

/* GAP instances */
let _gapApplication = null;
let _storage = null;
let _gap = null;
let _ccConfigs = new Map();

let _scanResponseData = null;

let _profile = new GATTServer.Profile();

// ----------------------------------------------------------------------------------
// Pins Configuration
// ----------------------------------------------------------------------------------

function pollTransport() {
	let buffer = _serial.read("ArrayBuffer");
	if (buffer.byteLength == 0) {
		logger.trace("serial.read returns 0");
		return;
	}
	_transport.receive(new Uint8Array(buffer), 0, buffer.byteLength);
}

exports.configure = function (data) {
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
				_storage.addBond(i, bond);
			}
		}
	}
	_gap = GAP.createLayer(_gapApplication, _storage);
	_gap.init(_transport);
};

exports.close = function () {
	logger.debug("Closing BLL");
	_notification.close();
	_notification = null;
	let closed = false;
	_gapApplication.onAllDisconnected = () => {
		logger.debug("Shutdown");
		_gap._hci._commandExec.submitCommand(0x0C03, null, {
			commandComplete: (opcode, buffer) => {
				_repeat.close();
				_serial.close();
				closed = true;
			}
		});
	};
	_gapApplication.disconnectAllConnections();
	while (!closed) {
		pollTransport();
	}
	logger.info("Exit");
};

function getBluetoothAddressByJSON(obj) {
	return obj != null ?
		BluetoothAddress.getByString(obj.address, obj.type != "public") :
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

function sendMessage(message) {
	if (_notification == null) {
		return;
	}
	let sanitised = preMarshall(message);

	if (message.notification == "gap/discover") {
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

class BondingStorage extends BTUtils.MemoryBondingStorage {
	constructor() {
		super();
	}
	addBond(index, info) {
		super.addBond(index, info);
		sendMessage({
			identifier: "gap/bond/add",
			index,
			info
		});
	}
}

function getClientConfigurations(connection) {
	let address = connection.address;
	let key = address.toString() + "/" + address.typeString;
	if (!_ccConfigs.has(key)) {
		_ccConfigs.set(key, new Map());
	}
	return _ccConfigs.get(key);
}

function removeClientConfigurations(connection) {
	let address = connection.address;
	let key = address.toString() + "/" + address.typeString;
	if (connection.securityInfo != null && connection.securityInfo.bonding && address.isIdentity()) {
		logger.debug("CCC will be kept");
		return;
	}
	_ccConfigs.delete(key);
	logger.debug("CCC for " + key + " has been deleted");
	// TODO: Persistence
}

function saveClientConfigurations(connection) {
	let address = connection.address;
	let key = address.toString() + "/" + address.typeString;
	logger.debug("CCC for " + key + " has been written");
	// TODO: Persistence
}

class ATTBearer extends ATT.ATTBearer {
	constructor(connection, database) {
		super(connection, database);
	}
	readClientConfiguration(characteristic, connection) {
		let uuid = characteristic.uuid.toString();
		let configs = getClientConfigurations(connection);
		if (!configs.has(uuid)) {
			return 0x0000;
		}
		return configs.get(uuid);
	}
	writeClientConfiguration(characteristic, connection, value) {
		let uuid = characteristic.uuid.toString();
		let configs = getClientConfigurations(connection);
		configs.set(uuid, value);
		saveClientConfigurations(connection);
	}
}

class GAPApplication extends BTUtils.GAPConnectionManager {
	constructor() {
		super();
	}
	/* GAP Callback */
	gapReady() {
		logger.info("GAP Ready");
		sendMessage({
			notification: "system/reset"
		});
	}
	/* GAP Callback */
	gapConnected(connection) {
		this.registerConnection(connection);
		let handle = connection.handle;
		connection.delegate = {
			/* GAP Callback */
			pairingFailed: reason => {
				sendMessage({
					notification: "sm/pairing/failed",
					connection: handle,
					reason
				});
			},
			/* GAP Callback */
			passkeyRequested: input => {
				sendMessage({
					notification: "sm/passkey",
					connection: handle,
					input
				});
			},
			/* GAP Callback */
			encryptionCompleted: securityChanged => {
				sendMessage({
					notification: "sm/encryption/complete",
					connection: handle,
					securityChanged
				});
			},
			/* GAP Callback */
			encryptionFailed: status => {
				sendMessage({
					notification: "sm/encryption/failed",
					connection: handle,
					status
				});
			},
			/* GAP Callback */
			disconnected: reason => {
				logger.debug("Disconnected: index=" + handle
					+ ", reason=" + Utils.toHexString(reason));
				this.unregisterConnection(handle);
				removeClientConfigurations(connection);
				sendMessage({
					notification: "gap/disconnect",
					connection: handle,
					reason
				});
			},
			/* GAP Callback */
			connectionUpdated: parameters => {
				logger.debug("Connection Updated: index=" + handle);
				sendMessage({
					notification: "gap/updated",
					connection: handle,
					parameters
				});
			}
		};
		/* Setup ATT bearer inside the BLL thread */
		let bearer = new ATTBearer(connection, _profile.database);
		/* Override callbacks */
		bearer.onNotification = (opcode, notification) => {
			sendMessage({
				notification: "gatt/characteristic/notify",
				connection: handle,
				characteristic: notification.handle,
				value: notification.value
			});
		};
		bearer.onIndication = (opcode, indication) => {
			sendMessage({
				notification: "gatt/characteristic/indicate",
				connection: handle,
				characteristic: indication.handle,
				value: indication.value
			});
		};
		if (!connection.peripheral) {
			GATTClient.exchangeMTU(bearer, DEFAULT_MTU);
		}
		sendMessage({
			notification: "gap/connect",
			connection: connection.handle,
			peripheral: connection.peripheral,
			address: connection.address.toString(),
			addressType: connection.address.typeString,
			interval: connection.parameters.interval,
			timeout: connection.parameters.supervisionTimeout,
			latency: connection.parameters.latency
		});
	}
	/* GAP Callback */
	gapDiscovered(device) {
		let message = {
			notification: "gap/discover",
			address: device.address.toString(),
			addressType: device.address.isRandom() ? "random" : "public",
			resolvable: device.address.isResolvable(),
			rssi: device.rssi,
		};
		if (device.hasOwnProperty("scanResponse")) {
			message.type = 0x04;
			message.data = parseADFromStructures(device.scanResponse);
		} else {
			if (device.directed) {
				message.type = 0x01;
				message.data = null;
			} else {
				message.type = (device.connectable ? 0x00 : (device.scannable ? 0x02 : 0x03));
				message.data = parseADFromStructures(device.advertising);
			}
		}
		sendMessage(message);
	}
	/* GAP Callback */
	privacyEnabled(privateAddress) {
		sendMessage({
			notification: "gap/privacy/complete",
			rpa: privateAddress.toString()
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
	_gap.startScanning({
		discovery: false,
		active,
		interval,
		window,
		duplicatesFilter: !(duplicates == "allow")
	});
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
	var peerAddress = BluetoothAddress.getByString(address, random);

	_gap.establishConnection(peerAddress, {
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
	_scanResponseData = serializeAD(params);
};

exports.gapStartAdvertising = function (params) {
	if (params === undefined || params == null) {
		params = {};
	}
	var intervalMin = ("intervals" in params && "min" in params.intervals) ? params.intervals.min : MIN_ADV_FAST_INTERVAL1;
	var intervalMax = ("intervals" in params && "max" in params.intervals) ? params.intervals.max : MAX_ADV_FAST_INTERVAL1;
	var structures = ("data" in params) ? serializeAD(params.data) : null;

	logger.debug("gapStartAdvertising");
	_gap.startAdvertising({
		advertising: structures,
		scanResponse: _scanResponseData,
		discoverable: true,
		limited: false,
		connectable: true,
		address: null,
		fast: true,
	});
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

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	gapConn.updateConnectionParameter({
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

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	gapConn.disconnect(0x13);
};

exports.gapEnablePrivacy = function () {
	logger.debug("gapEnablePrivacy");
	_gap.enablePrivacyFeature(true);
};

exports.gapDisablePrivacy = function () {
	logger.debug("gapDisablePrivacy");
	_gap.enablePrivacyFeature(false);
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

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	gapConn.startAuthentication();
};

exports.smSetSecurityParameter = function (params) {
	var index = params.connection;

	logger.debug("smSetSecurityParameter");

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	gapConn.setSecurityParameter(params);
};

exports.smPasskeyEntry = function (params) {
	var index = params.connection;

	logger.debug("smPasskeyEntry");

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	gapConn.passkeyEntry(params.passkey);
};

// ----------------------------------------------------------------------------------
// API - HCI
// ----------------------------------------------------------------------------------

exports.hciReadRSSI = function (params) {
	var index = params.connection;

	logger.debug("hciReadRSSI");

	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	_gap._hci.commands.status.readRSSI(gapConn.handle).then(response => {
		let hciHandle = response.getInt16();	// XXX: Should check
		let rssi = Utils.toSignedByte(response.getInt8());
		sendMessage({
			notification: "gap/rssi",
			connection: index,
			rssi: rssi
		});
	});
};

/* FIXME: Compatibility Alias */
exports.gapReadRSSI = exports.hciReadRSSI;

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

const serializeAD = require("bluetooth/core/gapadvdata").serialize


// ----------------------------------------------------------------------------------
// API - GATT (Standard Mode API)
// ----------------------------------------------------------------------------------

function procedureComplete(connHandle, handle) {
	sendMessage({
		notification: "gatt/request/complete",
		connection: connHandle
	});
}

function procedureCompleteWithError(procedure, errorCode, handle) {
	let msg = procedure + " failed: " + "ATT(" + Utils.toHexString(errorCode) + ")";
	logger.error(msg);
	throw new Error(msg);
}

exports.gattDiscoverAllPrimaryServices = function (params) {
	let index = params.connection;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.discoverPrimaryServices(bearer, results => {
		for (let result of results) {
			sendMessage({
				notification: "gatt/service",
				connection: index,
				start: result.start,
				end: result.end,
				uuid: result.uuid.toString()
			});
		}
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllPrimaryServices", errorCode, handle);
	});
};

exports.gattDiscoverAllCharacteristics = function (params) {
	let index = params.connection;
	let start = params.start;
	let end = params.end;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.discoverCharacteristics(bearer, 0x0001, 0xFFFF, results => {
		for (let result of results) {
			sendMessage({
				notification: "gatt/characteristic",
				connection: index,
				properties: parseCharacteristicProperties(result.properties),
				characteristic: result.handle,
				uuid: result.uuid.toString()
			});
		}
		return true;
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllCharacteristics", errorCode, handle);
	});
};

exports.gattDiscoverAllCharacteristicDescriptors = function (params) {
	let index = params.connection;
	let start = params.start;
	let end = params.end;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.discoverCharacteristicDescriptors(bearer, start, end, results => {
		for (let result of results) {
			sendMessage({
				notification: "gatt/descriptor",
				connection: index,
				descriptor: result.handle,
				uuid: result.uuid.toString()
			});
		}
		return true;
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllCharacteristicDescriptors", errorCode, handle);
	});
};

exports.gattReadCharacteristicValue = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.readValue(bearer, handle).then(result => {
		sendMessage({
			notification: "gatt/characteristic/value",
			connection: index,
			characteristic: handle,
			value: result
		});
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattReadCharacteristicValue", errorCode, handle);
	});
};

exports.gattWriteWithoutResponse = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let value = params.value;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.writeWithoutResponse(bearer, handle, new Uint8Array(value));
};

exports.gattWriteCharacteristicValue = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let value = params.value;
	let gapConn = _gapApplication.getConnection(index);
	if (gapConn == null) {
		return;
	}
	let bearer = gapConn.bearer;
	GATTClient.writeValue(bearer, handle, new Uint8Array(value)).then(() => {
		procedureComplete(index, handle);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattReadCharacteristicValue", errorCode, handle);
	});
};

function parseCharacteristicProperties(properties) {
	let parsed = [];
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

	for (let s = 0; s < params.services.length; s++) {
		let serviceTmp = params.services[s];
		logger.debug("Service#" + s + ": " + serviceTmp.uuid);
		let service = _profile.addService(serviceTmp);
		/* Setup callback */
		for (let characteristic of service.characteristics) {
			characteristic.onValueRead = c => {
				logger.debug("Characteristic value on read: handle="
					+ Utils.toHexString(c.handle, 2));
			};
			characteristic.onValueWrite = c => {
				logger.debug("Characteristic value on write: handle="
					+ Utils.toHexString(c.handle, 2));
				sendMessage({
					notification: "gatt/local/write",
					service: service.uuid,
					characteristic: c.uuid,
					value: c.value
				});
			};
			characteristic.config = new Map();
			let descriptor = characteristic.getDescriptorByUUID(
				GATT.UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);
			if (descriptor != null) {
				descriptor.onValueRead = (d, c) => {
					if (characteristic.config.has(c.handle)) {
						d.value = characteristic.config.get(c.handle);
					} else {
						d.value = 0x0000;
					}
				};
				descriptor.onValueWrite = (d, c) => {
					characteristic.config.set(c.handle, d.value);
				};
			}
		}
	}
	_profile.deploy();

	sendMessage({
		notification: "gatt/services/add",
		services: toAddServiceResults(_profile.services)
	});
};

function toAddServiceResults(services) {
	let results = [];
	for (let s = 0; s < services.length; s++) {
		let service = services[s];
		let result = {
			uuid: service.uuid.toString(),
			characteristics: []
		};
		for (let c = 0; c < service.characteristics.length; c++) {
			let characteristic = service.characteristics[c];
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
	let serviceUUID = UUID.getByString(params.service);
	let characteristicUUID = UUID.getByString(params.characteristic);
	let value = params.value;

	logger.debug("gattWriteLocal");

	/* Find characteristic */
	let service = _profile.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	let characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	/* Update characteristic value */
	characteristic.value = value;

	/* Auto Notify/Indication */
	_gapApplication.forEachConnection(connection => {
		/* Try notification first, then retry with indication. */
		if (!characteristic.notifyValue(connection)) {
			characteristic.indicateValue(connection);
		}
	});
};

exports.gattReadLocal = function (params) {
	let serviceUUID = UUID.getByString(params.service);
	let characteristicUUID = UUID.getByString(params.characteristic);

	logger.debug("gattReadLocal");

	/* Find characteristic */
	let service = _profile.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	let characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	sendMessage({
		notification: "gatt/local/read",
		service: serviceUUID,
		characteristic: characteristicUUID,
		value: characteristic.value
	});
};
