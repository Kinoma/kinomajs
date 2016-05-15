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
 * Bluetooth v4.2 - L2CAP Signaling Channel
 *
 */

const Utils = require("../../common/utils");
const Logger = Utils.Logger;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

var logger = Logger.getLogger("L2CAP.Signaling");

const Code = {
	COMMAND_REJECT: 0x01,
	CONNECTION_REQUEST: 0x02,
	ECHO_REQUEST: 0x08,
	ECHO_RESPONSE: 0x09,
	CONNECTION_PARAMETER_UPDATE_REQUEST: 0x12,
	CONNECTION_PARAMETER_UPDATE_RESPONES: 0x13
};

var signalingRequestHandler = {};

class L2CAPSignalingContext {
	constructor(connection) {
		this._connection = connection;
		connection.delegate = this;
	}
	/** L2CAP Connection delegate method */
	received(buffer) {
		let code = buffer.getInt8();
		let identifier = buffer.getInt8();
		let length = buffer.getInt16();
		buffer.setLimit(buffer.getPosition() + length);

		let respBuffer = ByteBuffer.allocateUint8Array(23);
		handleSignalingPacket(buffer, respBuffer, code, identifier, this._connection);
		if (respBuffer.getPosition() > 0) {
			respBuffer.flip();
			this._connection.sendBasicFrame(respBuffer.getByteArray());
		}
	}
	/** L2CAP Connection delegate method */
	disconnected() {
		logger.info("Signaling context disconnected.");
	}
}
exports.L2CAPSignalingContext = L2CAPSignalingContext;

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

signalingRequestHandler[Code.ECHO_REQUEST] = (request, response, identifier, connection) => {
	response.putInt8(Code.ECHO_RESPONSE);
	response.putInt8(identifier);
	response.putInt16(0);	// No data at all
};

signalingRequestHandler[Code.CONNECTION_PARAMETER_UPDATE_REQUEST] = (request, response, identifier, connection) => {
	var link = connection.link;
	if (!link.isLELink() || link.isLESlave()) {
		/* Non LE or LE slave does not support this request */
		doCommandReject(response, identifier, 0x0000, null);
		return;
	}
	let hci = link._linkMgr.context;	// FIXME
	hci.commands.le.connectionUpdate(
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
