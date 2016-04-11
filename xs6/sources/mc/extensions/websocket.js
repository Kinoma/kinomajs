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
import Bin from "bin";
import Crypt from "crypt";
import HTTPClient from "HTTPClient";
import HTTPServer from "HTTPServer";
import Utils from "utils";

function PacketReader() {
	Utils.BlobReader.call(this);
	this.frame = this.reset();
}

PacketReader.prototype = Object.create(Utils.BlobReader.prototype);

PacketReader.prototype.reset = function() {
	return {
	};
};

PacketReader.prototype.readFrame = function() {
	var frame = this.frame;

	if (frame.opcode === undefined) {
		var bytes = this.read(1);
		if (bytes === undefined) return undefined;

		var c = bytes[0];
		frame.fin = c >> 7;
		frame.opcode = c & 0x0f;
	}

	if (frame.masklen === undefined) {
		var bytes = this.read(1);
		if (bytes === undefined) return undefined;

		var c = bytes[0];
		frame.masklen = (c >> 7) ? 4 : 0;

		var len = c & 0x7f;

		if (len == 126) {
			frame.lenlen = 2;
		} else if (len == 127) {
			frame.lenlen = 8;
		}
		else
			frame.paylen = len;
	}

	if (frame.paylen === undefined) {
		var bytes = this.read(frame.lenlen);
		if (bytes === undefined) return undefined;

		frame.paylen = 0;
		while (bytes.length > 0) {
			var c = bytes.shift();
			frame.paylen = (frame.paylen * 256) + c;	// don't care about overflow
		}
	}

	if (frame.peerMask === undefined) {
		if (frame.masklen > 0) {
			var bytes = this.read(frame.masklen);
			if (bytes === undefined) return undefined;

			frame.peerMask = bytes;
		} else {
			frame.peerMask = null;
		}
	}

	if (frame.payload === undefined) {
		if (frame.paylen > 0) {
			var bytes = this.read(frame.paylen);
			if (bytes === undefined) return undefined;

			frame.payload = (new Uint8Array(bytes)).buffer;

			if (frame.peerMask) {
				xor(frame.payload, frame.peerMask);
			}
		} else {
			frame.payload = null;
		}
	}

	this.frame = this.reset();
	return frame;
};


function generateKey() {
	var key = Crypt.rng(16);
	return Bin.encode(key);	// base64 encoded random string
}

function hash(key) {
	var sha1 = new Crypt.SHA1();
	sha1.update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	var digest = sha1.close();
	return Bin.encode(digest);
}

function xor(payload, mask) {
	var a = new Uint8Array(payload);
	for (var i = 0, l = a.length; i < l; i++) {
		a[i] = a[i] ^ mask[i % 4];
	}
}

function WebSocketMessage(sock, proto, mask) {
	this.state = 0;
	this.data = undefined;
	this.closing = false;
	this.sock = sock;
	this.proto = proto;
	this.peerMask = undefined;

	if (mask) {
		var blob = Crypt.rng(4);
		this.maskKey = new Uint8Array(blob);
	}
	else
		this.maskKey = undefined;
	// upgrading
	var that = this;
	sock.onConnect = function() {};	// should not be called anymore
	sock.onMessage = function(n) {
		var frame = this.recv(n > 128 ? 128 : n);
		if (!frame) return;
		that.process(frame);
	};
	sock.onError = function() {
		proto.onerror({});
	};
	sock.onClose = function() {
		that.onClose();	// propagate the closing event
	};

	this.reader = new PacketReader();
};

WebSocketMessage.prototype = {
	onClose() {
	},

	process(blob) {
		this.reader.feed(blob);

		let frame;
		while(frame = this.reader.readFrame()){
			switch (frame.opcode) {
				case 0x01:
				case 0x02:
					this.data = this.data ? this.data.concat(frame.payload) : frame.payload;

					if (frame.fin) {
						var data = this.data
						if (frame.opcode == 0x01 && data) {
							data = String.fromArrayBuffer(data);
						}

						this.proto.onmessage({data});
						this.data = undefined;
					}
					break;

				case 0x08:	// close
					this.proto.onclose();
					if (!this.closing)
						this._send(0x08);
					this.onClose();
					break;

				case 0x09:	// ping
					this._send(0x0a, frame.payload);
					break;

				case 0x0a:	// pong
					break;
			}
		}
	},

	packetize(opcode, data) {
		var maskbit = this.maskKey ? 0x80 : 0;
		var payload = (typeof data == "string") ? ArrayBuffer.fromString(data) : data;
		var paylen = payload ? payload.byteLength : 0;
		var encoder = new Utils.Encoder();

		encoder.pushByte(0x80 | opcode);	// FIN | opcode
		// payload length (no mask key)
		if (paylen >= (2 << 16)) {
			encoder.pushByte(127 | maskbit)
			for (var i = 8; --i >= 0;)
				encoder.pushByte((paylen >> (i * 8)) & 0xff);
		}
		else if (paylen >= 126) {
			encoder.pushByte(126 | maskbit);
			for (var i = 2; --i >= 0;)
				encoder.pushByte((paylen >> (i * 8)) & 0xff);
		}
		else
			encoder.pushByte(paylen | maskbit);
		if (this.maskKey)
			for (var i = 0; i < 4; i++)
				encoder.pushByte(this.maskKey[i]);


		if (this.maskKey && paylen > 0) {
			xor(payload, this.maskKey);
		}

		if (payload) encoder.pushBlob(payload);

		return encoder.getBlob();
	},

	close(code, reason) {
		var encoder = new Utils.Encoder();
		if (code !== undefined) {
			encoder.pushUInt16(code);
		}
		if (reason !== undefined) {
			if (typeof reason == "string")
				encoder.pushString(reason);
			else if (reason instanceof ArrayBuffer)
				encoder.pushBlob(reason);
		}
		this._send(0x08, encoder.getBlob());
		this.closing = true;	/* client should wait for the server to close the connection */
	},

	_send(opcode, data) {
		var blob = this.packetize(opcode, data);
		this.sock.send(blob);
	}
}

export class WebSocketClient extends HTTPClient {
	constructor(host, subprotocols) {
		super(host);
		
		var key = generateKey();

		// calculate the response in advance
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
export const WebSocket = WebSocketClient;

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

export default {WebSocket, WebSocketClient, WebSocketServer};
