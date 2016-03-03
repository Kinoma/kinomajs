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
 * Bluetooth v4.2 - Non BLL Bluetooth LE API
 */

var Pins = require("pins");

var GAP = require("core/gap");
exports.GAP = GAP;

var ProxyTransport = require("transport/proxy");

exports.BLL = {
	require: "/lowpan/bluetooth/transport/bll",
	pins: {}
};

exports.activate = function (bll, application, storage) {
	ProxyTransport.setBLLName(bll);
	Pins.when(bll, "notification", function (responses) {
		for (var i = 0; i < responses.length; i++) {
			/* Recover TypedArray from array buffer */
			if (responses[i].length > 0) {
				responses[i].data = new Uint8Array(
					responses[i].buffer, 0, responses[i].length);
			}
			GAP.HCI.transportReceived(responses[i]);
		}
	});
	GAP.activate(ProxyTransport, application, storage);
};
