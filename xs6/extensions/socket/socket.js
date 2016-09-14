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

class SocketHost @ "xs_socket_destructor" {
	constructor(params) @ "xs_socket_constructor";
	close() @ "xs_socket_close";
	send(data, addr) @ "xs_socket_send";
	recv(n, b) @ "xs_socket_recv";
	connect(addr) @ "xs_socket_connect";
	flush() @ "xs_socket_flush";
	static aton(str) @ "xs_socket_aton";
	static ntoa(addr) @ "xs_socket_ntoa";

	get port() @ "xs_socket_getPort";
	get addr() @ "xs_socket_getAddr";
	get peerAddr() @ "xs_socket_getPeerAddr";
	get peerPort() @ "xs_socket_getPeerPort";
	get nativeSocket() @ "xs_socket_getNativeSocket";

	get bytesWritable() @ "xs_socket_getBytesWritable";
	get bytesAvailable() @ "xs_socket_getBytesAvailable";
};

export class Socket extends SocketHost {
	constructor(params) {
		super(params);
		if (params && params.host)
			Socket.resolv(params.host, addr => this.connect(addr));	// resolver has to be async
	};
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
	write(...params) {
		if (arguments.length == 1)
			return this.send(arguments[0]);
		else
			return this.send(Array.apply(null, arguments));
	};
	get peer() {
		return this.peerAddr + ":" + this.peerPort;
	};

	onConnect() {};
	onMessage(n) {this.onData(this.recv(n))};	// the default behavior
	onData(data) {};
	onClose() {};
	onError() {};
};

export function resolv(name, f) {
	let mdns = require.weak("mdns");
	mdns.resolv(name, f);
}

export const TCP = "tcp";
export const UDP = "udp";

Socket.resolv = resolv;
Socket.TCP = TCP;
Socket.UDP = UDP;

export class ListeningSocket extends Socket {
	constructor(params) @ "xs_socket_listeningSocket";
	_accept(s) @ "xs_socket_accept";
	accept(s) {
		if (!s)
			s = new Socket();
		this._accept(s);
		return s;
	};
};

export default {
	Socket,
	ListeningSocket,
	resolv,
	TCP,
	UDP
};
