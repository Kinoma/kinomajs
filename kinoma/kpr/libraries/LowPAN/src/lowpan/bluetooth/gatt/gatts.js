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
 * Bluetooth v4.2 - GATT - GATT Service
 */

var BTUtils = require("../core/btutils");
var UUID = BTUtils.UUID;

var GATT = require("../core/gatt");

var SERVICE_GATT = UUID.getByUUID16(0x1801);

var CHAR_SERVICE_CHANGED = UUID.getByUUID16(0x2A05);

function createService() {
	let service = new GATT.Service(SERVICE_GATT);
	let serviceChanged = new GATT.Characteristic(CHAR_SERVICE_CHANGED, GATT.Properties.INDICATE);
	serviceChanged.addFormat({format: GATT.Format.UINT16});
	serviceChanged.addFormat({format: GATT.Format.UINT16});
	service.addCharacteristic(serviceChanged);
	return service;
}
exports.createService = createService;
