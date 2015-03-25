<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package>
	<patch prototype="Crypt">
		<object name="x509">
			<object name="algorithmIdentifier">
				<array name="algorithm" contents="Number.prototype"/>
				<null name="parameters"/>	<!-- must be SEQUENCE of INTEGER -->
			</object>
			<object name="objectID">
				<array name="oid" contents="Number.prototype"/>
			</object>
			<object name="cert">
				<chunk name="__ber__"/>
				<null name="__key__"/>
				<object name="tbsCertificate">
					<chunk name="__ber__"/>
					<null name="version"/>		<!-- Integer -->
					<null name="serialNumber"/>	<!-- Integer -->
					<object name="signature" prototype="Crypt.x509.algorithmIdentifier"/>
					<string name="issuer"/>
					<null name="issuerDN"/>		<!-- BER decoded array -->
					<object name="validity">
						<null name="notBefore"/>	<!-- Date -->
						<null name="notAfter"/>		<!-- Date -->
					</object>
					<string name="subject"/>
					<null name="subjectDN"/>	<!-- BER decoded array -->
					<object name="subjectPublicKeyInfo">
						<object name="algorithm" prototype="Crypt.x509.algorithmIdentifier"/>	<!-- [0x30 algoId params] -->
						<chunk name="subjectPublicKey"/>
					</object>
					<chunk name="issuerUniqueID"/>
					<chunk name="subjectUniqueID"/>
					<object name="extensions">
						<object name="authorityKeyId">
							<chunk name="keyIdentifier"/>
							<string name="authorityCertIssuer"/>
							<string name="authorityCertSerialNumber"/>
						</object>
						<chunk name="subjectKeyId"/>	<!-- key identifier -->
						<chunk name="keyUsage"/>	<!-- bit string -->
						<object name="basicConstraints">
							<boolean name="cA"/>
							<number name="pathLenConstraint"/>
						</object>
						<array name="extendedKeyUsage" contents="Chunk.prototype"/>
						<array name="policies" contents="Crypt.x509.objectID"/>
						<array name="subjectAlternativeName" contents="String.prototype"/>
					</object>
				</object>
				<object name="signatureAlgorithm" prototype="Crypt.x509.algorithmIdentifier"/>	<!-- algorithm identifier -->
				<chunk name="signatureValue"/>		<!-- bit string -->

				<function name="ber" params="o">
					return (o ? o.__ber__: this.__ber__);
				</function>

				<function name="parse" params="b" script="false">
					return(Crypt.x509.decode(b));
				</function>

				<function name="serialize" params="o" script="false" check="nop">
					return this.ber(o);
				</function>

				<function name="getKey" params="">
					return(this.__key__);
				</function>

				<function name="setKey" params="o, k">
					this.__key__ = k;
				</function>

				<target name="debug">
					<function name="toString">
						var s = "";
						var cert = this.tbsCertificate;
						s += "version: " + cert.version + "\n";
						s += "serial: " + cert.serialNumber + "\n";
						s += "signature: " + cert.signature.algorithm + "\n";
						s += "issuer: " + cert.issuer + "\n";
						s += "validity.notBefore: " + cert.validity.notBefore + "\n";
						s += "validity.notAfter: " + cert.validity.notAfter + "\n";
						s += "subject: " + cert.subject + "\n";
						s += "publicKey.algorithm: " + cert.subjectPublicKeyInfo.algorithm.algorithm + "\n";
						if (cert.extensions.subjectKeyId.length)
							s += "ski: " + cert.extensions.subjectKeyId + "\n";
						if (this.__key__)
							s += "publicKey.key:\n" + Crypt.keyInfo.keyInfoProto.toString.call(this.__key__);
						return(s);
					</function>
				</target>
			</object>

			<!-- This DN conversion function is intended to be compatible with RFC2253 which is specified in the XML sig spec, but it is not guaranteed to be fully compatible. It should be good enough for our purpose for now even if the result is different. -->
			<object name="dn" prototype="Crypt.x509" script="false">
				<function name="binhex" params="b" script="false">
					var str = "";
					for (var i = 0; i < b.length; i++) {
						var d = b.peek(i);
						if (d == undefined)
							break;
						if (d < 0) d += 256;
						str += (d >> 4).toString(16) + (d & 15).toString(16);
					}
					return(str.toLowerCase());
				</function>

				<function name="canon" params="s" script="false">
					var out = "";
					for (var i = 0; i < s.length; i++) {
						var c = s.charAt(i);
						switch (c) {
						case ',': case '=': case '+': case '<': case '>': case '#': case ';':
							out += "\\" + c;
							break;
						default:
							var cc = c.charCodeAt(0);
							if (cc < 0x20 || cc >= 0x7f)
								out += "\\" + (cc >> 4).toString(16) + (cc & 15).toString(16);
							else
								out += c;
							break;
						}
					}
					return(out);
				</function>

				<function name="rdnToString" params="typeAndValue" script="false">
					var type = typeAndValue[1][1];
					var value = typeAndValue[2][1];
					if (!Crypt.ber.oideq(type, [2, 5, 4], 3))
						return(value);	// @@ return the value anyway @@
					switch (type[3]) {
					case 3:	return("CN=" + this.canon(value));		// common
					case 7: return("L=" + this.canon(value));		// locality
					case 8: return("ST=" + this.canon(value));		// state or province
					case 10: return("O=" + this.canon(value));		// organization
					case 11: return("OU=" + this.canon(value));		// organizational unit
					case 6: return("C=" + this.canon(value));		// country
					case 9: return("STREET=" + this.canon(value));		// local address (not in X509)
					case 17: return("POSTAL=" + this.canon(value));		// postal code (ditto)
					case 5: return("SERIAL=" + this.canon(value));		// serial number (ditto)
					case 15: return("BC=" + this.canon(value));		// business category (ditto)
					default:
						var s = "";
						for (var i = 0; i < type.length - 1; i++)
							s += type[i] + ".";
						s += type[i] + "=";
						if (typeof value == "string")
							s += value;
						else {
							// encode as an octet string
							var enc = Crypt.ber.encode(value);
							s += "#" + this.binhex(enc);
						}
						return(s);
					}
				</function>

				<function name="toString" params="name" script="false"><![CDATA[
					var str = "";
					for (var i = name.length; --i >= 1;) {	// reverse order
						var rdn = name[i];
						var rdnstr = "";
						for (var j = 1; j < rdn.length; j++) {
							var ts = this.rdnToString(rdn[j]);
							if (ts != "" && rdnstr != "")
								str += "+";
							rdnstr += ts;
						}
						if (str != "" && rdnstr != "")
							str += ",";
						str += rdnstr;
					}
					return(str);
				]]></function>
			</object>

			<function name="decodeAlgorithmId" params="a" script="false">
				var o = xs.newInstanceOf(Crypt.x509.algorithmIdentifier);
				o.algorithm = a[1][1];
				if (a.length > 2 && a[2][0] != 5)	// not null
					o.parameters = a[2];
				return(o);
			</function>

			<function name="decodeValidity" params="a" script="false">
				var o = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.validity);
				o.notBefore = a[1][1];
				o.notAfter = a[2][1];
				return(o);
			</function>

			<function name="decodePublicKeyInfo" params="a" script="false">
				var o = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.subjectPublicKeyInfo);
				o.algorithm = this.decodeAlgorithmId(a[1]);
				o.subjectPublicKey = a[2][1];
				return(o);
			</function>

			<function name="decodeExtensions" params="c" script="false">
				var e = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.extensions);
				var ber = Crypt.ber.decode(c);
				for (var i = 1; i < ber.length; i++) {
					var ext = ber[i];
					var extnID = ext[1][1];
					if (ext[2][0] == 0x01) {	// boolean
						var critical = ext[2][1];
						var extnValue = ext[3][1];
					}
					else {
						var critical = false;
						var extnValue = ext[2][1];
					}
					if (!Crypt.ber.oideq(extnID, [2, 5, 29], 3))	// id-ce
						continue;
					var extn = Crypt.ber.decode(extnValue);
					switch (extnID[3]) {
					case 14:	// subjectKeyId
						e.subjectKeyId = extn[1];
						break;
					case 35:	// authorityKeyId
						e.authorityKeyId = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.extensions.authorityKeyId);
						for (var j = 1; j < extn.length; j++) {
							switch (extn[j][0] & 0x1f) {
							case 0:	// keyIdentifier -- OCTET STRING
								e.authorityKeyId.keyIdentifier = extn[j][1];
								break;
							case 1:	// authorityCertIssuer -- GeneralNames
								e.authorityKeyId.authorityCertIssuer = this.dn.toString(Crypt.ber.decode(Crypt.ber.decode(extn[j][1])[1]));
								break;
							case 2:	// authorityCertSerialNumber -- INTEGER
								e.authorityKeyId.authorityCertSerialNumber = Crypt.ber.decodeTag(0x02, extn[j][1]);
								break;
							}
						}
						break;
					case 19:	// basicConstraints
						e.basicConstraints = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.extensions.basicConstraints);
						e.basicConstraints.cA = false;
						e.basicConstraints.pathLenConstraint = undefined;
						if (extn.length > 1)
							e.basicConstraints.cA = extn[1][1];
						if (extn.length > 2)
							e.basicConstraints.pathLenConstraint = extn[2][1];
						break;
					case 15:	// keyUsage
						e.keyUsage = extn[1];
						break;
					case 37:	// extendedKeyUsage
						e.extendedKeyUsage = new Array();
						for (var j = 1; j < extn.length; j++)
							e.extendedKeyUsage.push(extn[j]);
						break;
					case 32:	// certificate policies
						e.policies = new Array();
						for (var j = 1; j < extn.length; j++) {
							var o = xs.newInstanceOf(Crypt.x509.objectID);
							o.oid = extn[j][1][1];
							e.policies.push(o);
						}
						break;
					case 31:	// CRL distribution point
						break;
					case 17:	// subject alternative name
						e.subjectAlternativeName = new Array();
						for (var j = 1; j < extn.length; j++) {
							switch (extn[j][0]) {
							case 0x81:	// rfc822
							case 0x82:	// DNS name
							case 0x86:	// URI
								var name = extn[j][1].toRawString();
								break;
							default:
								var name = extn[j][1];	// as-is
								break;
							}
							e.subjectAlternativeName.push(name);
						}
						break;
					default:	// unsupported extensions -- just ignore
						break;
					}
				}
				return(e);
			</function>

			<function name="decodeTBSCert" params="ber" script="false">
				var o = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate);
				o.__ber__ = ber;
				var b = Crypt.ber.decode(ber);
				var i = 1;
				if (b[i][0] & 0x80)
					o.version = Crypt.ber.decode(b[i++][1])[1];
				else
					o.version = 0;
				o.serialNumber = b[i++][1];
				o.signature = this.decodeAlgorithmId(b[i++]);
				o.issuerDN = b[i++];
				o.issuer = this.dn.toString(o.issuerDN);
				o.validity = this.decodeValidity(b[i++]);
				o.subjectDN = b[i++];
				o.subject = this.dn.toString(o.subjectDN);
				o.subjectPublicKeyInfo = this.decodePublicKeyInfo(b[i++]);
				// process the OPTIONALs
				o.extensions = undefined;
				for (; i < b.length; i++) {
					var tag = b[i][0];
					switch (tag & 0x1f) {
					case 1:		// issuerUniqueID
						o.issuerUniqueID = b[i][1];
						break;
					case 2:		// subjectUniqueID
						o.subjectUniqueID = b[i][1];
						break;
					case 3:		// extensions
						o.extensions = this.decodeExtensions(b[i][1]);
						break;
					}
				}
				if (!o.extensions)
					o.extensions = xs.newInstanceOf(Crypt.x509.cert.tbsCertificate.extensions);	// needs a new instance
				return(o);
			</function>

			<function name="makeSKI" params="o" script="false">
				var h = new Crypt.SHA1();
				h.update(o.tbsCertificate.subjectPublicKeyInfo.subjectPublicKey);
				return(h.close());
			</function>

			<function name="decode" params="x509">
				var o = xs.newInstanceOf(Crypt.x509.cert);
				var ber = Crypt.ber.decode(x509, true);
				// ber must be [0x30 [0x30 tbsCert] [0x30 sigAlgo] [0x03 sigValue]]
				o.tbsCertificate = this.decodeTBSCert(ber[1][1]);
				o.signatureAlgorithm = this.decodeAlgorithmId(Crypt.ber.decode(ber[2][1]));
				o.signatureValue = ber[3][1];	// dont decode this here -- this depends on the signature algorithm
				// copy the original chunk
				o.__ber__ = x509.slice();
				// make sure the SKI exists
				if (o.tbsCertificate.extensions.subjectKeyId.length == 0)
					o.tbsCertificate.extensions.subjectKeyId = this.makeSKI(o);
				// extract the PK
				var pk = o.tbsCertificate.subjectPublicKeyInfo;
				o.__key__ = Crypt.keyInfo.pkcs1.parsePK(pk.algorithm, pk.subjectPublicKey);
				o.__key__.keyName = o.tbsCertificate.extensions.subjectKeyId.toString();
				// the key name needs to be unique for each cert
				/*
				var h = new Crypt.SHA1();
				h.update(x509);
				o.__key__.keyName = (h.close()).toString();
				*/
				return(o);
			</function>

			<function name="decodeSubjectKeyId" params="x509" c="xs_x509_decodeSubjectKeyId"/>

			<function name="verify" params="x509obj, key">
				// calculate the hash value according to the signatureAlgorithm
				// and verify it with the public key
				var result = false;
				var algoID = x509obj.signatureAlgorithm.algorithm;
				if (Crypt.ber.oideq(algoID, [1, 2, 840, 113549, 1, 1, 4]) ||
				    Crypt.ber.oideq(algoID, [1, 2, 840, 113549, 1, 1, 5]) ||
				    Crypt.ber.oideq(algoID, [1, 2, 840, 113549, 1, 1, 11])) {
					// sanity check
					if (!key.hasOwnProperty("rsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					if (Crypt.ber.oideq(algoID, [1, 2, 840, 113549, 1, 1, 11]))
						// PKCS-1 SHA256 with RSA encryption
						var digest = new Crypt.SHA256();
					else if (Crypt.ber.oideq(algoID, [1, 2, 840, 113549, 1, 1, 5]))
						// PKCS-1 SHA1 with RSA encryption
						var digest = new Crypt.SHA1();
					else
						// PKCS-1 MD5 with RSA encryption
						var digest = new Crypt.MD5();
					digest.update(x509obj.tbsCertificate.__ber__);
					var v = digest.close();
					var sig = new Crypt.PKCS1_5(key.rsaKey);
					result = sig.verify(v, x509obj.signatureValue);
				}
				else if (Crypt.ber.oideq(algoID, [1, 2, 840, 10040, 4, 3]) ||
					 Crypt.ber.oideq(algoID, [1, 3, 14, 3, 2, 27])) {
					// sanity check
					if (!key.hasOwnProperty("dsaKey"))
						throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
					// DSA with SHA1
					var digest = new Crypt.SHA1();
					digest.update(x509obj.tbsCertificate.__ber__);
					var v = digest.close();
					var sig = new Crypt.DSA(key.dsaKey);
					// signature is in SEQUENCE {r INTEGER, s INTEGER}
					var sigObj = Crypt.ber.decode(x509obj.signatureValue);
					var r = sigObj[1][1];
					var s = sigObj[2][1];
					result = sig._verify(v, r, s);
				}
				else
					throw new Crypt.Error(Crypt.error.kCryptInvalidAlgorithm);
				return(result);
			</function>
		</object>
	</patch>
</package>