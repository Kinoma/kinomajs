/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
export class Socket @ "xs_socket_destructor" {
	constructor(params) @ "xs_socket_constructor";
	close() @ "xs_socket_close";
	send(data, addr) @ "xs_socket_send";
	write(...items) @ "xs_socket_write";
	recv(n, b) @ "xs_socket_recv";

	get port() @ "xs_socket_getPort";
	get addr() @ "xs_socket_getAddr";
	get peerAddr() @ "xs_socket_getPeerAddr";
	get peerPort() @ "xs_socket_getPeerPort";
	get peer() {
		return this.peerAddr + ":" + this.peerPort;
	};
	get bytesWritable() @ "xs_socket_getBytesWritable";
	get bytesAvailable() @ "xs_socket_getBytesAvailable";

	read(cons, n, buf) {
		if ((buf = this.recv(n, buf)) === null)
			return buf;
		if (cons === undefined)
			;
		else if (cons == String) {
			// too bad we have to examine the constructor type just for String
			buf = String.fromArrayBuffer(buf);
		}
		else
			buf = new cons(buf);
		return buf;
	};

	// will be overwritten
	onConnect() {};
	onError() {};
	onClose() {};
	onMessage(n) {
		var data = this.recv(n);
		if (data && data.byteLength > 0) this.onData(data);
	}

	get reader() {
		if (!('_reader' in this)) {
			let Reader = require.weak("utils/buffer").Reader;
			this._reader = new Reader();
		}
		return this._reader;
	}

	onData(bytes) {
		if ('onDataReady' in this) {
			let reader = this.reader;
			reader.feed(bytes);

			this.onDataReady(reader);
		}
	}

	// onDataReady(reader) { }

	static resolv(name, f) @ "xs_socket_resolv";

	flush() @ "xs_socket_flush";
	get nativeSocket() @ "xs_socket_getNativeSocket";
};

export class ListeningSocket extends Socket {
	constructor(params) @ "xs_socket_listeningSocket";
	accept(s) @ "xs_socket_accept";
}

export const TCP = "tcp";
export const UDP = "udp";

Socket.TCP = TCP;
Socket.UDP = UDP;

export default {
	Socket,
	ListeningSocket,
	TCP,
	UDP
};
