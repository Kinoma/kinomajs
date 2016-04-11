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

import System from "system";
import TimeInterval from "timeinterval";
import server from "mdns/server";

const DEFAULT_TTL = 255;
const DEFAULT_INTERVAL = 250;
const MAX_INT = (1 << 31) - 1;

const DNS_RRTYPE_A = 1;		// a host address
const DNS_RRTYPE_PTR = 12;
const DNS_RRTYPE_TXT = 16;
const DNS_RRTYPE_SRV = 33;

class MSocket {
	constructor(sock, params) {
		this.sock = sock;
		this.restart(params);
	};
	restart(params) {
		this.data = params.data;
		this.unicast = params.unicast;
		this.onComplete = params.onComplete;
		this.timeout = params.timeout || 0;
		this.repeat = params.repeat || 0;
		this.interval = params.interval || [DEFAULT_INTERVAL];
		this.wait = params.wait || 0;
		let delay = params.delay ? params.delay * Math.random() : 0;
		this.t0 = new TimeInterval(() => {
			this.t0.close();
			delete this.t0;
			this.start();
		}, delay);
		this.t0.start();
	};
	start() {
		this.sock.send(this.data, this.unicast ? this.sock.peer : undefined);
		if (this.repeat == 0 && this.timeout == 0) {
			this.complete();
			return;
		}
		if (--this.repeat >= 0) {
			var i = 0;
			this.t1 = new TimeInterval(() => {
				this.sock.send(this.data, this.unicast ? this.sock.peer : undefined);
				if (--this.repeat <= 0)
					this.complete();
				else {
					if (i < this.interval.length)
						this.t1.reschedule(this.interval[i++]);
				}
			}, this.interval[i++]);
			this.t1.start();
		}
		if (this.timeout > 0 && this.onComplete) {
			this.t2 = new TimeInterval(() => {
				this.close();
				this.onComplete();
			}, this.timeout);
			this.t2.start();
		}
	};
	complete() {
		this.close();
		if (!this.onComplete)
			return;
		this.t0 = new TimeInterval(() => {
			this.t0.close();
			delete this.t0;
			this.onComplete();
		}, this.wait);
		this.t0.start();
	};
	close() {
		if (this.t0) {this.t0.close(); delete this.t0}
		if (this.t1) {this.t1.close(); delete this.t1}
		if (this.t2) {this.t2.close(); delete this.t2}
	};
};

class Service {
	constructor(service, servname, port, txt, ttl = DEFAULT_TTL) {
		this.service = service;
		this.servname = servname || server.host;
		this.port = port;
		this.ttl = ttl;
		this.txt = txt;
		this.name = this.servname + (service ? "." + this.service : "") + "." + server.domain;
		this.host = server.host + "." + server.domain;
		this.running = [];
	};
	close() {
		this.running.forEach(ms => ms.forEach(s => s.close()));
		this.running.length = 0;
	};

	probe(socks, onComplete) {
		this.run(this._probe, socks, onComplete);
	};
	_probe(sock, onComplete) {
		let message = require.weak("mdns/message");
		// the first probe with unicast
		if (!this.service)
			var msg = [
				{name: this.name, type: DNS_RRTYPE_A, addr: sock.addr, unicast: true},
				{name: sock.addr + ".in-addr.arpa", type: DNS_RRTYPE_PTR, domain: this.name, unicast: true},
			];
		else
			var msg = [{name: this.name, type: DNS_RRTYPE_SRV, target: this.host, port: this.port, unicast: true}];
		var q = message.probe(msg);
		var s = new MSocket(sock, {delay: 250, data: q, onComplete: () => {
			// the second and third probes with multicast
			msg.forEach(e => e.unicast = false);
			q = message.probe(msg);
			s.restart({delay: 250, wait: 250, interval: [250], repeat: 2, data: q, onComplete: onComplete});
		}});
		return s;
	};
	announce(socks, onComplete, flush = true, repeat = 2, unicast = false) {
		this.run(this._announce, socks, onComplete, flush, repeat, unicast);
	};
	_announce(sock, onComplete, flush, repeat, unicast) {
		let message = require.weak("mdns/message");
		if (unicast)
			flush = false;		// RFC6762 10.2
		if (this.service)
			var msg = [
				{name: this.host, type: DNS_RRTYPE_A, addr: sock.addr, ttl: this.ttl, flush: flush},
				{name: this.name, type: DNS_RRTYPE_SRV, target: this.host, port: this.port, ttl: this.ttl, flush: flush},
				{name: this.service + "." + server.domain, type: DNS_RRTYPE_PTR, domain: this.name, ttl: this.ttl}, // !!! 'flush' MUST NOT ever be set !!!
			];
		else
			var msg = [
				{name: this.name, type: DNS_RRTYPE_A, addr: sock.addr, ttl: this.ttl, flush: flush},
				{name: sock.addr + ".in-addr.arpa", type: DNS_RRTYPE_PTR, domain: this.name, ttl: this.ttl, flush: flush},
			];
		if (this.txt)
			msg.push({name: this.name, type: DNS_RRTYPE_TXT, txt: this.txt, ttl: this.ttl, flush: flush});
		var an = message.announce(msg);
		return new MSocket(sock, {repeat: repeat, interval: [1000], data: an, unicast: unicast, onComplete: onComplete});
	};
	deannounce(socks, onComplete) {
		if (!this.probed) {
			onComplete();
			return;
		}
		this.run(this._deannounce, socks, onComplete);
	};
	_deannounce(sock, onComplete) {
		let message = require.weak("mdns/message");
		if (this.service)
			var msg = [{name: this.service + "." + server.domain, type: DNS_RRTYPE_PTR, domain: this.name, ttl: 0}];
		else
			var msg = [{name: this.name, type: DNS_RRTYPE_A, addr: sock.addr, ttl: 0}];	// @@??
		var an = message.announce(msg);
		return new MSocket(sock, {repeat: 3, interval: [1000], data: an, onComplete: onComplete});
	};
	run(func, socks, onComplete, ...params) {
		var msocks = socks.map(sock => func.call(this, sock, () => {
			let i = msocks.findIndex(ms => ms.sock == sock);
			if (i >= 0)
				msocks.splice(i, 1);
			if (msocks.length == 0) {
				let i = this.running.indexOf(msocks);
				if (i >= 0)
					this.running.splice(i, 1);
				onComplete();
			}
		}, ...params));
		this.running.push(msocks);
	};
	query(socks) {
		let message = require.weak("mdns/message");
		let q = message.query(0, this.service + "." + server.domain, false, true);
		let interval = [];
		for (let i = 2; i < 60*60; i *= 2)
			interval.push(i * 1000);
		interval.push(60*60);
		this.running.push(socks.map(sock => new MSocket(sock, {repeat: MAX_INT, interval: interval, data: q, onComplete: () => {}})));	// never time out
		this.queryList = new Map();
		var state = 0, ptr, srv, adr, txt, l;
		this.answer = res => {
			if (!res) {
				if (state != 3) {
					trace(`mdns: query "${this.service}" failed: state = ${state}\n`);
					state = 0;
					return;
				}
			}
			switch (state) {
			case 0:
				if (res.type != DNS_RRTYPE_PTR)
					break;
				ptr = res;
				l = this.queryList.get(ptr.domain);
				if (res.ttl == 0) {
					if (!l)
						return;
					this.queryList.delete(ptr.domain);
					l.status = "lost";
					this.onFound(l);
					return;
				}
				this.resolv(socks, ptr.domain, DNS_RRTYPE_SRV);
				state++;
				break;
			case 1:
				if (res.type != DNS_RRTYPE_SRV)
					break;
				srv = res;
				this.resolv(socks, srv.target, DNS_RRTYPE_A);
				state++;
				break;
			case 2:
				if (res.type != DNS_RRTYPE_A)
					break;
				adr = res;
				this.resolv(socks, ptr.domain, DNS_RRTYPE_TXT);
				state++;
				break;
			case 3:
				txt = res && res.type == DNS_RRTYPE_TXT ? res : undefined;
				let q = {service: this.service, name: (srv.name.split('.'))[0], addr: adr.addr, port: srv.port};
				if (txt)
					q.keys = txt.txt;
				if (!l) {
					q.status = "found";
					this.queryList.set(ptr.domain, q);
					this.onFound(q);
				}
				else if (!this._equal(l, q)) {
					q.status = "updated";
					this.queryList.delete(ptr.domain);	// not sure if it'll be overwritten
					this.queryList.set(ptr.domain, q);
					this.onFound(q);
				}
				state = 0;
				break;
			}
		};
	};
	resolv(socks, name, type) {
		let cache = require.weak("mdns/cache");
		let srv = cache.get(name, type);
		if (srv) {
			System.sched(this.answer, srv);
			return;
		}
		let message = require.weak("mdns/message");
		let q = message.query(0, name, false, true);
		socks.forEach(sock => new MSocket(sock, {timeout: 250, data: q, onComplete: () => this.answer()}));
	};
	_equal(r1, r2) {
		if (r1 === undefined || r2 === undefined)
			return r1 === r2;
		for (let i in r1) {
			if (i == "status")
				continue;
			if (!(i in r2))
				return false;
			if (typeof r1[i] == "object")
				return this._equal(r1[i], r2[i]);
			else {
				if (r1[i] !== r2[i])
					return false;
			}
		}
		return true;
	};
	match(name) {
		if (name.startsWith("*."))
			name = name.substr(2);
		if (name.endsWith("."))
			name = name.slice(0, -1);
		if (name == this.name)
			return true;
		if (name.endsWith("." + server.domain))
			name = name.slice(0, -(server.domain.length + 1));
		if (this.service)
			return this.service == name;
		else
			return this.servname == name;
	};

	answer(res) {
		this.onFound(res);
	};
	onFound(res) {};
};

export default Service;
