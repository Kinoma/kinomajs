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
import SSL from "ssl";

export const supportedCipherSuites = [
	{
		// TLS_RSA_WITH_AES_128_CBC_SHA
		value: [0x00, 0x2f],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA1,
		hashSize: 20,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_RSA_WITH_AES_256_CBC_SHA
		value: [0x00, 0x35],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA1,
		hashSize: 20,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_RSA_WITH_DES_CBC_SHA
		value: [0x00, 0x09],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.DES,
		cipherKeySize: 8,
		cipherBlockSize: 8,
		hashAlgorithm: SSL.cipherSuite.SHA1,
		hashSize: 20,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_RSA_WITH_3DES_EDE_CBC_SHA
		value: [0x00, 0x0A],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.TDES,
		cipherKeySize: 24,
		cipherBlockSize: 8,
		hashAlgorithm: SSL.cipherSuite.SHA1,
		hashSize: 20,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_RSA_WITH_RC4_128_MD5
		value: [0x00, 0x04],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.RC4,
		cipherKeySize: 16,
		cipherBlockSize: 0,
		hashAlgorithm: SSL.cipherSuite.MD5,
		hashSize: 16,
		encryptionMode: SSL.cipherSuite.NONE,
	},
	{
		// TLS_RSA_WITH_RC4_128_SHA
		value: [0x00, 0x05],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.RC4,
		cipherKeySize: 16,
		cipherBlockSize: 0,
		hashAlgorithm: SSL.cipherSuite.SHA1,
		hashSize: 20,
		encryptionMode: SSL.cipherSuite.NONE,
	},
	{
		// TLS_RSA_WITH_AES_128_CBC_SHA256
		value: [0x00, 0x3c],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA256,
		hashSize: 32,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_RSA_WITH_AES_256_CBC_SHA256
		value: [0x00, 0x3d],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA256,
		hashSize: 32,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
		value: [0x00, 0x67],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.DHE_RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA256,
		hashSize: 32,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_DHE_RSA_WITH_AES_256_CBC_SHA256
		value: [0x00, 0x6b],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.DHE_RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA256,
		hashSize: 32,
		encryptionMode: SSL.cipherSuite.CBC,
	},
	{
		// TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 (RFC 5288)
		value: [0x00, 0x9e],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.DHE_RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA256,
		hashSize: 32,
		encryptionMode: SSL.cipherSuite.GCM,
		ivSize: 8,	// explicit nonce size
		saltSize: 4,	// implicit part
	},
	{
		// TLS_DHE_RSA_WITH_AES_256_GCM_SHA384 (RFC 5288)
		value: [0x00, 0x9f],
		isExportable: false,
		keyExchangeAlgorithm: SSL.cipherSuite.DHE_RSA,
		cipherAlgorithm: SSL.cipherSuite.AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SSL.cipherSuite.SHA384,
		hashSize: 48,
		encryptionMode: SSL.cipherSuite.GCM,
		ivSize: 8,	// explicit nonce size
		saltSize: 4,	// implicit part
	},
	{
		// TLS_NULL_WITH_NULL_NULL
		value: [0x00, 0x00],
		isExportable: true,	// ?
		keyExchangeAlgorithm: SSL.cipherSuite.NULL,
		cipherAlgorithm: SSL.cipherSuite.NULL,
		cipherKeySize: 0,
		cipherBlockSize: 0,
		hashAlgorithm: SSL.cipherSuite.NULL,
		hashSize: 0,
		encryptionMode: SSL.cipherSuite.NONE,
	},
];

export default supportedCipherSuites;
