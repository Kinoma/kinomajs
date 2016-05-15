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
	Option,
	OptionFormat,
	optionFormat
} from 'common';

export function encode(message) {
	var version = ('version' in message ? message.version : 1);
	if (version !== 1) throw "Invalid version";

	var code = message.code || [0, 0];
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
		if (bytes.byteLength > 0)
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
	writeChr(((code[0] & 0x7) << 5) | (code[1] & 0x1f));
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

	if (payload && payload.byteLength > 0) {
		writeChr(0xff);
		writeBytes(payload);
	}

	return blob.buffer;
}

export default encode;

