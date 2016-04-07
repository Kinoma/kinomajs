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
 * Bluetooth v4.2 - GATT - Device Information Service
 */

var Utils = require("../../common/utils");

var BTUtils = require("../core/btutils");
var UUID = BTUtils.UUID;

function unsignedInt64ToBinary(high, low) {
	return Utils.multiIntToByteArray([low, high], 4, 2, true);
}

var GATT = require("../core/gatt");

var OUI_MARVELL = 0x005043;

var SERVICE_DEVICE_INFORMATION = UUID.getByUUID16(0x180A);

var CHAR_MANUFACTURER_NAME_STRING = UUID.getByUUID16(0x2A29);
var CHAR_MODEL_NUMBER_STRING = UUID.getByUUID16(0x2A24);
var CHAR_SERIAL_NUMBER_STRING = UUID.getByUUID16(0x2A25);
var CHAR_HARDWARE_REVISION_STRING = UUID.getByUUID16(0x2A27);
var CHAR_FIRMWARE_REVISION_STRING = UUID.getByUUID16(0x2A26);
var CHAR_SOFTWARE_REVISION_STRING = UUID.getByUUID16(0x2A28);
var CHAR_SYSTEM_ID = UUID.getByUUID16(0x2A23);
var CHAR_IEEE_REG_CERT_DATA_LIST = UUID.getByUUID16(0x2A2A);

function createService(info) {
	var service = new GATT.Service(SERVICE_DEVICE_INFORMATION);
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_MANUFACTURER_NAME_STRING, info.manufacturer));
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_MODEL_NUMBER_STRING, info.modelNumber));
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_SERIAL_NUMBER_STRING, info.serialNumber));
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_HARDWARE_REVISION_STRING, info.hardwareRevision));
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_FIRMWARE_REVISION_STRING, info.firmwareRevision));
	service.addCharacteristic(
		GATT.createReadOnlyStringCharacteristic(
			CHAR_SOFTWARE_REVISION_STRING, info.softwareRevision));
	let systemId = new GATT.Characteristic(CHAR_SYSTEM_ID, GATT.Properties.READ);
	systemId.value = unsignedInt64ToBinary(info.systemIdHigh, info.systemIdLow);
	service.addCharacteristic(systemId);
	return service;
}
exports.createService = createService;
