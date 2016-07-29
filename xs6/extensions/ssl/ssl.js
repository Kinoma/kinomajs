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
var SSL = {
	protocolVersion: (3 << 8) | 1,	// default protocol version
	minProtocolVersion: (3 << 8) | 1,
	maxProtocolVersion: (3 << 8) | 3,

	get Session() {
		return require.weak("ssl/session");
	},

	get supportedCipherSuites() {
		return require.weak("ssl/ciphersuites");
	},

	supportedCompressionMethods: [0],	// NULL

	// constants
	cipherSuite: {
		// key exchange algorithms
		RSA: 0,
		DHE_DSS: 1,
		DHE_RSA: 2,
		DH_ANON: 3,
		DH_DSS: 4,
		DH_RSA: 5,
		// encryption algroithms
		AES: 0,
		DES: 1,
		TDES: 2,
		RC4: 3,
		// hash algorithms
		SHA1: 0,
		MD5: 1,
		SHA256: 2,
		SHA384: 3,
		NULL: 255,
		// certificate type
		CERT_RSA: 0,
		CERT_DSA: 1,
		// encryption mode
		NONE: 0,
		CBC: 1,
		GCM: 2,
	},
};
export default SSL;
