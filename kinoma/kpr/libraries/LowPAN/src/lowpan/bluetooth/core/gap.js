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

const BTUtils = require("./btutils");
const BluetoothAddress = BTUtils.BluetoothAddress;

const logger = new Logger("GAP");
logger.loggingLevel = Utils.Logger.Level.INFO;

exports.setLoggingLevel = level => logger.loggingLevel = level;

/* BT Stack Layer */
const HCI = require("./hci");
const L2CAP = require("./l2cap");
const GATT = require("./gatt");
const SM = require("./sm");
exports.HCI = HCI;
exports.L2CAP = L2CAP;
exports.GATT = GATT;
exports.SM = SM;

/* Bypass ADType flag */
const DISCOVER_BYPASS = true;

const SCAN_FAST_INTERVAL = 0x0030;		// TGAP(scan_fast_interval)		30ms to 60ms
const SCAN_FAST_WINDOW = 0x0030;			// TGAP(scan_fast_window)		30ms
const SCAN_SLOW_INTERVAL1 = 0x0800;		// TGAP(scan_slow_interval1)	1.28s
const SCAN_SLOW_WINDOW1 = 0x0012;			// TGAP(scan_slow_window1)		11.25ms
const SCAN_SLOW_INTERVAL2 = 0x1000;		// TGAP(scan_slow_interval2)	2.56s
const SCAN_SLOW_WINDOW2 = 0x0024;			// TGAP(scan_slow_window2)		22.5ms
const ADV_FAST_INTERVAL1 = {				// TGAP(adv_fast_interval1)		30ms to 60ms
	intervalMin: 0x0030,
	intervalMax: 0x0060
};
const ADV_FAST_INTERVAL2 = {				// TGAP(adv_fast_interval2)		100ms to 150ms
	intervalMin: 0x00A0,
	intervalMax: 0x00F0
};
const ADV_SLOW_INTERVAL = {				// TGAP(adv_slow_interval)		1s to 1.2s
	intervalMin: 0x0640,
	intervalMax: 0x0780
};
var MIN_INITIAL_CONN_INTERVAL = 0x18;	// TGAP(initial_conn_interval)	30ms to 50ms
var MAX_INITIAL_CONN_INTERVAL = 0x28;	// TGAP(initial_conn_interval)	30ms to 50ms

const DEFAULT_IR = [0x82, 0xA2, 0xE2, 0x62, 0x62, 0x63, 0x63, 0x63, 0x88, 0x88, 0x89, 0x8A, 0x8C, 0x80, 0x98, 0xA8];
const DEFAULT_ER = [0xEC, 0x2C, 0xAC, 0xAC, 0xAC, 0xAC, 0xAC, 0xAC, 0xD9, 0xDB, 0xDE, 0xD4, 0xC0, 0xE8, 0xB8, 0x18];

const DEFAULT_MTU = 158;

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

/** GAP local context */
var _profile = new GATT.Profile();
var _database = new GATT.ATT.AttributeDatabase();
var _privacyEnabled = false;
var _privateAddress = null;
var _staticAddress = null;
var _ownAddressType = HCI.LE.OwnAddressType.PUBLIC;
var _keyMgmt = new SM.DefaultKeyManagement(DEFAULT_IR, DEFAULT_ER);
var _localIRK = null;
var _connectableMode = ConnectableMode.NON_CONNECTABLE;
var _discoverableMode = DiscoverableMode.NON_DISCOVERABLE;

/** Upper layer application instance/module */
var _application = null;
var _storage = null;

exports.activate = function (transport, application, storage, resetHCI = true) {
	if (application == null) {
		throw "application is null";
	}
	_application = application;
	_storage = storage;

	HCI.registerTransport(transport);
	HCI.LE.callback.discovered = reports => {
		let discoveredList = new Array();
		for (let i = 0; i < reports.length; i++) {
			let discovered = processDiscovered(reports[i]);
			if (discovered != null) {
				discoveredList.push(discovered);
			}
		}
		if (discoveredList.length > 0) {
			_application.gapDiscovered(discoveredList);
		}
	};

	L2CAP.registerDelegate({
		l2capReady: function () {
			logger.debug("L2CAP Ready");
			_application.gapReady();
		},
		l2capConnected: function (connectionManager) {
			logger.debug("L2CAP Connected");
			let link = connectionManager.getHCILink();
			if (!link.isLELink()) {
				logger.debug("Ignore Non-LE Connection");
				return;
			}
			if (_ownAddressType == HCI.LE.OwnAddressType.RANDOM) {
				if (_privacyEnabled) {
					link.localAddress = _privateAddress;
				} else {
					link.localAddress = _staticAddress;
				}
			}
			let gapCtx = new GAPContext(connectionManager);
			if (link.remoteAddress.isResolvable()) {
				logger.debug("Connected peer uses RPA: " + link.remoteAddress.toString());
				resolvePrivateAddress(link.remoteAddress).then(address => {
					let rpa = link.remoteAddress;
					if (address != null) {
						logger.debug("Update remote identity: " + address.toString());
						link.remoteAddress = address;
					}
					_application.gapConnected(gapCtx, rpa);
				});
			} else {
				_application.gapConnected(gapCtx, null);
			}
		}
	});
	L2CAP.registerHCI(HCI);
	SM.registerHCI(HCI);

	HCI.activate(resetHCI);
};

function processDiscovered(report) {
	if (report.eventType == HCI.LEEvent.EventType.ADV_DIRECT_IND) {
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

class GAPContext {
	constructor(connectionManager) {
		this._connectionManager = connectionManager;
		this._connectionManager.delegate = this;
		this._delegate = null;
		// TODO: Open SDP Channel
		/* Open ATT Channel */
		let attCID = L2CAP.LEChannel.ATTRIBUTE_PROTOCOL;
		this._bearer = new GATT.ATT.ATTBearer(
			connectionManager.openConnection(attCID, attCID),
			_database);
		this._bearer.mtu = DEFAULT_MTU;
		/* Open SMP Channel */
		let smpCID = L2CAP.LEChannel.SECURITY_MANAGER_PROTOCOL;
		this._security = new SM.SecurityManagement(
			connectionManager.openConnection(smpCID, smpCID),
			_keyMgmt,
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
	get peripheral() {
		return this._connectionManager.getHCILink().isLESlave();
	}
	get bearer() {
		return this._bearer;
	}
	get security() {
		return this._security;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	isDisconnected() {
		return this._connectionManager.getHCILink() == null;
	}
	disconnect(reason) {
		let link = this._connectionManager.getHCILink();
		if (link == null) {
			logger.info("GAP connection has already been disconnected");
			return;
		}
		logger.debug("Disconnect link: handle=" + Utils.toHexString(link.handle, 2));
		link.disconnect(reason);
	}
	/* L2CAP Callback (ConnectionManager) */
	disconnected() {
		this._delegate.disconnected();
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
		} else {
			let identityAddress = null;
			let link = this._connectionManager.getHCILink();
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
				_storage.storeBond(div, {pairingInfo, address: identityAddress});
				logger.debug("Bonding Information stored: DIV=" + div);
			}
		}
		// TODO: Update security level on ATT Bearer
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
		let link = this._connectionManager.getHCILink();
		let remoteAddress = link.remoteAddress;
		if (!remoteAddress.isIdentity()) {
			logger.error("Not an identity address");
			return null;
		}
		let div = _storage.findBondIndexByAddress(remoteAddress);
		if (div != -1) {
			// TODO: Check security level
			return _storage.getBond(div);
		}
		return null;
	}
	/* SM Callback (SecurityManagement) */
	findBondByDIV(div) {
		return _storage.getBond(div);
	}
	/* SM Callback (SecurityManagement) */
	generateDIV() {
		return _storage.allocateBond();
	}
}

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 9.3.9 Connection Parameter Update Procedure
 */
exports.updateConnectionParameter = function (bearer, connParameter) {
	var handle = bearer.connection.getHCIHandle();
	HCI.LE.connectionUpdate(handle, connParameter);
};

function updateRandomAddress(address) {
	HCI.LE.setRandomAddress(address, {
		commandComplete: (opcode, buffer) => {
			let status = buffer.getInt8();
			if (status == 0) {
				logger.debug("OwnAddressType is changed to Random");
				_ownAddressType = HCI.LE.OwnAddressType.RANDOM;
				_privacyEnabled = address.isResolvable();
				if (_privacyEnabled) {
					HCI.LE.setAdvertiseEnable(true);
					// TODO: Need to schedule RPA refresing for at least every 15min
					_application.privacyEnabled(address);
				}
			} else {
				// TODO: Failed
			}
		}
	});
}

function restoreIdentityAddress() {
	if (_staticAddress == null) {
		logger.debug("OwnAddressType is changed to Public");
		_ownAddressType = HCI.LE.OwnAddressType.PUBLIC;
		return;
	}
	updateRandomAddress(_staticAddress);
}

/**
 * Core 4.2 Specification, Vol 6, Part B: Link Layer Specification
 * 1.3.2.1 Static Device Address
 */
exports.setStaticAddress = function (address) {
	if (address != null && (!address.isRandom() || !address.isIdentity())) {
		logger.error("Not a static random address");
		return;
	}
	_staticAddress = address;
	if (_privacyEnabled) {
		return;
	}
	restoreIdentityAddress();
};

function refreshLocalRPA() {
	SM.generatePrivateAddress(_localIRK).then(rpa => {
		logger.debug("RPA is generated:" + rpa.toString());
		_privateAddress = rpa;
		updateRandomAddress(_privateAddress);
	});
}

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 10.7 Privacy Feature
 */
exports.enablePrivacyFeature = function (enabled) {
	if (enabled) {
		if (_localIRK == null) {
			_keyMgmt.generateIRK().then(irk => {
				_localIRK = irk;
				refreshLocalRPA();
			});
		} else {
			refreshLocalRPA();
		}
	} else {
		restoreIdentityAddress();
	}
};

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 10.8.2.3 Resolvable Private Address Resolution Procedure
 */
function resolvePrivateAddress(address) {
	if (!address.isResolvable()) {
		logger.error("Not a resolvable address");
		return;
	}
	let bonds = _storage.getBonds();
	let _loop = ctx => {
		for (let i = ctx.index; i < bonds.length; i++) {
			let bond = bonds[ctx.index];
			if (bond.hasOwnProperty("pairingInfo") && bond.pairingInfo.keys.identityResolvingKey != null) {
				let irk = bond.pairingInfo.keys.identityResolvingKey;
				return SM.resolvePrivateAddress(irk, address).then(resolved => {
					if (resolved) {
						return bond.address;
					} else {
						ctx.index++;
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
};
exports.resolvePrivateAddress = resolvePrivateAddress;

exports.deployServices = function (services) {
	return _profile.deployServices(_database, services);
};

exports.getServiceByUUID = function (uuid) {
	return _profile.getServiceByUUID(uuid);
};

/******************************************************************************
 * LE Central
 ******************************************************************************/

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 9.2.6 General Discovery Procedure
 */
exports.startScanning = function (interval, window, duplicatesFilter) {
	if (duplicatesFilter === undefined) {
		duplicatesFilter = true;
	}
	HCI.LE.setScanParameters({
		scanType: HCI.LE.LEScanType.ACTIVE,				// Active scanning
		interval,
		window,
		addressType: _ownAddressType,
		filterPolicy: HCI.LE.ScanningFilterPolicy.ALL,	// Accept all
	});
	HCI.LE.setScanEnable(true, duplicatesFilter);
};

exports.stopScanning = function () {
	HCI.LE.setScanEnable(false, true);
};

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 9.3.8 Direct Connection Establishment Procedure
 */
exports.directConnection = function (address, connParameter) {
	if (connParameter === undefined || connParameter == null) {
		/* Default parameters as described in 9.3.12.2 */
		connParameter = {
			intervalMin: MIN_INITIAL_CONN_INTERVAL,
			intervalMax: MAX_INITIAL_CONN_INTERVAL,
			latency: 0,
			supervisionTimeout: supervisionTimeout,
			minimumCELength: 0,			// Most implementation uses zero
			maximumCELength: 0			// Most implementation uses zero
		};
	}
	HCI.LE.createConnection(
		/* Scan Parameter as described in 9.3.11.2 */
		{
			interval: SCAN_FAST_INTERVAL,
			window: SCAN_FAST_WINDOW
		},
		false,							// Ignore white list
		address.isRandom() ? 0x01 : 0x00,
		address,
		_ownAddressType,
		connParameter
	);
};

/******************************************************************************
 * LE Peripheral
 ******************************************************************************/

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 9.2 Discovery Modes and Procedures
 */
exports.setDiscoverableMode = function (discoverableMode) {
	if (_discoverableMode == discoverableMode) {
		return;
	}
	_discoverableMode = discoverableMode;
};

/**
 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
 * 9.3 Connection Modes and Procedures
 */
exports.setConnectableMode = function (connectableMode) {
	if (_connectableMode == connectableMode) {
		return;
	}
	_connectableMode = connectableMode;
};

exports.startAdvertising = function (intervalMin, intervalMax, structures = null) {
	if (_connectableMode == ConnectableMode.DIRECTED) {
		logger.error("TODO: Directed connectable mode is not supported");
		return;
	}
	if (structures == null) {
		structures = new Array();
	}
	let flags = Flags.NO_BR_EDR;
	switch (_discoverableMode) {
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
	HCI.LE.setAdvertisingData(toAdvertisingDataArray(structures));
	let advType = HCI.LE.AdvertisingType.ADV_IND;
	if (_connectableMode == ConnectableMode.NON_CONNECTABLE) {
		advType = HCI.LE.AdvertisingType.ADV_NONCONN_IND;		// XXX: Or ADV_SCAN_IND
		// TODO: Check intervals
	}
	HCI.LE.setAdvertisingParameters(
		{intervalMin, intervalMax},
		advType,
		_ownAddressType,
		0, null,
		0x7,											// XXX: All channel
		HCI.LE.AdvertisingFilterPolicy.ALL
	);
	HCI.LE.setAdvertiseEnable(true);
};

exports.stopAdvertising = function () {
	HCI.LE.setAdvertiseEnable(false);
};

exports.setScanResponseData = function (structures) {
	HCI.LE.setScanResponseData(toAdvertisingDataArray(structures));
};

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

function toAdvertisingDataArray(structures) {
	var buffer = ByteBuffer.allocateUint8Array(MAX_AD_LENGTH, true);
	writeAdvertisingData(buffer, structures);
	buffer.flip();
	return buffer.getByteArray();
}

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
