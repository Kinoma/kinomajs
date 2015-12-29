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

export default class ECDSA {
	constructor(key, priv) {
		super();
		this.u = priv ? key.du: key.Qu;
		this.G = key.G;
		this.orderSize = key.n.sizeof();
		this.z = new Arith.Z();
		this.n = new Arith.Module(this.z, key.n);
		this.m = new Arith.Module(this.z, key.p);
		this.ec = new Arith.EC(key.a, key.b, this.m);
	};
	_sign(H) {
		// (r, s) = (k*G, (e + du*r) / k)
		var ec = this.ec;
		var du = this.u;
		var G = this.G;
		var n = this.n;
		var e = new Arith.Integer(H);
		do {
			var k = this.randint(n.m, this.z);
			var R = ec.mul(G, k);
			var r = R.x;
			var s = n.mul(n.add(e, n.mul(du, r)), n.mulinv(k));
		} while (s.isZero());
		var sig = new Object;
		sig.r = r;
		sig.s = s;
		return sig;
	};
	sign(H) {
		var sig = this._sign(H);
		var os = new ArrayBuffer();
		var l = this.orderSize;
		return os.concat(Crypt.PKCS1.I2OSP(sig.r, l), Crypt.PKCS1.I2OSP(sig.s, l));
	};
	_verify(H, r, s) {
		// u1 = e / s
		// u2 = r / s
		// R = u1*G + u2*Qu
		// result = R.x == r
		var ec = this.ec;
		var Qu = this.u;
		var G = this.G;
		var n = this.n;
		var e = new Arith.Integer(H);
		var s_inv = n.mulinv(s);
		var u1 = n.mul(e, s_inv);
		var u2 = n.mul(r, s_inv);
		// var R = ec.add(ec.mul(G, u1), ec.mul(Qu, u2));
		var R = ec.mul2(G, u1, Qu, u2);
		return this.comp(R.x, r) == 0;
	};
	verify(H, sig) {
		var l = this.orderSize;
		var r = Crypt.PKCS1.OS2IP(sig.slice(0, l));
		var s = Crypt.PKCS1.OS2IP(sig.slice(l, l*2));
		return this._verify(H, r, s);
	};
};
