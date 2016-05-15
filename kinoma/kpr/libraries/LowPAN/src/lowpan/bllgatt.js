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
 * Bluetooth v4.2 - BLL GATT API
 *
 */

const Utils = require("./common/utils");
const Logger = Utils.Logger;

/* GATT related optional BLE stack modules */
const GATT = require("./bluetooth/core/gatt");
const ATT = GATT.ATT;
const GATTServer = require("./bluetooth/gatt/server");
const GATTClient = require("./bluetooth/gatt/client");

const BTUtils = require("./bluetooth/core/btutils");
const UUID = BTUtils.UUID;

/* API consts */
const DEFAULT_MTU = 158;

var logger = Logger.getLogger("GATT");
var _profile = new GATTServer.Profile();
var doNotification = null;
var _gapApplication = null;

exports.setCallback = f => doNotification = f;
exports.setGAPApplication = app => _gapApplication = app;

exports.onConnection = function (context, index) {
	/* Setup ATT bearer inside the BLL thread */
	let bearer = new ATT.ATTBearer(context.attConnection, _profile.database);
	/* Override callbacks */
	bearer.onNotification = (opcode, notification) => {
		doNotification({
			notification: "gatt/characteristic/notify",
			connection: index,
			characteristic: notification.handle,
			value: notification.value
		});
	};
	bearer.onIndication = (opcode, indication) => {
		doNotification({
			notification: "gatt/characteristic/indicate",
			connection: index,
			characteristic: indication.handle,
			value: indication.value
		});
	};
	if (!context.peripheral) {
		GATTClient.exchangeMTU(bearer, DEFAULT_MTU);
	}
	return bearer;
};

// ----------------------------------------------------------------------------------
// API - GATT (Standard Mode API)
// ----------------------------------------------------------------------------------

function procedureComplete(connHandle, handle) {
	doNotification({
		notification: "gatt/request/complete",
		connection: connHandle
	});
}

function procedureCompleteWithError(procedure, errorCode, handle) {
	let msg = procedure + " failed: " + "ATT(" + Utils.toHexString(errorCode) + ")";
	logger.error(msg);
	throw new Error(msg);
}

exports.gattDiscoverAllPrimaryServices = function (params) {
	let index = params.connection;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.discoverPrimaryServices(bearer, results => {
		for (let result of results) {
			doNotification({
				notification: "gatt/service",
				connection: index,
				start: result.start,
				end: result.end,
				uuid: result.uuid.toString()
			});
		}
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllPrimaryServices", errorCode, handle);
	});
};

exports.gattDiscoverAllCharacteristics = function (params) {
	let index = params.connection;
	let start = params.start;
	let end = params.end;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.discoverCharacteristics(bearer, 0x0001, 0xFFFF, results => {
		for (let result of results) {
			doNotification({
				notification: "gatt/characteristic",
				connection: index,
				properties: parseCharacteristicProperties(result.properties),
				characteristic: result.handle,
				uuid: result.uuid.toString()
			});
		}
		return true;
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllCharacteristics", errorCode, handle);
	});
};

exports.gattDiscoverAllCharacteristicDescriptors = function (params) {
	let index = params.connection;
	let start = params.start;
	let end = params.end;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.discoverCharacteristicDescriptors(bearer, start, end, results => {
		for (let result of results) {
			doNotification({
				notification: "gatt/descriptor",
				connection: index,
				descriptor: result.handle,
				uuid: result.uuid.toString()
			});
		}
		return true;
	}).then(() => {
		procedureComplete(index);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattDiscoverAllCharacteristicDescriptors", errorCode, handle);
	});
};

exports.gattReadCharacteristicValue = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.readValue(bearer, handle).then(result => {
		doNotification({
			notification: "gatt/characteristic/value",
			connection: index,
			characteristic: handle,
			value: result
		});
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattReadCharacteristicValue", errorCode, handle);
	});
};

exports.gattWriteWithoutResponse = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let value = params.value;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.writeWithoutResponse(bearer, handle, new Uint8Array(value));
};

exports.gattWriteCharacteristicValue = function (params) {
	let index = params.connection;
	let handle = params.characteristic;
	let value = params.value;
	let bearer = _gapApplication.getContext(index).bearer;
	GATTClient.writeValue(bearer, handle, new Uint8Array(value)).then(() => {
		procedureComplete(index, handle);
	}).catch((errorCode, handle) => {
		procedureCompleteWithError("gattReadCharacteristicValue", errorCode, handle);
	});
};

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

function parseCharacteristicProperties(properties) {
	let parsed = [];
	if ((properties & GATT.Properties.BROADCAST) > 0) {
		parsed.push("broadcast");
	}
	if ((properties & GATT.Properties.READ) > 0) {
		parsed.push("read");
	}
	if ((properties & GATT.Properties.WRITE_WO_RESP) > 0) {
		parsed.push("writeWithoutResponse");
	}
	if ((properties & GATT.Properties.WRITE) > 0) {
		parsed.push("write");
	}
	if ((properties & GATT.Properties.NOTIFY) > 0) {
		parsed.push("notify");
	}
	if ((properties & GATT.Properties.INDICATE) > 0) {
		parsed.push("indicate");
	}
	return parsed;
}

exports.gattAddServices = function (params) {
	logger.debug("gattAddServices");

	let services = [];
	for (let s = 0; s < params.services.length; s++) {
		let serviceTmp = params.services[s];
		logger.debug("Service#" + s + ": " + serviceTmp.uuid);
		let service = new GATTServer.Service(
			UUID.getByString(serviceTmp.uuid),
			serviceTmp.hasOwnProperty("primary") ? serviceTmp.primary : true);
		if (serviceTmp.hasOwnProperty("includes")) {
			for (let i = 0; i < serviceTmp.includes.length; i++) {
				let uuid = serviceTmp.includes[i];
				service.addInclude(new GATTServer.Include(UUID.getByString(uuid)));
			}
		}
		for (let c = 0; c < serviceTmp.characteristics.length; c++) {
			let characteristicTmp = serviceTmp.characteristics[c];
			logger.debug("Characteristic#" + c + ": " + characteristicTmp.uuid);
			let characteristic = new GATTServer.Characteristic(
				UUID.getByString(characteristicTmp.uuid),
				toCharacteristicProperties(characteristicTmp.properties));
			if (characteristicTmp.hasOwnProperty("description")) {
				characteristic.description = characteristicTmp.description;
			}
			if (characteristicTmp.hasOwnProperty("value")) {
				/* XXX: We override the formats if the default values is a known type. */
				if ((typeof characteristicTmp.value) == "string") {
					characteristicTmp.formats = ["utf8s"];
				} else if ((typeof characteristicTmp.value) == "boolean") {
					characteristicTmp.formats = ["boolean"];
				} else if ((typeof characteristicTmp.value) == "number") {
					characteristicTmp.formats = ["sint32"];
				}
				characteristic.value = characteristicTmp.value;
			}
			if (characteristicTmp.hasOwnProperty("formats")) {
				for (let f = 0; f < characteristicTmp.formats.length; f++) {
					let format = GATT.getFormatByName(characteristicTmp.formats[f]);
					if (format > 0) {
						characteristic.addFormat({format: format});
					}
				}
			}
			characteristic.onValueRead = c => {
				logger.debug("Characteristic value on read: handle="
					+ Utils.toHexString(c.handle, 2));
			};
			characteristic.onValueWrite = c => {
				logger.debug("Characteristic value on write: handle="
					+ Utils.toHexString(c.handle, 2));
				doNotification({
					notification: "gatt/local/write",
					service: service.uuid,
					characteristic: c.uuid,
					value: c.value
				});
			};
			service.addCharacteristic(characteristic);
		}
		services.push(service);
	}
	let result = _profile.deployServices(services);
	logger.debug("Service added: " + JSON.stringify(result));

	doNotification({
		notification: "gatt/services/add",
		services: toAddServiceResults(services)
	});
};

function toAddServiceResults(services) {
	let results = [];
	for (let s = 0; s < services.length; s++) {
		let service = services[s];
		let result = {
			uuid: service.uuid.toString(),
			characteristics: []
		};
		for (let c = 0; c < service.characteristics.length; c++) {
			let characteristic = service.characteristics[c];
			result.characteristics.push({
				uuid: characteristic.uuid.toString(),
				handle: characteristic.handle
			});
		}
		results.push(result);
	}
	return results;
}

exports.gattWriteLocal = function (params) {
	let serviceUUID = UUID.getByString(params.service);
	let characteristicUUID = UUID.getByString(params.characteristic);
	let value = params.value;

	logger.debug("gattWriteLocal");

	/* Find characteristic */
	let service = _profile.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	let characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	/* Update characteristic value */
	characteristic.value = value;

	/* Auto Notify/Indication */
	_gapApplication.forEachContext(context => {
		let bearer = context.bearer;
		let config = characteristic.getClientConfiguration(bearer);
		if ((config & GATT.ClientConfiguration.NOTIFICATION) > 0) {
			characteristic.notifyValue(bearer);
		} else if ((config & GATT.ClientConfiguration.INDICATION) > 0) {
			characteristic.indicateValue(bearer);
		}
	});
};

exports.gattReadLocal = function (params) {
	let serviceUUID = UUID.getByString(params.service);
	let characteristicUUID = UUID.getByString(params.characteristic);

	logger.debug("gattReadLocal");

	/* Find characteristic */
	let service = _profile.getServiceByUUID(serviceUUID);
	if (service == null) {
		logger.error("Service " + serviceUUID.toString() + " not found.");
		return;
	}
	let characteristic = service.getCharacteristicByUUID(characteristicUUID);
	if (characteristic == null) {
		logger.error("Characteristic " + characteristicUUID.toString() + " not found.");
		return;
	}

	doNotification({
		notification: "gatt/local/read",
		service: serviceUUID,
		characteristic: characteristicUUID,
		value: characteristic.value
	});
};
