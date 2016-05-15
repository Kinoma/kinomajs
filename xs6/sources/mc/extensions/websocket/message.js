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

function BlobReader() {
	this.blobs = [];
	this.remains = 0;
	this.pos = 0;
}

BlobReader.prototype = {
	feed: function(blob) {
		this.blobs.push(new DataView(blob));
		this.remains += blob.byteLength;
	},
	read: function(n) {
		if (this.blobs.length == 0 || this.remains < n) return undefined;

		var result = new Array(n);
		this.remains -= n;
		var p = 0;
		while (p < n && this.blobs.length > 0) {
			var blob = this.blobs[0];
			var pos = this.pos;
			for (var c = blob.byteLength; pos < c && p < n; pos++, p++) {
				result[p] = blob.getUint8(pos);
			}

			if (pos < blob.byteLength) {
				this.pos = pos;
			} else {
				this.blobs.shift();
				this.pos = 0;
			}
		}

		return result;
	},
};

function Encoder() {
	this.bytes = [];
};

Encoder.prototype = {
	pokeInto(blob, offset) {
		var bytes = this.bytes;
		var view = new DataView(blob, offset);
		for (var i = 0, c = bytes.length; i < c; i++) {
			view.setUint8(i, bytes[i]);
		}
	},

	pushByte(b) {
		this.bytes.push(b);
	},

	pushBytes(a) {
		var len = a.length;
		for (var i = 0; i < len; i++) {
			this.pushByte(a[i]);
		}
	},

	pushUInt16(val) {
		this.bytes.push((val >> 8) & 0xff);
		this.bytes.push(val & 0xff);
	},

	pushString(str) {
		if (str.length > 0)
			this.pushBlob(ArrayBuffer.fromString(str));
	},

	pushBlob(blob) {
		if (blob.byteLength > 0)
			this.pushBytes(new Uint8Array(blob));
	},

	get length() {
		return this.bytes.length;
	},

	getBlob() {
		var blob = new ArrayBuffer(this.length);
		this.pokeInto(blob);
		return blob;
	}
};

function PacketReader() {
	BlobReader.call(this);
	this.frame = this.reset();
}

PacketReader.prototype = Object.create(BlobReader.prototype);

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
		const {rng} = require.weak('crypt');
		this.maskKey = new Uint8Array(rng(4));
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
		var encoder = new Encoder();

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
		var encoder = new Encoder();
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

export default WebSocketMessage;
