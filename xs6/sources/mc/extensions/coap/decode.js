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
	OptionFormat,
	optionFormat,
	Message
} from 'common';

export function decode(blob) {
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
				optValue = String.fromArrayBuffer(readBytes(optLen));
				break;
		}
		options.push([opt, optValue]);
	}

	return {
		__proto__: Message,
		version: version,
		type: type,
		code: code,
		messageId: messageId,
		token: token,
		options: options,
		payload: payload
	};
}

export default decode;
