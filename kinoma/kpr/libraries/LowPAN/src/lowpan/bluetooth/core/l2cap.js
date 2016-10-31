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
		let sc = new SignalingContext(mgr.openConnection(signalingCID, signalingCID));

		this._delegate.l2capConnected(mgr, sc);
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
	disconnected(reason) {
		for (let key in this.connections) {
			if (this.connections.hasOwnProperty(key)) {
				this.connections[key].disconnected();
			}
		}
		if (this._delegate != null) {
			this._delegate.disconnected(reason);
		}
	}
	/* HCI Calback (LELink) */
	connectionUpdated(connParameters) {
		if (this._delegate != null) {
			this._delegate.connectionUpdated(connParameters);
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
		this._frameQueue = new Array();
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	get link() {
		return this._connectionManager.link;
	}
	dequeueFrame() {
		if (this._frameQueue.length == 0) {
			return null;
		}
		return this._frameQueue.shift();
	}
	/* L2CAP Callback (ConnectionManager) */
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
			this._frameQueue.push(this._rxBuffer);
			this._rxBuffer = null;
			if (this._delegate != null) {
				this._delegate.received();
			}
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

/******************************************************************************
 * Signaling
 ******************************************************************************/
const Code = {
	COMMAND_REJECT: 0x01,
	CONNECTION_REQUEST: 0x02,
	ECHO_REQUEST: 0x08,
	ECHO_RESPONSE: 0x09,
	CONNECTION_PARAMETER_UPDATE_REQUEST: 0x12,
	CONNECTION_PARAMETER_UPDATE_RESPONSE: 0x13
};

class SignalingContext {
	constructor(connection) {
		this._connection = connection;
		this._sequence = new Utils.Sequence(8);
		this._callbacks = new Map();
		connection.delegate = this;
	}
	allocateBuffer() {
		return ByteBuffer.allocateUint8Array(23, true);
	}
	sendSignalingPacket(code, identifier, packet, callback) {
		while (identifier == 0) {
			identifier = this._sequence.nextSequence();
		}
		let buffer = this.allocateBuffer();
		buffer.putInt8(code);
		buffer.putInt8(identifier);
		buffer.putInt16((packet != null) ? packet.length : 0);
		if (packet != null && packet.length > 0) {
			buffer.putByteArray(packet);
		}
		buffer.flip();
		this._connection.sendBasicFrame(buffer.getByteArray());
		if (callback !== undefined) {
			logger.debug("Callback set for identifier=" + identifier);
			this._callbacks.set(identifier, callback);
		}
	}
	sendCommandReject(identifier, reason, data) {
		let length = (data != null) ? data.length : 0;
		let packet = new Uint8Array(2 + length);
		packet[0] = reason & 0xFF;
		packet[1] = (reason >> 8) & 0xFF;
		for (let i = 0; i < length; i++) {
			packet[i + 2] = data[i];
		}
		this.sendSignalingPacket(Code.COMMAND_REJECT, identifier, packet);
	}
	sendConnectionParameterUpdateRequest(connParameters, callback = null) {
		let request = this.allocateBuffer();
		request.putInt16(connParameters.intervalMin);
		request.putInt16(connParameters.intervalMax);
		request.putInt16(connParameters.latency);
		request.putInt16(connParameters.supervisionTimeout);
		request.flip();
		let packet = request.getByteArray();
		this.sendSignalingPacket(Code.CONNECTION_PARAMETER_UPDATE_REQUEST, 0, packet, callback);
	}
	/** L2CAP Connection delegate method */
	received() {
		let buffer = this._connection.dequeueFrame();
		if (buffer == null) {
			return;
		}
		let code = buffer.getInt8();
		let identifier = buffer.getInt8();
		let length = buffer.getInt16();
		buffer.setLimit(buffer.getPosition() + length);
		switch (code) {
		case Code.ECHO_REQUEST:
			this.sendSignalingPacket(Code.ECHO_RESPONSE, identifier, null);
			return;
		case Code.CONNECTION_PARAMETER_UPDATE_REQUEST:
			{
				let link = this._connection.link;
				if (!link.isLELink() || link.isLESlave()) {
					/* Non LE or LE slave does not support this request */
					this.sendCommandReject(identifier, 0x0000, null);
					return;
				}
				let hci = link._linkMgr.context;	// FIXME
				hci.commands.le.connectionUpdate(
					link.handle,
					{
						intervalMin: buffer.getInt16(),
						intervalMax: buffer.getInt16(),
						latency: buffer.getInt16(),
						supervisionTimeout: buffer.getInt16(),
						minimumCELength: 0,
						maximumCELength: 0
					}
				).then(() => {
					this.sendSignalingPacket(
						Code.CONNECTION_PARAMETER_UPDATE_RESPONSE,
						identifier, Utils.toByteArray(0x0000, 2, true));
				}).catch(status => {
					this.sendSignalingPacket(
						Code.CONNECTION_PARAMETER_UPDATE_RESPONSE,
						identifier, Utils.toByteArray(0x0001, 2, true));
				});
			}
			return;
		default:
			if (this._callbacks.has(identifier)) {
				logger.debug("Callback has been found for identifier=" + identifier);
				let callback = this._callbacks.get(identifier);
				if (callback != null) {
					callback(buffer);
				}
				this._callbacks.delete(identifier);
			} else {
				logger.warn("Unsupported Signaling Command: code=" + Utils.toHexString(code));
				this.sendCommandReject(identifier, 0x0000, null);
			}
		}
	}
	/** L2CAP Connection delegate method */
	disconnected() {
		logger.info("Signaling context disconnected.");
	}
}