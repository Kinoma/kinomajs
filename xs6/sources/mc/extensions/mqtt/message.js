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
const Type = {
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

const bin = (val) => {
	if (!val || val instanceof ArrayBuffer) return val;
	return ArrayBuffer.fromString(`${val}`);
};

const readInt = (blob, offset) => {
	let view = new Uint8Array(blob, offset, 2);
	let value = (view[0] << 8) + view[1];
	return [value, offset + 2];
};

const readBin = (blob, offset, length) => {
	let end = offset + length;
	return [blob.slice(offset, end), end];
};

const readStr = (blob, offset) => {
	let length, value;
	[length, offset] = readInt(blob, offset);
	[value, offset] = readBin(blob, offset, length);
	return [String.fromArrayBuffer(value), offset];
};

const withPid = (type, packetId) => {
	return {
		type,
		packetId,
		body: [
			packetId,
		]
	};
}


// --------------------------------- CONNECT: 1,

export function packConnect({
	protocolLevel=4,
	keepAlive=60,
	cleanSession=true,
	clientIdentifier='',
	willTopic,
	willMessage,
	willQos=0,
	willRetain=false,
	userName,
	password
}) {
	if (!(protocolLevel == 3 || protocolLevel == 4)) throw "invalid level";
	if (!cleanSession && !clientIdentifier) throw "invalid clientIdentifier";

	willMessage = bin(willMessage);
	password = bin(password);

	let protocolName = protocolLevel == 4 ? 'MQTT' : 'MQIsdp';
	let flags = 0;

	if (cleanSession) flags |= 0b00000010;
	if (willTopic) {
		flags |= 0b00000100;
		flags |= ((willQos & 0b11) << 3);
		if (willRetain) flags |= 0b00100000;
	}
	if (password) flags |= 0b01000000;
	if (userName) flags |= 0b10000000;

	return {
		type: Type.CONNECT,
		body: [
			protocolName,		// protocol name (string)
			[
				protocolLevel,	// protocol level (byte)
				flags,			// connect flags (flags)
			],
			keepAlive,			// keepAlive (uint16)
			clientIdentifier,	// clientIdentifier (string)
			willTopic,			// will topic (string) : if willFlag
			willTopic ? (willMessage ? willMessage.byteLength : [0, 0]) : null,
								// will message length (uint16) : if willFlag
			willTopic ? willMessage : null,
								// will message (binary) : if willFlag
			userName,			// user name (string) : if userNameFlag
			password ? password.byteLength : null,
								// password length (uint16) : if passwordFlag
			password,			// password (binary) : if passwordFlag
		],
	};
}

export function unpackConnect(body) {
}

// --------------------------------- CONNACK: 2,

export function packConnAck() {

}

export function unpackConnAck(body) {
	return [
		0,		// returnCode
		false	// sessionPersist
	];
}

// --------------------------------- PUBLISH: 3,

export function packPublish({
	topic,
	qos=0,
	retain=false,
	payload=null,
	packetId=undefined,
}) {
	return {
		type: Type.PUBLISH,
		qos,
		retain,
		packetId,
		body: [
			topic,
			packetId,
			bin(payload),
		],
	};
}

export function unpackPublish(body, qos) {
	let topic, packetId, payload, offset;
	[topic, offset] = readStr(body, 0);
	if (qos > 0) [packetId, offset] = readInt(body, offset);
	[payload] = readBin(body, offset, body.byteLength - offset);

	return [
		topic,
		packetId,
		payload
	];
}

// --------------------------------- PUBACK: 4,

export function packPubAck(packetId) {
	return withPid(
		Type.PUBACK,
		packetId
	);
}

export function unpackPubAck(body) {
	let packetId, offset;
	[packetId, offset] = readInt(body, 0);

	return packetId;
}

// --------------------------------- PUBREC: 5,

export function packPubRec(packetId) {
	return withPid(
		Type.PUBREC,
		packetId
	);
}

export const unpackPubRec = unpackPubAck;

// --------------------------------- PUBREL: 6,

export function packPubRel(packetId) {
	let packet = withPid(
		Type.PUBREL,
		packetId
	);
	packet.qos = 1;
	return packet;
}

export const unpackPubRel = unpackPubAck;

// --------------------------------- PUBCOMP: 7,

export function packPubComp(packetId) {
	return withPid(
		Type.PUBCOMP,
		packetId
	);
}

export const unpackPubComp = unpackPubAck;

// --------------------------------- SUBSCRIBE: 8,

export function packSubscribe(topicFilters, qos, packetId) {
	let body = [packetId];
	for (let topicFilter of topicFilters) {
		body.push(topicFilter, [qos]);
	}

	return {
		type: Type.SUBSCRIBE,
		qos: 1,
		packetId,
		body,
	};
}

export function unpackSubscribe(body) {

}

// --------------------------------- SUBACK: 9,

export function packSubAck() {
}

export function unpackSubAck(body) {
	let packetId, result, offset;
	[packetId, offset] = readInt(body, 0);
	result = Array.from(new Uint8Array(body, 2));

	return [
		packetId,
		result,
	];
}

// --------------------------------- UNSUBSCRIBE: 10,

export function packUnsubscribe(topicFilters, packetId) {
	return {
		type: Type.UNSUBSCRIBE,
		qos: 1,
		packetId,
		body: [packetId].concat(topicFilters),
	};
}

export function unpackUnsubscribe(body) {

}

// --------------------------------- UNSUBACK: 11,

export function packUnsubAck() {

}

export const unpackUnsubAck = unpackPubAck;

// --------------------------------- PINGREQ: 12,

export function packPingReq() {
	return {
		type: Type.PINGREQ,
	};
}

// export function unpackPingReq(body) {
// }

// --------------------------------- PINGRESP: 13,

export function packPingResp() {
	return {
		type: Type.PINGRESP,
	};
}

// export function unpackPingResp(body) {
// }

// --------------------------------- DISCONNECT: 14,

export function packDisconnect() {
	return {
		type: Type.DISCONNECT,
	};
}

// export function unpackDisconnect(body) {
// }

// ---------------------------------

export default {
	packConnect,
	unpackConnect,

	packConnAck,
	unpackConnAck,

	packPublish,
	unpackPublish,

	packPubAck,
	unpackPubAck,

	packPubRec,
	unpackPubRec,

	packPubRel,
	unpackPubRel,

	packPubComp,
	unpackPubComp,

	packSubscribe,
	unpackSubscribe,

	packSubAck,
	unpackSubAck,

	packUnsubscribe,
	unpackUnsubscribe,

	packUnsubAck,
	unpackUnsubAck,

	packPingReq,
	// unpackPingReq,

	packPingResp,
	// unpackPingResp,

	packDisconnect,
	// unpackDisconnect
};

