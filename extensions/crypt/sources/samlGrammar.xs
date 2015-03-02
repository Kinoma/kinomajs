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
	<namespace prefix="ds" uri="http://www.w3.org/2000/09/xmldsig#"/>
	<namespace prefix="xenc" uri="http://www.w3.org/2001/04/xmlenc#"/>
	<namespace prefix="wsdl" uri="http://schemas.xmlsoap.org/wsdl/"/>
	<namespace prefix="saml" uri="urn:oasis:names:tc:SAML:2.0:assertion"/>
	<namespace prefix="saml1" uri="urn:oasis:names:tc:SAML:1.0:assertion"/>
	<namespace prefix="xsi" uri="http://www.w3.org/2001/XMLSchema-instance"/>

	<patch prototype="Crypt.saml">
		<object name="idNameQualifier">
			<string name="nameQualifier" pattern="@saml:NameQualifier"/>
			<string name="spNameQualifier" pattern="@saml:SPNameQualifier"/>
		</object>

		<object name="nameID" pattern="saml:NameID" prototype="Crypt.saml.idNameQualifier">
			<string name="format" pattern="@saml:Format"/>
			<string name="spProviderID" pattern="@saml:SPProviderID"/>
		</object>

		<object name="encryptedElementType">
			<object name="encryptedData" pattern="xenc:EncryptedData" prototype="Crypt.xml.encryption"/>
			<object name="encryptedKey" pattern="xenc:EncryptedKey" prototype="Crypt.xml.encryptedKey"/>
		</object>

		<object name="attributeValue" pattern="saml:AttributeValue">
			<!-- anyType -->
		</object>

		<object name="attribute" pattern="saml:Attribute">
			<array name="attributeValues" pattern="." contents="Crypt.saml.attributeValue"/>
			<string name="name" pattern="saml:Name"/>
			<string name="nameFormat" pattern="saml:NameFormat"/>	<!-- anyURI -->
			<string name="friendlyName" pattern="saml:FriendlyName"/>
		</object>

		<object name="attributeStatement" pattern="saml:AttributeStatement">
			<object name="attribute" pattern="saml:Attribute" prototype="Crypt.saml.attribute"/>
			<object name="encryptedAttribute" pattern="saml:EncryptedAttribute" prototype="Crypt.saml.encryptedElementType"/>
		</object>

		<object name="subjectConfirmationData" pattern="saml:SubjectConfirmationData">
			<string name="notBefore" pattern="@saml:NotBefore"/>
			<string name="notOnOrAfter" pattern="@saml:NotOnOrAfter"/>
			<string name="recipient" pattern="@saml:Recipient"/>
			<string name="inResponseTo" pattern="@saml:InResponseTo"/>
			<string name="address" pattern="@saml:Address"/>
		</object>

		<object name="subjectConfirmation" pattern="saml:SubjectConfirmation">
			<object name="subjectConfirmationData" pattern="saml:SubjectConfirmationData" prototype="Crypt.saml.subjectConfirmationData"/>
			<string name="method" pattern="saml:Method"/>
		</object>

		<object name="subject" pattern="saml:Subject">
			<object name="baseID" pattern="saml:BaseID" prototype="Crypt.saml.idNameQualifier"/>
			<object name="nameID" pattern="saml:NameID" prototype="Crypt.saml.nameID"/>
			<object name="encryptedID" pattern="saml:EncryptedID" prototype="Crypt.saml.encryptedElementType"/>
			<array name="subjectConfirmations" pattern="." contents="Crypt.saml.subjectConfirmation"/>
		</object>

		<object name="condition">
			<!-- abstract type -->
		</object>

		<string name="audience" pattern="saml:Audience"/>	<!-- anyURI -->
				
		<object name="audienceRestriction" pattern="saml:AudienceRestriction">
			<array name="audiences" pattern="." contents="Crypt.saml.audience"/>
		</object>

		<object name="oneTimeUse" pattern="saml:OneTimeUse" prototype="Crypt.saml.condition"/>

		<object name="proxyRestriction" pattern="saml:ProxyRestriction" prototype="Crypt.saml.condition">
			<array name="audiences" pattern="." contents="Crypt.saml.audience"/>
			<number name="count" pattern="@saml:Count"/>
		</object>

		<object name="conditions" pattern="saml:Conditions">
			<object name="condition" pattern="saml:Condition" prototype="Crypt.saml.condition"/>
			<object name="audienceRestriction" pattern="saml:AudienceRestriction" prototype="Crypt.saml.audienceRestriction"/>
			<object name="oneTimeUse" pattern="saml:OneTimeUse"/>
			<object name="proxyRestriction" pattern="saml:ProxyRestriction"/>
			<string name="notBefore" pattern="@saml:NotBefore"/>
			<string name="notOnOrAfter" pattern="@saml:NotOnOrAfter"/>
		</object>

		<object name="encryptedAssertion" pattern="saml:EncryptedAssertion" prototype="Crypt.saml.encryptedElementType"/>

		<object name="advice">
			<string name="assertionIDRef" pattern="saml:AssertionIDRef"/>	<!-- NCName -->
			<string name="assertionURIRef" pattern="saml:AssertionURIRef"/>	<!-- anyURI -->
<!--			<object name="assertion" pattern="saml:Assertion" prototype="Crypt.saml.assertion"/> -->
			<object name="encryptedAssertion" pattern="saml:EncryptedAssertion" prototype="Crypt.saml.encryptedAssertion"/>
		</object>

		<object name="statement" pattern="saml:Statement">
			<!-- abstract type -->
		</object>

		<object name="action" pattern="saml:Action">
			<string name="namespace" pattern="@saml:Namespace"/>
		</object>

		<object name="evidence" pattern="saml:Evidence">
			<string name="assertionIDRef" pattern="saml:AssertionIDRef"/>
			<string name="assertionURIRef" pattern="saml:AssertionURIRef"/>
<!--			<object name="assertion" prototype="Crypt.saml.assertion"/> -->
			<object name="encryptedAssertion" prototype="Crypt.saml.encryptedAssertion"/>
		</object>

		<object name="subjectLocality" pattern="saml:SubjectLocality">
			<string name="address" pattern="@saml:Address"/>
			<string name="dnsName" pattern="@saml:DNSName"/>
		</object>

		<string name="authnContextClassRef" pattern="saml:AuthnContextClassRef"/>	<!-- anyURI -->
		<string name="authnContextDecl" pattern="saml:AuthnContextDecl"/>	<!-- anyURI -->
		<string name="authnContextDeclRef" pattern="saml:AuthnContextDeclRef"/>		<!-- anyURI -->
		<string name="authenticatingAuthority" pattern="saml:AuthenticatingAuthority"/>		<!-- anyURI -->

		<object name="authzDecisionStatement" pattern="saml:AuthzDecisionStatement" prototype="Crypt.saml.statement">
			<object name="action" prototype="Crypt.saml.action"/>
			<object name="evidence" prototype="Crypt.saml.evidence"/>
			<string name="resource" pattern="@saml:Resource"/>		<!-- anyURI -->
			<string name="decision" pattern="@saml:Decision"/>		<!-- (Permit, Decy, Indeterminate) -->
		</object>

		<object name="authnContext" pattern="saml:AuthnContext">
			<string name="authnContextClassRef"/>
			<string name="authnContextDecl"/>
			<string name="authnContextDeclRef"/>
			<array name="authenticatingAuthorities" pattern="." contents="Crypt.saml.authenticatingAuthority"/>
		</object>

		<object name="authnStatement" pattern="saml:AuthnStatement" prototype="Crypt.saml.statement">
			<object name="subjectLocality" prototype="Crypt.saml.subjectLocality"/>
			<object name="authnContext" prototype="Crypt.saml.authnContext"/>
			<string name="authnInstant" pattern="@saml:AuthnInstant"/>		<!-- dateTime -->
			<string name="sessionIndex" pattern="@saml:SessionIndex"/>
			<string name="sessionNotOnOrAfter" pattern="@saml:SessionNotOnOrAfter"/>		<!-- dateTime -->
		</object>

		<object name="assertionStatements">
			<object name="statement" pattern="saml:Statement" prototype="Crypt.saml.statement"/>
			<object name="authnStatement" pattern="saml:AuthnStatement" prototype="Crypt.saml.authnStatement"/>
			<object name="authzDecisionStatement" pattern="saml:AuthzDecisionStatement" prototype="Crypt.saml.authzDecisionStatement"/>
			<object name="attributeStatement" pattern="saml:AttributeStatement" prototype="Crypt.saml.attributeStatement"/>
		</object>

		<object name="assertion2" pattern="/saml:Assertion">
			<object name="issuer" pattern="saml:Issuer" prototype="Crypt.saml.nameID"/>
			<object name="signature" pattern="ds:Signature" prototype="Crypt.xml.signature"/>
			<object name="subject" pattern="saml:Subject" prototype="Crypt.saml.subject"/>
			<object name="conditions" pattern="." prototype="Crypt.saml.conditions"/>
			<object name="advice" pattern="saml:Advice" prototype="Crypt.saml.advice"/>
			<array name="statements" pattern="." contents="Crypt.saml.assertionStatements"/>
			<string name="version" pattern="@saml:Version"/>
			<string name="ID" pattern="@saml:ID"/>
			<string name="issueInstance" pattern="@saml:IssueInstant"/>	<!-- dateTime -->
		</object>

		<object name="assertionAttribute" pattern="saml1:Attribute">
			<string name="name" pattern="@AttributeName"/>
			<string name="namespace" pattern="@AttributeNamespace"/>
			<string name="value" pattern="saml1:AttributeValue"/>
			<string name="xsiType" pattern="saml1:AttributeValue/@xsi:type"/>
		</object>

		<!-- verion 1.0 only? -->
		<object name="assertion" pattern="/saml1:Assertion">
			<string name="assertionID" pattern="@AssertionID"/>
			<string name="issuerInstant" pattern="@IssueInstant"/>
			<string name="issuerAttr" pattern="@Issuer"/>
			<string name="majorVersion" pattern="@MajorVersion"/>
			<string name="minorVersion" pattern="@MinorVersion"/>
			<object name="attributeStatement" pattern="saml1:AttributeStatement">
				<object name="subject" pattern="saml1:Subject">
					<object name="nameIdentifier" pattern="saml1:NameIdentifier">
						<string name="format" pattern="@Format"/>
					</object>
				</object>
				<array name="attributes" pattern="." contents="Crypt.saml.assertionAttribute"/>
			</object>
			<object name="signature" pattern="ds:Signature" prototype="Crypt.xml.signature"/>
		</object>
	</patch>

	<patch prototype="Crypt.saml.advice">
		<!-- for forward reference -->
		<object name="assertion" pattern="saml:Assertion" prototype="Crypt.saml.assertion"/>
	</patch>

	<patch prototype="Crypt.saml.evidence">
		<!-- for forward reference -->
		<object name="assertion" prototype="Crypt.saml.assertion"/>
	</patch>

</package>