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
	<namespace prefix="ds" uri="http://www.w3.org/2000/09/xmldsig#"/>
	<namespace prefix="xenc" uri="http://www.w3.org/2001/04/xmlenc#"/>
	<namespace prefix="ec" uri="http://www.w3.org/2001/10/xml-exc-c14n#"/>

	<patch prototype="Crypt.xml">
		<function name="setProperties" params="properties" script="false">
			if (properties) {
				for (var p in properties)
					this[p] = properties[p];
				for (var p in properties.sandbox)
					this[p] = properties.sandbox[p];
			}
		</function>

		<!-- grammar for the XML sig/enc standard -->
		<object name="x509DataElement">
			<chunk name="cert" pattern="ds:X509Certificate"/>
			<object name="issuerSerial" pattern="ds:X509IssuerSerial">
				<string name="name" pattern="ds:X509IssuerName"/>
				<string name="serial" pattern="ds:X509SerialNumber"/>
			</object>
			<function name="IssuerSerial" params="name, serial" prototype="Crypt.xml.x509DataElement.issuerSerial">
				this.name = name;
				this.serial = serial;
			</function>
			<string name="subjectName" pattern="ds:X509SubjectName"/>
			<chunk name="ski" pattern="ds:X509SKI"/>
			<chunk name="crl" pattern="ds:X509CRL"/>
		</object>

		<object name="x509Data">
			<array name="certs" pattern="." contents="Crypt.xml.x509DataElement.cert"/>
			<array name="issuerSerials" pattern="." contents="Crypt.xml.x509DataElement.issuerSerial"/>
			<array name="subjectNames" pattern="." contents="Crypt.xml.x509DataElement.subjectName"/>
			<array name="skis" pattern="." contents="Crypt.xml.x509DataElement.ski"/>
			<array name="crls" pattern="." contents="Crypt.xml.x509DataElement.crl"/>
		</object>
		<object name="x509DataObject" pattern="ds:X509Data" prototype="Crypt.xml.x509Data"/>
		<function name="X509DataObject" params="properties" prototype="Crypt.xml.x509DataObject">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="signatureReferenceTransform" pattern="ds:Transform">
			<string name="algorithm" pattern="@Algorithm"/>
			<string name="prefixList" pattern="ec:InclusiveNamespaces/@PrefixList"/>
			<object name="xpath" pattern="ds:XPath">
				<string name="attr" pattern="@."/>
				<string name="data" pattern="."/>
			</object>
		</object>
		<function name="SignatureReferenceTransform" params="properties" prototype="Crypt.xml.signatureReferenceTransform">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="keyInfo">
			<string name="keyName" pattern="ds:KeyName"/>
			<object name="rsaKey" pattern="ds:KeyValue/ds:RSAKeyValue">
				<chunk name="modulus" pattern="ds:Modulus"/>
				<chunk name="exponent" pattern="ds:Exponent"/>
			</object>
			<object name="dsaKey" pattern="ds:KeyValue/ds:DSAKeyValue">
				<chunk name="p" pattern="ds:P"/>
				<chunk name="q" pattern="ds:Q"/>
				<chunk name="g" pattern="ds:G"/>
				<chunk name="y" pattern="ds:Y"/>
				<chunk name="j" pattern="ds:J"/>
				<chunk name="seed" pattern="ds:Seed"/>
				<chunk name="pgencounter" pattern="ds:PgenCounter"/>
			</object>
			<object name="dhKey" pattern="ds:KeyValue/xenc:DHKeyValue">
				<chunk name="p" pattern="xenc:P"/>
				<chunk name="q" pattern="xenc:Q"/>
				<chunk name="generator" pattern="xenc:Generator"/>
				<chunk name="pub" pattern="xenc:Public"/>
				<chunk name="seed" pattern="xenc:seed"/>
				<chunk name="pgenCounter" pattern="xenc:pgenCounter"/>
			</object>
			<array name="x509" pattern="." contents="Crypt.xml.x509DataObject"/>
			<object name="retrievalMethod" pattern="ds:RetrievalMethod">
				<string name="uri" pattern="@URI"/>
				<string name="type" pattern="@Type"/>
				<array name="transforms" pattern="ds:Transforms/." contents="Crypt.xml.signatureReferenceTransform"/>
			</object>
			<object name="agreementMethod" pattern="xenc:AgreementMethod">
				<string name="algorithm" pattern="@Algorithm"/>
				<chunk name="nonce" pattern="xenc:KA-Nonce"/>
				<string name="digestAlgorithm" pattern="ds:DigestMethod/@Algorithm"/>
				<object name="originator" pattern="xenc:OriginatorKeyInfo" prototype="Crypt.xml.keyInfo"/>
				<object name="recipient" pattern="xenc:RecipientKeyInfo" prototype="Crypt.xml.keyInfo"/>
			</object>
		</object>
		<function name="KeyInfo" params="properties" prototype="Crypt.xml.keyInfo">
			Crypt.xml.setProperties.call(this, properties);
		</function>
		<function name="KeyInfoCerts" params="certChain" prototype="Crypt.xml.keyInfo">
			this.x509 = [new Crypt.xml.X509DataObject({certs: certChain})];
		</function>
		<function name="KeyInfoSKI" params="subjectName, ski" prototype="Crypt.xml.keyInfo">
			if (subjectName)
				this.x509 = [new Crypt.xml.X509DataObject({subjectNames: [subjectName]})];
			if (ski)
				this.x509 = [new Crypt.xml.X509DataObject({skis: [ski]})];
		</function>

		<object name="signatureReference" pattern="ds:Reference">
			<string name="uri" pattern="@URI"/>
			<string name="type" pattern="@Type"/>
			<string name="id" pattern="@Id"/>
			<array name="transforms" pattern="ds:Transforms/." contents="Crypt.xml.signatureReferenceTransform"/>
			<object name="digest" pattern=".">
				<string name="algorithm" pattern="ds:DigestMethod/@Algorithm"/>
				<chunk name="value" pattern="ds:DigestValue"/>
			</object>
			<function name="Digest" params="properties" prototype="Crypt.xml.signatureReference.digest">
				Crypt.xml.setProperties.call(this, properties);
			</function>
		</object>
		<function name="SignatureReference" params="properties" prototype="Crypt.xml.signatureReference">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="object" pattern="ds:Object">
			<string name="id" pattern="@Id"/>
			<string name="mime" pattern="@MimeType"/>
			<string name="encoding" pattern="@Encoding"/>
			<string name="data" pattern="."/>
			<object name="manifest" pattern="ds:Manifest">
				<string name="id" pattern="@Id"/>
				<array name="references" pattern="." contents="Crypt.xml.signatureReference"/>
			</object>
		</object>
		<function name="Object" params="properties" prototype="Crypt.xml.object">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="signature">
			<string name="id" pattern="@Id"/>
			<object name="signedInfo" pattern="/ds:SignedInfo">
				<string name="canonicalizationAlgorithm" pattern="ds:CanonicalizationMethod/@Algorithm"/>
				<string name="signatureAlgorithm" pattern="ds:SignatureMethod/@Algorithm"/>
				<number name="hmacOutputLength" pattern="ds:SignatureMethod/ds:HMACOutputLength"/>
				<array name="references" pattern="." contents="Crypt.xml.signatureReference"/>
			</object>
			<function name="SignedInfo" params="properties" prototype="Crypt.xml.signature.signedInfo">
				Crypt.xml.setProperties.call(this, properties);
			</function>
			<object name="keyInfo" pattern="ds:KeyInfo" prototype="Crypt.xml.keyInfo"/>
			<chunk name="value" pattern="ds:SignatureValue"/>
			<array name="objects" pattern="." contents="Crypt.xml.object"/>
		</object>
		<object name="signatureGrammar" pattern="/ds:Signature" prototype="Crypt.xml.signature"/>
		<function name="Signature" params="properties" prototype="Crypt.xml.signatureGrammar">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="dataReference" pattern="xenc:DataReference">
			<string name="uri" pattern="@URI"/>
			<!-- does not support child elements -->
		</object>
		<function name="DataReference" params="properties" prototype="Crypt.xml.dataReference">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="keyReference" pattern="xenc:KeyReference">
			<string name="uri" pattern="@URI"/>
			<!-- does not support child elements -->
		</object>
		<function name="KeyReference" params="properties" prototype="Crypt.xml.keyReference">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="encryption" pattern="/xenc:EncryptedData">
			<string name="type" pattern="@Type"/>
			<string name="id" pattern="@Id"/>
			<string name="mimeType" pattern="@MimeType"/>

			<object name="encryptionMethod" pattern="xenc:EncryptionMethod">
				<string name="encryptionAlgorithm" pattern="@Algorithm"/>
				<string name="digestAlgorithm" pattern="ds:DigestMethod/@Algorithm"/>
				<chunk name="oaepParams" pattern="xenc:OAEPparams"/>
				<string name="keySize" pattern="xenc:KeySize"/>
			</object>
			<function name="EncryptionMethod" params="properties" prototype="Crypt.xml.encryption.encryptionMethod">
				Crypt.xml.setProperties.call(this, properties);
			</function>

			<object name="keyInfo" pattern="ds:KeyInfo" prototype="Crypt.xml.keyInfo"/>
			<patch prototype="Crypt.xml.keyInfo">
				<object name="encryptedKey" pattern="xenc:EncryptedKey" prototype="Crypt.xml.encryption">
					<!-- this is a trick to stop the infinite references... -->
					<null name="encryptedKey"/>
				</object>
			</patch>

			<object name="cipherData" pattern="xenc:CipherData">
				<chunk name="value" pattern="xenc:CipherValue"/>
				<object name="reference" pattern="xenc:CipherReference">
					<string name="uri" pattern="@URI"/>
					<array name="transforms" pattern="xenc:Transforms/." contents="Crypt.xml.signatureReferenceTransform"/>
				</object>
			</object>

			<object name="referenceList" pattern="xenc:ReferenceList">
				<array name="dataRefs" contents="Crypt.xml.dataReference" pattern="."/>
				<array name="keyRefs" contents="Crypt.xml.keyReference" pattern="."/>
			</object>

			<function name="ReferenceList" params="properties" prototype="Crypt.xml.encryption.referenceList">
				if ("dataRefs" in properties) {
					this.dataRefs = new Array();
					for (var i = 0; i < properties.dataRefs.length; i++)
						this.dataRefs[i] = new Crypt.xml.DataReference(properties.dataRefs[i]);
				}
				if ("keyRefs" in properties) {
					this.keyRefs = new Array();
					for (var i = 0; i < properties.keyRefs.length; i++)
						this.keyRefs[i] = new Crypt.xml.KeyReference(properties.keyRefs[i]);
				}
			</function>

			<object name="encryptionProperty" pattern="xenc:EncryptionProperty">
				<string name="target" pattern="@Target"/>
				<string name="id" pattern="@Id"/>
				<!-- any content -->
			</object>
			<object name="encryptionProperties" pattern="xenc:EncryptionProperties">
				<string name="id" pattern="@Id"/>
				<array name="encryptionProperty" contents="Crypt.xml.encryption.encryptionProperty" pattern="."/>
			</object>

			<string name="carriedKeyName" pattern="xenc:CarriedKeyName"/>
		</object>
		<function name="Encryption" params="properties" prototype="Crypt.xml.encryption">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="encryptedKey" pattern="/xenc:EncryptedKey" prototype="Crypt.xml.encryption"/>
		<function name="EncryptedKey" params="properties" prototype="Crypt.xml.encryptedKey">
			Crypt.xml.Encryption.call(this, properties);
		</function>
	</patch>
</package>