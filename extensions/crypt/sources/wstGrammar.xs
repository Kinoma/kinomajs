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
	<namespace prefix="wst" uri="http://schemas.xmlsoap.org/ws/2005/02/trust"/>
	<namespace prefix="wsu" uri="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"/>

	<patch prototype="Crypt.wstrust">
		<object name="requestSecurityTokenContent"/>
		<object name="requestSecurityToken" pattern="/wst:RequestSecurityToken">
			<array name="contents" pattern="." contents="Crypt.wstrust.requestSecurityTokenContent"/>
			<string name="context" pattern="@Context"/>
		</object>

		<object name="requestSecurityTokenResponseContent"/>
		<object name="requestSecurityTokenResponse" pattern="/wst:RequestSecurityTokenResponse">
			<array name="contents" pattern="." contents="Crypt.wstrust.requestSecurityTokenResponseContent"/>
			<string name="context" pattern="@Context"/>
		</object>

		<object name="binarySecret" pattern="wst:BinarySecret">
			<string name="type" pattern="@Type"/>
			<!-- standard type values -->
			<string name="asymmetricKeyType" value="http://schemas.xmlsoap.org/ws/2005/02/trust/AsymmetricKey"/>
			<string name="symmetricKeyType" value="http://schemas.xmlsoap.org/ws/2005/02/trust/SymmetricKey"/>
			<string name="nonceType" value="http://schemas.xmlsoap.org/ws/2005/02/trust/Nonce"/>
		</object>

		<!-- Issueance Binding -->
		<object name="claims" pattern="wst:Claims">
			<string name="dialect" pattern="@Dialect"/>
		</object>

		<object name="entropy" pattern="wst:Entropy">
			<!-- either xenc:EncryptedKey or wst:BinarySecret -->
		</object>

		<object name="lifetime" pattern="wst:Lifetime">
			<string name="created" pattern="wsu:Created"/>
			<string name="expires" pattern="wsu:Expires"/>
		</object>

		<string name="computedKey" pattern="wst:ComputedKey"/>
		<!-- standard values for "cmputedKey" -->
		<string name="computedKeyPSHA1" value="http://schemas.xmlsoap.org/ws/2005/02/trust/CK/PSHA1"/>
		<string name="computedKeyHASH" value="http://schemas.xmlsoap.org/ws/2005/02/trust/CK/HASH"/>

		<object name="requestedAttachedReference" pattern="wst:RequestedAttachedReference">
			<reference name="securityTokenReference" pattern="." contents="Crypt.wsse.securityTokenReferenceGrammar"/>
		</object>

		<object name="requestedUnattachedReference" pattern="wst:RequestedUnattachedReference">
			<reference name="securityTokenReference" pattern="." contents="Crypt.wsse.securityTokenReferenceGrammar"/>
		</object>

		<object name="requestedProofToken" pattern="wst:RequestedProofToken">
		</object>

		<!-- Returning Multiple Security Tokens -->

		<object name="requestSecurityTokenResponseCollection" pattern="/wst:RequestSecurityTokenResponseCollection">
			<array name="collections" pattern="." contents="Crypt.wstrust.requestSecurityTokenResponse"/>
		</object>

		<!-- Returning Security Token in Headers -->

		<reference name="issuedTokens" pattern="." contents="Crypt.wstrust.requestSecurityTokenResponseCollection"/>

		<!-- Renewal Binding -->

		<object name="renewTarget" pattern="wst:RenewTarget">
		</object>

		<object name="renewing" pattern="wst:Renewing">
			<boolean name="allow" pattern="@Allow"/>
			<boolean name="ok" pattern="@OK"/>
		</object>

		<!-- Cancel Binding -->

		<object name="cancelTarget" pattern="wst:CancelTarget">
		</object>

		<object name="requestedTokenCancelled" pattern="wst:RequestedTokenCancelledType"/>

		<!-- Validation Binding -->

		<object name="status" pattern="wst:Status">
			<string name="code" pattern="wst:Code"/>
			<string name="reason" pattern="wst:Reason"/>

			<!-- standard values for 'wst:Code' -->
			<string name="valid" value="http://schemas.xmlsoap.org/ws/2005/02/trust/status/valid"/>
			<string name="invalid" value="http://schemas.xmlsoap.org/ws/2005/02/trust/status/invalid"/>
		</object>

		<!-- Signature Challenges -->

		<object name="signChallenge" pattern="wst:SignChallenge">
		</object>

		<!-- Binary Exchanges and Negotiations -->
		<object name="binaryExchange" pattern="wst:BinaryExchange">
			<string name="valueType" pattern="@ValueType"/>
			<string name="encodingType" pattern="@EncodingType"/>
		</object>

		<!-- Key Exchange Token -->
		<object name="requestKET" pattern="wst:RequestKET">
		</object>

		<object name="keyExchangeToken" pattern="wst:KeyExchangeToken">
		</object>

		<!-- Authenticating Exchanges -->
		<object name="authenticator" pattern="wst:Authenticator">
			<chunk name="combinedHash" pattern="wst:CombinedHash"/>
		</object>

		<!-- On-Behalf-Of Parameters -->

		<object name="onBehalfOf" pattern="wst:OnBehalfOf">
		</object>

		<object name="issuer" pattern="wst:Issuer" prototype="Crypt.wsa.endpointReference"/>

		<!-- Key and Encryption Requirements -->

		<string name="authenticationType" pattern="wst:AuthenticationType"/>

		<string name="keyType" pattern="wst:KeyType"/>
		<!-- standard values for wst:KeyType -->
		<string name="keyTypePublicKey" value="http://schemas.xmlsoap.org/ws/2005/02/trust/PublicKey"/>
		<string name="keyTypeSymmetricKey" value="http://schemas.xmlsoap.org/ws/2005/02/trust/SymmetricKey"/>

		<number name="keySize" pattern="wst:KeySize"/>

		<string name="signatureAlgorithm" pattern="wst:SignatureAlgorithm"/>
		<string name="encryptionAlgorithm" pattern="wst:EncryptionAlgorithm"/>
		<string name="canonicalizationAlgorithm" pattern="wst:CanonicalizationAlgorithm"/>
		<string name="computedKeyAlgorithm" pattern="wst:ComputedKeyAlgorithm"/>

		<object name="encryption" pattern="/wst:Encryption">
		</object>

		<object name="proofEncryption" pattern="/wst:ProofEncryption">
		</object>

		<object name="useKey" pattern="/wst:UseKey">
			<string name="sig" pattern="wst:Sig"/>
		</object>

		<string name="signWith" pattern="wst:SignWith"/>
		<string name="encryptWith" pattern="wst:EncryptWith"/>

		<!-- Delegation and Forwarding Requirements -->
		<object name="delegateTo" pattern="/wst:DelegateTo">
		</object>

		<boolean name="forwardable" pattern="wst:Forwardable"/>
		<boolean name="delegatable" pattern="wst:Delegatable"/>

		<!-- Authorized Token Participants -->
		<object name="participant"/>
		<object name="participantElement" pattern="wst:Participant" prototype="Crypt.wstrust.participant"/>
		<object name="participants" pattern="/wst:Participants">
			<object name="primary" pattern="wst:Primary" prototype="Crypt.wstrust.participant"/>
			<array name="participant" pattern="." contents="Crypt.wstrust.participantElement"/>
		</object>
	</patch>
</package>