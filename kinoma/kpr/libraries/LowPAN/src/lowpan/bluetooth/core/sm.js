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
 * Bluetooth v4.2 - Security Manager (LE Legacy Only)
 */

var Utils = require("../../common/utils");
var Logger = Utils.Logger;
var Buffers = require("../../common/buffers");
var ByteBuffer = Buffers.ByteBuffer;

var BTUtils = require("./btutils");
var BluetoothAddress = BTUtils.BluetoothAddress;

var logger = new Logger("SM");
logger.loggingLevel = Utils.Logger.Level.INFO;

exports.setLoggingLevel = level => logger.loggingLevel = level;

/** Lower HCI layer instance/module */
var _hci = null;

exports.registerHCI = function (hci) {
	_hci = hci;
};

const Code = {
	PAIRING_REQUEST: 0x01,
	PAIRING_RESPONSE: 0x02,
	PAIRING_CONFIRM: 0x03,
	PAIRING_RANDOM: 0x04,
	PAIRING_FAILED: 0x05,
	ENCRYPTION_INFORMATION: 0x06,
	MASTER_IDENTIFICATION: 0x07,
	IDENTITY_INFORMATION: 0x08,
	IDENTITY_ADDRESS_INFORMATION: 0x09,
	SIGNING_INFORMATION: 0x0A,
	SECURITY_REQUEST: 0x0B,
	PAIRING_PUBLIC_KEY: 0x0C,
	PAIRING_DHKEY_CHECK: 0x0D,
	PAIRING_KEYPRESS_NOTIFICATION: 0x0E
};

const KeyGenerationMethod = {
	OOB: Symbol("OutOfBand"),
	JUST_WORKS: Symbol("JustWorks"),
	PASSKEY_INITIATOR: Symbol("Passkey Entry Initiator Inputs"),
	PASSKEY_RESPONDER: Symbol("Passkey Entry Responder Inputs"),
	PASSKEY_BOTH: Symbol("Passkey Entry Both Inputs"),
	NUMERIC_COMPARISON: Symbol("Numeric Comparison")
};

const IOCapability = {
	DISPLAY_ONLY: 0x00,
	DISPLAY_YES_NO: 0x01,
	KEYBOARD_ONLY: 0x02,
	NO_IO: 0x03,
	KEYBOARD_DISPLAY: 0x04
};

const FailedReason = {
	PASSKEY_ENTRY_FAILED: 0x01,
	OOB_NOT_AVAILABLE: 0x02,
	AUTH_REQUIREMENTS: 0x03,
	CONFIRM_VALUE_FAILED: 0x04,
	PAIRING_NOT_SUPPORTED: 0x05,
	ENCRYPTION_KEY_SIZE: 0x06,
	COMMAND_NOT_SUPPORTED: 0x07,
	UNSPECIFIED_REASON: 0x08,
	REPEATED_ATTEMPTS: 0x09,
	INVALID_PARAMETERS: 0x0A,
	DHKEY_CHECK_FAILED: 0x0B,
	NUMERIC_COMPARISON_FAILED: 0x0C,
	BD_EDR_PIP: 0x0D,
	CK_DERIVATION_NOT_ALLOWED: 0x0E
};

const AuthReqFlags = {
	BONDING: 0x01,
	MITM: 0x04,
	SC: 0x08,
	KEYPRESS: 0x10
};

const KeyDistributionFormat = {
	ENC_KEY: 0x01,
	ID_KEY: 0x02,
	SIGN: 0x04,
	LINK_KEY: 0x08
};

const PASSKEY_MINIMUM = 0;
const PASSKEY_MAXIMUM = 999999;
const FORCE_LTK = true;		// FIXME: Temporary workaround for testing

class SecurityManagement {
	constructor(connection, keyMgmt, parameter) {
		this._connection = connection;
		this._parameter = parameter;
		this._keyMgmt = keyMgmt;
		this._delegate = null;
		connection.delegate = this;
		connection.getHCILink().security = this;
		this.reset();
	}
	get connection() {
		return this._connection;
	}
	get pairingInfo() {
		return this._pairingInfo;
	}
	get parameter() {
		return this._parameter;
	}
	set delegate(delegate) {
		this._delegate = delegate;
	}
	/* Public API */
	startEncryption() {
		let link = this._connection.getHCILink();
		if (link.isLESlave()) {
			/* Slave Security Request */
			this.sendCommand(assembleValue8(Code.SECURITY_REQUEST, this.generateAuthReq()));
		} else {
			let bond = this._delegate.findBondByAddress(link.remoteAddress);
			if (bond == null || bond.keys.longTermKey == null) {
				logger.debug("No LTK or Security level issue");
				this.startPairing(true);
				return;
			}
			if (link.encrptionStatus != 0) {	// FIXME
				logger.debug("Already encrypted, will refresh");
			}
			logger.debug("Start encryption using current LTK");
			link.startEncryption(bond.keys.random, bond.keys.ediv, bond.keys.longTermKey);
		}
	}
	/* Public API */
	passkeyEntry(passkey) {
		let p = parseInt(passkey);
		if (this._state != null &&
			(PASSKEY_MINIMUM <= p && p <= PASSKEY_MAXIMUM)) {
			let tk = new Uint8Array(16).fill(0);
			tk.set(Utils.toByteArray(p, Utils.INT_32_SIZE, true));
			this._state.setTemporaryKey(tk);
		} else {
			this.pairingFailed(FailedReason.PASSKEY_ENTRY_FAILED);
		}
	}
	generateAuthReq() {
		let authReq = 0;
		/* We do not support SC & Keypress at this time */
		if (this._parameter.bonding) {
			authReq |= AuthReqFlags.BONDING;
		}
		if (this._parameter.mitm) {
			authReq |= AuthReqFlags.MITM;
		}
		return authReq;
	}
	reset() {
		logger.debug("Reset");
		this._state = null;
		this._pairingInfo = null;
		this._shortTermKey = null;
		this._shortTermKeyRequested = false;
		this._div = -1;
	}
	sendCommand(packet) {
		logger.trace("Send SMP Command: code=" + Utils.toHexString(packet[0]) +
			" " + Utils.toFrameString(packet, 1, packet.length - 1));
		this._connection.sendBasicFrame(packet);
	}
	pairingFailed(reason) {
		logger.debug("Pairing Failed: reason=" + Utils.toHexString(reason));
		this.sendCommand(assembleValue8(Code.PAIRING_FAILED, reason));
		this.reset();
		this._delegate.pairingFailed(reason);
	}
	startPairing(sendRequest) {
		logger.debug("Start Phase 1");
		this._state = new PairingFeatureExchangeState(this);
		if (sendRequest) {
			this._state.sendPairingRequest();
		}
	}
	/* Phase 1 Callback (PairingFeatureExchangeState) */
	featureExchanged(info) {
		logger.debug("End Phase 1");
		// TODO: Check auth requirements
		if (info.legacy) {
			this._pairingInfo = info;
			logger.debug("Start Phase 2 Legacy");
			this._state = new LELegacyPairingState(this);
			if (info.method == KeyGenerationMethod.JUST_WORKS) {
				logger.info("Legacy: Just works, Set TK=0");
				this._state.setTemporaryKey(new Uint8Array(16).fill(0));
			} else {
				if (info.method == KeyGenerationMethod.PASSKEY_BOTH) {
					logger.info("Legacy: Passkey Both");
					this._delegate.passkeyRequested(true);
				} else if (info.method == KeyGenerationMethod.PASSKEY_INITIATOR) {
					logger.info("Legacy: Passkey Initiator");
					this._delegate.passkeyRequested(info.initiator);
				} else if (info.method == KeyGenerationMethod.PASSKEY_RESPONDER) {
					logger.info("Legacy: Passkey Responder");
					this._delegate.passkeyRequested(!info.initiator);
				} else {
					logger.warn("Legacy: Unexpected Method");
				}
			}
			return true;
		} else {
			logger.warn("LE Secure Connection is not supported yet.");
			return false;
		}
	}
	/* Phase 2 Legacy Callback (LELegacyPairingState) */
	shortTermKeyGenerated(stk) {
		logger.debug("End Legacy Phase 2: stk=" + Utils.toFrameString(stk));
		this._state = null;
		this._shortTermKey = stk;
		let link = this._connection.getHCILink();
		if (this._pairingInfo.initiator) {
			logger.debug("Legacy: Start encryption using STK");
			link.startEncryption(new Uint8Array(8).fill(0), 0x0000, stk);
		} else if (this._shortTermKeyRequested) {
			logger.debug("Legacy: Reply encryption using STK");
			link.replyLongTermKey(this._shortTermKey);
		}
	}
	/* Phase 3 Callback (KeyDistributionState) */
	keyExchanged(db) {
		logger.debug("End Phase 3");
		this._pairingInfo.keys = db;
		this._delegate.encryptionCompleted(this._div, this._pairingInfo);
		this.reset();
	}
	/* L2CAP Callback (Connection) */
	disconnected() {
		logger.warn("TODO: Disconnected");
	}
	/* L2CAP Callback (Connection) */
	received(buffer) {
		let code = buffer.getInt8();
		// TODO: 2.3.6 Repeated Attempts
		switch (code) {
		case Code.PAIRING_REQUEST:
			this.startPairing(false);
			break;	// PairingFeatureExchangeState will continue handle the request
		case Code.SECURITY_REQUEST:
			{
				logger.debug("Got Security Request");
				let authReq = buffer.getInt8();
				let link = this._connection.getHCILink();
				if (link.isLESlave() || this._state != null) {
					return;		// Ignore
				}
				this.startEncryption(authReq);
			}
			return;
		case Code.PAIRING_FAILED:
			{
				let status = buffer.getInt8();
				logger.debug("Got Pairing Failed " + Utils.toHexString(status));
				this.reset();
				this._delegate.pairingFailed(status);
			}
			return;
		}
		if (this._state != null) {
			if (!this._state.smpReceived(code, buffer)) {
				logger.debug("Code " + Utils.toHexString(code) + " is not handled.");
				this.pairingFailed(FailedReason.COMMAND_NOT_SUPPORTED);
			}
		}
	}
	/* HCI Callback (ACLLink) */
	encryptionFailed(status) {
		logger.debug("Encryption failed: " + Utils.toHexString(status));
		this.reset();
		this._delegate.encryptionFailed(status);
	}
	/* HCI Callback (ACLLink) */
	encryptionStatusChanged(link, enabled) {
		logger.debug("Encryption enabled: " + Utils.toHexString(enabled));
		if (enabled != 0x01) {
			logger.debug("Encryption is not ON");
			return;
		}
		if (this._shortTermKey != null) {
			this._shortTermKey = null;
			logger.debug("Start Phase 3");
			this._div = this._delegate.generateDIV();
			this._state = new KeyDistributionState(this, this._keyMgmt, this._div);
			/* Slave always distributes the keys first */
			if (link.isLESlave() || this.pairingInfo.responderKeys == 0) {
				this._state.distributeKeys();
			}
		} else {
			this._delegate.encryptionCompleted(this._div, this._pairingInfo);
			this.reset();
		}
	}
	/* HCI Callback (LELink) */
	longTermKeyRequested(link, random, ediv) {
		if (arrayIsZero(random) && ediv == 0) {
			logger.debug("STK is requested");
			if (this._shortTermKey == null) {
				logger.debug("STK is not ready yet");
				this._shortTermKeyRequested = true;
				return;
			}
			link.replyLongTermKey(this._shortTermKey);
		} else {
			logger.debug("LTK is requested: random=" + Utils.toFrameString(random)
				+ ", ediv=" + Utils.toHexString(ediv, 2));
			this._keyMgmt.recoverDIV(random, ediv).then(div => {
				logger.debug("DIV Recovered: div=" + Utils.toHexString(div, 2));
				let bond = this._delegate.findBondByDIV(div);
				if (!FORCE_LTK && bond == null) {
					logger.error("Bond is deleted");
					link.replyLongTermKey(null);
				}
				return this._keyMgmt.generateLTK(div);
			}).then(ltk => {
				link.replyLongTermKey(ltk);
			}).catch(() => {
				link.replyLongTermKey(null);
			});
		}
	}
}
exports.SecurityManagement = SecurityManagement;

/******************************************************************************
 * Pairing Phase 1-3 States
 ******************************************************************************/

class State {
	constructor(msg) {
		this._msg = msg;
	}
	log(log) {
		logger.debug("[" + this._msg + "] " + log);
	}
}

function selectKeyGenerationMethod(initiator, responder, legacy) {
	if (initiator.outOfBand && responder.outOfBand) {
		return KeyGenerationMethod.OOB;
	}
	if (initiator.outOfBand || responder.outOfBand) {
		if (!legacy) {
			return KeyGenerationMethod.OOB;
		}/* else {
			// Pairing Failed with OOB Not Available
		}*/
	}
	if ((initiator.authReq & AuthReqFlags.MITM) == 0
		&& (responder.authReq & AuthReqFlags.MITM) == 0) {
		return KeyGenerationMethod.JUST_WORKS;
	}
	if ((initiator.ioCapability == IOCapability.NO_IO)
		|| (responder.ioCapability == IOCapability.NO_IO)) {
		return KeyGenerationMethod.JUST_WORKS;
	}
	switch (responder.ioCapability) {
	case IOCapability.DISPLAY_ONLY:
		if ((initiator.ioCapability == IOCapability.KEYBOARD_ONLY)
			|| (initiator.ioCapability == IOCapability.KEYBOARD_DISPLAY)) {
			return KeyGenerationMethod.PASSKEY_INITIATOR;
		}
		return KeyGenerationMethod.JUST_WORKS;
	case IOCapability.DISPLAY_YES_NO:
		switch (initiator.ioCapability) {
		case IOCapability.DISPLAY_ONLY:
			return KeyGenerationMethod.JUST_WORKS;
		case IOCapability.DISPLAY_YES_NO:
			return legacy ? KeyGenerationMethod.JUST_WORKS
				: KeyGenerationMethod.NUMERIC_COMPARISON;
		case IOCapability.KEYBOARD_ONLY:
			return KeyGenerationMethod.PASSKEY_INITIATOR;
		case IOCapability.KEYBOARD_DISPLAY:
			return legacy ? KeyGenerationMethod.PASSKEY_INITIATOR
				: KeyGenerationMethod.NUMERIC_COMPARISON;
		}
		break;
	case IOCapability.KEYBOARD_ONLY:
		if (initiator.ioCapability == IOCapability.KEYBOARD_ONLY) {
			return KeyGenerationMethod.PASSKEY_BOTH;
		}
		return KeyGenerationMethod.PASSKEY_RESPONDER;
	case IOCapability.KEYBOARD_DISPLAY:
		switch (initiator.ioCapability) {
		case IOCapability.DISPLAY_ONLY:
			return KeyGenerationMethod.PASSKEY_RESPONDER;
		case IOCapability.KEYBOARD_ONLY:
			return KeyGenerationMethod.PASSKEY_INITIATOR;
		default:
			return legacy ? KeyGenerationMethod.PASSKEY_RESPONDER
				: KeyGenerationMethod.NUMERIC_COMPARISON;
		}
	}
	logger.error("Key generation method is not selected");
	return null;	// Should not reach here
}

class PairingFeatureExchangeState extends State {
	constructor(smCtx, request) {
		super("Phase 1");
		this._smCtx = smCtx;
		this._request = null;
	}
	generateFeature() {
		let parameter = this._smCtx.parameter;
		let ioCapability;
		if (parameter.display) {
			if (parameter.keyboard) {
				ioCapability = IOCapability.KEYBOARD_DISPLAY;
			} else {
				/* TODO: Yes/No */
				ioCapability = IOCapability.DISPLAY_ONLY;
			}
		} else {
			if (parameter.keyboard) {
				ioCapability = IOCapability.KEYBOARD_ONLY;
			} else {
				ioCapability = IOCapability.NO_IO;
			}
		}
		return {
			ioCapability: ioCapability,
			outOfBand: parameter.outOfBand,
			authReq: this._smCtx.generateAuthReq(),
			maxKeySize: parameter.maxKeySize,
			/* XXX: Will not exchange any keys when not bonding, otherwise request all */
			initiatorKeys: parameter.bonding ? 0x07 : 0x00,
			responderKeys: parameter.bonding ? 0x07 : 0x00
		};
	}
	sendPairingRequest() {
		this._request = assemblePairingFeature(true, this.generateFeature());
		this._smCtx.sendCommand(this._request);
	}
	smpReceived(code, buffer) {
		switch (code) {
		case Code.PAIRING_REQUEST:
		case Code.PAIRING_RESPONSE:
			{
				/* Needs a full PDU for later calculation */
				let packet = new Uint8Array(7);
				packet[0] = code;
				packet.set(buffer.getByteArray(6), 1);
				let remoteFeature = parsePairingFeature(packet);
				let localFeature = this.generateFeature();
				if (remoteFeature.maxKeySize < this._smCtx.parameter.minKeySize) {
					/* Key size is smaller than minimum */
					this._smCtx.pairingFailed(FailedReason.ENCRYPTION_KEY_SIZE);
					return;
				}
				let info = {
					initiator: (code == Code.PAIRING_RESPONSE),
					keySize: Math.min(localFeature.maxKeySize, remoteFeature.maxKeySize),
					legacy: true,
					bonding: false
				};
				this.log("Key Size: " + info.keySize);
				if ((remoteFeature.authReq & AuthReqFlags.BONDING) > 0
					&& (localFeature.authReq & AuthReqFlags.BONDING) > 0) {
					this.log("Both device require bonding");
					info.bonding = true;
				}
				if ((remoteFeature.authReq & AuthReqFlags.SC) > 0
					&& (localFeature.authReq & AuthReqFlags.SC) > 0) {
					this.log("Both device require secure connection");
					info.legacy = false;
				}
				if ((remoteFeature.authReq & AuthReqFlags.KEYPRESS) > 0) {
					/* Keypress is not supported at this time */
					this.log("TODO: Remote device requested Keypress but will ignore");
				}
				/* XXX: We may need to check distribution format */
				/* XXX: We simply and keys that both requested */
				info.initiatorKeys = localFeature.initiatorKeys & remoteFeature.initiatorKeys;
				info.responderKeys = localFeature.responderKeys & remoteFeature.responderKeys;
				this.log("Initiator Key Distribution Format: " + Utils.toHexString(info.initiatorKeys));
				this.log("Responder Key Distribution Format: " + Utils.toHexString(info.responderKeys));
				/* Use updated distribution format */
				localFeature.initiatorKeys = info.initiatorKeys;
				localFeature.responderKeys = info.responderKeys;
				if (info.initiator) {
					this.log("Got Pairing Response");
					info.request = this._request;
					info.response = packet;
					info.method = selectKeyGenerationMethod(localFeature, remoteFeature, info.legacy);
				} else {
					this.log("Got Pairing Request");
					info.request = packet;
					info.response = assemblePairingFeature(false, localFeature);
					info.method = selectKeyGenerationMethod(remoteFeature, localFeature, info.legacy);
				}
				this.log("Method selected: " + info.method.toString());
				if (this._smCtx.featureExchanged(info)) {
					if (!info.initiator) {
						this.log("Reply with Pairing Response");
						this._smCtx.sendCommand(info.response);
					}
				} else {
					this._smCtx.pairingFailed(FailedReason.AUTH_REQUIREMENTS);
				}
			}
			return true;
		}
		return false;
	}
}

class LELegacyPairingState extends State {
	constructor(smCtx) {
		super("Legacy Phase 2");
		this._smCtx = smCtx;
		this._temporaryKey = undefined;
		this._random = null;
		this._confirm = null;
	}
	generateRandom() {
		if (this._random != null) {
			return Promise.resolve(this._random);
		}
		return random128().then(random => {
			this.log("Set Random128: " + Utils.toFrameString(random));
			this._random = random;
			return random;
		});
	}
	generateConfirmValue(random) {
		let iaddr;
		let raddr;
		let link = this._smCtx.connection.getHCILink();
		if (this._smCtx.pairingInfo.initiator) {
			iaddr = link.localAddress;
			raddr = link.remoteAddress;
		} else {
			iaddr = link.remoteAddress;
			raddr = link.localAddress;
		}
		return generateLELegacyConfirmValue(
			this._temporaryKey, random,
			this._smCtx.pairingInfo.response,
			this._smCtx.pairingInfo.request,
			iaddr.isRandom() ? 0x01 : 0x00, iaddr.getRawArray(),
			raddr.isRandom() ? 0x01 : 0x00, raddr.getRawArray()
		);
	}
	sendConfirm() {
		if (this._temporaryKey == null) {
			this.log("TK is null, user canceled the pairing");
			this._smCtx.pairingFailed(Code.PASSKEY_ENTRY_FAILED);
			return;
		}
		this.generateRandom().then(random => {
			return this.generateConfirmValue(random);
		}).then(value => {
			this._smCtx.sendCommand(assembleValue128(Code.PAIRING_CONFIRM, value));
		});
	}
	setTemporaryKey(tk) {
		this._temporaryKey = tk;
		this.log("TK has been set: " + Utils.toFrameString(tk));
		if (this._smCtx.pairingInfo.initiator || (this._confirm != null)) {
			this.sendConfirm();
		}
	}
	smpReceived(code, buffer) {
		switch (code) {
		case Code.PAIRING_CONFIRM:
			{
				this._confirm = buffer.getByteArray(16);
				this.log("Got Pairing Confirm: " + Utils.toFrameString(this._confirm));
				if (this._smCtx.pairingInfo.initiator) {
					this._smCtx.sendCommand(assembleValue128(Code.PAIRING_RANDOM, this._random));
				} else {
					if (this._temporaryKey === undefined) {
						this.log("TK is not yet ready");
						// Will wait user for TK
					} else {
						this.sendConfirm();
					}
				}
			}
			return true;
		case Code.PAIRING_RANDOM:
			{
				let remoteRandom = buffer.getByteArray(16);
				this.log("Got Pairing Random: " + Utils.toFrameString(remoteRandom));
				this.generateConfirmValue(remoteRandom).then(calcValue => {
					this.log("Calculated confirm: " + Utils.toFrameString(calcValue));
					if (BTUtils.isArrayEquals(this._confirm, calcValue)) {
						this.log("Confirm Value matched");
						let r1;
						let r2;
						if (this._smCtx.pairingInfo.initiator) {
							r1 = remoteRandom;
							r2 = this._random;
						} else {
							this._smCtx.sendCommand(assembleValue128(Code.PAIRING_RANDOM, this._random));
							r1 = this._random;
							r2 = remoteRandom;
						}
						this.log("Generate STK");
						generateLELegacyKey(this._temporaryKey, r1, r2).then(stk => {
							this._smCtx.shortTermKeyGenerated(stk);
						});
					} else {
						this._smCtx.pairingFailed(FailedReason.CONFIRM_VALUE_FAILED);
					}
				});
			}
			return true;
		}
		return false;
	}
}

class KeyDistributionState extends State {
	constructor(smCtx, keyMgmt, div) {
		super("Phase 3");
		this._smCtx = smCtx;
		this._keyMgmt = keyMgmt;
		this._div = div;
		this._db = {
			longTermKey: null,
			ediv: null,
			random: null,
			identityResolvingKey: null,
			address: null,
			signatureKey: null
		};
		let info = this._smCtx.pairingInfo;
		if (info.initiator) {
			this._localKeys = info.initiatorKeys;
			this._remoteKeys = info.responderKeys;
		} else {
			this._localKeys = info.responderKeys;
			this._remoteKeys = info.initiatorKeys;
		}
		this._localKeys &= 0x07;
		this._remoteKeys &= 0x07;
	}
	distributeKeys() {
		this.log("Distribute Local Keys");
		let promises = [];
		/* Key Distribution order: LTK->EDIV/Rand->IRK->BD_ADDR->CSRK */
		if ((this._localKeys & KeyDistributionFormat.ENC_KEY) > 0 && this._smCtx.pairingInfo.legacy) {
			promises.push(
				this._keyMgmt.generateLTK(this._div).then(ltk => {
					return Promise.resolve(assembleValue128(Code.ENCRYPTION_INFORMATION, ltk));
				})
			);
			promises.push(
				this._keyMgmt.generateEDIV(this._div).then(r => {
					return Promise.resolve(assembleMasterIdentification(r.ediv, r.random));
				})
			);
		}
		if ((this._localKeys & KeyDistributionFormat.ID_KEY) > 0) {
			let identityAddress = this._smCtx.parameter.identityAddress;
			if (identityAddress == null) {
				this.log("Force using Public Address as identity");
				identityAddress = _hci.getPublicAddress();
			}
			promises.push(
				this._keyMgmt.generateIRK().then(irk => {
					return Promise.resolve(assembleValue128(Code.IDENTITY_INFORMATION, irk));
				})
			);
			promises.push(
				Promise.resolve(assembleIdentityAddressInformation(identityAddress))
			);

		}
		if ((this._localKeys & KeyDistributionFormat.SIGN) > 0) {
			promises.push(
				this._keyMgmt.generateCSRK().then(csrk => {
					return Promise.resolve(assembleValue128(Code.SIGNING_INFORMATION, csrk));
				})
			);
		}
		if (promises.length > 0) {
			Promise.all(promises).then(packets => {
				for (let packet of packets) {
					this._smCtx.sendCommand(packet);
				}
				this._localKeys = 0;
				this.log("All Keys are distributed");
				if (this._remoteKeys == 0) {
					this._smCtx.keyExchanged(this._db);
				}
			});
		} else if (this._remoteKeys == 0) {
			this._smCtx.keyExchanged(this._db);
		}
	}
	keyReceived(key) {
		this._remoteKeys &= ~key;
		if (this._remoteKeys == 0) {
			this.log("All Keys are received");
			if (this._localKeys != 0) {
				this.distributeKeys();
			} else {
				this._smCtx.keyExchanged(this._db);
			}
		}
	}
	smpReceived(code, buffer) {
		switch (code) {
		case Code.ENCRYPTION_INFORMATION:
			this.log("Got LTK");
			if (this._smCtx.pairingInfo.legacy) {
				/* TODO: Apply key size */
				this._db.longTermKey = buffer.getByteArray(16);
			}
			return true;
		case Code.MASTER_IDENTIFICATION:
			this.log("Got EDIV and Rand");
			if (this._smCtx.pairingInfo.legacy) {
				this._db.ediv = buffer.getInt16();
				this._db.random = buffer.getByteArray(8);
			}
			this.keyReceived(KeyDistributionFormat.ENC_KEY);
			return true;
		case Code.IDENTITY_INFORMATION:
			this.log("Got IRK");
			this._db.identityResolvingKey = buffer.getByteArray(16);
			return true;
		case Code.IDENTITY_ADDRESS_INFORMATION:
			this.log("Got BD_ADDR");
			let addressType = buffer.getInt8();
			this._db.address = BluetoothAddress.getByAddress(buffer.getByteArray(6), true, (addressType == 0x01));
			this.keyReceived(KeyDistributionFormat.ID_KEY);
			return true;
		case Code.SIGNING_INFORMATION:
			this.log("Got CSRK");
			this._db.signatureKey = buffer.getByteArray(16);
			this.keyReceived(KeyDistributionFormat.SIGN);
			return true;
		}
		return false;
	}
}

/******************************************************************************
 * Utils
 ******************************************************************************/

function arrayIsZero(a, len = a.length) {
	for (let i = 0; i < len; i++) {
		if (a[i] != 0) {
			return false;
		}
	}
	return true;
}

function arrayXOR(a1, a2, len = a1.length) {
	let xrd = new Uint8Array(len);
	for (let i = 0; i < len; i++) {
		xrd[i] = a1[i] ^ a2[i];
	}
	return xrd;
}

function arraySwap(ar) {
	let sw = new Uint8Array(ar.length);
	for (let i = 0; i < sw.length; i++) {
		sw[i] = ar[sw.length - i - 1];
	}
	return sw;
}

/******************************************************************************
 * SMP PDUs
 ******************************************************************************/

function assembleValue8(code, value) {
	logger.trace("Assemble 8-bit Value: code=" + Utils.toHexString(code)
		+ ", value=" + Utils.toHexString(value));
	let packet = new Uint8Array(2);
	packet[0] = code;
	packet[1] = value;
	return packet;
}

function assembleValue128(code, value) {
	logger.trace("Assemble 128-bit Value: code=" + Utils.toHexString(code)
		+ ", value=" + Utils.toFrameString(value));
	let packet = new Uint8Array(17);
	packet[0] = code;
	packet.set(value, 1);
	return packet;
}

function assemblePairingFeature(request, feature) {
	logger.trace("Assemble Pairing " + (request ? "Request" : "Response"));
	let buffer = ByteBuffer.allocateUint8Array(7, true);
	buffer.putInt8(request ? Code.PAIRING_REQUEST : Code.PAIRING_RESPONSE);
	buffer.putInt8(feature.ioCapability);
	buffer.putInt8(feature.outOfBand ? 0x01 : 0x00);
	buffer.putInt8(feature.authReq);
	buffer.putInt8(feature.maxKeySize);
	buffer.putInt8(feature.initiatorKeys);
	buffer.putInt8(feature.responderKeys);
	return buffer.array;
}

function parsePairingFeature(packet) {
	return {
		ioCapability: packet[1],
		outOfBand: (packet[2] == 0x01),
		authReq: packet[3],
		maxKeySize: packet[4],
		initiatorKeys: packet[5],
		responderKeys: packet[6]
	};
}

function assembleMasterIdentification(ediv, random) {
	logger.trace("Assemble Master Identification");
	let buffer = ByteBuffer.allocateUint8Array(11, true);
	buffer.putInt8(Code.MASTER_IDENTIFICATION);
	buffer.putInt16(ediv);
	buffer.putByteArray(random);
	return buffer.array;
}

function assembleIdentityAddressInformation(address) {
	logger.trace("Assemble Identity Address Information");
	let buffer = ByteBuffer.allocateUint8Array(8, true);
	buffer.putInt8(Code.IDENTITY_ADDRESS_INFORMATION);
	buffer.putInt8(address.isRandom() ? 0x01 : 0x00);
	buffer.putByteArray(address.getRawArray());
	return buffer.array;
}

/******************************************************************************
 * Appendix A EDIV And Rand Generation
 ******************************************************************************/

/**
 * A.1.1 DIV Mask generation function dm
 */
function divMaskGeneration(k, r) {
	let rd = new Uint8Array(16);
	rd.set(r);	// LSB First
	logger.trace("rd=" + Utils.toFrameString(rd));
	return encrypt(k, rd).then(e => {
		return Utils.toInt16(e, true);
	});
}

/******************************************************************************
 * Appendix B Key Management
 ******************************************************************************/

function diversifying(k, d, r) {
	let buffer = ByteBuffer.allocateUint8Array(16, true);	// LSB First
	buffer.putInt16(d);				// 16bit = 2octets
	buffer.putInt16(r);				// 16bit = 2octets
	let dd = buffer.array;
	logger.trace("dd=" + Utils.toFrameString(dd));
	return encrypt(k, dd);
}

class DefaultKeyManagement {
	constructor(ir, er) {
		this._ir = ir;
		this._er = er;
		this._random = null;
	}
	generateEDIV(div) {
		return Promise.all([this.generateDHK(), random64()]).then(r => {
			this._random = r[1];
			return divMaskGeneration(r[0], r[1]);
		}).then(y => {
			return {
				random: this._random,
				ediv: y ^ div,
			};
		});
	}
	recoverDIV(random, ediv) {
		return this.generateDHK().then(dhk => {
			return divMaskGeneration(dhk, random);
		}).then(y => {
			return y ^ ediv
		});
	}
	generateLTK(div) {
		/* TODO: Apply key size */
		return diversifying(this._er, div, 0);
	}
	generateCSRK(div) {
		return diversifying(this._er, div, 1);
	}
	generateIRK() {
		return diversifying(this._ir, 1, 0);
	}
	generateDHK() {
		return diversifying(this._ir, 3, 0);
	}
	generateLTKFromEDIV(random, ediv) {
		return this.generateDHK().then(dhk => {
			return divMaskGeneration(dhk, random);
		}).then(y => {
			let div = y ^ ediv;
			return this.generateLTK(div)
		});
	}
}
exports.DefaultKeyManagement = DefaultKeyManagement;

/******************************************************************************
 * 2.2 Crypto Toolbox
 ******************************************************************************/

function random64() {
	return new Promise((resolve, reject) => {
		_hci.LE.rand({
			commandComplete: (opcode, buffer) => {
				let status = buffer.getInt8();
				if (status == 0) {
					resolve(buffer.getByteArray(8));
				} else {
					logger.error("LE rand failed: status=" + Utils.toHexString(status));
					reject(status);
				}
			}
		});
	});
}

function random128() {
	return Promise.all([random64(), random64()]).then(rand => {
		let rand128 = new Uint8Array(16);
		rand128.set(rand[0]);
		rand128.set(rand[1], 8);
		return rand128;
	});
}

/**
 * 2.2.1 Security function e
 */
function encrypt(key, plainData) {
	return new Promise((resolve, reject) => {
		_hci.LE.encrypt(key, plainData, {
			commandComplete: (opcode, buffer) => {
				let status = buffer.getInt8();
				if (status == 0) {
					let e = buffer.getByteArray(16);
					resolve(e);
				} else {
					logger.error("LE encryption failed: status=" + Utils.toHexString(status));
					reject(status);
				}
			}
		});
	});
}

/**
 * 2.2.2 Random Address Hash function ah
 */
function generateRandomAddressHash(k, r) {
	let rd = new Uint8Array(16);
	rd.set(r);	// LSB First
	logger.trace("rd=" + Utils.toFrameString(rd));
	return encrypt(k, rd).then(e => {
		return e.slice(0, 3);
	});
}

/**
 * 2.2.3 Confirm value generation function c1 for LE Legacy Pairing
 */
function generateLELegacyConfirmValue(k, r, pres, preq, iat, ia, rat, ra) {
	let p1 = new Uint8Array(16);
	/* LSB First */
	p1[0] = iat & 0x01;
	p1[1] = rat & 0x01;
	p1.set(preq, 2);				// 56bit = 7octets
	p1.set(pres, 9);				// 56bit = 7octets
	let p2 = new Uint8Array(16).fill(0);	// XXX: XS6 init bug
	/* LSB First */
	p2.set(ra, 0);					// 48bit = 6octets
	p2.set(ia, 6);					// 48bit = 6octets
	logger.trace("p1=" + Utils.toFrameString(p1));
	logger.trace("p2=" + Utils.toFrameString(p2));
	logger.trace("r=" + Utils.toFrameString(r));
	let p1d = arrayXOR(r, p1);
	logger.trace("p1'=" + Utils.toFrameString(p1d));
	return encrypt(k, p1d).then(e => {
		let p2d = arrayXOR(e, p2);
		logger.trace("p2'=" + Utils.toFrameString(p2d));
		return encrypt(k, p2d);
	});
}

/**
 * 2.2.4 Key generation function s1 for LE Legacy Pairing
 */
function generateLELegacyKey(k, r1, r2) {
	let rd = new Uint8Array(16);	// LSB First
	rd.set(r2.slice(0, 8), 0);
	rd.set(r1.slice(0, 8), 8);
	logger.trace("rd=" + Utils.toFrameString(rd));
	return encrypt(k, rd);
}

exports.generatePrivateAddress = function (irk) {
	return random64().then(r => {
		let prand = r.slice(0, 3);
		prand[2] &= ~0x80;
		logger.trace("prand=" + Utils.toFrameString(prand));
		return generateRandomAddressHash(irk, prand).then(hash => {
			logger.trace("hash=" + Utils.toFrameString(hash));
			let address = new Uint8Array(6);
			address.set(hash);
			address.set(prand, 3);
			return BluetoothAddress.getByAddress(address, true, true);
		});
	});
};

exports.resolvePrivateAddress = function (irk, address) {
	let array = address.getRawArray();
	let prand = array.slice(3);
	logger.trace("prand=" + Utils.toFrameString(prand));
	return generateRandomAddressHash(irk, prand).then(hash => {
		logger.trace("hash=" + Utils.toFrameString(hash));
		return BTUtils.isArrayEquals(hash, array.slice(0, 3));
	});
};
