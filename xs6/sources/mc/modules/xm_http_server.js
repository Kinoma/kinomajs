/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

import HTTP from "http";
import {Socket, ListeningSocket, SecureSocket} from "socket";
import System from "system";

class HTTPServerMessage extends HTTP {
	constructor(sock, server) {
		super();
		this.sock = sock;
		this.server = server;
	}

	_response(status, reason, contentType, s, close) {
		if (close === undefined) {
			let connection = this.getHeader("connection");
			close = (undefined === connection) ? false : (connection.toLowerCase() == "close");
		}
		this.statusCode = status;
		this.setHeader("host", System.hostname);

		/* add headers for blockly web app */
//@@ maybe these shouldn't be present for all HTTP server requests
/*
		this.setHeader("access-control-allow-origin", "*");
		this.setHeader("access-control-allow-methods", "PUT,POST,GET,DELETE,OPTIONS");
		this.setHeader("access-control-allow-headers", "Origin, X-Requested-With, Content-Type, Accept");
*/

		if (close)
			this.setHeader("Connection", "close");
		if (contentType)
			this.setHeader("Content-Type", contentType);
		this.sock.send("HTTP/" + this.version + " " + this.statusCode + " " + reason + "\r\n");
		if (s !== undefined && s !== null) {
			let length = (typeof s == "string") ? s.length : s.byteLength;
			this.setHeader("Content-Length", length);
			this.sendHeaders();
			this.sock.send(s);
		}
		else if (s === null) {
			this.setHeader("Transfer-Encoding", "chunked");
			this.sendHeaders();
		}
		else {
			if (status != 204)	// no content
				this.setHeader("Content-Length", 0);
			this.sendHeaders();
		}
		if (this.version == "1.0" || close)
			this.sock.onClose();
		else
			this.sock.flush();
	}

	response(contentType, s, close) {
		this._response(200, "OK", contentType, s, close);
	}
	responseWithChunk(contentType) {
		this._response(200, "OK", contentType, null, false);
	}
	errorResponse(status, reason, s) {
		this._response(status, reason, undefined, s);
	}
	startss() {
		this.sock.startss.apply(this.sock, arguments);
	}

	callOnRequest() {
		setTimeout(() => {
			this.server.onRequest(this);
			this.content = null;
			this.initMessage();
		}, 0);
	}

	onHeaders() {
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
			let header = this.getHeader("content-length");
			if (header)
				this.contentLength = parseInt(header);
			if ((!header) || this.contentLength == 0)
				this.callOnRequest();
			break;
		default:
			this.errorResponse(405, "Method not Allowed");
			break;
		}
	}
	onDataReady(buffer) {
		if (!this.content)
			this.content = buffer;
		else
			this.content = this.content.concat(buffer);
		if (this.content.byteLength >= this.contentLength)
			this.callOnRequest();
	}
}

class HTTPServer {
	constructor(params) {
		this.clients = [];

		if (params) {
			let sock;
			if ("socket" in params)
				sock = params.socket;	// do not set to this.sock
			else if ("port" in params) {
				params.proto = "tcp";
				sock = this.sock = new ListeningSocket(params);
			}
			if (sock) {
				sock.onConnect = () => this.onConnect(sock);
				sock.onClose = () => this.close();
				sock.onError = sock.onClose;
			}
			this.params = params;
		}
		else
			this.params = {};
	}

	onClose() {	// will be overriden
		this.close();
	}
	close() {
		this.clients.forEach(client => client.close());
		this.clients = [];
		if (this.sock)
			this.sock.close();
	}
	closeClient(s) {
		let n = this.clients.indexOf(s);
		if (n >= 0)
			this.clients.splice(n, 1);
		s.close();
		/* server should not be gone when the only client is gone */
		// if (!this.clients.length)
		// 	this.onClose();	// nothing is running on this server
	}
	onConnect(s) {
		// got a new connection
		let nsock = s.accept();
		let securityProto = this.params.securityProto || (this.params.tls ? SecureSocket : undefined);
		if (securityProto) {
			let params = this.params;
			params.sock = nsock;
			if (!params.protocolVersion && params.tls && typeof params.tls == 'number')
				params.protocolVersion = (Math.floor(params.tls) << 8) | Math.round((params.tls % 1) * 10) | 0x0301;
			nsock = new securityProto(params);
		}
		let message = new HTTPServerMessage(nsock, this);
		nsock.onMessage = n => message.processMessage(n);
		nsock.onClose = () => this.closeClient(nsock);
		nsock.onError = nsock.onClose;
		this.clients.push(nsock);
	}

	// default actions
	onRequest(http) {
		if (!http.url) {
			http.errorResponse(400, "Bad Request");
			return;
		}
		let url = http.url;
		let idx1 = (url.indexOf("/") == 0) ? 1 : 0;
		let idx2 = url.indexOf("?");
		if (-1 == idx2)
			url = url.substring(idx1);
		else
			url = url.substring(idx1, idx2);
		if (url == "")
			url = "index.html";
		let s;
		try {
			let Files = require.weak("files");
			s = Files.read(url);
		} catch(e) {
		}
		if (s)
			http.response("text/html", s);
		else
			http.errorResponse(404, "Not Found");
	}
}

export default HTTPServer;
