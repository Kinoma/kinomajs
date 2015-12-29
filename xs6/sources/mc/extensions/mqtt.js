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
import {Logger} from 'utils/logging';

function BlobReader() {
	this.blobs = [];
	this.remains = 0;
	this.pos = 0;
}

BlobReader.prototype = {
	feed: function(blob) {
		this.blobs.push(new DataView(blob));
		this.remains += blob.byteLength;
	},
	read: function(n) {
		if (this.blobs.length == 0 || this.remains < n) return undefined;

		var result = new Array(n);
		this.remains -= n;
		var p = 0;
		while (p < n && this.blobs.length > 0) {
			var blob = this.blobs[0];
			var pos = this.pos;
			for (var c = blob.byteLength; pos < c && p < n; pos++, p++) {
				result[p] = blob.getUint8(pos);
			}

			if (pos < blob.byteLength) {
				this.pos = pos;
			} else {
				this.blobs.shift();
				this.pos = 0;
			}
		}

		return result;
	},
};

function Encoder() {
	this.bytes = [];
};

Encoder.prototype = {
	pokeInto: function(blob, offset) {
		var bytes = this.bytes;
		var view = new DataView(blob, offset);
		for (var i = 0, c = bytes.length; i < c; i++) {
			view.setUint8(i, bytes[i]);
		}
	},
	pushByte: function(b) {
		this.bytes.push(b);
	},
	pushUInt16: function(val) {
		this.bytes.push((val >> 8) & 0xff);
		this.bytes.push(val & 0xff);
	},
	get length() {
		return this.bytes.length;
	},
};

var util = {
	ptr(str) {
		return function(a,b) { return b === undefined ? str.charAt(a) : str.substring(a,b)}
	},
	attr(obj) {
		return function(name, fallback) { return (name in obj) ? obj[name] : fallback; };
	},
	rnd(n, m) {
		var start, size;
		if (m === undefined) {
			start = 0;
			size = n;
		} else {
			start = n;
			size = m - n;
		}
		return start + ((Math.random() * (size + 1)) | 0);
	},
	pow(n, m) {
		var val = 1;
		while (m-- > 0) val *= n;
		return val;
	},
	ref() {
		return new ReferenceKeeper();
	},
	now() {
		return (new Date()).getTime();
	},
	remove(a, e) {
		var pos = a.indexOf(e);
		if (pos >= 0) return a.splice(pos, 1)[0];
	},
	arrayToBlob(bytes) {
		var length = bytes.length;
		var blob = new ArrayBuffer(length);
		var view = new DataView(blob);
		for (var i = 0; i < length; i++) {
			view.setUint8(i, bytes[i]);
		}
		return blob;
	},
	BlobReader,
	Encoder,
};

var Port = 1883;

var Type = {
	CONNECT: 1,
	CONNACK: 2,
	PUBLISH: 3,
	PUBACK: 4,
	PUBREC: 5,
	PUBREL: 6,
	PUBCOMP: 7,
	SUBSCRIBE: 8,
	SUBACK: 9,
	UNSUBSCRIBE: 10,
	UNSUBACK: 11,
	PINGREQ: 12,
	PINGRESP: 13,
	DISCONNECT: 14
};

var notImplemented = 'not implemented';
var badPacket = 'bad packet';
var badArgs = 'bad args';
var serial = 1;

// Packet Reader =======================

function readPacket() {
	var bytes, c;
	var packet = this.packet;
	var tmp = this.tmp;

	if (packet.packetType === undefined) {
		if ((c = this.readByte()) === undefined) return;

		packet.packetType = (c >> 4) & 0x0f;

		tmp.fn = packetReaders[packet.packetType];
		if (!tmp.fn) throw badPacket;

		packet.flags = c & 0x0f;
		packet.dup = !!((c >> 3) & 0x01);
		packet.qos = (c >> 1) & 0x03;
		packet.retain = !!(c & 0x01);

		tmp.fn.call(this, packet, true); // initialize
	}

	if (packet.remainingLength === undefined) {
		do {
			if ((c = this.readByte()) === undefined) return;
			tmp.value += (c & 0x7f) * tmp.multiplier;
			tmp.multiplier *= 128;
			if (tmp.multiplier > 128 * 128 * 128) throw badPacket;
		} while ((c & 0x80) != 0);

		packet.remainingLength = tmp.remainingLength = tmp.value;
	}

	if (!tmp.fn.call(this, packet)) return;

	this.resetStates();
	return packet;
}

var packetReaders = [
	null,
	function CONNECT(p, initialize) {
		if (initialize) {
			p.protocolName = undefined;
			p.protocolLevel = undefined;
			p.connectFlags = undefined;
			p.keepAlive = undefined;
			p.clientIdentifier = undefined;
			p.willTopic = undefined;
			p.willMessage = undefined;
			p.userName = undefined;
			p.password = undefined;
			return;
		}

		if (p.protocolName === undefined) {
			if ((p.protocolName = this.readString()) === undefined) return;
		}

		if (p.protocolLevel === undefined) {
			if ((p.protocolLevel = this.readByte()) === undefined) return;
		}

		if (p.connectFlags === undefined) {
			var f;
			if ((f = this.readByte()) === undefined) return;

			p.connectFlags = f;
			p.cleanSession = !!((f >> 1) & 0x1);
			p.willFlag = !!((f >> 2) & 0x1);
			p.willQoS = (f >> 3) & 0x3;
			p.willRetain = !!((f >> 5) & 0x1);
			p.userNameFlag = !!((f >> 6) & 0x1);
			p.passwordFlag = !!((f >> 7) & 0x1);

			if (
				// 3.1.2.6
				(!p.willFlag && p.willQoS > 0)
				// 3.1.2.7
			 || (!p.willFlag && p.willRetain)
				// 3.1.2.9
			 || (!p.userNameFlag && p.passwordFlag)
				) throw badPacket;

			if (!p.willFlag) p.willTopic = p.willMessage = null;
			if (!p.userNameFlag) p.userName = null;
			if (!p.passwordFlag) p.password = null;
		}

		if (p.keepAlive === undefined) {
			if ((p.keepAlive = this.readUInt16()) === undefined) return;
		}

		if (p.clientIdentifier === undefined) {
			var blob;
			if ((blob = this.readBlob()) === undefined) return;

			var l = blob.byteLength;
			if (l < 1 || l > 23) throw badPacket;
			p.clientIdentifier = String.fromArrayBuffer(blob);
		}

		if (p.willTopic === undefined) {
			if ((p.willTopic = this.readString()) === undefined) return;
		}

		if (p.willMessage === undefined) {
			if ((p.willMessage = this.readBlob()) === undefined) return;
		}

		if (p.userName === undefined) {
			if ((p.userName = this.readString()) === undefined) return;
		}

		if (p.password === undefined) {
			if ((p.password = this.readBlob()) === undefined) return;
		}
		return true;
	},
	function CONNACK(p, initialize) {
		if (initialize) {
			p.connectAcknowledgeFlags = undefined;
			p.connectReturnCode = undefined;
			return;
		}

		if (p.connectAcknowledgeFlags === undefined) {
			var bytes = this.readBytes(2);
			if (bytes === undefined) return;

			p.connectAcknowledgeFlags = bytes[0];
			p.connectReturnCode = bytes[1];
		}
		return true;
	},
	function PUBLISH(p, initialize) {
		var tmp = this.tmp;

		if (initialize) {
			p.topicName = undefined;
			p.packetIdentifier = (p.qos >= 1 ? undefined : null);
			p.payload = undefined;
			tmp.payloadSize = undefined;
			return;
		}

		if (p.topicName === undefined) {
			if ((p.topicName = this.readString()) === undefined) return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		if (p.payload === undefined) {
			if (tmp.payloadSize === undefined) {
				tmp.payloadSize = tmp.remainingLength;
			}

			var payload;
			if ((payload = this.readBytes(tmp.payloadSize)) === undefined) return;
			p.payload = util.arrayToBlob(payload);
		}

		return true;
	},
	function PUBACK(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function PUBREC(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function PUBREL(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function PUBCOMP(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function SUBSCRIBE(p, initialize) {
		var tmp = this.tmp;

		if (initialize) {
			p.packetIdentifier = undefined;
			p.topics = [];
			tmp.topicName = undefined;
			tmp.qos = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		while (this.tmp.remainingLength > 0) {
			if (tmp.topicName === undefined) {
				tmp.topicName = this.readString();
				if (tmp.topicName === undefined) return;
			}

			if (tmp.qos === undefined) {
				tmp.qos = this.readByte();
				if (tmp.qos === undefined) return;
			}

			p.topics.push([tmp.topicName, tmp.qos]);

			tmp.topicName = undefined;
			tmp.qos = undefined;
		}

		return true;
	},
	function SUBACK(p, initialize) {
		if (initialize) {
			p.packetIdentifier = undefined;
			p.topics = [];
			tmp.qos = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		while (this.tmp.remainingLength > 0) {
			if (tmp.qos === undefined) {
				tmp.qos = this.readByte();
				if (tmp.qos === undefined) return;
			}

			p.topics.push([tmp.topicName, tmp.qos]);

			tmp.qos = undefined;
		}

		return true;
	},
	function UNSUBSCRIBE(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function UNSUBACK(p, initialize) {
		throw notImplemented;

		if (initialize) {
			p.packetIdentifier = undefined;
			return;
		}

		if (p.packetIdentifier === undefined) {
			if ((p.packetIdentifier = this.readUInt16()) === undefined) return;
		}

		return true;
	},
	function PINGREQ(p, initialize) {
		return true;
	},
	function PINGRESP(p, initialize) {
		return true;
	},
	function DISCONNECT(p, initialize) {
		return true;
	},
];

function PacketReader() {
	util.BlobReader.call(this);
	this.resetStates();
}

PacketReader.prototype = Object.create(util.BlobReader.prototype, {
	packet: { value: null, writable: true, enumerable: true },
	tmp: { value: null, writable: true, enumerable: true },
	bytes: { value: null, writable: true, enumerable: true },
	resetStates: { value: function() {
		this.packet = {
			packetType: undefined,
			flags: undefined,
			remainingLength: undefined,
		};
		this.tmp = {
			remainingLength: 0,
			blobSize: undefined,

			// prepare for remainingLength
			multiplier: 1,
			value: 0,
		};
	}},
	readBytes: { value: function(n) {
		var bytes = this.read(n);
		this.tmp.remainingLength -= n;
		return bytes;
	}},
	readByte: { value: function() {
		var bytes = this.read(1);
		if (bytes === undefined) return;
		this.tmp.remainingLength -= 1;
		return bytes[0];
	}},
	readUInt16: { value: function() {
		var bytes = this.read(2);
		if (bytes === undefined) return;
		this.tmp.remainingLength -= 2;
		return (bytes[0] << 8) + bytes[1];
	}},
	readSizedBlob: { value: function() {
		if (this.tmp.blobSize === undefined) {
			if ((this.tmp.blobSize = this.readUInt16()) === undefined) return;
		}

		var bytes = this.read(this.tmp.blobSize);
		if (bytes === undefined) return;

		this.tmp.remainingLength -= this.tmp.blobSize;
		this.tmp.blobSize = undefined;
		return util.arrayToBlob(bytes);
	}},
	readBlob: { value: function() {
		var blob = this.readSizedBlob();
		if (blob === undefined) return;

		return blob;
	}},
	readString: { value: function() {
		var blob = this.readSizedBlob();
		if (blob === undefined) return;

		return String.fromArrayBuffer(blob);
	}},
});

// Packet Encoder =======================

var serializers = [
	null,
	function CONNECT(p, encoder) {
		var $p = util.attr(p);
		var level = $p('level', 4), name;

		switch (level) {
			case 3:
				name = 'MQIsdp';
				break;
			case 4:
				name = 'MQTT';
				break;
			default:
				throw badArgs;
		}

		encoder.pushString(name);
		encoder.pushByte(level);
		encoder.pushByte(flags);
		encoder.pushUInt16(p.keepAlive);
		encoder.pushString(p.clientIdentifier);

		var flags = 0;
	},
	function CONNACK(p, encoder) {
		encoder.pushByte(0 + p.connectAcknowledgeFlags);
		encoder.pushByte(p.connectReturnCode);
	},
	function PUBLISH(p, encoder) {
		if (p.qos > 0 && p.packetIdentifier === undefined) {
			p.packetIdentifier = this.packetIdentifier++;
		}

		encoder.pushString(p.topicName);
		if (p.qos > 0) encoder.pushUInt16(p.packetIdentifier);
		encoder.pushBlob(p.payload);
	},
	function PUBACK(p, encoder) {
		throw notImplemented;

	},
	function PUBREC(p, encoder) {
		throw notImplemented;

	},
	function PUBREL(p, encoder) {
		throw notImplemented;

	},
	function PUBCOMP(p, encoder) {
		throw notImplemented;

	},
	function SUBSCRIBE(p, encoder) {
		throw notImplemented;

	},
	function SUBACK(p, encoder) {
		encoder.pushUInt16(p.packetIdentifier);
		p.returnCodes.forEach(function(code) {
			encoder.pushByte(code);
		});
	},
	function UNSUBSCRIBE(p, encoder) {
		throw notImplemented;

	},
	function UNSUBACK(p, encoder) {
		throw notImplemented;

	},
	function PINGREQ(p, encoder) {
	},
	function PINGRESP(p, encoder) {
	},
	function DISCONNECT(p, encoder) {
	},
];

function PacketEncoder() {
	util.Encoder.call(this);
}
PacketEncoder.prototype = Object.create(util.Encoder.prototype, {
	pushString: { value: function(s) {
		var blob = ArrayBuffer.fromString(s);

		var len = blob.byteLength;
		this.pushUInt16(len);
		this.pushBlob(blob);
	}},
	pushBlob: { value: function(blob) {
		var view = new DataView(blob);
		var len = view.byteLength;
		for (var i = 0; i < len; i++) {
			this.pushByte(view.getUint8(i));
		}
	}},
});

function Endpoint(host, port, delegate) {
	Socket.call(this, { host, port, proto: "tcp" });
	this.init(delegate);
	this.logger = null;
}

Endpoint.prototype = Object.create(Socket.prototype, {
	delegate: { value: null, writable: true, enumerable: true },
	reader: { value: null, writable: true, enumerable: true },
	init: { value: function(delegate) {
		this.delegate = delegate;
		this.packetIdentifier = 1;
	}},
	log: { value: function() { if (this.logger) this.logger.log.apply(this.logger, arguments); }},
	onError: { value: function() {
		this.log("onError");
	}},
	onClose: { value: function() {
		this.log("onClose");
		this.delegate.onClose(this);
	}},
	onMessage: { value: function(n) {
		if (!this.reader) {
			this.reader = new PacketReader();
		}

		var reader = this.reader, delegate = this.delegate;

		var blob = this.recv(n > 128 ? 128 : n);
		if (!blob) return;

		this.log('Receiving blob: ', blob);
		reader.feed(blob);

		try {
			var packet;
			while (!!(packet = readPacket.call(reader))) {
				this.log("Got a packet from", this.peer, packet);

				var fn = delegate.getHandler(packet.packetType);
				if (fn) {
					fn.call(delegate, this, packet);
				} else {
					this.log("ERROR: cannot handle packet: ", packet);
					delegate.onClose(this);
				}
			}
		} catch (e) {
			this.log("Error:", e);
			delegate.onClose(this);
		}
	}},
	sendPacket: { value: function(packet) {
		this.log("Sending a packet to", this.peer, packet);

		var fn = serializers[packet.packetType];
		var body = new PacketEncoder();
		fn.call(this, packet, body);

		var header = new PacketEncoder();

		var flag = packet.packetType << 4;
		if ('qos' in packet) flag |= ((packet.qos & 0x3) << 1);
		if ('retain' in packet && packet.retain) flag |= 1;
		if ('dup' in packet && packet.dup) flag |= (1 << 3);
		header.pushByte(flag);

		// 2.2.3 Remaining Length
		var x = body.length;
		do {
			var e = x % 0x80;
			x = x >> 7;
			if (x > 0) {
				e |= 0x80;
			}
			header.pushByte(e);
		} while (x > 0);

		var blob = new ArrayBuffer(header.length + body.length);
		var pos = 0;
		header.pokeInto(blob, 0);
		body.pokeInto(blob, header.length);

		this.log('Sending blob:', blob);
		this.send(blob);
	}},
});

// Broker =====================

var Broker = (function() {
	// private methods =====================

	function establishConnection(s) {
		this.log("establishConnection");

		// got a new connection
		var nsock = Object.create(Endpoint.prototype, {
			filters: { value: [], enumerable: true },
			logger: { writable: true, enumerable: true, value: this.logger },
		});
		nsock.init(this);

		s.accept(nsock);

		this.endpoints.push(nsock);
	}

	function handleListenerError(e) {
		this.log("onError:", e);
	}

	function listerClosed() {
		this.log("onClose");
	}

	function closeEndpoint(sock) {
		sock = util.remove(this.endpoints, sock);
		if (sock) sock.close();
	}

	function matchTopic(filter, topic) {
		return filter == topic;
	}

	function publish(sock, topic, retain, qos, payload) {
		// @TODO
		this.endpoints.forEach(function(endpoint) {
			endpoint.filters.forEach(function(pair) {
				var filter = pair[0];
				if (matchTopic(filter, topic)) {
					endpoint.sendPacket({
						packetType: Type.PUBLISH,
						qos: 0,
						topicName: topic,
						payload: payload,
					});
				}
			}, this)
		}, this);
	}

	function subscribeTopic(sock, topicFilter, qos) {
		unsubscribeTopic.call(this, sock, topicFilter);
		sock.filters.push([topicFilter, qos]);
		// @TODO send matching retained messages
		return qos;
	}

	function unsubscribeTopic(sock, topicFilter) {
		var found;

		sock.filters.some(function(pair, index) {
			if (pair[0] == topicFilter) {
				found = index;
				return true;
			}
		});

		if (found !== undefined) {
			sock.filters.splice(found, 1);
		}
	}

	var brokerHandlers = [
		null,
		function CONNECT(sock, packet) {
			this.log("connect:", packet.clientIdentifier);

			sock.sendPacket({
				packetType: Type.CONNACK,
				connectAcknowledgeFlags: false,
				connectReturnCode: 0,
			});
		},
		null, /* CONNACK */
		function PUBLISH(sock, packet) {
			this.log("publish:", packet.topicName, "data:", String.fromArrayBuffer(packet.payload));
			publish.call(this, sock, packet.topicName, packet.retain, packet.qos, packet.payload);
		},
		null, /* PUBACK */
		null, /* PUBREC */
		null, /* PUBREL */
		null, /* PUBCOMP */
		function SUBSCRIBE(sock, packet) {
			this.log("subscribe:", packet.topics);

			var returnCodes = packet.topics.map(function(pair) {
				return subscribeTopic.call(this, sock, pair[0], pair[1]);
			}, this);

			sock.sendPacket({
				packetType: Type.SUBACK,
				packetIdentifier: packet.packetIdentifier,
				returnCodes: returnCodes,
			});
		},
		null, /* SUBACK */
		function UNSUBSCRIBE(sock, packet) {
		},
		null, /* UNSUBACK */
		function PINGREQ(sock, packet) {
			this.log("ping");

			sock.sendPacket({
				packetType: Type.PINGRESP,
			});
		},
		null, /* PINGRESP */
		function DISCONNECT(sock, packet) {
			this.log("disconnect");
			this.onClose(sock);
		},
	];

	// constructor =====================

	function Broker(port) {
		this.port = port || Port;
		this.endpoints = [];
		this.logger = null;

		var sock = new ListeningSocket({
			proto: "tcp",
			port: this.port,

		});
		this.sock = sock;

		var broker = this;
		sock.onConnect = function() { establishConnection.call(broker, this); };
		sock.onError = function(e) { handleListenerError.call(broker, e); };
		sock.onClose = function() { listerClosed.call(broker); };
	}

	// Broker public interface =====================

	Broker.prototype = Object.create({}, {
		port: { value: null, writable: true, enumerable: true },
		sock: { value: null, writable: true, enumerable: true },
		endpoints: { value: null, writable: true, enumerable: true },
		log: { value: function() { if (this.logger) this.logger.log.apply(this.logger, arguments); }},
		setLogger: { value: function(logger) {
			this.logger = logger;
			this.endpoints.forEach(sock => sock.logger = logger );
		}},

		getHandler: { value: function(type) {
			return brokerHandlers[type];
		}},
		onClose: { value: function(sock) {
			closeEndpoint.call(this, sock);
		}},
	});

	return Broker;
})();

// Client =====================

var Client = (function() {
	// private methods

	// constructor
	function Client(host, port, clientIdentifier) {
		this.host = host || 'localhost';
		this.port = port || Port;
		this.logger = null;

		var cleanSession = !(clientIdentifier);
		if (cleanSession) clientIdentifier = ('kinoma-fsk-' + serial++);
		this.clientIdentifier = clientIdentifier;

		var sock = new Endpoint(this.host, this.port, this);
		this.sock = sock;

		var client = this;

		sock.onConnect = function() {
			var level = 4, name = 'MQTT', flags = 2, keepAlive = 60;

			this.sendPacket({
				packetType: Type.CONNECT,
				clientIdentifier: clientIdentifier,
				protocolName: name,
				protocolLevel: level,
				keepAlive: keepAlive,
			});
		};
	}

	// public methods
	Client.prototype = Object.create({}, {
		subscribe: { value: function(topicFilter, qos) {

		}},
		unsubscribe: { value: function(topicFilter) {

		}},
		publish: { value: function(topic, payload, qos, retain) {
			if (qos > 0 || retain) throw notImplemented;

			this.sock.sendPacket({
				packetType: Type.PUBLISH,
				qos: qos,
				retain: retain,
				topicName: topic,
				payload: payload,
			});
		}},
		log: { value: function() { if (this.logger) this.logger.log.apply(this.logger, arguments); }},
		setLogger: { value: function(logger) {
			this.sock.logger = this.logger = logger;
		}},

		getHandler: { value: function(type) {
			return clientHandlers[type];
		}},
		onClose: { value: function(sock) {
			closeEndpoint.call(this, sock, false);
		}},

		// override point
		onConnect: { enumerable: true, writable: true, value: function(cleanSession) {
		}},

		onMessage: { enumerable: true, writable: true, value: function(packet) {
		}},
	});

	function closeEndpoint(sock, graceful) {
		if (sock === this.sock) sock.close();
		this.sock = null;
	}

	var clientHandlers = [
		null,
		null, /* CONNECT */
		function CONNACK(sock, packet) {
			this.log("conn ack:", packet.connectReturnCode);

			this.onConnect(true);
		},
		function PUBLISH(sock, packet) {
			this.log("got message:", packet.topicName, "data:", String.fromArrayBuffer(packet.payload));
			this.onMessage(this, packet.topicName, packet.payload, packet);
		},
		null, /* PUBACK */
		null, /* PUBREC */
		null, /* PUBREL */
		null, /* PUBCOMP */
		null, /* SUBSCRIBE */
		function SUBACK(sock, packet) {
			this.log("subscribe done");
		},
		null, /* UNSUBSCRIBE */
		function UNSUBACK(sock, packet) {
			this.log("unsubscribe done");
		},
		null, /* PINGREQ */
		function PINGRESP(sock, packet) {
			this.log("pong");
		},
		null, /* DISCONNECT */
	];

	return Client;
})();

var broker;
var client;

export default {
	Port,
	Type,
	Broker,
	Client,

	test_server() {
		require.busy = true;

		// Broker test

		broker = new Broker();
		broker.setLogger(new Logger("MQTT Broker"));
	},

	test_client(host) {
		require.busy = true;

		if (!host) host = 'localhost';

		// Client test
		client = new Client(host);
		client.setLogger(new Logger("MQTT Client:" + host));

		client.onConnect = function(cleanSession) {
			this.log("Connected!!!");

			this.subscribe('hello');
			this.publish('hello', ArrayBuffer.fromString("Hello from device!"));
		};

		client.onMessage = function(topic, payload) {
			this.log([topic, String.fromArrayBuffer(payload)]);
		};
	},

};

