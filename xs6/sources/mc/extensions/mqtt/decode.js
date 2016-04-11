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

export function decode(blob) {
	let bytes = new Uint8Array(blob);
	let multiplier = 1, length = 0, header = 1, c;
	do {
		if (bytes.length <= header) return [blob];

		c = bytes[header++];

		length += (c & 0x7f) * multiplier;
		multiplier *= 128;
		if (multiplier > 128 * 128 * 128) {
			return [bytes.slice(header)];
		}
	} while ((c & 0x80) != 0);

	if (bytes.length < (header + length)) return [blob];

	let type = (bytes[0] >> 4) & 0xf;
	let dup = (bytes[0] >> 3) & 1;
	let qos = (bytes[0] >> 1) & 3;
	let retain = bytes[0] & 1;

	return [
		blob.slice(header + length),
		{
			type,
			dup,
			qos,
			retain,
			body: (length > 0 ? blob.slice(header, header + length) : null)
		},
	];
};

export default decode;
