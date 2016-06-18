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
	<import href="FskCore.xs" link="dynamic"/>
	<import href="Crypt.xs" link="dynamic"/>

	<object name="FskSSL">
		<number name="maxFragmentSize" value="16384"/>	<!-- maximum record layer framgment size (not a packet size): 2^14 -->

		<object name="protocolVersion">
			<number name="major" value="3"/>
			<number name="minor" value="1"/>

			<function name="serialize" params="s, o">
				if (!o) o = this;
				s.writeChar(o.major);
				s.writeChar(o.minor);
			</function>

			<function name="parse" params="s">
				var o = xs.newInstanceOf(FskSSL.protocolVersion);
				o.major = s.readChar();
				o.minor = s.readChar();
				return(o);
			</function>
		</object>

		<object name="cipherSuite">
			<!-- constants -->
			<number name="RSA" value="0"/>
			<number name="DSA" value="1"/>
			<number name="AES" value="0"/>
			<number name="DES" value="1"/>
			<number name="TDES" value="2"/>
			<number name="RC4" value="3"/>
			<number name="SHA1" value="0"/>
			<number name="MD5" value="1"/>
			<number name="SHA256" value="2"/>
			<number name="NULL" value="255"/>

			<array name="value" contents="Number.prototype"/>
			<boolean name="isExportable"/>
			<number name="keyExchangeAlgorithm"/>
			<number name="cipherAlgorithm"/>
			<number name="cipherKeySize"/>		<!-- in byte -->
			<number name="cipherBlockSize"/>		<!-- ditto -->
			<number name="hashAlgorithm"/>
			<number name="hashSize"/>		<!-- output size in bytes -->
		</object>

		<array name="supportedCipherSuites" contents="FskSSL.cipherSuite"/>
		<array name="supportedCompressionMethods" contents="Number.prototype"/>

		<object name="integer">
			<function name="inc">
				this.num = this.z.inc(this.num, 1);
			</function>
			<function name="toChunk">
				return this.num.toChunk();
			</function>
		</object>
		<function name="Integer" params="n" prototype="FskSSL.integer">
			this.z = new Arith.Z();
			this.num = new Arith.Integer(n);
		</function>

		<function name="RNG" params="size">
			return Crypt.defaultRNG.get(size);
		</function>

		<object name="error" prototype="Error.prototype">
			<number name="code" value="-9999"/>
		</object>
		<function name="Error" params="code" prototype="FskSSL.error">
			this.code = code;
		</function>

		<function name="PRF" params="session, secret, label, seed, n">
			function p_hash(hash, secret, seed) {
				var hmac = new Crypt.HMAC(hash, secret);
				var niter = Math.ceil(n / hash.outputSize);
				hmac.update(seed);
				var A = hmac.close();		// start from A(1) = hmac(seed)
				var p = new FskSSL.ChunkStream();
				for (var i = 0; i < niter; i++) {
					hmac.reset();
					hmac.update(A);
					hmac.update(seed);
					p.writeChunk(hmac.close());
					hmac.reset();
					hmac.update(A);
					A = hmac.close();
				}
				p.rewind();
				return(p);
			}

			function xor(s1, s2) {
				var r = new FskSSL.ChunkStream(Math.min(s1.bytesAvailable, s2.bytesAvailable));
				for (var c1, c2;;) {
					c1 = s1.readChar();
					c2 = s2.readChar();
					if (c1 == undefined || c2 == undefined)
						break;
					r.writeChar(c1 ^ c2);
				}
				var c = r.getChunk()
				r.detach(); r.close();
				return(c);
			}

			var s = new Crypt.bin.chunk.String(label);
			s.append(seed);
			if (session.protocolVersion.major == 3 && session.protocolVersion.minor <= 2)
				var r = xor(
					p_hash(new Crypt.MD5(), secret.slice(0, Math.ceil(secret.length / 2)), s),
					p_hash(new Crypt.SHA1(), secret.slice(Math.floor(secret.length / 2)), s)
				);
			else {
				var h = p_hash(new Crypt.SHA256(), secret, s);
				var r = h.getChunk();
			}
			return(r.slice(0, n));
		</function>

		<null name="sessionCacheInstance"/>
		<object name="sessionCache" prototype="Crypt.persistentList">
			<chunk name="id"/>
			<chunk name="masterSecret"/>

			<function name="parse" params="b">
				var o = xs.newInstanceOf(FskSSL.sessionCache);
				o.id = b.getAtom("SSID");
				o.masterSecret = b.getAtom("SCRT");
				return o;
			</function>

			<function name="serialize" params="o">
				var atom = new Chunk();
				atom.putAtom("SSID", o.id);
				atom.putAtom("SCRT", o.masterSecret);
				return atom;
			</function>
		</object>
		<function name="SessionCache" prototype="FskSSL.sessionCache">
			Crypt.PersistentList.call(this);
		</function>

		<object name="sessionCacheManager" prototype="Crypt.persistentListClient">
			<function name="saveSession" params="hostname, sessionID, masterSecret">
				var o = xs.newInstanceOf(FskSSL.sessionCache);
				o.id = sessionID;
				o.masterSecret = masterSecret;
				this.set(hostname, o);
			</function>

			<function name="getById" params="id">
				for (var i = 0, o; (o = this.nth(i)); i++) {
					if (o.id == id)
						return o.masterSecret;
				}
			</function>
		</object>
		<function name="SessionCacheManager" prototype="FskSSL.sessionCacheManager">
			Crypt.PersistentListClient.call(this, FskSSL.sessionCacheInstance);
		</function>

		<function name="loadRootCerts" params="calist">
			var certMgr = new Crypt.Certificate(FskSSL.certificatesInstance);
			certMgr.setPolicy(Crypt.certificate.POLICY_ALLOW_ORPHAN | Crypt.certificate.POLICY_ALLOW_LOOPHOLE); // trust all certs in the default CA list
			if (!FileSystem.getFileInfo(calist))
				return;
			var s = new Stream.File(calist);
			var c = s.readChunk(s.bytesAvailable);
			s.close();
						
			FskSSL.loadCerts(certMgr, c);
		</function>

		<object name="chunkStream" prototype="FskStream.Chunk">
			<function name="readChars" params="n">
				var c = 0;
				while (--n >= 0)
					c = (c << 8) | this.readChar();
				return(c);
			</function>
			<function name="writeChars" params="v, n">
				while (--n >= 0)
					this.writeChar((v >>> (n * 8)) & 0xff);
			</function>
			<function name="peekChar" params="n">
				return n < this.bytesAvailable ? this.chunk.peek(n) : undefined;
			</function>
			<function name="getChunk">
				this.chunk.length = this.bytesAvailable;
				return this.chunk;
			</function>
			<function name="rewind">
				this.chunk.length = this.bytesAvailable;
				FskStream.Chunk.rewind.call(this);
			</function>
		</object>
		<function name="ChunkStream" params="a" prototype="FskSSL.chunkStream">
			Stream.Chunk.call(this, a === undefined ? 128 : a);
		</function>
	</object>

	<import href="FskSSLProtocols.xs"/>
	<import href="FskSSLSession.xs"/>
	<import href="FskSSLCert.xs"/>

	<program>
		// set the supported cipher suites
		FskSSL.supportedCipherSuites = [
			// TLS_RSA_WITH_AES_128_CBC_SHA
			{value: [0x00, 0x2f],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.AES,
			 cipherKeySize: 16,
			 cipherBlockSize: 16,
			 hashAlgorithm: FskSSL.cipherSuite.SHA1,
			 hashSize: 20},
			// TLS_RSA_WITH_AES_256_CBC_SHA
			{value: [0x00, 0x35],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.AES,
			 cipherKeySize: 32,
			 cipherBlockSize: 16,
			 hashAlgorithm: FskSSL.cipherSuite.SHA1,
			 hashSize: 20},
			// TLS_RSA_WITH_DES_CBC_SHA
			{value: [0x00, 0x09],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.DES,
			 cipherKeySize: 8,
			 cipherBlockSize: 8,
			 hashAlgorithm: FskSSL.cipherSuite.SHA1,
			 hashSize: 20},
			// TLS_RSA_WITH_3DES_EDE_CBC_SHA
			{value: [0x00, 0x0A],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.TDES,
			 cipherKeySize: 24,
			 cipherBlockSize: 8,
			 hashAlgorithm: FskSSL.cipherSuite.SHA1,
			 hashSize: 20},
			// TLS_RSA_WITH_RC4_128_MD5
			{value: [0x00, 0x04],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.RC4,
			 cipherKeySize: 16,
			 cipherBlockSize: 0,
			 hashAlgorithm: FskSSL.cipherSuite.MD5,
			 hashSize: 16},
			// TLS_RSA_WITH_RC4_128_SHA
			{value: [0x00, 0x05],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.RC4,
			 cipherKeySize: 16,
			 cipherBlockSize: 0,
			 hashAlgorithm: FskSSL.cipherSuite.SHA1,
			 hashSize: 20},
			// TLS_RSA_WITH_AES_128_CBC_SHA256
			{value: [0x00, 0x3c],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.AES,
			 cipherKeySize: 16,
			 cipherBlockSize: 16,
			 hashAlgorithm: FskSSL.cipherSuite.SHA256,
			 hashSize: 32},
			// TLS_RSA_WITH_AES_256_CBC_SHA256
			{value: [0x00, 0x3d],
			 isExportable: false,
			 keyExchangeAlgorithm: FskSSL.cipherSuite.RSA,
			 cipherAlgorithm: FskSSL.cipherSuite.AES,
			 cipherKeySize: 32,
			 cipherBlockSize: 16,
			 hashAlgorithm: FskSSL.cipherSuite.SHA256,
			 hashSize: 32},
			// TLS_NULL_WITH_NULL_NULL
			{value: [0x00, 0x00],
			 isExportable: true,	// ?
			 keyExchangeAlgorithm: FskSSL.cipherSuite.NULL,
			 cipherAlgorithm: FskSSL.cipherSuite.NULL,
			 cipherKeySize: 0,
			 cipherBlockSize: 0,
			 hashAlgorithm: FskSSL.cipherSuite.NULL,
			 hashSize: 0},
		];

		FskSSL.supportedCompressionMethods = [0];	// NULL

		FskSSL.certificatesInstance = new Crypt.CertificateInstance();
		FskSSL.keyringInstance = new Crypt.KeyringInstance();

		FskSSL.sessionCacheInstance = new FskSSL.SessionCache();
	</program>
</package>
