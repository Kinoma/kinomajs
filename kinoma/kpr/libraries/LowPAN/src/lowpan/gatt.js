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
const UUID = require("./bluetooth/core/btutils").UUID;

const Buffers = require("./common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const Pins = require("pins");

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

class API {
	constructor(bll) {
		this._bll = bll;
		this._server = new GATTServer.Profile();
		this._contexts = {};
	}
	get server() {
		return this._server;
	}
	getContext(id) {
		if (this._contexts.hasOwnProperty(id)) {
			return this._contexts[id];
		}
		return null;
	}
	getClient(id) {
		let context = this.getContext(id);
		if (context != null) {
			return context.client;
		}
		return null;
	}
	getBearer(id) {
		let context = this.getContext(id);
		if (context != null) {
			return context.bearer;
		}
		return null;
	}
	onNotification(response) {
		let notification = response.notification;
		if ("gap/connect" == notification) {
			let id = response.connection;
			let connection = new ProxyATTConnection(this._bll, id);
			let bearer = new ATT.ATTBearer(connection, this._server.database);
			this._contexts[id] = {
				connection,
				bearer,
				client: new GATTClient.Profile(bearer)
			};
		} else if ("gap/disconnect" == notification) {
			let context = this.getContext(response.connection);
			if (context != null) {
				context.connection.disconnected();
			}
		} else if ("att/received" == notification) {
			let context = this.getContext(response.connection);
			if (context != null) {
				context.connection.received(new Uint8Array(response.buffer));
			}
		} else if ("sm/encryption/complete" == notification) {
			let context = this.getContext(response.connection);
			if (context != null) {
				context.connection._encrypted = true;
				context.connection.pairingInfo = response.pairingInfo;
			}
		} else if ("hci/encrypted" == notification) {
			// TODO
		} else if ("hci/rand" == notification) {
			// TODO
		}
	}
}

module.exports = GATT;
module.exports.Server = GATTServer;
module.exports.Client = GATTClient;
module.exports.UUID = UUID;
module.exports.API = API;
