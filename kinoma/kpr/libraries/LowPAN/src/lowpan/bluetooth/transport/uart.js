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
 * Bluetooth v4.2 - UART Transport Layer
 */

const Utils = require("../../common/utils");
const Ringbuffer = Utils.Ringbuffer;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

var logger = new Utils.Logger("UART");
logger.loggingLevel = Utils.Logger.Level.INFO;

const UART_COMMAND_PACKET = 0x01;
const UART_ACL_DATA_PACKET = 0x02;
const UART_SYNC_DATA_PACKET = 0x03;
const UART_EVENT_PACKET = 0x04;

const HCI_MAX_ACL_SIZE = 2048;
const HCI_MAX_SCO_SIZE = 255;
const HCI_MAX_EVENT_SIZE = 260;
const HCI_MAX_FRAME_SIZE = (HCI_MAX_ACL_SIZE + 4);

const MAX_RX_BUFFER = 16384;

const STATE_PACKET_TYPE = 0;
const STATE_PREAMBLE = 1;
const STATE_DATA = 2;

var PREAMBLE_SIZE = [];
var PACKET_READER = [];
PREAMBLE_SIZE[UART_ACL_DATA_PACKET - 1] = 4;
PACKET_READER[UART_ACL_DATA_PACKET - 1] = function (buffer) {
	let tmp = buffer.readByte() | (buffer.readByte() << 8);
	return {
		handle: tmp & 0xFFF,
		packetBoundary: (tmp >> 12) & 0x3,
		broadcast: (tmp >> 14) & 0x3,
		length: (buffer.readByte() | (buffer.readByte() << 8)),
		data: null
	};
};

PREAMBLE_SIZE[UART_SYNC_DATA_PACKET - 1] = 3;
PACKET_READER[UART_SYNC_DATA_PACKET - 1] = function (buffer) {
	let tmp = buffer.readByte() | (buffer.readByte() << 8);
	return {
		handle: tmp & 0xFFF,
		packetStatus: (tmp >> 12) & 0x3,
		length: buffer.readByte(),
		data: null
	};
};

PREAMBLE_SIZE[UART_EVENT_PACKET - 1] = 2;
PACKET_READER[UART_EVENT_PACKET - 1] = function (buffer) {
	return {
		eventCode: buffer.readByte(),
		length: buffer.readByte(),
		data: null
	};
};

class Transport {
	constructor(serial) {
		this._serial = serial;
		this._delegate = null;
		this._txBuffer = ByteBuffer.allocateUint8Array(HCI_MAX_ACL_SIZE, true);
		this._rxRingbuffer = new Ringbuffer(MAX_RX_BUFFER);
		this._reset();
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	_reset() {
		this._tempObject = null;
		this._expectedLength = 1;
		this._packetType = 0;
		this._state = STATE_PACKET_TYPE;
	}
	_flush() {
		this._txBuffer.flip();
		let array = this._txBuffer.getByteArray();
		this._serial.write(array.buffer);
	}
	sendCommand(command) {
		logger.debug("<==sendCommand");
		this._txBuffer.clear();
		this._txBuffer.putInt8(UART_COMMAND_PACKET);
		this._txBuffer.putInt16(command.opcode);
		this._txBuffer.putInt8(command.length);
		if (command.data != null) {
			this._txBuffer.putByteArray(command.data);
		}
		this._flush();
		logger.debug("==>sendCommand");
	}
	sendACLData(acl) {
		logger.debug("<==sendACLData");
		logger.debug("ACL (handle="
			+ Utils.toHexString(acl.handle, 2)
			+ "): << " + Utils.toFrameString(acl.data, 0, acl.length));
		this._txBuffer.clear();
		this._txBuffer.putInt8(UART_ACL_DATA_PACKET);
		this._txBuffer.putInt16(
			(acl.handle & 0xFFF) |
			((acl.packetBoundary & 0x3) << 12) |
			((acl.broadcast & 0x3) << 14)
		);
		this._txBuffer.putInt16(acl.length);
		if (acl.data != null) {
			this._txBuffer.putByteArray(acl.data);
		}
		this._flush();
		logger.debug("==>sendACLData");
	}
	sendSynchronousData(handle, packetStatus, length, data) {
		this._txBuffer.clear();
		this._txBuffer.putInt8(UART_SYNC_DATA_PACKET);
		this._txBuffer.putInt16(
			(handle & 0xFFF) |
			((packetStatus & 0x3) << 12)
		);
		this._txBuffer.putInt8(length);
		if (data != null) {
			this._txBuffer.putByteArray(data);
		}
		this._txBuffer.flush();
	}
	receive(byteArray, offset, length) {
		let responses = [];

		this._rxRingbuffer.write(byteArray, offset, length);

		while (this._rxRingbuffer.available() >= this._expectedLength) {
			switch (this._state) {
			case STATE_PACKET_TYPE:
				logger.trace("Read Packet Type");
				this._packetType = this._rxRingbuffer.readByte();
				if ((this._packetType < UART_ACL_DATA_PACKET) || (UART_EVENT_PACKET < this._packetType)) {
					logger.error("Unexpected packet type: " + Utils.toHexString(this._packetType));
					break;
				}
				this._expectedLength = PREAMBLE_SIZE[this._packetType - 1];
				this._state = STATE_PREAMBLE;
				break;
			case STATE_PREAMBLE:
				logger.trace("Read Preamble");
				this._tempObject = PACKET_READER[this._packetType - 1](this._rxRingbuffer);
				this._tempObject.packetType = this._packetType;
				this._expectedLength = this._tempObject.length;
				this._state = STATE_DATA;
				break;
			case STATE_DATA:
				logger.trace("Read Data: length=" + this._tempObject.length);
				if (this._tempObject.length > 0) {
					this._tempObject.data = new Uint8Array(this._tempObject.length);
					this._rxRingbuffer.read(this._tempObject.data, 0, this._tempObject.length);
				}
				responses.push(this._tempObject);
				this._reset();
				break;
			}
		}

		if (this._delegate != null) {
			for (let i = 0; i < responses.length; i++) {
				this._delegate.transportReceived(responses[i]);
			}
		}
	};
}
exports.Transport = Transport;
