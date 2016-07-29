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

// for MC
import Files from "files";
import File from "file";
import Crypt from "crypt";

export default class CertificateManager {
	constructor(options) {
		this.registeredCerts = [];
		if (options.certificate)
			this.register(options.certificate);
	};
	getCerts() {
		// return the self certs
		return [Files.read("srvcert.der")];
	};
	getKey(cert) {
		let Configuration = require.weak("config");
		let Bin = require.weak("bin");
		if (!cert) {
			// return the self key
			return Crypt.PKCS8.decrypt(Files.read("srvkey.pk8"), Configuration.pk8Password);
		}
		// look for the key corresponding to the cert
		// first, search in the registed cert
		for (var i = 0; i < this.registeredCerts.length; i++) {
			if (Bin.comp(this.registeredCerts[i], cert) == 0) {
				var x509 = Crypt.X509.decode(this.registeredCerts[i]);
				return x509.spki;	// public key only
			}
		}
		// at the moment there is only one key
		if (Bin.comp(Files.read("clntcert.der"), cert) == 0)
			return Crypt.PKCS8.decrypt(Files.read("clntkey.pk8"), Configuration.pk8Password);
	};
	findPreferedCert(types, names) {
		// MC has only one key pair...
		return [Files.read("clntcert.der")];
	};
	getIndex(fname, target) {
		let Bin = require.weak("bin");
		var f = new File(fname), buf, res = -1;
		for (var i = 0; buf = f._read(20); i++) {
			if (Bin.comp(buf, target) == 0) {
				res = i;
				break;
			}
		}
		f.close();
		return res;
	};
	findCert(fname, target) {
		var i = this.getIndex(fname, target);
		if (i < 0)
			return;	// undefined
		var ca = "ca" + i + ".der";
		return Crypt.X509.decodeSPKI(Files.read(ca));
	};
	verify(certs, options) {	// @@ support additional certificates
		var n = certs.length;
		var x509 = [];
		for (var i = 0; i < n; i++)
			x509[i] = Crypt.X509.decode(certs[i]);
		for (var i = 0; i < n - 1; i++) {
			var spki = x509[i + 1].spki;
			if (!this._verify(spki, x509[i]))
				return false;
		}
		var aki = Crypt.X509.decodeAKI(certs[n - 1]);
		if (aki) {
			var spki = this.findCert("ca.ski", aki);
			if (spki && this._verify(spki, x509[n - 1]))
				return true;
			// else fall thru
		}
		var tbs = Crypt.X509.decodeTBS(x509[n - 1].tbs);
		var sha1 = new Crypt.SHA1();
		var issuer = sha1.process(tbs.issuer);
		var spki = this.findCert("ca.subject", issuer);
		return spki && this._verify(spki, x509[n - 1]);
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
		this.registeredCerts.push(cert);
	};
	getDH() {
		let dh = Files.read("dh.der");
		let ber = new Crypt.BER(dh);
		if (ber.getTag() == 0x30) {
			ber.getLength();
			let p = ber.getInteger();
			let g = ber.getInteger();
			return {p, g};
		}
	};
};
