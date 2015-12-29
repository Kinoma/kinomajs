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
import Crypt from "crypt";
import Arith from "arith";
import Bin from "bin";

export default class PKCS1_5 {
	constructor(key, priv, oid) {
		this.rsa = new Crypt.RSA(priv ? key.privExponent: key.exponent, key.modulus, key.prim1, key.prim2, key.exponent1, key.exponent2, key.coefficient);
		this.modulusSize = this.rsa.modulusSize;
		this.oid = oid;
	};
	emsaEncode(H, emLen) {
		if (this.oid)
			var bc = Crypt.BER.encode([0x30, [0x30, [0x06, this.oid], [0x05]], [0x04, H]]);
		else
			var bc = H;
		// prepend the prefix part
		var ffsize = emLen - bc.byteLength - 2;
		var s = new Uint8Array(ffsize + 2);
		var i = 0;
		s[i++] = 0x01;
		for (; i <= ffsize; i++)
			s[i] = 0xff;
		s[i++] = 0x00;
		return new Arith.Integer(s.buffer.concat(bc));
	};
	emsaDecode(EM) {
		var s = new Uint8Array(EM.toChunk());
		var i = 0;
		if (s[i++] != 0x01)
			return;		// not an encoded data??
		for (; i < s.length && s[i] == 0xff; i++)
			;
		if (i >= s.length || s[i] != 0x00)
			return;		// decode failed
		var bc = s.slice(i+1);
		if (this.oid) {
			var ber = Crypt.BER.decode(bc);
			if (!ber || (this.oid.length > 0 && ber[1][1][1].toString() != this.oid.toString()))
				return;		// oid doesn't match
			return ber[2][1];
		}
		else
			return bc.buffer;
	};
	emeEncode(M, emLen) {
		var pssize = emLen - M.byteLength - 2;
		if (pssize < 0)
			throw new Error("emeEncode malformed input");
		var s = new Uint8Array(pssize + 2);
		var ps = new Uint8Array(Crypt.rng(pssize));
		var i = 0;
		s[i++] = 0x02;
		for (var j = 0; j < ps.length; j++) {
			// make sure of nonzero
			var c = ps[j];
			if (c == 0)
				c = 0xff;
			s[i++] = c;
		}
		s[i++] = 0x00;
		return new Arith.Integer(s.buffer.concat(M));
	};
	emeDecode(EM) {
		var s = new Uint8Array(EM.toChunk());
		var i = 0;
		if (s[i++] != 0x02)
			return;		// not an encoded data??
		var c;
		for (; i < s.length; i++) {
			if (s[i] == 0x00)
				return s.slice(i + 1).buffer;
		}
		// decode failed
	};
	sign(H) {
		var f = this.emsaEncode(H, this.modulusSize-1);
		var v = this.rsa.process(f);
		return Crypt.PKCS1.I2OSP(v, this.modulusSize);
	};
	verify(H, sig) {
		var v = this.rsa.process(Crypt.PKCS1.OS2IP(sig));
		var R = this.emsaDecode(v);
		return !!R && Bin.comp(H, R) == 0;
	};
	encrypt(M) {
		var e = this.emeEncode(M, this.modulusSize-1);
		var v = this.rsa.process(e);
		return Crypt.PKCS1.I2OSP(v, this.modulusSize);
	};
	decrypt(e) {
		var d = this.rsa.process(Crypt.PKCS1.OS2IP(e));
		var M = this.emeDecode(d);
		if (!M)
			throw new Error("pkcs1_5: malformed input");
		return M;
	};
};
