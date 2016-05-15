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
 * Bluetooth v4.2 - GATT Server Implementation
 */

const GATT = require("../core/gatt");
const ATT = GATT.ATT;

const Utils = require("../../common/utils");
const Logger = Utils.Logger;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

var logger = Logger.getLogger("GATTServer");

class Characteristic extends GATT.Characteristic {
	constructor(uuid, properties, extProperties = 0x0000) {
		super(uuid, properties);
		/* Descriptors */
		if ((this._properties & GATT.Properties.EXT) > 0) {
			this._extProperties = extProperties;
		} else {
			this._extProperties = 0x0000;
		}
		this._description = null;
		this._clientConfigurations = {};
		/* Security Configs */
		this._security = null;
		this._securityCC = null;
		this._securityDesc = null;
		/* ATT Contexts */
		this._attribute = null;
		this._valueAttribute = null;
		/* Callbacks */
		this._onValueRead = null;
		this._onValueWrite = null;
	}
	get extProperties() {
		return this._extProperties;
	}
	get description() {
		return this._description;
	}
	set description(description) {
		this._description = description;
	}
	set onValueRead(onValueRead) {
		this._onValueRead = onValueRead;
	}
	set onValueWrite(onValueWrite) {
		this._onValueWrite = onValueWrite;
	}
	set security(security) {
		this._security = security;
	}
	set clientConfigurationSecurity(security) {
		this._securityCC = security;
	}
	set descriptionSecurity(security) {
		this._securityDesc = security;
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
	 * 4.10.1 Notifications
	 */
	notifyValue(bearer, value) {
		if (value !== undefined) {
			this.value = value;
		}
		bearer.sendPDU(ATT.assembleHandleValueNotificationPDU(this._valueAttribute.handle, this.serializeValue()));
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
	 * 4.11.1 Indications
	 */
	indicateValue(bearer, value) {
		if (value !== undefined) {
			this.value = value;
		}
		return new Promise((resolve, reject) => {
			bearer.scheduleTransaction(
				ATT.assembleHandleValueIndicationPDU(this._valueAttribute.handle, this.serializeValue()),
				{
					transactionCompleteWithResponse: (opcode, response) => {
						if (opcode != ATT.Opcode.HANDLE_VALUE_CONFIRMATION) {
							return;
						}
						logger.info("Indicated for handle=" + Utils.toHexString(this._valueAttribute.handle, 2));
						resolve();
					},
					transactionCompleteWithError: (errorCode, handle) => {
						logger.error("Indicate error for handle=" + Utils.toHexString(handle, 2)
							+ ", errorCode=" + Utils.toHexString(errorCode));
						reject(errorCode, handle);
					}
				}
			);
		});
	}
	serializeDefinition() {
		let uuid16 = this._uuid.isUUID16();
		let buffer = ByteBuffer.allocate(3 + (uuid16 ? 2 : 16), true);
		buffer.putInt8(this._properties);
		buffer.putInt16(this._valueAttribute.handle);
		if (uuid16) {
			buffer.putInt16(this._uuid.toUUID16());
		} else {
			buffer.putByteArray(this._uuid.getRawArray());
		}
		buffer.flip();
		return buffer.getByteArray();
	}
	hasClientConfiguration() {
		return (this._properties & GATT.Properties.NOTIFY) > 0 || (this._properties & GATT.Properties.INDICATE) > 0;
	}
	getClientConfiguration(bearer) {
		let identifier = bearer.identifier;
		if (!this._clientConfigurations.hasOwnProperty(identifier)) {
			this._clientConfigurations[identifier] = 0x0000;
		}
		return this._clientConfigurations[identifier];
	}
	deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC);
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Characteristic definition on read");
			return this.serializeDefinition();
		};
		/* Deploy value attribute */
		let valueAttribute = db.allocateAttribute(this._uuid);
		if ((this._properties & GATT.Properties.READ) > 0) {
			valueAttribute.callback.onRead = (attribute) => {
				if (this._onValueRead != null) {
					this._onValueRead(this);
				}
				return this.serializeValue();
			};
		}
		let allowWrite = (this._properties & GATT.Properties.WRITE) > 0;
		let allowCommand = (this._properties & GATT.Properties.WRITE_WO_RESP) > 0;
		let allowAuthWrite = (this._properties & GATT.Properties.AUTH_WRITE) > 0;
		if (allowWrite || allowCommand || allowAuthWrite) {
			valueAttribute.callback.onWrite = (attribute, value, context) => {
				if ((context.opcode == ATT.Opcode.WRITE_REQUEST) && !allowWrite) {
					logger.error("WriteRequest is not allowed");
					throw ATT.ErrorCode.REQUEST_NOT_SUPPORTED;
				}
				if ((context.opcode == ATT.Opcode.WRITE_COMMAND) && !allowCommand) {
					logger.error("WriteCommand is not allowed");
					throw ATT.ErrorCode.REQUEST_NOT_SUPPORTED;
				}
				if ((context.opcode == ATT.Opcode.SIGNED_WRITE_COMMAND) && !allowAuthWrite) {
					logger.error("SignedWriteCommand is not allowed");
					throw ATT.ErrorCode.REQUEST_NOT_SUPPORTED;
				}
				if (this._onValueWrite != null) {
					this._onValueWrite(this);
				}
				this.parseValue(value);
			};
		}
		if (this._security != null) {
			if (this._security.hasOwnProperty("read")) {
				valueAttribute.security.read = this._security.read;
			}
			if (this._security.hasOwnProperty("write")) {
				valueAttribute.security.write = this._security.write;
			}
		}
		this._valueAttribute = valueAttribute;
		/* Deploy descriptors */
		/* 3.3.3.1 Characteristic Extended Properties */
		if ((this._properties & GATT.Properties.EXT) > 0) {
			logger.debug("Deploy: Characteristic Extended Properties");
			let descriptorAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC_EXTENDED_PROPERTIES);
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic ext properties on read");
				return GATT.serializeWithFormat(this._extProperties, GATT.Format.UINT16);
			};
		}
		/* 3.3.3.2 Characteristic User Description */
		let writableAux = (this._extProperties & GATT.ExtendedProperties.WRITABLE_AUX) > 0;
		if (this._description != null || writableAux) {
			logger.debug("Deploy: Characteristic User Description");
			let descriptorAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC_USER_DESCRIPTION);
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic user description on read");
				return GATT.serializeWithFormat(this._description, GATT.Format.UTF8S);
			};
			if (writableAux) {
				descriptorAttribute.callback.onWrite = (attribute, value) => {
					logger.debug("Characteristic user description on write");
					this._description = GATT.parseWithFormat(value, GATT.Format.UTF8S);
				};
			}
			if (this._securityDesc != null) {
				if (this._securityDesc.hasOwnProperty("read")) {
					descriptorAttribute.security.read = this._securityDesc.read;
				}
				if (this._securityDesc.hasOwnProperty("write")) {
					descriptorAttribute.security.write = this._securityDesc.write;
				}
			}
			this._description.attribute = descriptorAttribute;
		}
		/* 3.3.3.3 Client Characteristic Configuration */
		if (this.hasClientConfiguration()) {
			logger.debug("Deploy: Client Characteristic Configuration");
			let descriptorAttribute = db.allocateAttribute(GATT.UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);
			descriptorAttribute.callback.onRead = (attribute, context) => {
				logger.debug("Characteristic client config on read");
				if (context == null) {
					logger.error("ATT context not found while reading");
					/* XXX: We assume ATT is requesting find attributes by value */
					return null;
				}
				let config = this.getClientConfiguration(context.bearer);
				return GATT.serializeWithFormat(config, GATT.Format.UINT16);
			};
			descriptorAttribute.callback.onWrite = (attribute, value, context) => {
				logger.debug("Characteristic client config on write");
				if (!this.hasClientConfiguration()) {
					logger.debug("Characteristic has no client configuration");
					return;
				}
				let identifier = context.bearer.identifier;
				let config = 0x0000;
				let configToWrite = GATT.parseWithFormat(value, GATT.Format.UINT16);
				if ((this._properties & GATT.Properties.NOTIFY) > 0 && (configToWrite & GATT.ClientConfiguration.NOTIFICATION) > 0) {
					logger.debug("Config notification for link=" + Utils.toHexString(identifier, 2));
					config |= GATT.ClientConfiguration.NOTIFICATION;
				}
				if ((this._properties & GATT.Properties.INDICATE) > 0 && (configToWrite & GATT.ClientConfiguration.INDICATION) > 0) {
					logger.debug("Config indication for link=" + Utils.toHexString(identifier, 2));
					config |= GATT.ClientConfiguration.INDICATION;
				}
				this._clientConfigurations[identifier] = config;
			};
			if (this._securityCC != null) {
				if (this._securityCC.hasOwnProperty("write")) {
					descriptorAttribute.security.write = this._securityCC.write;
				}
			}
		}
		/* 3.3.3.4 Server Characteristic Configuration */
		// TODO
		/* 3.3.3.5 Characteristic Presentation Format */
		for (let format of this._formats) {
			logger.debug("Deploy: Characteristic Presentation Format");
			let descriptorAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC_PRESENTATION_FORMAT);
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic presentation format on read");
				return GATT.serializeFormat(format);
			};
			format.attribute = descriptorAttribute;
		}
		/* 3.3.3.6 Characteristic Aggregate Format */
		if (this._formats.length > 1) {
			logger.debug("Deploy: Characteristic Presentation Format");
			let descriptorAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC_AGGREGATE_FORMAT);
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic Presentation Format on read");
				let buffer = ByteBuffer.allocate(this._formats.length * 2);
				for (let format of this._formats) {
					buffer.putInt16(format.attribute.handle);
				}
				buffer.flip();
				return buffer.getByteArray();
			};
		}
		definitionAttribute.groupEnd = db.getEndHandle();	// Should fix it
		this._attribute = definitionAttribute;
	}
}
exports.Characteristic = Characteristic;

class Include extends GATT.Include {
	constructor(uuid, profile) {
		super(uuid);
		this._profile = profile;
		/* ATT Contexts */
		this._attribute = null;
	}
	serializeDefinition() {
		let candidate = this._profile.getServiceByUUID(this._uuid);
		let buffer = ByteBuffer.allocate(6, true);
		buffer.putInt16(candidate.start);
		buffer.putInt16(candidate.end);
		if (this._uuid.isUUID16()) {
			buffer.putInt16(this._uuid.toUUID16());
		}
		buffer.flip();
		return buffer.getByteArray();
	}
	deploy(db) {
		let definitionAttribute = db.allocateAttribute(GATT.UUID_INCLUDE);
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Include definition on read");
			return this.serializeDefinition();
		};
		this._attribute = definitionAttribute;
	}
}
exports.Include = Include;

class Service extends GATT.Service {
	constructor(uuid, primary = true) {
		super(uuid, primary);
		/* ATT Contexts */
		this._attribute = null;
	}
	serializeDefinition() {
		if (this._uuid.isUUID16()) {
			return Utils.toByteArray(this._uuid.toUUID16(), 2, true);
		}
		return this._uuid.getRawArray();
	}
	deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(
			this._primary ? GATT.UUID_PRIMARY_SERVICE : GATT.UUID_SECONDARY_SERVICE);
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Service definition on read");
			return this.serializeDefinition();
		};
		/* Deploy includes */
		for (let include of this.includes) {
			include.deploy(db);
		}
		/* Deploy characteristics */
		for (let characteristic of this.characteristics) {
			characteristic.deploy(db);
		}
		definitionAttribute.groupEnd = db.getEndHandle();	// Should fix it
		this._attribute = definitionAttribute;
	}
}
exports.Service = Service;

class Profile extends GATT.Profile {
	constructor(database = null) {
		super();
		if (database != null) {
			this._database = database;
		} else {
			this._database = new ATT.AttributeDatabase();
		}
	}
	get database() {
		return this._database;
	}
	deployServices(services) {
		let start = this._database.getEndHandle() + 1;
		/* Add all Services */
		for (let service of services) {
			service.deploy(this._database);
			this.addService(service);
		}
		/* Generate handles */
		this._database.assignHandles();

		let end = this._database.getEndHandle();
		logger.info("Service Changed(Added): start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));

		return {
			start,
			end
		};
	}
}
exports.Profile = Profile;
