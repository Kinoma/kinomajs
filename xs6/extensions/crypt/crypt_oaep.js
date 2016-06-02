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

export default class OAEP {
	constructor(key, priv, H, P, MGF) {
		this.rsa = new Crypt.RSA(priv ? key.privExponent: key.exponent, key.modulus, key.prim1, key.prim2, key.exponent1, key.exponent2, key.coefficient);
		this.modulusSize = this.rsa.modulusSize;
		this.H = H ? H: new Crypt.SHA1();
		this.P = P ? P: new Chunk();
		this.MGF = MGF ? MGF: this.MGF1SHA1;
	};
	static MGF1SHA1(Z, l) {
		var H = new Crypt.SHA1();
		var hLen = H.outputSize;
		var T = new ArrayBuffer();
		for (var counter = 0; l > 0; counter++, l -= hLen) {
			var C = Crypt.PKCS1.sI2OSP(counter, 4);
			H.reset();
			H.update(Z, C);
			var bc = H.close();
			if (l < hLen)
				bc.length = l;
			T = T.concat(bc);
		}
		return(T);
	};
	xor(b1, b2) {
		if (!(b1 instanceof Uint8Array))
			b1 = new Uint8Array(b1);
		if (!(b2 instanceof Uint8Array))
			b2 = new Uint8Array(b2);
		for (var i = 0, len1 = b1.length, len2 = b2.length; i < len1 && i < len2; i++)
			b1[i] ^= b2[i];
	};
	emeEncode(M, P, emLen) {
		var H = this.H;
		var hLen = H.outputSize;
		// 3. Generate an octet string PS consisting of emLen-||M||-2hLen-1 zero octets.
		var psLen = emLen - M.length - 2*hLen - 1;
		if (psLen < 0)
			throw new Error("malformed input");
		var PS = new Uint8Array(psLen + 1);
		PS.fill(0);
		PS[psLen] = 0x01;
		// 4. Let pHash = Hash(P)
		H.reset();
		H.update(P);
		var pHash = H.close();
		// 5. Concatenate pHash, PS, the message M, ...
		var DB = new ArrayBuffer();
		DB = DB.concat(pHash, PS, M);
		// 6. Generate a random octet string seed of length hLen.
		var seed = Crypt.rng(hLen);
		// 7. Let dbMask = MGF(seed, emLen - hLen).
		var dbMask = this.MGF(seed, emLen - hLen);
		// 8. Let maskedDB = DB \xor dbMask.
		this.xor(DB, dbMask);
		var maskedDB = DB;
		// 9. Let seedMask = MGF(maskedDB, hLen).
		var seedMask = this.MGF(maskedDB, hLen);
		// 10. Let maskedSeed = seed \xor seedMask.
		this.xor(seed, seedMask);
		var maskedSeed = seed;
		// 11. Let EM = maskedSeed || maskedDB.
		var EM = maskedSeed;
		EM = EM.concat(maskedDB);
		return new Arith.Integer(EM);
	};
	ncomp(a, b, l) {
		for (var i = 0, len1 = a.length, len2 = b.length; i < l; i++) {
			if (i >= len1)
				return -1;
			if (i >= len2)
				return 1;
			if (a[i] != b[i])
				return a[i] - b[i];
		}
		return 0;
	};
	emeDecode(iEM, P, emLen) {
		var EM = new Uint8Array(iEM.toChunk(emLen));
		var H = this.H;
		var hLen = H.outputSize;
		var maskedSeed = EM.slice(0, hLen);
		var maskedDB = EM.slice(hLen);
		var seedMask = Crypt.PKCS1.MGF(maskedDB, hLen);
		this.xor(maskedSeed, seedMask);
		var dbMask = this.MGF(maskedSeed, emLen - hLen);
		this.xor(maskedDB, dbMask);
		var DB = maskedDB;
		var pHash = H.process(P);
		// check to see if the pHash equals the first hLen of DB
		if (this.ncomp(pHash, DB, hLen) != 0) {
			throw new Error("malformed input");
		}
		var c, i;
		for (i = hLen; i < DB.length && (c = DB[i]) == 0x00; i++)
			;
		if (c != 0x01) {
			throw new Error("malformed input");
		}
		return DB.slice(i+1);
	};
	encrypt(M) {
		var e = this.emeEncode(M, this.P, this.modulusSize - 1);
		var v = this.rsa.process(e);
		return Crypt.PKCS1.I2OSP(v, this.modulusSize);
	};
	decrypt(e) {
		var d = this.rsa.process(Crypt.PKCS1.OS2IP(e));
		return this.emeDecode(d, this.P, this.modulusSize - 1);
	};
};
