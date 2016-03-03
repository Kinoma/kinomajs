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
 * Bluetooth v4.2 - GATT - GAP Service
 */

var BTUtils = require("../core/btutils");
var UUID = BTUtils.UUID;

var GATT = require("../core/gatt");

var SERVICE_GAP = UUID.getByUUID16(0x1800);

var CHAR_DEVICE_NAME = UUID.getByUUID16(0x2A00);
var CHAR_APPEARANCE = UUID.getByUUID16(0x2A01);
var CHAR_PERIPHERAL_PREFERRED_CONN_PARAMS = UUID.getByUUID16(0x2A04);
var CHAR_CENTRAL_ADDRESS_RESOLUTION = UUID.getByUUID16(0x2AA6);
