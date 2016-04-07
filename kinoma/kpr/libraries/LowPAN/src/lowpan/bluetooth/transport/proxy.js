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
 * Bluetooth v4.2 - BLL Proxy Transport Layer
 */

var Pins = require("pins");

var _bll = "hci";

exports.setBLLName = function (bll) {
	_bll = bll;
};

exports.sendCommand = function (command) {
	/* Do not use TypedArray directly */
	command.buffer = (command.data != null) ? command.data.buffer : null;
	command.data = null;
	Pins.invoke("/" + _bll + "/sendCommand", command);
};

exports.sendACLData = function (acl) {
	/* Do not use TypedArray directly */
	acl.buffer = (acl.data != null) ? acl.data.buffer : null;
	acl.data = null;
	Pins.invoke("/" + _bll + "/sendACLData", acl);
};
