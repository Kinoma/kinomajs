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
import SSLProtocol from "ssl/protocol";
import SSLStream from "ssl/stream";
import Crypt from "crypt";
import Bin from "bin";

var recordProtocol = {
	name: "recordProtocol",
	// global constants
	change_cipher_spec: 20,
	alert: 21,
	handshake: 22,
	application_data: 23,

	// protocols
	unpacketize(session, buf) {
		session.traceProtocol(this);
		this.tlsCipherText.unpacketize(session, new SSLStream(buf));
	},
	packetize(session, type, fragment) {
		session.traceProtocol(this);
		return this.tlsPlainText.packetize(session, type, fragment);
	},

	tlsPlainText: {
		name: "tlsPlainText",
		unpacketize(session, type, fragment) {
			session.traceProtocol(this);
			switch (type) {
			case recordProtocol.change_cipher_spec:
				SSLProtocol.changeCipherSpec.unpacketize(session, fragment);
				break;
			case recordProtocol.alert:
				SSLProtocol.alert.unpacketize(session, fragment);
				break;
			case recordProtocol.handshake:
				var s = new SSLStream(fragment);
				while (s.bytesAvailable > 0)
					SSLProtocol.handshakeProtocol.unpacketize(session, s);
				break;
			case recordProtocol.application_data:
				session.putData(fragment);
				break;
			default:
				throw new Error("SSL: recordProtocol: bad data");
			}
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			return recordProtocol.tlsCompressed.packetize(session, type, fragment);
		},
	},

	tlsCompressed: {
		name: "tlsCompressed",
		unpacketize(session, type, fragment) {
			session.traceProtocol(this);
			// unsupported -- just pass through
			recordProtocol.tlsPlainText.unpacketize(session, type, fragment);
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			// unsupported -- just pass through
			return recordProtocol.tlsCipherText.packetize(session, type, fragment);
		},
	},

	tlsCipherText: {
		name: "tlsCipherText",
		calculateMac(hmac, seqNum, type, version, content) {
			hmac.reset();
			var c = seqNum.toChunk();
			var tmps = new SSLStream();
			for (var i = 0, len = 8 - c.byteLength; i < len; i++)
				tmps.writeChar(0);
			tmps.writeChunk(c);
			tmps.writeChar(type);
			tmps.writeChars(version, 2);
			tmps.writeChars(content.byteLength, 2);
			hmac.update(tmps.getChunk());
			hmac.update(content);
			return hmac.close();
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var type = s.readChar();
			var version = s.readChars(2);
			var fragmentLen = s.readChars(2);
			var cipher = session.connectionEnd ? session.serverCipher : session.clientCipher;
			if (cipher) {
				var blksz = session.chosenCipher.cipherBlockSize;
				var hashsz = session.chosenCipher.hashSize;
				if (version >= 0x302 && blksz > 0) { // 3.2 or higher && block cipher
					var iv = s.readChunk(blksz);
					cipher.enc.setIV(iv);
					fragmentLen -= blksz;
				}
				var fragment = s.readChunk(fragmentLen);
				s.close();
				cipher.enc.decrypt(fragment, fragment);
				var padLen = blksz ? (new Uint8Array(fragment))[fragment.byteLength - 1] + 1 : 0;
				fragmentLen -= hashsz + padLen;
				var mac = fragment.slice(fragmentLen, fragmentLen + hashsz);
				if (fragment.byteLength > fragmentLen)
					fragment = fragment.slice(0, fragmentLen);
				if (cipher.hmac) {
					if (Bin.comp(mac, this.calculateMac(cipher.hmac, session.readSeqNum, type, version, fragment)) != 0)
						throw new Error("SSL: recordProtocol: auth failed");
				}
				session.readSeqNum.inc();
			}
			else {
				var fragment = s.readChunk(fragmentLen);
				s.close();
			}
			recordProtocol.tlsCompressed.unpacketize(session, type, fragment);
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			var cipher = session.connectionEnd ? session.clientCipher : session.serverCipher;
			if (cipher) {
				var mac = this.calculateMac(cipher.hmac, session.writeSeqNum, type, session.protocolVersion, fragment);
				session.writeSeqNum.inc();
				// block cipher only, at the moment
				if (cipher.enc) {
					var blksz = session.chosenCipher.cipherBlockSize, iv;
					var tmps = new SSLStream();
					tmps.writeChunk(fragment);
					tmps.writeChunk(mac);
					if (blksz) {
						var length = tmps.bytesWritten + 1;
						var padSize = length % blksz;
						if (padSize > 0)
							padSize = blksz - padSize;
						for (var i = 0; i < padSize; i++)
							tmps.writeChar(padSize);
						tmps.writeChar(padSize);
					}
					if (session.protocolVersion >= 0x302 && blksz) { // 3.2 or higher && block cipher
						iv = Crypt.rng(blksz);
						cipher.enc.setIV(iv);
					}
					fragment = cipher.enc.encrypt(tmps.getChunk());
					if (iv)
						fragment = iv.concat(fragment);
				}
			}
			var s = new SSLStream();
			s.writeChar(type);
			s.writeChars(session.protocolVersion, 2);
			s.writeChars(fragment.byteLength, 2);
			s.writeChunk(fragment);
			return s.getChunk();
		},
	},
};

export default recordProtocol;
