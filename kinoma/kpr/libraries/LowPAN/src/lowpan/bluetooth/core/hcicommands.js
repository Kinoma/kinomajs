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
 * Bluetooth v4.2 - HCI Command Sets
 */

const Utils = require("../../common/utils");
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const BUFFER_SIZE = 256;

var logger = new Utils.Logger("HCIComm");
logger.loggingLevel = Utils.Logger.Level.INFO;

function toOpcode(ogf, ocf) {
	return (ocf & 0x3FF) | ((ogf & 0x3F) << 10);
}

exports.createCommandSets = commandExec => {
	return {
		linkControl: new LinkControlCommandSet(commandExec),
		controller: new ControllerCommandSet(commandExec),
		informational: new InformationalCommandSet(commandExec),
		status: new StatusCommandSet(commandExec),
		le: new LECommandSet(commandExec)
	};
};

class CommandSet {
	constructor(commandExec) {
		this._commandExec = commandExec;
	}
}

/******************************************************************************
 * Link Control Commands
 ******************************************************************************/
const OGF_LINK_CONTROL = 0x01;

const LinkControl = {
	DISCONNECT: toOpcode(OGF_LINK_CONTROL, 0x0006)
};
exports.LinkControl = LinkControl;

class LinkControlCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	/* HCI will generate command status */
	disconnect(handle, reason, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putInt8(reason);
		buffer.flip();
		logger.debug("LinkControl::Disconnect");
		this._commandExec.submitCommand(LinkControl.DISCONNECT, buffer, callback);
	}
}

/******************************************************************************
 * Controller & Baseband Commands
 ******************************************************************************/
const OGF_CONTROLLER = 0x03;

const Controller = {
	RESET: toOpcode(OGF_CONTROLLER, 0x0003)
};
exports.Controller = Controller;

class ControllerCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	reset(callback) {
		logger.debug("Controller::Reset");
		this._commandExec.submitCommand(Controller.RESET, null, callback);
	}
}

/******************************************************************************
 * Informational Commands
 ******************************************************************************/
const OGF_INFORMATIONAL = 0x04;

const Informational = {
	READ_BUFFER_SIZE: toOpcode(OGF_INFORMATIONAL, 0x0005),
	READ_BD_ADDR: toOpcode(OGF_INFORMATIONAL, 0x0009)
};
exports.Informational = Informational;

class InformationalCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	readBufferSize(callback) {
		logger.debug("Informational::ReadBufferSize");
		this._commandExec.submitCommand(Informational.READ_BUFFER_SIZE, null, callback);
	}
	readBDAddr(callback) {
		logger.debug("Informational::ReadBD_ADDR");
		this._commandExec.submitCommand(Informational.READ_BD_ADDR, null, callback);
	}
}

/******************************************************************************
 * Status Commands
 ******************************************************************************/
const OGF_STATUS = 0x05;

const Status = {
	READ_RSSI: toOpcode(OGF_STATUS, 0x0005)
};
exports.Status = Status;

class StatusCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	readRSSI(handle, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("Status::ReadRSSI");
		this._commandExec.submitCommand(Status.READ_RSSI, buffer, callback);
	}
}

/******************************************************************************
 * LE Commands
 ******************************************************************************/
const OGF_LE = 0x08;

const LE = {
	SET_EVENT_MASK: toOpcode(OGF_LE, 0x0001),
	READ_BUFFER_SIZE: toOpcode(OGF_LE, 0x0002),
	SET_RANDOM_ADDRESS: toOpcode(OGF_LE, 0x0005),
	SET_ADVERTISING_PARAMETERS: toOpcode(OGF_LE, 0x0006),
	SET_ADVERTISING_DATA: toOpcode(OGF_LE, 0x0008),
	SET_SCAN_RESPONSE_DATA: toOpcode(OGF_LE, 0x0009),
	SET_ADVERTISE_ENABLE: toOpcode(OGF_LE, 0x000A),
	SET_SCAN_PARAMETERS: toOpcode(OGF_LE, 0x000B),
	SET_SCAN_ENABLE: toOpcode(OGF_LE, 0x000C),
	CREATE_CONNECTION: toOpcode(OGF_LE, 0x000D),
	CREATE_CONNECTION_CANCEL: toOpcode(OGF_LE, 0x000E),
	CONNECTION_UPDATE: toOpcode(OGF_LE, 0x0013),
	ENCRYPT: toOpcode(OGF_LE, 0x0017),
	RAND: toOpcode(OGF_LE, 0x0018),
	START_ENCRYPTION: toOpcode(OGF_LE, 0x0019),
	LTK_REQUEST_REPLY: toOpcode(OGF_LE, 0x001A),
	LTK_REQUEST_NEGATIVE_REPLY: toOpcode(OGF_LE, 0x001B),
	ADD_DEVICE_TO_RESOLVING_LIST: toOpcode(OGF_LE, 0x0027),
	REMOVE_DEVICE_FROM_RESOLVING_LIST: toOpcode(OGF_LE, 0x0028),
	CLEAR_RESOLVING_LIST: toOpcode(OGF_LE, 0x0029),
	SET_ADDRESS_RESOLUTION_ENABLE: toOpcode(OGF_LE, 0x002D)
};
exports.LE = LE;

class LECommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	setEventMask(eventMask, callback) {
		logger.debug("LE::SetEventMask");
		this._commandExec.submitCommandWithArray(LE.SET_EVENT_MASK, eventMask, callback);
	}
	readBufferSize(callback) {
		logger.debug("LE::ReadBufferSize");
		this._commandExec.submitCommand(LE.READ_BUFFER_SIZE, null, callback);
	}
	setRandomAddress(address, callback) {
		logger.debug("LE::SetRandomAddress");
		this._commandExec.submitCommandWithArray(LE.SET_RANDOM_ADDRESS, address.getRawArray(), callback);
	}
	setAdvertisingParameters(parameters, type, ownAddressType, peerAddressType, address, channelMap, filterPolicy, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(parameters.intervalMin);
		buffer.putInt16(parameters.intervalMax);
		buffer.putInt8(type);
		buffer.putInt8(ownAddressType);
		buffer.putInt8(peerAddressType);
		if (address != null) {
			buffer.putByteArray(address.getRawArray());
		} else {
			buffer.skip(6);
		}
		buffer.putInt8(channelMap);
		buffer.putInt8(filterPolicy);
		buffer.flip();
		logger.debug("LE::SetAdvertisingParameters");
		this._commandExec.submitCommand(LE.SET_ADVERTISING_PARAMETERS, buffer, callback);
	}
	setAdvertisingData(data, callback) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetAdvertisingData");
		this._commandExec.submitCommandWithArray(LE.SET_ADVERTISING_DATA, buffer.array, callback);
	}
	setScanResponseData(data, callback) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetScanResponseData");
		this._commandExec.submitCommandWithArray(LE.SET_SCAN_RESPONSE_DATA, buffer.array, callback);
	}
	setAdvertiseEnable(enable, callback) {
		logger.debug("LE::SetAdvertiseEnable");
		let array = new Uint8Array(1);
		array[0] = enable ? 0x01 : 0x00;
		this._commandExec.submitCommandWithArray(LE.SET_ADVERTISE_ENABLE, array, callback);
	}
	setScanParameters(parameters, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(parameters.scanType);
		buffer.putInt16(parameters.interval);
		buffer.putInt16(parameters.window);
		buffer.putInt8(parameters.addressType);
		buffer.putInt8(parameters.filterPolicy);
		buffer.flip();
		logger.debug("LE::SetScanParameters");
		this._commandExec.submitCommand(LE.SET_SCAN_PARAMETERS, buffer, callback);
	}
	setScanEnable(enable, duplicatesFilter, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(enable ? 0x01 : 0x00);
		buffer.putInt8(duplicatesFilter ? 0x01 : 0x00);
		buffer.flip();
		logger.debug("LE::Scan=" + enable);
		this._commandExec.submitCommand(LE.SET_SCAN_ENABLE, buffer);
	}
	/* HCI will generate command status */
	createConnection(scanParameters, useWhiteList, peerAddressType, address, ownAddressType, connParameters, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(scanParameters.interval);
		buffer.putInt16(scanParameters.window);
		buffer.putInt8(useWhiteList ? 0x01 : 0x00);
		buffer.putInt8(peerAddressType);
		buffer.putByteArray(address.getRawArray());
		buffer.putInt8(ownAddressType);
		buffer.putInt16(connParameters.intervalMin);
		buffer.putInt16(connParameters.intervalMax);
		buffer.putInt16(connParameters.latency);
		buffer.putInt16(connParameters.supervisionTimeout);
		buffer.putInt16(connParameters.minimumCELength);
		buffer.putInt16(connParameters.maximumCELength);
		buffer.flip();
		logger.debug("LE::CreateConnection");
		this._commandExec.submitCommand(LE.CREATE_CONNECTION, buffer, callback);
	}
	createConnectonCancel(callback) {
		logger.debug("LE::CreateConnectionCancel");
		this._commandExec.submitCommand(LE.CREATE_CONNECTION_CANCEL, null, callback);
	}
	/* HCI will generate command status */
	connectionUpdate(handle, connParameters, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putInt16(connParameters.intervalMin);
		buffer.putInt16(connParameters.intervalMax);
		buffer.putInt16(connParameters.latency);
		buffer.putInt16(connParameters.supervisionTimeout);
		buffer.putInt16(connParameters.minimumCELength);
		buffer.putInt16(connParameters.maximumCELength);
		buffer.flip();
		logger.debug("LE::ConnectionUpdate");
		this._commandExec.submitCommand(LE.CONNECTION_UPDATE, buffer, callback);
	}
	encrypt(key, plainData, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, false);
		buffer.putByteArray(key);
		buffer.putByteArray(plainData);
		buffer.flip();
		logger.debug("LE::Encrypt");
		this._commandExec.submitCommand(LE.ENCRYPT, buffer, callback);
	}
	rand(callback) {
		logger.debug("LE::Rand");
		this._commandExec.submitCommand(LE.RAND, null, callback);
	}
	startEncryption(handle, random, ediv, ltk, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(random);
		buffer.putInt16(ediv);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::StartEncryption");
		this._commandExec.submitCommand(LE.START_ENCRYPTION, buffer, callback);
	}
	longTermKeyRequestReply(handle, ltk, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestReply");
		this._commandExec.submitCommand(LE.LTK_REQUEST_REPLY, buffer, callback);
	}
	longTermKeyRequestNegativeReply(handle, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestNegativeReply");
		this._commandExec.submitCommand(LE.LTK_REQUEST_NEGATIVE_REPLY, buffer, callback);
	}
	addDeviceToResolvingList(address, peerIRK, localIRK, callback) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.putByteArray(peerIRK);
		buffer.putByteArray(localIRK);
		buffer.flip();
		logger.debug("LE::AddDeviceToResolvingList");
		this._commandExec.submitCommand(LE.ADD_DEVICE_TO_RESOLVING_LIST, buffer, callback);
	}
	removeDeviceFromResolvingList(address, callback) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.flip();
		logger.debug("LE::RemoveDeviceFromResolvingList");
		this._commandExec.submitCommand(LE.REMOVE_DEVICE_FROM_RESOLVING_LIST, buffer, callback);
	}
	clearResolvingList(callback) {
		logger.debug("LE::ClearResolvingList");
		this._commandExec.submitCommand(LE.CLEAR_RESOLVING_LIST, null, callback);
	}
	setAddressResolutionEnable(enable, callback) {
		logger.debug("LE:SetAddressResolutionEnable");
		this._commandExec.submitCommandWithArray(LE.SET_ADDRESS_RESOLUTION_ENABLE, [enable ? 0x01 : 0x00], callback);
	}
}
