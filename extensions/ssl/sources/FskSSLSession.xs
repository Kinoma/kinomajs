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
	<object name="Priv" script="false"/>
	<import href="FskTimer.xs"/>

	<patch prototype="FskSSL">
		<object name="session" prototype="FskSSL">
			<chunk name="clientSessionID"/>
			<chunk name="serverSessionID"/>
			<boolean name="connectionEnd"/>		<!-- false: server, true: client -->
			<boolean name="clientAuth" value="false"/>
			<boolean name="verifyHost" value="false"/>

			<object name="chosenCipher" prototype="FskSSL.cipherSuite"/>
			<number name="chosenCompressionMethod"/>

			<null name="clientRandom"/>	<!-- "random" at the client hello protocol in chunk -->
			<null name="serverRandom"/>	<!-- "random" at the server hello protocol in chunk -->
			<null name="masterSecret"/>	<!-- master secret in chunk -->

			<number name="handshakeProcess"/>

			<undefined name="clientCerts"/>
			<undefined name="serverCerts"/>

			<null name="myCert"/>
			<null name="peerCert"/>		<!-- sender's cert in x509 object -->

			<null name="handshakeMessages"/>		<!-- record all handshake messages -->

			<object name="cipherObject">	<!-- just a template -->
				<null name="enc"/>
				<null name="hmac"/>
			</object>
			<null name="clientCipher"/>	<!-- prototype="FskSSL.session.cipher" -->
			<null name="serverCipher"/>	<!-- prototype="FskSSL.session.cipher" -->

			<null name="applicationData"/>
			<null name="recordBuffer"/>
			<null name="writeBuffer"/>

			<null name="readSeqNum"/>	<!-- Integer type -->
			<null name="writeSeqNum"/>	<!-- Integer type -->

			<target name="ssl_debug">
			<number name="traceLevel" value="0"/>
			<string name="traceDirection"/>
			<function name="traceProtocol" params="protocol">
				var indent = ""
				for (var n = this.traceLevel + 1; --n >= 0;)
					indent += ">";
				trace(indent + " " + protocol.name + "." + this.traceDirection + "\n");
				this.traceLevel++;
			</function>
			<function name="startTrace" params="direction">
				this.traceLevel = 0;
				this.traceDirection = direction;
			</function>
			</target>
			<target name="!ssl_debug">
			<function name="traceProtocol"/>
			<function name="startTrace"/>
			</target>

			<function name="certChain" params="certs, descendant, certList">
				if (descendant)
					var target = certs[0];
				else
					var target = certs[certs.length - 1];
				for (var i in certList) {
					var o = certList[i];
					if (o == target)
						continue;
					if (descendant) {
						if (o.tbsCertificate.issuer == target.tbsCertificate.subject) {	// may not unique in theory, but..
							certs.unshift(o);
							return(this.certChain(certs, descendant, certList));
						}
					}
					else {
						if (o.tbsCertificate.subject == target.tbsCertificate.issuer) {
							certs.push(o);
							return(this.certChain(certs, descendant, certList));
						}
					}
				}
				return(certs);
			</function>

			<function name="findPreferedCert" params="types, names, descendant">
				var certList = this.certificates.getList();
				for (var i = 0; i < types.length; i++) {
					switch (types[i]) {
					case FskSSL.cipherSuite.RSA:
						var algo = [1, 2, 840, 113549, 1, 1, 1];
						break;
					case FskSSL.cipherSuite.DSA:
						var algo = [1, 2, 840, 10040, 4, 1];
						break;
					default:
						continue;
					}
					for (var j = 0; j < names.length; j++) {
						var dn = Crypt.x509.dn.toString(Crypt.ber.decode(names[j]));
						for (var i in certList) {
							var o = certList[i];
							if (o.tbsCertificate.subject == dn && Crypt.ber.oideq(algo, o.tbsCertificate.subjectPublicKeyInfo.algorithm.algorithm))
								return(this.certChain([o], descendant, certList));
						}
					}
				}
			</function>

			<function name="setupCipher" params="connectionEnd">
				function setupSub(o, cipher) {
					switch (cipher.cipherAlgorithm) {
					case FskSSL.cipherSuite.DES:
						var enc = new Crypt.DES(o.key);
						break;
					case FskSSL.cipherSuite.TDES:
						var enc = new Crypt.TDES(o.key);
						break;
					case FskSSL.cipherSuite.AES:
						var enc = new Crypt.AES(o.key);
						break;
					case FskSSL.cipherSuite.RC4:
						var enc = new Crypt.RC4(o.key);
						break;
					default:
						throw new FskSSL.Error(-9);	// unknown algorithm -> kFskErrUnimplemented
					}
					switch (cipher.encryptionMode) {
					case FskSSL.cipherSuite.CBC:
					case FskSSL.cipherSuite.NONE:
						var h;
						switch (cipher.hashAlgorithm) {
						case FskSSL.cipherSuite.MD5: h = new Crypt.MD5(); break;
						case FskSSL.cipherSuite.SHA1: h = new Crypt.SHA1(); break;
						case FskSSL.cipherSuite.SHA256: h = new Crypt.SHA256(); break;
						default:
							throw new Error("SSL: SetupCipher: unknown hash algorithm");
						}
						o.hmac = new Crypt.HMAC(h, o.macSecret);
						if (cipher.encryptionMode == FskSSL.cipherSuite.CBC)
							o.enc = new Crypt.CBC(enc, o.iv);	// no padding -- SSL 3.2 requires padding process beyond RFC2630
						else
							o.enc = enc;
						break;
					case FskSSL.cipherSuite.GCM:
						o.enc = new Crypt.GCM(enc);
						o.nonce = new FskSSL.Integer(1);
						break;
					default:
						o.enc = enc;
						break;
					}
				}

				var random = new Chunk();
				random.append(this.serverRandom);
				random.append(this.clientRandom);
				var macSize = this.chosenCipher.encryptionMode == FskSSL.cipherSuite.GCM ? 0 : this.chosenCipher.hashSize;
				var ivSize = (this.protocolVersion.major == 3 && this.protocolVersion.minor == 1) ? this.chosenCipher.cipherBlockSize : (this.chosenCipher.saltSize || 0);
				var nbytes = this.chosenCipher.cipherKeySize * 2 + macSize * 2 + ivSize * 2;
				var keyBlock = FskSSL.PRF(this, this.masterSecret, "key expansion", random, nbytes);
				random.free();
				var s = new FskSSL.ChunkStream(keyBlock);
				var o = xs.newInstanceOf(FskSSL.session.cipherObject);
				var cipher = this.chosenCipher;
				if (connectionEnd) {
					// client side
					if (macSize > 0) {
						o.macSecret = s.readChunk(macSize);
						void s.readChunk(macSize);
					}
					o.key = s.readChunk(cipher.cipherKeySize);
					void s.readChunk(cipher.cipherKeySize);
					if (ivSize > 0)
						o.iv = s.readChunk(ivSize);
					else
						o.iv = undefined;
					setupSub(o, cipher);
					this.clientCipher = o;
				}
				else {
					// server side
					if (macSize > 0) {
						void s.readChunk(macSize);
						o.macSecret = s.readChunk(macSize);
					}
					void s.readChunk(cipher.cipherKeySize);
					o.key = s.readChunk(cipher.cipherKeySize);
					if (ivSize > 0) {
						void s.readChunk(ivSize);
						o.iv = s.readChunk(ivSize);
					}
					else
						o.iv = undefined;
					setupSub(o, cipher);
					this.serverCipher = o;
				}
				s.close();
			</function>

			<function name="processCerts" params="certs">
				return this.certificates.processCerts(certs, false);
			</function>

			<function name="getKey">
				return this.keyring.get(this.myCert.tbsCertificate.extensions.subjectKeyId.toString());
			</function>

			<function name="doProtocol" params="s, o, param1, param2">
				this.startTrace("packetize");
				var packet = o.packetize(this, param1, param2);
				s.writeChunk(packet.getChunk());
				s.flush();
				packet.close();
			</function>

			<target name="unused">
			<function name="clientHandshake" params="s">
				// C -> S: ClientHello 
				this.doProtocol(s, FskSSL.clientHello);

				// S -> C: ServerHello
				FskSSL.recordProtocol.unpacketize(this, s);
				if (this.handshakeProcess != FskSSL.handshakeProtocol.server_hello)
					throw new FskSSL.Error(-3);	// bad state
				if (this.clientSessionID.length && this.clientSessionID.comp(this.serverSessionID) == 0) {
					// resumed handshake
					// S -> C: [ChangeCipherSpec]
					// S -> C: Finished
					do {
						FskSSL.recordProtocol.unpacketize(this, s);
					} while (this.handshakeProcess != FskSSL.handshakeProtocol.finished);

					// C -> S: [ChangeCipherSpec]
					// C -> S: Finished
					this.doProtocol(s, FskSSL.changeCipherSpec);
					this.doProtocol(s, FskSSL.finished);
				}
				else {
					// full handshake
					// S -> C: <various messages>
					// S -> C: ServerHelloDone
					do {
						FskSSL.recordProtocol.unpacketize(this, s);
					} while (this.handshakeProcess != FskSSL.handshakeProtocol.server_hello_done);

					// C -> S: Certificate, ... Finished
					if (this.clientCerts !== undefined)
						this.doProtocol(s, FskSSL.certificate, this.clientCerts);
					this.doProtocol(s, FskSSL.clientKeyExchange);
					if (this.clientCerts !== undfeined)
						this.doProtocol(s, FskSSL.certificateVerify);
					this.doProtocol(s, FskSSL.changeCipherSpec);
					this.doProtocol(s, FskSSL.finished);

					// S -> C: [ChangeCipherSpec]
					// S -> C: Finished
					FskSSL.recordProtocol.unpacketize(this, s);	// should be ChangeCipherSpec
					FskSSL.recordProtocol.unpacketize(this, s);	// should be Finished
					if (this.handshakeProcess != FskSSL.handshakeProtocol.finished)
						throw new FskSSL.Error(-3);	// bad state
				}
			</function>

			<function name="serverHandshake" params="s">
				// C -> S: ClientHello
				FskSSL.recordProtocol.unpacketize(this, s);
				if (this.handshakeProcess != FskSSL.handshakeProtocol.client_hello)
					throw new FskSSL.Error(-3);	// bad state

				// S -> C: ServerHello
				var masterSecret = this.cacheManager.getById(this.clientSessionID);
				if (masterSecret) {
					this.serverSessionID = this.clientSessionID;
					this.masterSecret = masterSecret;
				}

				if (!this.serverSessionID || !this.serverSessionID.length) {
					// assign a new one
					this.serverSessionID = (new Arith.Integer(((new Date()).valueOf()).toString())).toChunk();
					this.doProtocol(s, FskSSL.serverHello);
					// S -> C: Certificate   (always -- i.e. not support anonymous auth.)
					var certs = this.serverCerts;
					if (!certs || !certs.length)
						throw new FskSSL.Error(-13);	// bad data
					this.doProtocol(s, FskSSL.certificate, certs);
					if (System.applyEnvironment("[sslClientAuth]"))
						// S -> C: CertificateRequest  (always, for now..)
						this.doProtocol(s, FskSSL.certificateRequest, [FskSSL.cipherSuite.RSA, FskSSL.cipherSuite.DSA], [Crypt.ber.encode(certs[certs.length - 1].tbsCertificate.subjectDN)]);
					// S -> C: ServerHelloDone
					this.doProtocol(s, FskSSL.serverHelloDone);

					// C -> S: Certificate ... Finished
					do {
						this.recordProtocol.unpacketize(this, s);
					} while (this.handshakeProcess != FskSSL.handshakeProtocol.finished);

					// S -> C: [ChangeCipherSpec], Finished
					this.doProtocol(s, FskSSL.changeCipherSpec);
					this.doProtocol(s, FskSSL.finished);
				}
				else {
					this.doProtocol(s, FskSSL.serverHello);
					this.doProtocol(s, FskSSL.changeCipherSpec);
					this.doProtocol(s, FskSSL.finished);

					// C -> S: [ChangeCipherSpec], Finished
					FskSSL.recordProtocol.unpacketize(this, s);
					FskSSL.recordProtocol.unpacketize(this, s);
				}
			</function>
			</target>	<!-- unused -->

			<function name="loadCerts" params="certFile, key">
				var certs = FskSSL.loadCerts(this.certificates, certFile, true);
				this.clientAuth = ("policies" in certFile) && certFile.policies.toLowerCase().indexOf("clientauth") >= 0;
				this.verifyHost = this.certificates.getPolicy() & Crypt.certificate.POLICY_VERIFY_HOST;
				if (key) {
					this.serverCerts = [];
					for (var i = 0; i < certs.length; i++)
						this.serverCerts.push(Crypt.x509.decode(certs[i]));
					// take the SKID of the first cert which has to be the subject cert
					var skid = certs.length > 0 && this.serverCerts[0].tbsCertificate.extensions.subjectKeyId;
					if (skid)
						FskSSL.loadKey(this.keyring, key, skid.toString());
				}
			</function>

			<function name="resumed">
				return this.clientSessionID.length > 0 && this.clientSessionID.comp(this.serverSessionID) == 0;
			</function>

			<function name="handshake" params="s, onFinishedCallback, initiate, timeout">
				this.connectionEnd = initiate;
				// event driven version
				var processing = false;
				s.sslSession = this;
				this.socket = s;
				this.handshakeMessages = new FskSSL.ChunkStream();	// holds the entire handshake messages to check integrity
				this.handshakeProcess = -1;	// wait until the connection is established
				this.onFinished = onFinishedCallback;
				this.timeout = timeout;
				this.timer = undefined;
				this.saveOnConnected = ("onConnected" in s) ? s.onConnected: undefined;
				this.saveOnReadable = ("onReadable" in s) ? s.onReadable: undefined;
				this.saveOnWritable = ("onWritable" in s) ? s.onWritable: undefined;

				function exceptionGuard(f) {
					return function() {
						var that = this;
						try {
							f.call(this);
						} catch(e) {
							var err = (e instanceof FskSSL.Error) ? e.code : -9999;
							that.allDone.call(that, err);
						}
					}
				}

				s.allDone = function(err) {
					var session = this.sslSession;
					if (session.timer) {
						session.timer.close();
						session.timer = undefined;
					}
					var fskSocket = this.detachData();
					this.onConnected = session.saveOnConnected;
					this.onReadable = session.saveOnReadable;
					this.onWritable = session.saveOnWritable;
					if (session.onFinished) {
						session.onFinished.call(session, fskSocket, err);
						session.onFinished = null;
					}
					if (session.handshakeMessages) {
						session.handshakeMessages.close();
						session.handshakeMessages = null;
					}
					session.applicationData = new FskSSL.ChunkStream();
				}

				s.onConnected = function() {
					if (!this.connected) {
						this.allDone(this.connectionError);
						return;
					}
					if (this.sslSession.connectionEnd)	// client starts a handshake with hello_request
						this.sslSession.handshakeProcess = FskSSL.handshakeProtocol.hello_request;
					if (this.sslSession.timeout) {
						var session = this.sslSession;
						session.timer = new Timer();
						session.timer.onCallback = function() {
							session.socket.allDone(-120);	// handshake failed
						};
						session.timer.schedule(session.timeout);
					}
				};

				s.onReadable = exceptionGuard(function() {
					var session = this.sslSession;
					if (!processing) {	// to guard from calling the callback while reading data
						processing = true;
						while (this.bytesAvailable > 0) {
							session.startTrace("unpacketize");
							FskSSL.recordProtocol.unpacketize(session, this);
							if (session.handshakeProcess == FskSSL.handshakeProtocol.finished) {
								if (session.connectionEnd ^ session.resumed())
									this.allDone(0);
								// otherwise wait for the write side to finish the rest of protocols
								session.cacheManager.saveSession(this.hostname, session.serverSessionID, session.masterSecret);
								break;
							}
						}
						processing = false;
					}
				});

				s.onWritable = exceptionGuard(function() {
					var session = this.sslSession;
					switch (session.handshakeProcess) {
					case FskSSL.handshakeProtocol.hello_request:	// C
						var o = session.cacheManager.get(this.hostname);
						if (o) {
							session.clientSessionID = o.id;
							session.masterSecret = o.masterSecret;
						}
						session.doProtocol(this, FskSSL.clientHello);
						break;
					case FskSSL.handshakeProtocol.client_hello:	// S
						var masterSecret = session.cacheManager.getById(session.clientSessionID);
						if (masterSecret) {
							// resumed handshake
							session.serverSessionID = session.clientSessionID;
							session.masterSecret = masterSecret;
							session.doProtocol(this, FskSSL.serverHello);
							session.doProtocol(this, FskSSL.changeCipherSpec);
							session.doProtocol(this, FskSSL.finished);
						}
						else {
							// assign a new one
							session.serverSessionID = (new Arith.Integer(((new Date()).valueOf()).toString())).toChunk();
							session.doProtocol(this, FskSSL.serverHello);
							// S -> C: Certificate   (always -- i.e. not support anonymous auth.)
							var certs = session.serverCerts;
							if (!certs || !certs.length)
								throw new FskSSL.Error(-13);	// bad data
							session.doProtocol(this, FskSSL.certificate, certs);
							if (session.clientAuth)
								// S -> C: CertificateRequest
								session.doProtocol(this, FskSSL.certificateRequest, [FskSSL.cipherSuite.RSA, FskSSL.cipherSuite.DSA], [Crypt.ber.encode(session.myCert.tbsCertificate.subjectDN)]);
							// S -> C: ServerHelloDone
							session.doProtocol(this, FskSSL.serverHelloDone);
						}
						break;
					case FskSSL.handshakeProtocol.server_hello_done:	// C
						if (session.clientCerts !== undefined)
							session.doProtocol(this, FskSSL.certificate, session.clientCerts);
						session.doProtocol(this, FskSSL.clientKeyExchange);
						if (session.myCert)	// client cert request && and the certs is not empty
							session.doProtocol(this, FskSSL.certificateVerify);
						session.doProtocol(this, FskSSL.changeCipherSpec);
						session.doProtocol(this, FskSSL.finished);
						break;
					case FskSSL.handshakeProtocol.finished:	// C, S
						if (!(session.connectionEnd ^ session.resumed())) {
							session.doProtocol(this, FskSSL.changeCipherSpec);
							session.doProtocol(this, FskSSL.finished);
							this.allDone(0);
						}
						break;
					default:
						// just let it go
						break;
					}
					session.handshakeProcess = -1;
				});

				if (s.connected) {
					if (this.timeout) {
						var that = this;
						this.timer = new Timer();
						this.timer.onCallback = function() {
							that.socket.allDone(-120);	// handshake failed
						};
						this.timer.schedule(this.timeout);
					}
					if (this.connectionEnd) {
						s.attachData(s.detachData());	// just to de-register callbacks so SSL can take it over
						this.handshakeProcess = FskSSL.handshakeProtocol.hello_request;
						s.onWritable();
					}
					else {
						this.startTrace("unpacketize[init]");
						try {
							FskSSL.recordProtocol.unpacketize(this, s);
						} catch(e) {
							var err = (e instanceof FskSSL.Error) ? e.code : -9999;
							s.allDone(err);
						}
					}
				}
			</function>

			<function name="write" params="s, data">
				var ret = 0;	// returns the byte size of the original data (not encrypted one)
				if (this.writeBuffer) {
					s.flush();
					if (s.bytesWritable <= 0) {
						// trace("### SSL Socket full (0) " + this.writeBuffer.bytesAvailable + " in the buffer###\n");
						return 0;
					}
					if (this.writeBuffer.bytesAvailable <= s.bytesWritable) {
						s.writeChunk(this.writeBuffer.readChunk(this.writeBuffer.bytesAvailable));
						this.writeBuffer.close();
						this.writeBuffer = null;
					}
					else {
						s.writeChunk(this.writeBuffer.readChunk(s.bytesWritable));
						// trace("### SSL Socket full (" + s.bytesWritable + ") " + this.writeBuffer.bytesAvailable + " in the buffer###\n");
						return 0;
					}
				}
				var tmps = new FskSSL.ChunkStream(data);
				if (data.length <= FskSSL.maxFragmentSize) {
					// a little optimization
					this.startTrace("packetize");
					var cipher = FskSSL.recordProtocol.packetize(this, FskSSL.recordProtocol.application_data, tmps);
					if (cipher.bytesAvailable <= s.bytesWritable) {
						s.writeChunk(cipher.getChunk());
						cipher.close();
					}
					else {
						cipher.rewind();
						if (s.bytesWritable > 0) {
							// we need to write something here
							var c = cipher.readChunk(s.bytesWritable);
							s.writeChunk(c);
							c.free();
						}
						this.writeBuffer = cipher;
					}
					ret = data.length;
				}
				else {
					var c;
					while (c = tmps.readChunk(FskSSL.maxFragmentSize)) {
						this.startTrace("packetize");
						ret += c.length;
						var fragment = new FskSSL.ChunkStream(c);
						var cipher = FskSSL.recordProtocol.packetize(this, FskSSL.recordProtocol.application_data, fragment);
						fragment.close();
						if (cipher.bytesAvailable <= s.bytesWritable) {
							s.writeChunk(cipher.getChunk());
							cipher.close();
						}
						else {
							cipher.rewind();
							if (s.bytesWritable > 0) {
								// we need to write something here
								c = cipher.readChunk(s.bytesWritable);
								s.writeChunk(c);
								c.free();
							}
							this.writeBuffer = cipher;
							break;
						}
					}
				}
				tmps.detach();
				tmps.close();
				s.flush();
				return(ret);
			</function>

			<function name="read" params="s, size">
				var data = this.applicationData;
				if (!data)
					return;		// called before handshake hasn't finished

				while (s.bytesAvailable > 0) {
					this.startTrace("unpacketize");
					FskSSL.recordProtocol.unpacketize(this, s);
					if (data.bytesAvailable >= size)
						break;
				}

				if (data.bytesAvailable > 0) {
					data.rewind();
					var c = data.readChunk(size);
					var rem = data.readChunk(data.bytesAvailable);
					// close and re-create -- this is the only way to clear the stream.chunk
					data.close();
					this.applicationData = data = new FskSSL.ChunkStream();
					if (rem) data.writeChunk(rem);
					return c;
				}
				else if (s.bytesAvailable < 0) {
					return;	// connection closed
				}
				else {
					return new Chunk();	// return an empty chunk to indicate "no data"
				}
			</function>

			<function name="get bytesAvailable">
				return this.applicationData ? this.applicationData.bytesAvailable : -1;
			</function>

			<function name="flush" params="s">
				// trace("flushing\n");
				if (!s.flush()) {
					// trace("flush failed.\n");
					return false;
				}
				if (this.writeBuffer) {
					// trace("flushing the write buffer\n");
					if (this.writeBuffer.bytesAvailable <= s.bytesWritable) {
						s.writeChunk(this.writeBuffer.readChunk(this.writeBuffer.bytesAvailable));
						this.writeBuffer.close();
						this.writeBuffer = null;
						return s.flush();
					}
					// trace("flush failed.\n");
					return false;
				}
				return true;
			</function>

			<function name="close" params="s">
				if (this.alert)
					return;		// probably the peer has already closed the connection
				if (this.writeBuffer) {
					s.flush();
					if (this.writeBuffer.bytesAvailable <= s.bytesWritable)
						s.writeChunk(this.writeBuffer.readChunk(this.writeBuffer.bytesAvailable));
					this.writeBuffer.close();
					this.writeBuffer = null;
				}
				this.startTrace("packetize");
				var alert = FskSSL.alert.packetize(this, 0, FskSSL.alert.close_notify);
				s.writeChunk(alert.getChunk());
				alert.close();
			</function>
		</object>
		<function name="Session" params="options" prototype="FskSSL.session">
			this.certificates = new Crypt.Certificate(FskSSL.certificatesInstance);
			this.keyring = new Crypt.Keyring(FskSSL.keyringInstance);
			this.cacheManager = new FskSSL.SessionCacheManager();
			this.alert = undefined;
			this.extensions = options;
			this.protocolVersion = FskSSL.protocolVersion;
			if (options && options.protocolVersion) {
				this.protocolVersion.major = options.protocolVersion.major;
				this.protocolVersion.minor = options.protocolVersion.minor;
			}
		</function>
	</patch>
</package>
