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
 * Bluetooth v4.2 - HCI Socket Transport Layer (for NodeJS)
 *
 */

/* http://github.com/sandeepmistry/node-bluetooth-hci-socket */
var BluetoothHciSocket = require('bluetooth-hci-socket');

const UART = require("./uart");
const Utils = require("../../common/utils");
const Logger = Utils.Logger;

const TYPE_MASK = (1 << HCI_COMMAND_PKT) | (1 << HCI_EVENT_PKT) | (1 << HCI_ACLDATA_PKT);
const EVENT_MASK1 = (1 << 0x05) | (1 << 0x08) | (1 << 0x0E) | (1 << 0x0F) | (1 << 0x13);
const EVENT_MASK2 = (1 << (0x3E - 32));

var logger = Logger.getLogger("HCI.Socket");

class Transport extends UART.Transport {
	constructor() {
		super({
			write: arrayBuffer => {
				this._socket.write(new Buffer(arrayBuffer));
			}
		});
		this._hci = null;
		this._socket = new BluetoothHciSocket();
	}
	init(hci, deviceId) {
		this._hci = hci;
		this._socket.on("data", this.dataReceived.bind(this));
		this._socket.bindRaw(deviceId);
		this._socket.start();
		this.setSocketFilter();
	}
	setSocketFilter() {
		let filter = new Buffer(14);
		let opcode = 0;
		filter.writeUInt32LE(TYPE_MASK, 0);
		filter.writeUInt32LE(EVENT_MASK1, 4);
		filter.writeUInt32LE(EVENT_MASK2, 8);
		filter.writeUInt16LE(opcode, 12);
		logger.debug('setting filter to: ' + filter.toString('hex'));
		this._socket.setFilter(filter);
	}
	dataReceived(data) {
		logger.trace('data: ' + data.toString('hex'));

		let responses = super.received(data, 0, data.length);
		for (let i = 0; i < responses.length; i++) {
			this._hci.transportReceived(responses[i]);
		}
	}
}
exports.Transport = Transport;
