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
	submitCommand(opcode, command) {
		return new Promise((resolve, reject) => {
			let cb = {
				commandStatus: (opcode, status) => {
					(status == 0x00) ? resolve() : reject(status);
				},
				commandComplete: (opcode, buffer) => {
					let status = buffer.getInt8();
					(status == 0x00) ? resolve(buffer) : reject(status);
				}
			};
			if ((command == null) || (command instanceof ByteBuffer)) {
				this._commandExec.submitCommand(opcode, command, cb);
			} else {
				this._commandExec.submitCommandWithArray(opcode, command, cb);
			}
		});
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
	disconnect(handle, reason) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putInt8(reason);
		buffer.flip();
		logger.debug("LinkControl::Disconnect");
		return this.submitCommand(LinkControl.DISCONNECT, buffer);
	}
}

/******************************************************************************
 * Controller & Baseband Commands
 ******************************************************************************/
const OGF_CONTROLLER = 0x03;

const Controller = {
	RESET: toOpcode(OGF_CONTROLLER, 0x0003),
	SET_EVENT_MASK: toOpcode(OGF_CONTROLLER, 0x0001),
	RESET: toOpcode(OGF_CONTROLLER, 0x0003),
	READ_LE_HOST_SUPPORT: toOpcode(OGF_CONTROLLER, 0x006C),
	WRITE_LE_HOST_SUPPORT: toOpcode(OGF_CONTROLLER, 0x006D)
};
exports.Controller = Controller;

class ControllerCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	setEventMask(eventMask) {
		logger.debug("Controller::SetEventMask");
		return this.submitCommand(Controller.SET_EVENT_MASK, eventMask);
	}
	reset() {
		logger.debug("Controller::Reset");
		return this.submitCommand(Controller.RESET, null);
	}
	readLEHostSupport() {
		logger.debug("Controller::ReadLEHostSupport");
		return this.submitCommand(Controller.READ_LE_HOST_SUPPORT, null);
	}
	writeLEHostSupport(leSupported, simultaneous) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(leSupported);
		buffer.putInt8(simultaneous);
		buffer.flip();
		logger.debug("Controller::WriteLEHostSupport");
		return this.submitCommand(Controller.WRITE_LE_HOST_SUPPORT, buffer);
	}
}

/******************************************************************************
 * Informational Commands
 ******************************************************************************/
const OGF_INFORMATIONAL = 0x04;

const Informational = {
	READ_LOCAL_VERSION_INFORMATION: toOpcode(OGF_INFORMATIONAL, 0x0001),
	READ_BUFFER_SIZE: toOpcode(OGF_INFORMATIONAL, 0x0005),
	READ_BD_ADDR: toOpcode(OGF_INFORMATIONAL, 0x0009)
};
exports.Informational = Informational;

class InformationalCommandSet extends CommandSet {
	constructor(commandExec) {
		super(commandExec);
	}
	readLocalVersionInformation() {
		logger.debug("Informational::ReadLocalVersionInformation");
		return this.submitCommand(Informational.READ_LOCAL_VERSION_INFORMATION, null);
	}
	readBufferSize() {
		logger.debug("Informational::ReadBufferSize");
		return this.submitCommand(Informational.READ_BUFFER_SIZE, null);
	}
	readBDAddr() {
		logger.debug("Informational::ReadBD_ADDR");
		return this.submitCommand(Informational.READ_BD_ADDR, null);
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
	readRSSI(handle) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("Status::ReadRSSI");
		return this.submitCommand(Status.READ_RSSI, buffer);
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
	READ_WHITE_LIST_SIZE: toOpcode(OGF_LE, 0x000F),
	CLEAR_WHITE_LIST: toOpcode(OGF_LE, 0x0010),
	ADD_DEVICE_TO_WHITE_LIST: toOpcode(OGF_LE, 0x0011),
	REMOVE_DEVICE_FROM_WHITE_LIST: toOpcode(OGF_LE, 0x0012),
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
	setEventMask(eventMask) {
		logger.debug("LE::SetEventMask");
		return this.submitCommand(LE.SET_EVENT_MASK, eventMask);
	}
	readBufferSize() {
		logger.debug("LE::ReadBufferSize");
		return this.submitCommand(LE.READ_BUFFER_SIZE, null);
	}
	setRandomAddress(address) {
		logger.debug("LE::SetRandomAddress");
		return this.submitCommand(LE.SET_RANDOM_ADDRESS, address.getRawArray());
	}
	setAdvertisingParameters(parameters, type, ownAddressType, address, channelMap, filterPolicy) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(parameters.intervalMin);
		buffer.putInt16(parameters.intervalMax);
		buffer.putInt8(type);
		buffer.putInt8(ownAddressType);
		if (address != null) {
			buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
			buffer.putByteArray(address.getRawArray());
		} else {
			buffer.skip(7);
		}
		buffer.putInt8(channelMap);
		buffer.putInt8(filterPolicy);
		buffer.flip();
		logger.debug("LE::SetAdvertisingParameters");
		return this.submitCommand(LE.SET_ADVERTISING_PARAMETERS, buffer);
	}
	setAdvertisingData(data) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetAdvertisingData");
		return this.submitCommand(LE.SET_ADVERTISING_DATA, buffer.array);
	}
	setScanResponseData(data) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetScanResponseData");
		return this.submitCommand(LE.SET_SCAN_RESPONSE_DATA, buffer.array);
	}
	setAdvertiseEnable(enable) {
		logger.debug("LE::SetAdvertiseEnable");
		let array = new Uint8Array(1);
		array[0] = enable ? 0x01 : 0x00;
		return this.submitCommand(LE.SET_ADVERTISE_ENABLE, array);
	}
	setScanParameters(parameters) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(parameters.scanType);
		buffer.putInt16(parameters.interval);
		buffer.putInt16(parameters.window);
		buffer.putInt8(parameters.addressType);
		buffer.putInt8(parameters.filterPolicy);
		buffer.flip();
		logger.debug("LE::SetScanParameters");
		return this.submitCommand(LE.SET_SCAN_PARAMETERS, buffer);
	}
	setScanEnable(enable, duplicatesFilter) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(enable ? 0x01 : 0x00);
		buffer.putInt8(duplicatesFilter ? 0x01 : 0x00);
		buffer.flip();
		logger.debug("LE::Scan=" + enable);
		return this.submitCommand(LE.SET_SCAN_ENABLE, buffer);
	}
	/* HCI will generate command status */
	createConnection(scanParameters, useWhiteList, peerAddressType, address, ownAddressType, connParameters) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(scanParameters.interval);
		buffer.putInt16(scanParameters.window);
		buffer.putInt8(useWhiteList ? 0x01 : 0x00);
		buffer.putInt8(peerAddressType);
		if (address != null) {
			buffer.putByteArray(address.getRawArray());
		} else {
			buffer.skip(6);
		}
		buffer.putInt8(ownAddressType);
		buffer.putInt16(connParameters.intervalMin);
		buffer.putInt16(connParameters.intervalMax);
		buffer.putInt16(connParameters.latency);
		buffer.putInt16(connParameters.supervisionTimeout);
		buffer.putInt16(connParameters.minimumCELength);
		buffer.putInt16(connParameters.maximumCELength);
		buffer.flip();
		logger.debug("LE::CreateConnection");
		return this.submitCommand(LE.CREATE_CONNECTION, buffer);
	}
	createConnectonCancel(callback) {
		logger.debug("LE::CreateConnectionCancel");
		return this.submitCommand(LE.CREATE_CONNECTION_CANCEL, null);
	}
	readWhiteListSize(callback) {
		logger.debug("LE::ReadWhiteListSize");
		return this.submitCommand(LE.READ_WHITE_LIST_SIZE, null);
	}
	clearWhiteList(callback) {
		logger.debug("LE::ClearWhiteList");
		return this.submitCommand(LE.CLEAR_WHITE_LIST, null);
	}
	addDeviceToWhiteList(address) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.flip();
		logger.debug("LE::AddDeviceToWhiteList");
		return this.submitCommand(LE.ADD_DEVICE_TO_WHITE_LIST, buffer);
	}
	removeDeviceFromWhiteList(address) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.flip();
		logger.debug("LE::RemoveDeviceFromWhiteList");
		return this.submitCommand(LE.REMOVE_DEVICE_FROM_WHITE_LIST, buffer);
	}
	/* HCI will generate command status */
	connectionUpdate(handle, connParameters) {
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
		return this.submitCommand(LE.CONNECTION_UPDATE, buffer);
	}
	encrypt(key, plainData) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, false);
		buffer.putByteArray(key);
		buffer.putByteArray(plainData);
		buffer.flip();
		logger.debug("LE::Encrypt");
		return this.submitCommand(LE.ENCRYPT, buffer);
	}
	rand(callback) {
		logger.debug("LE::Rand");
		return this.submitCommand(LE.RAND, null);
	}
	startEncryption(handle, random, ediv, ltk) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(random);
		buffer.putInt16(ediv);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::StartEncryption");
		return this.submitCommand(LE.START_ENCRYPTION, buffer);
	}
	longTermKeyRequestReply(handle, ltk) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestReply");
		return this.submitCommand(LE.LTK_REQUEST_REPLY, buffer);
	}
	longTermKeyRequestNegativeReply(handle) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestNegativeReply");
		return this.submitCommand(LE.LTK_REQUEST_NEGATIVE_REPLY, buffer);
	}
	addDeviceToResolvingList(address, peerIRK, localIRK) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.putByteArray(peerIRK);
		buffer.putByteArray(localIRK);
		buffer.flip();
		logger.debug("LE::AddDeviceToResolvingList");
		return this.submitCommand(LE.ADD_DEVICE_TO_RESOLVING_LIST, buffer);
	}
	removeDeviceFromResolvingList(address) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.flip();
		logger.debug("LE::RemoveDeviceFromResolvingList");
		return this.submitCommand(LE.REMOVE_DEVICE_FROM_RESOLVING_LIST, buffer);
	}
	clearResolvingList(callback) {
		logger.debug("LE::ClearResolvingList");
		return this.submitCommand(LE.CLEAR_RESOLVING_LIST, null);
	}
	setAddressResolutionEnable(enable) {
		logger.debug("LE:SetAddressResolutionEnable");
		return this.submitCommand(LE.SET_ADDRESS_RESOLUTION_ENABLE, [enable ? 0x01 : 0x00]);
	}
}
