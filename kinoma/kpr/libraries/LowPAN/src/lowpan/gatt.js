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

import Pins from "pins";

import Buffers from "./common/buffers";
import Utils from "./common/utils";

import GATT from "./bluetooth/core/gatt";
import GATTServer from "./bluetooth/gatt/server";
import GATTClient from "./bluetooth/gatt/client";
//import BTUtils from "./bluetooth/core/btutils";
const BTUtils = require("./bluetooth/core/btutils");

export const BluetoothAddress = BTUtils.BluetoothAddress;
export const UUID = BTUtils.UUID;

const ByteBuffer = Buffers.ByteBuffer;
const Logger = Utils.Logger;

const DEFAULT_BONDING_FILE_NAME = "ble_bondings.json";

/* Default Scanning Parameters: General Discovery */
const DEFAULT_SCANNING_PARAMETERS = {
	observer: false,
	duplicatesFilter: true
};

/* Default Advertising Parameters: General Discoverable + Undirected Connectable */
const DEFAULT_ADVERTISING_PARAMETERS = {
	discoverable: true,
	conneactable: true
};

let logger = Logger.getLogger("BLE");

export default class BLE {
	constructor(clearBondings = false) {
		this._bll = "";
		this._server = new GATTServer.Profile();
		this._connections = new Map();
		this._ready = false;
		this._bondingFileURI = mergeURI(Files.documentsDirectory, DEFAULT_BONDING_FILE_NAME);
		if (!clearBondings && Files.exists(this._bondingFileURI)) {
			this._bondingDB = Files.readJSON(this._bondingFileURI);
		} else {
			this._bondingDB = new Array();
		}
		this._ccConfigs = new Map();
		/* Event Handlers */
		this._onReady = null;
		this._onConnected = null;
		this._onDiscovered = null;
		this._onPrivacyEnabled = null;
	}
	get configuration() {
		return {
			require: "/lowpan/bluetooth/core/bll",
			bondings: this._bondingDB,
			logging: true,
			loggers: [
		//		{ name: "BLL", level: "DEBUG" },
		//		{ name: "GAP", level: "DEBUG" },
		//		{ name: "SM", level: "DEBUG" },
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
		Pins.when(bll, "notification", response => this.onNotification(response));
	}
	invoke(name, arg) {
		Pins.invoke("/" + this._bll + "/" + name, arg);
	}
	get bll() {
		return this._bll;
	}
	get server() {
		return this._server;
	}
	startScanning(parameters = DEFAULT_SCANNING_PARAMETERS) {
		this.invoke("startScanning", {parameters});
	}
	stopScanning() {
		this.invoke("stopScanning");
	}
	connect(address = null, parameters = null) {
		this.invoke("establishConnection", {
			address: (address != null) ? Array.from(address.toByteArray()) : null,
			parameters
		});
	}
	startAdvertising(parameters = DEFAULT_ADVERTISING_PARAMETERS) {
		this.invoke("startAdvertising", {parameters});
	}
	stopAdvertising() {
		this.invoke("stopAdvertising");
	}
	enablePrivacy() {
		this.invoke("enablePrivacy");
	}
	disablePrivacy() {
		this.invoke("disablePrivacy");
	}
	setWhiteList(addresses) {
		let a = new Array();
		for (let address of addresses) {
			a.push(address.toByteArray());
		}
		this.invoke("setWhiteList", {
			addresses: a
		});
	}
	getClientConfigurations(connection) {
		let address = connection.address;
		let key = address.toString() + "/" + address.typeString;
		if (!this._ccConfigs.has(key)) {
			this._ccConfigs.set(key, new Map());
		}
		return this._ccConfigs.get(key);
	}
	removeClientConfigurations(connection) {
		let address = connection.address;
		let key = address.toString() + "/" + address.typeString;
		if (connection.securityInfo != null && connection.securityInfo.bonding && address.isIdentity()) {
			logger.debug("CCC will be kept");
			return;
		}
		this._ccConfigs.delete(key);
		logger.debug("CCC for " + key + " has been deleted");
		// TODO: Persistence
	}
	saveClientConfigurations(connection) {
		logger.debug("CCC for " + key + " has been written");
		// TODO: Persistence
	}
	onNotification(message) {
		if (message.hasOwnProperty("connection")) {
			let connection = this._connections.get(message.connection);
			if (connection !== undefined) {
				connection.onNotification(message);
			}
		}
		switch (message.identifier) {
		case "ready":
			this._ready = true;
			if (this._onReady != null) {
				this._onReady();
			}
			break;
		case "connected":
			{
				let connection = new BLEConnection(this, message);
				this._connections.set(message.connection, connection);
				if (this._onConnected != null) {
					this._onConnected(connection);
				}
			}
			break;
		case "discovered":
			if (this._onDiscovered != null) {
				message.address = BluetoothAddress.getByAddress(message.address);
				if (message.hasOwnProperty("directAddress")) {
					message.directAddress = BluetoothAddress.getByAddress(message.directAddress);
				}
				this._onDiscovered(message);
			}
			break;
		case "privacyEnabled":
			if (this._onPrivacyEnabled != null) {
				this._onPrivacyEnabled(BluetoothAddress.getByAddress(message.address));
			}
			break;
		case "disconnected":
			this._connections.delete(message.connection);
			break;
		case "bondModified":
			this._bondingDB[message.index] = message.hasOwnProperty("info") ? message.info : null;
			Files.writeJSON(this._bondingFileURI, this._bondingDB);
		}
	}
}

class ATTBearer extends GATT.ATT.ATTBearer {
	constructor(ble, connection, database) {
		super(connection, database);
		this._ble = ble;
	}
	readClientConfiguration(characteristic, connection) {
		let uuid = characteristic.uuid.toString();
		let configs = this._ble.getClientConfigurations(connection);
		if (!configs.has(uuid)) {
			return 0x0000;
		}
		return configs.get(uuid);
	}
	writeClientConfiguration(characteristic, connection, value) {
		let uuid = characteristic.uuid.toString();
		let configs = this._ble.getClientConfigurations(connection);
		configs.set(uuid, value);
		this._ble.saveClientConfigurations(connection);
	}
}

class BLEConnection {
	constructor(ble, info) {
		this._ble = ble;
		this._info = info;
		this._parameters = info.parameters;
		this._securityInfo = info.securityInfo;
		this._encrypted = false;
		/* ATT & GATT */
		this._bearer = new ATTBearer(ble, this, ble.server.database);
		this._client = new GATTClient.Profile(this._bearer);
		/* Event Handlers */
		this._onPasskeyRequested = null;
		this._onAuthenticationCompleted = null;
		this._onAuthenticationFailed = null;
		this._onDisconnected = null;
		this._onUpdated = null;
	}
	set onPasskeyRequested(cb) {
		this._onPasskeyRequested = cb;
	}
	set onAuthenticationCompleted(cb) {
		this._onAuthenticationCompleted = cb;
	}
	set onAuthenticationFailed(cb) {
		this._onAuthenticationFailed = cb;
	}
	set onDisconnected(cb) {
		this._onDisconnected = cb;
	}
	set onUpdated(cb) {
		this._onUpdated = cb;
	}
	get bearer() {
		return this._bearer;
	}
	set bearer(bearer) {
		this._bearer = bearer;
	}
	get client() {
		return this._client;
	}
	get handle() {
		return this._info.connection;
	}
	get peripheral() {
		return this._info.peripheral;
	}
	get parameters() {
		return this._parameters;
	}
	get address() {
		return BluetoothAddress.getByAddress(this._info.address);
	}
	get identity() {
		if (this._securityInfo != null && this._securityInfo.address != null) {
			return this._securityInfo.address;
		}
		let address = this.address;
		if (address.isIdentity()) {
			return address;
		}
		return null;
	}
	get encrypted() {
		return this._encrypted;
	}
	get securityInfo() {
		return this._securityInfo;
	}
	isPeripheral() {
		return this._info.peripheral;
	}
	updateConnection(parameters, l2cap) {
		this._ble.invoke("updateConnection", {
			connection: this._info.connection,
			parameters,
			l2cap
		});
	}
	disconnect(reason) {
		this._ble.invoke("disconnect", {
			connection: this._info.connection,
			reason
		});
	}
	startAuthentication() {
		this._ble.invoke("startAuthentication", {
			connection: this._info.connection
		});
	}
	setSecurityParameter(parameter) {
		this._ble.invoke("setSecurityParameter", {
			connection: this._info.connection,
			parameter
		});
	}
	passkeyEntry(passkey) {
		this._ble.invoke("passkeyEntry", {
			connection: this._info.connection,
			passkey
		});
	}
	sendAttributePDU(data) {
		this._ble.invoke("sendAttributePDU", {
			connection: this._info.connection,
			buffer: data.buffer
		});
	}
	onNotification(message) {
		switch (message.identifier) {
		case "attReceived":
			if (this._bearer != null) {
				this._bearer.received(ByteBuffer.wrap(new Uint8Array(message.buffer)));
			}
			break;
		case "passkeyRequested":
			if (this._onPasskeyRequested != null) {
				this._onPasskeyRequested(message.input);
			}
			break;
		case "encryptionCompleted":
			this._encrypted = true;
			if (message.securityChanged) {
				logger.debug("securityInfo is updated on Proxy ATT connection");
				if (message.securityInfo.address != null) {
					message.securityInfo.address =
						BluetoothAddress.getByAddress(message.securityInfo.address);
				}
				this._securityInfo = message.securityInfo;
			}
			if (this._onAuthenticationCompleted != null) {
				this._onAuthenticationCompleted(message.securityChanged);
			}
			break;
		case "pairingFailed":
			if (this._onAuthenticationFailed != null) {
				this._onAuthenticationFailed(message.reason, true);
			}
			break;
		case "encryptionFailed":
			if (this._onAuthenticationFailed != null) {
				this._onAuthenticationFailed(message.reason, false);
			}
			break;
		case "disconnected":
			if (this._onDisconnected != null) {
				this._onDisconnected(message.reason);
			}
			break;
		case "updated":
			this._parameters = message.parameters;
			if (this._onUpdated != null) {
				this._onUpdated(message.parameters);
			}
			break;
		case "rssi":
			break;
		}
	}
}

export {GATT};