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

const warning = 1;
const fatal = 2;
const close_notify = 0;
const unexpected_message = 10;
const bad_record_mac = 20;
const decryption_failed = 21;
const record_overflow = 22;
const decompression_failure = 30;
const handshake_failure = 40;
const bad_certificate = 42;
const unsupported_certificate = 43;
const certificate_revoked = 44;
const certificate_expired = 45;
const certificate_unknown = 46;
const illegal_parameter = 47;
const unknown_ca = 48;
const access_denied = 49;
const decode_error = 50;
const decrypt_error = 51;
const export_restriction = 60;
const protocol_version = 70;
const insufficient_security = 71;
const internal_error = 80;
const user_canceled = 90;
const no_negotiation = 100;
const unsupported_extension = 110;           /* new */
const certificate_unobtainable = 111;        /* new */
const unrecognized_name = 112;               /* new */
const bad_certificate_status_response = 113; /* new */
const bad_certificate_hash_value = 114;      /* new */
          
var alert = {
	name: "alert",
	// global constants
	close_notify: close_notify,

	unpacketize(session, fragment) {
		session.traceProtocol(this);
		var s = new SSLStream(fragment);
		session.alert = {level: s.readChar(), description: s.readChar()};
		if (session.alert.description != close_notify)
			throw new Error("SSL: alert: " + session.alert.level + ", " + session.alert.description);
	},
	packetize(session, level, description) {
		session.traceProtocol(this);
		var s = new SSLStream();
		s.writeChar(level);
		s.writeChar(description);
		var upper = SSLProtocol.recordProtocol.packetize(session, SSLProtocol.recordProtocol.alert, s.getChunk());
		return upper;
	},
};
export default alert;
