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
import HTTPServer from "HTTPServer";
import WebSocketMessage from "message";

var webSocketServerMessage = Object.create(WebSocketMessage.prototype);

webSocketServerMessage.onopen = function() {};
webSocketServerMessage.onmessage = function() {};
webSocketServerMessage.onclose = function() {};
webSocketServerMessage.onerror = function() {};

webSocketServerMessage.send = function(data) {
	if (typeof data == "string")
		var opcode = 0x01;
	else
		var opcode = 0x02;
	this._send(opcode, data);
};

function WebSocketServerMessage(sock) {
	WebSocketMessage.call(this, sock, this);
}

WebSocketServerMessage.prototype = webSocketServerMessage;

export class WebSocketServer extends HTTPServer {
	constructor(port) {
		super({port});
		this.wsclients = [];
	}
	onStart(client) {}
	close() {
		for (var n = this.wsclients.length; --n >= 0;) {
			var ws = this.wsclients[n];
			ws.close();
		}
		if (this.sock)
			this.sock.close();
	}
	onRequest(http) {
		var key = http.getHeader("Sec-WebSocket-Key");

		if (!key) {
			http.response();	// @@
			return;
		}

		const {hash} = require.weak('common');
		http.setHeader("Sec-WebSocket-Accept", hash(key));
		http.setHeader("Connection", "Upgrade");
		http.setHeader("Upgrade", "websocket");
		http._response(101, "Switching Protocols");

		var client = new WebSocketServerMessage(http.sock);
		var that = this;
		client.onClose = function() {
			var i = that.wsclients.indexOf(this);
			if (i >= 0)
				that.wsclients.splice(i, 1);
			http.sock.close();
			// if (that.wsclients.length == 0)	// we should not close the server when the only one client is gone..
			// 	that.onClose();
		};
		this.onStart(client);
		if (client.onmessage) {

			this.wsclients.push(client);
		}
	}
}

export default WebSocketServer;
