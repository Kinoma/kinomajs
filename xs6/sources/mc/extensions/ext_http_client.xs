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
	<import href="ext_http.xs" link="dynamic"/>

	<program>
		import {Socket} from "socket";
		var http = require.weak("http");
	</program>
	<object name="httpClient" prototype="http">
		<null name="adrs"/>
		<boolean name="ssl"/>
		<null name="stream"/>

		<function name="start" params="s">
			var idx, adr, port;
			if ((idx = this.adrs.indexOf(":")) >= 0) {
				adr = this.adrs.substring(0, idx);
				port = parseInt(this.adrs.substring(idx + 1));
			}
			else {
				adr = this.adrs;
				port = this.ssl ? 443 : 80;
			}
			this.stream = s;
			if (this.ssl) {
				var SecureSocket = require.weak("SecureSocket");
				this.sock = new SecureSocket({host: adr, port: port, proto: "tcp", options: {extensions: {server_name: adr, max_fragment_length: 1024}}});
			}
			else
				this.sock = new Socket({host: adr, port: port, proto: "tcp"});
			this.sock.onConnect = this.onConnect;
			this.sock.onMessage = this.onMessage;
			this.sock.onError = this.onError;
			this.sock.onClose = this.onClose;
			this.sock.http = this;
			this.addHeader("Host", adr);
		</function>

		<function name="setRequestMethod" params="method">
			this.method = method;
		</function>

		<!-- default action -->
		<function name="onDataReady" params="buf">
			if (!this.content)
				this.content = buf;
			else
				this.content = this.content.concat(buf);
		</function>

		<!-- callbacks from the socket -->
		<function name="onConnect">
			var http = this.http;
			this.send(http.method + " " + http.url + " HTTP/" + http.version + "\r\n");
			http.sendHeaders();
			if (http.stream) {
				this.send(http.stream);
				http.onDataSent(http.stream.length);
			}
			http.initMessage();
			http.content = null;
		</function>
		<function name="onMessage" params="n">
			this.http.processMessage(n);
		</function>
		<function name="onError">
			this.close();
			try {
				this.http.onTransferComplete(false);
			}
			finally {
			}
		</function>
		<function name="onClose">
			this.close();
			try {
				this.http.onTransferComplete(true);
			}
			finally {
			}
		</function>
	</object>
	<function name="HTTPClient" params="uri" prototype="httpClient">
		var idx, idx2;
		if (uri.indexOf("http://") == 0) {
			this.ssl = false;
			idx = 7;
		}
		else if (uri.indexOf("https://") == 0) {
			this.ssl = true;
			idx = 8;
		}
		else if (uri.indexOf("ws://") == 0) {
			this.ssl = false;
			idx = 5;
		}
		else if (uri.indexOf("wss://") == 0) {
			this.ssl = true;
			idx = 6;
		}
		else
			throw new this.Error(-1);
		if ((idx2 = uri.indexOf("/", idx)) >= 0 || (idx2 = uri.indexOf("?", idx)) >= 0) {
			this.adrs = uri.substring(idx, idx2);
			this.url = uri.substring(idx2);
		}
		else {
			this.adrs = uri.substring(idx);
			this.url = "/";
		}
		this.initMessage();
	</function>
</package>
