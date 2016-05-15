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

import {
	Type,

	ACK_TIMEOUT,
	ACK_RANDOM_FACTOR,
	MAX_RETRANSMIT,
	NSTART,
	DEFAULT_LEISURE,
	PROBING_RATE,
	MAX_LATENCY,
	PROCESSING_DELAY,
	MAX_TRANSMIT_SPAN,
	EXCHANGE_LIFETIME
} from 'coap/common';

const remove = require.weak('utils').remove;

// ======================================================
//  CoAP Endpoint (Private)
// ======================================================

export class Endpoint {
	constructor(peer, transport, error) {
		this.transport = transport;
		this.error = error;
		this.resends = []; // requests who is waiting for ack
		this.timer = null;
		this.peer = peer;
	}

	close() {
		// super.close();
		if (this.timer) clearTimeout(this.timer);
	}

	send(msg) {
		if (!('trial' in msg)) msg.trial = 0;

		this.transport.send(msg, this.peer);

		if (msg.type === Type.Con) this.queue(msg);
	}

	sendAck(msg) {
		this.transport.sendAck(msg, this.peer);
	}

	reschedule() {
		if (this.timer) {
			clearTimeout(this.timer);
			this.timer = null;
		}

		if (this.resends.length == 0) return;

		var now = Date.now();

		this.resends.sort(function(a, b) {
			if (a.nextSend < b.nextSend) return -1;
			if (a.nextSend > b.nextSend) return 1;
			return 0;
		});

		// recalc timer
		var msg = this.resends[0];
		var delay = msg.nextSend - now;

		this.timer = setTimeout(() => {
			msg = this.resends.shift();
			this.send(msg);
		}, delay);
	}

	invalidateRequest(request) {
		remove(this.resends, request);
		this.reschedule();
	}

	queue(msg) {
		if (msg.trial == 0) {
			/* For a new Confirmable message, the initial timeout is set
			   to a random duration (often not an integral number of seconds)
			   between ACK_TIMEOUT and (ACK_TIMEOUT * ACK_RANDOM_FACTOR)
			 */
			var timeout = ACK_TIMEOUT * 1000;
			msg.interval = timeout + Math.random() * (timeout * (ACK_RANDOM_FACTOR - 1));
		} else {
			msg.interval *= 2;
		}

		if (msg.trial++ >= MAX_RETRANSMIT) {
			this.error("cannot send. max retry", msg);
			return;
		}

		var now = Date.now();
		msg.nextSend = now + msg.interval;

		this.resends.push(msg);
		this.reschedule();
	}

	dequeue(msg) {
		remove(this.resends, msg);
		this.reschedule();
	}
}

export default Endpoint;
