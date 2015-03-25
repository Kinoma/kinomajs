<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package>
	<patch prototype="Crypt">
		<object name="rsa">
			<null name="e" script="false"/>
			<null name="z" script="false"/>
			<null name="mn" script="false"/>
			<null name="mp" script="false"/>
			<null name="mq" script="false"/>
			<null name="dp" script="false"/>
			<null name="dq" script="false"/>
			<null name="C2" script="false"/>
			<null name="p" script="false"/>

			<function name="process" params="c">
				if (this.mp && this.mq) {
					var z = this.z;
					var v1 = this.mq.exp(c, this.dq);
					var v2 = this.mp.exp(c, this.dp);
					var u = this.mp.mul(this.C2, this.mp.sub(v2, v1));
					return(z.add(v1, z.mul(u, this.q)));
				}
				else
					return(this.mn.exp(c, this.e));
			</function>

			<function name="modulusSize">
				return this.mn ? this.mn.m.sizeof() : this.z.mul(this.p, this.q).sizeof();
			</function>
		</object>
		<function name="RSA" params="e, m, p, q, dp, dq, C2" prototype="Crypt.rsa">
			var esize = e.sizeof();
			var z = new Arith.Z();
			if (esize > 4 && p && !p.isNaN() && q && !q.isNaN()) {
				// use CRT
				this.dp = dp && !dp.isNaN() ? dp: z.mod(e, z.inc(p, -1));
				this.dq = dq && !dq.isNaN() ? dq: z.mod(e, z.inc(q, -1));
				this.mp = new Arith.Module(z, p);
				this.mq = new Arith.Module(z, q);
				this.C2 = C2 && !C2.isNaN() ? C2: this.mp.mulinv(q);	// (C2 = q^-1 mod p) according to PKCS8
				this.p = p;
				this.q = q;
			}
			else {
				if (!m && p && !p.isNaN() && q && !q.isNaN())
					m = z.mul(p, q);
				this.mn = new Arith.Module(z, m, esize <= 4 ? 2: 0);	// use the simple LR method in the case of small exponent (i.e public key)
				this.e = e;
			}
			this.z = z;
		</function>

		<!-- PKCS#1 primitives -->
		<object name="pkcs1">
			<function name="I2OSP" params="I, l">
				var c = I.toChunk();
				if (l && l > c.length) {
					// prepend 0
					var d = l - c.length;
					var t = new Chunk(d);
					for (var i = 0; i < d; i++)	// just in case
						t.poke(i, 0x00);
					t.append(c);
					c.free();
					c = t;
				}
				return(c);
			</function>

			<function name="sI2OSP" params="sI, l">
				if (!l) {
					l = 4;
					var c = new Chunk(l);
					var skip = true;
					var i = 0;
					while (--l >= 0) {
						var x;
						if ((x = (sI >>> (l*8))) != 0 || !skip) {
							c.poke(i++, x & 0xff);
							skip = false;
						}
					}
					if (i == 0)
						c.poke(i++, 0);
					c.length = i;
				}
				else {
					// l must be <= 4
					var c = new Chunk(l);
					var i = 0;
					while (--l >= 0)
						c.poke(i++, (sI >>> (l*8)) & 0xff);
				}
				return(c);
			</function>

			<function name="OS2IP" params="OS">
				return(new Arith.Integer(OS));
			</function>
		</object>

		<!-- PKCS1_5 (signature and encryption scheme) -->
		<object name="pkcs1_5" prototype="Crypt.pkcs1">
			<null name="rsa" script="false"/>
			<null name="modulusSize" script="false"/>
			<null name="oid"/>

			<!-- EMSA-PKCS1 encoding method -->
			<function name="emsaEncode" params="H, emLen, flag" script="false"><![CDATA[
				if (!flag) {
					var oid = this.oid ? this.oid: [1,3,14,3,2,26];	// use SHA1 by default if not specified
					var bc = Crypt.ber.encode([0x30, [0x30, [0x06, oid], [0x05]], [0x04, H]]);
				}
				else
					var bc = H;
				// prepend the prefix part
				var ffsize = emLen - bc.length - 2;
				var s = new Chunk(ffsize + 2);
				var i = 0;
				s.poke(i++, 0x01);
				for (; i <= ffsize; i++)
					s.poke(i, 0xff);
				s.poke(i++, 0x00);
				s.append(bc);
				var EM = new Arith.Integer(s);
				s.free();
				return(EM);
			]]></function>

			<function name="emsaDecode" params="EM, flag" script="false"><![CDATA[
				var s = EM.toChunk();
				var i = 0;
				if (s.peek(i++) != 0x01)
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				for (; i < s.length && s.peek(i) == 0xff; i++)
					;
				if (i >= s.length || s.peek(i) != 0x00)
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				if (flag)
					return(s.slice(i+1));
				var ber = Crypt.ber.decode(s.slice(i+1));
				s.free();
				if (!ber || (this.oid && !Crypt.ber.oideq(ber[1][1][1], this.oid)))
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				return(ber[2][1]);
			]]></function>

			<!-- EME-PKCS1 encoding method -->
			<function name="emeEncode" params="M, emLen" script="false"><![CDATA[
				var pssize = emLen - M.length - 2;
				if (pssize < 0)
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				var s = new Chunk(pssize + 2);
				var ps = Crypt.defaultRNG.get(pssize);
				var i = 0;
				s.poke(i++, 0x02);
				for (var j = 0; j < ps.length; j++) {
					// make sure of nonzero
					var c = ps.peek(j);
					if (c == 0)
						c = 0xff;
					s.poke(i++, c);
				}
				ps.free();
				s.poke(i++, 0x00);
				s.append(M);
				var EM = new Arith.Integer(s);
				s.free();
				return(EM);
			]]></function>

			<function name="emeDecode" params="EM" script="false"><![CDATA[
				var s = EM.toChunk();
				var i = 0;
				if (s.peek(i++) != 0x02)
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				var c;
				for (; i < s.length; i++) {
					if (s.peek(i) == 0x00)
						return(s.slice(i + 1));
				}
				s.free();
				throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
			]]></function>

			<function name="sign" params="H, flag">
				var f = this.emsaEncode(H, this.modulusSize-1, flag);
				var v = this.rsa.process(f);
				return(this.I2OSP(v, this.modulusSize));
			</function>

			<function name="verify" params="H, sig, flag">
				var v = this.rsa.process(this.OS2IP(sig));
				return(H.comp(this.emsaDecode(v, flag)) == 0);
			</function>

			<function name="encrypt" params="M">
				var e = this.emeEncode(M, this.modulusSize-1);
				var v = this.rsa.process(e);
				return(this.I2OSP(v, this.modulusSize));
			</function>

			<function name="decrypt" params="e">
				var d = this.rsa.process(this.OS2IP(e));
				return(this.emeDecode(d));
			</function>
		</object>
		<function name="PKCS1_5" params="key, priv, oid" prototype="Crypt.pkcs1_5">
			this.rsa = new Crypt.RSA(priv ? key.privExponent: key.exponent, key.modulus, key.prim1, key.prim2, key.exponent1, key.exponent2, key.coefficient);
			this.modulusSize = this.rsa.modulusSize();
			if (oid)
				this.oid = oid;
		</function>

		<!-- OAEP (encryption scheme) -->
		<object name="oaep" prototype="Crypt.pkcs1">
			<null name="rsa" script="false"/>
			<null name="modulusSize" script="false"/>
			<null name="P" script="false"/>
			<null name="H" script="false"/>
			<null name="MGF" script="false"/>

			<function name="MGF1SHA1" params="Z, l" script="false"><![CDATA[
				var H = new Crypt.SHA1();
				var hLen = H.outputSize;
				var T = new Chunk();
				for (var counter = 0; l > 0; counter++, l -= hLen) {
					var C = this.sI2OSP(counter, 4);
					H.reset();
					H.update(Z);
					H.update(C);
					var bc = H.close();
					if (l < hLen)
						bc.length = l;
					T.append(bc);
					bc.free();
				}
				return(T);
			]]></function>

			<function name="xor" params="b1, b2" script="false">
				Crypt.bin.chunk.xor(b1, b2, b1);
			</function>

			<function name="emeEncode" params="M, P, emLen" script="false"><![CDATA[
				var H = this.H;
				var hLen = H.outputSize;
				// 3. Generate an octet string PS consisting of emLen-||M||-2hLen-1 zero octets.
				var psLen = emLen - M.length - 2*hLen - 1;
				if (psLen < 0)
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				var PS = new Chunk(psLen + 1);
				for (var i = 0; i < psLen; i++)
					PS.poke(i, 0);
				PS.poke(psLen, 0x01);
				// 4. Let pHash = Hash(P)
				H.reset();
				H.update(P);
				var pHash = H.close();
				// 5. Concatenate pHash, PS, the message M, ...
				var DB = new Chunk();
				DB.append(pHash);	pHash.free();
				DB.append(PS);		PS.free();
				DB.append(M);
				// 6. Generate a random octet string seed of length hLen.
				var seed = Crypt.defaultRNG.get(hLen);
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
				EM.append(maskedDB);
				var iEM = new Arith.Integer(EM);
				DB.free();
				seed.free();
				dbMask.free();
				seedMask.free();
				return(iEM);
			]]></function>

			<function name="emeDecode" params="iEM, P, emLen" script="false"><![CDATA[
				var EM = iEM.toChunk(emLen);
				var H = this.H;
				var hLen = H.outputSize;
				var maskedSeed = EM.slice(0, hLen);
				var maskedDB = EM.slice(hLen);
				EM.free();
				var seedMask = this.MGF(maskedDB, hLen);
				this.xor(maskedSeed, seedMask);
				seedMask.free();
				var dbMask = this.MGF(maskedSeed, emLen - hLen);
				maskedSeed.free();
				this.xor(maskedDB, dbMask);
				dbMask.free();
				var DB = maskedDB;
				H.reset();
				H.update(P);
				var pHash = H.close();
				// check to see if the pHash equals the first hLen of DB
				if (pHash.ncomp(DB, hLen) != 0) {
					DB.free();
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				}
				var c, i;
				for (i = hLen; i < DB.length && (c = DB.peek(i)) == 0x00; i++)
					;
				if (c != 0x01) {
					DB.free();
					throw new Crypt.Error(Crypt.error.kCryptMalformedInput);
				}
				var chunk = DB.slice(i+1);
				DB.free();
				return(chunk);
			]]></function>

			<function name="encrypt" params="M">
				var e = this.emeEncode(M, this.P, this.modulusSize - 1);
				var v = this.rsa.process(e);
				return(this.I2OSP(v, this.modulusSize));
			</function>

			<function name="decrypt" params="e">
				var d = this.rsa.process(this.OS2IP(e));
				return(this.emeDecode(d, this.P, this.modulusSize - 1));
			</function>
		</object>
		<function name="OAEP" params="key, priv, H, P, MGF" prototype="Crypt.oaep">
			this.rsa = new Crypt.RSA(priv ? key.privExponent: key.exponent, key.modulus, key.prim1, key.prim2, key.exponent1, key.exponent2, key.coefficient);
			this.modulusSize = this.rsa.modulusSize();
			this.H = H ? H: new Crypt.SHA1();
			this.P = P ? P: new Chunk();
			this.MGF = MGF ? MGF: this.MGF1SHA1;
		</function>

		<function name="randint" params="max, z" script="false">
			var i = new Arith.Integer(Crypt.defaultRNG.get(max.sizeof()));
			while (i.comp(max) >= 0)
				i = z.lsr(i, 1);
			return(i);
		</function>

		<!-- DSA (fips186-2) -->
		<object name="dsa" prototype="Crypt.pkcs1">
			<null name="p" script="false"/>
			<null name="q" script="false"/>
			<null name="g" script="false"/>
			<null name="x" script="false"/>		<!-- or y in the case of verification -->
			<null name="z" script="false"/>

			<function name="_sign" params="H" script="false">
				// r = (g^k mod p) mod q
				// s = (SHA_1(M) + xr)/k mod q
				var p = this.p;
				var q = this.q;
				var g = this.g;
				var x = this.x;
				var k = Crypt.randint(q.m, this.z);
				var r = q.mod(p.exp(g, k));
				var H = new Arith.Integer(H);
				var s = q.mul(q.mulinv(k), q.add(H, q.mul(x, r)));
				var sig = new Object;
				sig.r = r;
				sig.s = s;
				return(sig);
			</function>

			<function name="sign" params="H">
				var sig = this._sign(H);
				var os = new Chunk();
				os.append(this.I2OSP(sig.r, 20));
				os.append(this.I2OSP(sig.s, 20));
				return(os);
			</function>

			<function name="_verify" params="H, r, s" script="false">
				// w = 1/s mod q
				// u1 = (SHA_1(M) * w) mod q
				// u2 = rw mod q
				// v = (g^u1 * y^u2 mod p) mod q
				var p = this.p;
				var q = this.q;
				var g = this.g;
				var y = this.x;		// as the public key
				var w = q.mulinv(s);
				var h = new Arith.Integer(H);
				var u1 = q.mul(h, w);
				var u2 = q.mul(r, w);
				var v = q.mod(p.exp2(g, u1, y, u2));
				return(v.comp(r) == 0);
			</function>

			<function name="verify" params="H, sig">
				// "20" is specified in the xmldsig-core spec.
				var r = this.OS2IP(sig.slice(0, 20));
				var s = this.OS2IP(sig.slice(20, 40));
				return(this._verify(H, r, s));
			</function>
		</object>
		<function name="DSA" params="key, priv" prototype="Crypt.dsa">
			this.x = priv ? key.x: key.y;
			this.g = key.g;
			this.z = new Arith.Z();
			this.p = new Arith.Module(this.z, key.p);
			this.q = new Arith.Module(this.z, key.q);
		</function>

		<!-- EC-DSA -->
		<object name="ecdsa" prototype="Crypt.pkcs1">
			<null name="u" script="false"/>
			<null name="ec" script="false"/>
			<null name="n" script="false"/>
			<null name="G" script="false"/>
			<null name="orderSize" script="false"/>
			<null name="z" script="false"/>

			<function name="_sign" params="H" script="false">
				// (r, s) = (k*G, (e + du*r) / k)
				var ec = this.ec;
				var du = this.u;
				var G = this.G;
				var n = this.n;
				var e = new Arith.Integer(H);
				do {
					var k = Crypt.randint(n.m, this.z);
					var R = ec.mul(G, k);
					var r = R.x;
					var s = n.mul(n.add(e, n.mul(du, r)), n.mulinv(k));
				} while (s.isZero());
				var sig = new Object;
				sig.r = r;
				sig.s = s;
				return(sig);
			</function>

			<function name="sign" params="H">
				var sig = this._sign(H);
				var os = new Chunk();
				var l = this.orderSize;
				os.append(this.I2OSP(sig.r, l));
				os.append(this.I2OSP(sig.s, l));
				return(os);
			</function>

			<function name="_verify" params="H, r, s" script="false">
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
				return(R.x.comp(r) == 0);
			</function>

			<function name="verify" params="H, sig">
				var l = this.orderSize;
				var r = this.OS2IP(sig.slice(0, l));
				var s = this.OS2IP(sig.slice(l, l*2));
				return(this._verify(H, r, s));
			</function>
		</object>
		<function name="ECDSA" params="key, priv" prototype="Crypt.ecdsa">
			this.u = priv ? key.du: key.Qu;
			this.G = key.G;
			this.orderSize = key.n.sizeof();
			this.z = new Arith.Z();
			this.n = new Arith.Module(this.z, key.n);
			this.m = new Arith.Module(this.z, key.p);
			this.ec = new Arith.EC(key.a, key.b, this.m);
		</function>
	</patch>
</package>