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


/*
const dir = "crypt/"

var Crypt = {
	get Digest() {
		return require.weak(dir + "digest");
	},
	get SHA1() {
		return require.weak(dir + "sha1");
	},
	get SHA256() {
		return require.weak(dir + "sha256");
	},
	get SHA512() {
		return require.weak(dir + "sha512");
	},
	get SHA224() {
		return require.weak(dir + "sha224");
	},
	get SHA384() {
		return require.weak(dir + "sha384");
	},
	get MD5() {
		return require.weak(dir + "md5");
	},
	get BlockCipher() {
		return require.weak(dir + "cipher");
	},
	get AES() {
		return require.weak(dir + "aes");
	},
	get DES() {
		return require.weak(dir + "des");
	},
	get TDES() {
		return require.weak(dir + "tdes");
	},
	get StreamCipher() {
		return require.weak(dir + "stream");
	},
	get RC4() {
		return require.weak(dir + "rc4");
	},
	get Chacha() {
		return require.weak(dir + "chacha");
	},
	get Mode() {
		return require.weak(dir + "mode");
	},
	get CBC() {
		return require.weak(dir + "cbc");
	},
	get CTR() {
		return require.weak(dir + "ctr");
	},
	get ECB() {
		return require.weak(dir + "ecb");
	},
	get HMAC() {
		return require.weak(dir + "hmac");
	},
	get HKDF() {
		return require.weak(dir + "hkdf");
	},
	get RSA() {
		return require.weak(dir + "rsa");
	},
	get DSA() {
		return require.weak(dir + "dsa");
	},
	get ECDSA() {
		return require.weak(dir + "ecdsa");
	},
	get PKCS1() {
		return require.weak(dir + "pkcs1");
	},
	get PKCS1_5() {
		return require.weak(dir + "pkcs1_5");
	},
	get PKCS8() {
		return require.weak(dir + "pkcs8");
	},
	get OAEP() {
		return require.weak(dir + "oaep");
	},
	get Curve25519() {
		return require.weak(dir + "curve25519");
	},
	get Ed25519() {
		return require.weak(dir + "ed25519_c");
	},
	get Poly1305() {
		return require.weak(dir + "poly1305");
	},
	get AEAD() {
		return require.weak(dir + "aead");
	},
	get SRPServer() {
		return require.weak(dir + "srp");
	},
	get BER() {
		return require.weak(dir + "ber");
	},
	get X509() {
		return require.weak(dir + "x509");
	},
	rng(n) {
		var RNG = require(dir + "rng");
		return RNG.get(n);
	},
};
export default Crypt;

*/

export default new Proxy({
}, {
	get: function(target, key, receiver) {
		if ("rng" == key) {
			let rng = require.weak("crypt/rng");
			return rng.get;
		}

		//@@ rename modules to eliminate need for this mapping
		if ("SRPServer" == key)
			key = "srp";
		else if ("Ed25519" == key)
			key = "ed25519_c";
		else if ("StreamCipher" == key)
			key = "stream";
		else if ("BlockCipher" == key)
			key = "cipher";

		return require.weak("crypt/" + key.toLowerCase());
	},
	set: function() {
		throw new Error("Crypt is read-only");
	}
});
