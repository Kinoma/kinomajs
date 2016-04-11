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

/** @TODO
 * - resend QoS >= 1 packets when reconnect
 * - ping resp check. disconnect when no resp arrive.
 */

import {Port} from 'common';
import TimeInterval from "timeinterval";

export const State = {
	Disconnected: 0,
	Connecting: 1,
	Handshaking: 2,
	Established: 3,
};

// constructor
export class Client {
	constructor(clientIdentifier="", cleanSession=true) {
		if (!cleanSession && !clientIdentifier) throw "no clientIdentifier";

		this.clientIdentifier = clientIdentifier;
		this.cleanSession = cleanSession;
		this.state = State.Disconnected;

		this._pending = {};
		this.Endpoint = require.weak('endpoint');
	}

	connect(host='localhost', port=Port, options={}) {
		this.host = host;
		this.port = port;

		this.protocolLevel = options.protocolLevel ? options.protocolLevel : 4;
		this.keepAlive = ('keepAlive' in options) ? parseInt(options.keepAlive) : 60;
		this.userName = options.userName;
		this.password = options.password;

		if (options.will) {
			this.willTopic = options.will.topic;
			this.willMessage = options.will.message;
			this.willQos = options.will.qos;
			this.willRetain = options.will.retain;
		}

		this.reconnect();
	}

	reconnect() {
		if (!this.host || !this.port) throw "no host or port specified";

		let sock = this.sock = new this.Endpoint(this.host, this.port);

		sock.onConnect = () => {
			this.state = State.Handshaking;

			const pack = require.weak('message').packConnect;
			sock.send(pack(this));	// parameters are collected on connect() method.
		};

		sock.onPacket = (packet) => {
			try {
				this._handlePacket(packet);
			} catch (e) {
				this._handleError(e);
			}
		};

		sock.onError = () => {
			this._forceDisconnect(false);
		};

		sock.onClose = () => {
			this._forceDisconnect(false);
		};

		this.state = State.Connecting;
	}

	disconnect() {
		const pack = require.weak('message').packDisconnect;
		this._send(pack());

		this._forceDisconnect(true);
		this._stopKeepAlive();
	}

	subscribe(topicFilter, qos=0) {
		if (typeof topicFilter == 'string')
			topicFilter = [topicFilter];

		const pack = require.weak('message').packSubscribe;
		const packetId = this.sock.issuePacketId();
		this._send(pack(topicFilter, qos, packetId));
		return packetId;
	}

	unsubscribe(topicFilter) {
		if (typeof topicFilter == 'string')
			topicFilter = [topicFilter];

		const pack = require.weak('message').packUnsubscribe;
		const packetId = this.sock.issuePacketId();
		this._send(pack(topicFilter, packetId));
		return packetId;
	}

	publish(topic, payload=null, qos=0, retain=false) {
		const pack = require.weak('message').packPublish;
		const packetId = (qos > 0) ? this.sock.issuePacketId() : undefined;
		this._send(pack({
			topic,
			qos,
			retain,
			payload,
			packetId
		}));
		return packetId;
	}

	_callback(name, ...args) {
		if (this.behavior && this.behavior[name]) {
			this.behavior[name](this, ...args);
		}
	}

	_handlePacket({type, qos, retain, dup, body}) {
		switch (type) {
			case 2: /* CONNACK */ {
				let unpack = require.weak('message').unpackConnAck;
				let [returnCode, sessionPersist] = unpack(body);

				if (returnCode == 0) {
					this.state = State.Established;

					if (this.keepAlive > 0) this._startKeepAlive();
				}

				this._callback('onMQTTClientConnect', returnCode, sessionPersist);

				if (returnCode != 0) {
					this._forceDisconnect(false);
				}
				break;
			}
			case 3: /* PUBLISH */ {
				let unpack = require.weak('message').unpackPublish;
				let [topic, packetId, payload] = unpack(body, qos);

				if (qos < 2) {
					if (qos == 1) {
						let pack = require.weak('message').packPubAck;
						this._send(pack(packetId));
					}

					this._callback('onMQTTClientMessage', topic, payload, qos, retain, dup);
				} else {
					let pack = require.weak('message').packPubRec;
					this._send(pack(packetId));

					this._record(packetId, topic, payload, qos, retain, dup);
				}
				break;
			}
			case 4: /* PUBACK */
			case 7: /* PUBCOMP */ {
				let unpack = require.weak('message').unpackPubAck;

				let packetId = unpack(body);
				if (this.sock.ack(packetId)) {
					this._callback('onMQTTClientPublish', packetId);
				}
				break;
			}
			case 5: /* PUBREC */ {
				let unpack = require.weak('message').unpackPubRec;

				let packetId = unpack(body);
				if (this.sock.ack(packetId)) {
					let pack = require.weak('message').packPubRel;
					this._send(pack(packetId));
				}
				break;
			}
			case 6: /* PUBREL */ {
				let {unpackPubRel:unpack, packPubComp:pack} = require.weak('message');

				let packetId = unpack(body);
				this._send(pack(packetId));

				let result = this._release(packetId);
				if (result) {
					this._callback('onMQTTClientMessage', ...result);
				}
				break;
			}
			case 9: /* SUBACK */ {
				let unpack = require.weak('message').unpackSubAck;
				let [packetId, result] = unpack(body);

				if (this.sock.ack(packetId)) {
					this._callback('onMQTTClientSubscribe', packetId, result);
				}
				break;
			}
			case 11: /* UNSUBACK */ {
				let unpack = require.weak('message').unpackUnsubAck;
				let packetId = unpack(body);

				if (this.sock.ack(packetId)) {
					this._callback('onMQTTClientUnsubscribe', packetId);
				}
				break;
			}
			case 13: /* PINGRESP */ {
				break;
			}
			default:
				// error. throw everything away.
				this._handleError("Error: invalid packet type:" + type);
				this._forceDisconnect(false);
				return;
		}
	}

	_send(packet) {
		if (this.sock) this.sock.send(packet);
		if (this.keepAlive > 0 && this._timer) this._timer.start();
	}

	_handleError(err) {
		this._calback('onMQTTClientError', -1, err);
		console.log(err);
	}

	_forceDisconnect(cleanClose) {
		if (this.state == State.Disconnected) return;

		if (this.sock) {
			this.sock.close();
			delete this.sock;
		}

		this._callback('onMQTTClientDisconnect', cleanClose);
		this.state = State.Disconnected;
	}

	_record(packetId, ...message) {
		this._pending[packetId] = message;
	}

	_release(packetId) {
		if (this._pending[packetId]) {
			let result = this._pending[packetId];
			delete this._pending[packetId];
			return result;
		}
	}

	_startKeepAlive() {
		if (!this._timer) {
			this._timer = new TimeInterval(() => {
				const pack = require.weak('message').packPingReq;
				this._send(pack());
			}, this.keepAlive * 1000);
		}
		this._timer.start();
	}

	_stopKeepAlive() {
		if (this._timer) {
			this._timer.close();
			delete this._timer;
		}
	}
}

Client.State = State;

export default Client;
