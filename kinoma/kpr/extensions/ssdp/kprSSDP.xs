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
<package script="true">
	<import href="kpr.xs" link="dynamic"/>
	<object name="SSDP">
		<!-- server -->
		<object name="server" c="SSDP_server">
			<function name="addService" c="SSDP_addService"/>
			<function name="get behavior" c="SSDP_get_behavior"/>
			<function name="get path" c="SSDP_server_get_path"/>
			<function name="get port" c="SSDP_server_get_port"/>
			<function name="get type" c="SSDP_get_type"/>
			<function name="get uuid" c="SSDP_server_get_uuid"/>
			<function name="set behavior" params="it" c="SSDP_set_behavior"/>
			<function name="start" c="SSDP_server_start"/>
			<function name="stop" c="SSDP_server_stop"/>
			<!-- ids for callbacks to the behavior -->
			<null name="onSSDPServerRegistered"/> <!-- params="server" -->
			<null name="onSSDPServerUnregistered"/> <!-- params="server" -->
		</object>
		<function name="Server" params="type, port, path, expire" prototype="SSDP.server" c="SSDP_Server"/>
		<!-- client -->
		<object name="client" c="SSDP_client">
			<function name="addService" c="SSDP_addService"/>
			<function name="get behavior" c="SSDP_get_behavior"/>
			<function name="get type" c="SSDP_get_type"/>
			<function name="set behavior" params="it" c="SSDP_set_behavior"/>
			<function name="start" c="SSDP_client_start"/>
			<function name="stop" c="SSDP_client_stop"/>
			<!-- ids for callbacks to the behavior -->
			<null name="onSSDPServerUp"/> <!-- params="server" -->
			<null name="onSSDPServerDown"/> <!-- params="server" -->
		</object>
		<function name="Client" params="type" prototype="SSDP.client" c="SSDP_Client"/>
	</object>
</package>