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
import SSLProtocol from "ssl/protocol";
import SetupCipher from "ssl/setup";
import SSLStream from "ssl/stream";
import Arith from "arith";

// type
const change_cipher_spec = 1;

var changeCipherSpec = {
	name: "changeCipherSpec",
	unpacketize(session, fragment) {
		session.traceProtocol(this);
		var type = (new SSLStream(fragment)).readChar();
		if (type == change_cipher_spec) {
			session.readSeqNum = new Arith.Integer(0);	// the specification is very ambiguous about the sequence number...
			SetupCipher(session, !session.connectionEnd);
		}
		else
			throw new Error("SSL: changeCipherSpec: bad type");
	},
	packetize(session, type) {
		session.traceProtocol(this);
		var s = new SSLStream();
		if (type === undefined) type = change_cipher_spec;
		s.writeChar(type);
		var upper = SSLProtocol.recordProtocol.packetize(session, SSLProtocol.recordProtocol.change_cipher_spec, s.getChunk());
		session.writeSeqNum = new Arith.Integer(0);	// the specification is very ambiguous about the sequence number...
		SetupCipher(session, session.connectionEnd);
		return upper;
	},
};

export default changeCipherSpec;
