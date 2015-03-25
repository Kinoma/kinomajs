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
	<namespace prefix="wsse" uri="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"/>

	<patch prototype="Crypt">
		<object name="wsse">
			<string name="base64encodingURI" value="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#Base64Binary"/>
			<string name="x509typeURI" value="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#X509v3"/>
			<string name="x509pathURI" value="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509PKIPathv1"/>
			<string name="x509SKIURI" value="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd#X509SubjectKeyIdentifier"/>

			<function name="getElement" params="id, element" script="false">
				for (var i = 0, children = element.children; i < children.length; i++) {
					var c = children[i];
					if (!xs.isInstanceOf(c, xs.infoset.element))
						continue;
					for (var j = 0, attrs = c.attributes; j < attrs.length; j++) {
						var a = attrs[j];
						if (a.name == "Id" && a.value == id)	// do not care of namespace for now
							return(c);
					}
					var ret = this.getElement(id, c);
					if (ret) return(ret);
				}
			</function>

			<function name="fixNS" params="element" script="false">
				if (!element.namespace) {
					var prefix = element.prefix;
					if (!prefix) prefix = "xmlns";
					var nsAttr = Crypt.xml.getAncientNS(element, prefix);
					if (nsAttr)
						element.namespace = nsAttr.value;
				}
				for (var i = 0, children = element.children; i < children.length; i++) {
					var c = children[i];
					if (xs.isInstanceOf(c, xs.infoset.element))
						this.fixNS(c);
				}
			</function>

			<function name="replaceElement" params="id, element, newElm, fixNS" script="false">
				for (var i = 0, children = element.children; i < children.length; i++) {
					var c = children[i];
					if (!xs.isInstanceOf(c, xs.infoset.element))
						continue;
					for (var j = 0, attrs = c.attributes; j < attrs.length; j++) {
						var a = attrs[j];
						if (a.name == "Id" && a.value == id) {	// do not care of namespace for now
							children[i] = newElm;
							newElm.parent = c.parent;
							if (fixNS) this.fixNS(newElm);
							return(true);
						}
					}
					if (this.replaceElement(id, c, newElm, fixNS))
						return(true);
				}
				return(false);
			</function>

			<function name="findSecurityHeader" params="element" script="false">
				for (var i = 0, children = element.children; i < children.length; i++) {
					var c = children[i];
					if (!xs.isInstanceOf(c, xs.infoset.element))
						continue;
					if (c.name == "Security" && c.namespace == "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd")
						return(c);
					var ret = this.findSecurityHeader(c);
					if (ret) return(ret);
				}
			</function>

			<function name="getKeyFromBinarySecurityToken" params="token">
				if (token.encodingType != this.base64encodingURI)
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);	// unknown, or not supported encoding type
				switch (token.valueType) {
				case this.x509typeURI:
					var cert = this.certificates.processCerts([token.value]);
					break;
				case this.x509pathURI:
					var ber = Crypt.ber.decode(token.value, true);
					var certs = [];
					for (var i = 1; i < ber.length; i++)	// skip tag and start with the decoded elements
						certs.push(ber[i][1]);
					var cert = this.certificates.processCerts(certs); // do not register, just want to know the most descendent cert
					break;
				default:
					throw new Crypt.Error(Crypt.error.kCryptUnsupportedAlgorithm);	// unknown value type
				}
				var pubKey = cert.getKey();
				var privKey = this.keyring.get(pubKey.keyName);
				return(privKey ? privKey: pubKey);
			</function>

			<function name="getObjectByID" params="id" script="false">
				for (var i = 0, elms = this.proto.elements, l = elms.length; i < l; i++) {
					var e = elms[i];
					if (e.getID() == id)
						return(e);
				}
			</function>

			<function name="resolveReferencedKey" params="tref">
				var key;
				// try out in the preferred order specified in the spec
				if (tref.reference.uri) {
					var uri = tref.reference.uri;
					if (uri.charAt(0) == '#') {
						var o = this.getObjectByID(uri.slice(1));
						if (o && xs.isInstanceOf(o, Crypt.wsse.binarySecurityToken))
							key = this.getKeyFromBinarySecurityToken(o);
					}
					else
						throw new Crypt.Error(Crypt.error.kCryptParameterError);	// unsupported reference type, maybe such as http://...
				}
				else if (tref.keyIdentifier.value) {
					switch (tref.keyIdentifier.valueType) {
					case this.x509SKIURI:
						// try to get a private key by the SKI as keyName
						var ski = Crypt.xml.removeSpaces(tref.keyIdentifier.value);
						key = this.keyring.get(ski);
						if (!key) {
							var cert = this.certificates.get(ski);
							if (cert) key = cert.getKey();
						}
						break;
					default:
						// search a security token by this value
						// not supported yet
						break;
					}
				}
				else {
					// embedded reference
					for (var i = 0, elms = tref.embedded.elements; i < elms.length; i++) {
						var e = elms[i];
						// only well known contents are processed
						if (xs.isInstanceOf(e, Crypt.saml.assertion)) {
							// get the verification key from the SAML assertion?
							// not supported yet
						}
						else if (xs.isInstanceOf(e, Crypt.wsse.binarySecurityToken))
							key = this.getKeyFromBinarySecurityToken(e);
					}
				}
				return(key);
			</function>

			<function name="getKeyInfo" params="key" script="false">
				if (typeof key == "string") {
					// must be securityTokenReference
					var o = this.getObjectByID(key);
					if (!o)
						throw new Crypt.Error(Crypt.error.kCryptKeyNotFound);	// 'key' not found
					var keyID = key;
					key = o;
				}
				if (xs.isInstanceOf(key, Crypt.wsse.securityTokenReferenceGrammar))
					key = key.content;
				if (xs.isInstanceOf(key, Crypt.wsse.securityTokenReference)) {
					var keyInfo = new Crypt.xml.KeyInfo({securityTokenReference: key}); // just copy?
					keyInfo.rawKey = this.resolveReferencedKey(key);
				}
				else if (xs.isInstanceOf(key, Crypt.wsse.binarySecurityToken)) {
					if (keyID)
						var keyInfo = new Crypt.xml.KeyInfo({securityTokenReference: {reference: {uri: '#' + keyID}}}); // make up a Security Token Reference
					else
						var keyInfo = new Crypt.xml.KeyInfo({securityTokenReference: {embedded: {elements: [key]}}}); // make it an embedded type
					keyInfo.rawKey = this.getKeyFromBinarySecurityToken(key);
				}
				else if (xs.isInstanceOf(key, Crypt.keyInfo.keyInfoProto)) {
					var keyInfo = Crypt.xml.getKeyInfoFromKey(key);
					keyInfo.rawKey = key;
				}
				else if (xs.isInstanceOf(key, Crypt.x509.cert)) {
					// var keyInfo = new Crypt.xml.KeyInfo({x509: [new Crypt.xml.X509DataObject({certs: [key.serialize()]})]});
					var keyInfo = new Crypt.xml.KeyInfo({x509: [new Crypt.xml.X509DataObject({skis: [key.tbsCertificate.extensions.subjectKeyId]})]});
					keyInfo.rawKey = key.getKey();
				}
				else
					throw new Crypt.Error(Crypt.error.kCryptTypeError);	// parameter type error
				return(keyInfo);
			</function>

			<function name="verify" params="rootIS, key">
				// all signatures whose parent is <Security> are verified blindly
				if (key)
					var keyInfo = this.getKeyInfo(key);
				var verified = [];
				for (var i = 0, elms = this.proto.elements; i < elms.length; i++) {
					var e = elms[i];
					if (e.hasOwnProperty("content") && (xs.isInstanceOf(e.content, Crypt.xml.signature))) {
						var sig = e.content;
						if (keyInfo)
							var xsigKey = keyInfo.rawKey;
						else
							// use securityTokenReference method to resolve the key, otherwise let the standard XML sig processing do
							var xsigKey = this.resolveReferencedKey(sig.keyInfo.securityTokenReference);
						var ds = new Crypt.XML(sig, xsigKey, this.keyring, this.certificates);
						for (var j = 0, refs = sig.signedInfo.references, l = refs.length; j < l; j++) {
							var ref = refs[j];
							var t = this.getElement(ref.uri.slice(1), rootIS);
							if (!t || !ds.verify.addRef(t, ref))
								throw new Crypt.Error(Crypt.error.kCryptVerificationError, "does not match digest: " + ref.uri);
						}
						if (!ds.verify.done(e.__xs__infoset))
							throw new Crypt.Error(Crypt.error.kCryptVerificationError, "does not match signature");
						verified.push(sig);
					}
				}
				return(verified);
			</function>

			<function name="sign" params="rootIS, targets, key, id">
				// sign all the targets and prepend a <Signature> element to the security header
				// is it OK either rootIS is 'document' or 'element'?

				var keyInfo = this.getKeyInfo(key);
				var proto = new Crypt.xml.Signature(this.defaultSignatureProperties);
				proto.keyInfo = keyInfo;
				proto.id = id;
				var xsig = new Crypt.XML(proto, keyInfo.rawKey, this.keyring, this.certificates);
				for (var i = 0; i < targets.length; i++) {
					var ref = new Crypt.xml.SignatureReference(this.defaultSignatureReferenceProperties);
					var id = targets[i];
					ref.uri = "#" + id;
					var t = this.getElement(id, rootIS);
					if (!t)
						throw new Crypt.Error(Crypt.error.kCryptObjectNotFound);	// target not found
					xsig.sign.addRef(t, ref);
				}
				var str = xsig.sign.done();

				// prepend the <Signature> element to the Security Header
				var sigIS = xs.infoset.scan(str);
				var securityHeader = this.findSecurityHeader(rootIS);
				securityHeader.children.unshift(sigIS.element);
			</function>

			<function name="decrypt" params="rootIS, key">
				// decrypt all encrypted data and replace it with the plain data
				if (key)
					var keyInfo = this.getKeyInfo(key);
				// find <EncryptedKey>
				for (var i = 0, elms = this.proto.elements; i < elms.length; i++) {
					var e = elms[i];
					if (e.hasOwnProperty("content") && (e.content instanceof Crypt.xml.EncryptedKey)) {
						var enc = e.content;
						if (keyInfo)
							var xencKey = keyInfo.rawKey;
						else
							// use securityTokenReference method to resolve the key, otherwise let the standard XML enc processing do
							var xencKey = this.resolveReferencedKey(enc.keyInfo.securityTokenReference);
						var decKey = (new Crypt.XML(enc, xencKey, this.keyring, this.certificates)).decrypt();
						var symKey = new Crypt.KeyInfo({sym: decKey});
						for (var j = 0, refs = enc.referenceList.dataRefs; j < refs.length; j++) {
							var id = refs[j].uri.slice(1);
							var targetIS = this.getElement(id, rootIS);
							if (targetIS.instance)
								var target = targetIS.instance;
							else
								var target = xs.infoset.parse(Crypt.xml.makeItRoot(targetIS), xs.infoset.PARSE_DEFAULT, Crypt.xml.encryption);
							var c = (new Crypt.XML(target, symKey, this.keyring, this.certificates)).decrypt();
							this.replaceElement(id, rootIS, (xs.infoset.scan(c.toRawString())).element, true);
						}
					}
				}

				// get the changes back to this.proto
				var securityHeader = this.findSecurityHeader(rootIS);
				this.proto = xs.infoset.parse(Crypt.xml.makeItRoot(securityHeader), xs.infoset.PARSE_DEFAULT, Crypt.wsse.security);
			</function>

			<function name="encrypt" params="rootIS, targets, key, id, symKey">
				// encrypt all the targets and replace it with the <EncryptedData> element
				// the EncryptedKey method is adopted
				// is it OK either rootIS is 'document' or 'element'?

				if (!symKey) {
					// make up a random symmetric key if not specified
					var symKey = Crypt.defaultRNG.get(16);	// must be the same size as in the WSSE default encryption properties
				}

				// encrypt symKey with the given (probably public) key
				var keyInfo = this.getKeyInfo(key);
				var proto = new Crypt.xml.EncryptedKey(this.defaultEncryptedKeyProperties);
				proto.keyInfo = keyInfo;
				proto.id = id;
				// add the reference list for the targets
				var refList = [];
				for (var i = 0; i < targets.length; i++) {
					var o = {uri: "#encrypted-" + targets[i]};	// should be an unique name
					refList.push(o);
				}
				proto.referenceList = new Crypt.xml.encryption.ReferenceList({dataRefs: refList});
				var xencKey = new Crypt.XML(proto, keyInfo.rawKey, this.keyring, this.certificates);
				xencKey.encrypt(symKey);

				// using this symmetric key, encrypt all the targets and replace the target with the encrypted one
				var xenc = new Crypt.XML(new Crypt.xml.Encryption(this.defaultEncryptionProperties), new Crypt.KeyInfo({sym: symKey}), this.keyring, this.certificates);
				for (var i = 0; i < targets.length; i++) {
					var targetElement = this.getElement(targets[i], rootIS);
					var targetChunk = new Crypt.bin.chunk.String(xs.infoset.print(Crypt.xml.makeItRoot(targetElement, Crypt.xml.CANON_ASIS)));
					var cipherReferenceProp = {
						id: refList[i].uri.slice(1),
						encryptionMethod: xenc.proto.encryptionMethod,
						cipherData: {value: xenc.encrypt(targetChunk)},
					};
					var enc = new Crypt.xml.Encryption(cipherReferenceProp);

					// replace the target with this <EncryptedData> element
					var is = xs.infoset.serialize(enc);
					this.replaceElement(targets[i], rootIS, is.element);
				}

				// prepend the <EncryptedKey> element to the Security Header
				var encIS = xs.infoset.serialize(xencKey.proto);
				var securityHeader = this.findSecurityHeader(rootIS);
				securityHeader.children.unshift(encIS.element);
			</function>

			<function name="createPassword" params="username, password, digestFlag">
				if (!this.proto)
					this.proto = new Crypt.wsse.Security();
				if (digestFlag) {
					// generate a nonce
					var nonce = Crypt.defaultRNG.get(16);
					var created = (new Crypt.wsu.Date()).toString();
					var sha1 = new Crypt.SHA1();
					sha1.update(nonce);
					sha1.update(new Crypt.bin.chunk.String(created));
					sha1.udpate(new Crypt.bin.chunk.String(password));
					var digest = sha1.close();
					var nonceObj = new Crypt.wsse.Nonce({encodingType: this.base64encodingURI, value: nonce.toString()});
					var usernameToken = Crypt.wsse.createUsernameToken(username, digest, nonceObj, created);
				}
				else
					var usernameToken = Crypt.wsse.createUsernameToken(username, password);
				this.proto.elements.push(usernameToken);
				return(usernameToken);
			</function>

			<function name="checkPassword" params="username, password">
				var headers = this.proto.elements;
				for (var i = 0; i < headers.length; i++) {
					if (headers[i] instanceof Crypt.wsse.UsernameToken) {
						var usernameToken = headers[i];
						if (username != usernameToken.username.value)
							continue;
						if (usernameToken.password.type == "wsse:PasswordDigest") {
							if (usernameToken.nonce.encodingType != this.base64encodingURI)
								continue;
							var sha1 = new Crypt.SHA1();
							sha1.update(new Chunk(usernameToken.nonce.value));
							sha1.update(new Crypt.bin.chunk.String(usernameToken.craeted));
							sha1.update(new Crypt.bin.chunk.String(password));
							if (sha1.close().toString() == usernameToken.password.value)
								return(true);
						}
						else {
							if (password == usernameToken.password.value)
								return(true);
						}
					}
				}
				return(false);
			</function>

			<function name="makePKIPath" params="proto, descendant, rootDN">
				var token = new Crypt.wsse.BinarySecurityToken(proto);
				token.encodingType = this.base64encodingURI;
				token.valueType = this.x509pathURI;

				var certs = this.certificates.makeCertChain(descendant, rootDN);
				certs.push(0x30);	// indicate SEQUENCE {}
				certs.reverse();
				token.value = Crypt.ber.encode(certs, true, true);
				return(token);
			</function>
		</object>
		<function name="WSSE" params="proto, keyring, certificates" prototype="Crypt.wsse">
			this.proto = proto;
			this.keyring = keyring ? keyring: new Crypt.Keyring();
			this.certificates = certificates ? certificates: new Crypt.Certificate();
		</function>
	</patch>

	<import href="wsseGrammar.xs"/>

	<patch prototype="Crypt.xml.keyInfo">
		<object name="securityTokenReference" pattern="wsse:SecurityTokenReference" prototype="Crypt.wsse.securityTokenReference"/>
	</patch>

	<program>
		Crypt.wsse.defaultSignatureReferenceProperties = {
			uri: "",
			transforms: [new Crypt.xml.SignatureReferenceTransform({algorithm: "http://www.w3.org/2001/10/xml-exc-c14n#"})],
			digest: new Crypt.xml.signatureReference.Digest({algorithm: "http://www.w3.org/2000/09/xmldsig#sha1"}),
		};

		Crypt.wsse.defaultSignatureProperties = {
			signedInfo: new Crypt.xml.signature.SignedInfo({
				canonicalizationAlgorithm: "http://www.w3.org/2001/10/xml-exc-c14n#",
				signatureAlgorithm: "http://www.w3.org/2000/09/xmldsig-more#rsa-sha256",
			}),
		};

		Crypt.wsse.defaultEncryptedKeyProperties = {
			encryptionMethod: new Crypt.xml.encryption.EncryptionMethod({
				encryptionAlgorithm: "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p",
				digestAlgorithm: "http://www.w3.org/2000/09/xmldsig#sha1",
			}),
		};

		Crypt.wsse.defaultEncryptionProperties = {
			encryptionMethod: new Crypt.xml.encryption.EncryptionMethod({
				encryptionAlgorithm: "http://www.w3.org/2001/04/xmlenc#aes128-cbc"
			}),
		};
	</program>
</package>