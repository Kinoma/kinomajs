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

const bin = arr => (new Uint8Array(arr)).buffer;
const int = val => bin([((val >> 8) & 0xff), (val & 0xff)]);

export function encodeHeader({type, qos=0, retain=false, dup=false}, len) {
	let flags = type << 4;
	if (dup) flags |= 0b00001000;
	if (qos) flags |= (qos << 1);
	if (retain) flags |= 0b00000001;

	let header = [flags];
	let x = len;
	do {
		let e = len % 0x80;
		len = len >> 7;
		if (len > 0) e |= 0x80;
		header.push(e);
	} while (len > 0);

	return bin(header);
}

/**
 * encode sequence of values into binary.
 * values can be:
 * - array of Uint8 as sequence of byte flags
 * - ArrayBuffer as binary
 * - integer as Uint16
 * - string as string
 * - null, undefined as ignored
 */

export function encodeBody(body) {
	return body.map(elm => {
		if (Array.isArray(elm)) {
			// [Uint8]
			return bin(elm);
		} else if (elm instanceof ArrayBuffer) {
			// Binary
			return elm;
		} else if (typeof elm == 'number') {
			// Uint16
			return int(elm);
		} else if (typeof elm == 'string') {
			// string
			let str = ArrayBuffer.fromString(elm);
			return int(str.byteLength).concat(str);
		}
	}).reduce((prev, value) => {
		if (!prev) return value;
		if (!value) return prev;
		return prev.concat(value);
	}, null);
}

export function encode(packet) {
	let body = packet.body ? encodeBody(packet.body) : null;
	let header = encodeHeader(packet, body ? body.byteLength : 0);
	return body ? header.concat(body) : header;
}

encode.body = encodeBody;
encode.header = encodeHeader;

export default encode;
