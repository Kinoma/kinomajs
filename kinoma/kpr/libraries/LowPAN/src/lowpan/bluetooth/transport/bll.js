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

const UART = require("./uart");

const DEFAULT_UART_DEVICE = "/dev/mbtchar0";
const DEFAULT_UART_BAUDRATE = 115200;

/* Pins instances */
var _notification = null;
var _serial = null;
var _repeat = null;

/* Transport */
var _transport = null;

// ----------------------------------------------------------------------------------
// Pins Configuration
// ----------------------------------------------------------------------------------

function pollTransport() {
	let buffer = _serial.read("ArrayBuffer");
	if (buffer.byteLength == 0) {
		logger.debug("serial.read returns 0");
		return;
	}
	_transport.receive(new Uint8Array(buffer), 0, buffer.byteLength);
}

exports.configure = function () {
	this.notification = _notification = PINS.create({
		type: "Notification"
	});
	this.serial = _serial = PINS.create({
		type: "Serial",
		path: DEFAULT_UART_DEVICE,
		baud: DEFAULT_UART_BAUDRATE
	});
	_serial.init();
	_repeat = PINS.repeat("serial", this, pollTransport);
	_transport = new UART.Transport(_serial);
	_transport.delegate = {
		transportReceived: response => {
			/* Do not use TypedArray directly */
			response.buffer = (response.data != null) ? response.data.buffer : null;
			response.data = null;
			_notification.invoke(response);
		}
	};
};

exports.sendCommand = function (command) {
	/* Recover TypedArray from array buffer */
	if (command.length > 0) {
		command.data = new Uint8Array(command.buffer, 0, command.length);
	}
	_transport.sendCommand(command);
};

exports.sendACLData = function (acl) {
	/* Recover TypedArray from array buffer */
	if (acl.length > 0) {
		acl.data = new Uint8Array(acl.buffer, 0, acl.length);
	}
	_transport.sendACLData(acl);
};

exports.close = function () {
	_notification.close();
	_repeat.close();
	_serial.close();
};
