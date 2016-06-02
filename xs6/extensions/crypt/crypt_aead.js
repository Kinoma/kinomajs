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
import Bin from "bin";

const version = 1;	// IETF draft chacha20-poly1305-1

export default class AEAD {
	constructor(key, nonce, aad, cipher, auth) {
		if (typeof nonce == "string")
			nonce = ArrayBuffer.fromString(nonce);
		this.cipher = new cipher(key, nonce);
		var zeros = new Uint8Array(auth.prototype.keySize);
		zeros.fill(0);
		var authKey = this.cipher.encrypt(zeros.buffer);	// encrypt 00 00 00...
		// this.cipher.setIV(nonce, 1);				// increment the counter no matter what the size of "zeros" is
		this.cipher = new cipher(key, nonce, 1);		// same as the above
		this.auth = new auth(authKey);
		if (aad === undefined || aad === null) {
			aad = "";
			this.aadLength = 0;
		}
		else
			this.aadLength = aad.byteLength;
		this.auth.update(aad);
		if (version == 1)
			this.padding(this.aadLength);
		else
			this.auth.update(this.i2os(this.aadLength));
		this.length = 0;
		this._macSize = this.auth.outputSize;
	};
	i2os(num) {
		var c = new Uint8Array(8);
		c.fill(0);
		for (var i = 0; i < 8 && num != 0; i++, num >>= 8)
			c[i] = num & 0xff;
		return c.buffer;
	};
	encrypt(input, n) {
		var output = this.cipher.encrypt(input, undefined, n);
		this.auth.update(output);
		this.length += output.byteLength;
		return output;
	};
	decrypt(input, n) {
		if (!n)
			n = input.byteLength;
		this.auth.update(input.slice(0, n));
		this.length += n;
		return this.cipher.decrypt(input, undefined, n);
	};
	close() {
		if (version == 1) {
			this.padding(this.length);
			this.auth.update(this.i2os(this.aadLength));
		}
		this.auth.update(this.i2os(this.length));
		var mac = this.auth.close();
		delete this.auth;
		delete this.cipher;
		return mac;
	};
	get macSize() {
		return this._macSize;
	};
	verify(msg) {
		var mac = this.close();
		return Bin.comp(mac, msg.slice(msg.byteLength - this.macSize)) == 0;
	};
	padding(len) {
		var n = (16 - (len % 16)) % 16;
		if (n > 0) {
			var c = new Uint8Array(n);
			c.fill(0);
			this.auth.update(c.buffer);
		}
	};
	process(data, encFlag) {
		if (encFlag) {
			var c = this.encrypt(data);
			return c.concat(this.close());
		}
		else {
			var c = this.decrypt(data, data.byteLength - this.macSize);
			return this.verify(data) ? c : undefined;
		}
	};
};
