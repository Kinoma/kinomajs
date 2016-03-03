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

var Utils = require("/lowpan/common/utils");
var Ringbuffer = Utils.Ringbuffer;
var Buffers = require("/lowpan/common/buffers");
var SerialBuffer = Buffers.SerialBuffer;
var ByteBuffer = Buffers.ByteBuffer;

var logger = new Utils.Logger("UART");
logger.loggingLevel = Utils.Logger.Level.INFO;

var DEFAULT_UART_DEVICE = "/dev/mbtchar0";
var DEFAULT_UART_BAUDRATE = 115200;

var UART_COMMAND_PACKET = 0x01;
var UART_ACL_DATA_PACKET = 0x02;
var UART_SYNC_DATA_PACKET = 0x03;
var UART_EVENT_PACKET = 0x04;

var HCI_MAX_ACL_SIZE = 2048;
var HCI_MAX_SCO_SIZE = 255;
var HCI_MAX_EVENT_SIZE = 260;
var HCI_MAX_FRAME_SIZE = (HCI_MAX_ACL_SIZE + 4);

var MAX_RX_BUFFER = 16384;

var STATE_PACKET_TYPE = 0;
var STATE_PREAMBLE = 1;
var STATE_DATA = 2;

var state = STATE_PACKET_TYPE;
var expectedLength = 1;
var packetType = 0;
var readRingbuffer;

var tempObject = null;

var preambleSize = [];
var packetReader = [];

var _serial = null;
var writeBuffer;

/** Debugging purpose */
var DEBUG_PARSER = true;
var MAX_COUNT = 1000;
var elapsedTime;
var totalLength = 0;
var totalResponses = 0;
var totalTime = 0;
var counter = 0;

exports.open = function () {
	_serial = PINS.create({
		type: "Serial",
		path: DEFAULT_UART_DEVICE,
		baud: DEFAULT_UART_BAUDRATE
	});
	_serial.init();
	writeBuffer = new SerialBuffer(_serial, HCI_MAX_ACL_SIZE, true);
	readRingbuffer = new Ringbuffer(MAX_RX_BUFFER);
	return _serial;
};

exports.close = function () {
	_serial.close();
};

exports.sendCommand = function (command) {
	logger.trace("<==sendCommand");
	writeBuffer.clear();
	writeBuffer.putInt8(UART_COMMAND_PACKET);
	writeBuffer.putInt16(command.opcode);
	writeBuffer.putInt8(command.length);
	if (command.data != null) {
		writeBuffer.putByteArray(command.data);
	}
	writeBuffer.flush();
	logger.trace("==>sendCommand");
};

exports.sendACLData = function (acl) {
	logger.trace("<==sendACLData");
	logger.debug("ACL (handle="
		+ Utils.toHexString(acl.handle, 2)
		+ "): << " + Utils.toFrameString(acl.data, 0, acl.length));
	writeBuffer.clear();
	writeBuffer.putInt8(UART_ACL_DATA_PACKET);
	writeBuffer.putInt16(
		(acl.handle & 0xFFF) |
		((acl.packetBoundary & 0x3) << 12) |
		((acl.broadcast & 0x3) << 14)
	);
	writeBuffer.putInt16(acl.length);
	if (acl.data != null) {
		writeBuffer.putByteArray(acl.data);
	}
	writeBuffer.flush();
	logger.trace("==>sendACLData");
};

preambleSize[UART_ACL_DATA_PACKET - 1] = 4;
packetReader[UART_ACL_DATA_PACKET - 1] = function (buffer) {
	var tmp = buffer.readByte() | (buffer.readByte() << 8);
	return {
		handle: tmp & 0xFFF,
		packetBoundary: (tmp >> 12) & 0x3,
		broadcast: (tmp >> 14) & 0x3,
		length: (buffer.readByte() | (buffer.readByte() << 8)),
		data: null
	};
};

exports.sendSynchronousData = function (handle, packetStatus, length, data) {
	writeBuffer.clear();
	writeBuffer.putInt8(UART_SYNC_DATA_PACKET);
	writeBuffer.putInt16(
		(handle & 0xFFF) |
		((packetStatus & 0x3) << 12)
	);
	writeBuffer.putInt8(length);
	if (data != null) {
		writeBuffer.putByteArray(data);
	}
	writeBuffer.flush();
};

preambleSize[UART_SYNC_DATA_PACKET - 1] = 3;
packetReader[UART_SYNC_DATA_PACKET - 1] = function (buffer) {
	var tmp = buffer.getInt16();
	return {
		handle: tmp & 0xFFF,
		packetStatus: (tmp >> 12) & 0x3,
		length: buffer.readByte(),
		data: null
	};
};

preambleSize[UART_EVENT_PACKET - 1] = 2;
packetReader[UART_EVENT_PACKET - 1] = function (buffer) {
	return {
		eventCode: buffer.readByte(),
		length: buffer.readByte(),
		data: null
	};
};

exports.receive = function () {
	var responses = [];

	var rxStartTime = new Date();

	var buffer = _serial.read("ArrayBuffer");
	if (buffer.byteLength == 0) {
		logger.debug("serial.read returns 0");
		return responses;
	}
	readRingbuffer.write(new Uint8Array(buffer), 0, buffer.byteLength);

	if (DEBUG_PARSER) {
		elapsedTime = new Date().getTime() - rxStartTime.getTime();
		logger.trace("RX Time: " + elapsedTime);
		if (elapsedTime > 100) {
			logger.loggingLevel = Utils.Logger.Level.TRACE;
		}
		totalLength += buffer.byteLength;
	}

	while (readRingbuffer.available() >= expectedLength) {
		switch (state) {
		case STATE_PACKET_TYPE:
			logger.trace("Read Packet Type");
			packetType = readRingbuffer.readByte();
			if ((packetType < UART_ACL_DATA_PACKET) || (UART_EVENT_PACKET < packetType)) {
				logger.error("Unexpected packet type: " + Utils.toHexString(packetType));
				break;
			}
			expectedLength = preambleSize[packetType - 1];
			state = STATE_PREAMBLE;
			break;
		case STATE_PREAMBLE:
			logger.trace("Read Preamble");
			tempObject = packetReader[packetType - 1](readRingbuffer);
			tempObject.packetType = packetType;
			expectedLength = tempObject.length;
			state = STATE_DATA;
			break;
		case STATE_DATA:
			logger.trace("Read Data: length=" + tempObject.length);
			if (tempObject.length > 0) {
				tempObject.data = new Uint8Array(tempObject.length);
				readRingbuffer.read(tempObject.data, 0, tempObject.length);
			}
			responses.push(tempObject);
			tempObject = null;
			expectedLength = 1;
			state = STATE_PACKET_TYPE;
			break;
		}
	}

	if (DEBUG_PARSER) {
		totalTime += elapsedTime;
		totalResponses += responses.length;
		if (++counter > MAX_COUNT) {
			logger.info("Average length=" + (totalLength / MAX_COUNT)
				+ ", responses=" + (totalResponses / MAX_COUNT)
				+ ", time=" + (totalTime / MAX_COUNT));
			totalLength = 0;
			totalResponses = 0;
			totalTime = 0;
			counter = 0;
		}
	}

	return responses;
};
