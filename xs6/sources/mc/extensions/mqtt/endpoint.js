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

import {Socket} from 'socket';

export class Endpoint extends Socket {
	constructor(host, port) {
		super({ host, port, proto: "tcp" });

		this.packetId = ((Math.random() * 0xfffe) | 0) + 1;
		this.buffer = new ArrayBuffer(0);
		this._unacked = [];
	}

	onData(blob) {
		// if (blob.byteLength == 0) return;
		this.buffer = this.buffer.concat(blob);

		const decode = require("mqtt/decode");
		while (true) {
			let [buffer, packet] = decode(this.buffer);
			this.buffer = buffer;
			if (packet)
				this.gotPacket(packet);
			else
				break;
		}
	}

	gotPacket(packet) {
		this.onPacket(packet);
	}

	send(packet) {
		const encode = require.weak("encode");
		let binary = encode(packet);
		super.send(binary);

		if (packet.qos > 0) {
			this.queue(packet);
		}
	}

	issuePacketId() {
		let pid = this.packetId;
		this.packetId += 1;
		if (this.packetId > 0xffff) this.packetId = 1;
		return pid;
	}

	queue(packet) {
		this._unacked.push(packet);
	}

	ack(packetId) {
		let beforeLength = this._unacked.length;
		this._unacked = this._unacked.filter(packet => packet.packetId != packetId);
		return beforeLength > this._unacked.length;
	}
}

export default Endpoint;

