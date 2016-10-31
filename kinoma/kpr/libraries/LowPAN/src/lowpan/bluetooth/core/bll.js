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
 * Bluetooth v4.2 - BLL API Bridge
 */

/* Mandatory BLE stack modules */
const GAP = require("./gap");
const GAPADV = require("./gapadvdata");
const UART = require("../transport/uart");
const BTUtils = require("./btutils");
const UUID = BTUtils.UUID;
const BluetoothAddress = BTUtils.BluetoothAddress;

/* Mandatory utilities */
const Utils = require("../../common/utils");
const Logger = Utils.Logger;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

/* Marvell BT chip */
const DEFAULT_UART_DEVICE = "/dev/mbtchar0";
const DEFAULT_UART_BAUDRATE = 115200;

/* API consts */
const LOGGING_ENABLED = false;

let logger = Logger.getLogger("BLL");

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

function pollTransport() {
	let buffer = _serial.read("ArrayBuffer");
	if (buffer.byteLength == 0) {
		logger.trace("serial.read returns 0");
		return;
	}
	_transport.receive(new Uint8Array(buffer), 0, buffer.byteLength);
}

exports.configure = function (data) {
	logger.info("Initializing BLE Stack...");
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
				bond.address = parseAddress(bond.address);
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

function sendMessage(message) {
	if (_notification == null) {
		return;
	}
	if (message.identifier == "discovered" || message.identifier == "attReceived") {
		/* Suppress Logging... */
		logger.trace("Message: " + JSON.stringify(message));
	} else {
		logger.debug("Message: " + JSON.stringify(message));
	}
	_notification.invoke(message);
}

function parseAddress(address) {
	return (address != null) ? BluetoothAddress.getByAddress(address) : null;
}

function serializeAddress(address) {
	return (address != null) ? Array.from(address.toByteArray()) : null;
}

function serializeByteArray(byteArray) {
	return (byteArray != null) ? Array.from(byteArray) : null;
}

function serializeSecurityInfo(securityInfo) {
	if (securityInfo == null) {
		return null;
	}
	return {
		longTermKey: serializeByteArray(securityInfo.longTermKey),
		ediv: securityInfo.ediv,
		random: serializeByteArray(securityInfo.random),
		identityResolvingKey: serializeByteArray(securityInfo.identityResolvingKey),
		address: serializeAddress(securityInfo.address),
		signatureKey: serializeByteArray(securityInfo.signatureKey),
		keySize: securityInfo.keySize,
		legacy: securityInfo.legacy,
		bonding: securityInfo.bonding,
		authenticated: securityInfo.authenticated
	};
}

class BondingStorage extends BTUtils.MemoryBondingStorage {
	constructor() {
		super();
	}
	addBond(index, info) {
		super.addBond(index, info);
		sendMessage({
			identifier: "bondModified",
			index,
			info: serializeSecurityInfo(info)
		});
	}
}

class GAPApplication extends BTUtils.GAPConnectionManager {
	constructor() {
		super();
	}
	/* GAP Callback */
	gapReady() {
		sendMessage({
			identifier: "ready"
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
					identifier: "pairingFailed",
					connection: handle,
					reason
				});
			},
			/* GAP Callback */
			passkeyRequested: input => {
				sendMessage({
					identifier: "passkeyRequested",
					connection: handle,
					input
				});
			},
			/* GAP Callback */
			encryptionCompleted: securityChanged => {
				sendMessage({
					identifier: "encryptionCompleted",
					connection: handle,
					securityChanged,
					securityInfo: securityChanged ? serializeSecurityInfo(connection.securityInfo) : null
				});
			},
			/* GAP Callback */
			encryptionFailed: reason => {
				sendMessage({
					identifier: "encryptionFailed",
					connection: handle,
					reason
				});
			},
			/* GAP Callback */
			disconnected: reason => {
				this.unregisterConnection(handle);
				sendMessage({
					identifier: "disconnected",
					connection: handle,
					reason
				});
			},
			/* GAP Callback */
			connectionUpdated: parameters => {
				sendMessage({
					identifier: "updated",
					connection: handle,
					parameters
				});
			}
		};
		connection.bearer = {
			received: buffer => {
				sendMessage({
					identifier: "attReceived",
					connection: handle,
					buffer: buffer.getByteArray().buffer
				});
			}
		};
		sendMessage({
			identifier: "connected",
			connection: connection.handle,
			peripheral: connection.peripheral,
			address: serializeAddress(connection.address),
			parameters: connection.parameters,
			securityInfo: serializeSecurityInfo(connection.securityInfo)
		});
	}
	/* GAP Callback */
	gapDiscovered(device) {
		let message = {
			identifier: "discovered",
			address: serializeAddress(device.address),
			rssi: device.rssi
		};
		if (device.hasOwnProperty("scanResponse")) {
			message.scanResponse = GAPADV.parse(device.scanResponse);
		} else {
			message.directed = device.directed;
			message.connectable = device.connectable;
			message.scannable = device.scannable;
			if (device.directed) {
				message.directAddress = serializeAddress(device.directAddress);
			} else {
				message.advertising = GAPADV.parse(device.advertising);
			}
		}
		sendMessage(message);
	}
	/* GAP Callback */
	privacyEnabled(privateAddress) {
		sendMessage({
			identifier: "privacyEnabled",
			address: serializeAddress(privateAddress)
		});
	}
}

/******************************************************************************
 * API Calls
 ******************************************************************************/

exports.startScanning = ({parameters}) => {
	_gap.startScanning(parameters);
};

exports.stopScanning = () => {
	_gap.stopScanning();
};

exports.establishConnection = ({address, parameters}) => {
	_gap.establishConnection(parseAddress(address), parameters);
};

exports.startAdvertising = ({parameters}) => {
	if (parameters.hasOwnProperty("advertising")) {
		parameters.advertising = GAPADV.serialize(parameters.advertising);
	}
	if (parameters.hasOwnProperty("scanResponse")) {
		parameters.scanResponse = GAPADV.serialize(parameters.scanResponse);
	}
	if (parameters.hasOwnProperty("address")) {
		parameters.address = parseAddress(parameters.address);
	}
	_gap.startAdvertising(parameters);
};

exports.stopAdvertising = () => {
	_gap.stopAdvertising();
};

exports.enablePrivacy = () => {
	_gap.enablePrivacyFeature(true);
};

exports.disablePrivacy = () => {
	_gap.enablePrivacyFeature(false);
};

exports.setWhiteList = ({addresses}) => {
	let a = new Array();
	for (let address of addresses) {
		a.push(parseAddress(address));
	}
	_gap.setWhiteList(a);
};

exports.updateConnection = ({connection, parameters, l2cap}) => {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.updateConnectionParameter(parameters, l2cap);
};

exports.disconnect = ({connection, reason}) => {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.disconnect(reason);
};

exports.startAuthentication = ({connection}) => {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.startAuthentication();
};

exports.setSecurityParameter = ({connection, parameter}) => {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.setSecurityParameter(parameter);
};

exports.passkeyEntry = ({connection, passkey}) => {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.passkeyEntry(passkey);
};

exports.readRSSI = function ({connection}) {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	_gap._hci.commands.status.readRSSI(gapConn.handle).then(response => {
		let handle = response.getInt16();
		let rssi = Utils.toSignedByte(response.getInt8());
		sendMessage({
			notification: "rssi",
			connection: handle,
			rssi: rssi
		});
	});
};

exports.sendAttributePDU = function ({connection, buffer}) {
	let gapConn = _gapApplication.getConnection(connection);
	if (gapConn == null) {
		return;
	}
	gapConn.sendAttributePDU(new Uint8Array(buffer));
};