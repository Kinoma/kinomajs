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

import {resolv} from 'socket';

import {
	Type,
	Method,
	Option,
	Port,
	OptionFormat,
	ContentFormat,
	Observe,
	optionFormat,

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

const isEmpty = response => response.code[0] == 0 && response.code[1] == 0;
const remove = require.weak('utils').remove;

// ======================================================
//  Token Repository (Private)
//   to generate and recycle request tokens
// ======================================================

export class TokenRepository {
	constructor() {
		let tokens = [];
		let seed = 0;

		this.issue = function() {
			const Bin = require.weak('bin');
			return tokens.length > 0 ? tokens.shift() : Bin.num2bin(seed++);
		}

		this.recycle = function(t) {
			tokens.push(t);
		}
	}
}

// ======================================================
//  CoAP Endpoint (Private)
// ======================================================

class Endpoint {
	constructor(peer, sock, client) {
		this.sock = sock;
		this.client = client;
		this.resends = []; // requests who is waiting for ack
		this.waiting = []; // requests who is waiting for response
		this.observing = []; // observing requests
		this.timer = null;
		this.peer = peer;
	}

	close() {
		// super.close();
		if (this.timer) clearTimeout(this.timer);
	}

	send(msg) {
		if (!('trial' in msg)) msg.trial = 0;

		this.sock.send(msg, this.peer);

		if (msg.type === Type.Con) this.queue(msg);
		if (msg.observe) this.observing.push(msg);
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

	get waitingCount() {
		return this.resends.length + this.waiting.length + this.observing.length;
	}

	invalidateRequest(request) {
		remove(this.resends, request);
		remove(this.waiting, request);
		remove(this.observing, request);
		this.reschedule();
	}

	gotAck(response, request) {
		let client = this.client;

		if (!request['ack']) {
			request['ack'] = Date.now();

			if (request.onAck) {
				request.onAck();
			} else if (client.onAck) {
				client.onAck(request);
			}

			remove(this.resends, request);
			this.reschedule();

			this.waiting.push(request);
		}
	}

	gotResponse(response, request) {
		let client = this.client;

		if (request['response'] && !request.observe) return;

		request['response'] = Date.now();

		if (request.onResponse) {
			request.onResponse(response);
		} else if (client.onResponse) {
			client.onResponse(response, request);
		}

		if (response.type == Type.Con) {
			this.sock.sendAck(response, this.peer);
		}

		remove(this.waiting, request);
		if (request.observe) return;

		if (this.waitingCount == 0) {
			client._closeEndpoint(this);
		}
		client.disposeRequest(request);
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
			let client = this.client;

			// this.pendingRequests.forEach(request => {
			// 	this.invalidateRequest(request);
			// 	client.disposeRequest(request);
			// });

			// client._closeEndpoint(this);
			client.handleError("cannot send. max retry", msg);
			return;
		}

		var now = Date.now();
		msg.nextSend = now + msg.interval;

		this.resends.push(msg);
		this.reschedule();
	}
}

// ======================================================
//  CoAP Client
// ======================================================

export class Client {
	constructor() {
		this.sock = null;
		this.endpoints = {};
		this.requests = [];
		this.tokens = new TokenRepository();
		this.messageId = (Math.random() * 0xffff) | 0;
	}

	get(url, callback) {
		return this.send({url, method:Method.GET}, callback);
	}

	post(url, payload, callback) {
		return this.send({url, method:Method.POST}, payload, callback);
	}

	put(url, payload, callback) {
		return this.send({url, method:Method.PUT}, payload, callback);
	}

	observe(url, callback) {
		return this.send({
			url,
			method:Method.GET,
			observe: true,
		}, callback);
	}

	cancel(request, sendDeregister=false) {
		if (this.requests.indexOf(request) < 0) throw "cannot find obsrving request";

		let peer = request.peer;
		let endpoint = this.endpoints[peer];
		if (!peer || !endpoint || !request.observe) throw "cannot find obsrving request";

		endpoint.invalidateRequest(request);
		this.disposeRequest(request);
	}

	send(data, payload, callback) {
		if (typeof data === 'string') {
			data = {url: data};
		}

		switch (typeof payload) {
			case 'function':
				callback = payload;
				break;

			case 'object':
				data.payload = payload;
				if (!data.method) data.method = Method.POST;
				break;
		}

		data.onResponse = callback;

		let request = this.createRequest(data)
		let host = request.host;

		resolv(host, address => {
			if (address) {
				this.sendTo(request, address);
			} else {
				var err = "resolve error";
				if (request.onError) {
					request.onError(err);
				} else if (this.onError) {
					this.onError(err, request);
				}
			}
		});

		this.requests.push(request);
		return request;
	}

	sendTo(request, address) {
		// check if the request is not canceled while resolving address.
		if (this.requests.indexOf(request) < 0) return;

		let peer = address + ':' + request.port;
		let endpoint = this.endpoints[peer];
		if (!endpoint) {
			this.endpoints[peer] = endpoint = this._createEndpoint(peer);
		}

		request.peer = peer;
		endpoint.send(request);
	}

	createRequest(data) {
		let request = require.weak('coap/create_request')(data);
		if (!request.messageId) request.messageId = this.issueMessageId();
		if (!request.token) request.token = this.tokens.issue();
		return request;
	}

	issueMessageId() {
		var mid = this.messageId++;
		if (this.messageId > 0xffff) this.messageId = 0;
		return mid;
	}

	_createEndpoint(peer) {
		let client = this;
		if (!this.sock) {
			this.sock = this._createSocket();
		}

		return new Endpoint(peer, this.sock, this);
	}

	_createSocket() {
		// const name = 'coap/' + (this.debug ? 'debug' : 'coap') + '_socket';
		const name = 'coap/coap_socket';
		const CoAPSocket = require.weak(name);

		return new CoAPSocket(
			(response, peer) => {
				let request = this.findRequest(response);
				let endpoint = this.endpoints[peer];

				if (!request || !endpoint) {
					this.sock.sendReset(response, peer);
					return;
				}

				if (response.type === Type.Ack) {
					endpoint.gotAck(response, request);
				}

				if (!isEmpty(response)) {
					endpoint.gotResponse(response, request);
				}
			},
			err => {
				this.handleError(err);
			}
		);
	}

	handleError(err, request=null) {
		if (request && request.onError) {
			request.onError(err);
		} else if (this.onError) {
			this.onError(err, request);
		} else {
			trace('CoAP.Client Error: ' + err + "\n");
		}
	}

	findRequest(response) {
		let token = response.token;
		let mid = response.messageId;
		const Bin = require.weak('bin');

		for (let request of this.requests) {
			if (token && request.token && Bin.comp(token, request.token) == 0) return request;
			if (mid === request.messageId) return request;
		}
	}

	disposeRequest(request) {
		if (request.token) this.tokens.recycle(request.token);
		remove(this.requests, request);
	}

	_closeEndpoint(endpoint) {
		let peer = endpoint.peer;
		if (this.endpoints[peer]) {
			delete this.endpoints[peer];
			endpoint.close();
		}
	}
}

export default {
	Type,
	Method,
	Option,
	OptionFormat,
	ContentFormat,
	Observe,
	TokenRepository,
	Client,
	Port,

	// test() {
	// 	function printResponse(response) {
	// 		console.log("🐱");
	// 		console.log("version: " + response.version);
	// 		console.log("type: " + response.type);
	// 		console.log("code: " + response.code);
	// 		console.log("messageId: " + response.messageId);
	// 		console.log("token: " + binary(response.token));
	// 		console.log("options: " + JSON.stringify(response.options));
	// 		console.log("payload: " + binary(response.payload));
	// 		console.log("🐕");
	// 	}

	// 	function binary(val) {
	// 		if (!val) return null;
	// 		val = (val.byteLength == 0) ? [] : Array.from(new Uint8Array(val));
	// 		return '[' + val.map(v => v.toString(16)).join(' ') + ']';
	// 	}

	// 	let client = new Client();
	// 	client.onResponse = response => {
	// 		printResponse(response);
	// 	}

	// 	let server = '10.85.20.118'	// Mac

	// 	let req = client.observe(`coap://${server}/color`);
	// 	// let req = client.post(`coap://${server}/color?red=100&green=200&blue=150`, (new Uint8Array([0])).buffer);
	// 	console.log(req);

	// 	setInterval(() => {
	// 		console.log(client.requests);
	// 	}, 1000)
	// }
};

