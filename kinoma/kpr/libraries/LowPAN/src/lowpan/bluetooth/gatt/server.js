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
const BTUtils = require("../core/btutils");
const UUID = BTUtils.UUID;

const Utils = require("../../common/utils");
const Logger = Utils.Logger;
const Buffers = require("../../common/buffers");
const ByteBuffer = Buffers.ByteBuffer;

const DESCRIPTOR_PERM_READ_ONLY = {readable: true, writable: false};
const DESCRIPTOR_PERM_READ_WRITE = {readable: true, writable: true};

var logger = Logger.getLogger("GATTServer");

class Descriptor extends GATT.Descriptor {
	constructor(uuid, {readable, writable}) {
		super(uuid);
		this._readable = readable;
		this._writable = writable;
		/* Security Requirements */
		this._requirements = null;
		/* Callbacks */
		this._onValueRead = null;
		this._onValueWrite = null;
		/* ATT Contexts */
		this._attribute = null;
	}
	get handle() {
		return (this._attribute != null) ? this._attribute.handle : 0;
	}
	get readable() {
		return this._readable;
	}
	get writable() {
		return this._writable;
	}
	set onValueRead(onValueRead) {
		this._onValueRead = onValueRead;
	}
	set onValueWrite(onValueWrite) {
		this._onValueWrite = onValueWrite;
	}
	set requirements(requirements) {
		if (this._attribute != null) {
			this._attribute.requirements = requirements;
		} else {
			this._requirements = requirements;
		}
	}
	_deploy(db) {
		/* Deploy attribute */
		let valueAttribute = db.allocateAttribute(this._uuid);
		if (this._readable) {
			valueAttribute.callback.onRead = (attribute, context) => {
				if (this._onValueRead != null) {
					this._onValueRead(this, context.bearer.connection);
				}
				return this._serializeValue();
			};
		}
		if (this._writable) {
			valueAttribute.callback.onWrite = (attribute, value, context) => {
				this._parseValue(value);
				if (this._onValueWrite != null) {
					this._onValueWrite(this, context.bearer.connection);
				}
			};
		}
		if (this._requirements != null) {
			valueAttribute.requirements = this._requirements;
		}
		this._attribute = valueAttribute;
	}
}

class Characteristic extends GATT.Characteristic {
	constructor(uuid, properties, extProperties = 0x0000) {
		super(uuid, properties);
		/* Standard descriptor values */
		if ((properties & GATT.Properties.EXT) > 0) {
			this._extProperties = extProperties;
		} else {
			this._extProperties = 0x0000;
		}
		/* Default configuration */
		this._configuration = {
			description: {
				readable: true,
				value: null,
				requirements: null
			},
			client: {
				writable: true,
				requirements: null
			},
			server: {
				writable: true,
				value: 0x0000,
				requirements: null
			}
		};
		/* Security Requirements */
		this._requirements = null;
		/* ATT Contexts */
		this._attribute = null;
		this._valueAttribute = null;
		/* Callbacks */
		this._onValueRead = null;
		this._onValueWrite = null;
	}
	get handle() {
		return (this._attribute != null) ? this._attribute.handle : 0;
	}
	get extProperties() {
		return this._extProperties;
	}
	get configuration() {
		return this._configuration;
	}
	set configuration(configuration) {
		this._configuration = configuration;
	}
	set onValueRead(onValueRead) {
		this._onValueRead = onValueRead;
	}
	set onValueWrite(onValueWrite) {
		this._onValueWrite = onValueWrite;
	}
	set requirements(requirements) {
		this._requirements = requirements;
	}
	addDescriptor(template) {
		if (this._attribute != null) {
			logger.error("Adding descriptors after deploy is not supported.");
			return;
		}
		let uuid = ((typeof template.uuid) == "string") ?
			UUID.getByString(template.uuid) : template.uuid;
		let descriptor = new Descriptor(uuid, {
			readable: template.readable,
			writable: template.writable
		});
		if (template.hasOwnProperty("value")) {
			descriptor.value = template.value;
		}
		if (template.hasOwnProperty("requirements")) {
			descriptor.requirements = template.requirements;
		}
		if (template.hasOwnProperty("onValueWrite")) {
			descriptor.onValueWrite = template.onValueWrite;
		}
		if (template.hasOwnProperty("onValueRead")) {
			descriptor.onValueRead = template.onValueRead;
		}
		this._addDescriptor(descriptor);
		return descriptor;
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
	 * 4.10.1 Notifications
	 */
	notifyValue(connection, value) {
		let config = connection.bearer.readClientConfiguration(this, connection);
		if ((config & GATT.ClientConfiguration.NOTIFICATION) == 0) {
			logger.debug("Client is not configured for notification");
			return false;
		}
		if (value !== undefined) {
			this.value = value;
		}
		connection.bearer.sendPDU(ATT.assembleHandleValueNotificationPDU(this._valueAttribute.handle, this._serializeValue()));
		return true;
	}
	/**
	 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
	 * 4.11.1 Indications
	 */
	indicateValue(connection, value) {
		let config = connection.bearer.readClientConfiguration(this, connection);
		if ((config & GATT.ClientConfiguration.INDICATION) == 0) {
			logger.debug("Client is not configured for indication");
			return null;
		}
		if (value !== undefined) {
			this.value = value;
		}
		return new Promise((resolve, reject) => {
			connection.bearer.scheduleTransaction(
				ATT.assembleHandleValueIndicationPDU(this._valueAttribute.handle, this._serializeValue()),
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
	_serializeDefinition() {
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
	_deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(GATT.UUID_CHARACTERISTIC);
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Characteristic definition on read");
			return this._serializeDefinition();
		};
		/* Deploy value attribute */
		let valueAttribute = db.allocateAttribute(this._uuid);
		if ((this._properties & GATT.Properties.READ) > 0) {
			valueAttribute.callback.onRead = (attribute, context) => {
				if (this._onValueRead != null) {
					this._onValueRead(this, context.bearer.connection);
				}
				return this._serializeValue();
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
				this._parseValue(value);
				if (this._onValueWrite != null) {
					this._onValueWrite(this, context.bearer.connection);
				}
			};
		}
		if (this._requirements != null) {
			valueAttribute.requirements = this._requirements;
		}
		this._valueAttribute = valueAttribute;

		/* Deploy GATT defined format descriptors
		 * Unlike other descriptors, those descriptors are not visible
		 * to user.
		 */
		/* 3.3.3.1 Characteristic Extended Properties */
		if ((this._properties & GATT.Properties.EXT) > 0) {
			logger.debug("Deploy: Characteristic Extended Properties");
			let descriptor = new Descriptor(GATT.UUID_CHARACTERISTIC_EXTENDED_PROPERTIES, DESCRIPTOR_PERM_READ_ONLY);
			descriptor.value = this._extProperties;
			descriptor._deploy(db);
		}
		/* 3.3.3.2 Characteristic User Description */
		let writableAux = (this._extProperties & GATT.ExtendedProperties.WRITABLE_AUX) > 0;
		if (writableAux || this._userDescription != null) {
			logger.debug("Deploy: Characteristic User Description");
			let descriptor = new Descriptor(GATT.UUID_CHARACTERISTIC_USER_DESCRIPTION, {
				readable: this._configuration.description.readable,
				writable: writableAux
			});
			descriptor.onValueRead = (d, connection) => d.value = this._configuration.description.value;
			descriptor.onValueWrite = (d, connection) => this._configuration.description.value = d.value;
			descriptor.requirements = this._configuration.description.requirements;
			descriptor._deploy(db);
		}
		/* 3.3.3.3 Client Characteristic Configuration */
		if ((this._properties & GATT.Properties.NOTIFY) > 0 || (this._properties & GATT.Properties.INDICATE) > 0) {
			logger.debug("Deploy: Client Characteristic Configuration");
			let descriptor = new Descriptor(GATT.UUID_CLIENT_CHARACTERISTIC_CONFIGURATION, {
				readable: true,
				writable: this._configuration.client.writable
			});
			descriptor.onValueRead = (d, connection) =>
				d.value = connection.bearer.readClientConfiguration(this, connection);
			descriptor.onValueWrite = (d, connection) =>
				connection.bearer.writeClientConfiguration(this, connection, d.value);
			descriptor.requirements = this._configuration.client.requirements;
			descriptor._deploy(db);
		}
		/* 3.3.3.4 Server Characteristic Configuration */
		if ((this._properties & GATT.Properties.BROADCAST) > 0) {
			logger.debug("Deploy: Server Characteristic Configuration");
			let descriptor = new Descriptor(GATT.UUID_SERVER_CHARACTERISTIC_CONFIGURATION, {
				readable: true,
				writable: this._configuration.server.writable
			});
			descriptor.onValueRead = (d, connection) => d.value = this._configuration.server.value;
			descriptor.onValueWrite = (d, connection) => this._configuration.server.value = d.value;
			descriptor.requirements = this._configuration.server.requirements;
			descriptor._deploy(db);
		}
		/* 3.3.3.5 Characteristic Presentation Format */
		let formatHandles = new Array();
		for (let format of this._formats) {
			logger.debug("Deploy: Characteristic Presentation Format");
			let descriptor = new Descriptor(GATT.UUID_CHARACTERISTIC_PRESENTATION_FORMAT, DESCRIPTOR_PERM_READ_ONLY);
			descriptor.value = format;
			descriptor._deploy(db);
			formatHandles.push(descriptor._attribute.handle);
		}
		/* 3.3.3.6 Characteristic Aggregate Format */
		if (this._formats.length > 1) {
			logger.debug("Deploy: Characteristic Aggregate Format");
			let descriptor = new Descriptor(GATT.UUID_CHARACTERISTIC_AGGREGATE_FORMAT, DESCRIPTOR_PERM_READ_ONLY);
			descriptor.value = formatHandles;
			descriptor._deploy(db);
		}
		/* Deploy other descriptors */
		for (let descriptor of this.descriptors) {
			descriptor._deploy(db);
		}
		definitionAttribute.groupEnd = db.getEndHandle();	// Should fix it
		this._attribute = definitionAttribute;
	}
}
exports.Characteristic = Characteristic;

function toCharacteristicProperties(params) {
	var properties = 0;
	for (var i = 0; i < params.length; i++) {
		switch (params[i]) {
		case "broadcast":
			properties |= GATT.Properties.BROADCAST;
			break;
		case "read":
			properties |= GATT.Properties.READ;
			break;
		case "writeWithoutResponse":
			properties |= GATT.Properties.WRITE_WO_RESP;
			break;
		case "write":
			properties |= GATT.Properties.WRITE;
			break;
		case "notify":
			properties |= GATT.Properties.NOTIFY;
			break;
		case "indicate":
			properties |= GATT.Properties.INDICATE;
			break;
		}
	}
	return properties;
}

class Service extends GATT.Service {
	constructor(uuid, primary = true) {
		super(uuid, primary);
		/* ATT Contexts */
		this._attribute = null;
	}
	get start() {
		return (this._attribute != null) ? this._attribute.handle : 0;
	}
	get end() {
		return (this._attribute != null) ? this._attribute.groupEnd : 0;
	}
	addCharacteristic(template) {
		if (this._attribute != null) {
			logger.error("Adding characteristics after deploy is not supported.");
			return;
		}
		let properties = template.properties;
		if (properties instanceof Array) {
			properties = toCharacteristicProperties(properties);
		}
		let uuid = ((typeof template.uuid) == "string") ?
			UUID.getByString(template.uuid) : template.uuid;
		let extProperties = template.hasOwnProperty("extProperties") ?
			template.extProperties : 0x0000;
		let characteristic = new Characteristic(uuid, properties, extProperties);
		if (template.hasOwnProperty("value")) {
			characteristic.value = template.value;
		}
		if (template.hasOwnProperty("formats")) {
			characteristic.formats = template.formats;
		}
		if (template.hasOwnProperty("requirements")) {
			characteristic.requirements = template.requirements;
		}
		if (template.hasOwnProperty("onValueWrite")) {
			characteristic.onValueWrite = template.onValueWrite;
		}
		if (template.hasOwnProperty("onValueRead")) {
			characteristic.onValueRead = template.onValueRead;
		}
		if (template.hasOwnProperty("configuration")) {
			characteristic.configuration = template.configuration;
		}
		if (template.hasOwnProperty("descriptors")) {
			for (let templateDesc of template.descriptors) {
				characteristic.addDescriptor(templateDesc);
			}
		}
		this._addCharacteristic(characteristic);
		return characteristic;
	}
	_serializeDefinition() {
		if (this._uuid.isUUID16()) {
			return Utils.toByteArray(this._uuid.toUUID16(), 2, true);
		}
		return this._uuid.getRawArray();
	}
	_serializeIncludeDefinition() {
		let buffer = ByteBuffer.allocate(6, true);
		buffer.putInt16(this._attribute.handle);
		buffer.putInt16(this._attribute.groupEnd);
		if (this.uuid.isUUID16()) {
			buffer.putInt16(this.uuid.toUUID16());
		}
		buffer.flip();
		return buffer.getByteArray();
	}
	_deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(
			this._primary ? GATT.UUID_PRIMARY_SERVICE : GATT.UUID_SECONDARY_SERVICE);
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Service definition on read");
			return this._serializeDefinition();
		};
		/* Deploy includes */
		for (let include of this.includes) {
			let includeAttribute = db.allocateAttribute(GATT.UUID_INCLUDE);
			includeAttribute.callback.onRead = (attribute) => {
				logger.debug("Include definition on read");
				return include._serializeIncludeDefinition();
			};
		}
		/* Deploy characteristics */
		for (let characteristic of this.characteristics) {
			characteristic._deploy(db);
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
	addService(template) {
		let service = new Service(
			((typeof template.uuid) == "string") ?
				UUID.getByString(template.uuid) : template.uuid,
			template.primary);
		if (template.hasOwnProperty("characteristics")) {
			for (let templateChar of template.characteristics) {
				service.addCharacteristic(templateChar);
			}
		}
		this._addService(service);
		return service;
	}
	deploy() {
		/* Add all Services */
		for (let service of this.services) {
			service._deploy(this._database);
		}
		/* Generate handles */
		this._database.assignHandles();
	}
}
exports.Profile = Profile;