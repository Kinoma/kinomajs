/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
import {Socket, ListeningSocket} from 'socket';
import Utils from 'utils';
import Net from 'net';
import Bin from 'bin';

var Type = {
	Con: 0,
	Non: 1,
	Ack: 2,
	Rst: 3
};

var Method = {
	GET: 1,
	POST: 2,
	PUT: 3,
	DELETE: 4
};

var Option = {
	// RFC 7252 core
	IfMatch: 1,
	UriHost: 3,
	ETag: 4,
	IfNoneMatch: 5,
	UriPort: 7,
	LocationPath: 8,
	UriPath: 11,
	ContentFormat: 12,
	MaxAge: 14,
	UriQuery: 15,
	Accept: 17,
	LocationQuery: 20,
	ProxyUri: 35,
	ProxyScheme: 39,
	Size1: 60,

	// draft-ietf-core-block-15
	Block2: 23,
	Block1: 27,
	Size2: 28,

	// draft-ietf-core-observe-14
	Observe: 6,
};

var OptionFormat = {
	Empty: 0,
	Opaque: 1,
	Uint: 2,
	String: 3,
};

var Port = 5683;
var ACK_TIMEOUT = 2;  /* sec */
var ACK_RANDOM_FACTOR = 1.5;
var MAX_RETRANSMIT = 4;
var NSTART = 1;
var DEFAULT_LEISURE = 5; /* sec */
var PROBING_RATE = 1; /* byte/second */

/* MAX_LATENCY is the maximum time a datagram is expected to take
 * from the start of its transmission to the completion of its
 * reception.  This constant is related to the MSL (Maximum Segment
 * Lifetime) of [RFC0793], which is "arbitrarily defined to be 2
 * minutes" ([RFC0793] glossary, page 81).  Note that this is not
 * necessarily smaller than MAX_TRANSMIT_WAIT, as MAX_LATENCY is not
 * intended to describe a situation when the protocol works well, but
 * the worst-case situation against which the protocol has to guard.
 * We, also arbitrarily, define MAX_LATENCY to be 100 seconds.  Apart
 * from being reasonably realistic for the bulk of configurations as
 * well as close to the historic choice for TCP, this value also
 * allows Message ID lifetime timers to be represented in 8 bits
 * (when measured in seconds).  In these calculations, there is no
 * assumption that the direction of the transmission is irrelevant
 * (i.e., that the network is symmetric); there is just the
 * assumption that the same value can reasonably be used as a maximum
 * value for both directions.  If that is not the case, the following
 * calculations become only slightly more complex.
 */
var MAX_LATENCY = 100; /* sec */

/*
 * PROCESSING_DELAY is the time a node takes to turn around a
 * Confirmable message into an acknowledgement.  We assume the node
 * will attempt to send an ACK before having the sender time out, so
 * as a conservative assumption we set it equal to ACK_TIMEOUT.
 */
var PROCESSING_DELAY = ACK_TIMEOUT;

/* MAX_TRANSMIT_SPAN is the maximum time from the first transmission
 * of a Confirmable message to its last retransmission.  For the
 * default transmission parameters, the value is (2+4+8+16)*1.5 = 45
 * seconds, or more generally:
 *
 *   ACK_TIMEOUT * ((2 ** MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR
 */
var MAX_TRANSMIT_SPAN =  ACK_TIMEOUT * (Math.pow(2, MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR;

/*
 * EXCHANGE_LIFETIME is the time from starting to send a Confirmable
 * message to the time when an acknowledgement is no longer expected,
 * i.e., message-layer information about the message exchange can be
 * purged.  EXCHANGE_LIFETIME includes a MAX_TRANSMIT_SPAN, a
 * MAX_LATENCY forward, PROCESSING_DELAY, and a MAX_LATENCY for the
 * way back.  Note that there is no need to consider
 * MAX_TRANSMIT_WAIT if the configuration is chosen such that the
 * last waiting period (ACK_TIMEOUT * (2 ** MAX_RETRANSMIT) or the
 * difference between MAX_TRANSMIT_SPAN and MAX_TRANSMIT_WAIT) is
 * less than MAX_LATENCY -- which is a likely choice, as MAX_LATENCY
 * is a worst-case value unlikely to be met in the real world.  In
 * this case, EXCHANGE_LIFETIME simplifies to:
 *
 *   MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY
 *
 * or 247 seconds with the default transmission parameters.
 */
var EXCHANGE_LIFETIME = MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY;

function optionFormat(opt) {
	switch (opt) {
		case Option.IfNoneMatch:
			return OptionFormat.Empty;

		case Option.IfMatch:
		case Option.ETag:
			return OptionFormat.Opaque;

		case Option.UriPort:
		case Option.ContentFormat:
		case Option.MaxAge:
		case Option.Accept:
		case Option.Size1:
		case Option.Block2:
		case Option.Block1:
		case Option.Size2:
		case Option.Observe:
			return OptionFormat.Uint;

		case Option.UriHost:
		case Option.LocationPath:
		case Option.UriPath:
		case Option.UriQuery:
		case Option.LocationQuery:
		case Option.ProxyUri:
		case Option.ProxyScheme:
			return OptionFormat.String;
	}

	throw "Invalid option";
}

// ======================================================
//  Token Repository (Private)
//   to generate and recycle request tokens
// ======================================================

function generateToken(val) {
	var size = (val <= 0xff ? 1 : val <= 0xffff ? 2 : val <= 0xffffff ? 3 : 4);
	var t = new Uint8Array(size);
	while (size-- > 0) {
		var c = val & 0xff;
		val >>= 8;

		t[size] = c;
	}
	return t.buffer;
}

class TokenRepository {
	constructor() {
		var tokens = [];
		var seed = 0;

		this.issue = function() {
			return tokens.length > 0 ? tokens.shift() : generateToken(seed++);
		}

		this.recycle = function(t) {
			tokens.push(t);
		}
	}
}

// ======================================================
//  CoAP SharedSocket (Private)
//   shared by
// ======================================================

class SharedSocket extends Socket {
	constructor() {
		super({proto: "udp"});

		this.tasks = [];
		this.logger = null;
	}

	log() {
		if (this.logger) this.logger.log.apply(this.logger, arguments);
	}

	onConnect() {
		this.log("CoAP.SharedSocket.onConnect");

		while (this.tasks.length > 0) {
			let task = this.tasks.shift();
			task();
		}

		delete this.tasks;
	}

	queue(task) {
		if (this.tasks) {
			this.log("task queued");
			this.tasks.push(task);
		} else {
			task();
		}
	}

	send(data, addr) {
		this.queue(() => {
			this.log("send invoked");
			super.send(data, addr);
		});
	}
}

// ======================================================
//  CoAP Endpoint (Private)
// ======================================================

// ----- Private -------

function decodeMessage(blob) {
	var view = new Uint8Array(blob);
	var offset = 0, len = view.length;
	var readChr = function() {
		if (offset >= len) throw "Invalid message";
		return view[offset++];
	};

	var readBytes = function(n) {
		var pos = offset;
		offset += n;
		if (offset > len) throw "Invalid message";
		return view.slice(pos, offset).buffer;
	};

	var readOptExtra = function(val) {
		if (val == 13) {
			val = 13 + readChr();
		} else if (val == 14) {
			val = 13 + 256 + (readChr() * 256 + readChr());
		} else if (val == 15) {
			throw "Invalid option";
		}
		return val;
	};

	var flag = readChr();
	var version = flag >> 6;
	var type = (flag & 0x30) >> 4;
	var tokenLength = flag & 0x0f;
	var code = readChr();
	code = [(code >> 5) & 0x7, code & 0x1f];
	var messageId = (readChr() << 8) + readChr();
	var token = tokenLength > 0 ? readBytes(tokenLength) : null;
	var options = [];
	var payload = null;

	var opt = 0;
	while (len > offset) {
		var mark = readChr();
		if (mark == 0xff) {
			var size = len - offset;
			if (size <= 0) throw "Invalid payload size";
			payload = blob.slice(offset, offset + size);
			break;
		}

		var optDelta = (mark & 0xf0) >> 4, optLen = (mark & 0x0f), optValue = null;

		opt += readOptExtra(optDelta);
		optLen = readOptExtra(optLen);
		var format = optionFormat(opt);
		switch (format) {
			case OptionFormat.Empty:
				if (len != 0) throw "Invalid option";
				break;
			case OptionFormat.Opaque:
				optValue = readBytes(optLen);
				break;
			case OptionFormat.Uint:
				optValue = 0;
				while (optLen-- > 0) {
					optValue = optValue * 256 + readChr();
				}
				break;
			case OptionFormat.String:
				optValue = readBytes(optLen).toRawString();
				break;
		}
		options.push([opt, optValue]);
	}

	return {
		version: version,
		type: type,
		code: code,
		messageId: messageId,
		token: token,
		options: options,
		payload: payload
	};
}

function encodeMessage(message) {
	var version = ('version' in message ? message.version : 1);
	if (version !== 1) throw "Invalid version";

	var code = message.code;
	var token = 'token' in message ? message.token : null;
	var payload = 'payload' in message ? message.payload : null;

	var options = ('options' in message ? message.options : []).map(function(option, index) {
		var value = option[1];
		var option = option[0];
		var format = optionFormat(option);
		switch (format) {
			case OptionFormat.Empty:
				if (value !== null) throw "Invalid option";
				break;

			case OptionFormat.Opaque:
				break;

			case OptionFormat.Uint:
				var bytes = [];
				while (value > 0) {
					var a = value % 256;
					value = (value - a) / 256;
					bytes.unshift(a);
				}
				value = (new Uint8Array(bytes)).buffer;
				break;

			case OptionFormat.String:
				var blob = ArrayBuffer.fromString(value);
				value = blob;
				break;
		}
		return [option, index, value, null, null];
	});
	options.sort(function(a, b) {
		if (a[0] < b[0]) return -1;
		if (a[0] > b[0]) return 1;
		if (a[1] < b[1]) return -1;
		if (a[1] > b[1]) return 1;
		return 0;
	});

	var size = 4;
	if (token) size += token.byteLength;

	var sizeOptExtra = function(val) {
		if (val < 13) {
			return [val, []];
		} else if (val < (13 + 256)) {
			return [13, [val - 13]];
		} else if (val < (13 + 256 + 65536)) {
			val -= (13 + 256);
			return [14, [(val >> 8) & 0xff, val & 0xff]];
		} else {
			throw "Invalid option";
		}
	};

	var prev = 0;
	options.forEach(function(option) {
		size += 1;

		var delta = option[0] - prev;
		prev = option[0];

		// option[3] = [option delta, option delta extended bytes]
		option[3] = delta = sizeOptExtra(delta);
		size += delta[1].length;

		var optLen = option[2] ? option[2].byteLength : 0;
		size += optLen;

		// option[4] = [option length, option length extended bytes]
		option[4] = optLen = sizeOptExtra(optLen);
		size += optLen[1].length;
	});

	if (payload && payload.byteLength > 0) {
		size += 1 + payload.byteLength;
	}

	var blob = new Uint8Array(size);
	var offset = 0;

	function writeChr(c) {
		blob[offset++] = c;
	}

	function writeBytes(bytes) {
		writeArrays(new Uint8Array(bytes));
	}

	function writeArrays(bytes) {
		for (var i = 0; i < bytes.length; i++) {
			writeChr(bytes[i]);
		}
	}

	var tokenLength = token ? token.byteLength : 0;
	var flag = ((version & 0x3) << 6) | ((message.type & 0x3) << 4) | (tokenLength & 0xf);
	writeChr(flag);
	var code = ((code[0] & 0x3) << 5) | (code[1] & 0x1f);
	writeChr(code);
	writeChr((message.messageId & 0xff00) >> 8);
	writeChr(message.messageId & 0x00ff);
	if (tokenLength > 0) writeBytes(token);

	options.forEach(function(option) {
		var b = (option[3][0] << 4) | option[4][0];
		writeChr(b);

		writeArrays(option[3][1]);
		writeArrays(option[4][1]);

		if (option[2]) writeBytes(option[2]);
	});

	if (payload && payload.length > 0) {
		writeChr(0xff);
		writeBytes(payload);
	}

	return blob.buffer;
}

function queueForResend(msg) {
	if (msg.trial == 0) {
		/* For a new Confirmable message, the initial timeout is set
		   to a random duration (often not an integral number of seconds)
		   between ACK_TIMEOUT and (ACK_TIMEOUT * ACK_RANDOM_FACTOR)
		 */
		var timeout = ACK_TIMEOUT * 1000;
		msg.interval = Utils.rnd(timeout, timeout * ACK_RANDOM_FACTOR);
	} else {
		msg.interval *= 2;
	}

	if (msg.trial++ >= MAX_RETRANSMIT) {
		this.delegate.onError(this, "cannot send. max retry", msg);
		return;
	}

	var now = Utils.now();
	msg.nextSend = now + msg.interval;

	this.waitingAck.push(msg);
	this.reschedule();
}

// ----- Public -------

class Endpoint {
	constructor(peer, sock, delegate) {
		this.sock = sock;
		this.delegate = delegate;
		this.queue = [];
		this.waitingAck = [];
		this.timer = null;
		this.peer = peer;
		this.logger = null;
	}

	log() {
		if (this.logger) this.logger.log.apply(this.logger, arguments);
	}

	onError() {
		this.log("CoAP.Endpoint.onError");
	}

	onClose() {
		this.log("CoAP.Endpoint.onClose");
	}

	close() {
		// super.close();
		if (this.timer) clearTimeout(this.timer);
	}

	send(msg) {
		if (!('blob' in msg)) {
			msg.blob = encodeMessage(msg);
			msg.trial = 0;
		}

		this.log("CoAP.Endpoint.send: will send message", msg, " to ", this.peer);
		this.sock.send(msg.blob, this.peer);

		if (msg.type === Type.Con) queueForResend.call(this, msg);
	}

	reschedule() {
		if (this.timer) {
			clearTimeout(this.timer);
			this.timer = null;
		}

		if (this.waitingAck.length == 0) return;

		var now = Utils.now();

		this.waitingAck.sort(function(a, b) {
			if (a.nextSend < b.nextSend) return -1;
			if (a.nextSend > b.nextSend) return 1;
			return 0;
		});

		// recalc timer
		var msg = this.waitingAck[0];
		var delay = msg.nextSend - now;

		this.log("message was rescheduled in " + delay + " msec\n");
		this.timer = setTimeout(function() {
			msg = this.waitingAck.shift();
			this.send(msg);
		}.bind(this), delay);
	}

	get pendingRequests() {
		return this.queue.concat(this.waitingAck);
	}

	invalidateRequest(request) {
		Utils.remove(this.queue, request);
		Utils.remove(this.waitingAck, request);
		this.reschedule();
	}

	gotAck(msg) {
		var found;
		this.waitingAck.some(function(m, index) {
			if (m.messageId == msg.messageId) {
				found = index;
				return true;
			}
		});
		if (found !== undefined) {
			this.delegate.onAck(this, msg);
			this.waitingAck.splice(found, 1);
			this.reschedule();
		}
	}

	gotResponse(msg) {
		this.delegate.onResponse(this, msg);
	}
}

// ======================================================
//  CoAP Client
// ======================================================

// ----- Private -------

function createSocket(client) {
	var sock = new SharedSocket();
	sock.logger = client.logger;

	sock.onMessage = function(n) {
		var blob = this.recv(n);
		if (!blob) return;

		var endpoint = findEndpoint(client, sock.peer);
		if (!endpoint) {
			client.log("CoAP.SharedSocket.onMessage: cannot find endpoint for", sock.peer);
			return;
		}

		client.log("CoAP.SharedSocket.onMessage: got raw message", blob, " from", sock.peer);
		var message = decodeMessage(blob);
		client.log("CoAP.SharedSocket.onMessage: got message", message);

		if (message.type === Type.Ack) {
			endpoint.gotAck(message);
		}

		endpoint.gotResponse(message);
	}

	return sock;
}

function makeRequest($data) {
	var url;
	if (!(url = $data('url'))) throw "url is required";

	var parts = Net.parseUrl(url);
	var $parts = Utils.attr(parts);
	var host = $parts('host');
	var port = $parts('port', Port);
	if (!$parts('scheme') || !host || !port) throw "bad url";

	var options = $data('options', []);

	if (!Net.isDottedAddress(host)) {
		options.push([Option.UriHost, host]);
	}

	if (port != Port) {
		options.push([Option.UriPort, port]);
	}

	var path = $parts('path');
	if (path) {
		path.split('/').slice(1).forEach(function(part) {
			options.push([Option.UriPath, decodeURIComponent(part)]);
		});
	}

	var query = $parts('query');
	if (query) {
		query.split('?').slice(1).forEach(function(part) {
			options.push([Option.UriQuery, decodeURIComponent(part)]);
		});
	}

	return {
		url: url,
		host: host,
		port: port,

		type: $data('type') || ($data('confirmable', true) ? Type.Con : Type.Non),
		code: [0, $data('method', Method.GET)],
		options: options,
		payload: $data('payload'),

		created: Utils.now(),
		ack: false,
		res: false,

		onResponse: $data('onResponse'),
		onAck: $data('onAck'),
		onError: $data('onError')
	};
}

function findEndpoint(client, peer) {
	for (var i = 0, n = client.endpoints.length; i < n; i++) {
		var endpoint = client.endpoints[i];
		if (endpoint.peer == peer) return endpoint;
	}
}

function findRequest(client, response) {
	var token = response.token;
	var mid = response.messageId;

	for (var i = 0, n = client.requests.length; i < n; i++) {
		var request = client.requests[i];
		if (token && request.token && Bin.comp(token, request.token) == 0) return request;
		if (mid === request.messageId) return request;
	}
}

function invalidateClientRequest(client, request) {
	if (request.token) client.tokens.recycle(request.token);
	Utils.remove(client.requests, request);
}

function closeEndpoint(client, endpoint) {
	endpoint = Utils.remove(client.endpoints, endpoint);
	if (endpoint) {
		endpoint.close();
	}
}

function issueMessageId(client) {
	var mid = client.messageId++;
	if (client.messageId > 0xffff) client.messageId = 0;
	return mid;
}

function has(obj, slot) {
	return (obj && slot in obj && obj[slot]);
}

function createEndpoint(client, peer) {
	if (!client.sock) {
		client.sock = createSocket(client);
	}

	var endpoint = new Endpoint(peer, client.sock, {
		// callback for endpoint
		onResponse(endpoint, response) {
			var request = findRequest(client, response);
			if (!request || request['responseReceived']) return;

			request['responseReceived'] = Utils.now();

			if (has(request, 'onResponse')) {
				request.onResponse.call(client, response);
			} else if (has(client, 'onResponse')) {
				client.onResponse.call(client, response, request);
			}

			endpoint.invalidateRequest(request);
			if (endpoint.pendingRequests.length == 0) {
				closeEndpoint(client, endpoint);
			}
			invalidateClientRequest(client, request);
		},

		onAck(endpoint, response) {
			var request = findRequest(client, response);
			if (!request || request['ackReceived']) return;

			request['ackReceived'] = Utils.now();

			if (has(request, 'onAck')) {
				request.onAck.call(client);
			} else if (has(client, 'onAck')) {
				client.onAck.call(client, request);
			}

			return request;
		},

		onError(endpoint, err, request) {
			endpoint.pendingRequests.forEach(function(request) {
				endpoint.invalidateRequest(request);
				invalidateClientRequest(client, request);
			}, client);

			closeEndpoint(client, endpoint);

			if (has(request, 'onError')) {
				request.onError.call(client, err);
			} else if (has(client, 'onError')) {
				client.onError.call(client, err, request);
			}
		}
	});

	endpoint.logger = client.logger;
	client.log("CoAP.Clinet: endpoint created", endpoint)
	return endpoint;
}

// ----- Public -------

class Client {
	constructor() {
		this.sock = null;
		this.resolvings = [];
		this.endpoints = [];
		this.requests = [];
		this.tokens = new TokenRepository();
		this.messageId = Utils.rnd(0xffff);
		this.logger = null;
	}

	get logger() {
		return this._logger;
	}

	set logger(logger) {
		this._logger = logger;
		this.endpoints.forEach(endpoint => endpoint.logger = logger);
		if (this.sock) this.sock.logger = logger;
	}

	log() {
		if (this._logger) this._logger.log.apply(this._logger, arguments);
	}

	send(url, payload, callback) {
		var data;

		if (typeof url === 'object') {
			data = url;
		} else {
			data = {url: url};
			switch (typeof payload) {
				case 'function':
					data.onResponse = payload;
					break;

				case 'object':
					data.payload = payload;
					data.method = Method.POST;

					if (callback) data.onResponse = callback;
					break;
			}
		}

		var request = makeRequest(Utils.attr(data)), host = request.host;
		request.messageId = issueMessageId(this);
		request.token = this.tokens.issue();

		Socket.resolv(host, address => {
			if (address) {
				var peer = address + ':' + request.port;
				var endpoint = findEndpoint(this, peer);
				if (!endpoint) {
					endpoint = createEndpoint(this, peer);

					this.endpoints.push(endpoint);
				}

				endpoint.send(request);
			} else {
				var err = "resolve error";
				if (has(request, 'onError')) {
					request.onError.call(this, err);
				} else if (has(this, 'onError')) {
					this.onError.call(this, err, request);
				}
			}
		});

		this.requests.push(request);
		return request;
	}
}

// ======================================================
//  CoAP Server
// ======================================================

// ----- Private -------

// ----- Public -------

export class Server {
	constructor() {

	}
}

function t1() {
	function createResponse(request) {
		return {
			version: 'version' in request ? request.version : 1,
			type: request.type,
			code: [2, 5],
			messageId: request.messageId,
			token: request.token,
			options: [],
			payload: null
		};
	}

	coap_server = new ListeningSocket(null, 5000, "udp");
	coap_server.onConnect = function() { trace("onConnect [t1]\n"); };
	coap_server.onError = function() { trace("onError [t1]\n"); };
	coap_server.onClose = function() { trace("onClose [t1]\n"); };
	coap_server.onMessage = function(n) {
		trace("onMessage [t1] " + n + " bytes.\n");

		var blob = this.recv(n);
		if (!blob) return;

		var peer = coap_server.getPeer();
		debug.dump(peer);

		var message = coap.decodeMessage(blob);
		debug.dump(peer, blob, message, coap.encodeMessage(message));

		var response = createResponse(message);
		var encoded = coap.encodeMessage(response);
		debug.dump(encoded);
		coap_server.send(encoded, peer);

		blob.free();
	};
}

export default {
	Type,
	Method,
	Option,
	OptionFormat,
	Client,
};

