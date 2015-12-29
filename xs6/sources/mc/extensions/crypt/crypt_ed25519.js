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
const params = {
	b: 256,
	q: "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed",
	l: "0x1000000000000000000000000000000014def9dea2f79cd65812631a5cf5d3ed",
	d: "-0x98412dfc9311d490018c7338bf8688861767ff8ff5b2bebe27548a14b235ec8feda4",
	I: "0x2b8324804fc1df0b2b4d00993dfbd7a72f431806ad2fe478c4ee1b274a0ea0b0",
	B: {
		x: "0x216936d3cd6e53fec0a4e231fdd6dc5c692cc7609525a7b2c9562d608f25d51a",
		y: "0x6666666666666666666666666666666666666666666666666666666666666658",
	},
};

export default class Ed25519 {
	constructor() {
		this.b = params.b;
		this.q = new Arith.Integer(params.q);
		this.l = new Arith.Integer(params.l);
		this.d = new Arith.Integer(params.d);
		this.I = new Arith.Integer(params.I);
		this.B = {x: new Arith.Integer(params.B.x), y: new Arith.Integer(params.B.y)};
		var SHA512 = require.weak("Crypt").SHA512;
		this.hash = new SHA512();
		this.ed = new Arith.Ed(this.q, this.d);
		this.mod = this.ed.mod;
		this.z = this.mod.z;
	};
	getX(y) {
		var z = this.z;
		var mod = this.mod;
		var xx = mod.mul(z.inc(mod.square(y), -1), mod.mulinv(z.inc(mod.mul(this.d, mod.square(y)), +1)));
		var x = mod.exp(xx, z.lsr(z.inc(this.q, +3), 3));
		var t = mod.square(x);
		if (!mod.sub(t, xx).isZero())
			x = mod.mul(x, this.I);
		if (!z.and(x, new Arith.Integer(1)).isZero())
			x = z.sub(this.q, x);
		return x;
	};
	encode(P) {
		var c = P.y.toChunk(this.b / 8, false, true);
		var one = new Arith.Integer(1);
		var b = this.z.and(P.x, one);
		if (!b.isZero()) {
			var a = new Uint8Array(c);
			c[c.length - 1] |= 0x80;
		}
		return c;
	};
	decode(s) {
		var y = new Arith.Integer(s, true, true);
		var sign = y.sign();
		if (sign)
			y.negate();
		var x = this.getX(y);
		var one = new Arith.Integer(1);
		var b = this.z.and(x, one);
		if (b.isZero() == sign)
			x = this.z.sub(this.q, x);
		return {x: x, y: y};
	};
	getA(h) {
		var z = this.z;
		var a = new Arith.Integer(1);
		a = z.lsl(a, this.b - 2);
		hl = new Arith.Integer(h, false, true);
		hl = z.lsl(z.lsr(hl, 3), 3);	// mask with ~3
		hl = z.mod(hl, a);	// mask with ~2^{b-3}
		return z.or(a, hl);
	};
	getPK(sk) {
		var h = this.hash.process(sk);
		var a = this.getA(h.slice(0, 32));
		var A = this.ed.mul(this.B, a);
		return this.encode(A);
	};
	sign(msg, sk, pk) {
		if (!pk)
			pk = this.getPK(sk);
		var h = this.hash.process(sk);
		var r = new Arith.Integer(this.hash.process(h.slice(32, 64), msg), false, true);
		// R = rB
		var R = this.ed.mul(this.B, r);
		var R_ = this.encode(R);
		// S = (r + H(R, A, M)a mod l
		var hram = new Arith.Integer(this.hash.process(R_, pk, msg), false, true);
		var a = this.getA(h.slice(0, 32));
		var mod = new Arith.Mont({z: this.z, m: this.l});
		var S = mod.add(r, mod.mul(hram, a));
		R_.append(S.toChunk(this.b / 8, false, true));
		return R_;
	};
	verify(msg, signature, pk) {
		var R_ = signature.slice(0, 32);
		var R = this.decode(R_);
		var S = new Arith.Integer(signature.slice(32, 64), false, true);
		var A = this.decode(pk);
		var h = new Arith.Integer(this.hash.process(R_, pk, msg), false, true);
		var P1 = this.ed.mul(this.B, S);
		var P2 = this.ed.add(R, this.ed.mul(A, h));
		return P1.x.comp(P2.x) == 0 && P1.y.comp(P2.y) == 0;
	};
};
