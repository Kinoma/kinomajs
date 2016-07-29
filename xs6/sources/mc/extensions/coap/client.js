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

import {resolv, Socket} from 'socket';

import {
	Type,
	Method
} from 'coap/common';

import {Endpoint} from 'coap/endpoint';

const isEmpty = response => response.code[0] == 0 && response.code[1] == 0;
const remove = require.weak('utils').remove;

export class ClientEndpoint extends Endpoint {
	constructor(peer, transport, error) {
		super(peer, transport, error);
		this.waiting = []; // requests who is waiting for response
		this.observing = []; // observing requests
	}

	send(msg) {
		super.send(msg);
		this.waiting.push(msg);
		if (msg.observe) this.observing.push(msg);
	}

	invalidateRequest(request) {
		remove(this.waiting, request);
		remove(this.observing, request);

		super.invalidateRequest(request);
	}

	gotResponse(request) {
		remove(this.waiting, request);
	}

	get isSomebodyWaiting() {
		return (this.resends.length + this.waiting.length + this.observing.length) > 0;
	}
}

// ======================================================
//  CoAP Client
// ======================================================

export class Client {
	constructor() {
		this.transport = null;
		this.endpoints = {};
		this.requests = [];
		const xorshift = require.weak('utils/xorshift');
		this.genToken = xorshift();
		this.messageId = (Math.random() * 0xffff) | 0;
		this.Transport = require.weak('coap/transport');
		this.Endpoint = ClientEndpoint;
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
		if (typeof payload === 'function') {
			callback = payload;
			payload = undefined;
		}

		let request = this.createRequest(data, payload)
		let host = request.host;
		if (callback) request.onResponse = callback;

		resolv(host, address => {
			if (address) {
				this._sendTo(request, address);
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

	_sendTo(request, address) {
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

	createRequest(data, payload) {
		if (typeof data === 'string') {
			data = {url: data};
		}

		if (payload) {
			if (typeof payload === 'string') {
				payload = ArrayBuffer.fromString(payload);
			}

			data.payload = payload;
			if (!data.method) data.method = Method.POST;
		}

		const request = require.weak('coap/create_request')(data);
		if (!request.messageId) request.messageId = this._issueMessageId();
		if (!request.token) request.token = this._issueToken();
		return request;
	}

	_issueMessageId() {
		var mid = this.messageId++;
		if (this.messageId > 0xffff) this.messageId = 0;
		return mid;
	}

	_issueToken() {
		const value = this.genToken();
		return (new Uint32Array([value])).buffer;
	}

	_createEndpoint(peer) {
		if (!this.transport) {
			this.transport = this._createTransport();
		}

		return new this.Endpoint(peer, this.transport, (err, msg) => {
			this.handleError(err, msg);
		});
	}

	_createTransport() {
		const sock = new Socket({proto: Socket.UDP});

		return new this.Transport(
			sock,
			(response, peer) => {
				let request = this.findRequest(response);
				let endpoint = this.endpoints[peer];

				if (!request || !endpoint) {
					this.transport.sendReset(response, peer);
					return;
				}

				if (response.type == Type.Con) {
					this.transport.sendAck(response, peer);
				}

				if (response.type === Type.Ack) {
					if (!request['ack']) {
						request['ack'] = Date.now();

						if (request.onAck) {
							request.onAck();
						} else if (this.onAck) {
							this.onAck(request);
						}

						endpoint.dequeue(request);
					}
				}

				if (!isEmpty(response)) {
					if (!request['response'] || request.observe) {
						request['response'] = Date.now();

						if (request.onResponse) {
							request.onResponse(response);
						} else if (this.onResponse) {
							this.onResponse(request, response);
						}

						endpoint.gotResponse(request);

						if (!request.observe) {
							if (!endpoint.isSomebodyWaiting) {
								this._closeEndpoint(endpoint);
							}
							this.disposeRequest(request);
						}
					}
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
		const {comp} = require.weak('bin');

		for (let request of this.requests) {
			if (token && request.token && comp(token, request.token) == 0) return request;
			if (mid === request.messageId) return request;
		}
	}

	disposeRequest(request) {
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

export default Client;
