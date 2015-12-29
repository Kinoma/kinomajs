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
		import {Socket, ListeningSocket} from "socket";
		import System from "system";
		var http = require.weak("http");
	</program>
	<object name="httpServerMessage" prototype="http">
		<function name="_response" params="status, reason, contentType, s">
			var closeConnection = (this.getHeader("Connection") == "close");
			this.statusCode = status;
			this.addHeader("Host", System.hostname);

			/* add headers for blockly web app */
			this.addHeader("Access-Control-Allow-Origin", "*");
			this.addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,DELETE,OPTIONS");
			this.addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
			
			if (closeConnection)
				this.addHeader("Connection", "close");
			if (contentType)
				this.addHeader("Content-Type", contentType);
			this.sock.send("HTTP/" + this.version + " " + this.statusCode + " " + reason + "\r\n");
			if (s !== undefined && s !== null) {
				if (typeof s == "string")
					var len = s.length;
				else	// assume s is ArrayBuffer
					var len = s.byteLength;
				this.addHeader("Content-Length", len.toString());
				this.sendHeaders();
				this.sock.send(s);
			}
			else if (s === null) {
				this.addHeader("Transfer-Encoding", "chunked");
				this.sendHeaders();
			}
			else {
				this.addHeader("Content-Length", 0);
				this.sendHeaders();
			}
			if (this.version == "1.0" || closeConnection)
				this.sock.close();
			else
				this.sock.flush();
		</function>
		<function name="response" params="contentType, s">
			this._response(200, "OK", contentType, s);
		</function>
		<function name="responseWithChunk" params="contentType">
			this._response(200, "OK", contentType, null);
		</function>
		<function name="errorResponse" params="status, reason, s">
			this._response(status, reason, undefined, s);
		</function>
		<function name="startss">
			this.sock.startss.apply(this.sock, arguments);
		</function>

		<function name="callOnRequest">
			var that = this;
			setTimeout(function() {
				that.server.onRequest(that);
				that.content = null;
				that.initMessage();
			}, 0);
		</function>

		<function name="onHeaders">
			this.content = null;
			switch (this.method) {
			case "GET":
			case "OPTIONS":
				// ignore the content
				this.callOnRequest();
				break;
			case "POST":
			case "PUT":
				// read up the content and call onRequest
				var header = this.getHeader("Content-Length");
				if (header)
					this.contentLength = parseInt(header);
				if ((!header) || this.contentLength == 0)
					this.callOnRequest();
				break;
			default:
				this.errorResponse(405, "Method not Allowed");
				break;
			}
		</function>
		<function name="onDataReady" params="buf">
			if (!this.content)
				this.content = buf;
			else
				this.content = this.content.concat(buf);
			if (this.content.byteLength >= this.contentLength) {
				this.callOnRequest();
			}
		</function>
	</object>
	<function name="HTTPServerMessage" params="sock, server" prototype="httpServerMessage">
		this.sock = sock;
		this.server = server;
		this.initMessage();
	</function>

	<object name="httpServer">
		<null name="clients"/>	<!-- content = httpServerInstance -->
		<function name="onClose"/>
		<undefined name="securityProto"/>

		<function name="close">
			for (var n = this.clients.length; --n >= 0;)
				this.clients[n].close();
			this.clients = [];
			if (this.sock)
				this.sock.close();
			this.onClose(true);
		</function>
		<function name="closeClient" params="s">
			var n = this.clients.length;
			while (--n >= 0) {
				if (this.clients[n] == s) {
					this.clients.splice(n, 1);
					break;
				}
			}
			s.close();
			s.message = undefined;
			if (this.clients.length == 0)
				this.onClose();	// nothing is running on this server
		</function>

		<function name="onConnect" params="s">
			// got a new connection
			var nsock = Object.create(Socket.prototype);
			s.accept(nsock);
			if (this.securityProto)
				nsock = new this.securityProto({sock: nsock});
			nsock.message = new HTTPServerMessage(nsock, this);
			var that = this;
			nsock.onMessage = function(n) {
				this.message.processMessage(n);
			};
			nsock.onClose = function() {
				that.closeClient(this);
			};
			nsock.onError = function() {
				that.closeClient(this);
			};
			this.clients.push(nsock);
		</function>

		<!-- default actions -->
		<function name="onRequest" params="http">
			if (!http.url) {
				http.errorResponse(400, "Bad Request");
				return;
			}
			var Files = require.weak("files");
			var url = http.url;
			var idx1 = (url.indexOf("/") == 0) ? 1 : 0;
			var idx2 = url.indexOf("?");
			if (idx2 == -1)
				url = url.substring(idx1);
			else
				url = url.substring(idx1, idx2);
			if (url == "")
				url = "index.html";
			var s;
			try {
				s = Files.readChunk(url);
			} catch(e) {
			}
			if (s) {
				http.response("text/html", s);
			}
			else
				http.errorResponse(404, "Not Found");
		</function>
	</object>
	<function name="HTTPServer" params="params" prototype="httpServer">
		this.clients = new Array();
		if (params) {
			this.securityProto = params.securityProto || (params.ssl ? require.weak("SecureSocket") : undefined);
			var sock;
			if ("socket" in params)
				sock = params.socket;	// do not set to this.sock
			else if ("port" in params) {
				params.proto = "tcp";
				sock = this.sock = new ListeningSocket(params);
			}
			if (sock) {
				var that = this;
				sock.onConnect = function() {that.onConnect(this);};
				sock.onClose = function() {that.close();};
				sock.onError = function() {this.close();};
			}
		}
	</function>
</package>
