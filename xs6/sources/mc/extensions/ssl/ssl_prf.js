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
import Bin from "bin";
import SSLStream from "ssl/stream";

function idiv(a, b)
{
	return (a / b) | 0;
}

function iceil(x, y)
{
	return idiv(x, y) + (x % y ? 1 : 0);
}

function p_hash(hash, secret, seed, sz)
{
	var hmac = new Crypt.HMAC(hash, secret);
	var niter = iceil(sz, hash.outputSize);
	var A = hmac.process(seed);		// start from A(1) = hmac(seed)
	var p = new SSLStream();
	while (--niter >= 0) {
		var c = hmac.process(A, seed);
		p.writeChunk(c);
		if (niter > 0)
			A = hmac.process(A);
	}
	return p.getChunk();
}

function PRF(session, secret, label, seed, n, hash)
{
	var s = ArrayBuffer.fromString(label);
	s = s.concat(seed);
	if (session.protocolVersion <= 0x302)
		var r = Bin.xor(
			p_hash(new Crypt.MD5(), secret.slice(0, iceil(secret.byteLength, 2)), s, n),
			p_hash(new Crypt.SHA1(), secret.slice(idiv(secret.byteLength, 2)), s, n)
		);
	else {
		if (!hash) hash = Crypt.SHA256;
		var r = p_hash(new hash(), secret, s, n);
	}
	return r.slice(0, n);
}

export default PRF;
