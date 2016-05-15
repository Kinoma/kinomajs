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
import HTTPClient from "HTTPClient";

function generateKey() {
	const {rng} = require.weak('crypt');
	const {encode} = require.weak('bin');
	return encode(rng(16));	// base64 encoded random string
}

export class WebSocketClient extends HTTPClient {
	constructor(host, subprotocols) {
		super(host);

		var key = generateKey();

		// calculate the response in advance
		const {hash} = require.weak('common');
		this.response = hash(key);

		var headers = [
			["Upgrade", "websocket"],
			["Connection", "Upgrade"],
			["Sec-WebSocket-Key", key],
			["Origin", "http://kinoma.com"],	// @@
			["Sec-WebSocket-Version", "13"]];
		for (var i = 0; i < headers.length; i++)
			this.setHeader(headers[i][0], headers[i][1]);
		if (subprotocols)
			this.setHeader("Sec-WebSocket-Protocol", subprotocols);

		this.version = "1.1";

		this.start();
	}
	onopen() {
	}
	onmessage() {
	}
	onclose() {
	}
	onerror() {
	}
	send(data) {
		if (typeof data == "string")
			var opcode = 0x01;
		else
			var opcode = 0x02;
		this.ws._send(opcode, data);
	}
	close(code, reason) {
		this.ws.close(code, reason);
	}
	onHeaders() {
		var response = this.getHeader("Sec-WebSocket-Accept");
		// var bodyResponse = String.fromArrayBuffer(this.response);

		if (this.statusCode != 101 || response != this.response) {
			this.close();
			return;
		}

		const WebSocketMessage = require.weak('message');
		this.ws = new WebSocketMessage(this.sock, this, true);
		this.ws.onClose = function() {
			this.sock.close();
			this.proto.onclose();
		};

		this.onopen();
	}
	onTransferComplete(status) {
		if (status && this.statusCode >= 200 && this.statusCode < 300)
			this.onclose();
		else
			this.onerror({statusCode: this.statusCode});
	}
}

export default WebSocketClient;
