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
	<namespace prefix="S11" uri="http://schemas.xmlsoap.org/soap/envelope"/>
	<namespace prefix="S12" uri="http://www.w3.org/2003/05/soap-envelope"/>
	<namespace prefix="wsa" uri="http://schemas.xmlsoap.org/ws/2004/08/addressing"/>
	<namespace prefix="wsp" uri="http://schemas.xmlsoap.org/ws/2002/12/policy"/>
	<namespace prefix="xs" uri="http://www.w3.org/2001/XMLSchema"/>
	<namespace prefix="ds" uri="http://www.w3.org/2000/09/xmldsig#"/>
	<namespace prefix="xenc" uri="http://www.w3.org/2001/04/xmlenc#"/>
	<namespace prefix="m" uri="http://schemas.xmlsoap.org/rp"/>
	<namespace prefix="wsse" uri="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"/>
	<namespace prefix="wsu" uri="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"/>

	<patch prototype="Crypt.wsse">
		<object name="securityHeader">
			<function name="getID">
				return(this.hasOwnProperty("content") ? this.content.id: this.id);
			</function>
		</object>

		<!-- Nonces -->
		<object name="nonce" pattern="wsse:Nonce" prototype="Crypt.wsse.securityHeader">
			<string name="id" pattern="@wsu:Id"/>
			<string name="encodingType" pattern="@EncodingType"/>
			<string name="value" pattern="."/>
		</object>
		<function name="Nonce" params="properties" prototype="Crypt.wsse.nonce">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<!-- Security Timestamps -->
		<object name="timeStamp" pattern="wsu:Timestamp" prototype="Crypt.wsse.securityHeader">
			<string name="id" pattern="@wsu:Id"/>
			<string name="created" pattern="wsu:Created"/>
		</object>
		<function name="TimeStamp" params="properties" prototype="Crypt.wsse.timeStamp">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<!-- Security Tokens -->
		<object name="usernameToken" pattern="wsse:UsernameToken" prototype="Crypt.wsse.securityHeader">
			<string name="id" pattern="@wsu:Id"/>
			<object name="username" pattern="wsse:Username">
				<!-- extensible attributes -->
				<string name="value" pattern="."/>
			</object>
			<object name="password" pattern="wsse:Password">
				<!-- extensible attributes -->
				<string name="type" pattern="@Type"/>
				<string name="value" pattern="."/>
			</object>
			<reference name="nonce" pattern="." contents="Crypt.wsse.nonce"/>
			<string name="created" pattern="wsu:Created"/>
		</object>
		<function name="UsernameToken" params="properties" prototype="Crypt.wsse.usernameToken">
			Crypt.xml.setProperties.call(this, properties);
		</function>
		<function name="createUsernameToken" params="username, password, nonce, created">
			var usernameObj = xs.newInstanceOf(Crypt.wsse.usernameToken.username);
			usernameObj.value = username;
			var passwordObj = xs.newInstanceOf(Crypt.wsse.usernameToken.password);
			if (password instanceof Chunk) {
				passwordObj.type = "wsse:PasswordDigest";
				passwordObj.value = password.toString();
			}
			else {
				passwordObj.type = "wsse:PasswordText";
				passwordObj.value = password;
			}
			var usernameToken = new Crypt.wsse.UsernameToken({username: usernameObj, password: passwordObj});
			if (nonce)
				usernameToken.nonce = nonce;
			if (created)
				usernameToken.created = created;
			return(usernameToken);
		</function>

		<object name="binarySecurityToken" pattern="wsse:BinarySecurityToken" prototype="Crypt.wsse.securityHeader">
			<string name="id" pattern="@wsu:Id"/>
			<string name="valueType" pattern="@ValueType"/>
			<string name="encodingType" pattern="@EncodingType"/>
			<chunk name="value" pattern="."/>
		</object>
		<function name="BinarySecurityToken" params="properties" prototype="Crypt.wsse.binarySecurityToken">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<!-- Token References -->
		<object name="securityTokenReference">
			<string name="id" pattern="@wsu:Id"/>
			<object name="reference" pattern="wsse:Reference">
				<string name="uri" pattern="@URI"/>
				<string name="valueType" pattern="@ValueType"/>
			</object>
			<object name="embedded" pattern="wsse:Embedded">
				<string name="id" pattern="@wsu:Id"/>
				<string name="valueType" pattern="@ValueType"/>
				<array name="elements" pattern="." contents="Crypt.wsse.securityHeader"/>	<!-- anything that can be a security header? -->
			</object>
			<object name="keyIdentifier" pattern="wsse:KeyIdentifier">
				<string name="valueType" pattern="@ValueType"/>
				<string name="encodingType" pattern="@EncodingType"/>
				<string name="value" pattern="."/>
			</object>
			<object name="keyInfo" pattern="ds:KeyInfo" prototype="Crypt.xml.keyInfo"/>
			<string name="keyName" pattern="ds:KeyName"/>
		</object>
		<object name="securityTokenReferenceGrammar" pattern="wsse:SecurityTokenReference" prototype="Crypt.wsse.securityHeader">
			<object name="content" pattern="." prototype="Crypt.wsse.securityTokenReference"/>
		</object>
		<function name="SecurityTokenReference" params="properties" prototype="Crypt.wsse.securityTokenReferenceGrammar">
			this.content = xs.newInstanceOf(Crypt.wsse.securityTokenReference);
			Crypt.xml.setProperties.call(this.content, properties);
		</function>

		<!-- Signature -->
		<object name="signature" pattern="ds:Signature" prototype="Crypt.wsse.securityHeader">
			<object name="content" pattern="." prototype="Crypt.xml.signature"/>
		</object>
		<function name="Signature" params="properties" prototype="Crypt.wsse.signature">
			this.content = new Crypt.xml.Signature(properties);
		</function>

		<!-- Encryption -->
		<object name="referenceList" pattern="xenc:ReferenceList" prototype="Crypt.wsse.securityHeader">
			<object name="content" pattern="." prototype="Crypt.xml.encryption.referenceList"/>
		</object>
		<function name="ReferenceList" params="properties" prototype="Crypt.wsse.referenceList">
			this.content = new Crypt.xml.encryption.ReferenceList(properties);
		</function>

		<object name="encryptedKey" pattern="xenc:EncryptedKey" prototype="Crypt.wsse.securityHeader">
			<object name="content" pattern="." prototype="Crypt.xml.encryptedKey"/>
		</object>
		<function name="EncryptedKey" params="properties" prototype="Crypt.wsse.encryptedKey">
			this.content = new Crypt.xml.EncryptedKey(properties);
		</function>

		<object name="encryptedHeader" pattern="wsse:EncryptedData" prototype="Crypt.wsse.securityHeader">
			<object name="encryptedData" prototype="Crypt.xml.encryption"/>
		</object>
		<function name="EncryptedHeader" params="properties" prototype="Crypt.wsse.encryptedHeader">
			Crypt.xml.setProperties.call(this, properties);
		</function>

		<object name="encryptedData" pattern="xenc:EncryptedData" prototype="Crypt.wsse.securityHeader">
			<object name="content" pattern="." prototype="Crypt.xml.encryption"/>
		</object>
		<function name="EncryptedData" params="properties" prototype="Crypt.wsse.encryptedData">
			this.content = new Crypt.xml.Encryption(properties);
		</function>

		<!-- Security Header -->
		<object name="security" pattern="/wsse:Security">
			<array name="elements" pattern="." contents="Crypt.wsse.securityHeader"/>
		</object>
		<function name="Security" params="" prototype="Crypt.wsse.security">
			this.elements = new Array();
		</function>
	</patch>
</package>