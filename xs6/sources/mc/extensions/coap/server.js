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
	EXCHANGE_LIFETIME
} from 'coap/common';

import {ListeningSocket, UDP} from "socket";
import {Endpoint} from 'coap/endpoint';

const remove = require.weak('utils').remove;

export class ServerEndpoint extends Endpoint {
}

export class Server {
	constructor() {
		this.Endpoint = ServerEndpoint;
		this.Transport = require.weak('coap/transport');
		this.sessionExpirePeriod = EXCHANGE_LIFETIME * 1000;

		this.messageId = (Math.random() * 0xffff) | 0;
		this.sessionId = 1;
		this.endpoints = {}; // `peer` is the key
		this.sessions = [];
		this.resources = [];
		this.expireCheck = null;
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
		switch (typeof response) {
			case 'string':
				response = ArrayBuffer.fromString(response);
				// fallthrough
			case 'object':
				if (response instanceof ArrayBuffer) {
					response = {payload: response};
				}
				break;
		}

		response.type = session.type;
		response.token = session.token;
		if (!response.code) response.code = [2, 5];

		if (!response.options) response.options = [];
		if (response.contentFormat) response.options.push([Option.ContentFormat, response.contentFormat]);

		if (session.confirmable && !session.ackSent) {
			response.type = Type.Ack;
			response.messageId = session.messageId;
		} else {
			response.messageId = this._issueMessageId();
		}

		session.endpoint.send(response);
		session.response = response;
	}

	onRequest(session) {
		const path = session.path;

		for (const [pattern, func] of this.resources) {
			if (pattern == path || pattern == '*') {
				try {
					func.call(this, session);
				} catch (e) {
					if (typeof e == 'string') e = {message: "Internal server error: " + e};
					if (!e.code) e.code = [5, 0];
					throw e;
				}
				return;
			}
		}

		throw {code: [4, 4], message: "resource not found"};
	}

	_issueMessageId() {
		var mid = this.messageId++;
		if (this.messageId > 0xffff) this.messageId = 0;
		return mid;
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
			this.send(session.response, session);
		} else {
			try {
				this.onRequest(session);

				if (!session.response && session.confirmable) {
					session.endpoint.sendAck(session);
					session.ackSent = Date.now();
				}
			} catch (e) {
				if (session.observe) this.cancelObserve(session);
				this._sendError(session, e.code, e.message);
			}
		}

		if (!session.confirmable && session.response && !session.observe) {
			remove(this.sessions, session);
		}
	}

	_sendError(session, code, message) {
		this.send({
			code: code,
			payload: message ? ArrayBuffer.fromString(message) : nil,
		}, session);
	}

	_findOrCreateSession(request, endpoint) {
		let session = this._findSession(request, endpoint);
		if (session) return session;
		return this._createSession(request, endpoint);
	}

	_findSession(request, endpoint) {
		for (const s of this.sessions) {
			if (s.messageId == request.messageId && s.endpoint == endpoint) {
				return s;
			}
		}
	}

	_createSession(request, endpoint) {
		log("created session for " + endpoint.peer)
		const session = {
			__proto__: request,
			endpoint,
			id: this.sessionId++,
			expireAt: Date.now() + this.sessionExpirePeriod,
			getOptions(option) {
				if (!this.options || this.options.length == 0) return [];

				return this.options
					.filter(([opt, value]) => opt == option)
					.map(([opt, value]) => value);
			},
			get confirmable() {
				return this.type == Type.Con;
			},
			get host() {
				return this.getOptions(Option.UriHost).shift();
			},
			get port() {
				return this.getOptions(Option.UriPort).shift();
			},
			get path() {
				return '/' + this.getOptions(Option.UriPath)
								.map(encodeURIComponent)
								.join('/');
			},
			get query() {
				return this.getOptions(Option.UriQuery)
								.map(encodeURIComponent)
								.join('&');
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

		this.sessions
			.filter(a => a.expireAt > now)
			.sort((a, b) => {
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
