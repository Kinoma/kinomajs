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
import Crypt from "crypt";

export default class HKDF {
	constructor(key, salt, digest) {
		this.hash = new digest();
		if (!salt) {
			salt = new Uint8Array(this.hash.outputSize);
			salt.fill(0);
			salt = salt.buffer;
		}
		var hmac = new Crypt.HMAC(this.hash, salt);
		hmac.update(key);
		this.prk = hmac.close();
	};
	process(len, info) {
		var hmac = new Crypt.HMAC(this.hash, this.prk);
		var res = new ArrayBuffer(0);
		var counter = new Uint8Array(1);
		var hashlen = this.hash.outputSize;
		var block = "";
		for (var i = 1, l = len; l > 0; l -= hashlen, i++) {
			hmac.reset();
			hmac.update(block);
			hmac.update(info);
			counter[0] = i & 0xff;
			hmac.update(counter.buffer);
			block = hmac.close();
			res = res.concat(block);
		}
		return len % hashlen == 0 ? res : res.slice(0, len);
	};
};
