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

const Pins = require("pins");

class Transport {
	constructor(bll = null) {
		this._bll = (bll == null) ? "hci" : bll;
	}
	sendCommand(command) {
		/* Do not use TypedArray directly */
		command.buffer = (command.data != null) ? command.data.buffer : null;
		command.data = null;
		Pins.invoke("/" + this._bll + "/sendCommand", command);
	}
	sendACLData(acl) {
		/* Do not use TypedArray directly */
		acl.buffer = (acl.data != null) ? acl.data.buffer : null;
		acl.data = null;
		Pins.invoke("/" + this._bll + "/sendACLData", acl);
	}
}
exports.Transport = Transport;

exports.notificationReceived = (gap, response) => {
	/* Recover TypedArray from array buffer */
	if (response.length > 0) {
		response.data = new Uint8Array(response.buffer, 0, response.length);
	}
	gap.hci.transportReceived(response);
};
