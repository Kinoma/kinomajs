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

import {ListeningSocket} from 'socket';
import {Endpoint} from 'endpoint';

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

export function Broker(port) {
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

export default Broker;
