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
	<import href="ext_ssdp.xs" link="dynamic"/>

	<program>
		var SSDPMessage = require.weak("SSDPMessage");
		var ssdpMessage = SSDPMessage.prototype;
	</program>
	<object name="ssdpControl" prototype="ssdpMessage">
		<number name="MX" value="1"/>
		<undefined name="http_location"/>
		<function name="onFound"/>
		<function name="onLost"/>

		<function name="onStart" params="sock">
			// nothing to do for a control point
		</function>
		<function name="onStop" params="sock">
			// nothing to do for a control piont
		</function>
		<function name="onAlive" params="o, sock">
			if ("ST" in o && this.matchSearchTarget(o.ST)) {
				this.http_location = o.LOCATION;
				this.onFound(this.http_location);
			}
		</function>
		<function name="onByeBye" params="o, sock">
			if ("ST" in o && this.matchSearchTarget(o.ST)) {
				this.http_location = undefined;
				this.onLost();
			}
		</function>
		<function name="onDiscover" params="o, sock">
			// shouldn't be called
		</function>
		<function name="onResponse" params="o, sock">
			this.http_location = o.LOCATION;
			this.onFound(this.http_location);
		</function>
		<function name="search" params="st">
			var msg = this.SEARCH_REQ + "\r\n" +
			"HOST: " + this.MCAST_ADDR + ":" + this.MCAST_PORT + "\r\n" +
			'MAN: "ssdp:discover"\r\n' +
			"MX: " + this.MX + "\r\n" +
			"ST: " + (st ? st : this.DEVICE_SCHEMA + ":device:" + this.DEVICE_TYPE + ":" + this.DEVICE_VERSION) + "\r\n" +
			"USER_AGENT: " + System.osVersion + " UPnP/1.1 Kinoma/1.0\r\n" +
			"\r\n";
			this.http_location = undefined;
			this.sendMessage(msg);
			this.retry = 3;
			var that = this;
			var timer = setInterval(function() {
				if (that.http_location || --that.retry < 0)
					clearInterval(timer);	// end the timer
				else
					that.sendMessage(msg);
			}, this.MX * 1000);
		</function>
	</object>
	<function name="SSDPControl" params="" prototype="ssdpControl">
		SSDPMessage.call(this);
	</function>
</package>
