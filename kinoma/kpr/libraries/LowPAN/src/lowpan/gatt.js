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
 * Bluetooth v4.2 - Public GATT API
 *
 */

const GATT = require("./bluetooth/core/gatt");
const ATT = GATT.ATT;
const GATTServer = require("./bluetooth/gatt/server");
const GATTClient = require("./bluetooth/gatt/client");
const BTUtils = require("./bluetooth/core/btutils");
const BluetoothAddress = BTUtils.BluetoothAddress;
const UUID = BTUtils.UUID;

const Buffers = require("./common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const Pins = require("pins");

const DEFAULT_BONDING_FILE_NAME = "ble_bondigs.json";

class ProxyATTConnection {
	constructor(bll, id) {
		this._bll = bll;
		this._id = id;
		this._onReceived = null;
		this._pairingInfo = null;
		this._encrypted = false;
	}
	set onReceived(callback) {
		this._onReceived = callback;
	}
	get identifier() {
		return this._id;
	}
	get pairingInfo() {
		return this._pairingInfo;
	}
	set pairingInfo(info) {
		this._pairingInfo = info;
	}
	get encrypted() {
		return this._encrypted;
	}
	sendAttributePDU(data) {
		Pins.invoke("/" + this._bll + "/attSendAttributePDU", {
			connection: this._id,
			buffer: data.buffer
		});
	}
	received(data) {
		this._onReceived(ByteBuffer.wrap(data));
	}
	disconnected() {
	//	logger.info("Disconnected: pendingPDUs=" + this.pendingTransactions.length);
	}
}

function getAddressFromResponse(response) {
	if (response.hasOwnProperty("address")) {
		return BluetoothAddress.getByString(
			response.address, true, response.addressType != "public");
	}
	return null;
}

class BLE {
	constructor() {
		this._bll = "";
		this._server = new GATTServer.Profile();
		this._connections = new Map();
		this._ready = false;
		this._bondingFileURI = mergeURI(Files.documentsDirectory, DEFAULT_BONDING_FILE_NAME);
		if (Files.exists(this._bondingFileURI)) {
			this._bondingDB = Files.readJSON(this._bondingFileURI);
		} else {
			this._bondingDB = new Array();
		}
		/* Event Handlers */
		this._onReady = null;
		this._onConnected = null;
		this._onDiscovered = null;
		this._onPrivacyEnabled = null;
	}
	get configuration() {
		return {
			require: "/lowpan/ble",
			mode: "advanced",
			bondings: this._bondingDB,
		//	logging: true,
			loggers: [
		//		{ name: "BLE", level: "TRACE" },
		//		{ name: "GAP", level: "DEBUG" },
		//		{ name: "L2CAP", level: "DEBUG" },
		//		{ name: "HCI", level: "TRACE" },
		//		{ name: "HCIComm", level: "TRACE" },
			]
		};
	}
	set onReady(cb) {
		this._onReady = cb;
	}
	set onConnected(cb) {
		this._onConnected = cb;
	}
	set onDiscovered(cb) {
		this._onDiscovered = cb;
	}
	set onPrivacyEnabled(cb) {
		this._onPrivacyEnabled = cb;
	}
	isReady() {
		return this._ready;
	}
	init(bll) {
		this._bll = bll;
		Pins.when("ble", "notification", response => this.onNotification(response));
	}
	get bll() {
		return this._bll;
	}
	get server() {
		return this._server;
	}
	startScanning() {
		Pins.invoke("/ble/gapStartScanning");
	}
	stopScanning() {
		Pins.invoke("/ble/gapStopScanning");
	}
	connect(address) {
		Pins.invoke("/ble/gapConnect", {
			address: address.toString(),
			addressType: address.typeString
		});
	}
	setScanResponseData(data) {
		Pins.invoke("/ble/gapSetScanResponseData", data);
	}
	startAdvertising(parameter) {
		Pins.invoke("/ble/gapStartAdvertising", parameter);
	}
	stopAdvertising() {
		Pins.invoke("/ble/gapStopAdvertising");
	}
	enablePrivacy() {
		Pins.invoke("/ble/gapEnablePrivacy");
	}
	deleteBonding(bond) {
		Pins.invoke("/ble/gapDeleteBond", {
			index: bond
		});
	}
	onNotification(response) {
		if (response.hasOwnProperty("connection")) {
			let connection = this._connections.get(response.connection);
			if (connection !== undefined) {
				connection.onNotification(response);
			}
		}
		let notification = response.notification;
		switch (notification) {
		case "system/reset":
			this._ready = true;
			if (this._onReady != null) {
				this._onReady();
			}
			break;
		case "gap/connect":
			{
				let connection = new BLEConnection(this, response);
				this._connections.set(response.connection, connection);
				if (this._onConnected != null) {
					this._onConnected(connection);
				}
			}
			break;
		case "gap/discover":
			if (this._onDiscovered != null) {
				let device = {
					address: getAddressFromResponse(response),
					data: response.data,
					type: response.type,
					rssi: response.rssi
				};
				this._onDiscovered(device);
			}
			break;
		case "gap/privacy/complete":
			if (this._onPrivacyEnabled != null) {
				this._onPrivacyEnabled(BluetoothAddress.getByString(response.rpa, true, true));
			}
			break;
		case "gap/disconnect":
			this._connections.delete(response.connection);
			break;
		case "gap/bond/add":
		case "gap/bond/remove":
			this._bondingDB[response.index] = response.hasOwnProperty("info") ? response.info : null;
			Files.writeJSON(this._bondingFileURI, this._bondingDB);
		}
	}
}

class BLEConnection {
	constructor(ble, info) {
		this._ble = ble;
		this._info = info;
		this._proxy = new ProxyATTConnection(ble.bll, info.connection);
		/* ATT & GATT */
		this._bearer = new ATT.ATTBearer(this._proxy, ble.server.database);
		this._client = new GATTClient.Profile(this._bearer);
		/* SM */
		this._bond = info.bond;
		/* Event Handlers */
		this._onPasskeyRequested = null;
		this._onEncryptionCompleted = null;
		this._onEncryptionFailed = null;
		this._onDisconnected = null;
	}
	set onPasskeyRequested(cb) {
		this._onPasskeyRequested = cb;
	}
	set onEncryptionCompleted(cb) {
		this._onEncryptionCompleted = cb;
	}
	set onEncryptionFailed(cb) {
		this._onEncryptionCompleted = cb;
	}
	set onDisconnected(cb) {
		this._onDisconnected = cb;
	}
	get bond() {
		return this._bond;
	}
	get client() {
		return this._client;
	}
	get address() {
		return getAddressFromResponse(this._info);
	}
	isPeripheral() {
		return this._info.peripheral;
	}
	updateConnection(parameter) {
		parameter.connection = this._info.connection;
		Pins.invoke("/ble/gapUpdateConnection", parameter);
	}
	disconnect() {
		Pins.invoke("/ble/gapDisconnect", {
			connection: this._info.connection
		});
	}
	startEncryption() {
		Pins.invoke("/ble/smStartEncryption", {
			connection: this._info.connection
		});
	}
	setSecurityParameter(parameter) {
		parameter.connection = this._info.connection;
		Pins.invoke("/ble/smSetSecurityParameter", parameter);
	}
	passkeyEntry(passkey) {
		Pins.invoke("/ble/smPasskeyEntry", {
			connection: this._info.connection,
			passkey
		});
	}
	onNotification(response) {
		let notification = response.notification;
		switch (notification) {
		case "att/received":
		//	this._proxy.received(new Uint8Array(response.buffer));
			this._proxy.received(response.array);
			break;
		case "sm/passkey":
			if (this._onPasskeyRequested != null) {
				this._onPasskeyRequested(response.input);
			}
			break;
		case "sm/encryption/complete":
			this._bond = response.bond;
			if (this._onEncryptionCompleted != null) {
				this._onEncryptionCompleted();
			}
			break;
		// TODO: onEncryptionFailed
		case "gap/disconnect":
			if (this._onDisconnected != null) {
				this._onDisconnected(response.reason);
			}
			break;
		}
	}
}

module.exports = GATT;
module.exports.Server = GATTServer;
module.exports.Client = GATTClient;
module.exports.UUID = UUID;
module.exports.BLE = BLE;