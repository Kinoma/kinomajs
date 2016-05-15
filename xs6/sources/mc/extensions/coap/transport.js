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
import {Type} from 'common';

export class Transport {
	constructor(sock, receive, error) {
		this.sock = sock;

		sock.onData = (blob) => {
			if (!blob || blob.byteLength == 0) return;

			const decode = require.weak('decode');
			try {
				receive(decode(blob), sock.peer);
			}
			catch (e) {
				error(e);
			}
		};
	}

	send(message, peer) {
		const encode = require.weak('encode');
		const blob = encode(message);

		this.sock.send(blob, peer);
	}

	emptyMessage(type, message) {
		return {
			type: type,
			code: [0, 0],
			messageId: message.messageId,
		};
	}

	sendAck(message, peer) {
		this.send(this.emptyMessage(Type.Ack, message), peer);
	}

	sendReset(message, peer) {
		this.send(this.emptyMessage(Type.Rst, message), peer);
	}
}

export default Transport;
