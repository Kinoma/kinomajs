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

const secretSize = 32;

export default class SRPServer {
	constructor(g, m, hash) {
		this.g = g || new Arith.Integer(5);
		this.m = m || new Arith.Integer(this._load_default_modulus());
		this.mod = new Arith.Mont({m: this.m, method: Arith.Mont.SW, sw_param: 3});
		this.z = this.mod.z;
		this.hash = hash || Crypt.SHA1;
	};
	generate(salt, username, password) {
		// x = SHA(<salt> || SHA(<username>:<password>))
		var sha = new this.hash();
		var u = sha.process(username, ":", password);
		var x = new Arith.Integer(sha.process(salt, u));
		// <password verifier> = v = g^x % N
		this.v = this.mod.exp(this.g, x);
		// keep username and salt to verify the proof
		this.username = username;
		this.salt = salt;
		return this.v.toChunk();
	};
	_getPub(v, k) {
		// B = (kv + G^b) % N
		if (!this.secret)
			this.secret = new Arith.Integer(Crypt.rng(secretSize));
		var B = this.z.add(this.z.mul(k, v), this.mod.exp(this.g, this.secret));
		return this.mod.mod(B);
	};
	getPub6a(v) {
		var v = v ? new Arith.Integer(v) : this.v;
		var sha = new this.hash();
		var k = new Arith.Integer(sha.process(this.m.toChunk(), this.g.toChunk(this.m.sizeof()))); // pad g to the modulus size
		var B = this._getPub(v, k);
		this.pubSelf = B.toChunk();
		return this.pubSelf;
	};
	MGF1SHA1(Z, l) {
		var H = new Crypt.SHA1();
		var hLen = H.outputSize;
		var T = new ArrayBuffer();
		for (var counter = 0; l > 0; counter++, l -= hLen) {
			var C = this.sI2OSP(counter, 4);
			var bc = H.process(Z, C);
			if (l < hLen)
				bc.length = l;
			T = T.concat(bc);
		}
		return T;
	};
	pad(c) {
		// padding to the modulus size
		if (c.byteLength < this.m.sizeof()) {
			var nc = new ArrayBuffer(this.m.sizeof());
			var a = new Uint8Array(nc);
			a.fill(0);
			a.set(c);
			c = nc;
		}
		return c;
	};
	computeKey(pub, v) {
		var v = v ? new Arith.Integer(v) : this.v;
		this.pubPeer = pub;
		var A = new Arith.Integer(pub);
		// S = (A * v^u) ^ b % N, u = H(A || B)
		var sha = new this.hash();
		var u = new Arith.Integer(sha.process(this.pad(this.pubPeer), this.pad(this.pubSelf)));
		var t1 = this.mod.mul(A, this.mod.exp(v, u));
		var k = this.mod.exp(t1, this.secret);
		if (sha.outputSize < 40)
			this.key = this.MGF1SHA1(k.toChunk(), 40);
		else
			this.key = sha.process(k.toChunk());
		return this.key;
	};
	verify(proof) {
		var h = new this.hash();
		// check the old version first H(B || K)
		var expected = h.process(this.pubSelf, this.key);
		if (Bin.comp(proof, expected) == 0) {
			// response: H(A || K)
			return h.process(this.pubPeer, this.key);
		}
		// if not match, then try the new version H(H(N) XOR H(g) | H(U) | s | A | B | K)
		var c1 = Bin.xor(h.process(this.m.toChunk()), h.process(this.g.toChunk()));
		var c2 = h.process(this.username);
		var expected = h.process(c1, c2, this.salt, this.pubPeer, this.pubSelf, this.key);
		if (Bin.comp(proof, expected) == 0) {
			// response: H(A || M || K)
			return h.process(this.pubPeer, proof, this.key);
		}
		// return undefined;
	};
	_load_default_modulus() @ "xs_srp_load_default_modulus";
};
