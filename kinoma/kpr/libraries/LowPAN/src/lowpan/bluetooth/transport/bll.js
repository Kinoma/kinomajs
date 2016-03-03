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
 * Bluetooth v4.2 - UART Transport Layer (BLL Wrapper)
 */

var UART = require("uart");

var _notification = null;
var _repeat = null;

exports.configure = function () {
	this.notification = _notification = PINS.create({
		type: "Notification"
	});
	this.serial = UART.open();
	_repeat = PINS.repeat("serial", this, function () {
		var responses = UART.receive();
		if (responses.length > 0) {
			for (var i = 0; i < responses.length; i++) {
				/* Do not use TypedArray directly */
				responses[i].buffer = (responses[i].data != null) ? responses[i].data.buffer : null;
				responses[i].data = null;
			}
			_notification.invoke(responses);
		}
	});
};

exports.sendCommand = function (command) {
	/* Recover TypedArray from array buffer */
	if (command.length > 0) {
		command.data = new Uint8Array(command.buffer, 0, command.length);
	}
	UART.sendCommand(command);
};

exports.sendACLData = function (acl) {
	/* Recover TypedArray from array buffer */
	if (acl.length > 0) {
		acl.data = new Uint8Array(acl.buffer, 0, acl.length);
	}
	UART.sendACLData(acl);
};

exports.close = function () {
	_repeat.close();
	UART.close();
	_notification.close();
};
