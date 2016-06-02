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

import SSLProtocol from "ssl/protocol";

const maxFragmentSize = 16384	// maximum record layer framgment size (not a packet size): 2^14

export default class SSLSession {
	constructor(options) {
		let SSL = require.weak("ssl");
		let CacheManager = require.weak("ssl/cache");
		let CertificateManager = require.weak("ssl/cert");
		this.options = options || {};
		this.packetBuffer = new ArrayBuffer(0);
		this.handshakeMessages = undefined;
		this.clientSessionID = this.serverSessionID = undefined;
		this.clientCerts = undefined;
		this.myCert = undefined;
		this.handshakeProcess = -1;
		this.connectionEnd = false;
		this.clientCipher = null;
		this.serverCipher = null;
		this.alert = undefined;
		this.protocolVersion = this.options.protocolVersion || SSL.protocolVersion;
		this.minProtocolVersion = this.options.protocolVersion || SSL.minProtocolVersion;	// only for the server side
		this.maxProtocolVersion = SSL.maxProtocolVersion;	// ditto
		this.applicationData = undefined;
		this.cacheManager = (this.options.cache === undefined || this.options.cache) && new CacheManager();
		this.certificateManager = new CertificateManager(options);
	};
	initiateHandshake(s) {
		this.connectionEnd = true;
		this.handshakeProcess = SSLProtocol.handshakeProtocol.helloRequest.msgType;
	};
	handshake(s, n) {
		var handshakeProtocol = SSLProtocol.handshakeProtocol;
		var state = 0;
		switch (this.handshakeProcess) {
		case handshakeProtocol.helloRequest.msgType:		// C
			var cache = this.cacheManager && this.cacheManager.getByHost(s.host);
			if (cache) {
				this.clientSessionID = cache.id;
				this.masterSecret = cache.secret;
			}
			this.doProtocol(s, handshakeProtocol.clientHello);
			break;
		case handshakeProtocol.clientHello.msgType:		// S
			var masterSecret = this.clientSessionID && this.cacheManager && this.cacheManager.getByID(this.clientSessionID);
			if (masterSecret) {
				// resumed handshake
				this.serverSessionID = this.clientSessionID;
				this.masterSecret = masterSecret;
				this.doProtocol(s, handshakeProtocol.serverHello);
				this.doProtocol(s, SSLProtocol.changeCipherSpec);
				this.doProtocol(s, handshakeProtocol.finished);
			}
			else {
				// assign a new one
				let Arith = require.weak("arith");
				this.serverSessionID = (new Arith.Integer(((new Date()).valueOf()).toString())).toChunk();
				this.doProtocol(s, handshakeProtocol.serverHello);
				// S -> C: Certificate   (always -- i.e. not support anonymous auth.)
				var certs = this.certificateManager.getCerts();
				if (!certs || !certs.length)
					throw new Error("SSL: client_hello: no certificate");
				this.doProtocol(s, handshakeProtocol.certificate, certs);
				if (this.options.clientAuth)
					// S -> C: CertificateRequest
					this.doProtocol(s, handshakeProtocol.certificateRequest, options.clientAuth.cipherSuites, options.clientAuth.subjectDN);
				// S -> C: ServerHelloDone
				this.doProtocol(s, handshakeProtocol.serverHelloDone);
			}
			break;
		case handshakeProtocol.serverHelloDone.msgType:		// C
			if (this.clientCerts !== undefined)
				this.doProtocol(s, handshakeProtocol.certificate, this.clientCerts);
			this.doProtocol(s, handshakeProtocol.clientKeyExchange);
			if (this.myCert)	// client cert request && and the certs is not empty
				this.doProtocol(s, handshakeProtocol.certificateVerify, this.myCert);
			this.doProtocol(s, SSLProtocol.changeCipherSpec);
			this.doProtocol(s, handshakeProtocol.finished);
			break;
		case handshakeProtocol.finished.msgType:		// C, S
			let Bin = require.weak("bin");
			var resumed = this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) == 0;
			if (!(this.connectionEnd ^ resumed)) {
				this.doProtocol(s, SSLProtocol.changeCipherSpec);
				this.doProtocol(s, handshakeProtocol.finished);
			}
			state = 2;
			if (this.cacheManager) {
				if (this.serverSessionID && s.host)
					this.cacheManager.saveSession(s.host, this.serverSessionID, this.masterSecret);
				if (this.connectionEnd && this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) != 0)
					// C: tried to resume but got rejected
					this.cacheManager.deleteSessionID(this.clientSessionID);
				this.cacheManager = undefined;
			}
			// set undefined to instance variables that are no longer necessary
			this.handshakeMessages = this.clientSessionID = this.serverSessionID = this.clientCerts = this.myCert = this.certificateManager = undefined;
			break;
		default:
			if (n == 0)
				break;
			// process a packet once even if more than one packets are available
			var buf = this.readPacket(s, n);
			if (buf) {
				this.startTrace("unpacketize");
				SSLProtocol.recordProtocol.unpacketize(this, buf);
			}
			state = 1;
			break;
		}
		if (this.alert) {
			if (this.alert.description != SSLProtocol.alert.close_notify)
				this.doProtocol(s, SSLProtocol.alert, this.alert.level, this.alert.description);
			return true;	// stop handshaking right away
		}
		if (state == 0)
			this.handshakeProcess = -1;
		return state == 2;
	};
	read(s, n) {
		var data = this.applicationData;
		if (!data || data.byteLength == 0) {
			// read at least one packet and just keep it
			var buf = this.readPacket(s, s.bytesAvailable);
			if (!buf)
				return data;	// return an empty buffer
			this.startTrace("unpacketize");
			SSLProtocol.recordProtocol.unpacketize(this, buf);
			if (this.alert)
				return null;	// probably the session has been closed by the peer
		}
		if (n === undefined)
			return this.applicationData;	// return the whole data whether it is empty or not -- DO NOT modify the content
		else if (this.applicationData) {
			data = this.applicationData;
			this.applicationData = data.slice(n);
			if (data.byteLength > n)
				data = data.slice(0, n);
			return data;
		}
		return null;
	};
	write(s, data) {
		if (data.byteLength > maxFragmentSize)
			return -1;	// too large
		this.startTrace("packetize");
		var cipher = SSLProtocol.recordProtocol.packetize(this, SSLProtocol.recordProtocol.application_data, data);
		s.send(cipher);
		return data.length;
	};
	close(s) {
		this.startTrace("packetize");
		var packet = SSLProtocol.alert.packetize(this, 0, SSLProtocol.alert.close_notify);
		s.send(packet);
	};
	doProtocol(s, protocol, param1, param2) {
		this.startTrace("packetize");
		var packet = protocol.packetize(this, param1, param2);
		s.send(packet);
	};
	readPacket(s, n) {
		var buf = this.packetBuffer;
		if (buf.byteLength < 5) {
			if (n < 5)
				return;
			var c = s.recv(5);
			if (!c)
				return;
			this.packetBuffer = buf = buf.concat(c);
			if (buf.byteLength < 5)
				return;
			n -= 5;
		}
		var a = new Uint8Array(buf);
		var len = (a[3] << 8) | a[4];
		len += 5 /* header size */;
		if (len > buf.byteLength) {
			var rest = len - buf.byteLength;
			if (rest > n)
				rest = n;
			var c = s.recv(rest);
			if (!c)
				return;
			this.packetBuffer = buf = buf.concat(c);
			if (buf.byteLength < len)
				return;
		}
		this.packetBuffer = buf.slice(len);
		return buf.byteLength > len ? buf.slice(0, len) : buf;
	};
	get bytesAvailable() {
		return this.applicationData ? this.applicationData.byteLength : 0;
	};
	putData(data) {
		this.applicationData = !this.applicationData || this.applicationData.byteLength == 0 ? data : this.applicationData.concat(data);
	};

	// for debugging only
	/*
	startTrace(direction) {
		this.traceLevel = 0;
		this.traceDirection = direction;
	};
	traceProtocol(protocol) {
		var Debug = require.weak("debug");
		var muse = Debug.report(true);
		var indent = "";
		for (var n = this.traceLevel + 1; --n >= 0;)
			indent += ">";
		trace(indent + " " + protocol.name + "." + this.traceDirection + ", chunk = " + muse.chunk + ", slot = " + muse.slot + "\n");
		this.traceLevel++;
	}
	*/
	startTrace() {};
	traceProtocol() {};
};
