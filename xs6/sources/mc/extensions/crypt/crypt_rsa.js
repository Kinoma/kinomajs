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
import Arith from "arith";

export default class RSA {
	constructor(e, m, p, q, dp, dq, C2) {
		var esize = e.sizeof();
		var z = new Arith.Z();
		if (esize > 4 && p && !p.isNaN() && q && !q.isNaN()) {
			// use CRT
			this.dp = dp && !dp.isNaN() ? dp : z.mod(e, z.inc(p, -1));
			this.dq = dq && !dq.isNaN() ? dq : z.mod(e, z.inc(q, -1));
			this.mp = new Arith.Mont({z: z, m: p});
			this.mq = new Arith.Mont({z: z, m: q});
			this.C2 = C2 && !C2.isNaN() ? C2 : this.mp.mulinv(q);	// (C2 = q^-1 mod p) according to PKCS8
			this.p = p;
			this.q = q;
			this.order = m ? m.sizeof() : z.mul(p, q).sizeof();
		}
		else {
			if (!m && p && !p.isNaN() && q && !q.isNaN())
				m = z.mul(p, q);
			this.mn = new Arith.Module(z, m, esize <= 4 ? 2: 0);	// use the simple LR method in the case of small exponent (i.e public key)
			this.e = e;
			this.order = m.sizeof();
		}
		this.z = z;
	};
	process(c) {
		if (this.mp && this.mq) {
			var z = this.z;
			var v1 = this.mq.exp(c, this.dq);
			var v2 = this.mp.exp(c, this.dp);
			var u = this.mp.mul(this.C2, this.mp.sub(v2, v1));
			return(z.add(v1, z.mul(u, this.q)));
		}
		else
			return(this.mn.exp(c, this.e));
	};
	get modulusSize() {
		return this.order;
	};
};
