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

const BTUtils = require("./btutils");
const BluetoothAddress = BTUtils.BluetoothAddress;

const Utils = require("../../common/utils");
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const Commands = require("./hcicommands");

const UART_COMMAND_PACKET = 0x01;
const UART_ACL_DATA_PACKET = 0x02;
const UART_SYNC_DATA_PACKET = 0x03;
const UART_EVENT_PACKET = 0x04;

const EVENT_DISCONNECTION_COMPLETE = 0x05;
const EVENT_ENCRYPTION_CHANGE = 0x08;
const EVENT_COMMAND_COMPLETE = 0x0E;
const EVENT_COMMAND_STATUS = 0x0F;
const EVENT_NUMBER_OF_COMPLETED_PACKETS = 0x13;
const EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE = 0x30;
const EVENT_LE_META = 0x3E;

const HANDLE_DISCONNECTED_MASK = 0x8000;

const LEEvent = {
	CONNECTION_COMPLETE: 0x01,
	ADVERTISING_REPORT: 0x02,
	CONNECTION_UPDATE_COMPLETE: 0x03,
	LTK_REQUEST: 0x05,
	DIRECT_ADVERTISING_REPORT: 0x0B
};

const LEConst = {
	AdvertisingType: {
		ADV_IND: 0x00,
		ADV_DIRECT_IND_HDC: 0x01,
		ADV_SCAN_IND: 0x02,
		ADV_NONCONN_IND: 0x03,
		ADV_DIRECT_IND_LDC: 0x04
	},
	AdvertisingFilterPolicy: {
		ALL: 0x00,
		SCAN_WHITE_LIST: 0x01,
		CONNECTION_WHITE_LIST: 0x02
	},
	ScanType: {
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
		WHITE_LIST: 0x01,
		RESOLVABLE_DIRECTED: 0x02
	},
	EventType: {
		ADV_IND: 0x00,
		ADV_DIRECT_IND: 0x01,
		ADV_SCAN_IND: 0x02,
		ADV_NONCONN_IND: 0x03,
		SCAN_RSP: 0x04
	}
};
exports.LEConst = LEConst;

const DEFAULT_EVENT_MASK = Utils.multiIntToByteArray(
	[0xfffbffff, 0x3dbff807], Utils.INT_32_SIZE, 2, true
);

const DEFAULT_LE_EVENT_MASK = Utils.multiIntToByteArray(
	[0x0000041F, 0x00000000], Utils.INT_32_SIZE, 2, true
);

const logger = new Utils.Logger("HCI");
logger.loggingLevel = Utils.Logger.Level.INFO;

exports.createLayer = transport => {
	logger.info("Init");
	return new Context(transport);
};

class Context {
	constructor(transport) {
		this._transport = transport;
		this._transport.delegate = this;
		this._delegate = null;
		this._discoveryCallback = null;
		this._linkMgr = new LinkManager(this, transport);
		this._linkMgr.delegate = this;
		this._commandExec = new CommandExecutor(transport);
		this._publicAddress = null;
		this._initDone = false;
		this._linksBeforeReady = new Array();
		this._commands = Commands.createCommandSets(this._commandExec);
	}
	get publicAddress() {
		return this._publicAddress;
	}
	get commands() {
		return this._commands;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	set discoveryCallback(callback) {
		this._discoveryCallback = callback;
	}
	init(reset = true) {
		let p;
		if (reset) {
			logger.info("Reset HCI");
			p = this._commands.controller.reset();
		} else {
			p = Promise.resolve(null);
		}

		p.then(response => {
			return this._commands.controller.setEventMask(DEFAULT_EVENT_MASK, this);
		}).then(response => {
			return this._commands.le.setEventMask(DEFAULT_LE_EVENT_MASK);
		}).then(response => {
			return this._commands.informational.readLocalVersionInformation(this);
		}).then(response => {
			return this._commands.controller.writeLEHostSupport(0x01, 0x00, this);
		}).then(response => {
			return this._commands.informational.readBDAddr();
		}).then(response => {
			this._publicAddress = BluetoothAddress.getByAddress(response.getByteArray(6), false);
			logger.info("publicAddress=" + this._publicAddress.toString());
			return this._commands.le.readBufferSize();
		}).then(response => {
			let linkCtx = this._linkMgr.getLinkContext(true);
			linkCtx.maxDataLength = response.getInt16();
			linkCtx.maxPackets = response.getInt8();
			logger.debug("maxDataLength(LE)=" + linkCtx.maxDataLength);
			logger.debug("maxPackets(LE)=" + linkCtx.maxPackets);
			return this._commands.informational.readBufferSize();
		}).then(response => {
			let linkCtx = this._linkMgr.getLinkContext(false);
			linkCtx.maxDataLength = response.getInt16();
			response.skip(1);
			linkCtx.maxPackets = response.getInt16();
			logger.debug("maxDataLength(ACL)=" + linkCtx.maxDataLength);
			logger.debug("maxPackets(ACL)=" + linkCtx.maxPackets);
			let linkCtxLE = this._linkMgr.getLinkContext(true);
			if (linkCtxLE.maxDataLength == 0 || linkCtxLE.maxPackets == 0) {
				logger.info("Use ACL Buffer Size");
				linkCtxLE.maxDataLength = linkCtx.maxDataLength;
				linkCtxLE.maxPackets = linkCtx.maxPackets;
			}
			this.initComplete();
		});

	}
	initComplete() {
		this._initDone = true;
		this._delegate.hciReady();
		while (this._linksBeforeReady.length > 0) {
			let link = this._linksBeforeReady.shift();
			link.localAddress = this._publicAddress;	// FIXME
			this._delegate.hciConnected(link);
		}
	}
	/* Transport Callback */
	transportReceived(response) {
		/* FIXME: We assume the response from UART transport */
		switch (response.packetType) {
		case UART_ACL_DATA_PACKET:
			let link = this._linkMgr.getACLLink(response.handle);
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
			switch (response.eventCode) {
			case EVENT_COMMAND_STATUS:
				this._commandExec.commandEventReceived(false, buffer);
				break;
			case EVENT_COMMAND_COMPLETE:
				this._commandExec.commandEventReceived(true, buffer);
				break;
			case EVENT_DISCONNECTION_COMPLETE:
				this._linkMgr.eventDisconnectionComplete(buffer);
				break;
			case EVENT_NUMBER_OF_COMPLETED_PACKETS:
				this._linkMgr.eventNumberOfCompletedPackets(buffer);
				break;
			case EVENT_ENCRYPTION_CHANGE:
				this._linkMgr.eventEncryptionChange(buffer);
				break;
			case EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE:
				this._linkMgr.eventEncryptionKeyRefreshComplete(buffer);
				break;
			case EVENT_LE_META:
				this.leEventReceived(buffer);
				break;
			default:
				logger.warn("Event Unhandled: eventCode=" + Utils.toHexString(response.eventCode));
			}
			break;
		}
	}
	leEventReceived(buffer) {
		let code = buffer.getInt8();
		switch (code) {
		case LEEvent.CONNECTION_COMPLETE:
			logger.trace("CONNECTION_COMPLETE");
			this._linkMgr.eventLEConnectionComplete(buffer);
			break;
		case LEEvent.ADVERTISING_REPORT:
			logger.trace("ADVERTISING_REPORT");
			this._discoveryCallback(readLEAdvertisingReport(buffer, false));
			break;
		case LEEvent.DIRECT_ADVERTISING_REPORT:
			logger.trace("DIRECT ADVERTISING_REPORT");
			this._discoveryCallback(readLEAdvertisingReport(buffer, true));
			break;
		case LEEvent.CONNECTION_UPDATE_COMPLETE:
			logger.trace("CONNECTION_UPDATE_COMPLETE");
			this._linkMgr.eventLEConnectionUpdateComplete(buffer);
			break;
		case LEEvent.LTK_REQUEST:
			logger.trace("LTK_REQUEST");
			this._linkMgr.eventLELTKRequest(buffer);
			break;
		default:
			logger.warn("Unhandled LE event: code=" + Utils.toHexString(code));
		}
	}
	linkAdded(link) {
		if (!this._initDone) {
			logger.warn("Connected before HCI is ready");
			this._linksBeforeReady.push(link);
			return;
		}
		link.localAddress = this._publicAddress;	// FIXME
		this._delegate.hciConnected(link);
	}
	/* Crypto Implementation */
	get random64() {
		return this._random64.bind(this);
	}
	_random64() {
		return this._commands.le.rand().then(response => {
			return response.getByteArray(8);
		});
	}
	/* Crypto Implementation */
	get encrypt() {
		return this._encrypt.bind(this);
	}
	_encrypt(key, plainData) {
		return this._commands.le.encrypt(key, plainData).then(response => {
			return response.getByteArray(16);
		});
	}
}

function readLEAdvertisingReport(buffer, direct) {
	let reports = [];
	let numReports = buffer.getInt8();
	for (let i = 0; i < numReports; i++) {
		let report = {};
		report.eventType = buffer.getInt8();
		let addressType = buffer.getInt8();
		report.address = BluetoothAddress.getByAddress(buffer.getByteArray(6), ((addressType & 0x01) > 0));
		report.resolved = (addressType & 0x02) > 0;
		if (direct) {
			/* Direct_Address is always RPA */
			buffer.skip(1);
			report.directAddress = BluetoothAddress.getByAddress(buffer.getByteArray(6), true);
		} else {
			/* Data available */
			report.length = buffer.getInt8();
			report.data = null;
			if (report.length > 0) {
				report.data = buffer.getByteArray(report.length);
			}
		}
		report.rssi = Utils.toSignedByte(buffer.getInt8());
		reports.push(report);
	}
	return reports;
}

/******************************************************************************
 * HCI ACL Links
 ******************************************************************************/
class LinkManager {
	constructor(ctx, transport) {
		this._ctx = ctx;
		this._transport = transport;
		this._delegate = null;
		this._linkMap = {};
		this._linkContext = {
			maxDataLength: 0,
			maxPackets: 0,
			currentPackets: 0
		};
		this._linkContextLE = {
			maxDataLength: 0,
			maxPackets: 0,
			currentPackets: 0
		};
	}
	get context() {
		return this._ctx;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	/**
	 * Submit ACL Data Packet
	 */
	submitData(handle, packetBoundary, broadcast, data = null) {
		let acl = {
			handle: handle,
			packetBoundary: packetBoundary,
			broadcast: broadcast
		};
		if (data == null) {
			acl.length = 0;
			acl.data = null;
		} else {
			acl.length = data.length;
			acl.data = data;
		}
		this._transport.sendACLData(acl);
	}
	getLinkContext(le) {
		return le ? this._linkContextLE : this._linkContext;
	}
	addACLLink(link) {
		this._linkMap[link.handle] = link;
		this._delegate.linkAdded(link);
	}
	getACLLink(handle) {
		var key = handle & 0x0FFF;
		if (key in this._linkMap) {
			return this._linkMap[key];
		}
		return null;
	}
	eventDisconnectionComplete(buffer) {
		let status = buffer.getInt8();
		let handle = buffer.getInt16();
		let reason = buffer.getInt8();
		if (status == 0x00) {
			let link = this.getACLLink(handle);
			if (link == null) {
				logger.warn("Unknown disconnection: handle=" + Utils.toHexString(handle, 2));
				return;
			}
			logger.info("Disonnected: handle=" + Utils.toHexString(handle, 2)
				+ ", reason=" + Utils.toHexString(reason));
			this._linkMap[handle] = null;
			link.disconnected(reason);
			// TODO
		} else {
			logger.warn("Disconnection failed: status=" + Utils.toHexString(status));
			// TODO
		}
	}
	eventNumberOfCompletedPackets(buffer) {
		let length = buffer.getInt8();
		for (let i = 0; i < length; i++) {
			let handle = buffer.getInt16();
			let numPackets = buffer.getInt16();
			let link = this.getACLLink(handle);
			if (link != null) {
				let context = link.isLELink() ? this._linkContextLE : this._linkContext;
				context.currentPackets -= numPackets;
				logger.trace("Link Status(NOCP): currentPackets=" + context.currentPackets
					+ ", remainingPackets=" + (context.maxPackets - context.currentPackets));
			} else {
				logger.warn("NOCP: Unknown handle=" + Utils.toHexString(handle, 2));
			}
		}
	}
	eventEncryptionChange(buffer) {
		let status = buffer.getInt8();
		let handle = buffer.getInt16() & 0x0FFF;
		let enabled = buffer.getInt8();
		let link = this.getACLLink(handle);
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
	eventEncryptionKeyRefreshComplete(buffer) {
		let status = buffer.getInt8();
		let handle = buffer.getInt16() & 0x0FFF;
		let link = this.getACLLink(handle);
		if (link != null) {
			if (status == 0) {
				link.encryptionRefreshed();
			} else {
				link.encryptionFailed(status);
			}
		} else {
			logger.warn("EKRC: Unknown handle=" + Utils.toHexString(handle, 2));
		}
	}
	eventLEConnectionComplete(buffer) {
		let status = buffer.getInt8();
		if (status == 0x00) {
			let handle = buffer.getInt16() & 0x0FFF;
			let role = buffer.getInt8();
			let random = buffer.getInt8() == 0x01;
			let address = BluetoothAddress.getByAddress(buffer.getByteArray(6), random);
			let connParameters = {
				interval: buffer.getInt16(),
				latency: buffer.getInt16(),
				supervisionTimeout: buffer.getInt16()
			};
			let masterClockAccuracy = buffer.getInt8();
			let link = new LELink(handle, null, address, {
				role,
				connParameters,
				masterClockAccuracy
			}, this);
			this.addACLLink(link);
		} else {
			logger.error("Connection complete with error: status="
				+ Utils.toHexString(status));
			// TODO: Notify upper layer
		}
	}
	eventLEConnectionUpdateComplete(buffer) {
		let status = buffer.getInt8();
		if (status == 0x00) {
			let handle = buffer.getInt16() & 0x0FFF;
			let connParameters = {
				interval: buffer.getInt16(),
				latency: buffer.getInt16(),
				supervisionTimeout: buffer.getInt16()
			};
			let link = this.getACLLink(handle);
			if (link != null) {
				link.connectionUpdated(connParameters);
			}
		} else {
			logger.error("Connection update complete with error: status="
				+ Utils.toHexString(status));
		}
	}
	eventLELTKRequest(buffer) {
		let handle = buffer.getInt16() & 0x0FFF;
		let random = buffer.getByteArray(8);
		let ediv = buffer.getInt16();
		let link = this.getACLLink(handle);
		if (link != null && link.isLELink()) {
			link.longTermKeyRequested(random, ediv);
		} else {
			logger.warn("LTKRequested: Unknown or NonLE handle=" + Utils.toHexString(handle, 2));
		}
	}
}

/**
 * A class represents ACL-U and LE-U
 */
class ACLLink {
	constructor(handle, address, le, linkMgr) {
		this._handle = handle;
		this._address = address;
		this._le = le;
		this._linkMgr = linkMgr;
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
	isDisconnected() {
		return (this._handle & HANDLE_DISCONNECTED_MASK) > 0;
	}
	isLELink() {
		return this._le;
	}
	send(buffer) {
		if (this.isDisconnected()) {
			logger.error("Link is disconnected");
			return;
		}
		let context = this._linkMgr.getLinkContext(this._le);
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
			this._linkMgr.submitData(this.handle, packetBoundary, 0x00, fragment);
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
	disconnected(reason) {
		this._handle &= HANDLE_DISCONNECTED_MASK;
		this._delegate.disconnected(reason);
	}
	updateEncryptionStatus(enabled) {
		this._encryptionStatus = enabled;
		this._security.encryptionEnabled(this);
	}
	encryptionRefreshed() {
		this._security.encryptionEnabled(this);
	}
	encryptionFailed(status) {
		this._security.encryptionFailed(status);
	}
}

class LELink extends ACLLink {
	constructor(handle, localAddress, remoteAddress, info, linkMgr) {
		super(handle, remoteAddress, true, linkMgr);
		this._localAddress = localAddress;
		this._remoteAddress = remoteAddress;
		this._slave = (info.role == 0x01);
		this._connParameters = info.connParameters;
	}
	isLESlave() {
		return this._slave;
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
	get connParameters() {
		return this._connParameters;
	}
	startEncryption(random, ediv, ltk) {
		return this._linkMgr.context.commands.le.startEncryption(this.handle, random, ediv, ltk);
	}
	replyLongTermKey(ltk) {
		if (ltk != null) {
			return this._linkMgr.context.commands.le.longTermKeyRequestReply(this.handle, ltk);
		} else {
			return this._linkMgr.context.commands.le.longTermKeyRequestNegativeReply(this.handle);
		}
	}
	longTermKeyRequested(random, ediv) {
		if (this._security == null) {
			this._linkMgr.context.commands.le.longTermKeyRequestNegativeReply(this.handle);
		} else {
			this._security.longTermKeyRequested(this, random, ediv);
		}
	}
	connectionUpdated(connParameters) {
		this._connParameters = connParameters;
		this._delegate.connectionUpdated(connParameters);
	}
}

/******************************************************************************
 * HCI Command
 ******************************************************************************/
class CommandExecutor {
	constructor(transport) {
		this._transport = transport;
		this._commandCallbacks = {};
		this._numCommandsAllowed = 1;
		this._pendingCommands = [];
	}
	enqueueCommand(command, callback = null) {
		logger.trace("Enqueue Command: opcode=" + Utils.toHexString(command.opcode, 2));
		this._pendingCommands.push({
			command: command,
			callback: callback
		});
		this.processPendingCommands();
	}
	getCallbackQueue(opcode) {
		if (this._commandCallbacks[opcode] == null) {
			this._commandCallbacks[opcode] = [];
		}
		return this._commandCallbacks[opcode];
	}
	processPendingCommands() {
		let numProcessed = 0;
		while (this._numCommandsAllowed > 0 && this._pendingCommands.length > 0) {
			let obj = this._pendingCommands.shift();
			let queue = this.getCallbackQueue(obj.command.opcode);
			queue.push(obj.callback);
			logger.trace("Command: opcode=" + Utils.toHexString(obj.command.opcode, 2)
				+ " " + Utils.toFrameString(obj.command.data, 0, obj.command.length));
			this._transport.sendCommand(obj.command);
			this._numCommandsAllowed--;
			numProcessed++;
		}
		logger.trace("Command Status: current numProcessed=" + numProcessed
			+ ", nca=" + this._numCommandsAllowed
			+ ", pending=" + this._pendingCommands.length);
	}
	submitCommand(opcode, buffer, callback) {
		let command = {
			opcode: opcode
		};
		if (buffer === undefined || buffer == null) {
			command.length = 0;
			command.data = null;
		} else {
			command.length = buffer.remaining();
			command.data = buffer.getByteArray(command.length);
		}
		this.enqueueCommand(command, callback);
	}
	submitCommandWithArray(opcode, array, callback) {
		this.enqueueCommand({
			opcode: opcode,
			length: array.length,
			data: array
		}, callback);
	}
	commandEventReceived(complete, buffer) {
		let status;
		if (!complete) {
			status = buffer.getInt8();
		}
		this._numCommandsAllowed = buffer.getInt8();
		if (this._numCommandsAllowed == 0) {
			logger.info("Controller set NCA to zero");
		} else {
			this.processPendingCommands();
		}
		let opcode = buffer.getInt16();
		let queue = this.getCallbackQueue(opcode);
		logger.trace("Command Event: opcode=" + Utils.toHexString(opcode, 2) + ", complete=" + complete
			+ ", queue.length=" + queue.length);
		let callback;
		if (queue.length > 0) {
			callback = queue.shift();
			if (callback == undefined) {
				callback = null;
			}
		}
		if (!complete) {
			if (status != 0) {
				logger.warn("Command Status with error: opcode="
					+ Utils.toHexString(opcode, 2)
					+ ", status=" + Utils.toHexString(status));
			}
			if (callback != null && "commandStatus" in callback) {
				callback.commandStatus(opcode, status);
			}
		} else {
			status = buffer.peek();	// XXX: May not be status
			if (status != 0) {
				logger.warn("Command Complete with error: opcode="
					+ Utils.toHexString(opcode, 2)
					+ ", status=" + Utils.toHexString(status));
			}
			if (callback != null && "commandComplete" in callback) {
				callback.commandComplete(opcode, buffer);
			}
		}
	}
}
