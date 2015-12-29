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
	<program>
		import {Socket, ListeningSocket} from "socket";
	</program>
	<object name="ssdpMessage">
		<string name="MCAST_ADDR" value="239.255.255.250"/>
		<number name="MCAST_PORT" value="1900"/>
		<number name="TTL" value="2"/>
		<string name="NOTIFY_REQ" value="NOTIFY * HTTP/1.1"/>
		<string name="SEARCH_REQ" value="M-SEARCH * HTTP/1.1"/>
		<string name="RESPONSE" value="HTTP/1.1 200 OK"/>

		<!-- should be override -->
		<number name="MAX_AGE" value="1800"/>	<!-- the default value is from UPnP -->
		<number name="HTTP_PORT" value="4321"/>
		<string name="LOCATION" value="/"/>

		<!-- handlers -->
		<function name="onAlive"/>
		<function name="onByeBye"/>
		<function name="onDiscover"/>
		<function name="onStart"/>
		<function name="onStop"/>

		<function name="matchSearchTarget" params="st">
			var i;
			return (i = st.lastIndexOf(":")) > 0 && st.slice(0, i) == this.DEVICE_SCHEMA + ":device:" + this.DEVICE_TYPE && parseInt(st.slice(i+1)) <= this.DEVICE_VERSION;
		</function>
		<function name="formatMessage" params="types, sock">
			return (types.subtype ? this.NOTIFY_REQ : this.RESPONSE) + "\r\n" +
				"SERVER: MC200/2.3.35, UPnP/1.0, Kinoma/1.0\r\n" +
				"HOST: " + this.MCAST_ADDR + ":" + this.MCAST_PORT + "\r\n" +
				(types.subtype ?
				 "USN: " + "uuid:" + this.uuid + (types.NT ? "::" + types.NT : "") + "\r\n" +
				 "NT: " + (types.NT ? types.NT : "uuid:" + this.uuid) + "\r\n" +
				 "NTS: " + types.subtype + "\r\n" :
				 "USN: " + "uuid:" + this.uuid + (types.ST ? "::" + types.ST : "") + "\r\n" +
				 "ST: " + (types.ST ? types.ST : "uuid:" + this.uuid) + "\r\n") +
				(!types.subtype || types.subtype == "ssdp:alive" ?
				 "CACHE-CONTROL: max-age=" + this.MAX_AGE + "\r\n" +
				 "LOCATION: " + "http://" + sock.addr + ":" + this.HTTP_PORT + this.LOCATION :
				 "") + "\r\n" +
				"\r\n";
		</function>
		<function name="multicast" params="response, msgs, delay, f">
			var interval = function() {return Math.random() * 100};
			if (delay == 0) delay = 1;	// immediately
			var that = this;
			var timer = setInterval(function() {
				var msg = this.msgs.shift();
				if (msg) {
					response.send(that.formatMessage(msg, response));
					/*
					var s = that.formatMessage(msg, response);
					trace("# SSDP: multicast:\n");
					trace(s);
					response.send(s);
					*/
				}
				if (msgs.length > 0) {
					var itvl = interval();
					this.reschedule(itvl);
				}
				else {
					clearInterval(timer);
					if (f) f();
				}
			}, delay);
			timer.msgs = msgs;
			return timer;
		</function>
		<function name="unicast" params="listener, msgs, delay">
			if (delay == 0) delay = 1;
			var response = listener.response;
			var interval = Math.random() * delay / msgs.length;
			var that = this;
			try {
				var timer = setInterval(function() {
					var msg = this.msgs.shift();
					if (msg) {
						response.send(that.formatMessage(msg, response), listener.peer);
						/*
						var s = that.formatMessage(msg, response);
						trace("# SSDP: unicast:\n");
						trace(s);
						response.send(s, listener.peer);
						*/
					}
					if (this.msgs.length <= 0)
						clearInterval(timer);
				}, interval);
				timer.msgs = msgs;
			}
			catch(error){
				trace("### error: no time slot\n");
			}
			// timer should be retained until it's closed
		</function>
		<function name="sendMessage" params="msg">
			this.socks.forEach(function(s) {
				s.response.send(msg);
			});
		</function>
		<function name="parseMessage" params="msg">
			var o = {startLine: undefined};
			msg.split("\n").forEach(function(elm) {
				if (o.startLine == undefined)
					o.startLine = elm.trim();
				else {
					var i = elm.indexOf(":");
					if (i > 0)
						o[elm.slice(0, i).trim()] = elm.slice(i + 1).trim();
				}
			});
			return o;
		</function>
		<function name="handleMessage" params="msg, sock">
			var o = this.parseMessage(msg);
			switch (o.startLine) {
			case this.NOTIFY_REQ:
				switch (o.NTS) {
				case "ssdp:alive":
					this.onAlive(o, sock);
					break;
				case "ssdp:byebye":
					this.onByeBye(o, sock);
					break;
				}
				break;
			case this.SEARCH_REQ:
				this.onDiscover(o, sock);
				break;
			case this.RESPONSE:
				this.onResponse(o, sock);
				break;
			}
		</function>
		<function name="bind" params="s, advertising">
			var that = this;
			s.onConnect = function() {
				if (advertising)
					that.onStart(this);
			};
			s.onMessage = function(n) {
				var msg = this.read(String, n);
				if (!msg)
					return;
				that.handleMessage(msg, this);
			};
			s.onError = function() {
				this.close();
			};
			s.onClose = function() {
				if (advertising)
					that.onStop(s);
				else
					this.close();
			};
		</function>
		<function name="start">
			var Connection = require("wifi");
			this.socks = [];
			var interfaces = Connection.getInterfaces();
			for (var i in interfaces) {
				var nif = interfaces[i];
				if (nif.UP && nif.MULTICAST && nif.addr && nif.addr != "127.0.0.1") {
					var s = new ListeningSocket({addr: this.MCAST_ADDR, port: this.MCAST_PORT, proto: "udp", membership: nif.addr, ttl: this.TTL});
					this.bind(s, false);
					var response = new Socket({host: this.MCAST_ADDR, port: this.MCAST_PORT, proto: "udp", multicast: nif.addr, ttl: this.TTL});
					this.bind(response, true);
					s.response = response;	// bind the response socket to the listening socket
					response.timer = undefined;
					this.socks.push(s);
				}
			}
		</function>
		<function name="stop">
			for (var i = 0; i < this.socks.length; i++) {
				var s = this.socks[i];
				this.onStop(s.response);	// will be closed when the byebye broadcast is finished
				s.close();		// the listening socket can be closed here
			}
			// this.socks[] should be retained until the byebye broadcast is finished
		</function>
	</object>
	<function name="SSDPMessage" params="" prototype="ssdpMessage">
		var uuid = require.weak("uuid");
		this.uuid = uuid.getUUID();
		this.socks = [];
	</function>
</package>
