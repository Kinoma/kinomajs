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
<package script="true">
	<object name="Crypt" script="true"/>

	<!-- utilities -->
	<import href="common.xs"/>
	<import href="bin.xs"/>

	<!-- crypto primitive functions and math library -->
	<import href="primitives.xs"/>
	<import href="arith.xs"/>
	<import href="arith_ec.xs"/>
	<import href="pk.xs"/>
	<import href="hmac.xs"/>
	<import href="x917.xs"/>

	<!-- crypto standard format -->
	<import href="ber.xs"/>
	<import href="pem.xs"/>
	<import href="persistentList.xs"/>
	<import href="keyinfo.xs"/>
	<import href="keyring.xs"/>
	<import href="cert.xs"/>
	<import href="x509.xs"/>

	<!-- Web security standards -->
	<import href="wsAddressing.xs"/>
	<import href="wsu.xs"/>
	<import href="xml.xs"/>
	<import href="saml.xs"/>
	<import href="wsse.xs"/>
	<import href="wst.xs"/>
	<import href="wsdl.xs"/>

	<import href="contentEnc.xs"/>
</package>