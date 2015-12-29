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
import SSL from "ssl";
import PRF from "ssl/prf";
import SSLStream from "ssl/stream";
import Crypt from "crypt";

function setupSub(o, cipher)
{
	o.hmac = new Crypt.HMAC(cipher.hashAlgorithm == SSL.cipherSuite.MD5 ? new Crypt.MD5() : new Crypt.SHA1(), o.macSecret);
	switch (cipher.cipherAlgorithm) {
	case SSL.cipherSuite.DES:
		var enc = new Crypt.DES(o.key);
		break;
	case SSL.cipherSuite.TDES:
		var enc = new Crypt.TDES(o.key);
		break;
	case SSL.cipherSuite.AES:
		var enc = new Crypt.AES(o.key);
		break;
	case SSL.cipherSuite.RC4:
		var enc = new Crypt.RC4(o.key);
		break;
	default:
		throw new Error("SSL: SetupCipher: unkown algorithm");
	}
	if (cipher.cipherBlockSize != 0)
		o.enc = new Crypt.CBC(enc, o.iv);	// no padding -- SSL 3.2 requires padding process beyond RFC2630
	else
		o.enc = enc;
}

function SetupCipher(session, connectionEnd)
{
	var random = session.serverRandom;
	random = random.concat(session.clientRandom);
	var chosenCipher = session.chosenCipher;
	if (session.protocolVersion.major == 3 && session.protocolVersion.minor == 1)	// version 3.1
		var nbytes = chosenCipher.cipherKeySize * 2 + chosenCipher.cipherBlockSize * 2 + chosenCipher.hashSize * 2;
	else	// version 3.2 or higher
		var nbytes = chosenCipher.cipherKeySize * 2 + chosenCipher.hashSize * 2;
	var keyBlock = PRF(session.masterSecret, "key expansion", random, nbytes);
	var s = new SSLStream(keyBlock);
	var o = {};
	if (connectionEnd) {
		o.macSecret = s.readChunk(chosenCipher.hashSize);
		void s.readChunk(chosenCipher.hashSize);
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		void s.readChunk(chosenCipher.cipherKeySize);
		if (s.bytesAvailable > 0)
			o.iv = s.readChunk(chosenCipher.cipherBlockSize);
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.clientCipher = o;
	}
	else {
		void s.readChunk(chosenCipher.hashSize);
		o.macSecret = s.readChunk(chosenCipher.hashSize);
		void s.readChunk(chosenCipher.cipherKeySize);
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		if (s.bytesAvailable > 0) {
			void s.readChunk(chosenCipher.cipherBlockSize);
			o.iv = s.readChunk(chosenCipher.cipherBlockSize);
		}
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.serverCipher = o;
	}
}

export default SetupCipher;
