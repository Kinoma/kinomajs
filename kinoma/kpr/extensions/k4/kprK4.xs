<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
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
    <object name="K4">
        <function name="set timezone" params="timezone" c="KPR_K4_set_timezone"/>
        <function name="set date" params="secsSinceEpoch" c="KPR_K4_set_date"/>
        <function name="reboot" c="KPR_K4_reboot"/>
        <function name="shutdown" c="KPR_K4_shutdown"/>
        <function name="hibernate" c="KPR_K4_hibernate"/>
		
		<function name="onSD" c="KPR_K4_onSD"/>
		<function name="SDInserted" c="KPR_K4_SDInserted"/>
		<function name="isCreateSD" c="KPR_K4_isCreateSD"/>
		<function name="isSDCardBootable" c="KPR_K4_isSDBootable"/>
        
        <function name="is8887" c=" KPR_K4_is8887"/>
        <function name="isYocto" c=" KPR_K4_isYocto"/>
		
		<function name="checkFileExists" c="KPR_K4_checkFileExists"/>
		
		<function name="muxAllDigital" c="KPR_K4_muxAllGPIO"/>
		<function name="muxUART" c="KPR_K4_muxUART"/>
		<function name="muxUARTFlip" c="KPR_K4_muxUARTFlip"/>
		<function name="muxSPI" c="KPR_K4_muxSPI"/>
		

        <function name="get deviceID" c="KPR_K4_get_deviceID"/>
        <function name="get MAC" c="KPR_K4_get_MAC"/>

        <function name="captureScreen" c="KPR_K4_captureScreen"/>

        <function name="log" params="logName, logValue">
            var directory = this.logPath;
            var path = directory + logName + ".log";
            var d = new Date();

			// Pad date/time components to two characters so timestamps line up in log
			var year = d.getFullYear();
			var month = 1 + d.getMonth();
			var date = d.getDate();
			var dateStr = "";
			if (month < 10) dateStr += "0";
			dateStr += month + "/";
			if (date < 10) dateStr += "0";
			dateStr += date + "/";
			dateStr += year.toString().substring(2);
		
			var hour = d.getHours();
			var minute = d.getMinutes();
			var second = d.getSeconds();
			var timeStr = "";
			if (hour < 10) timeStr += "0";
			timeStr += hour + ":";
			if (minute < 10) timeStr += "0";
			timeStr += minute + ":";
			if (second < 10) timeStr += "0";
			timeStr += second;

            var line = dateStr + ' ' + timeStr + ' ' + logValue + '\n';
            try {
                Files.appendText(path, line);
            }
            catch (e) {
            }
        </function>
        <function name="get logPath">
            return this._logPath;
        </function>
        <string name="_logPath" script="false"/>
    </object>
    <program>
            var directory;
            if ("linux" == system.platform)
            directory = Files.documentsDirectory;
            else
                directory = Files.preferencesDirectory;
            directory += "logs/";
            Files.ensureDirectory(directory);
        K4._logPath = directory;
    </program>
</package>
