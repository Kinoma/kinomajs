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

const maxMessageLength = 16384;		// 16k is the maximum fragment length of SSL

export default class HTTP {
	constructor() {
		this.version = "1.1";

		this.url = null;
		this.content = null;
		this.sock = undefined;
		this.statusCode = 0;
		this.method = "GET";

		this.initMessage();
	}

	initMessage() {
		this.process = 0;
		this.message = "";
		this.outgoingHeaders = new Map();
	}

//@@ decomposeUrl should be static, right?
	decomposeUrl(url) {
		let parts = url.split("/"), args;
		parts = parts.filter(Boolean);
		let idx = parts[parts.length - 1].indexOf("?");
		if (idx >= 0) {
			let last = parts[parts.length - 1];
			parts[parts.length - 1] = last.substring(0, idx);
			args = {};
			last.substring(idx + 1).split("&").forEach(function(elm, i, arr) {
				if ((i = elm.indexOf("=")) > 0)
					args[decodeURIComponent(elm.substring(0, i))] = decodeURIComponent(elm.substring(i + 1));
			});
		}
		return {
			path: parts,
			query: args,
		};
	}

	addHeader(key, value) {		//@@ deprecated. remove after SXSW 2016
		this.setHeader(key, value);
	}
	setHeader(key, value) {
		this.outgoingHeaders.set(key, value);
	}
	getHeader(key) {
		return this.incomingHeaders.get(key.toLowerCase());
	}
	get isChunked() {
		return (this.getHeader('transfer-encoding') == 'chunked');
	}
	sendHeaders() {
		for (let [key, value] of this.outgoingHeaders)
			this.sock.send(`${key}: ${value}\r\n`);
		this.sock.send("\r\n");
	}

	putChunk(s) {
		let length = (typeof s == "string") ? s.length : s.byteLength;
		this.sock.send(length.toString(16) + "\r\n");
		this.sock.send(s);
		this.sock.send("\r\n");
	}
	terminateChunk(footer) {
		this.sock.send("0\r\n");
		if (footer) {
			for (let name in footer)
				this.sock.send(name + ": " + footer[name] + "\r\n");
		}
		this.sock.send("\r\n");
	}

	processMessage(n) {
		let buf = this.sock.recv(n > this.maxMessageLength ? this.maxMessageLength : n);
		if (!buf)
			return;

		/*
			process == 0 : first line of headers
			process == 1 : rest of headers
			process == 2 : body
		 */

		if (this.process < 2) {
			this.message = this.message.concat(String.fromArrayBuffer(buf));

			while (true) {
				let n = this.message.indexOf("\r\n");
				if (n < 0) break;

				let line = this.message.substring(0, n);
				buf = buf.slice(n + 2);

				if (line === "") {	// end of the headers -- go on to the response data
					this.onHeaders();
					this.process = 2;
					this.message = null; // reuse for chunked body
					break; // fall thru to process rest of `buf`
				}

				this.message = this.message.substring(n + 2);

				if (this.process == 0) {	// in the status line or the request
					let parts = line.split(" ");
					if (parts[0].indexOf("HTTP/") == 0)
						this.statusCode = parseInt(parts[1]);
					else {
						this.method = parts[0];
						this.url = parts[1];
						this.version = (parts[2].split("/"))[1];
					}
					this.incomingHeaders = new Map();
					this.process++;
				}
				else {	// in the response headers
					let idx = line.indexOf(":");
					if (idx >= 0)
						this.incomingHeaders.set(line.substring(0, idx).trim().toLowerCase(), line.substring(idx + 1).trim());
				}
			}
		}

		if (this.process == 2 && buf.byteLength > 0) {
			this.processBody(buf);
		}
	}

	processBody(blob) {
		if (this.isChunked) {
			if (this.message) {
				this.message = this.message.concat(blob);
			} else {
				this.message = blob;
			}

			while (true) {
				let msg = String.fromArrayBuffer(this.message);
				let n = msg.indexOf("\r\n");
				if (n < 0) break;

				let len = parseInt(msg.substring(0, n), 16);
				n += 2;
				if (this.message.byteLength < (n + len + 2)) break;

				blob = this.message.slice(n, n + len);
				this.message = this.message.slice(n + len + 2);
				this.onDataReady(blob);
			}
		} else {
			this.onDataReady(blob);
		}
	}

	onHeaders() {
	}
	onDataReady() {
	}
	onDataSent() {
	}
	onTransferComplete() {
	}
}
