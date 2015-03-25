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
	<object name="UPnP">
		<function name="dummy">
			var onUPnPAdded, onUPnPRemoved;
		</function>
		<object name="Device">
			<function name="get friendlyName" c="UPnP_Device_getFriendlyName"/>
			<function name="get running" c="UPnP_Device_getRunning"/>
			<function name="get uuid" c="UPnP_Device_getUUID"/>
			<function name="getVariable" params="service,variable" c="UPnP_Device_getVariable"/>
			<function name="changed" c="UPnP_Device_changed"/>
			<function name="lastChangeVariables" params="service,namespace,names" c="UPnP_Device_lastChangeVariables"/>
			<function name="setError" params="service,error,description" c="UPnP_Device_setError"/>
			<function name="setVariable" params="service,variable,value" c="UPnP_Device_setVariable"/>
			<function name="start" params="type,name,configId" c="UPnP_Device_start"/>
			<function name="stop" params="" c="UPnP_Device_stop"/>
		</object>
		<object name="Controller">
			<function name="isBackgroundPlaying" c="UPnP_Controller_isBackgroundPlaying"/>
			<function name="discover" params="type,services" c="UPnP_Controller_discover"/>
			<function name="forget" params="type,services" c="UPnP_Controller_forget"/>
			<function name="subscribe" params="uuid,service,timeout" c="UPnP_Controller_subscribe"/>
			<function name="unsubscribe" params="uuid,service" c="UPnP_Controller_unsubscribe"/>
			<function name="getVariable" params="uuid,service,variable" c="UPnP_Controller_getVariable"/>
			<function name="getVariableMinimum" params="uuid,service,variable" c="UPnP_Controller_getVariableMinimum"/>
			<function name="getVariableMaximum" params="uuid,service,variable" c="UPnP_Controller_getVariableMaximum"/>
			<function name="setVariable" params="uuid,service,variable,value" c="UPnP_Controller_setVariable"/>
			<function name="invokeAction" params="uuid,service,action" prototype="KPR.message" c="UPnP_Controller_invokeAction"/>
			<function name="isAction" params="uuid,service,action" prototype="KPR.message" c="UPnP_Controller_isAction"/>
			<function name="utility" params="uuid,action" c="UPnP_Controller_utility"/>
		</object>
	</object>
	<object name="DLNA">
		<string name="MediaRenderer" value="urn:schemas-upnp-org:device:MediaRenderer:1"/>
		<string name="AVTransport" value="urn:schemas-upnp-org:service:AVTransport:1"/>
		<string name="ConnectionManager" value="urn:schemas-upnp-org:service:ConnectionManager:1"/>
		<string name="RenderingControl" value="urn:schemas-upnp-org:service:RenderingControl:1"/>
	</object>
	<patch prototype="KPR">
		<function name="get mediaMimes" c="KPR_get_mediaMimes"/>
	</patch>
</package>