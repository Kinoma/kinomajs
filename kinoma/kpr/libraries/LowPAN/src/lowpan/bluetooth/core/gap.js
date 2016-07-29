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
 * Bluetooth v4.2 - Generic Access Profile (LE Only)
 */

const Utils = require("../../common/utils");
const Logger = Utils.Logger;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;
const AESCMAC = require("../../common/aescmac");

const BTUtils = require("./btutils");
const BluetoothAddress = BTUtils.BluetoothAddress;

const logger = Logger.getLogger("GAP");

/* BT Stack Layer */
const HCI = require("./hci");
const L2CAP = require("./l2cap");
const SM = require("./sm");

/* Bypass ADType flag */
const DISCOVER_BYPASS = true;

const SCAN_FAST_INTERVAL = 0x0030;			// TGAP(scan_fast_interval)		30ms to 60ms
const SCAN_FAST_WINDOW = 0x0030;			// TGAP(scan_fast_window)		30ms
const SCAN_SLOW_INTERVAL1 = 0x0800;			// TGAP(scan_slow_interval1)	1.28s
const SCAN_SLOW_WINDOW1 = 0x0012;			// TGAP(scan_slow_window1)		11.25ms
const SCAN_SLOW_INTERVAL2 = 0x1000;			// TGAP(scan_slow_interval2)	2.56s
const SCAN_SLOW_WINDOW2 = 0x0024;			// TGAP(scan_slow_window2)		22.5ms
const ADV_FAST_INTERVAL1 = {				// TGAP(adv_fast_interval1)		30ms to 60ms
	intervalMin: 0x0030,
	intervalMax: 0x0060
};
const ADV_FAST_INTERVAL2 = {				// TGAP(adv_fast_interval2)		100ms to 150ms
	intervalMin: 0x00A0,
	intervalMax: 0x00F0
};
const ADV_SLOW_INTERVAL = {					// TGAP(adv_slow_interval)		1s to 1.2s
	intervalMin: 0x0640,
	intervalMax: 0x0780
};
var MIN_INITIAL_CONN_INTERVAL = 0x18;		// TGAP(initial_conn_interval)	30ms to 50ms
var MAX_INITIAL_CONN_INTERVAL = 0x28;		// TGAP(initial_conn_interval)	30ms to 50ms

const DEFAULT_IR = [0x82, 0xA2, 0xE2, 0x62, 0x62, 0x63, 0x63, 0x63, 0x88, 0x88, 0x89, 0x8A, 0x8C, 0x80, 0x98, 0xA8];
const DEFAULT_ER = [0xEC, 0x2C, 0xAC, 0xAC, 0xAC, 0xAC, 0xAC, 0xAC, 0xD9, 0xDB, 0xDE, 0xD4, 0xC0, 0xE8, 0xB8, 0x18];

const DiscoverableMode = {
	NON_DISCOVERABLE: 0,
	LIMITED: 1,
	GENERAL: 2
};
exports.DiscoverableMode = DiscoverableMode;

const ConnectableMode = {
	NON_CONNECTABLE: 0,
	DIRECTED: 1,
	UNDIRECTED: 2
};
exports.ConnectableMode = ConnectableMode;

const SecurityMode = {
	MODE1_LEVEL1: null,
	MODE1_LEVEL2: {
		authentication: false,
		encryption: true,
		secureConnection: false
	},
	MODE1_LEVEL3: {
		authentication: true,
		encryption: true,
		secureConnection: false
	},
	MODE1_LEVEL4: {
		authentication: true,
		encryption: true,
		secureConnection: true
	},
	MODE2_LEVEL1: {
		authentication: false,
		encryption: false,
		secureConnection: false
	},
	MODE2_LEVEL2: {
		authentication: true,
		encryption: false,
		secureConnection: false
	}
};
exports.SecurityMode = SecurityMode;

exports.createLayer = (application, storage) => {
	logger.info("Init");
	return new Context(application, storage);
};

class Context {
	constructor(application, storage) {
		this._application = application;
		this._storage = storage;
		this._privacyEnabled = false;
		this._privateAddress = null;
		this._staticAddress = null;
		this._ownAddressType = HCI.LEConst.OwnAddressType.PUBLIC;
		this._keyMgmt = new SM.DefaultKeyManagement(DEFAULT_IR, DEFAULT_ER);
		this._localIRK = null;
		this._connectableMode = ConnectableMode.NON_CONNECTABLE;
		this._discoverableMode = DiscoverableMode.NON_DISCOVERABLE;
	}
	init(transport, resetHCI = true) {
		/* HCI Layer */
		this._hci = HCI.createLayer(transport);
		this._hci.discoveryCallback = reports => {
			let discoveredList = new Array();
			for (let i = 0; i < reports.length; i++) {
				let discovered = processDiscovered(reports[i]);
				if (discovered != null) {
					discoveredList.push(discovered);
				}
			}
			if (discoveredList.length > 0) {
				this._application.gapDiscovered(discoveredList);
			}
		};
		/* L2CAP Layer */
		this._l2cap = L2CAP.createLayer(this._hci);
		this._l2cap.delegate = this;
		this._hci.init(resetHCI);
	}
	get hci() {
		return this._hci;
	}
	get storage() {
		return this._storage;
	}
	get keyManagement() {
		return this._keyMgmt;
	}
	/* L2CAP Callback */
	l2capReady() {
		logger.debug("L2CAP Ready");
		this._keyMgmt.encrypt = this._hci.encrypt;
		this._keyMgmt.random64 = this._hci.random64;
		this._application.gapReady();
	}
	/* L2CAP Callback */
	l2capConnected(connectionManager, signalingCtx) {
		logger.debug("L2CAP Connected");
		let link = connectionManager.link;
		if (!link.isLELink()) {
			logger.debug("Ignore Non-LE Connection");
			return;
		}
		if (this._ownAddressType == HCI.LEConst.OwnAddressType.RANDOM) {
			if (this._privacyEnabled) {
				link.localAddress = this._privateAddress;
			} else {
				link.localAddress = this._staticAddress;
			}
		}
		let gapConn = new GAPConnection(this, connectionManager, signalingCtx);
		// TODO: Check if there is bonding
		if (link.remoteAddress.isResolvable()) {
			logger.debug("Connected peer uses RPA: " + link.remoteAddress.toString());
			this.resolvePrivateAddress(link.remoteAddress).then(address => {
				let rpa = link.remoteAddress;
				if (address != null) {
					logger.debug("Update remote identity: " + address.toString());
					link.remoteAddress = address;
				}
				this._application.gapConnected(gapConn, rpa);
			});
		} else {
			this._application.gapConnected(gapConn, null);
		}
	}
	disconnect(gapConn, reason) {
		let handle = gapConn.handle;
		logger.debug("Disconnect link: handle=" + Utils.toHexString(handle, 2));
		this._hci.commands.linkControl.disconnect(handle, reason);
	}
	_updateRandomAddress(address) {
		this._hci.commands.le.setRandomAddress(address, {
			commandComplete: (opcode, buffer) => {
				let status = buffer.getInt8();
				if (status == 0) {
					logger.debug("OwnAddressType is changed to Random");
					this._ownAddressType = HCI.LEConst.OwnAddressType.RANDOM;
					this._privacyEnabled = address.isResolvable();
					if (this._privacyEnabled) {
						this._hci.commands.le.setAdvertiseEnable(true);
						// TODO: Need to schedule RPA refresing for at least every 15min
						this._application.privacyEnabled(address);
					}
				} else {
					// TODO: Failed
				}
			}
		});
	}
	_restoreIdentityAddress() {
		if (this._staticAddress == null) {
			logger.debug("OwnAddressType is changed to Public");
			this._ownAddressType = HCI.LEConst.OwnAddressType.PUBLIC;
			return;
		}
		this._updateRandomAddress(this._staticAddress);
	}
	/**
	 * Core 4.2 Specification, Vol 6, Part B: Link Layer Specification
	 * 1.3.2.1 Static Device Address
	 */
	setStaticAddress(address) {
		if (address != null && (!address.isRandom() || !address.isIdentity())) {
			logger.error("Not a static random address");
			return;
		}
		this._staticAddress = address;
		if (this._privacyEnabled) {
			return;
		}
		this._restoreIdentityAddress();
	}
	_refreshLocalRPA() {
		SM.generatePrivateAddress(this._hci.encrypt, this._hci.random64, this._localIRK).then(rpa => {
			logger.debug("RPA is generated:" + rpa.toString());
			this._privateAddress = rpa;
			_updateRandomAddress(this._privateAddress);
		});
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 10.7 Privacy Feature
	 */
	enablePrivacyFeature(enabled) {
		if (enabled) {
			if (this._localIRK == null) {
				this._keyMgmt.generateIRK().then(irk => {
					this._localIRK = irk;
					this._refreshLocalRPA();
				});
			} else {
				this._refreshLocalRPA();
			}
		} else {
			this._restoreIdentityAddress();
		}
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 10.8.2.3 Resolvable Private Address Resolution Procedure
	 */
	resolvePrivateAddress(address) {
		if (!address.isResolvable()) {
			logger.error("Not a resolvable address");
			return;
		}
		let bonds = this._storage.getBonds();
		let _loop = ctx => {
			for (let i = ctx.index; i < bonds.length; i++) {
				let bond = bonds[i];
				if (bond.hasOwnProperty("keys") && bond.keys.identityResolvingKey != null) {
					let irk = bond.keys.identityResolvingKey;
					return SM.resolvePrivateAddress(this._hci.encrypt, irk, address).then(resolved => {
						if (resolved) {
							return bond.address;
						} else {
							ctx.index = i + 1;
							return _loop(ctx);
						}
					});
				}
			}
			return Promise.resolve(null);

		};
		return _loop({
			index: 0
		});
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.2.6 General Discovery Procedure
	 */
	startScanning(interval, window, duplicatesFilter) {
		if (duplicatesFilter === undefined) {
			duplicatesFilter = true;
		}
		this._hci.commands.le.setScanParameters({
			scanType: HCI.LEConst.ScanType.ACTIVE,				// Active scanning
			interval,
			window,
			addressType: this._ownAddressType,
			filterPolicy: HCI.LEConst.ScanningFilterPolicy.ALL,	// Accept all
		});
		this._hci.commands.le.setScanEnable(true, duplicatesFilter);
	}
	stopScanning() {
		this._hci.commands.le.setScanEnable(false, true);
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3.8 Direct Connection Establishment Procedure
	 */
	directConnection(address, connParameter) {
		if (connParameter === undefined || connParameter == null) {
			/* Default parameters as described in 9.3.12.2 */
			connParameter = {
				intervalMin: MIN_INITIAL_CONN_INTERVAL,
				intervalMax: MAX_INITIAL_CONN_INTERVAL,
				latency: 0,
				supervisionTimeout: supervisionTimeout,	// FIXME
				minimumCELength: 0,			// Most implementation uses zero
				maximumCELength: 0			// Most implementation uses zero
			};
		}
		this._hci.commands.le.createConnection(
			/* Scan Parameter as described in 9.3.11.2 */
			{
				interval: SCAN_FAST_INTERVAL,
				window: SCAN_FAST_WINDOW
			},
			false,							// Ignore white list
			address.isRandom() ? 0x01 : 0x00,
			address,
			this._ownAddressType,
			connParameter
		);
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.2 Discovery Modes and Procedures
	 */
	setDiscoverableMode(discoverableMode) {
		if (this._discoverableMode == discoverableMode) {
			return;
		}
		this._discoverableMode = discoverableMode;
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3 Connection Modes and Procedures
	 */
	setConnectableMode(connectableMode) {
		if (this._connectableMode == connectableMode) {
			return;
		}
		this._connectableMode = connectableMode;
	}
	startAdvertising(intervalMin, intervalMax, structures = null) {
		if (this._connectableMode == ConnectableMode.DIRECTED) {
			logger.error("TODO: Directed connectable mode is not supported");
			return;
		}
		if (structures == null) {
			structures = new Array();
		}
		let flags = Flags.NO_BR_EDR;
		switch (this._discoverableMode) {
		case DiscoverableMode.LIMITED:
			flags |= Flags.LE_LIMITED_DISCOVERABLE_MODE;
			break;
		case DiscoverableMode.GENERAL:
			flags |= Flags.LE_GENERAL_DISCOVERABLE_MODE;
			break;
		}
		structures.push({
			type: ADType.FLAGS,
			data: [flags]
		});
		this._hci.commands.le.setAdvertisingData(toAdvertisingDataArray(structures));
		let advType = HCI.LEConst.AdvertisingType.ADV_IND;
		if (this._connectableMode == ConnectableMode.NON_CONNECTABLE) {
			advType = HCI.LEConst.AdvertisingType.ADV_NONCONN_IND;		// XXX: Or ADV_SCAN_IND
			// TODO: Check intervals
		}
		this._hci.commands.le.setAdvertisingParameters(
			{intervalMin, intervalMax},
			advType,
			this._ownAddressType,
			0, null,
			0x7,											// XXX: All channel
			HCI.LEConst.AdvertisingFilterPolicy.ALL
		);
		this._hci.commands.le.setAdvertiseEnable(true);
	}
	stopAdvertising() {
		this._hci.commands.le.setAdvertiseEnable(false);
	}
	setScanResponseData(structures) {
		this._hci.commands.le.setScanResponseData(toAdvertisingDataArray(structures));
	}
}

function processDiscovered(report) {
	if (report.eventType == HCI.LEConst.EventType.ADV_DIRECT_IND) {
		// TODO
		return null;
	}
	if (report.length == 0) {
		return null;
	}
	let structures = readAdvertisingData(
		ByteBuffer.wrap(report.data, 0, report.length, true));
	let discovered = DISCOVER_BYPASS;
	if (!discovered) {
		for (let i = 0; i < structures.length; i++) {
			if (structures[i].type != ADType.FLAGS) {
				return null;
			}
			if ((structures[i].data[0] &
					(Flags.LE_LIMITED_DISCOVERABLE_MODE |
					 Flags.LE_GENERAL_DISCOVERABLE_MODE)) > 0) {
				/* FIXME: Mark as discovered for both general and limited */
				discovered = true;
				break;
			}
		}
	}
	if (discovered) {
		let random = false;
		if (report.peer.addressType == 0x01 || report.peer.addressType == 0x03) {
			random = true;
		}
		let remoteAddress = BluetoothAddress.getByAddress(report.peer.address, true, random);
		return {
			eventType: report.eventType,
			remoteAddress,
			rssi: report.rssi,
			structures
		};
	}
}

function toAdvertisingDataArray(structures) {
	var buffer = ByteBuffer.allocateUint8Array(MAX_AD_LENGTH, true);
	writeAdvertisingData(buffer, structures);
	buffer.flip();
	return buffer.getByteArray();
}

class ATTConnection {
	constructor(hci, connection) {
		this._hci = hci;
		this._connection = connection;
		this._connection.delegate = this;
		this._onReceived = null;
		this._pairingInfo = null;
		this._signCounter = 0;	// XXX: Need to be stored in extarnal database
	}
	set onReceived(callback) {
		this._onReceived = callback;
	}
	get identifier() {
		return this._connection.link.handle;
	}
	get pairingInfo() {
		return this._pairingInfo;
	}
	set pairingInfo(info) {
		logger.debug("pairingInfo is updated on ATT connection");
		this._pairingInfo = info;
	}
	get encrypted() {
		return this._connection.link.encrptionStatus == 0x01;
	}
	sendAttributePDU(pdu) {
		let signed = (pdu[0] & 0x80) > 0;
		if (signed) {
			logger.debug("Signature flag is enabled in PDU");
			if (this._pairingInfo == null || this._pairingInfo.keys.signatureKey == null) {
				logger.error("TODO: CSRK is not available (Calculation)");
				return;
			}
			let csrk = this._pairingInfo.keys.signatureKey;
			let message = new Uint8Array(pdu.length + 12);
			message.set(pdu);
			message.set(Utils.toByteArray(this._signCounter, Utils.INT_32_SIZE, true), pdu.length);
			AESCMAC.cmac(this._hci.encrypt, csrk, message, pdu.length + Utils.INT_32_SIZE).then(mac => {
				mac = mac.slice(Util.INT_64_SIZE);
				logger.debug("CMAC is calculated: mac=" + Utils.toFrameString(mac)
					+ ", signCounter=" + this._signCounter);
				this._signCounter++;
				message.set(mac, pdu.length + Utils.INT_32_SIZE);
				this._connection.sendBasicFrame(message);
			});
		} else {
			this._connection.sendBasicFrame(pdu);
		}
	}
	_received(buffer) {
		// TODO: FIFO Buffer
		if (this._onReceived == null) {
			logger.warn("ATT Received but no callback");
		} else {
			this._onReceived(buffer);
		}
	}
	/** L2CAP Connection delegate method */
	received(buffer) {
		const opcode = buffer.peek();
		logger.debug("ATT Received: opcode=" + Utils.toHexString(opcode));
		if ((opcode & 0x80) > 0) {
			logger.debug("Signature flag is enabled in PDU (RX)");
			if (this._pairingInfo == null || this._pairingInfo.keys.signatureKey == null) {
				logger.error("TODO: CSRK is not available (RX)");
				return;
			}
			buffer.mark();
			let csrk = this._pairingInfo.keys.signatureKey;
			let message = buffer.getByteArray(buffer.remaining() - Util.INT_64_SIZE);
			let signature = buffer.getByteArray();
			buffer.reset();
			AESCMAC.cmac(this._hci.encrypt, csrk, message).then(mac => {
				mac = mac.slice(Util.INT_64_SIZE);
				logger.debug("CMAC is calculated (RX): " + Utils.toFrameString(mac));
				opcode &= ~0x80;
				if (BTUtils.isArrayEquals(signature, mac)) {
					logger.debug("Signature verified");
					// TODO: Store counter
					this._received(buffer);
				} else {
					logger.error("Signature verification has failed");
				}
			});
		} else {
			this._received(buffer);
		}
	}
	/** L2CAP Connection delegate method */
	disconnected() {
	//	logger.info("Disconnected: pendingPDUs=" + this.pendingTransactions.length);
		this._connection = null;
	}
}

class GAPConnection {
	constructor(ctx, connectionManager, signalingCtx) {
		this._ctx = ctx;
		this._connectionManager = connectionManager;
		this._connectionManager.delegate = this;
		this._signalingCtx = signalingCtx;
		this._delegate = null;
		// TODO: Open SDP Channel
		/* Open ATT Channel */
		let attCID = L2CAP.LEChannel.ATTRIBUTE_PROTOCOL;
		this._attConnection = new ATTConnection(
			ctx.hci,
			connectionManager.openConnection(attCID, attCID));
		/* Open SMP Channel */
		let smpCID = L2CAP.LEChannel.SECURITY_MANAGER_PROTOCOL;
		this._security = new SM.SecurityManagement(
			ctx.hci,
			connectionManager.openConnection(smpCID, smpCID),
			ctx.keyManagement,
			{
				bonding: false,
				outOfBand: false,
				mitm: false,
				minKeySize: 7,
				maxKeySize: 16,
				display: false,
				keyboard: false,
				identityAddress: null	// Use public address as identity
			});
		this._security.delegate = this;
	}
	get handle() {
		return this._connectionManager.link.handle;
	}
	get peripheral() {
		return this._connectionManager.link.isLESlave();
	}
	get attConnection() {
		return this._attConnection;
	}
	get security() {
		return this._security;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3.9 Connection Parameter Update Procedure
	 */
	updateConnectionParameter(connParameter, l2cap = false) {
		if (l2cap) {
			this._signalingCtx.sendConnectionParameterUpdateRequest(connParameter, response => {
				logger.info("L2CAP CPU result=" + response.getInt16());
			});
		} else {
			this._ctx.hci.commands.le.connectionUpdate(this.handle, connParameter);
		}
	}
	isDisconnected() {
		return this._connectionManager.link == null;
	}
	/* L2CAP Callback (ConnectionManager) */
	disconnected(reason) {
		this._delegate.disconnected(reason);
	}
	/* SM Callback (SecurityManagement) */
	pairingFailed(reason) {
		logger.error("Pairing Failed: reason=" + Utils.toHexString(reason));
		// TODO
	}
	/* SM Callback (SecurityManagement) */
	passkeyRequested(input) {
		this._delegate.passkeyRequested(input);
	}
	/* SM Callback (SecurityManagement) */
	encryptionCompleted(div, pairingInfo) {
		if (pairingInfo == null) {
			logger.info("Encryption completed with bonded device");
			pairingInfo = this._ctx.storage.getBond(div);
			if (pairingInfo == null) {
				logger.warn("Could not find the bonding info: div=" + div);
			}
		} else {
			let identityAddress = null;
			let link = this._connectionManager.link;
			if (link.remoteAddress.isIdentity()) {
				identityAddress = link.remoteAddress;
			} else if (pairingInfo.keys.address != null) {
				identityAddress = pairingInfo.keys.address;
				logger.debug("Update remote identity: " + identityAddress.toString());
				link.remoteAddress = identityAddress;
				if (pairingInfo.keys.identityResolvingKey == null) {
					logger.warn("Identity received but IRK is not available");
				} else {
					logger.debug("IRK: " + Utils.toFrameString(pairingInfo.keys.identityResolvingKey));
				}
			} else {
				logger.warn("Cannot bond with non identity address");
			}
			if (pairingInfo.bonding && identityAddress != null) {
				pairingInfo.address = identityAddress;	// Save identity
				this._ctx.storage.storeBond(div, pairingInfo);
				logger.debug("Bonding Information stored: DIV=" + div);
			}
		}
		this._attConnection.pairingInfo = pairingInfo;	// Update security information
		this._delegate.encryptionCompleted(div, pairingInfo);
	}
	/* SM Callback (SecurityManagement) */
	encryptionFailed(status) {
		logger.error("Encryption Failed: status=" + Utils.toHexString(status));
		this._delegate.encryptionFailed(status);
	}
	/* SM Callback (SecurityManagement) */
	findBondByAddress(address) {
		logger.debug("Bonding Information requested");
		let link = this._connectionManager.link;
		let remoteAddress = link.remoteAddress;
		if (!remoteAddress.isIdentity()) {
			logger.error("Not an identity address");
			return null;
		}
		let div = this._ctx.storage.findBondIndexByAddress(remoteAddress);
		if (div != -1) {
			// TODO: Check security level
			return this._ctx.storage.getBond(div);
		}
		return null;
	}
	/* SM Callback (SecurityManagement) */
	findBondByDIV(div) {
		return this._ctx.storage.getBond(div);
	}
	/* SM Callback (SecurityManagement) */
	generateDIV() {
		return this._ctx.storage.allocateBond();
	}
}

/******************************************************************************
 * Advertisement Data
 ******************************************************************************/
var ADType = {
	/* Service UUID */
	INCOMPLETE_UUID16_LIST: 0x02,
	COMPLETE_UUID16_LIST: 0x03,
	INCOMPLETE_UUID128_LIST: 0x06,
	COMPLETE_UUID128_LIST: 0x07,
	/* Local Name */
	SHORTENED_LOCAL_NAME: 0x08,
	COMPLETE_LOCAL_NAME: 0x09,
	/* Flags */
	FLAGS: 0x01,
	/* Manufacturer Specific Data */
	MANUFACTURER_SPECIFIC_DATA: 0xFF,
	/* TX Power Level */
	TX_POWER_LEVEL: 0x0A,
	SLAVE_CONNECTION_INTERVAL_RANGE: 0x12,
	/* Service Solicitation */
	SOLICITATION_UUID16_LIST: 0x14,
	SOLICITATION_UUID128_LIST: 0x15,
	/* Service Data */
	SERVICE_DATA_UUID16: 0x16,
	SERVICE_DATA_UUID128: 0x21,
	/* Appearance */
	APPEARANCE: 0x19,
	/* Public Target Address */
	PUBLIC_TARGET_ADDRESS: 0x17,
	/* Random Target Address */
	RANDOM_TARGET_ADDRESS: 0x18,
	/* Advertising Interval */
	ADVERTISING_INTERVAL: 0x1A,
	/* LE Bluetooth Device Address */
	LE_BLUETOOTH_DEVICE_ADDRESS: 0x1B,
	/* LE Role */
	LE_ROLE: 0x1C,
	/* URI */
	URI: 0x24
};
exports.ADType = ADType;

var MAX_AD_LENGTH = 31;
exports.MAX_AD_LENGTH = MAX_AD_LENGTH;

var Flags = {
	LE_LIMITED_DISCOVERABLE_MODE: 0x01,
	LE_GENERAL_DISCOVERABLE_MODE: 0x02,
	NO_BR_EDR: 0x04,
	LE_BR_EDR_CONTROLLER: 0x08,
	LE_BR_EDR_HOST: 0x10,
};

function readAdvertisingData(buffer) {
	var structures = [];
	while (buffer.remaining() > 0) {
		var length = buffer.getInt8();
		if (length == 0) {
			/* Early termination of data */
			break;
		}
		var structure = {
			type: buffer.getInt8(),
			data: buffer.getByteArray(length - 1)
		};
		structures.push(structure);
	}
	return structures;
}

function writeAdvertisingData(buffer, structures) {
	for (var i = 0; i < structures.length; i++) {
		var structure = structures[i]
		if (structure.data == null) {
			break;
		}
		buffer.putInt8(structure.data.length + 1);
		buffer.putInt8(structure.type);
		buffer.putByteArray(structure.data);
	}
}
