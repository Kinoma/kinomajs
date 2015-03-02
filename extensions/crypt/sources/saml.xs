<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package>
	<patch prototype="Crypt">
		<object name="saml"/>
	</patch>

	<import href="samlGrammar.xs"/>

	<patch prototype="Crypt.saml.assertion">
		<function name="verify" params="keyring, certificates">
			// only support the case the signature element resides in the assertion element (i.e. this) -- must be an eveloped type
			var ds = new Crypt.XML(new Crypt.xml.Signature(this.signature), undefined, keyring, certificates);
			if (!ds.verify.addRefID(this.__xs__infoset, this.assertionID))
				return(false);
			return(ds.verify.done(this.signature.__xs__infoset));
		</function>

		<function name="sign" params="keyInfo, keyring, certificates">
			// sign this
			var defaultSignatureTemplate = {
				signedInfo: new Crypt.xml.signature.SignedInfo({
					canonicalizationAlgorithm: "http://www.w3.org/2001/10/xml-exc-c14n#",
					signatureAlgorithm: "http://www.w3.org/2000/09/xmldsig#rsa-sha1",
					references: [],
				}),
				keyInfo: keyInfo,
			};
			var defaultReferenceTemplate = {
				uri: '#' + this.assertionID,
				transforms: [new Crypt.xml.SignatureReferenceTransform({algorithm: "http://www.w3.org/2001/10/xml-exc-c14n#"})],
				digest: new Crypt.xml.signatureReference.Digest({algorithm: "http://www.w3.org/2000/09/xmldsig#sha1"}),
			};
			var o = new Crypt.xml.Signature(defaultSignatureTemplate);
			var ds = new Crypt.XML(o, undefined, keyring, certificates);	// the key should be solved in Crypt.XML with the given keyInfo, keyring and certificates
			var ref = new Crypt.xml.SignatureReference(defaultReferenceTemplate);
			ds.sign.addRef(this.__xs__infoset, ref);
			var str = ds.sign.done();
			this.signature = xs.infoset.parse(xs.infoset.scan(str));
		</function>
	</patch>
</package>