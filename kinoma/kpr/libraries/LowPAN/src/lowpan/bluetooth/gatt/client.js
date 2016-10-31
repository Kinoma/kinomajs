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
 * Bluetooth v4.2 - GATT Client Implementation
 */

const GATT = require("../core/gatt");
const ATT = GATT.ATT;
const BTUtils = require("../core/btutils");
const UUID = BTUtils.UUID;

const Utils = require("../../common/utils");
const Logger = Utils.Logger;

var logger = Logger.getLogger("GATTClient");

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.3 Server Configuration
 */
function exchangeMTU(bearer, mtu) {
	if (mtu === undefined) {
		mtu = bearer.mtu;
	}
	return new Promise((resolve, reject) => {
		logger.debug("exchangeMTU");
		bearer.scheduleTransaction(
			ATT.assembleExchangeMTURequestPDU(mtu),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.EXCHANGE_MTU_RESPONSE) {
						return;
					}
					let serverMTU = response.mtu;
					if (serverMTU < bearer.mtu) {
						bearer.mtu = serverMTU;
					} else {
						bearer.mtu = mtu;
					}
					logger.info("ATT Bearer MTU has been changed to: " + bearer.mtu);
					resolve(bearer.mtu);
				},
				transactionCompleteWithError: (errorCode, handle) => {
					logger.error("Exchange MTU Response: errorCode=" + Utils.toHexString(errorCode));
					bearer.mtu = ATT.ATT_MTU;	// Use default MTU
					reject(errorCode, bearer.mtu);
				}
			}
		);
	});
}
exports.exchangeMTU = exchangeMTU;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.4 Primary Service Discovery
 */
function discoverPrimaryServices(bearer, callback, uuid = null) {
	let _procedure = (start, end, uuid, resolve, reject) => {
		logger.debug("Start Primary Service Discovery: "
			+ " start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));
		let discoverAll = (uuid == null);
		let pdu;
		if (discoverAll) {
			pdu = ATT.assembleReadByGroupTypeRequestPDU(start, end,
				GATT.UUID_PRIMARY_SERVICE);
		} else {
			pdu = ATT.assembleFindByTypeValueRequestPDU(start, end,
				GATT.UUID_PRIMARY_SERVICE, uuid.getRawArray());
		}
		bearer.scheduleTransaction(pdu, {
			transactionCompleteWithResponse: (opcode, response) => {
				if (opcode != (discoverAll
									? ATT.Opcode.READ_BY_GROUP_TYPE_RESPONSE
									: ATT.Opcode.FIND_BY_TYPE_VALUE_RESPONSE)) {
					return;
				}
				let services = [];
				for (let i = 0; i < response.length; i++) {
					let attribute = response[i];
					services.push({
						primary: true,
						uuid: (discoverAll ? UUID.getByUUID(attribute.value) : uuid),
						start: attribute.handle,
						end: attribute.groupEnd
					});
				}
				callback(services);
				let lastHandle = response[response.length - 1].groupEnd;
				if (lastHandle == 0xFFFF) {
					logger.debug("SD complete with max handle");
					resolve(lastHandle);
				} else {
					if (discoverAll) {
						_procedure(lastHandle + 1, end, null, resolve, reject);
					} else {
						_procedure(lastHandle + 1, end, uuid, resolve, reject);
					}
				}
			},
			transactionCompleteWithError: (errorCode, handle) => {
				if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
					logger.debug("SD complete with attribute not found");
					resolve(handle);
				} else {
					logger.debug("SD complete with error=" + Utils.toHexString(errorCode));
					reject(errorCode, handle);
				}
			}
		});
	};
	return new Promise((resolve, reject) => {
		_procedure(0x0001, 0xFFFF, uuid, resolve, reject);
	});
}
exports.discoverPrimaryServices = discoverPrimaryServices;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.5 Relationship Discovery
 */
function findIncludedServices(bearer, start, end, callback) {
	let _procedure = (start, end, resolve, reject) => {
		logger.debug("Start Relationship Discovery: "
			+ " start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));
		bearer.scheduleTransaction(
			ATT.assembleReadByTypeRequestPDU(start, end, GATT.UUID_INCLUDE),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
						return;
					}
					let includes = [];
					for (let i = 0; i < response.length; i++) {
						let attribute = response[i];
						let include = {
							handle: attribute.handle,	// XXX: Could be optional
							start: Utils.toInt(attribute.value, 0, 2, true),
							end: Utils.toInt(attribute.value, 2, 2, true)
						};
						if (attribute.value > 4) {
							include.uuid = UUID.getByUUID16(Utils.toInt(attribute.value, 4, 2, true));
							includes.push(include);
						} else {
							/* Issue ReadRequest to read 128-bit UUID */
							bearer.scheduleTransaction(ATT.assembleReadRequestPDU(include.start), {
								transactionCompleteWithResponse: (opcode, response) => {
									if (opcode != ATT.Opcode.READ_REQUEST) {
										return;
									}
									include.uuid = UUID.getByUUID(response);
									callback([include]);	// TODO: Early termination
								},
								transactionCompleteWithError: (errorCode, handle) => {
									// TODO
								}
							});
						}
					}
					if (!callback(includes)) {
						return;	// Terminate the sub-procedure
					}
					let lastHandle = response[response.length - 1].handle;
					if (lastHandle == end) {
						resolve(lastHandle);
					} else {
						_procedure(lastHandle + 1, end, resolve, reject);
					}
				},
				transactionCompleteWithError: (errorCode, handle) => {
					if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
						resolve(handle);
					} else {
						reject(errorCode, handle);
					}
				}
			}
		);
	};
	return new Promise((resolve, reject) => {
		_procedure(start, end, resolve, reject);
	});
}
exports.findIncludedServices = findIncludedServices;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.6 Characteristic Discovery
 */
function discoverCharacteristics(bearer, start, end, callback, uuid = null) {
	let _procedure = (start, end, uuid, resolve, reject) => {
		logger.debug("Start Characteristic Discovery:"
			+ " start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));
		bearer.scheduleTransaction(
			ATT.assembleReadByTypeRequestPDU(start, end, GATT.UUID_CHARACTERISTIC),
			{
				transactionCompleteWithResponse:(opcode, response) => {
					if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
						return;
					}
					let characteristics = [];
					for (let i = 0; i < response.length; i++) {
						let attribute = response[i];
						let characteristic = {
							definition_handle: attribute.handle,			// XXX: We may not need
							properties: attribute.value[0],
							handle: Utils.toInt(attribute.value, 1, 2, true),
							uuid: UUID.getByUUID(attribute.value.slice(3))
						};
						if ((uuid != null) && !uuid.equals(characteristic.uuid)) {
							continue;	// Skip unmatched characteristic
						}
						characteristics.push(characteristic);
					}
					if (!callback(characteristics)) {
						logger.debug("CD terminated earlier");
						return;	// Terminate the sub-procedure
					}
					let lastHandle = response[response.length - 1].handle;
					if (lastHandle == end) {
						logger.debug("CD complete with end handle=" + Utils.toHexString(end, 2));
						resolve(lastHandle);
					} else {
						_procedure(lastHandle + 1, end, uuid, resolve, reject);
					}
				},
				transactionCompleteWithError: (errorCode, handle) => {
					if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
						logger.debug("CD complete with attribute not found");
						resolve(handle);
					} else {
						logger.debug("CD complete with error=" + Utils.toHexString(errorCode));
						reject(errorCode, handle);
					}
				}
			}
		);
	};
	return new Promise((resolve, reject) => {
		_procedure(start, end, uuid, resolve, reject);
	});
}
exports.discoverCharacteristics = discoverCharacteristics;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.7 Characteristic Descriptor Discovery
 */
function discoverCharacteristicDescriptors(bearer, start, end, callback) {
	let _procedure = (start, end, resolve, reject) => {
		logger.debug("Start Characteristic Descriptor Discovery:"
			+ " start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));
		bearer.scheduleTransaction(
			ATT.assembleFindInformationRequestPDU(start, end),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.FIND_INFORMATION_RESPONSE) {
						return;
					}
					let descriptors = [];
					for (let i = 0; i < response.length; i++) {
						let attribute = response[i];
						descriptors.push({
							handle: attribute.handle,
							uuid: attribute.type
						});
					}
					if (!callback(descriptors)) {
						logger.debug("CDD terminated earlier");
						return;	// Terminate the sub-procedure
					}
					let lastHandle = response[response.length - 1].handle;
					if (lastHandle == end) {
						logger.debug("CDD complete with end handle=" + Utils.toHexString(end, 2));
						resolve(lastHandle);
					} else {
						_procedure(lastHandle + 1, end, resolve, reject);
					}
				},
				transactionCompleteWithError: (errorCode, handle) => {
					if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
						logger.debug("CDD complete with attribute not found");
						resolve(handle);
					} else {
						logger.debug("CDD complete with error=" + Utils.toHexString(errorCode));
						reject(errorCode, handle);
					}
				}
			}
		);
	}
	return new Promise((resolve, reject) => {
		_procedure(start, end, resolve, reject);
	});
}
exports.discoverCharacteristicDescriptors = discoverCharacteristicDescriptors;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Characteristic Value
 * 4.8.3 Read Long Characteristic Value
 * 4.12.1 Read Characteristic Descriptors
 * 4.12.2 Read Long Characteristic Descriptors
 */
function readValue(bearer, handle, length) {
	let long = ((length !== undefined) && length > 0);
	if (long) {
		this._tempValue = new Uint8Array(length);
		this._tempOffset = 0;
	}
	return new Promise((resolve, reject) => {
		logger.debug("readCharacteristicValue");
		let cb = {
			transactionCompleteWithResponse: (opcode, response) => {
				if (opcode != ATT.Opcode.READ_RESPONSE) {
					return;
				}
				if (long) {
					this._tempValue.set(response, this._tempOffset);
					this._tempOffset += response.length;
					if (response.length >= length) {
						resolve(this._tempValue.subarray(0, this._tempOffset));
					} else {
						bearer.scheduleTransaction(ATT.assembleReadBlobRequestPDU(handle, this._tempOffset), cb);
					}
				} else {
					resolve(response);
				}
			},
			transactionCompleteWithError: reject
		};
		bearer.scheduleTransaction(ATT.assembleReadRequestPDU(handle), cb);
	});
}
exports.readValue = readValue;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Using Characteristic UUID
 */
function readUsingCharacteristicUUID(bearer, start, end, uuid) {
	return new Promise((resolve, reject) => {
		bearer.scheduleTransaction(
			ATT.assembleReadByTypeRequestPDU(start, end, uuid),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
						return;
					}
					resolve(response);
				},
				transactionCompleteWithError: reject
			}
		);
	});
}
exports.readUsingCharacteristicUUID = readUsingCharacteristicUUID;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Multiple Characteristic Values
 */
function readMultipleCharacteristicValues(bearer, characteristics, sizeList) {
	let handles = [];
	for (let i = 0; i < characteristics.length; i++) {
		handles.push(characteristics[i].handle);
	}
	return new Promise((resolve, reject) => {
		bearer.scheduleTransaction(
			ATT.assembleReadMultipleRequestPDU(handles),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.READ_MULTIPLE_RESPONSE) {
						return;
					}
					let offset = 0;
					for (let i = 0; i < characteristics.length; i++) {
						let remaining = response.length - offset;
						let len = sizeList[i];
						if (len <= 0) {
							len = remaining;
						}
						len = Math.min(len, remaining);
						let value = response.subarray(offset, offset + len);
						characteristics[i]._parseValue(value);
						offset += len;
						if (offset >= response.length) {
							break;
						}
					}
					resolve();
				},
				transactionCompleteWithError: (errorCode, handle) => {
					reject(errorCode, handle);
				}
			}
		);
	});
}
exports.readMultipleCharacteristicValues = readMultipleCharacteristicValues;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.9.1 Write Without Response
 * 4.9.2 Signed Write Without Response
 */
function writeWithoutResponse(bearer, handle, value, signed = false) {
	logger.debug("writeWithoutResponse: signed=" + signed
		+ ", handle=" + Utils.toHexString(handle, 2)
		+ ", value=" + Utils.toFrameString(value));
	let pdu = ATT.assembleWriteRequestPDU(handle, value);
	pdu[0] |= ATT.Opcode.COMMAND;
	if (signed) {
		pdu[0] |= ATT.Opcode.SIGNED;
	}
	bearer.sendPDU(pdu);
}
exports.writeWithoutResponse = writeWithoutResponse;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.9.3 Write Characteristic Value
 * 4.12.3 Write Characteristic Descriptors
 */
function writeValue(bearer, handle, value) {
	return new Promise((resolve, reject) => {
		logger.debug("writeCharacteristicValue: handle=" + Utils.toHexString(handle, 2)
			+ ", value=" + Utils.toFrameString(value));
		bearer.scheduleTransaction(
			ATT.assembleWriteRequestPDU(handle, value),
			{
				transactionCompleteWithResponse: (opcode, response) => {
					if (opcode != ATT.Opcode.WRITE_RESPONSE) {
						return;
					}
					resolve();
				},
				transactionCompleteWithError: (errorCode, handle) => {
					reject(errorCode, handle);
				}
			}
		);
	});
}
exports.writeValue = writeValue;

class Descriptor extends GATT.Descriptor {
	constructor(bearer, {uuid, handle}) {
		super(uuid);
		this._handle = handle;
		/* Client Functions */
		this.readDescriptorValue = length => {
			return readValue(bearer, handle, length).then(response => {
				this._parseValue(response);
				return this.value;
			});
		};
		this.writeDescriptorValue = value => {
			if (value !== undefined) {
				this.value = value;
			}
			return writeValue(bearer, handle, this._serializeValue());
		};
	}
	get handle() {
		return this._handle;
	}
}
exports.Descriptor = Descriptor;

class Characteristic extends GATT.Characteristic {
	constructor(bearer, {uuid, properties, handle}) {
		super(uuid, properties);
		this._handle = handle;
		this._end = handle;
		/* Client Functions */
		this.discoverAllCharacteristicDescriptors = () => {
			return discoverCharacteristicDescriptors(bearer, handle, this._end, results => {
				for (let result of results) {
					logger.debug("Descriptor discovered: uuid=" + result.uuid.toString()
						+ ", handle=" + Utils.toHexString(result.handle, 2));
					this._addDescriptor(new Descriptor(bearer, result));
				}
				return true;
			});
		};
		this.readCharacteristicValue = length => {
			return readValue(bearer, handle, length).then(response => {
				this._parseValue(response);
				return this.value;
			});
		};
		this.writeWithoutResponse = (signed, value) => {
			if (value !== undefined) {
				this.value = value;
			}
			writeWithoutResponse(bearer, handle, this._serializeValue(), signed);
		};
		this.writeCharacteristicValue = value => {
			if (value !== undefined) {
				this.value = value;
			}
			return writeValue(bearer, handle, this._serializeValue());
		};
		/* Callbacks */
		this._onIndication = null;
		this._onNotification = null;
	}
	get handle() {
		return this._handle;
	}
	get end() {
		return this._end;
	}
	set onIndication(onIndication) {
		this._onIndication = onIndication;
	}
	set onNotification(onNotification) {
		this._onNotification = onNotification;
	}
}
exports.Characteristic = Characteristic;

class Service extends GATT.Service {
	constructor(bearer, {uuid, /*primary,*/ start, end}) {
		super(uuid);
		this._start = start;
		this._end = end;
		/* Client Functions */
		this.findIncludedServices = findIncludedServices.bind(this, bearer, start, end,
			results => {
				for (let result of results) {
					logger.debug("Included service discovered: uuid=" + result.uuid.toString()
						+ ", handle=" + Utils.toHexString(result.handle, 2));
					this.addIncludedService(new Service(bearer, result));
				}
				return true;
			}
		);
		this.discoverCharacteristics = discoverCharacteristics.bind(this, bearer, start, end,
			results => {
				for (let result of results) {
					logger.debug("Characteristic discovered: uuid=" + result.uuid.toString()
						+ ", handle=" + Utils.toHexString(result.handle, 2));
					this._addCharacteristic(new Characteristic(bearer, result));
				}
				return true;
			}
		);
		this.discoverCharacteristicsByUUID = this.discoverCharacteristics;
		this.readUsingCharacteristicUUID = (uuid) => {
			return readUsingCharacteristicUUID(bearer, start, end, uuid).then((response) => {
				if (response.length > 1) {
					logger.warn("Multiple characteristics with the same UUID");
				}
				let attribute = response[0];
				let characteristic = new Characteristic(
					bearer,
					{
						handle: attribute.handle,
						properties: GATT.Properties.READ,	// XXX: At least it reads.
						uuid
					}
				);
				characteristic.value = attribute.value;
				this._addCharacteristic(characteristic);
				return characteristic.value;
			});
		};
	}
	get start() {
		return this._start;
	}
	get end() {
		return this._end;
	}
	getCharacteristicByHandle(handle) {
		for (let characteristic of this.characteristics) {
			if (characteristic.handle == handle) {
				return characteristic;
			}
		}
		return null;
	}
	discoverAllCharacteristics() {
		return this.discoverCharacteristics(null).then(() => {
			logger.debug("Sort Characteristics");
			let characteristics = Array.from(this.characteristics);
			characteristics.sort((a, b) => {
				return a.handle - b.handle;
			});
			for (let i = 0; i < characteristics.length; i++) {
				let characteristic = characteristics[i];
				let next = i + 1;
				if (next == characteristics.length) {
					characteristic._end = this._end;
				} else {
					characteristic._end = characteristics[next].handle;
				}
			}
		});
	}
}
exports.Service = Service;

class Profile extends GATT.Profile {
	constructor(bearer) {
		super();
		bearer.onNotification = (opcode, notification) =>
			this.characteristicValueReceived(opcode, notification);
		bearer.onIndication = (opcode, indication) =>
			this.characteristicValueReceived(opcode, indication);
		/* Client Functions */
		this.exchangeMTU = exchangeMTU.bind(this, bearer);
		let _cb = results => {
			for (let result of results) {
				logger.debug("Service discovered: uuid=" + result.uuid.toString());
				this._addService(new Service(bearer, result));
			}
		};
		this.discoverAllPrimaryServices = discoverPrimaryServices.bind(this, bearer, _cb, null);
		this.discoverPrimaryServiceByServiceUUID = discoverPrimaryServices.bind(this, bearer, _cb);
		this.readMultipleCharacteristicValues = readMultipleCharacteristicValues.bind(this, bearer);
	}
	getCharacteristicByHandle(handle) {
		for (let service of this.services) {
			let characteristic = service.getCharacteristicByHandle(handle);
			if (characteristic != null) {
				return characteristic;
			}
		}
		return null;
	}
	characteristicValueReceived(opcode, result) {
		logger.debug("Value received opcode=" + Utils.toHexString(opcode)
			+ ", handle=" + Utils.toHexString(result.handle, 2));
		let characteristic = this.getCharacteristicByHandle(result.handle);
		if (characteristic != null) {
			logger.debug("Value received on characteristic uuid=" + characteristic.uuid.toString()
				+ ", opcode=" + Utils.toHexString(opcode));
			characteristic._parseValue(result.value);
			if (opcode == ATT.Opcode.HANDLE_VALUE_NOTIFICATION) {
				if (characteristic._onNotification != null) {
					characteristic._onNotification(characteristic.value);
				}
			} else if (opcode == ATT.Opcode.HANDLE_VALUE_INDICATION) {
				if (characteristic._onIndication != null) {
					characteristic._onIndication(characteristic.value);
				}
			}
		}
	}
}
exports.Profile = Profile;
