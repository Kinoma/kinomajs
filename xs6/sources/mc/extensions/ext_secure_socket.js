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
import {Socket} from "socket";
import SSL from "ssl";

export default class SecureSocket {
	constructor(dict) {
		const sock = dict.sock ? dict.sock : new Socket(dict);
		const options = (dict.secure && typeof dict.secure != 'boolean') ? dict.secure : dict;

		this._init(sock, options);
	};
	_init(sock, options) {
		this.sock = sock;
		this.handshaking = true;
		this.closing = false;
		this.error = false;
		this.ssl = new SSL.Session(options);
		var that = this;
		sock.onConnect = function() {
			// should be called only on the client side
			try {
				that.ssl.initiateHandshake(this);
			} catch (e) {
				that.error = true;
				that.onError();		// leave the caller to call close
			}
		};
		sock.onMessage = function(n) {
			try {
				that.messageHandler(n);
			} catch (e) {
				that.error = true;
				that.onError();
			}
		};
		sock.onWritable = function(n) {
			try {
				that.messageHandler(0);
			} catch (e) {
				that.error = true;
				that.onError();
			}
		};
		sock.onError = function() {
			that.error = true;
			that.onError();
		};
		sock.onClose = function() {
			if (!that.closing) {
				// the connection has been closed somehow before receiving the closure alert
				// trace("# SSL: connection closed without alert!\n");
			}
			that.closing = true;
			that.onClose();
		};
	};
	messageHandler(n) {
		if (this.handshaking) {
			if (this.ssl.handshake(this.sock, n)) {
				this.handshaking = false;
				if (this.onConnect)
					this.onConnect();	// only clients should set onConnect
			}
		}
		else {
			if (n <= 0)
				return;
			if (this.ssl.bytesAvailable == 0) {
				// decode a packet to get how many bytes avialbe in plain
				if (!this.ssl.read(this.sock)) {
					// the session has been closed
					this.closing = true;
					return;
				}
			}
			while ((n = this.ssl.bytesAvailable) > 0)
				this.onMessage(n);
		}
	};
	send(data) {
		switch (typeof data) {
		case "number":
			var a = Uint8Array(1);
			a[0] = data & 0xff;
			data = a.buffer;
			break;
		case "string":
			data = ArrayBuffer.fromString(data);
			break;
		case "object":
			if (data instanceof Array) {
				data.forEach(function(e) {
					this.send(e);
				}, this);
			}
			break;
		}
		this.ssl.write(this.sock, data);
	};
	recv(nbytes) {
		return this.ssl.read(this.sock, nbytes);
	};
	flush() {
		// nothing to flush
	};
	get bytesWritable(){
		return this.sock.bytesWritable;
	};
	close() {
		if (this.error || this.closing) {
			this.shutdown();
			return;
		}
		// give a chance to read up the closing alert
		if (this.ssl.read(this.sock))
			this.ssl.close(this.sock);
	};
	shutdown() {
		this.sock.close();
	};
	starttls() {
		this.handshaking = true;
		this.ssl.initiateHandshake(this.sock);
	};
};
