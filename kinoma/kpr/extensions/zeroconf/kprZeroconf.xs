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
	<import href="kpr.xs" link="dynamic"/>
	<object name="Zeroconf">
		<!-- advertisement -->
		<object name="advertisement" c="Zeroconf_advertisement">
			<function name="get behavior" c="Zeroconf_get_behavior"/>
			<function name="get port" c="Zeroconf_advertisement_get_port"/>
			<function name="get serviceName" c="Zeroconf_advertisement_get_serviceName"/>
			<function name="get serviceType" c="Zeroconf_get_serviceType"/>
			<function name="set behavior" params="it" c="Zeroconf_set_behavior"/>
			<function name="start" c="Zeroconf_advertisement_start"/>
			<function name="stop" c="Zeroconf_advertisement_stop"/>
			<!-- ids for callbacks to the behavior -->
			<null name="onZeroconfServiceRegistered"/> <!-- params="service" -->
			<null name="onZeroconfServiceUnregistered"/> <!-- params="service" -->
		</object>
		<function name="Advertisement" params="serviceType, serviceName, port, txt" prototype="Zeroconf.advertisement" c="Zeroconf_Advertisement"/>
		<!-- browser -->
		<object name="browser" c="Zeroconf_browser">
			<function name="get behavior" c="Zeroconf_get_behavior"/>
			<function name="get serviceType" c="Zeroconf_get_serviceType"/>
			<function name="set behavior" params="it" c="Zeroconf_set_behavior"/>
			<function name="start" c="Zeroconf_browser_start"/>
			<function name="stop" c="Zeroconf_browser_stop"/>
			<!-- ids for callbacks to the behavior -->
			<null name="onZeroconfServiceDown"/> <!-- params="service" -->
			<null name="onZeroconfServiceUp"/> <!-- params="service" -->
		</object>
		<function name="Browser" params="serviceType" prototype="Zeroconf.browser" c="Zeroconf_Browser"/>
	</object>
</package>