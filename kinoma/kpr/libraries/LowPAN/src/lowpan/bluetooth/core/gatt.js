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
 * Bluetooth v4.2 - Generic Attribute Profile (GATT)
 */

var Utils = require("../../common/utils");
var Logger = Utils.Logger;
var Buffers = require("../../common/buffers");
var ByteBuffer = Buffers.ByteBuffer;

var BTUtils = require("./btutils");
var UUID = BTUtils.UUID;

var logger = new Logger("GATT");
logger.loggingLevel = Utils.Logger.Level.INFO;

exports.setLoggingLevel = level => logger.loggingLevel = level;

const UUID_PRIMARY_SERVICE	= UUID.getByUUID16(0x2800);
const UUID_SECONDARY_SERVICE	= UUID.getByUUID16(0x2801);
const UUID_INCLUDE			= UUID.getByUUID16(0x2802);
const UUID_CHARACTERISTIC		= UUID.getByUUID16(0x2803);
const UUID_CHARACTERISTIC_EXTENDED_PROPERTIES		= UUID.getByUUID16(0x2900);
const UUID_CHARACTERISTIC_USER_DESCRIPTION		= UUID.getByUUID16(0x2901);
const UUID_CLIENT_CHARACTERISTIC_CONFIGURATION	= UUID.getByUUID16(0x2902);
const UUID_SERVER_CHARACTERISTIC_CONFIGURATION	= UUID.getByUUID16(0x2903);
const UUID_CHARACTERISTIC_PRESENTATION_FORMAT		= UUID.getByUUID16(0x2904);
const UUID_CHARACTERISTIC_AGGREGATE_FORMAT		= UUID.getByUUID16(0x2905);

const NAMESPACE_BLUETOOTH_SIG = 0x01;

const Properties = {
	BROADCAST: 0x01,
	READ: 0x02,
	WRITE_WO_RESP: 0x04,
	WRITE: 0x08,
	NOTIFY: 0x10,
	INDICATE: 0x20,
	AUTH_WRITE: 0x40,
	EXT: 0x80
};
exports.Properties = Properties;

const ExtendedProperties = {
	RELIABLE_WRITE: 0x0001,
	WRITABLE_AUX: 0x0002
};
exports.ExtendedProperties = ExtendedProperties;

const ClientConfiguration = {
	NOTIFICATION: 0x0001,
	INDICATION: 0x0002
};
exports.ClientConfiguration = ClientConfiguration;

var SUPPORTED_GROUPS = [UUID_PRIMARY_SERVICE, UUID_SECONDARY_SERVICE];

var ATT = require("./att");
ATT.setSupportedGroups(SUPPORTED_GROUPS);
exports.ATT = ATT;

exports.createReadOnlyStringCharacteristic = function (uuid, defaultValue = null) {
	let characteristic = new Characteristic(uuid, Properties.READ);
	characteristic.addFormat({format: Format.UTF8S});
	if (defaultValue != null) {
		characteristic.value = defaultValue;
	}
	return characteristic;
};

function serializeInt32Data(octets, data) {
	return Utils.toByteArray(data, octets, true);
}

function parseInt32Value(octets, signed, bit, buffer) {
	let data = Utils.toInt(buffer.getByteArray(octets), 0, octets, true);
	if (bit == 32) {
		return signed ? data : data >>> 0;
	}
	if (signed && ((data >> (bit - 1)) > 0)) {
		data = (-1 << bit) | data;
	}
	return data;
}

function serializeString(data) {
	let value = [];
	for (let i = 0; i < data.length; i++) {
		value.push(data.charCodeAt(i));
	}
	return value;
}

function parseString(buffer) {
	let str = "";
	let value = buffer.getByteArray();
	for (val of value) {
		str = str + String.fromCharCode(val);
	}
	return str;
}

const Format = {
	BOOLEAN: 0x01,
	UINT2: 0x02,
	UINT4: 0x03,
	UINT8: 0x04,
	UINT12: 0x05,
	UINT16: 0x06,
	UINT24: 0x07,
	UINT32: 0x08,
	UINT48: 0x09,
	UINT64: 0x0A,
	UINT128: 0x0B,
	SINT8: 0x0C,
	SINT12: 0x0D,
	SINT16: 0x0E,
	SINT24: 0x0F,
	SINT32: 0x10,
	SINT48: 0x11,
	SINT64: 0x12,
	SINT128: 0x13,
	FLOAT32: 0x14,
	FLOAT64: 0x15,
	SFLOAT: 0x16,
	FLOAT: 0x17,
	DUINT16: 0x18,
	UTF8S: 0x19,
	UTF16S: 0x1A,
	STRUCT: 0x1B
};
exports.Format = Format;

const FormatSerializer = {
	[Format.BOOLEAN]: (data) => {return [data ? 1 : 0];},
	[Format.UINT2]: serializeInt32Data.bind(null, 1),
	[Format.UINT4]: serializeInt32Data.bind(null, 1),
	[Format.UINT8]: serializeInt32Data.bind(null, 1),
	[Format.UINT12]: serializeInt32Data.bind(null, 2),
	[Format.UINT16]: serializeInt32Data.bind(null, 2),
	[Format.UINT24]: serializeInt32Data.bind(null, 3),
	[Format.UINT32]: serializeInt32Data.bind(null, 4),
	[Format.UINT48]: (data) => {return new Uint8Array(data.slice(0, 6));},
	[Format.UINT64]: (data) => {return new Uint8Array(data.slice(0, 8));},
	[Format.SINT8]: serializeInt32Data.bind(null, 1),
	[Format.SINT12]: serializeInt32Data.bind(null, 2),
	[Format.SINT16]: serializeInt32Data.bind(null, 2),
	[Format.SINT24]: serializeInt32Data.bind(null, 3),
	[Format.SINT32]: serializeInt32Data.bind(null, 4),
	[Format.SINT48]: (data) => {return new Uint8Array(data.slice(0, 6));},
	[Format.SINT64]: (data) => {return new Uint8Array(data.slice(0, 8));},
	[Format.UTF8S]: serializeString,
	[Format.UTF16S]: serializeString		// FIXME
};

const FormatParser = {
	[Format.BOOLEAN]: (buffer) => {return buffer.getInt8() == 1;},
	[Format.UINT2]: (buffer) => {return buffer.getInt8() & 0x03;},
	[Format.UINT4]: (buffer) => {return buffer.getInt8() & 0x0F;},
	[Format.UINT8]: (buffer) => {return buffer.getInt8() & 0xFF;},
	[Format.UINT12]: parseInt32Value.bind(null, 2, false, 12),
	[Format.UINT16]: parseInt32Value.bind(null, 2, false, 16),
	[Format.UINT24]: parseInt32Value.bind(null, 3, false, 24),
	[Format.UINT32]: parseInt32Value.bind(null, 4, false, 32),
	[Format.UINT48]: (buffer) => {return buffer.getByteArray(6)},
	[Format.UINT64]: (buffer) => {return buffer.getByteArray(8)},
	[Format.SINT8]: parseInt32Value.bind(null, 1, true, 8),
	[Format.SINT12]: parseInt32Value.bind(null, 2, true, 12),
	[Format.SINT16]: parseInt32Value.bind(null, 2, true, 16),
	[Format.SINT24]: parseInt32Value.bind(null, 3, true, 24),
	[Format.SINT32]: parseInt32Value.bind(null, 4, true, 32),
	[Format.SINT48]: (buffer) => {return buffer.getByteArray(6)},
	[Format.SINT64]: (buffer) => {return buffer.getByteArray(8)},
	[Format.UTF8S]: parseString,
	[Format.UTF16S]: parseString		// FIXME
};

const FormatNames = {
	["boolean"]: Format.BOOLEAN,
	["2bit"]: Format.UINT2,
	["nibble"]: Format.UINT4,
	["uint8"]: Format.UINT8,
	["uint12"]: Format.UINT12,
	["uint16"]: Format.UINT16,
	["uint24"]: Format.UINT24,
	["uint32"]: Format.UINT32,
	["uint48"]: Format.UINT48,
	["uint64"]: Format.UINT64,
	["sint8"]: Format.SINT8,
	["sint12"]: Format.SINT12,
	["sint16"]: Format.SINT16,
	["sint24"]: Format.SINT24,
	["sint32"]: Format.SINT32,
	["sint48"]: Format.SINT48,
	["sint64"]: Format.SINT64,
	["float32"]: Format.FLOAT32,
	["float64"]: Format.FLOAT64,
	["SFLOAT"]: Format.SFLOAT,
	["FLOAT"]: Format.FLOAT,
	["duint16"]: Format.DUINT16,
	["utf8s"]: Format.UTF8S,
	["utf16s"]: Format.UTF16S,
	["struct"]: Format.STRUCT
};

exports.getFormatByName = function (name) {
	if (FormatNames.hasOwnProperty(name)) {
		return FormatNames[name];
	}
	return -1;
};

class Characteristic {
	constructor(uuid, properties, extProperties = 0x0000) {
		this._uuid = uuid;
		this._properties = properties;
		if ((this._properties & Properties.EXT) > 0) {
			this._extProperties = extProperties;
		} else {
			this._extProperties = 0x0000;
		}
		this._description = null;
		this._clientConfigurations = {};
		this._formats = [];
		/* ATT Contexts */
	//	this._attribute = null;
		this._valueAttribute = null;
	//	this._descriptorAttributes = null;

		this._localValue = null;
		this._onValueRead = null;
		this._onValueWrite = null;
	}
	get uuid() {
		return this._uuid;
	}
	get properties() {
		return this._properties;
	}
	get handle() {
		if (this._valueAttribute == null) {
			return ATT.INVALID_HANDLE;
		}
		return this._valueAttribute.handle;
	}
	set value(value) {
		this._localValue = value;
	}
	get value() {
		return this._localValue;
	}
	set desription(desription) {
		this._description = description;
	}
	set onValueRead(onValueRead) {
		this._onValueRead = onValueRead;
	}
	set onValueWrite(onValueWrite) {
		this._onValueWrite = onValueWrite;
	}
	_readDefinition() {
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
	_readValue() {
		if (this._formats.length > 0) {
			if (this._formats.length == 1) {
				return FormatSerializer[this._formats[0].format](this._localValue);
			}
			let value = [];
			for (let format of this._formats) {
				value.concat(FormatSerializer[format.format](this._localValue[0]));
			}
			return value;
		}
		return this._localValue;
	}
	_writeValue(value) {
		let buffer = ByteBuffer.wrap(value);
		if (this._formats.length > 0) {
			if (this._formats.length == 1) {
				this._localValue = FormatParser[this._formats[0].format](buffer);
				return;
			}
			let data = [];
			for (let format of this._formats) {
				data.concat(FormatParser[format.format](buffer));
			}
			this._localValue = data;
			return;
		}
		this._localValue = buffer.getByteArray();
	}
	hasExtendedProperties() {
		return (this._properties & Properties.EXT) > 0;
	}
	_readExtendedProperties() {
		return Utils.toByteArray(this._extProperties, 2, true);
	}
	_readUserDescription() {
		if (this._description != null) {
			return BTUtils.toCharArray(this._description);
		}
	}
	hasClientConfiguration() {
		return (this._properties & Properties.NOTIFY) > 0 || (this._properties & Properties.INDICATE) > 0;
	}
	getClientConfiguration(bearer) {
		let linkHandle = bearer.getHCILinkHandle();
		if (!this._clientConfigurations.hasOwnProperty(linkHandle)) {
			this._clientConfigurations[linkHandle] = 0x0000;
		}
		return this._clientConfigurations[linkHandle];
	}
	_readClientConfiguration(bearer) {
		return Utils.toByteArray(this.getClientConfiguration(bearer), 2, true);
	}
	_writeClientConfiguration(bearer, value) {
		if (!this.hasClientConfiguration()) {
			logger.debug("Characteristic has no client configuration");
			return;
		}
		let linkHandle = bearer.getHCILinkHandle();
		let config = 0x0000;
		let configToWrite = Utils.toInt16(value, true);
		if ((this._properties & Properties.NOTIFY) > 0 && (configToWrite & ClientConfiguration.NOTIFICATION) > 0) {
			logger.debug("Config notification for link=" + Utils.toHexString(linkHandle, 2));
			config |= ClientConfiguration.NOTIFICATION;
		}
		if ((this._properties & Properties.INDICATE) > 0 && (configToWrite & ClientConfiguration.INDICATION) > 0) {
			logger.debug("Config indication for link=" + Utils.toHexString(linkHandle, 2));
			config |= ClientConfiguration.INDICATION;
		}
		this._clientConfigurations[linkHandle] = config;
	}
	addFormat(format) {
		this._formats.push(format);
	}
	deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(UUID_CHARACTERISTIC);
		definitionAttribute.permission.readable = true;
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Characteristic definition on read");
			attribute.value = this._readDefinition();
		};
		/* Deploy value attribute */
		let valueAttribute = db.allocateAttribute(this._uuid);
		if ((this._properties & Properties.READ) > 0) {
			valueAttribute.permission.readable = true;
		}
		if ((this._properties & Properties.WRITE) > 0) {
			valueAttribute.permission.writable = true;
		}
		if ((this._properties & Properties.WRITE_WO_RESP) > 0) {
			valueAttribute.permission.commandable = true;
		}
		if (this._defaultValue != null) {
			valueAttribute.value = this._defaultValue;
		}
		valueAttribute.callback.onRead = (attribute) => {
			if (this._onValueRead != null) {
				this._onValueRead(this);
			}
			attribute.value = this._readValue();
		};
		valueAttribute.callback.onWrite = (attribute, value) => {
			this._writeValue(value);
			if (this._onValueWrite != null) {
				this._onValueWrite(this, this._localValue);
			}
		};
		this._valueAttribute = valueAttribute;
		/* Deploy descriptors */
		/* 3.3.3.1 Characteristic Extended Properties */
		if (this.hasExtendedProperties()) {
			logger.debug("Deploy: Characteristic Extended Properties");
			let descriptorAttribute = db.allocateAttribute(UUID_CHARACTERISTIC_EXTENDED_PROPERTIES);
			descriptorAttribute.permission.readable = true;
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic ext properties on read");
				attribute.value = this._readExtendedProperties();
			};
		}
		/* 3.3.3.2 Characteristic User Description */
		let writableAux = (this._extProperties & ExtendedProperties.WRITABLE_AUX) > 0;
		if (this._description != null || writableAux) {
			logger.debug("Deploy: Characteristic User Description");
			let descriptorAttribute = db.allocateAttribute(UUID_CHARACTERISTIC_USER_DESCRIPTION);
			descriptorAttribute.permission.readable = true;
			descriptorAttribute.permission.writable = writableAux;
			descriptorAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic user description on read");
				attribute.value = this._readUserDescription();
			};
		}
		/* 3.3.3.3 Client Characteristic Configuration */
		if (this.hasClientConfiguration()) {
			logger.debug("Deploy: Client Characteristic Configuration");
			let descriptorAttribute = db.allocateAttribute(UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);
			descriptorAttribute.permission.readable = true;
			descriptorAttribute.permission.writable = true;
			descriptorAttribute.permission.authentication = true;
			descriptorAttribute.permission.authorization = true;
			descriptorAttribute.callback.onRead = (attribute, context = null) => {
				logger.debug("Characteristic client config on read");
				if (context == null) {
					logger.error("ATT context not found while reading");
					return;
				}
				attribute.value = this._readClientConfiguration(context.bearer);
			};
			descriptorAttribute.callback.onWrite = (attribute, value, context = null) => {
				logger.debug("Characteristic client config on write");
				if (context == null) {
					logger.error("ATT context not found while writing");
					return;
				}
				this._writeClientConfiguration(context.bearer, value);
			};
		}
		/* 3.3.3.4 Server Characteristic Configuration */
		// TODO
		/* 3.3.3.5 Characteristic Presentation Format */
		for (let format of this._formats) {
			logger.debug("Deploy: Characteristic Presentation Format");
			let formatAttribute = db.allocateAttribute(UUID_CHARACTERISTIC_PRESENTATION_FORMAT);
			formatAttribute.permission.readable = true;
			/* Read Only */
			let buffer = ByteBuffer.allocate(7);
			buffer.putInt8(format.format);
			buffer.putInt8(format.hasOwnProperty("exponent") ? format.exponent : 0);
			buffer.putInt16(format.hasOwnProperty("unit") ? format.unit.toUUID16() : 0);
			buffer.putInt8(format.hasOwnProperty("nameSpace") ? format.nameSpace : NAMESPACE_BLUETOOTH_SIG);
			buffer.putInt16(format.hasOwnProperty("description") ? format.description : 0);
			buffer.flip();
			formatAttribute.value = buffer.getByteArray();
			format._attribute = formatAttribute;
		}
		/* 3.3.3.6 Characteristic Aggregate Format */
		if (this._formats.length > 1) {
			logger.debug("Deploy: Characteristic Presentation Format");
			let aggregateAttribute = db.allocateAttribute(UUID_CHARACTERISTIC_AGGREGATE_FORMAT);
			aggregateAttribute.permission.readable = true;
			aggregateAttribute.callback.onRead = (attribute) => {
				logger.debug("Characteristic Presentation Format on read");
				let buffer = ByteBuffer.allocate(this._formats.length * 2);
				for (let format of this._formats) {
					buffer.putInt16(format._attribute.handle);
				}
				buffer.flip();
				return buffer.getByteArray();
			};
		}
		definitionAttribute.groupEnd = db.getEndHandle();	// Should fix it
	//	this._attribute = definitionAttribute;
	}
}
exports.Characteristic = Characteristic;

class Include {
	constructor(uuid) {
		this._uuid = uuid;
		/* ATT Contexts */
		this._attribute = null;
	}
	_readDefinition() {
		let candidate = searchCandidate();	// TODO
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
		let definitionAttribute = db.allocateAttribute(UUID_INCLUDE);
		definitionAttribute.permission.readable = true;
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Include definition on read");
			attribute.value = this._readDefinition();
		};
		this._attribute = definitionAttribute;
	}
}
exports.Include = Include;

class Service {
	constructor(uuid, primary = true) {
		this._uuid = uuid;
		this._primary = primary;
		this._includes = [];
		this._characteristics = [];
		/* ATT Contexts */
		this._attribute = null;
	}
	get uuid() {
		return this._uuid;
	}
	get characteristics() {
		return this._characteristics.slice();
	}
	getCharacteristicByUUID(uuid) {
		for (let characteristic of this._characteristics) {
			if (characteristic.uuid.equals(uuid)) {
				return characteristic;
			}
		}
		return null;
	}
	addInclude(include) {
		if (this._attribute != null) {
			throw "Service has already been deployed";
		}
		this._includes.push(include);
	}
	addCharacteristic(characteristic) {
		if (this._attribute != null) {
			throw "Service has already been deployed";
		}
		this._characteristics.push(characteristic);
	}
	_readDefinition() {
		if (this._uuid.isUUID16()) {
			return Utils.toByteArray(this._uuid.toUUID16(), 2, true);
		}
		return this._uuid.getRawArray();
	}
	deploy(db) {
		/* Deploy definition attribute */
		let definitionAttribute = db.allocateAttribute(
			this._primary ? UUID_PRIMARY_SERVICE : UUID_SECONDARY_SERVICE);
		definitionAttribute.permission.readable = true;
		definitionAttribute.callback.onRead = (attribute) => {
			logger.debug("Service definition on read");
			attribute.value = this._readDefinition();
		};
		/* Deploy includes */
		for (let include of this._includes) {
			include.deploy(db);
		}
		/* Deploy characteristics */
		for (let characteristic of this._characteristics) {
			characteristic.deploy(db);
		}
		definitionAttribute.groupEnd = db.getEndHandle();	// Should fix it
		this._attribute = definitionAttribute;
	}
}
exports.Service = Service;

class Profile {
	constructor() {
		this._services = [];
	}
	get services() {
		return this._services.slice();
	}
	getServiceByUUID(uuid) {
		for (let service of this._services) {
			if (service.uuid.equals(uuid)) {
				return service;
			}
		}
		return null;
	}
	deployServices(db, services) {
		let start = db.getEndHandle() + 1;
		/* Add all Services */
		for (let service of services) {
			service.deploy(db);
			this._services.push(service);
		}
		/* Generate handles */
		db.assignHandles();

		let end = db.getEndHandle();
		logger.info("Service Changed(Added): start=" + Utils.toHexString(start, 2)
			+ ", end=" + Utils.toHexString(end, 2));

		return {
			start,
			end
		};
	}
}
exports.Profile = Profile;

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.3.1 Exchange MTU
 */
exports.exchangeMTU = function (bearer, mtu) {
	if (mtu === undefined) {
		mtu = bearer.mtu;
	}
	bearer.scheduleTransaction(
		ATT.assembleExchangeMTURequestPDU(mtu),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.EXCHANGE_MTU_RESPONSE) {
					return;
				}
				var serverMTU = response.mtu;
				if (serverMTU < bearer.mtu) {
					bearer.mtu = serverMTU;
				} else {
					bearer.mtu = mtu;
				}
				logger.info("ATT Bearer MTU has been changed to: " + bearer.mtu);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				logger.error("Exchange MTU Response: Error");
				bearer.mtu = ATT.ATT_MTU;	// Use default MTU
			}
		}
	);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.4.1 Discovery All Primary Serices
 */
exports.discoverAllPrimaryServices = function (bearer, callback) {
	startPrimaryServiceDiscovery(bearer, null, callback);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.4.2 Discovery Primary Serice by Service UUID
 */
exports.discoverPrimaryServiceByServiceUUID = function (bearer, uuid, callback) {
	startPrimaryServiceDiscovery(bearer, uuid, callback);
};

function startPrimaryServiceDiscovery(bearer, uuid, callback, start, end) {
	if (start === undefined) {
		start = 0x0001;
	}
	if (end === undefined) {
		end = 0xFFFF;
	}
	logger.debug("Start Primary Service Discovery: start=" + Utils.toHexString(start, 2)
		+ " end=" + Utils.toHexString(end, 2));
	var discoverAll = (uuid == null);
	var pdu;
	if (discoverAll) {
		pdu = ATT.assembleReadByGroupTypeRequestPDU(start, end,
			UUID_PRIMARY_SERVICE);
	} else {
		pdu = ATT.assembleFindByTypeValueRequestPDU(start, end,
			UUID_PRIMARY_SERVICE, uuid.getRawArray());
	}
	bearer.scheduleTransaction(pdu, {
		transactionCompleteWithResponse: function (opcode, response) {
			if (opcode != (discoverAll
								? ATT.Opcode.READ_BY_GROUP_TYPE_RESPONSE
								: ATT.Opcode.FIND_BY_TYPE_VALUE_RESPONSE)) {
				return;
			}
			var services = [];
			for (var i = 0; i < response.length; i++) {
				var attribute = response[i];
				services.push({
					primary: true,
					uuid: (discoverAll ? UUID.getByUUID(attribute.value) : uuid),
					start: attribute.handle,
					end: attribute.groupEnd
				});
			}
			callback.primaryServicesDiscovered(services);
			var lastHandle = response[response.length - 1].groupEnd;
			if (lastHandle == 0xFFFF) {
				logger.debug("SD complete with max handle");
				callback.procedureComplete(lastHandle);
			} else {
				if (discoverAll) {
					startPrimaryServiceDiscovery(bearer, null, callback, lastHandle + 1, end);
				} else {
					startPrimaryServiceDiscovery(bearer, uuid, callback, lastHandle + 1, end);
				}
			}
		},
		transactionCompleteWithError: function (errorCode, handle) {
			if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
				logger.debug("SD complete with attribute not found");
				callback.procedureComplete(handle);
			} else {
				logger.debug("SD complete with error=" + Utils.toHexString(errorCode));
				callback.procedureCompleteWithError(errorCode, handle);
			}
		}
	});
}

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.5.1 Find Included Services
 */
exports.findIncludedServices = function (bearer, start, end, callback) {
	startRelationShipDiscovery(bearer, start, end, callback);
};

function startRelationShipDiscovery(bearer, start, end, callback) {
	bearer.scheduleTransaction(
		ATT.assembleReadByTypeRequestPDU(start, end, UUID_INCLUDE),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
					return;
				}
				var includes = [];
				for (var i = 0; i < response.length; i++) {
					var attribute = response[i];
					var include = {
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
							transactionCompleteWithResponse: function (opcode, response) {
								if (opcode != ATT.Opcode.READ_REQUEST) {
									return;
								}
								include.uuid = UUID.getByUUID(response);
								callback.includedServicesDiscovered([include]);	// TODO: Early termination
							},
							transactionCompleteWithError: function (requestOpcode, handle, errorCode) {
								// TODO
							}
						});
					}
				}
				if (!callback.includedServicesDiscovered(includes)) {
					return;	// Terminate the sub-procedure
				}
				var lastHandle = response[response.length - 1].handle;
				if (lastHandle == end) {
					callback.procedureComplete(lastHandle);
				} else {
					startRelationShipDiscovery(bearer, lastHandle + 1, end, callback);
				}
			},
			transactionCompleteWithError: function (errorCode, handle) {
				if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
					callback.procedureComplete(handle);
				} else {
					callback.procedureCompleteWithError(errorCode, handle);
				}
			}
		}
	);
}

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.6.1 Discover All Characteristics of a Service
 */
exports.discoverAllCharacteristics = function (bearer, start, end, callback) {
	startCharacteristicDiscovery(bearer, start, end, null, callback);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.6.2 Discover Characteristics by UUID
 */
exports.discoverCharacteristicsByUUID = function (bearer, start, end, uuid, callback) {
	startCharacteristicDiscovery(bearer, start, end, uuid, callback);
};

function startCharacteristicDiscovery(bearer, start, end, uuid, callback) {
	bearer.scheduleTransaction(
		ATT.assembleReadByTypeRequestPDU(start, end, UUID_CHARACTERISTIC),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
					return;
				}
				var characteristics = [];
				for (var i = 0; i < response.length; i++) {
					var attribute = response[i];
					var characteristic = {
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
				if (!callback.characteristicsDiscovered(characteristics)) {
					return;	// Terminate the sub-procedure
				}
				var lastHandle = response[response.length - 1].handle;
				if (lastHandle == end) {
					callback.procedureComplete(lastHandle);
				} else {
					startCharacteristicDiscovery(bearer, lastHandle + 1, end, uuid, callback);
				}
			},
			transactionCompleteWithError: function (errorCode, handle) {
				if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
					callback.procedureComplete(handle);
				} else {
					callback.procedureCompleteWithError(errorCode, handle);
				}
			}
		}
	);
}

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.7.1 Discover All Characteristic Descriptors
 */
exports.discoverAllCharacteristicDescriptors = function (bearer, start, end, callback) {
	startCharacteristicDescriptorDiscovery(bearer, start, end, callback);
};

function startCharacteristicDescriptorDiscovery(bearer, start, end, callback) {
	logger.debug("Start Characteristic Descriptor Discovery: start=" + Utils.toHexString(start, 2)
		+ ", end=" + Utils.toHexString(end, 2));
	bearer.scheduleTransaction(
		ATT.assembleFindInformationRequestPDU(start, end),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.FIND_INFORMATION_RESPONSE) {
					return;
				}
				var descriptors = [];
				for (var i = 0; i < response.length; i++) {
					var attribute = response[i];
					descriptors.push({
						handle: attribute.handle,
						uuid: attribute.type
					});
				}
				if (!callback.characteristicDescriptorsDiscovered(descriptors)) {
					return;	// Terminate the sub-procedure
				}
				var lastHandle = response[response.length - 1].handle;
				if (lastHandle == end) {
					logger.debug("CDD complete with end handle=" + Utils.toHexString(end, 2));
					callback.procedureComplete(lastHandle);
				} else {
					startCharacteristicDescriptorDiscovery(bearer, lastHandle + 1, end, callback);
				}
			},
			transactionCompleteWithError: function (errorCode, handle) {
				if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
					logger.debug("CDD complete with attribute not found");
					callback.procedureComplete(handle);
				} else {
					logger.debug("CDD complete with error=" + Utils.toHexString(errorCode));
					callback.procedureCompleteWithError(errorCode, handle);
				}
			}
		}
	);
}

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Characteristic Value
 */
exports.readCharacteristicValue = function (bearer, handle, callback) {
	bearer.scheduleTransaction(
		ATT.assembleReadRequestPDU(handle),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.READ_RESPONSE) {
					return;
				}
				callback.procedureComplete(handle, response);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				callback.procedureCompleteWithError(errorCode, handle);
			}
		}
	);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Using Characteristic UUID
 */
exports.readUsingCharacteristicUUID = function (bearer, start, end, uuid, callback) {
	bearer.scheduleTransaction(
		ATT.assembleReadByTypeRequestPDU(start, end, uuid),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.READ_BY_TYPE_RESPONSE) {
					return;
				}
				/* XXX: How should we handle if we have multiple response? */
				var attribute = response[0];
				callback.procedureComplete(attribute.handle, attribute.value);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				if (errorCode == ATT.ErrorCode.ATTRIBUTE_NOT_FOUND) {
					callback.procedureComplete(handle, null);
				} else {
					callback.procedureCompleteWithError(errorCode, handle);
				}
			}
		}
	);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.3 Read Long Characteristic Value
 */
exports.readLongCharacteristicValues = function (bearer, handle, length, callback) {
	var value = [];
	var cb = {
		transactionCompleteWithResponse: function (opcode, response) {
			if (opcode != ATT.Opcode.READ_BLOB_RESPONSE) {
				return;
			}
			value = value.concat(response);
			if (value.length >= length) {
				callback.procedureComplete(handle, value);
			} else {
				bearer.scheduleTransaction(ATT.assembleReadBlobRequestPDU(handle, value.length), cb);
			}
		},
		transactionCompleteWithError: function (errorCode, handle) {
			callback.procedureCompleteWithError(errorCode, handle);
		}
	};
	bearer.scheduleTransaction(ATT.assembleReadBlobRequestPDU(handle, value.length), cb);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.8.1 Read Multiple Characteristic Values
 */
exports.readMultipleCharacteristicValues = function (bearer, handles, callback) {
	bearer.scheduleTransaction(
		ATT.assembleReadMultipleRequestPDU(handles),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.READ_MULTIPLE_RESPONSE) {
					return;
				}
				callback.procedureComplete(handle, response);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				callback.procedureCompleteWithError(errorCode, handle);
			}
		}
	);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.9.1 Write Without Response
 */
exports.writeWithoutResponse = function (bearer, handle, data) {
	bearer.sendPDU(ATT.assembleWriteCommandPDU(handle, data));
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.9.3 Write Characteristic Value
 */
exports.writeCharacteristicValue = function (bearer, handle, data, callback) {
	bearer.scheduleTransaction(
		ATT.assembleWriteRequestPDU(handle, data),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.WRITE_RESPONSE) {
					return;
				}
				callback.procedureComplete(handle, response);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				callback.procedureCompleteWithError(errorCode, handle);
			}
		}
	);
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.10.1 Notifications
 */
exports.notifyValue = function (bearer, handle, data) {
	bearer.sendPDU(ATT.assembleHandleValueNotificationPDU(handle, data));
};

/**
 * Core 4.2 Specification, Vol 3, Part G: Generic Attribute Profile
 * 4.11.1 Indications
 */
exports.indicateValue = function (bearer, handle, data, callback) {
	bearer.scheduleTransaction(
		ATT.assembleHandleValueIndicationPDU(handle, data),
		{
			transactionCompleteWithResponse: function (opcode, response) {
				if (opcode != ATT.Opcode.HANDLE_VALUE_CONFIRMATION) {
					return;
				}
				callback.procedureComplete(handle, response);
			},
			transactionCompleteWithError: function (errorCode, handle) {
				callback.procedureCompleteWithError(errorCode, handle);
			}
		}
	);
};
