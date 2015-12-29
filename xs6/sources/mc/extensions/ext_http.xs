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
	<object name="http">
		<string name="version" value="1.1"/>
		<number name="maxMessageLength" value="16384"/>	<!-- 16k is the maximum fragment length of SSL -->
		<null name="incomingHeaders"/>
		<null name="outgoingHeaders"/>
		<number name="process" value="0"/>
		<string name="message" value=""/>
		<undefined name="statusCode"/>
		<string name="method" value="GET"/>
		<null name="url"/>
		<null name="content"/>
		<undefined name="sock"/>
		<function name="onHeaders"/>
		<function name="onDataReady"/>
		<function name="onDataSent"/>
		<function name="onTransferComplete"/>

		<object name="error" prototype="Error.prototype">
			<number name="code" value="0"/>
		</object>
		<function name="Error" params="code" prototype="http.error">
			this.code = code;
		</function>

		<function name="decomposeUrl" params="url">
			var parts = url.split("/"), args;
			parts = parts.filter(Boolean);
			var idx = parts[parts.length - 1].indexOf("?");
			if (idx >= 0) {
				var last = parts[parts.length - 1];
				parts[parts.length - 1] = last.substring(0, idx);
				args = {};
				last.substring(idx + 1).split("&").forEach(function(elm, i, arr) {
					if ((i = elm.indexOf("=")) > 0)
						args[elm.substring(0, i)] = elm.substring(i + 1);
				});
			}
			return {
				path: parts,
				query: args,
			};
		</function>

		<function name="addHeader" params="header, value">
			if (!this.outgoingHeaders)
				this.outgoingHeaders = {};
			this.outgoingHeaders[header] = value;
		</function>
		<function name="getHeader" params="name">
			return name in this.incomingHeaders ? this.incomingHeaders[name] : undefined;
		</function>
		<function name="sendHeaders">
			var h = this.outgoingHeaders;
			for (var name in h) {
				this.sock.send(name + ": " + h[name] + "\r\n");
			}
			this.sock.send("\r\n");
		</function>
		<function name="putChunk" params="s">
			var len;
			if (typeof s == "string")
				len = s.length;
			else
				len = s.byteLength;
			this.sock.send(len.toString(16) + "\r\n");
			this.sock.send(s);
			this.sock.send("\r\n");
		</function>
		<function name="terminateChunk" params="footer">
			this.sock.send("0\r\n");
			if (footer) {
				for (var name in footer)
					this.sock.send(name + ": " + footer[name] + "\r\n");
			}
			this.sock.send("\r\n");
		</function>
		<function name="initMessage">
			this.process = 0;
			this.message = "";
			this.outgoingHeaders = null;
		</function>
		<function name="processMessage" params="n">
			var buf = this.sock.recv(n > this.maxMessageLength ? this.maxMessageLength : n);
			if (!buf)
				return;
			if (this.process < 2) {
				var msg = this.message + String.fromArrayBuffer(buf);
				var start = 0;
				for (var end ; (end = msg.indexOf("\n", start)) >= 0; start = end + 1) {
					var line = msg.substring(start, end > start && msg.charAt(end - 1) == '\r' ? end - 1 : end);
					if (this.process == 0) {	// in the status line or the request
						var parts = line.split(" ");
						if (parts[0].indexOf("HTTP/") == 0)
							this.statusCode = parseInt(parts[1]);
						else {
							this.method = parts[0];
							this.url = parts[1];
							this.version = (parts[2].split("/"))[1];
						}
						this.incomingHeaders = {};
						this.process++;
					}
					else if (this.process == 1) {	// in the response headers
						var idx = line.indexOf(":");
						if (idx >= 0)
							this.incomingHeaders[line.substring(0, idx).trim()] = line.substring(idx + 1).trim();
					}
					if (line === "") {	// end of the headers -- go on to the response data
						this.onHeaders();
						this.process++;
						start = end + 1;
						break;
					}
				}
				if (this.process == 2) {
					start -= this.message.length;
					if (buf.byteLength > start)
						this.onDataReady(buf.slice(start));
				}
				else
					this.message = msg.substring(start);
			}
			else if (this.process == 2) {	// in the response data
				this.onDataReady(buf);
			}
		</function>
	</object>
</package>
