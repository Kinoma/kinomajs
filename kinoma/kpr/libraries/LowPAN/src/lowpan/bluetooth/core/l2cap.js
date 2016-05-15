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
 * Bluetooth v4.2 - L2CAP (LE Only)
 */

const Utils = require("../../common/utils");
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const L2CAPSignalingContext = require("../l2cap/signaling").L2CAPSignalingContext;

var logger = new Utils.Logger("L2CAP");
logger.loggingLevel = Utils.Logger.Level.INFO;

const Channel = {
	NULL: 0x0000,
	L2CAP_SIGNALING: 0x0001
};
exports.Channel = Channel;

const LEChannel = {
	NULL: 0x0000,
	ATTRIBUTE_PROTOCOL: 0x0004,
	LE_L2CAP_SIGNALING: 0x0005,
	SECURITY_MANAGER_PROTOCOL: 0x0006
};
exports.LEChannel = LEChannel;

function toKeyString(cid) {
	return Utils.toHexString(cid, 2, "");
}

exports.createLayer = (hci) => {
	logger.info("Init");
	return new Context(hci);
};

class Context {
	constructor(hci) {
		this._hci = hci;
		this._delegate = null;
		hci.delegate = this;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	/* HCI Callback (HCIContext) */
	hciReady() {
		logger.debug("HCI Ready");
		this._delegate.l2capReady();
	}
	/* HCI Callback (HCIContext) */
	hciConnected(link) {
		logger.info("HCI Connected: handle=" + Utils.toHexString(link.handle, 2)
			+ ", address=" + link.remoteAddress.toString() + ", type=" + link.remoteAddress.typeString);
		let mgr = new ConnectionManager(link);

		let signalingCID = LEChannel.LE_L2CAP_SIGNALING;
		let sc = new L2CAPSignalingContext(mgr.openConnection(signalingCID, signalingCID));

		this._delegate.l2capConnected(mgr);
	}
}

class ConnectionManager {
	constructor(link) {
		this._link = link;
		this.connections = {};
		this.currentChannel = -1;
		link.delegate = this;
		this._delegate = null;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	get link() {
		return this._link;
	}
	openConnection(srcCID, dstCID) {
		let cidStr = toKeyString(srcCID);
		logger.debug("Open Connection: src=" + cidStr);
		if (cidStr in this.connections) {
			logger.debug("Connection has already been opened");
			return null;
		}
		let connection = new Connection(this, srcCID, dstCID);
		this.connections[cidStr] = connection;
		return connection;
	}
	getConnection(cid) {
		return this.connections[toKeyString(cid)];
	}
	/* HCI Callback (ACLLink) */
	received(data, firstFragment) {
		let fragment;
		let length = -1;
		if (firstFragment) {
			// TODO: Drop incomplete handleSignalingPacket
			length = Utils.toInt(data, 0, 2, true);
			let cid = Utils.toInt(data, 2, 2, true);
			logger.trace("First flushable packet: cid=" + toKeyString(cid)
				+ ", length=" + length);
			this.currentChannel = cid;
			fragment = data.slice(4);
		} else {
			logger.trace("Continuing fragment");
			fragment = data;
		}

		if (this.currentChannel > 0) {
			let key = toKeyString(this.currentChannel);
			if (!(key in this.connections)) {
				logger.warn("Discard packet for unavailable channel cid=" + key);
				return;
			}
			let connection = this.connections[key];
			if (length > 0) {
				connection.allocateReceiveBuffer(length);
			}
			connection.received(fragment);
		}
	}
	/* HCI Callback (ACLLink) */
	disconnected() {
		this._link = null;
		for (let key in this.connections) {
			if (this.connections.hasOwnProperty(key)) {
				this.connections[key].disconnected();
			}
		}
		if (this._delegate != null) {
			this._delegate.disconnected();
		}
	}
}

class Connection {
	constructor(connectionManager, sourceChannel, destinationChannel) {
		this._connectionManager = connectionManager;
		this._sourceChannel = sourceChannel;
		this._destinationChannel = destinationChannel;
		this._rxBuffer = null;
		this._delegate = null;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	get link() {
		return this._connectionManager.link;
	}
	allocateReceiveBuffer(length) {
		logger.trace("Start receiving length=" + length);
		this._rxBuffer = ByteBuffer.allocateUint8Array(length, true);
	}
	/* L2CAP Callback (ConnectionManager) */
	received(data) {
		logger.trace("Fragment received: len=" + data.length);
		if (this._rxBuffer == null) {
			logger.warn("RX buffer is null");
			return;
		}
		this._rxBuffer.putByteArray(data);
		if (this._rxBuffer.remaining() == 0) {
			logger.debug("Receiving B-Frame: src="
				+ Utils.toHexString(this._destinationChannel, 2)
				+ " dst=" + Utils.toHexString(this._sourceChannel, 2)
				+ " " + Utils.toFrameString(this._rxBuffer.array));
			this._rxBuffer.flip();
			if (this._delegate != null) {
				this._delegate.received(this._rxBuffer);
			} else {
				logger.warn("Dropping frame");
			}
			this._rxBuffer = null;
		}
	}
	sendBasicFrame(data) {
		let buffer = ByteBuffer.allocateUint8Array(data.length + 4, true);
		buffer.putInt16(data.length);
		buffer.putInt16(this._destinationChannel);
		buffer.putByteArray(data);
		buffer.flip();
		logger.debug("Submit B-Frame: src="
			+ Utils.toHexString(this._sourceChannel, 2)
			+ " dst=" + Utils.toHexString(this._destinationChannel, 2)
			+ " " + Utils.toFrameString(data));
		this.link.send(buffer);
	}
	disconnected() {
		if (this._delegate != null) {
			this._delegate.disconnected();
		} else {
			logger.info("Connection has been disconnected: cid="
				+ Utils.toHexString(this._sourceChannel, 2));
		}
	}
}
