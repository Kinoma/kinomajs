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

class RawSocket @ "xs_socket_destructor" {
	constructor(params) @ "xs_socket_constructor";
	close() @ "xs_socket_close";
	send(data, addr) @ "xs_socket_send";
	recv(n, b) @ "xs_socket_recv";
	connect(addr) @ "xs_socket_connect";
	flush() @ "xs_socket_flush";
	static aton(str) @ "xs_socket_aton";
	static ntoa(addr) @ "xs_socket_ntoa";
	static resolv(name, f) @ "xs_socket_resolv";	// obsolete

	get port() @ "xs_socket_getPort";
	get addr() @ "xs_socket_getAddr";
	get peerAddr() @ "xs_socket_getPeerAddr";
	get peerPort() @ "xs_socket_getPeerPort";
	get nativeSocket() @ "xs_socket_getNativeSocket";

	get bytesWritable() @ "xs_socket_getBytesWritable";
	get bytesAvailable() @ "xs_socket_getBytesAvailable";

	// will be overwritten (kind of virtual)
	_onConnect() {};
	_onMessage(n) {};
	_onError() {};
	_onClose() {};
};

export class Socket extends RawSocket {
	constructor(params) {
		var host;
		if (params && params.host) {
			host = params.host;
			params.host = "";	// tell Socket that the hostname is resolving
		}
		super(params);
		if (host)
			Socket.resolv(host, addr => this.connect(addr));	// resolver has to be async
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
	onMessage(n) {this.onData(this.recv(n))};
	onData(data) {};
	onClose() {};
	onError() {};
	_onConnect() {
		this.onConnect();
	};
	_onMessage(n) {
		this.onMessage(n);
	};
	_onClose() {
		this.onClose();
	};
	_onError() {
		this.onError();
	};
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
