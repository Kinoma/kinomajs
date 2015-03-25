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
	<namespace prefix="wsdl" uri="http://schemas.xmlsoap.org/wsdl/"/>
	<namespace prefix="wsdlsoap" uri="http://schemas.xmlsoap.org/wsdl/soap/"/>

	<patch prototype="Crypt">
		<object name="wsdl">
			<object name="tDocumentation" pattern="wsdl:documentation"/>
			<object name="tDocumented">
				<array name="documentations" pattern="." contents="Crypt.wsdl.tDocumentation"/>
			</object>
			<object name="tImport" pattern="wsdl:import" prototype="Crypt.wsdl.tDocumented">
				<string name="namespace" pattern="@namespace"/>
				<string name="location" pattern="@location"/>
			</object>
			<object name="tTypes" pattern="wsdl:types" prototype="Crypt.wsdl.tDocumented"/>
			<object name="tPart" pattern="wsdl:part" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<string name="element" pattern="@element"/>
				<string name="type" pattern="@type"/>
			</object>
			<object name="tMessage" pattern="wsdl:message" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<array name="part" pattern="." contents="Crypt.wsdl.tPart"/>
			</object>
			<object name="tParam" pattern="." prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<string name="message" pattern="@message"/>
				<object name="soapBody" pattern="wsdlsoap:body">
					<string name="use" pattern="@use"/>
				</object>
				<object name="soapFault" pattern="wsdlsoap:fault">
					<string name="name" pattern="@name"/>
					<string name="use" pattern="@use"/>
				</object>
			</object>
			<object name="tFault" pattern="wsdl:fault" prototype="Crypt.wsdl.tParam"/>
			<object name="tOperation" pattern="wsdl:operation" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<string name="parameterOrder" pattern="@parameterOrder"/>
				<object name="input" pattern="wsdl:input" prototype="Crypt.wsdl.tParam"/>
				<object name="output" pattern="wsdl:output" prototype="Crypt.wsdl.tParam"/>
				<array name="fault" pattern="." contents="Crypt.wsdl.tFault"/>
				<object name="soapOperation" pattern="wsdlsoap:operation">
					<string name="soapAction" pattern="@soapAction"/>
				</object>
			</object>
			<object name="tPortType" pattern="wsdl:portType" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<array name="operations" pattern="." contents="Crypt.wsdl.tOperation"/>
			</object>
			<object name="tBinding" pattern="wsdl:binding" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<string name="type" pattern="@type"/>
				<array name="operations" pattern="." contents="Crypt.wsdl.tOperation"/>
				<object name="soapBinding" pattern="wsdlsoap:binding">
					<string name="style" pattern="@style"/>
					<string name="transport" pattern="@transport"/>
				</object>
			</object>
			<object name="tPort" pattern="wsdl:port" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<string name="binding" pattern="@binding"/>
				<object name="soapAddress" pattern="wsdlsoap:address">
					<string name="location" pattern="@location"/>
				</object>
			</object>
			<object name="tService" pattern="wsdl:service" prototype="Crypt.wsdl.tDocumented">
				<string name="name" pattern="@name"/>
				<array name="ports" pattern="." contents="Crypt.wsdl.tPort"/>
			</object>
			<object name="tDefinitions" prototype="Crypt.wsdl.tDocumented">
				<string name="targetNamespace" pattern="@targetNamespace"/>
				<string name="name" pattern="@name"/>
				<array name="imports" pattern="." contents="Crypt.wsdl.tImport"/>
				<array name="types" pattern="." contents="Crypt.wsdl.tTypes"/>
				<array name="messages" pattern="." contents="Crypt.wsdl.tMessage"/>
				<array name="portTypes" pattern="." contents="Crypt.wsdl.tPortType"/>
				<array name="bindings" pattern="." contents="Crypt.wsdl.tBinding"/>
				<array name="services" pattern="." contents="Crypt.wsdl.tService"/>
			</object>
			<object name="definitions" pattern="/wsdl:definitions" prototype="Crypt.wsdl.tDefinitions"/>
		</object>
	</patch>
</package>