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

var Utils = require("/lowpan/common/utils");
var Buffers = require("/lowpan/common/buffers");
var ByteBuffer = Buffers.ByteBuffer;

var logger = new Utils.Logger("L2CAP");
logger.loggingLevel = Utils.Logger.Level.INFO;

var Channel = {
	NULL: 0x0000,
	L2CAP_SIGNALING: 0x0001
};
exports.Channel = Channel;

var LEChannel = {
	NULL: 0x0000,
	ATTRIBUTE_PROTOCOL: 0x0004,
	LE_L2CAP_SIGNALING: 0x0005,
	SECURITY_MANAGER_PROTOCOL: 0x0006
};
exports.LEChannel = LEChannel;

/** Lower HCI layer instance/module */
var _hci = null;

/** Upper layer instance/module (e.g. GAP) */
var _delegate = null;

exports.registerHCI = function (hci) {
	_hci = hci;
	_hci.registerDelegate({
		hciReady: function () {
			logger.debug("HCI Ready");
			_delegate.l2capReady();
		},
		hciConnected: function (link) {
			logger.info("HCI Connected: handle=" + Utils.toHexString(link.handle, 2)
				+ ", address=" + link.remoteAddress.toString() + ", identity=" + link.remoteAddress.isIdentity());
			initL2CAP(link);
		}
	});
};

exports.registerDelegate = function (delegate) {
	_delegate = delegate;
};

function toKeyString(cid) {
	return Utils.toHexString(cid, 2, "");
}

function initL2CAP(link) {
	logger.debug("Init L2CAP for link: handle=" + Utils.toHexString(link.handle, 2));
	var mgr = new ConnectionManager(link);

	var signalingCID = LEChannel.LE_L2CAP_SIGNALING;
	var connection = mgr.openConnection(signalingCID, signalingCID);
	connection.delegate = new L2CAPSignalingContext(connection);

	_delegate.l2capConnected(mgr);
}

/******************************************************************************
 * L2CAP Connection Management
 ******************************************************************************/
function ConnectionManager(link) {
	this.link = link;
	this.connections = {};
	this.currentChannel = -1;
	link.delegate = this;
	this.delegate = null;
}

ConnectionManager.prototype.openConnection = function (srcCID, dstCID) {
	var cidStr = toKeyString(srcCID);
	logger.debug("Open Connection: src=" + cidStr);
	if (cidStr in this.connections) {
		logger.debug("Connection has already been opened");
		return null;
	}
	var connection = new Connection(this, srcCID, dstCID);
	this.connections[cidStr] = connection;
	return connection;
};

ConnectionManager.prototype.getConnection = function (cid) {
	return this.connections[toKeyString(cid)];
};

ConnectionManager.prototype.getHCILink = function () {
	return this.link;
};

ConnectionManager.prototype.received = function (data, firstFragment) {
	var fragment;
	var length = -1;
	if (firstFragment) {
		// TODO: Drop incomplete handleSignalingPacket
		length = Utils.toInt(data, 0, 2, true);
		var cid = Utils.toInt(data, 2, 2, true);
		logger.trace("First flushable packet: cid=" + toKeyString(cid)
			+ ", length=" + length);
		this.currentChannel = cid;
		fragment = data.slice(4);
	} else {
		logger.trace("Continuing fragment");
		fragment = data;
	}

	if (this.currentChannel > 0) {
		var key = toKeyString(this.currentChannel);
		if (!(key in this.connections)) {
			logger.warn("Discard packet for unavailable channel cid=" + key);
			return;
		}
		var connection = this.connections[key];
		if (length > 0) {
			connection.allocateReceiveBuffer(length);
		}
		connection.received(fragment);
	}
};

ConnectionManager.prototype.disconnected = function () {
	this.link = null;
	for (var key in this.connections) {
		if (this.connections.hasOwnProperty(key)) {
			this.connections[key].disconnected();
		}
	}
	if (this.delegate != null) {
		this.delegate.disconnected();
	}
};

/******************************************************************************
 * L2CAP Connection
 ******************************************************************************/
function Connection(connectionManager, sourceChannel, destinationChannel) {
	this.connectionManager = connectionManager;
	this.sourceChannel = sourceChannel;
	this.destinationChannel = destinationChannel;
	this.rxBuffer = null;
	this.delegate = null;
}

Connection.prototype.getHCILink = function () {
	return this.connectionManager.link;
};

Connection.prototype.allocateReceiveBuffer = function (length) {
	logger.trace("Start receiving length=" + length);
	this.rxBuffer = ByteBuffer.allocateUint8Array(length, true);
};

Connection.prototype.received = function (data) {
	logger.trace("Fragment received: len=" + data.length);
	if (this.rxBuffer == null) {
		logger.warn("RX buffer is null");
		return;
	}
	this.rxBuffer.putByteArray(data);
	if (this.rxBuffer.remaining() == 0) {
		logger.debug("Receiving B-Frame: src="
			+ Utils.toHexString(this.destinationChannel, 2)
			+ " dst=" + Utils.toHexString(this.sourceChannel, 2)
			+ " " + Utils.toFrameString(this.rxBuffer.array));
		this.rxBuffer.flip();
		if (this.delegate != null) {
			this.delegate.received(this.rxBuffer);
		} else {
			logger.warn("Dropping frame");
		}
		this.rxBuffer = null;
	}
};

Connection.prototype.sendBasicFrame = function (data) {
	var buffer = ByteBuffer.allocateUint8Array(data.length + 4, true);
	buffer.putInt16(data.length);
	buffer.putInt16(this.destinationChannel);
	buffer.putByteArray(data);
	buffer.flip();
	logger.debug("Submit B-Frame: src="
		+ Utils.toHexString(this.sourceChannel, 2)
		+ " dst=" + Utils.toHexString(this.destinationChannel, 2)
		+ " " + Utils.toFrameString(data));
	this.connectionManager.link.send(buffer);
};

Connection.prototype.disconnected = function () {
	if (this.delegate != null) {
		this.delegate.disconnected();
	} else {
		logger.info("Connection has been disconnected: cid="
			+ Utils.toHexString(this.sourceChannel, 2));
	}
};

/******************************************************************************
 * L2CAP Signaling Channel
 ******************************************************************************/
var Code = {
	COMMAND_REJECT: 0x01,
	CONNECTION_REQUEST: 0x02,
	ECHO_REQUEST: 0x08,
	ECHO_RESPONSE: 0x09,
	CONNECTION_PARAMETER_UPDATE_REQUEST: 0x12,
	CONNECTION_PARAMETER_UPDATE_RESPONES: 0x13
};

var signalingRequestHandler = {};

function L2CAPSignalingContext(connection) {
	this.connection = connection;
}

/** L2CAP Connection delegate method */
L2CAPSignalingContext.prototype.received = function (buffer) {
	var code = buffer.getInt8();
	var identifier = buffer.getInt8();
	var length = buffer.getInt16();
	buffer.setLimit(buffer.getPosition() + length);

	var respBuffer = ByteBuffer.allocateUint8Array(23);
	handleSignalingPacket(buffer, respBuffer, code, identifier, this.connection);
	if (respBuffer.getPosition() > 0) {
		respBuffer.flip();
		this.connection.sendBasicFrame(respBuffer.getByteArray());
	}
};

/** L2CAP Connection delegate method */
L2CAPSignalingContext.prototype.disconnected = function () {
	logger.info("Signaling context disconnected.");
};

function handleSignalingPacket(request, response, code, identifier, connection) {
	if (!signalingRequestHandler.hasOwnProperty(code)) {
		logger.warn("Unsupported Signaling Command: code=" + Utils.toHexString(code));
		doCommandReject(response, identifier, 0x0000, null);
	} else {
		signalingRequestHandler[code](request, response, identifier, connection);
	}
}

function doCommandReject(response, identifier, reason, data) {
	response.putInt8(Code.COMMAND_REJECT);
	response.putInt8(identifier);
	response.putInt16(2 + (data != null) ? data.length : 0);
	response.putInt16(reason);
	if (data != null && data.length > 0) {
		response.putByteArray(data);
	}
}

signalingRequestHandler[Code.ECHO_REQUEST] = function (request, response, identifier, connection) {
	response.putInt8(Code.ECHO_RESPONSE);
	response.putInt8(identifier);
	response.putInt16(0);	// No data at all
};

signalingRequestHandler[Code.CONNECTION_PARAMETER_UPDATE_REQUEST] = function (request, response, identifier, connection) {
	var link = connection.getHCILink();
	if (!link.isLELink() || link.isLESlave()) {
		/* Non LE or LE slave does not support this request */
		doCommandReject(response, identifier, 0x0000, null);
		return;
	}
	_hci.LE.connectionUpdate(
		link.handle,
		{
			intervalMin: request.getInt16(),
			intervalMax: request.getInt16(),
			latency: request.getInt16(),
			supervisionTimeout: request.getInt16(),
			minimumCELength: 0,
			maximumCELength: 0
		}
	);
	response.putInt8(Code.CONNECTION_PARAMETER_UPDATE_RESPONES);
	response.putInt8(identifier);
	response.putInt16(2);
	response.putInt16(0x0000);
};
