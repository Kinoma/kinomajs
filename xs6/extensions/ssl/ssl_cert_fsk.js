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

export default class CertificateManager {
	constructor(options) {
		this.calist = new Map();
		this.registeredCerts = new Map();
		this.self = {certs: [], keys: []};
		if (options.calist) {
			let Files = require.weak("files");
			let bundle = Files.read(options.calist);
			if (calist)
				this.processCAList(bundle, this.calist);
		}
		if (options.certificates)	// use this option to register application specific certs
			options.certificates.forEach(e => this.register(e));
		if (options.self)		// key pairs to be used for either server cert/key or client auth cert/key
			this.processKeyPairs(options.self, this.self, options.password);
		if (options.dh) {
			let ber = new Crypt.BER(options.dh);
			if (ber.getTag() == 0x30) {
				ber.getLength();
				let p = ber.getInteger();
				let g = ber.getInteger();
				this.dh = {p, g};
			}
		}
	};
	getCerts() {
		// return the self certs
		return this.self.certs;
	};
	getKey(cert) {
		let Bin = require.weak("bin");
		if (!cert || Bin.comp(cert, this.self.certs[0]) == 0)
			return this.self.keys[0];
		if (this.registeredCerts.has(cert)) {
			var x509 = Crypt.X509.decode(cert);
			return x509.spki;	// public key only
		}
	};
	findPreferedCert(types, names) {
		// should choose the cert according to types and names, but for now...
		return this.self.certs[0];
	};
	verify(certs, options) {
		let n = certs.length;
		let x509 = [];
		for (let i = 0; i < n; i++)
			x509[i] = Crypt.X509.decode(certs[i]);
		for (let i = 0; i < n - 1; i++) {
			let spki = x509[i + 1].spki;
			if (!this._verify(spki, x509[i]))
				return false;
		}
		let tbs = Crypt.X509.decodeTBS(x509[n - 1].tbs);
		let issuer = (new Crypt.SHA1()).process(tbs.issuer);
		let root = this.calist.get(issuer);
		if (root) {
			let spki = Crypt.X509.decodeSPKI(root);
			return spki && this._verify(spki, x509[n - 1]);
		}
	};
	_verify(spki, x509) {
		var pk;
		var hash;
		var sig;
		switch (x509.algo.toString()) {
		case [1, 2, 840, 113549, 1, 1, 4].toString():	// PKCS-1 MD5 with RSA encryption
			hash = Crypt.MD5;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 5].toString():	// PKCS-1 SHA1 with RSA encryption
			hash = Crypt.SHA1;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 11].toString():	// PKCS-1 SHA256 with RSA encryption
			hash = Crypt.SHA256;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 12].toString():	// PKCS-1 SHA384 with RSA encryption
			hash = Crypt.SHA384;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 13].toString():	// PKCS-1 SHA512 with RSA encryption
			hash = Crypt.SHA512;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 113549, 1, 1, 14].toString():	// PKCS-1 SHA224 with RSA encryption
			hash = Crypt.SHA224;
			pk = Crypt.PKCS1_5;
			sig = x509.sig;
			break;
		case [1, 2, 840, 10040, 4, 3].toString():
		case [1, 3, 14, 3, 2, 27].toString():
			hash = Crypt.SHA1;
			pk = Crypt.DSA;
			// needs to decode the sig value into <r, s>
			var ber = new Crypt.BER(x509.sig);
			if (ber.getTag() == 0x30) {
				ber.getLength();
				var r = ber.getInteger();
				var s = ber.getInteger();
				sig = r.concat(s);
			}
			break;
		default:
			throw new Error("Cert: unsupported algorithm: " + x509.algo.toString());
			break;
		}
		var H = (new hash()).process(x509.tbs);
		return (new pk(spki, false, [] /* any oid */)).verify(H, sig);
	};
	register(cert) {
		this.registeredCerts.set(cert);
	};
	getDH() {
		return this.dh;
	};

	processCAList(pem, calist) {
		let Bin = require.weak("bin");
		let msgs = Bin.pem_decode(c);
		for (let i = 0; i < msgs.length; i++) {
			var o = msgs[i];
			switch (o.keyword) {
			case "CERTIFICATE":
			case "X509 CERTIFICATE":
				let x509 = Crypt.X509.decode(o.body);
				let tbs = Crypt.X509.decodeTBS(x509.tbs)
				let subject = (new Crypt.SHA1()).process(tbs.subject);
				calist.set(subject, o.body);
				break;
			}
		}
		return calist;
	};
	processKeyPairs(c, pair, pass) {
		let Bin = require.weak("bin");
		let msgs = Bin.pem_decode(c), pk8;
		for (let i = 0; i < msgs.length; i++) {
			var o = msgs[i];
			switch (o.keyword) {
			case "CERTIFICATE":
			case "X509 CERTIFICATE":
				pair.certs.push(o.body);
				break;
			case "PRIVATE KEY":
				pk8 = Crypt.PKCS8.parse(o.body);
				if (!pk8)
					throw new Error("SSL: PEM: malformed PKCS8");
				pair.keys.push(pk8);
				break;
			case "ENCRYPTED PRIVATE KEY":
				pk8 = Crypt.PKCS8.decrypt(o.body, pass);
				if (!pk8)
					throw new Error("SSL: PEM: PKCS8 decryption failed");
				pair.keys.push(pk8);
				break;
			case "RSA PRIVATE KEY":
			case "DSA PRIVATE KEY":
			default:
				throw new Error("SSL: PEM: unsupported keyword: " + o.keyword);
				break;
			}
		}
	}
};
