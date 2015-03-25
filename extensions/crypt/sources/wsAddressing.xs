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
	<namespace prefix="S" uri="http://www.w3.org/2003/05/soap-envelope"/>
	<namespace prefix="S11" uri="http://schemas.xmlsoap.org/soap/envelope"/>
	<namespace prefix="wsa" uri="http://www.w3.org/2005/08/addressing"/>
	<namespace prefix="wsp" uri="http://schemas.xmlsoap.org/ws/2002/12/policy"/>
	<namespace prefix="xs" uri="http://www.w3.org/2001/XMLSchema"/>

	<patch prototype="Crypt">
		<object name="wsa" pattern=".">
			<object name="proto"/>

			<object name="any" prototype="Crypt.wsa.proto" script="false"/>
			<object name="endpointReference" pattern="/wsa:EndpointReference" prototype="Crypt.wsa.proto">
				<string name="address" pattern="wsa:Address"/>
				<object name="referenceProperties" pattern="wsa:ReferenceProperties">
					<array name="any" contents="Crypt.wsa.any" pattern="."/>
				</object>
				<object name="referenceParameters" pattern="wsa:ReferenceParameters">
					<array name="any" contents="Crypt.wsa.any" pattern="."/>
				</object>
				<string name="portType" pattern="wsa:PortType"/>
				<object name="serviceName" pattern="wsa:ServiceName">
					<string name="portName" pattern="@PortName"/>
					<string name="value" pattern="."/>
				</object>
				<object name="policy" pattern="wsa:Policy" prototype="Crypt.wsa.any"/>
			</object>

			<object name="messageID" pattern="wsa:MessageID" prototype="Crypt.wsa.proto">
				<string name="value" pattern="."/>	<!-- anyURI -->
			</object>
			<object name="relatesTo" pattern="/wsa:RelatesTo" prototype="Crypt.wsa.proto">
				<string name="relationshipType" pattern="@RelationshipType"/>
				<string name="value" pattern="."/>
			</object>
			<object name="replyTo" pattern="/wsa:ReplyTo" prototype="Crypt.wsa.endpointReference"/>
			<object name="from" pattern="/wsa:From" prototype="Crypt.wsa.endpointReference"/>
			<object name="faultTo" pattern="/wsa:FaultTo" prototype="Crypt.wsa.endpointReference"/>
			<object name="to" pattern="wsa:To" prototype="Crypt.wsa.proto">
				<string name="value" pattern="."/>	<!-- anyURI -->
			</object>
			<object name="action" pattern="wsa:Action" prototype="Crypt.wsa.proto">
				<string name="value" pattern="."/>	<!-- anyURI -->
			</object>
		</object>
	</patch>
</package>