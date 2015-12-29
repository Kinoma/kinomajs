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
var SSL = {
	// default protocol version
	protocolVersion: {
		major: 3,
		minor: 1,
	},

	get Session() {
		return require.weak("ssl/session");
	},

	get supportedCipherSuites() {
		return require.weak("ssl/ciphersuites");
	},

	supportedCompressionMethods: [0],	// NULL

	// constants
	cipherSuite: {
		RSA: 0,
		DSA: 1,
		AES: 0,
		DES: 1,
		TDES: 2,
		RC4: 3,
		SHA1: 0,
		MD5: 1,
		NULL: 255
	},
};
export default SSL;
