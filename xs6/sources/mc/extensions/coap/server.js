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
	Method,
	Option,
	Port,
	EXCHANGE_LIFETIME,
	Message,
	ContentFormat
} from 'coap/common';

import {ListeningSocket, UDP} from "socket";
import {Endpoint} from 'coap/endpoint';

export class ServerEndpoint extends Endpoint {
	constructor(peer, transport, error) {
		super(peer, transport, error);

		this._messageId = (Math.random() * 0xffff) | 0;
	}

	issueMessageId() {
		var mid = this._messageId++;
		if (this._messageId > 0xffff) this._messageId = 0;
		return mid;
	}

	send(response) {
		if (!response.messageId) {
			response.messageId = this.issueMessageId();
		}

		return super.send(response);
	}
}

export const Response = {
	__proto__: Message,
	code: [2, 5],
	payload: null,
};

function isSameBinary(a, b) {
	if (a === undefined && b === undefined) return true;
	if (typeof a !== 'string' || typeof b !== 'string') return false;
	if (!(a instanceof ArrayBuffer) || !(b instanceof ArrayBuffer)) return false;

	const Bin = require.weak('bin');
	return Bin.comp(a, b) === 0;
}

export class Server {
	constructor() {
		this.Endpoint = ServerEndpoint;
		this.Transport = require.weak('coap/transport');

		this.autoAck = true;

		this.sessionId = 1;
		this.endpoints = {}; // `peer` is the key
		this.sessions = [];
		this.resources = [];
		this.expireCheck = null;
	}

	get sessionExpirePeriod() {
		return EXCHANGE_LIFETIME * 1000;
	}

	start(port=Port) {
		if (this.transport) throw "already started";

		const sock = new ListeningSocket({port, proto: UDP});

		this.transport = new this.Transport(
			sock,
			(request, peer) => this._onReceive(request, peer),
			err => this._handleError(err)
		);
		this.port = port;
	}

	stop() {
		if (!this.transport) throw "already stopped";

		delete this.port;
		delete this.transport;
		// @TODO
	}

	bind(path, func) {
		this.resources.push([path, func]);
	}

	send(response, session) {
		let contentFormat = ContentFormat.OctetStream;
		switch (typeof response) {
			case 'string':
				response = ArrayBuffer.fromString(response);
				contentFormat = ContentFormat.PlainText;
				// fallthrough
			case 'object':
				if (response instanceof ArrayBuffer) {
					response = session.createResponse(response, contentFormat);
				}
				break;
		}

		response.token = session.token;
		if (!response.code) response.code = [2, 5];

		if (!response.options) response.options = [];

		if (session.confirmable && !session.response && !session.ackSent) {
			response.type = Type.Ack;
			response.messageId = session.messageId;
		}

		if (session._observeId) {
			response.observeId = session.issueObserveId();
		}

		session.endpoint.send(response);
		session.response = response;
	}

	onRequest(session) {
		const path = session.path;

		if (session.observeDeregister) {

		}


		for (const [pattern, func] of this.resources) {
			if (pattern == path || pattern == '*') {
				try {
					func.call(this, session);
				} catch (e) {
					if (typeof e == 'string') e = {message: "Internal server error: " + e};
					if (!e.code) e.code = [5, 0];
					return e;
				}
				return;
			}
		}

		return {code: [4, 4], message: "resource not found"};
	}

	_onReceive(request, peer) {
		let endpoint = this.endpoints[peer];

		if (request.type == Type.Ack || request.type == Type.Rst) {
			if (endpoint) {
				endpoint.dequeue(request);
			}
		} else {
			if (!endpoint) {
				endpoint = this.endpoints[peer] = new this.Endpoint(
					peer,
					this.transport,
					(err, msg) => this.handleError(err, msg)
				);
			}

			let session = this._findOrCreateSession(request, endpoint);
			this._runSession(session);
		}
	}

	_runSession(session) {
		if (session.response) {
			if (session.ackSent) session.endpoint.sendAck(session);
			session.send(session.response);
		} else {
			const error = this.onRequest(session);
			if (!error) {
				if (!session.response && session.confirmable && session.autoAck) {
					session.endpoint.sendAck(session);
					session.ackSent = Date.now();
				}
			} else {
				if (session.observe) session.cancelObserve();
				this._sendError(session, error.code, error.message);
			}
		}

		if (!session.confirmable && session.response && !session.observe) {
			const pos = this.sessions.indexOf(session);
			if (pos >= 0) this.sessions.splice(pos, 1);
		}
	}

	_sendError(session, code, message) {
		const response = session.createResponse(message);
		response.code = code;
		session.send(response);
	}

	_findOrCreateSession(request, endpoint) {
		let session = this._findSession(request, endpoint);
		if (session) return session;
		return this._createSession(request, endpoint);
	}

	_findSession(request, endpoint) {
		for (const s of this.sessions) {
			if (s.endpoint == endpoint && s.messageId == request.messageId) {
				return s;
			}
		}
	}

	_findSameClientSession(request, endpoint) {
		const {comp} = require.weak('bin');
		const uri = request.uri;

		for (const s of this.sessions) {
			if (s._observeId && s.endpoint == endpoint && s.uri == uri && isSameBinary(s.token, request.token)) {
				return s;
			}
		}
	}

	_createSession(request, endpoint) {
		const server = this;

		const session = {
			__proto__: request,
			endpoint,
			id: this.sessionId++,
			expireAt: Date.now() + this.sessionExpirePeriod,
			autoAck: server.autoAck,

			createResponse(payload, contentFormat) {
				const response = {__proto__: Response};
				response.type = this.type;
				response.setPayload(payload, contentFormat);
				return response;
			},
			send(response) {
				server.send(response, this);
			},
			acceptObserve() {
				if (this._observeId) throw new Error('session is already accepted');
				this._observeId = 2;
			},
			cancelObserve() {
				delete this._observeId;
			},
			issueObserveId() {
				const id = this._observeId++;
				if (this._observeId > 0xffffff) this._observeId = 3;
				return id;
			},
			get isExpired() {
				if (this._observeId) return false;
				return this.expireAt <= Date.now()
			}
		};

		this.sessions.push(session);
		this._sortSessions();
		return session;
	}

	_sortSessions() {
		if (this.expireCheck) {
			clearTimeout(this.expireCheck);
			this.expireCheck = null;
		}

		const now = Date.now();

		this.sessions = this.sessions.filter(a => !a.isExpired);
		this.sessions.sort((a, b) => {
			return a.expireAt - b.expireAt;
		});

		if (this.sessions.length > 0) {
			this.expireCheck = setTimeout(() => {
				this._sortSessions();
			}, this.sessions[0].expireAt - now);
		}
	}

	_handleError(err, msg=null) {
		if (this.onError) {
			this.onError(err, msg);
		} else {
			trace('CoAP.Server Error: ' + err + "\n");
		}
	}
};

export default Server;
