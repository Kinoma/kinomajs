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
 		
    <object name="wifimanager" c="KPR_wifimanager_destructor">
		<function name="getNetId" c="KPR_wifimanager_getNetId"  script="true"/>
		<function name="getIpAddress" c="KPR_wifimanager_getIpAddress"  script="true"/>
		<function name="getSsid" c="KPR_wifimanager_getSsid"  script="true"/>
        <function name="updateNetwork" params="ssid, key, security" c="KPR_wifimanager_updateNetwork"  script="true"/>
        <function name="selectNetworkById" params="ssid, key, security" c="KPR_wifimanager_selectNetworkById"  script="true"/>
		<function name="getScanResults" c="KPR_wifimanager_getScanResults"  script="true"/>
    </object>
	<function name="WifiManager" prototype="wifimanager" c="KPR_WifiManager_URL"/>
	
</package>

