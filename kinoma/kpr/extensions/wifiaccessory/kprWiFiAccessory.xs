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
	<object name="WiFiAccessory">
		<object name="browser" c="KPR_WiFiAccessory_browser_destructor">
			<function name="get accessories" c="KPR_WiFiAccessory_browser_get_accessories" enum="false"/>
			<function name="start" params="" c="KPR_WiFiAccessory_browser_start"/>
			<function name="stop" params="" c="KPR_WiFiAccessory_browser_stop"/>

			<function name="onWiFiAccessoryBrowserDidFindAccessory" params="browser">
				application.delegate("onWiFiAccessoryBrowserDidFindAccessory", browser);
			</function>

			<function name="onWiFiAccessoryBrowserDidLoseAccessory" params="browser">
				application.delegate("onWiFiAccessoryBrowserDidLoseAccessory", browser);
			</function>

			<function name="onWiFiAccessoryBrowserDidFinishConfiguringAccessory" params="browser">
				application.delegate("onWiFiAccessoryBrowserDidFinishConfiguringAccessory", browser);
			</function>

		</object>
		<function name="Browser" params="behavior" prototype="WiFiAccessory.browser" c="KPR_WiFiAccessory_Browser"/>
		<object name="accessory" c="KPR_WiFiAccessory_accessory_destructor">
			<function name="get name" c="KPR_WiFiAccessory_accessory_get_name" enum="false"/>
			<function name="configure" params="complete" c="KPR_WiFiAccessory_accessory_configure"/>
		</object>
	</object>
</package>
