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
	<patch prototype="FskSSL">
		<object name="recordProtocol">
			<string name="name" value="recordProtocol"/>
			<!-- content type -->
			<number name="change_cipher_spec" value="20"/>
			<number name="alert" value="21"/>
			<number name="handshake" value="22"/>
			<number name="application_data" value="23"/>

			<function name="readPacket" params="ss, cs">
				// this is only for unblocking socket
				if (cs.bytesAvailable < 5) {
					var c = ss.readChunk(5 - cs.bytesAvailable);
					if (!c)
						return;
					cs.writeChunk(c);
					if (cs.bytesAvailable < 5)
						return;
				}
				var len = (cs.peekChar(3) << 8) | cs.peekChar(4);
				len += 5;	// header size
				if (cs.bytesAvailable < len) {
					var c = ss.readChunk(len - cs.bytesAvailable);
					if (!c)
						return;
					cs.writeChunk(c);
					if (cs.bytesAvailable < len)
						return;
				}
				cs.rewind();
				return true;
			</function>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				if (!session.recordBuffer)
					session.recordBuffer = new FskSSL.ChunkStream();
				if (!this.readPacket(s, session.recordBuffer))
					return;
				FskSSL.tlsCipherText.unpacketize(session, session.recordBuffer);
				session.recordBuffer.close();
				session.recordBuffer = null;
			</function>

			<function name="packetize" params="session, type, fragment">
				session.traceProtocol(this);
				return(FskSSL.tlsPlainText.packetize(session, type, fragment));
			</function>
		</object>

		<object name="tlsPlainText" prototype="FskSSL.recordProtocol">
			<string name="name" value="tlsPlainText"/>
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var type = s.readChar();
				var version = FskSSL.protocolVersion.parse(s);
				var fragmentLen = s.readChars(2);
				// it is safer to readChunk(fragmentLen) and then make it up a new stream than passing all the rest of the stuff to the hight layer blindly, but considering efficiency in speed and memory, ...
				switch (type) {
				case this.change_cipher_spec:
					FskSSL.changeCipherSpec.unpacketize(session, s);
					break;
				case this.alert:
					FskSSL.alert.unpacketize(session, s);
					break;
				case this.handshake:
					while (s.bytesAvailable > 0)
						FskSSL.handshakeProtocol.unpacketize(session, s);
					break;
				case this.application_data:
					if (session.applicationData && fragmentLen) {
						var fragment = s.readChunk(fragmentLen);
						session.applicationData.writeChunk(fragment);
						fragment.free();
					}
					break;
				default:
					throw new FskSSL.Error(-13);	// bad data
				}
			</function>

			<function name="packetize" params="session, type, fragment">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				s.writeChar(type);
				session.protocolVersion.serialize(s);
				s.writeChars(fragment.bytesAvailable, 2);
				s.writeChunk(fragment.getChunk());
				return(FskSSL.tlsCompressed.packetize(session, s));
			</function>
		</object>

		<object name="tlsCompressed" prototype="FskSSL.tlsPlainText">
			<string name="name" value="tlsCompressed"/>
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				// unsupported -- just pass through
				FskSSL.tlsPlainText.unpacketize(session, s);
			</function>

			<function name="packetize" params="session, s">
				session.traceProtocol(this);
				// unsupported -- just pass through
				return(FskSSL.tlsCipherText.packetize(session, s));
			</function>
		</object>

		<object name="tlsCipherText" prototype="FskSSL.tlsCompressed">
			<string name="name" value="tlsCipherText"/>
			<function name="calculateMac" params="hmac, seqNum, compressedPacket">
				hmac.reset();
				var c = seqNum.toChunk();
				var tmps = new FskSSL.ChunkStream();
				for (var i = 0, len = 8 - c.length; i < len; i++)
					tmps.writeChar(0);
				tmps.writeChunk(c);
				hmac.update(tmps.getChunk());
				tmps.close();
				hmac.update(compressedPacket.getChunk());
				return(hmac.close());
			</function>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var cipher = session.connectionEnd ? session.serverCipher: session.clientCipher;
				if (!cipher) {
					FskSSL.tlsCompressed.unpacketize(session, s);
					return;
				}
				var type = s.readChar();
				var version = FskSSL.protocolVersion.parse(s);
				var fragmentLen = s.readChars(2);
				if (!(session.protocolVersion.major == 3 && session.protocolVersion.minor == 1) && session.chosenCipher.cipherBlockSize) { // 3.2 or higher && block cipher
					var iv = s.readChunk(session.chosenCipher.cipherBlockSize);
					cipher.enc.setIV(iv);
					iv.free();
					fragmentLen -= session.chosenCipher.cipherBlockSize;
				}
				var fragment = s.readChunk(fragmentLen);
				var plain = cipher.enc.decrypt(fragment);
				var padLen = session.chosenCipher.cipherBlockSize ? plain.peek(plain.length - 1) + 1: 0;
				var contentLen = fragmentLen - session.chosenCipher.hashSize - padLen;
				var content = plain.slice(0, contentLen);
				var mac = plain.slice(contentLen, contentLen + session.chosenCipher.hashSize);
				fragment.free();

				var s = new FskSSL.ChunkStream();
				s.writeChar(type);
				s.writeChar(version.major);
				s.writeChar(version.minor);
				s.writeChars(contentLen, 2);
				s.writeChunk(content);
				s.rewind();
				content.free();
				if (cipher.hmac) {
					if (mac.comp(this.calculateMac(cipher.hmac, session.readSeqNum, s)) != 0)
						throw new FskSSL.Error(-121);	// SSL auth failed
				}
				FskSSL.tlsCompressed.unpacketize(session, s);
				session.readSeqNum.inc();
				s.close();
			</function>

			<function name="packetize" params="session, compressed">
				session.traceProtocol(this);
				var cipher = session.connectionEnd ? session.clientCipher: session.serverCipher;
				if (!cipher)
					return(compressed);
				compressed.rewind();
				// calculate MAC in either case if a hash alogithm is specified in the cipher suite
				var mac = this.calculateMac(cipher.hmac, session.writeSeqNum, compressed);
				session.writeSeqNum.inc();
				var s = new FskSSL.ChunkStream();
				s.writeChar(compressed.readChar());		// type
				s.writeChars(compressed.readChars(2), 2);	// protocol version
				var compLength = compressed.readChars(2);	// size
				// block cipher only at the moment
				if (cipher.enc) {
					var tmps = new FskSSL.ChunkStream();
					if (!(session.protocolVersion.major == 3 && session.protocolVersion.minor == 1) && session.chosenCipher.cipherBlockSize) { // 3.2 or higher && block cipher
						var iv = FskSSL.RNG(session.chosenCipher.cipherBlockSize);
						tmps.writeChunk(iv);
						cipher.enc.setIV(iv);
						iv.free();
					}
					tmps.writeChunk(compressed.readChunk(compLength));
					tmps.writeChunk(mac);
					if (session.chosenCipher.cipherBlockSize) {
						var length = compLength + session.chosenCipher.hashSize + 1;
						var padSize = length % session.chosenCipher.cipherBlockSize;
						if (padSize > 0)
							padSize = session.chosenCipher.cipherBlockSize - padSize;
						for (var i = 0; i < padSize; i++)
							tmps.writeChar(padSize);
						tmps.writeChar(padSize);
					}
					var fragment = cipher.enc.encrypt(tmps.getChunk());
					tmps.close();
				}
				else
					var fragment = compressed.readChunk(compLength);
				s.writeChars(fragment.length, 2);
				s.writeChunk(fragment);
				return(s);
			</function>
		</object>

		<object name="changeCipherSpec">
			<string name="name" value="changeCipherSpec"/>
			<!-- type -->
			<number name="change_cipher_spec" value="1"/>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var type = s.readChar();
				if (type == this.change_cipher_spec) {
					session.readSeqNum = new FskSSL.Integer(0);	// the specification is very ambiguous about the sequence number...
					session.setupCipher(!session.connectionEnd);
				}
				else
					throw new FskSSL.Error(-3);	// bad state
			</function>

			<function name="packetize" params="session, type">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				if (!type) type = this.change_cipher_spec;
				s.writeChar(type);
				var upper = FskSSL.recordProtocol.packetize(session, FskSSL.recordProtocol.change_cipher_spec, s);
				session.writeSeqNum = new FskSSL.Integer(0);	// the specification is very ambiguous about the sequence number...
				session.setupCipher(session.connectionEnd);
				s.close();
				return(upper);
			</function>
		</object>

		<object name="alert">
			<string name="name" value="alert"/>
			<!-- alert level -->
			<number name="warning" value="1"/>
			<number name="fatal" value="2"/>
			<!-- alert description -->
			<number name="close_notify" value="0"/>
			<number name="unexpected_message" value="10"/>
			<number name="bad_record_mac" value="20"/>
			<number name="decryption_failed" value="21"/>
			<number name="record_overflow" value="22"/>
			<number name="decompression_failure" value="30"/>
			<number name="handshake_failure" value="40"/>
			<number name="bad_certificate" value="42"/>
			<number name="unsupported_certificate" value="43"/>
			<number name="certificate_revoked" value="44"/>
			<number name="certificate_expired" value="45"/>
			<number name="certificate_unknown" value="46"/>
			<number name="illegal_parameter" value="47"/>
			<number name="unknown_ca" value="48"/>
			<number name="access_denied" value="49"/>
			<number name="decode_error" value="50"/>
			<number name="decrypt_error" value="51"/>
			<number name="export_restriction" value="60"/>
			<number name="protocol_version" value="70"/>
			<number name="insufficient_security" value="71"/>
			<number name="internal_error" value="80"/>
			<number name="user_canceled" value="90"/>
			<number name="no_negotiation" value="100"/>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				session.alert = new Object();
				session.alert.level = s.readChar();
				session.alert.description = s.readChar();
				if (session.alert.description != FskSSL.alert.close_notify)
					throw new FskSSL.Error(-113);	// connection closed
			</function>

			<function name="packetize" params="session, level, description">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				s.writeChar(level);
				s.writeChar(description);
				var upper = FskSSL.recordProtocol.packetize(session, FskSSL.recordProtocol.alert, s);
				s.close();
				return(upper);
			</function>
		</object>

		<object name="handshakeProtocol">
			<string name="name" value="handshakeProtocol"/>
			<!-- handshake type -->
			<number name="hello_request" value="0"/>
			<number name="client_hello" value="1"/>
			<number name="server_hello" value="2"/>
			<number name="certificate" value="11"/>
			<number name="server_key_exchange" value="12"/>
			<number name="certificate_request" value="13"/>
			<number name="server_hello_done" value="14"/>
			<number name="certificate_verify" value="15"/>
			<number name="client_key_exchange" value="16"/>
			<number name="finished" value="20"/>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var msgType = s.readChar(0);
				var bodyLen = s.readChars(3);
				// here, take the safe way unlink at the tlsPlanText, because some protocol handler does not want to take into account the size of the body so it might will do readChunk(bytesAvailable)...
				if (bodyLen > 0) {
					var body = s.readChunk(bodyLen);
					var tmps = new FskSSL.ChunkStream(body);
				}
				else {
					var body;
					var tmps = new FskSSL.ChunkStream();
				}
				switch (msgType) {
				case this.hello_request:
					FskSSL.helloRequest.unpacketize(session, tmps);
					break;
				case this.client_hello:
					FskSSL.clientHello.unpacketize(session, tmps);
					break;
				case this.server_hello:
					FskSSL.serverHello.unpacketize(session, tmps);
					break;
				case this.certificate:
					FskSSL.certificate.unpacketize(session, tmps);
					break;
				case this.server_key_exchange:
					FskSSL.serverKeyExchange.unpacketize(session, tmps);
					break;
				case this.certificate_request:
					FskSSL.certificateRequest.unpacketize(session, tmps);
					break;
				case this.server_hello_done:
					FskSSL.serverHelloDone.unpacketize(session, tmps);
					break;
				case this.certificate_verify:
					FskSSL.certificateVerify.unpacketize(session, tmps);
					break;
				case this.client_key_exchange:
					FskSSL.clientKeyExchange.unpacketize(session, tmps);
					break;
				case this.finished:
					FskSSL.finished.unpacketize(session, tmps);
					break;
				default:
					throw new FskSSL.Error(-13);	// bad data
					break;
				}
				// add the whole message
				session.handshakeMessages.writeChar(msgType);
				session.handshakeMessages.writeChars(bodyLen, 3);
				if (body)
					session.handshakeMessages.writeChunk(body);
				session.handshakeProcess = msgType;
				if (tmps)
					tmps.close();
			</function>

			<function name="packetize" params="session, msgType, body">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				s.writeChar(msgType);
				s.writeChars(body.bytesAvailable, 3);
				s.writeChunk(body.getChunk());
				session.handshakeMessages.writeChunk(s.getChunk());
				var upper = FskSSL.recordProtocol.packetize(session, FskSSL.recordProtocol.handshake, s);
				s.close();
				return(upper);
			</function>
		</object>

		<object name="helloRequest">
			<string name="name" value="helloRequest"/>
			<function name="unpacketize" params="session">
				session.traceProtocol(this);
				// nothing to unpacketize
			</function>

			<function name="packetize" params="session">
				session.traceProtocol(this);
				// nothing to packetize
				return(FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.hello_request, new FskSSL.ChunkStream()));
			</function>
		</object>

		<object name="helloProtocol">
			<object name="extension_type">
				<number name="server_name" value="0"/>
				<number name="max_fragment_length" value="1"/>
				<number name="client_certification_url" value="2"/>
				<number name="trusted_ca_keys" value="3"/>
				<number name="trusted_hmac" value="4"/>
				<number name="status_request" value="5"/>
			</object>
			<string name="name" value="helloProtocol"/>
			<object name="random">
				<number name="qmt_unix_time"/>
				<chunk name="random_bytes"/>

				<function name="parse" params="s">
					var o = {};
					var t = s.readChars(4);
					o.qmt_unix_time = t;
					o.random_bytes = s.readChunk(28);
					return(o);
				</function>

				<function name="serialize" params="">
					var s = new FskSSL.ChunkStream();
					var qmt_unix_time = (new Date()).getTime() / 1000;	// in sec
					var random_bytes = FskSSL.RNG(28);
					s.writeChars(qmt_unix_time, 4);
					s.writeChunk(random_bytes);
					return(s.getChunk());
				</function>
			</object>

			<function name="selectCipherSuite" params="peerSuites">
				for (var i = 0, suites = FskSSL.supportedCipherSuites; i < suites.length; i++) {
					var mySuite = suites[i].value;
					for (var j = 0; j < peerSuites.length; j++) {
						if (mySuite[0] == peerSuites[j][0] && mySuite[1] == peerSuites[j][1])
							return(suites[i]);
					}
				}
				throw new FskSSL.Error(-9);	// unimplemented
			</function>

			<function name="selectCompressionMethod" params="peerMethods">
				for (var i = 0, methods = FskSSL.supportedCompressionMethods; i < methods.length; i++) {
					for (var j = 0; j < peerMethods.length; j++) {
						if (methods[i] == peerMethods[j])
							return(methods[i]);
					}
				}
				throw new FskSSL.Error(-9);	// unimplemented
			</function>

			<function name="unpacketize" params="session, s, msgType">
				session.traceProtocol(this);
				session.protocolVersion = FskSSL.protocolVersion.parse(s);
				var random = s.readChunk(32);
				var sessionIDLen = s.readChar();
				var sessionID = sessionIDLen > 0 ? s.readChunk(sessionIDLen): new Chunk();
				var suites = [];
				var compressionMethods = [];
				if (msgType == FskSSL.handshakeProtocol.client_hello) {
					session.clientRandom = random;
					session.clientSessionID = sessionID;
					for (var nsuites = s.readChars(2) / 2; --nsuites >= 0;) {
						var c1 = s.readChar();
						var c2 = s.readChar();
						suites.push([c1, c2]);
					}
					// select the most suitable one
					for (var nmethods = s.readChar(); --nmethods >= 0;)
						compressionMethods.push(s.readChar());
				}
				else {
					session.serverRandom = random;
					session.serverSessionID = sessionID;
					var c1 = s.readChar();
					var c2 = s.readChar();
					suites.push([c1, c2]);
					compressionMethods.push(s.readChar());
				}
				session.chosenCipher = this.selectCipherSuite(suites);
				session.compressionMethod = this.selectCompressionMethod(compressionMethods);
			</function>

			<function name="packetize" params="session, cipherSuites, compressionMethods, msgType">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				FskSSL.protocolVersion.serialize(s);
				session.protocolVersion = FskSSL.protocolVersion;
				var random = this.random.serialize();
				s.writeChunk(random);
				if (msgType == FskSSL.handshakeProtocol.client_hello) {
					session.clientRandom = random;
					var sessionID = session.clientSessionID;
				}
				else {
					session.serverRandom = random;
					var sessionID = session.serverSessionID;
				}
				if (!sessionID || !sessionID.length)
					s.writeChar(0);
				else {
					s.writeChar(sessionID.length);
					s.writeChunk(sessionID);
				}
				if (msgType == FskSSL.handshakeProtocol.client_hello) {
					s.writeChars(cipherSuites.length * 2, 2);
					for (var i = 0; i < cipherSuites.length; i++) {
						var val = cipherSuites[i].value;
						s.writeChar(val[0]);
						s.writeChar(val[1]);
					}
					s.writeChar(compressionMethods.length);
					for (var i = 0; i < compressionMethods.length; i++)
						s.writeChar(compressionMethods[i]);
					if (session.extensions) {
						var es = new FskSSL.ChunkStream();
						for (var i in session.extensions) {
							var ext = session.extensions[i];
							var type = this.extension_type[i];
							es.writeChars(type, 2);
							switch (type) {
							case this.extension_type.server_name:
								var len = 1 + 2 + ext.length;
								es.writeChars(2 + len, 2);
								es.writeChars(len, 2);
								es.writeChar(0);		// name_type, 0 -- host_name
								es.writeChars(ext.length, 2);
								es.writeString(ext);
								break;
							case this.extension_type.max_fragment_length:
								es.writeChars(2 + 1, 2);
								es.writeChars(1, 2);
								var j;
								for (j = 1; j <= 4; j++) {
									var e = j + 9;	// start with 2^9
									if ((ext >>> e) == 0)
										break;
								}
								if (j > 4)
									j = 4;
								es.writeChar(j);
								break;
							default:
								// not supported yet
								break;
							}
						}
						if (es.bytesAvailable) {
							s.writeChars(es.bytesAvailable, 2);
							s.writeChunk(es.getChunk());
						}
					}
				}
				else {
					var val = cipherSuites[0].value;
					s.writeChar(val[0]);
					s.writeChar(val[1]);
					s.writeChar(compressionMethods[0]);
				}
				var upper = FskSSL.handshakeProtocol.packetize(session, msgType, s);
				s.close();
				return(upper);
			</function>
		</object>

		<object name="clientHello" prototype="FskSSL.helloProtocol">
			<string name="name" value="clientHello"/>
			<target name="!wm || server">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				FskSSL.helloProtocol.unpacketize(session, s, FskSSL.handshakeProtocol.client_hello);
			</function>
			</target>

			<target name="!wm || client">
			<function name="packetize" params="session">
				session.traceProtocol(this);
				return(FskSSL.helloProtocol.packetize(session, FskSSL.supportedCipherSuites, FskSSL.supportedCompressionMethods, FskSSL.handshakeProtocol.client_hello));
			</function>
			</target>
		</object>

		<object name="serverHello" prototype="FskSSL.helloProtocol">
			<string name="name" value="serverHello"/>
			<target name="!wm || client">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				FskSSL.helloProtocol.unpacketize(session, s, FskSSL.handshakeProtocol.server_hello);
			</function>
			</target>

			<target name="!wm || server">
			<function name="packetize" params="session">
				session.traceProtocol(this);
				return(FskSSL.helloProtocol.packetize(session, [session.chosenCipher], [session.compressionMethod], FskSSL.handshakeProtocol.server_hello));
			</function>
			</target>
		</object>

		<object name="certificate" prototype="FskSSL.handshakeProtocol">
			<string name="name" value="certificate"/>
			<target name="!wm || verifyHost">
			<function name="toIP" params="hostname">
				// IPv4 only
				var c = new Chunk(4);
				var a = hostname.split(".");
				for (var i = 0; i < 4; i++) {
					if (i < a.length) {
						var v = parseInt(a[i]);
						if (v != a[i])
							return null;
					}
					else
						var v = 0;
					c.poke(i, v);
				}
				return c;
			</function>
			<function name="matchName" params="re, name">
				re = re.replace(/\./g, "\\.").replace(/\*/g, "[^.]*");
				var a = name.match(new RegExp("^" + re + "$", "i"));
				return a && a.length == 1;
			</function>
			<function name="verifyHost" params="session">
				var hostname = session.socket.hostname;
				var altNames = session.peerCert.tbsCertificate.extensions.subjectAlternativeName;
				var ipaddress = this.toIP(hostname);
				if (ipaddress) {
					for (var i = 0; i < altNames.length; i++) {
						var name = altNames[i];
						if (typeof name == "object" && name instanceof Chunk && name.comp(ipaddress) == 0)
							return true;
					}
				}
				else {
					for (var i = 0; i < altNames.length; i++) {
						var name = altNames[i];
						if (typeof altNames[i] == "string" && this.matchName(name, hostname))
							return true;
					}
					// try the common name
					var arr = session.peerCert.tbsCertificate.subject.match(/CN=([^,]*)/);
					if (arr && arr.length > 1 && this.matchName(arr[1], hostname))
						return true;
				}
				return false;
			</function>
			</target>
			<target name="!wm || client">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var certs = [];
				var ttlSize = s.readChars(3);
				while (ttlSize > 0 && s.bytesAvailable > 0) {
					var certSize = s.readChars(3);
					certs.push(s.readChunk(certSize));
					ttlSize -= certSize + 3;
				}
				session.peerCert = session.processCerts(certs);
				if (session.verifyHost) {
					if (!this.verifyHost(session))
						throw new FskSSL.Error(-121);	// auth failed
				}
			</function>
			</target>

			<target name="!wm || server || clientAuth">
			<function name="packetize" params="session, certs">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				// calculate the length
				var len = 0;
				for (var i = 0; i < certs.length; i++)
					len += (Crypt.x509.cert.serialize(certs[i])).length;
				len += 3;
				s.writeChars(len, 3);
				for (var i = 0; i < certs.length; i++) {
					var c = Crypt.x509.cert.serialize(certs[i]);
					s.writeChars(c.length, 3);
					s.writeChunk(c);
				}

				if (certs.length > 0) {
					// must be the first one
					session.myCert = certs[0];
				}
				return(FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.certificate, s));
			</function>
			</target>
		</object>

		<object name="serverKeyExchange" prototype="FskSSL.handshakeProtocol">
			<string name="name" value="serverKeyExchange"/>
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				// unsupported
			</function>

			<function name="packetize" params="session">
				session.traceProtocol(this);
				// unsupported
			</function>
		</object>

		<target name="!wm || clientAuth">
		<object name="certificateRequest">
			<string name="name" value="certificateRequest"/>
			<!-- client certificate type -->
			<number name="rsa_sign" value="1"/>
			<number name="dss_sign" value="2"/>
			<number name="rsa_fixed_dh" value="3"/>
			<number name="dss_fixed_dh" value="4"/>

			<target name="!wm || client">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var nCertTypes = s.readChar();
				var types = [];
				for (var i = 0; i < nCertTypes; i++) {
					switch (s.readChar()) {
					case this.rsa_sign:
						types.push(FskSSL.cipherSuite.RSA);
						break;
					case this.dss_sign:
						types.push(FskSSL.cipherSuite.DSA);
						break;
					default:
						// not supported
						break;
					}
				}
				var ttlSize = s.readChars(2);
				var names = [];
				while (ttlSize > 0) {
					var nbytes = s.readChars(2);
					names.push(s.readChunk(nbytes));
					ttlSize -= nbytes + 2;
				}
				session.clientCerts = session.findPreferedCert(types, names, true);
				if (!session.clientCerts)
					session.clientCerts = [];	// proceed to the "certificate" protocol with a null certificate

				// can't include the CA's cert?????
				if (session.clientCerts.length > 1)
					session.clientCerts.pop();
			</function>
			</target>

			<target name="!wm || server">
			<function name="packetize" params="session, types, authorities">
				session.traceProtocol(this);
				var s = new FskSSL.ChunkStream();
				s.writeChar(types.length);
				for (var i = 0; i < types.length; i++) {
					switch (types[i]) {
					case FskSSL.cipherSuite.RSA:
						s.writeChar(this.rsa_sign);
						break;
					case FskSSL.cipherSuite.DSA:
						s.writeChar(this.dss_sign);
						break;
					}
				}
				var ttlSize = 0;
				for (var i = 0; i < authorities.length; i++)
					ttlSize += authorities[i].length + 2;
				s.writeChars(ttlSize, 2);
				for (var i = 0; i < authorities.length; i++) {
					s.writeChars(authorities[i].length, 2);
					s.writeChunk(authorities[i]);
				}
				var upper = FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.certificate_request, s);
				s.close();
				return(upper);
			</function>
			</target>
		</object>
		</target>	<!-- clientAuth -->

		<object name="serverHelloDone">
			<string name="name" value="serverHelloDone"/>
			<target name="!wm || client">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				// nothing to unpacketize
			</function>
			</target>

			<target name="!wm || server">
			<function name="packetize" params="session">
				session.traceProtocol(this);
				// nothing to packetize
				return(FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.server_hello_done, new FskSSL.ChunkStream()));
			</function>
			</target>
		</object>

		<object name="clientKeyExchange" prototype="FskSSL.handshakeProtocol">
			<string name="name" value="clientKeyExchange"/>
			<function name="generateMasterSecret" params="session, preMasterSecret">
				var random = new Chunk();
				random.append(session.clientRandom);
				random.append(session.serverRandom);
				session.masterSecret = FskSSL.PRF(preMasterSecret, "master secret", random, 48);
				random.free();
			</function>

			<target name="!wm || server">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var n = s.readChars(2);
				var cipher = s.readChunk(n);
				if (session.chosenCipher.keyExchangeAlgorithm == FskSSL.cipherSuite.RSA) {
					// PKCS1.5
					if (!session.myCert)
						throw new FskSSL.Error(-2);	// out of sequence
					var key = session.getKey();
					var rsa = new Crypt.PKCS1_5(key.rsaKey, true);
					var plain = rsa.decrypt(cipher);
					// the first 2 bytes are client_version
					// seems like the client version is not the protocol version the client and server have agreed on...
					/*
					if (!(plain.peek(0) == session.protocolVersion.major && plain.peek(1) == session.protocolVersion.minor)) {
						throw new FskSSL.Error(-13);	// bad data
					}
					*/
					var preMasterSecret = plain;
				}
				else {
					// RSA only for now
				}
				this.generateMasterSecret(session, preMasterSecret);
			</function>
			</target>

			<target name="!wm || client">
			<function name="packetize" params="session">
				session.traceProtocol(this);
				if (session.chosenCipher.keyExchangeAlgorithm == FskSSL.cipherSuite.RSA) {
					var plain = new FskSSL.ChunkStream();
					FskSSL.protocolVersion.serialize(plain);
					plain.writeChunk(FskSSL.RNG(46));
					var preMasterSecret = plain.getChunk();
					var key = session.peerCert.getKey();
					var rsa = new Crypt.PKCS1_5(key.rsaKey);
					var cipher = rsa.encrypt(preMasterSecret);
					var s = new FskSSL.ChunkStream();
					s.writeChars(cipher.length, 2);
					s.writeChunk(cipher);
				}
				else {
					// RSA only for now
				}
				this.generateMasterSecret(session, preMasterSecret);
				return(FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.client_key_exchange, s));
			</function>
			</target>
		</object>

		<target name="!wm || clientAuth">
		<object name="certificateVerify" prototype="FskSSL.handshakeProtocol">
			<string name="name" value="certificateVerify"/>
			<function name="calculateDigest" params="session">
				var m;
				if (session.chosenCipher.keyExchangeAlgorithm == FskSSL.cipherSuite.RSA) {
					var md5 = new Crypt.MD5();
					md5.update(session.handshakeMessages.getChunk());
					m = md5.close();
				}
				var sha1 = new Crypt.SHA1();
				sha1.update(session.handshakeMessages.getChunk());
				var h = sha1.close();
				if (m) {
					m.append(h);
					h = m;
				}
				return(h);
			</function>

			<target name="!wm || server">
			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var n = s.readChars(2);
				var sig = s.readChunk(n);
				if (session.chosenCipher.keyExchangeAlgorithm == FskSSL.cipherSuite.RSA) {
					var key = session.peerCert.getKey();
					var rsa = new Crypt.PKCS1_5(key.rsaKey);
					if (!rsa.verify(this.calculateDigest(session), sig, true))
						throw new FskSSL.Error(-121);	// auth failed
				}
			</function>
			</target>

			<target name="!wm || client">
			<function name="packetize" params="session">
				session.traceProtocol(this);
				if (session.chosenCipher.keyExchangeAlgorithm == FskSSL.cipherSuite.RSA) {
					if (!session.myCert)
						throw new FskSSL.Error(-2);	// out of sequence
					var key = session.getKey();
					var rsa = new Crypt.PKCS1_5(key.rsaKey, true);
					var sig = rsa.sign(this.calculateDigest(session), true);
					var s = new FskSSL.ChunkStream();
					s.writeChars(sig.length, 2);
					s.writeChunk(sig);
				}
				else {
					throw new FskSSL.Error(-9);	// unimplemented
				}
				return(FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.certificate_verify, s));
			</function>
			</target>
		</object>
		</target>	<!-- clientAuth -->

		<object name="finished" prototype="FskSSL.handshakeProtocol">
			<string name="name" value="finished"/>
			<string name="client_finished_label" value="client finished"/>
			<string name="server_finished_label" value="server finished"/>

			<function name="calculateVerifyData" params="session, flag">
				var finishLabel = (session.connectionEnd ^ flag) ? this.client_finished_label: this.server_finished_label;
				var wholeMessage = session.handshakeMessages.getChunk();
				var digest = new Chunk();
				var hash = function(f) {
					var h = new f();
					h.update(wholeMessage);
					digest.append(h.close());
				};
				hash(Crypt.MD5);
				hash(Crypt.SHA1);
				var v = FskSSL.PRF(session.masterSecret, finishLabel, digest, 12);
				digest.free();
				return(v);
			</function>

			<function name="unpacketize" params="session, s">
				session.traceProtocol(this);
				var verify = this.calculateVerifyData(session, 1);
				if (verify.comp(s.readChunk(12)) != 0) {
					session.masterSecret = null;
					throw new FskSSL.Error(-121);	// auth failed
				}
			</function>

			<function name="packetize" params="session">
				session.traceProtocol(this);
				var verify = this.calculateVerifyData(session, 0);
				var packet = FskSSL.handshakeProtocol.packetize(session, FskSSL.handshakeProtocol.finished, new FskSSL.ChunkStream(verify));
				return(packet);
			</function>
		</object>
	</patch>
</package>

