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

		this._discoverAll = true;				// Discover all advertising packets
		this._discoverLimited = false;			// Discover only if limited AD flag is enabled
		this._currentScanParameters = null;		// Current in progress scan parameters
		this._connecting = false;				// Current connection procedure status
		this._currentAdvertisingParameters = null;
	}
	init(transport, resetHCI = true) {
		/* HCI Layer */
		this._hci = HCI.createLayer(transport);
		this._hci.discoveryCallback = reports => this._processDiscovered(reports);
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

		/* Connection procedure is done. */
		this._connecting = false;

		/* Update local address with the actual address we used
		 * during the connection.
		 */
		if (this._ownAddressType == HCI.LEConst.OwnAddressType.RANDOM) {
			if (this._privacyEnabled) {
				link.localAddress = this._privateAddress;
			} else {
				link.localAddress = this._staticAddress;
			}
		}
		let gapConn = new GAPConnection(this, connectionManager, signalingCtx);
		if (link.remoteAddress.isResolvable()) {
			logger.debug("Connected peer uses RPA: " + link.remoteAddress.toString());
			this.resolvePrivateAddress(link.remoteAddress).then(bond => {
				gapConn.securityInfo = bond;
				this._application.gapConnected(gapConn);
				gapConn.initChannels();
			});
		} else {
			if (link.remoteAddress.isIdentity()) {
				gapConn.securityInfo = this._storage.findBondByAddress(link.remoteAddress);
			}
			this._application.gapConnected(gapConn);
			gapConn.initChannels();
		}
	}
	_updateRandomAddress(address) {
		return this._hci.commands.le.setRandomAddress(address).then(response => {
			logger.debug("OwnAddressType is changed to Random");
			this._ownAddressType = HCI.LEConst.OwnAddressType.RANDOM;
			this._privacyEnabled = address.isRandom() && !address.isIdentity();
			if (this._privacyEnabled) {
				// TODO: Need to schedule RPA refresing for at least every 15min
				this._application.privacyEnabled(address);
			}
		});
	}
	_restoreIdentityAddress() {
		if (this._staticAddress == null) {
			logger.debug("OwnAddressType is changed to Public");
			this._ownAddressType = HCI.LEConst.OwnAddressType.PUBLIC;
			return Promise.resolve(null);
		}
		return this._updateRandomAddress(this._staticAddress);
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
		return SM.generatePrivateAddress(this._hci.encrypt, this._hci.random64, this._localIRK).then(rpa => {
			logger.debug("RPA is generated:" + rpa.toString());
			this._privateAddress = rpa;
			return _updateRandomAddress(this._privateAddress);
		});
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 10.7 Privacy Feature
	 */
	enablePrivacyFeature(enabled) {
		if (enabled) {
			if (this._localIRK == null) {
				return this._keyMgmt.generateIRK().then(irk => {
					this._localIRK = irk;
					return this._refreshLocalRPA();
				});
			} else {
				return this._refreshLocalRPA();
			}
		} else {
			return this._restoreIdentityAddress();
		}
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 10.8.2.3 Resolvable Private Address Resolution Procedure
	 */
	resolvePrivateAddress(address) {
		if (!address.isResolvable()) {
			logger.error("Not a resolvable address");
			return Promise.reject(null);
		}
		let bonds = this._storage.getBonds();
		let _loop = ctx => {
			for (let i = ctx.index; i < bonds.length; i++) {
				let bond = bonds[i];
				if (bond.hasOwnProperty("keys") && bond.keys.identityResolvingKey != null) {
					let irk = bond.keys.identityResolvingKey;
					return SM.resolvePrivateAddress(this._hci.encrypt, irk, address).then(resolved => {
						if (resolved) {
							return bond;
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
	 * 9.1.2 Observation Procedure
	 * 9.2.5 Limited Discovery Procedure
	 * 9.2.6 General Discovery Procedure
	 * 9.3.6 General Connection Establishment Procedure
	 */
	startScanning(parameters) {
		if (this._currentScanParameters != null || this._connecting) {
			/* Device is busy, ignore the call. */
			return Promise.reject(null);
		}

		let sparams = {};

		if (parameters.discovery) {
			/* Will perform General or Limited discovery procedure.
			 * Disocery procedures always use active scan, and will check AD Flags.
			 */
			sparams.scanType = HCI.LEConst.ScanType.ACTIVE;
			this._discoverAll = false;
			/* Limited discovery by default */
			if (parameters.hasOwnProperty("limited") && parameters.limited) {
				/* Will only discover if limited-flag is enabled. */
				this._discoverLimited = true;
			} else {
				this._discoverLimited = false;
			}
		} else {
			/* Else host shall perform Observation or General connection establishment procedure.
			 * User will choose scan method either passive or active. General connection establishment procedure
			 * should use passive scan, since scan responses are not required.
			 * AD Flags check will be skipped.
			 */
			this._discoverAll = true;
			sparams.scanType = parameters.active ? HCI.LEConst.ScanType.ACTIVE : HCI.LEConst.ScanType.PASSIVE;
		}

		/* Scan interval & window can be configured arbitrary by user. */
		if (parameters.hasOwnProperty("interval") && parameters.hasOwnProperty("window")) {
			sparams.interval = parameters.interval;
			sparams.window = parameters.window;
		} else {
			sparams.interval = 0x03C0;		// (set to 600ms per Wi-Fi team, AJC)
			sparams.window = 0x0040;		// (set to 40ms per Wi-Fi team, AJC)
		}

		/* Privacy-enabled device will use private address. */
		sparams.addressType = this._ownAddressType;

		/* Accept all advertising packets */
		sparams.filterPolicy = HCI.LEConst.ScanningFilterPolicy.ALL;

		/* When perform passive scan, we will also accept directed advertising packets
		 * where the initiator address is a RPA. Becase the address resolution is disabled
		 * at controller-level so we let user to choose whether to resolve the RPA and connect or not.
		 */
		if (sparams.scanType == HCI.LEConst.ScanType.PASSIVE) {
			sparams.filterPolicy |= HCI.LEConst.ScanningFilterPolicy.RESOLVABLE_DIRECTED;
		}

		/* Filter duplicate advertising packets by default */
		let filter = parameters.hasOwnProperty("duplicatesFilter") ? parameters.duplicatesFilter : true;

		return this._hci.commands.le.setScanParameters(sparams).then(() => {
			return this._hci.commands.le.setScanEnable(true, filter);
		}).then(() => {
			/* Keep current scan parameters, indicates scanning is enabled. */
			this._currentScanParameters = sparams;
		});
	}
	stopScanning() {
		if (this._currentScanParameters == null) {
			return Promise.resolve(null);
		}
		return this._hci.commands.le.setScanEnable(false, true).then(() => {
			/* Clear current scan parameters, indicates scanning is disabled. */
			this._currentScanParameters = null;
		});
	}
	_processDiscovered(reports) {
		for (let report of reports) {
			/* Connection procedure could be happened after device is dicovered,
			 * then we should terminate further discovery procedure.
			 */
			if (this._connecting) {
				return;
			}

			/* Flag for General & Limited discovery procedure */
			let discovered = false;

			/* Build an report object according to the event type.
			 *
			 * ADV_DIRECT_IND will includes 'directAddress' property.
			 * ADV_IND, ADV_SCAN_IND, and ADV_NONCONN_IND will includes 'advertising' property.
			 * SCAN_RSP does not includes 'directed', 'connectable' and 'scannable' property,
			 * but includes 'scanResponse' property.
			 * All event types will includes 'address' and 'rssi' proprty.
			 */
			let device = {
				address: report.address,	// Remote Address
				rssi: report.rssi			// RSSI
			};
			if (report.eventType == HCI.LEConst.EventType.ADV_DIRECT_IND) {
				device.directed = true;
				device.directAddress = report.hasOwnProperty("directAddress") ? report.directAddress : null;
				/* Directed advertising is connectable and not scannable. (Vol 5, Part B, 4.4.2) */
				device.connectable = true;
				device.scannable = false;
			} else {
				let structures = readAdvertisingData(ByteBuffer.wrap(report.data, 0, report.length, true));
				if (report.eventType == HCI.LEConst.EventType.SCAN_RSP) {
					device.scanResponse = structures;
					/* Scan response will always be considered as discovered */
					discovered = true;
				} else {
					/* From Vol 5, Part B, 4.4.2:
					 * ADV_IND (and ADV_DIRECT_IND) is connectable
					 * ADV_IND and ADV_SCAN_IND is scannable, i.e. ADV_NONCONN_IND is not scannable.
					 */
					device.directed = false;
					device.connectable = (report.eventType == HCI.LEConst.EventType.ADV_IND);
					device.scannable = (report.eventType != HCI.LEConst.EventType.ADV_NONCONN_IND);
					/* Keep Adv data */
					device.advertising = structures;
					/* Check AD Flags for discovery */
					discovered = checkDiscoveryFlag(device.advertising, this._discoverLimited);
				}
			}

			/* Discover All option will bypass the filter */
			if (discovered || this._discoverAll) {
				this._application.gapDiscovered(device);
			}
		}
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3.5 Auto Connection Establishment Procedure
	 * 9.3.8 Direct Connection Establishment Procedure
	 */
	establishConnection(address = null, connParameter = null) {
		if (this._connecting) {
			/* Device is busy */
			return Promise.reject(null);
		}

		if (connParameter == null) {
			/* Default parameters as described in 9.3.12.2 */
			connParameter = {
				intervalMin: MIN_INITIAL_CONN_INTERVAL,
				intervalMax: MAX_INITIAL_CONN_INTERVAL,
				latency: 0,
				supervisionTimeout: 3200,	// FIXME
				minimumCELength: 0,			// Most implementation uses zero
				maximumCELength: 0			// Most implementation uses zero
			};
		}

		/* There is some limitation for Auto connection establishment procedure.
		 * Since the address resolution is disabled at controller-level, any connectable
		 * advertising packets using RPA cannot be compared against white-list.
		 */
		let useWhiteList = (address != null) ? false : true;

		/* Stop scanning before we attempt to create LE connection */
		let stop;
		if (this._currentScanParameters != null) {
			stop = this.stopScanning();
		} else {
			stop = Promise.resolve(null);
		}

		/* TODO: There is no timeout for LE connection */

		/* Connection is in progress. */
		this._connecting = true;

		return stop.then(() => {
			return this._hci.commands.le.createConnection(
				/* Scan Parameter as described in 9.3.11.2 */
				{
					interval: SCAN_FAST_INTERVAL,
					window: SCAN_FAST_WINDOW
				},
				useWhiteList,
				!useWhiteList ? (address.isRandom() ? 0x01 : 0x00) : 0,
				address,
				this._ownAddressType,
				connParameter
			);
		}).catch(() => {
			/* Connection procedure is suspended. */
			this._connecting = false;
		});
	}
	setWhiteList(addresses) {
		this._hci.commands.le.clearWhiteList();
		for (let address of addresses) {
			this._hci.commands.le.addDeviceToWhiteList(address);
		}
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.1.1 Broadcast Mode
	 * 9.2.2 Non-Discoverable Mode
	 * 9.2.3 Limited Discoverable Mode
	 * 9.2.4 General Discoverable Mode
	 * 9.3.2 Non-Connectable Mode
	 * 9.3.3 Directed Connectable Mode
	 * 9.3.4 Undirected Connectable Mode
	 */
	startAdvertising(parameters) {
		if (this._currentAdvertisingParameters != null) {
			/* Device is busy */
			return Promise.reject(null);
		}

		let aparams = {};

		/* Use fast advertisement interval by default */
		let fast = parameters.hasOwnProperty("fast") ? parameters.fast : true;

		aparams.address = parameters.hasOwnProperty("address") ? parameters.address : null;

		let updateADStructures;
		if (parameters.connectable && aparams.address != null) {
			/* Directed Connectable Mode */
			aparams.address = parameters.address;
			/* Fast interval option will use High-duty-cycle advertising,
			 * which intervals settings will be ignored.
			 */
			aparams.advType = fast ?
					HCI.LEConst.AdvertisingType.ADV_DIRECT_IND_HDC :
					HCI.LEConst.AdvertisingType.ADV_DIRECT_IND_LDC;
			aparams.intervals = ADV_FAST_INTERVAL1;

			/* ADV_DIRECT_IND does not contains any AD structures, thus we will skip
			 * updating advertising data and scan response data.
			 */
			updateADStructures = Promise.resolve(null);
		} else {
			/* Undirected advertising packet */
			aparams.address = null;

			/* Build advertising AD Data structure */
			let advertising = parameters.hasOwnProperty("advertising") ? parameters.advertising : new Array();
			let flags = Flags.NO_BR_EDR;
			if (parameters.discoverable) {
				/* General discoverable mode by default */
				if (parameters.hasOwnProperty("limited") && parameters.limited) {
					/* Limited Discoverable Mode */
					flags |= Flags.LE_LIMITED_DISCOVERABLE_MODE;
				} else {
					/* General Discoverable Mode */
					flags |= Flags.LE_GENERAL_DISCOVERABLE_MODE;
				}
			} else {
				/* Non-Discoverable Mode */
			}
			advertising.push({
				type: ADType.FLAGS,
				data: [flags]
			});

			/* Scan response is optional */
			let scanResponse = parameters.hasOwnProperty("scanResponse") ? parameters.scanResponse : null;

			updateADStructures = this._hci.commands.le.setAdvertisingData(toAdvertisingDataArray(advertising)).then(() => {
				if (scanResponse != null) {
					return this._hci.commands.le.setScanResponseData(toAdvertisingDataArray(scanResponse));
				}
			});

			if (parameters.connectable) {
				/* Undirected Connectable Mode */
				aparams.intervals = fast ?
					(parameters.discoverable ? ADV_FAST_INTERVAL1 : ADV_FAST_INTERVAL2) :
					ADV_SLOW_INTERVAL;
				aparams.advType = HCI.LEConst.AdvertisingType.ADV_IND;
			} else {
				/* Non-Connectable Mode */
				aparams.intervals = fast ? ADV_FAST_INTERVAL2 : ADV_SLOW_INTERVAL;
				if (scanResponse != null) {
					/* Scannable undirected advertising */
					aparams.advType = HCI.LEConst.AdvertisingType.ADV_SCAN_IND;
				} else {
					aparams.advType = HCI.LEConst.AdvertisingType.ADV_NONCONN_IND;
				}
			}
		}

		/* XXX: Process scan & connection requests from all devices. */
		aparams.filter = HCI.LEConst.AdvertisingFilterPolicy.ALL;
//		if (!parameters.connectable && !parameters.discoverable) {
//			// TODO: Filter may be ALL_WHITE_LIST
//		}

		return updateADStructures.then(() => {
			return this._hci.commands.le.setAdvertisingParameters(
				aparams.intervals,
				aparams.advType,
				this._ownAddressType,
				aparams.address,
				0x7,											// XXX: All channel
				aparams.filter
			);
		}).then(() => {
			return this._hci.commands.le.setAdvertiseEnable(true);
		}).then(() => {
			/* Keep current advertising parameters, indicates advertising is enabled. */
			this._currentAdvertisingParameters = aparams;
		});
	}
	stopAdvertising() {
		if (this._currentAdvertisingParameters == null) {
			return Promise.resolve(null);
		}

		return this._hci.commands.le.setAdvertiseEnable(false).then(() => {
			/* Clear current advertising parameters, indicates advertising is disabled. */
			this._currentAdvertisingParameters = null;
		});
	}
}

function toAdvertisingDataArray(structures) {
	var buffer = ByteBuffer.allocateUint8Array(MAX_AD_LENGTH, true);
	writeAdvertisingData(buffer, structures);
	buffer.flip();
	return buffer.getByteArray();
}

class GAPConnection {
	constructor(ctx, connectionManager, signalingCtx) {
		this._ctx = ctx;
		this._connectionManager = connectionManager;
		this._connectionManager.delegate = this;
		this._signalingCtx = signalingCtx;
		this._securityInfo = null;
		this._delegate = null;
		// TODO: Open SDP Channel
		/* Open ATT Channel */
		this._attConnection = connectionManager.openConnection(
			L2CAP.LEChannel.ATTRIBUTE_PROTOCOL,
			L2CAP.LEChannel.ATTRIBUTE_PROTOCOL);
		/* Open SMP Channel */
		this._smpConnection = connectionManager.openConnection(
			L2CAP.LEChannel.SECURITY_MANAGER_PROTOCOL,
			L2CAP.LEChannel.SECURITY_MANAGER_PROTOCOL);

		this._security = new SM.SecurityManagement(
			this._ctx.hci, this._smpConnection,
			this._ctx.keyManagement,
			{
				bonding: false,
				outOfBand: false,
				mitm: false,
				minKeySize: 7,
				maxKeySize: 16,
				display: false,
				keyboard: false,
				identityAddress: null	// Use public address as identity
			}
		);
		this._security.delegate = this;

		this._signCounter = 0;	// XXX: Need to be stored in extarnal database
		this._bearer = null;
	}
	get handle() {
		return this._connectionManager.link.handle;
	}
	get peripheral() {
		return this._connectionManager.link.isLESlave();
	}
	get parameters() {
		return this._connectionManager.link.connParameters;
	}
	get address() {
		return this._connectionManager.link.remoteAddress;
	}
	get identity() {
		if (this._securityInfo != null && this._securityInfo.address != null) {
			return this._securityInfo.address;
		}
		let address = this.address;
		if (address.isIdentity()) {
			return address;
		}
		return null;
	}
	get encrypted() {
		return this._connectionManager.link.encrptionStatus == 0x01;
	}
	get securityInfo() {
		return this._securityInfo;
	}
	set securityInfo(info) {
		this._securityInfo = info;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	get bearer() {
		return this._bearer;
	}
	set bearer(bearer) {
		this._bearer = bearer;
	}
	initChannels() {
		/* Initialize channel contexts */
		let attDelegate =  {
			disconnected: () => logger.debug("ATT Disconnected"),
			received: () => {
				let buffer;
				while ((buffer = this._attConnection.dequeueFrame()) != null) {
					this._attReceived(buffer);
				}
			}
		};
		this._attConnection.delegate = attDelegate;
		this._smpConnection.delegate = this._security;

		/* Will dequeue all L2CAP frames that were arrived before initialization. */
		attDelegate.received();
		this._security.received();
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3.9 Connection Parameter Update Procedure
	 */
	updateConnectionParameter(connParameter, l2cap = false) {
		if (l2cap) {
			this._signalingCtx.sendConnectionParameterUpdateRequest(connParameter, response => {
				let success = (response.getInt16() == 0x0000);
				if (!success) {
					logger.warn("L2CAP CPU Rejected");
				}
			});
		} else {
			connParameter.minimumCELength = 0;
			connParameter.maximumCELength = 0;
			this._ctx.hci.commands.le.connectionUpdate(this.handle, connParameter);
		}
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part C: Generic Access Profile
	 * 9.3.10 Terminate Connection Procedure
	 */
	disconnect(reason = 0x13) {
		let handle = this.handle;
		logger.debug("Disconnect link: handle=" + Utils.toHexString(handle, 2));
		this._ctx.hci.commands.linkControl.disconnect(handle, reason);
	}
	startAuthentication() {
		let link = this._connectionManager.link;
		if (link.isLESlave()) {
			this._security.sendSecurityRequest();
		} else {
			if (this._securityInfo != null) {
				// TODO: Check security level
				logger.info("Has LTK, start encryption");
				this._security.startEncryption(this._securityInfo);
			} else {
				logger.warn("No LTK, start pairing");
				this._security.startPairing();
			}
		}
	}
	setSecurityParameter(parameter) {
		if ("bonding" in parameter) {
			this._security.parameter.bonding = parameter.bonding;
		}
		if ("mitm" in parameter) {
			this._security.parameter.mitm = parameter.mitm;
		}
		if ("display" in parameter) {
			this._security.parameter.display = parameter.display;
		}
		if ("keyboard" in parameter) {
			this._security.parameter.keyboard = parameter.keyboard;
		}
	}
	passkeyEntry(passkey) {
		this._security.passkeyEntry(passkey);
	}
	isDisconnected() {
		return this._connectionManager.link == null;
	}
	sendAttributePDU(pdu) {
		let signed = (pdu[0] & 0x80) > 0;
		if (signed) {
			logger.debug("Signature flag is enabled in PDU");
			if (this._securityInfo == null || this._securityInfo.signatureKey == null) {
				logger.error("TODO: CSRK is not available (Calculation)");
				return;
			}
			let csrk = this._securityInfo.signatureKey;
			let message = new Uint8Array(pdu.length + 12);
			message.set(pdu);
			message.set(Utils.toByteArray(this._signCounter, Utils.INT_32_SIZE, true), pdu.length);
			AESCMAC.cmac(this._ctx.hci.encrypt, csrk, message, pdu.length + Utils.INT_32_SIZE).then(mac => {
				mac = mac.slice(Util.INT_64_SIZE);
				logger.debug("CMAC is calculated: mac=" + Utils.toFrameString(mac)
					+ ", signCounter=" + this._signCounter);
				this._signCounter++;
				message.set(mac, pdu.length + Utils.INT_32_SIZE);
				this._attConnection.sendBasicFrame(message);
			});
		} else {
			this._attConnection.sendBasicFrame(pdu);
		}
	}
	_attVerified(buffer) {
		if (this._bearer == null) {
			logger.warn("ATT Received but no bearer");
		} else {
			this._bearer.received(buffer);
		}
	}
	_attReceived(buffer) {
		let opcode = buffer.peek();
		logger.debug("ATT Received: opcode=" + Utils.toHexString(opcode));
		if ((opcode & 0x80) > 0) {
			logger.debug("Signature flag is enabled in PDU (RX)");
			if (this._securityInfo == null || this._securityInfo.signatureKey == null) {
				logger.error("TODO: CSRK is not available (RX)");
				return;
			}
			buffer.mark();
			let csrk = this._securityInfo.signatureKey;
			let message = buffer.getByteArray(buffer.remaining() - Util.INT_64_SIZE);
			let signature = buffer.getByteArray();
			buffer.reset();
			AESCMAC.cmac(this._ctx.hci.encrypt, csrk, message).then(mac => {
				mac = mac.slice(Util.INT_64_SIZE);
				logger.debug("CMAC is calculated (RX): " + Utils.toFrameString(mac));
				opcode &= ~0x80;
				if (BTUtils.isArrayEquals(signature, mac)) {
					logger.debug("Signature verified");
					// TODO: Store counter
					this._attVerified(buffer);
				} else {
					logger.error("Signature verification has failed");
				}
			});
		} else {
			this._attVerified(buffer);
		}
	}
	/* L2CAP Callback (ConnectionManager) */
	disconnected(reason) {
		this._delegate.disconnected(reason);
	}
	/* L2CAP Callback (ConnectionManager) */
	connectionUpdated(parameters) {
		this._delegate.connectionUpdated(parameters);
	}
	/* SM Callback (SecurityManagement) */
	pairingFailed(reason) {
		logger.error("Pairing Failed: reason=" + Utils.toHexString(reason));
		this._delegate.pairingFailed(reason);
	}
	/* SM Callback (SecurityManagement) */
	passkeyRequested(input) {
		this._delegate.passkeyRequested(input);
	}
	/* SM Callback (SecurityManagement) */
	encryptionCompleted(securityInfo) {
		/* If the pairing has been performed over SMP, securityInfo will not be null.
		 * e.g. in case there is bonding, securityInfo will be null.
		 */
		let securityInfoChanged = (securityInfo != null);
		if (securityInfoChanged) {
			/* Update pairing information */
			let identityAddress = null;
			let link = this._connectionManager.link;
			if (link.remoteAddress.isIdentity()) {
				identityAddress = link.remoteAddress;
			} else if (securityInfo.address != null) {
				identityAddress = securityInfo.address;
				logger.debug("Remote identity available: " + identityAddress.toString());
				if (securityInfo.identityResolvingKey == null) {
					logger.warn("Identity received but IRK is not available");
				} else {
					logger.debug("IRK: " + Utils.toFrameString(securityInfo.identityResolvingKey));
				}
			} else {
				logger.warn("Cannot bond with non identity address");
			}
			if (securityInfo.bonding && identityAddress != null) {
				this._ctx.storage.storeBond(identityAddress, securityInfo);
				logger.debug("Bonding Information stored: address=" + identityAddress.toString());
			}
			this._securityInfo = securityInfo;
		} else {
			logger.info("Encryption completed without pairing");
		}
		this._delegate.encryptionCompleted(securityInfoChanged);
	}
	/* SM Callback (SecurityManagement) */
	encryptionFailed(status) {
		logger.error("Encryption Failed: status=" + Utils.toHexString(status));
		this._delegate.encryptionFailed(status);
	}
	/* SM Callback (SecurityManagement) */
	securityRequested(authReq) {
		logger.debug("Got security request from slave");
		// TODO
		this.startAuthentication();
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

function checkDiscoveryFlag(structures, limited) {
	let mask = Flags.LE_LIMITED_DISCOVERABLE_MODE;
	if (!limited) {
		mask |= Flags.LE_GENERAL_DISCOVERABLE_MODE;
	}

	for (let structure of structures) {
		if (structure.type != ADType.FLAGS) {
			continue;
		}
		if ((structure.data[0] & mask) > 0) {
			return true;
		}
	}
	return false;
}