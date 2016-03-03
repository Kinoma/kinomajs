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
 * Bluetooth v4.2 - HCI (LE Only)
 */

var BTUtils = require("btutils");
var BluetoothAddress = BTUtils.BluetoothAddress;

var Utils = require("/lowpan/common/utils");
var Buffers = require("/lowpan/common/buffers");
var ByteBuffer = Buffers.ByteBuffer;

var UART_COMMAND_PACKET = 0x01;
var UART_ACL_DATA_PACKET = 0x02;
var UART_SYNC_DATA_PACKET = 0x03;
var UART_EVENT_PACKET = 0x04;

var EVENT_DISCONNECTION_COMPLETE = 0x05;
var EVENT_ENCRYPTION_CHANGE = 0x08;
var EVENT_COMMAND_COMPLETE = 0x0E;
var EVENT_COMMAND_STATUS = 0x0F;
var EVENT_NUMBER_OF_COMPLETED_PACKETS = 0x13;
var EVENT_LE_META = 0x3E;

var BUFFER_SIZE = 256;

var DEFAULT_LE_EVENT_MASK = Utils.multiIntToByteArray(
	[0x0000001F, 0x00000000], Utils.INT_32_SIZE, 2, true
);

var logger = new Utils.Logger("HCI");
logger.loggingLevel = Utils.Logger.Level.INFO;

/** Lower transport layer instance/module */
var _transport = null;

/** Upper data layer instance/module (i.e. L2CAP) */
var _delegate = null;

var _publicAddress;
var _initDone = false;
var _linksBeforeReady = new Array();

exports.registerTransport = function (transport) {
	_transport = transport;
	// XXX: We do not register delegate here
};

exports.registerDelegate = function (delegate) {
	_delegate = delegate;
};

exports.activate = function (reset) {
	if (reset === undefined) {
		reset = true;
	}
	if (reset) {
		logger.info("Reset HCI");
		Controller.reset(InitContext);
	} else {
		LE.setEventMask(DEFAULT_LE_EVENT_MASK, InitContext);
	}
};

exports.getPublicAddress = function () {
	return _publicAddress;
};

function hciReady() {
	_initDone = true;
	while (_linksBeforeReady.length > 0) {
		initL2CAP(_linksBeforeReady.shift());
	}
	_delegate.hciReady();
}

var InitContext = {
	commandComplete: function (opcode, response) {
		var status = response.getInt8();
		if (status != 0) {
			logger.warn("Init Error opcode=" + Utils.toHexString(opcode, 2)
				+ ", status=" + Utils.toHexString(status));
			return;
		}
		switch (opcode) {
		case ControllerCommand.RESET:
			LE.setEventMask(DEFAULT_LE_EVENT_MASK, InitContext);
			break;
		case LECommand.SET_EVENT_MASK:
			Informational.readBDAddr(InitContext);
			break;
		case InformationalCommand.READ_BD_ADDR:
			_publicAddress = BluetoothAddress.getByAddress(response.getByteArray(6), true, false);
			logger.info("publicAddress=" + _publicAddress.toString());
			LE.readBufferSize(InitContext);
			break;
		case LECommand.READ_BUFFER_SIZE:
			linkContext.le.maxDataLength = response.getInt16();
			linkContext.le.maxPackets = response.getInt8();
			if (linkContext.le.maxDataLength == 0 || linkContext.le.maxPackets == 0) {
				Informational.readBufferSize(InitContext);
			} else {
				logger.debug("maxDataLength(LE)=" + linkContext.le.maxDataLength);
				logger.debug("maxPackets(LE)=" + linkContext.le.maxPackets);
				hciReady();
			}
			break;
		case InformationalCommand.READ_BUFFER_SIZE:
			linkContext.maxDataLength = response.getInt16();
			response.skip(1);
			linkContext.maxPackets = response.getInt16();
			logger.debug("maxDataLength(ACL)=" + linkContext.maxDataLength);
			logger.debug("maxPackets(ACL)=" + linkContext.maxPackets);
			logger.info("Use ACL Buffer Size");
			linkContext.maxLEDataLength = linkContext.maxDataLength;
			linkContext.maxLEPackets = linkContext.maxPackets;
			hciReady();
			break;
		}
	}
};

var eventCallbacks = {};

function toOpcode(ogf, ocf) {
	return (ocf & 0x3FF) | ((ogf & 0x3F) << 10);
}

/**
 * Callback from lower transport layer
 */
exports.transportReceived = function (response) {
	/* FIXME: We assume the response from UART transport */
	switch (response.packetType) {
	case UART_ACL_DATA_PACKET:
		let link = getACLLink(response.handle);
		if (link != null) {
			link.received(
				response.packetBoundary,
				response.broadcast,
				response.data
			);
		} else {
			logger.warn("Unknown ACL Packet handle=" + Utils.toHexString(response.handle, 2));
		}
		break;
	case UART_EVENT_PACKET:
		let buffer = null;
		if (response.length > 0) {
			buffer = ByteBuffer.wrap(response.data);
			buffer.littleEndian = true;
		}
		if (response.eventCode in eventCallbacks) {
			logger.trace("Event: eventCode=" + Utils.toHexString(response.eventCode)
				+ " " + Utils.toFrameString(response.data));
			eventCallbacks[response.eventCode].eventReceived(buffer);
		} else {
			logger.warn("Event Unhandled: eventCode=" + Utils.toHexString(response.eventCode));
		}
		break;
	}
};

/******************************************************************************
 * HCI ACL Links
 ******************************************************************************/
var linkContext = {
	linkMap: {},
	maxDataLength: 0,
	maxPackets: 0,
	currentPackets: 0,
	le: {
		maxDataLength: 0,
		maxPackets: 0,
		currentPackets: 0
	}
};

/**
 * A class represents ACL-U and LE-U
 */
class ACLLink {
	constructor(handle, address, le) {
		this._handle = handle;
		this._address = address;
		this._le = le;
		this._delegate = null;
		this._encryptionStatus = 0x00;
		this._security = null;
	}
	get handle() {
		return this._handle & 0x0FFF;
	}
	get address() {
		return this._address;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	set security(security) {
		this._security = security;
	}
	get encrptionStatus() {
		return this._encryptionStatus;
	}
	isLELink() {
		return this._le;
	}
	send(buffer) {
		let context = this._le ? linkContext.le : linkContext;
		if (context.maxDataLength == 0) {
			throw "Link buffer size is zero!";
		}
		let length;
		let packetBoundary = 0x0;
		while ((length = buffer.remaining()) > 0) {
			if (length > context.maxDataLength) {
				length = context.maxDataLength;
			}
			if (context.currentPackets >= context.maxPackets) {
				logger.error("Link buffer is full");
				return;
			}
			let fragment = buffer.getByteArray(length);
			logger.trace("Submit ACL Data Packet: handle=" + Utils.toHexString(this.handle, 2)
				+ ", pb=" + Utils.toHexString(packetBoundary)
				+ " " + Utils.toFrameString(fragment));
			submitData(this.handle, packetBoundary, 0x00, fragment);
			context.currentPackets++;
			logger.trace("Link Status(Sent): currentPackets=" + context.currentPackets
				+ ", remainingPackets=" + (context.maxPackets - context.currentPackets));
			if (packetBoundary == 0x0) {
				packetBoundary = 0x1;
			}
		}
	}
	received(packetBoundary, broadcast, data) {
		if (broadcast != 0x0) {
			logger.warn("TODO: Broadcast packet");
			return;
		}
		logger.trace("Receiving ACL Data Packet: pb=" + Utils.toHexString(packetBoundary)
			+ " " + Utils.toFrameString(data));
		if (packetBoundary == 0x2) {
			this._delegate.received(data, true);
		} else if (packetBoundary == 0x1) {
			this._delegate.received(data, false);
		} else {
			logger.warn("Invalid packet boundary: " + packetBoundary);
			return;
		}
	}
	disconnect(reason) {
		LinkControl.disconnect(this.handle, reason);
	}
	disconnected() {
		this._delegate.disconnected();
	}
	updateEncryptionStatus(enabled) {
		this._encryptionStatus = enabled;
		this._security.encryptionStatusChanged(this, enabled);
	}
	encryptionFailed(status) {
		this._security.encryptionFailed(status);
	}
}

class LELink extends ACLLink {
	constructor(handle, localAddress, remoteAddress, info) {
		super(handle, remoteAddress, true);
		this._localAddress = localAddress;
		this._remoteAddress = remoteAddress;
		this._info = info;
	}
	isLESlave() {
		return (this._info.role == 0x01);
	}
	get localAddress() {
		return this._localAddress;
	}
	set localAddress(address) {
		this._localAddress = address;
	}
	get remoteAddress() {
		return this._remoteAddress;
	}
	set remoteAddress(address) {
		if (address.isIdentity()) {
			/* Allow identity address only */
			this._remoteAddress = address;
		}
	}
	startEncryption(random, ediv, ltk) {
		LE.startEncryption(this.handle, random, ediv, ltk);
	}
	replyLongTermKey(ltk) {
		if (ltk != null) {
			LE.longTermKeyRequestReply(this.handle, ltk);
		} else {
			LE.longTermKeyRequestNegativeReply(this.handle);
		}
	}
	longTermKeyRequested(random, ediv) {
		if (this._security == null) {
			LE.longTermKeyRequestNegativeReply(this.handle);
		} else {
			this._security.longTermKeyRequested(this, random, ediv);
		}
	}
}

function addACLLink(link) {
	linkContext.linkMap[link.handle] = link;
	if (!_initDone) {
		logger.warn("Connected before HCI is ready");
		_linksBeforeReady.push(link);
		return;
	}
	_delegate.hciConnected(link);
}

function getACLLink(handle) {
	var key = handle & 0x0FFF;
	if (key in linkContext.linkMap) {
		return linkContext.linkMap[key];
	}
	return null;
}

/**
 * Submit ACL Data Packet
 */
function submitData(handle, packetBoundary, broadcast, data) {
	var acl = {
		handle: handle,
		packetBoundary: packetBoundary,
		broadcast: broadcast
	};
	if (data === undefined || data == null) {
		acl.length = 0;
		acl.data = null;
	} else {
		acl.length = data.length;
		acl.data = data;
	}
	_transport.sendACLData(acl);
}

eventCallbacks[EVENT_DISCONNECTION_COMPLETE] = {
	eventReceived: function (buffer) {
		var status = buffer.getInt8();
		var handle = buffer.getInt16() & 0x0FFF;
		var reason = buffer.getInt8();
		if (status == 0x00) {
			var link = linkContext.linkMap[handle];
			if (link == null) {
				logger.warn("Unknown disconnection: handle=" + Utils.toHexString(handle, 2));
				return;
			}
			logger.info("Disonnected: handle=" + Utils.toHexString(handle, 2));
			link.disconnected();
			linkContext.linkMap[handle] = null;
			// TODO
		} else {
			logger.warn("Disconnection failed: status=" + Utils.toHexString(status));
			// TODO
		}
	}
};

eventCallbacks[EVENT_NUMBER_OF_COMPLETED_PACKETS] = {
	eventReceived: function (buffer) {
		var length = buffer.getInt8();
		for (var i = 0; i < length; i++) {
			var handle = buffer.getInt16() & 0x0FFF;
			var numPackets = buffer.getInt16();
			var link = getACLLink(handle);
			if (link != null) {
				var context = link.isLELink() ? linkContext.le : linkContext;
				context.currentPackets -= numPackets;
				logger.trace("Link Status(NOCP): currentPackets=" + context.currentPackets
					+ ", remainingPackets=" + (context.maxPackets - context.currentPackets));
			} else {
				logger.warn("NOCP: Unknown handle=" + Utils.toHexString(handle, 2));
			}
		}
	}
};

eventCallbacks[EVENT_ENCRYPTION_CHANGE] = {
	eventReceived: (buffer) => {
		let status = buffer.getInt8();
		let handle = buffer.getInt16() & 0x0FFF;
		let enabled = buffer.getInt8();
		let link = getACLLink(handle);
		if (link != null) {
			if (status == 0) {
				link.updateEncryptionStatus(enabled);
			} else {
				link.encryptionFailed(status);
			}
		} else {
			logger.warn("EC: Unknown handle=" + Utils.toHexString(handle, 2));
		}
	}
};

/******************************************************************************
 * HCI Command
 ******************************************************************************/
var commandCallbacks = {};
var numCommandsAllowed = 1;
var pendingCommands = [];

function enqueueCommand(command, callback = null) {
	logger.trace("Enqueue Command: opcode=" + Utils.toHexString(command.opcode, 2));
	pendingCommands.push({
		command: command,
		callback: callback
	});
	processPendingCommands();
}

function getCallbackQueue(opcode) {
	if (commandCallbacks[opcode] == null) {
		commandCallbacks[opcode] = [];
	}
	return commandCallbacks[opcode];
}

function processPendingCommands() {
	var numProcessed = 0;
	while (numCommandsAllowed > 0 && pendingCommands.length > 0) {
		var obj = pendingCommands.shift();
		var queue = getCallbackQueue(obj.command.opcode);
		queue.push(obj.callback);
		logger.trace("Command: opcode=" + Utils.toHexString(obj.command.opcode, 2)
			+ " " + Utils.toFrameString(obj.command.data, 0, obj.command.length));
		_transport.sendCommand(obj.command);
		numCommandsAllowed--;
		numProcessed++;
	}
	logger.trace("Command Status: current numProcessed=" + numProcessed
		+ ", nca=" + numCommandsAllowed
		+ ", pending=" + pendingCommands.length);
}

function submitCommand(opcode, buffer, callback) {
	var command = {
		opcode: opcode
	};
	if (buffer === undefined || buffer == null) {
		command.length = 0;
		command.data = null;
	} else {
		command.length = buffer.remaining();
		command.data = buffer.getByteArray(command.length);
	}
	enqueueCommand(command, callback);
}
exports.submitCommand = submitCommand;

function submitCommandWithArray(opcode, array, callback) {
	enqueueCommand({
		opcode: opcode,
		length: array.length,
		data: array
	}, callback);
}
exports.submitCommandWithArray = submitCommandWithArray;

eventCallbacks[EVENT_COMMAND_STATUS] = {
	eventReceived: function (buffer) {
		commandEventReceived(false, buffer);
	}
};

eventCallbacks[EVENT_COMMAND_COMPLETE] = {
	eventReceived: function (buffer) {
		commandEventReceived(true, buffer);
	}
};

function commandEventReceived(complete, buffer) {
	var status;
	if (!complete) { 
		status = buffer.getInt8();
	}
	numCommandsAllowed = buffer.getInt8();
	if (numCommandsAllowed == 0) {
		logger.info("Controller set NCA to zero");
	} else {
		processPendingCommands();
	}
	var opcode = buffer.getInt16();
	var queue = getCallbackQueue(opcode);
	logger.trace("Command Event: opcode=" + Utils.toHexString(opcode, 2) + ", complete=" + complete
		+ ", queue.length=" + queue.length);
	if (queue.length > 0) {
		var callback = queue.shift();
		if (callback != undefined) {
			if (!complete) {
				if (status != 0) {
					logger.warn("Command Status with error: opcode="
						+ Utils.toHexString(opcode, 2)
						+ ", status=" + Utils.toHexString(status));
				}
				if ("commandStatus" in callback) {
					callback.commandStatus(opcode, status);
				}
			} else {
				status = buffer.peek();	// XXX: May not be status
				if (status != 0) {
					logger.warn("Command Complete with error: opcode="
						+ Utils.toHexString(opcode, 2)
						+ ", status=" + Utils.toHexString(status));
				}
				if ("commandComplete" in callback) {
					callback.commandComplete(opcode, buffer);
				}
			}
		}
	}
}

/******************************************************************************
 * Link Control Commands
 ******************************************************************************/
var OGF_LINK_CONTROL = 0x01;

var LinkControlCommand = {
	DISCONNECT: toOpcode(OGF_LINK_CONTROL, 0x0006)
};

var LinkControl = {
	/* HCI will generate command status */
	disconnect: function (handle, reason, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putInt8(reason);
		buffer.flip();
		logger.debug("LinkControl::Disconnect");
		submitCommand(LinkControlCommand.DISCONNECT, buffer, callback);
	}
};
exports.LinkControl = LinkControl;

/******************************************************************************
 * Controller & Baseband Commands
 ******************************************************************************/
var OGF_CONTROLLER = 0x03;

var ControllerCommand = {
	RESET: toOpcode(OGF_CONTROLLER, 0x0003)
};

var Controller = {
	reset: function (callback) {
		logger.debug("Controller::Reset");
		submitCommand(ControllerCommand.RESET, null, callback);
	}
};
exports.Controller = Controller;

/******************************************************************************
 * Informational Commands
 ******************************************************************************/
var OGF_INFORMATIONAL = 0x04;

var InformationalCommand = {
	READ_BUFFER_SIZE: toOpcode(OGF_INFORMATIONAL, 0x0005),
	READ_BD_ADDR: toOpcode(OGF_INFORMATIONAL, 0x0009)
};

var Informational = {
	readBufferSize: function (callback) {
		logger.debug("Informational::ReadBufferSize");
		submitCommand(InformationalCommand.READ_BUFFER_SIZE, null, callback);
	},
	readBDAddr: function (callback) {
		logger.debug("Informational::ReadBD_ADDR");
		submitCommand(InformationalCommand.READ_BD_ADDR, null, callback);
	}
};

/******************************************************************************
 * Status Commands
 ******************************************************************************/
var OGF_STATUS = 0x05;

var StatusCommand = {
	READ_RSSI: toOpcode(OGF_STATUS, 0x0005)
};

var Status = {
	readRSSI: function (handle, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("Status::ReadRSSI");
		submitCommand(StatusCommand.READ_RSSI, buffer, callback);
	}
};
exports.Status = Status;

/******************************************************************************
 * LE Commands
 ******************************************************************************/
var OGF_LE = 0x08;

var LECommand = {
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

var LE = {
	AdvertisingType: {
		ADV_IND: 0x00,
		ADV_DIRECT_IND_HDC: 0x01,
		ADV_SCAN_IND: 0x02,
		ADV_NONCONN_IND: 0x03,
		ADV_DIRECT_IND_LDC: 0x04
	},
	AdvertisingFilterPolicy: {
		ALL: 0x00,
		CONNECTION_WHITE_LIST: 0x01,
		SCAN_WHITE_LIST: 0x02,
		ALL_WHITE_LIST: 0x03
	},
	LEScanType: {
		PASSIVE: 0x00,
		ACTIVE: 0x01
	},
	OwnAddressType: {
		PUBLIC: 0x00,
		RANDOM: 0x01,
		PRIVATE_PUBLIC: 0x02,
		PRIVATE_RANDOM: 0x03
	},
	ScanningFilterPolicy: {
		ALL: 0x00,
		WHITE_LIST_ONLY: 0x01,
		ALL_UNDIRECTED: 0x02,
		ALL_WHITE_LIST: 0x03
	},
	callback: {
		dicovered: null
	},
	setEventMask: function (eventMask, callback) {
		logger.debug("LE::SetEventMask");
		submitCommandWithArray(LECommand.SET_EVENT_MASK, eventMask, callback);
	},
	readBufferSize: function (callback) {
		logger.debug("LE::ReadBufferSize");
		submitCommand(LECommand.READ_BUFFER_SIZE, null, callback);
	},
	setRandomAddress: function (address, callback) {
		logger.debug("LE::SetRandomAddress");
		submitCommandWithArray(LECommand.SET_RANDOM_ADDRESS, address.getRawArray(), callback);
	},
	setAdvertisingParameters: function (parameters, type, ownAddressType, peerAddressType, address, channelMap, filterPolicy, callback) {
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
		submitCommand(LECommand.SET_ADVERTISING_PARAMETERS, buffer, callback);
	},
	setAdvertisingData: function (data, callback) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetAdvertisingData");
		submitCommandWithArray(LECommand.SET_ADVERTISING_DATA, buffer.array, callback);
	},
	setScanResponseData: function (data, callback) {
		var buffer = ByteBuffer.allocateUint8Array(32, true);	// Fixed length
		buffer.putInt8(data.length);
		buffer.putByteArray(data);
		logger.debug("LE::SetScanResponseData");
		submitCommandWithArray(LECommand.SET_SCAN_RESPONSE_DATA, buffer.array, callback);
	},
	setAdvertiseEnable: function (enable, callback) {
		logger.debug("LE::SetAdvertiseEnable");
		let array = new Uint8Array(1);
		array[0] = enable ? 0x01 : 0x00;
		submitCommandWithArray(LECommand.SET_ADVERTISE_ENABLE, array, callback);
	},
	setScanParameters: function (parameters, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(parameters.scanType);
		buffer.putInt16(parameters.interval);
		buffer.putInt16(parameters.window);
		buffer.putInt8(parameters.addressType);
		buffer.putInt8(parameters.filterPolicy);
		buffer.flip();
		logger.debug("LE::SetScanParameters");
		submitCommand(LECommand.SET_SCAN_PARAMETERS, buffer, callback);
	},
	setScanEnable: function (enable, duplicatesFilter, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(enable ? 0x01 : 0x00);
		buffer.putInt8(duplicatesFilter ? 0x01 : 0x00);
		buffer.flip();
		logger.debug("LE::Scan=" + enable);
		submitCommand(LECommand.SET_SCAN_ENABLE, buffer);
	},
	/* HCI will generate command status */
	createConnection: function (scanParameters, useWhiteList, peerAddressType, address, ownAddressType, connParameters, callback) {
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
		submitCommand(LECommand.CREATE_CONNECTION, buffer, callback);
	},
	createConnectonCancel: function (callback) {
		logger.debug("LE::CreateConnectionCancel");
		submitCommand(LECommand.CREATE_CONNECTION_CANCEL, null, callback);
	},
	/* HCI will generate command status */
	connectionUpdate: function (handle, connParameters, callback) {
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
		submitCommand(LECommand.CONNECTION_UPDATE, buffer, callback);
	},
	encrypt: function (key, plainData, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, false);
		buffer.putByteArray(key);
		buffer.putByteArray(plainData);
		buffer.flip();
		logger.debug("LE::Encrypt");
		submitCommand(LECommand.ENCRYPT, buffer, callback);
	},
	rand: function (callback) {
		logger.debug("LE::Rand");
		submitCommand(LECommand.RAND, null, callback);
	},
	startEncryption: function (handle, random, ediv, ltk, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(random);
		buffer.putInt16(ediv);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::StartEncryption");
		submitCommand(LECommand.START_ENCRYPTION, buffer, callback);
	},
	longTermKeyRequestReply: function (handle, ltk, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.putByteArray(ltk);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestReply");
		submitCommand(LECommand.LTK_REQUEST_REPLY, buffer, callback);
	},
	longTermKeyRequestNegativeReply: function (handle, callback) {
		var buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt16(handle & 0x0FFF);
		buffer.flip();
		logger.debug("LE::LongTermKeyRequestNegativeReply");
		submitCommand(LECommand.LTK_REQUEST_NEGATIVE_REPLY, buffer, callback);
	},
	addDeviceToResolvingList: function (address, peerIRK, localIRK, callback) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.putByteArray(peerIRK);
		buffer.putByteArray(localIRK);
		buffer.flip();
		logger.debug("LE::AddDeviceToResolvingList");
		submitCommand(LECommand.ADD_DEVICE_TO_RESOLVING_LIST, buffer, callback);
	},
	removeDeviceFromResolvingList: function (address, callback) {
		let buffer = ByteBuffer.allocateUint8Array(BUFFER_SIZE, true);
		buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
		buffer.putByteArray(address.getRawArray());
		buffer.flip();
		logger.debug("LE::RemoveDeviceFromResolvingList");
		submitCommand(LECommand.REMOVE_DEVICE_FROM_RESOLVING_LIST, buffer, callback);
	},
	clearResolvingList: function (callback) {
		logger.debug("LE::ClearResolvingList");
		submitCommand(LECommand.CLEAR_RESOLVING_LIST, null, callback);
	},
	setAddressResolutionEnable: function (enable, callback) {
		logger.debug("LE:SetAddressResolutionEnable");
		submitCommandWithArray(LECommand.SET_ADDRESS_RESOLUTION_ENABLE, [enable ? 0x01 : 0x00], callback);
	}
};
exports.LE = LE;

var LEEvent = {
	CONNECTION_COMPLETE: 0x01,
	ADVERTISING_REPORT: 0x02,
	CONNECTION_UPDATE_COMPLETE: 0x03,
	LTK_REQUEST: 0x05,
	EventType: {
		ADV_IND: 0x00,
		ADV_DIRECT_IND: 0x01,
		ADV_SCAN_IND: 0x02,
		ADV_NONCONN_IND: 0x03,
		SCAN_RSP: 0x04
	}
};
exports.LEEvent = LEEvent;

eventCallbacks[EVENT_LE_META] = {
	eventReceived: function (buffer) {
		var code = buffer.getInt8();
		switch (code) {
		case LEEvent.CONNECTION_COMPLETE:
			logger.trace("CONNECTION_COMPLETE");
			{
				let status = buffer.getInt8();
				if (status == 0x00) {
					let handle = buffer.getInt16() & 0x0FFF;
					let role = buffer.getInt8();
					let random = buffer.getInt8() == 0x01;
					let address = BluetoothAddress.getByAddress(buffer.getByteArray(6), true, random);
					let connParameters = {
						interval: buffer.getInt16(),
						latency: buffer.getInt16(),
						supervisionTimeout: buffer.getInt16()
					};
					let masterClockAccuracy = buffer.getInt8();
					let link = new LELink(handle, _publicAddress, address, {
						role,
						connParameters,
						masterClockAccuracy
					});
					addACLLink(link);
				} else {
					logger.error("Connection complete with error: status="
						+ Utils.toHexString(info.status));
				}
			}
			break;
		case LEEvent.ADVERTISING_REPORT:
			logger.trace("ADVERTISING_REPORT");
			if (LE.callback.discovered != null) {
				LE.callback.discovered(readLEAdvertisingReport(buffer));
			}
			break;
		case LEEvent.LTK_REQUEST:
			logger.trace("LTK_REQUEST");
			{
				let handle = buffer.getInt16() & 0x0FFF;
				let random = buffer.getByteArray(8);
				let ediv = buffer.getInt16();
				let link = getACLLink(handle);
				if (link != null && link.isLELink()) {
					link.longTermKeyRequested(random, ediv);
				} else {
					logger.warn("LTKRequested: Unknown or NonLE handle=" + Utils.toHexString(handle, 2));
				}
			}
			break;
		default:
			logger.warn("Unhandled LE event: code=" + Utils.toHexString(code));
		}
	}
};

function readLEAdvertisingReport(buffer) {
	var reports = [];
	var numReports = buffer.getInt8();
	for (var i = 0; i < numReports; i++) {
		var report = {
			eventType: buffer.getInt8(),
			peer: {
				addressType: buffer.getInt8(),
				address: buffer.getByteArray(6)
			},
			length: buffer.getInt8()
		};
		if (report.length > 0) {
			report.data = buffer.getByteArray(report.length);
		}
		report.rssi = Utils.toSignedByte(buffer.getInt8());
		reports.push(report);
	}
	return reports;
}
