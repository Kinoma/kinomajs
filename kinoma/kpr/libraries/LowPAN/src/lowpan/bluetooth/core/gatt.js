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

var logger = Logger.getLogger("GATT");

const UUID_PRIMARY_SERVICE						= UUID.getByUUID16(0x2800);
const UUID_SECONDARY_SERVICE					= UUID.getByUUID16(0x2801);
const UUID_INCLUDE								= UUID.getByUUID16(0x2802);
const UUID_CHARACTERISTIC						= UUID.getByUUID16(0x2803);
const UUID_CHARACTERISTIC_EXTENDED_PROPERTIES	= UUID.getByUUID16(0x2900);
const UUID_CHARACTERISTIC_USER_DESCRIPTION		= UUID.getByUUID16(0x2901);
const UUID_CLIENT_CHARACTERISTIC_CONFIGURATION	= UUID.getByUUID16(0x2902);
const UUID_SERVER_CHARACTERISTIC_CONFIGURATION	= UUID.getByUUID16(0x2903);
const UUID_CHARACTERISTIC_PRESENTATION_FORMAT	= UUID.getByUUID16(0x2904);
const UUID_CHARACTERISTIC_AGGREGATE_FORMAT		= UUID.getByUUID16(0x2905);
exports.UUID_PRIMARY_SERVICE = UUID_PRIMARY_SERVICE;
exports.UUID_SECONDARY_SERVICE = UUID_SECONDARY_SERVICE;
exports.UUID_INCLUDE = UUID_INCLUDE;
exports.UUID_CHARACTERISTIC = UUID_CHARACTERISTIC;
exports.UUID_CHARACTERISTIC_EXTENDED_PROPERTIES = UUID_CHARACTERISTIC_EXTENDED_PROPERTIES;
exports.UUID_CHARACTERISTIC_USER_DESCRIPTION = UUID_CHARACTERISTIC_USER_DESCRIPTION;
exports.UUID_CLIENT_CHARACTERISTIC_CONFIGURATION = UUID_CLIENT_CHARACTERISTIC_CONFIGURATION;
exports.UUID_SERVER_CHARACTERISTIC_CONFIGURATION = UUID_SERVER_CHARACTERISTIC_CONFIGURATION;
exports.UUID_CHARACTERISTIC_PRESENTATION_FORMAT = UUID_CHARACTERISTIC_PRESENTATION_FORMAT;
exports.UUID_CHARACTERISTIC_AGGREGATE_FORMAT = UUID_CHARACTERISTIC_AGGREGATE_FORMAT;

const NAMESPACE_BLUETOOTH_SIG = 0x01;
exports.NAMESPACE_BLUETOOTH_SIG = NAMESPACE_BLUETOOTH_SIG;

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

function getFormatByName(name) {
	if (FormatNames.hasOwnProperty(name)) {
		return FormatNames[name];
	}
	return -1;
};

class Descriptor {
	constructor(uuid) {
		this._uuid = uuid;
		this._localValue = null;
		this._serializer = null;
		this._parser = null;
		/* Known Descriptors */
		if (uuid.equals(UUID_CHARACTERISTIC_EXTENDED_PROPERTIES) ||
			uuid.equals(UUID_CLIENT_CHARACTERISTIC_CONFIGURATION) ||
			uuid.equals(UUID_SERVER_CHARACTERISTIC_CONFIGURATION)) {
			this._serializer = FormatSerializer[Format.UINT16];
			this._parser = FormatParser[Format.UINT16];
		} else if (uuid.equals(UUID_CHARACTERISTIC_USER_DESCRIPTION)) {
			this._serializer = FormatSerializer[Format.UTF8S];
			this._parser = FormatParser[Format.UTF8S];
		} else if (uuid.equals(UUID_CHARACTERISTIC_PRESENTATION_FORMAT)) {
			this._serializer = format => {
				let buffer = ByteBuffer.allocate(7);
				buffer.putInt8(format.format);
				buffer.putInt8(format.hasOwnProperty("exponent") ? format.exponent : 0);
				buffer.putInt16(format.hasOwnProperty("unit") ? format.unit.toUUID16() : 0);
				buffer.putInt8(format.hasOwnProperty("nameSpace") ? format.nameSpace : NAMESPACE_BLUETOOTH_SIG);
				buffer.putInt16(format.hasOwnProperty("description") ? format.description : 0);
				buffer.flip();
				return buffer.getByteArray();
			};
			this._parser = data => {
				let buffer = ByteBuffer.wrap(data);
				return {
					format: buffer.getInt8(),
					exponent: buffer.getInt8(),
					unit: UUID.getByUUID16(buffer.getInt16()),
					nameSpace: buffer.getInt8(),
					description: buffer.getInt16()
				};
			};
		} else if (uuid.equals(UUID_CHARACTERISTIC_AGGREGATE_FORMAT)) {
			this._serializer = handles => {
				let buffer = ByteBuffer.allocate(handles.length * 2);
				for (let i = 0; i < handles.length; i++) {
					buffer.putInt16(handles[i]);
				}
				buffer.flip();
				return buffer.getByteArray();
			};
			this._parser = data => {
				let handles = new Array();
				let buffer = ByteBuffer.wrap(data);
				while (buffer.remaining() > 0) {
					handles.push(buffer.getInt16());
				}
				return handles;
			};
		}
	}
	get uuid() {
		return this._uuid;
	}
	get value() {
		return this._localValue;
	}
	set value(value) {
		this._localValue = value;
	}
	_serializeValue() {
		if (this._serializer != null) {
			return this._serializer(this._localValue);
		}
		return this._localValue;
	}
	_parseValue(data) {
		if (data == null) {
			this._localValue = [];
			return;
		}
		let buffer = ByteBuffer.wrap(data);
		if (this._parser != null) {
			this._localValue = this._parser(buffer);
			return;
		}
		this._localValue = data;
	}
}
exports.Descriptor = Descriptor;

class Characteristic {
	constructor(uuid, properties) {
		this._uuid = uuid;
		this._properties = properties;
		this._localValue = null;
		this._formats = [];
		this._serializer = null;
		this._parser = null;
		this._descriptors = new Array();
	}
	get uuid() {
		return this._uuid;
	}
	get properties() {
		return this._properties;
	}
	get value() {
		return this._localValue;
	}
	set value(value) {
		this._localValue = value;
	}
	set serializer(serializer) {
		this._serializer = serializer;
	}
	set parser(parser) {
		this._parser = parser;
	}
	get descriptors() {
		return this._descriptors;
	}
	set formats(formats) {
		this._formats = new Array();
		for (let f = 0; f < formats.length; f++) {
			let format = getFormatByName(formats[f]);
			if (format > 0) {
				this._formats.push({format: format});
			}
		}
	}
	_addDescriptor(descriptor) {
		this._descriptors.push(descriptor);
	}
	getDescriptorByUUID(uuid) {
		for (let descriptor of this._descriptors) {
			if (descriptor.uuid.equals(uuid)) {
				return descriptor;
			}
		}
		return null;
	}
	_serializeValue() {
		if (this._serializer != null) {
			return this._serializer(this._localValue);
		}
		if (this._formats.length > 0) {
			if (this._formats.length == 1) {
				return FormatSerializer[this._formats[0].format](this._localValue);
			}
			let data = [];
			for (let i = 0; i < this._formats.length; i++) {
				data.concat(FormatSerializer[this._formats[i].format](this._localValue[i]));
			}
			return data;
		}
		return this._localValue;
	}
	_parseValue(data) {
		if (data == null) {
			this._localValue = [];
			return;
		}
		let buffer = ByteBuffer.wrap(data);
		if (this._parser != null) {
			this._localValue = this._parser(buffer);
			return;
		}
		if (this._formats.length > 0) {
			if (this._formats.length == 1) {
				this._localValue = FormatParser[this._formats[0].format](buffer);
				return;
			}
			let value = [];
			for (let format of this._formats) {
				value.concat(FormatParser[format.format](buffer));
			}
			this._localValue = value;
			return;
		}
		this._localValue = data;
	}
}
exports.Characteristic = Characteristic;

class Service {
	constructor(uuid, primary = true) {
		this._uuid = uuid;
		this._primary = primary;
		this._includes = new Map();
		/* FIXME: Need support of duplicate characteristics with the same UUID */
		this._characteristics = new Map();
	}
	get uuid() {
		return this._uuid;
	}
	get includes() {
		return this._includes.values();
	}
	get characteristics() {
		return this._characteristics.values();
	}
	addIncludedService(include) {
		this._includes.set(include.uuid.toString(), include);
	}
	_addCharacteristic(characteristic) {
		this._characteristics.set(characteristic.uuid.toString(), characteristic);
	}
	getIncludedServiceByUUID(uuid) {
		return this._includes.get(uuid.toString());
	}
	getCharacteristicByUUID(uuid) {
		return this._characteristics.get(uuid.toString());
	}
}
exports.Service = Service;

class Profile {
	constructor() {
		this._services = new Map();
	}
	get services() {
		return this._services.values();
	}
	_addService(service) {
		this._services.set(service.uuid.toString(), service);
	}
	getServiceByUUID(uuid) {
		return this._services.get(uuid.toString());
	}
}
exports.Profile = Profile;
