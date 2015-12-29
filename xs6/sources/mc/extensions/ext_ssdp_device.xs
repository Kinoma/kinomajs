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
	<object name="ssdpDevice" prototype="ssdpMessage">
		<number name="MAX_AGE" value="1800"/>
		<!-- should be override -->
		<string name="DEVICE_TYPE" value="MediaServer"/>
		<number name="DEVICE_VERSION" value="1"/>
		<string name="DEVICE_SCHEMA" value="urn:schemas-upnp-org"/>
		<number name="HTTP_PORT" value="4321"/>

		<!-- SSDP handlers -->
		<function name="advMessages" params="subtype">
			var msgs = [
				{subtype: subtype, NT: "upnp:rootdevice"},
				{subtype: subtype, NT: undefined},
				{subtype: subtype, NT: this.DEVICE_SCHEMA + ":device:" + this.DEVICE_TYPE + ":" + this.DEVICE_VERSION},
			];
			return msgs.concat(msgs);	// twice??
		</function>
		<function name="onStart" params="sock">
			// trace("# SSDP: onStart\n");
			// advertising ssdp:alive
			var msgs = this.advMessages("ssdp:alive");
			var that = this;
			var f = function() {
				that.timer = that.multicast(sock, that.advMessages("ssdp:alive"), that.MAX_AGE * 1000, f);
			};
			this.multicast(sock, msgs, 0, f);	// do not keep the multicast timer here
		</function>
		<function name="onStop" params="sock">
			// trace("# SSDP: onStop\n");
			if (this.timer) {
				clearInterval(this.timer);
				this.timer = undefined;
			}
			// advertising ssdp:byebye
			var msgs = this.advMessages("ssdp:byebye");
			var that = this;
			this.multicast(sock, msgs, 0, function() {sock.close();});
		</function>
		<function name="onAlive" params="o, sock">
			// trace("# SSDP: onAlive\n");
		</function>
		<function name="onByeBye" params="o, sock">
			// trace("# SSDP: onByeBye\n");
		</function>
		<function name="onDiscover" params="o, sock">
			// trace("# SSDP: onDiscover: ST = " + ("ST" in o ? o.ST : "nil") + "\n");
			if (!("MX" in o) || !("ST" in o))
				return;
			var delay = (o.MX > 5 ? 5 : o.MX) * 1000;
			var msgs;
			var t = o.ST;
			if (t == "ssdp:all")
				msgs = [
					{subtype: undefined, ST: "upnp:rootdevice"},
					{subtype: undefined, ST: undefined},
					{subtype: undefined, ST: this.DEVICE_SCHEMA + ":device:" + this.DEVICE_TYPE + ":" + this.DEVICE_VERSION},
				];
			else if (t == "ssdp:rootdevice" || t == "uuid:" + this.uuid || this.matchSearchTarget(t))
				msgs = [{subtype: undefined, ST: t}];
			if (msgs)
				this.unicast(sock, msgs, delay);
		</function>
		<function name="onResponse" params="o, sock">
			// trace("# SSDP: onResponse\n");
		</function>
	</object>
	<function name="SSDPDevice" params="" prototype="ssdpDevice">
		this.timer = undefined;
		SSDPMessage.call(this);
	</function>
</package>
