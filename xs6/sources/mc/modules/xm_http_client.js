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
import {Socket} from "socket";

class HTTPClient extends HTTP {
	constructor(uri, options) {
		super();

		let idx, idx2;
		if (uri.startsWith("http://")) {
			this.ssl = false;
			idx = 7;
		}
		else if (uri.startsWith("https://")) {
			this.ssl = true;
			idx = 8;
		}
		else if (uri.startsWith("ws://")) {
			this.ssl = false;
			idx = 5;
		}
		else if (uri.startsWith("wss://")) {
			this.ssl = true;
			idx = 6;
		}
		else
			throw new Error(-1);
		if ((idx2 = uri.indexOf("/", idx)) >= 0 || (idx2 = uri.indexOf("?", idx)) >= 0) {
			this.adrs = uri.substring(idx, idx2);
			this.url = uri.substring(idx2);
		}
		else {
			this.adrs = uri.substring(idx);
			this.url = "/";
		}

		this.stream = null;
		this.options = options || {};
	}

	start(s) {
		let idx, adr, port;
		if ((idx = this.adrs.indexOf(":")) >= 0) {
			adr = this.adrs.substring(0, idx);
			port = parseInt(this.adrs.substring(idx + 1));
		}
		else {
			adr = this.adrs;
			port = this.ssl ? 443 : 80;
		}
		this.stream = s;
		let params = {host: adr, port: port, proto: "tcp"};
		for (let i in this.options)
			params[i] = this.options[i];
		if (this.ssl || params.tls) {
			let SecureSocket = require.weak("SecureSocket");
			if (!params.tls_server_name)
				params.tls_server_name = adr;
			if (!params.tls_max_fragment_length)
				params.tls_max_fragment_length = 1024;
			if (!params.protocolVersion && params.tls && typeof params.tls == 'number')
				params.protocolVersion = (Math.floor(params.tls) << 8) | Math.round((params.tls % 1) * 10) | 0x0301;
			this.sock = new SecureSocket(params);
		}
		else
			this.sock = new Socket(params);
		this.sock.onConnect = this.onConnect;
		this.sock.onMessage = this.onMessage;
		this.sock.onError = this.onError;
		this.sock.onClose = this.onClose;
		this.sock.http = this;
		this.setHeader("host", adr);
	}

//@@ deprecated. set the method property directly. remove after SXSW 2016
	setRequestMethod(method) {
		this.method = method;
	}

	setAuth(user, password) {
		const Bin = require.weak('bin');
		let data = ArrayBuffer.fromString(user + ':' + password);
		let auth = 'Basic ' + Bin.encode(data);

		this.setHeader("Authorization", auth);
	}

	// default action
	onDataReady(buffer) {
		if (!this.content)
			this.content = buffer;
		else
			this.content = this.content.concat(buffer);
	}

	// callbacks from the socket
	onConnect() {
		let http = this.http;
		this.send(http.method + " " + http.url + " HTTP/" + http.version + "\r\n");
		http.sendHeaders();
		if (http.stream) {
			this.send(http.stream);
			http.onDataSent(http.stream.length);
		}
		http.initMessage();
		http.content = null;
	}
	onMessage(n) {
		this.http.processMessage(n);
	}
	onError() {
		this.close();
		try {
			this.http.onTransferComplete(false);
		}
		finally {
		}
	}
	onClose() {
		this.close();
		try {
			this.http.onTransferComplete(true);
		}
		finally {
		}
	}
}

export default HTTPClient;
