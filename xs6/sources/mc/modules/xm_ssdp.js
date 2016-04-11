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

export default new Proxy({
	// state variables
	description: [],
	packets: undefined,
	socks: [],
	timer: undefined,		// packet send timer
	update: 0,				// next ssdp:alive time
	u: undefined,			// uuid

	onMessage(sock, length) {
		let packet = sock.read(String, length);
		if (packet)
			this.packet(packet, sock);
	},
	callback() {
		if (this.packets && (this.transmit() || !this.timer))
			return;

		if (Date.now() >= this.update) {
			this.description.forEach(description => {
				this.announce(description, "ssdp:alive");
			});
			this.reschedule(true);
		}

		if (!this.packets)
			this.reschedule();
	},
	busy(busy) {
		require.busy = busy;
	}
}, {
	get: function(target, key, receiver) {
		let result = target[key];
		if (result)
			return result;

		let module = require.weak("ssdp_implementation");
		return module[key];
	},
	set: Reflect.set
});
