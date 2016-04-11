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

import {ListeningSocket} from "socket";
import TimeInterval from "timeinterval";
import Service from "mdns/service";	// more like include.. live along with the server

const BROADCAST_ADDR = "224.0.0.251";
const BROADCAST_PORT = 5353;

const DNS_RRTYPE_A = 1;		// a host address
const DNS_RRTYPE_PTR = 12;
const DNS_RRTYPE_TXT = 16;
const DNS_RRTYPE_SRV = 33;

var server = {
	domain: undefined,
	host: undefined,
	socks: [],
	services: [],

	init() {
		let Connection = require.weak("wifi");
		let interfaces = Connection.getInterfaces();
		for (let i in interfaces) {
			let nif = interfaces[i];
			if (nif.UP && nif.MULTICAST && !nif.LOOPBACK && nif.addr) {
				let s = new ListeningSocket({addr: BROADCAST_ADDR, port: BROADCAST_PORT, proto: "udp", membership: nif.addr, ttl: 255, loop: false});
				s.onData = pkt => this.handler(pkt, s);
				s.onClose = () => {};		// do nothing until stop
				s.onError = () => {		// @@ should close all services running on this socket
					s.close();
					let i = this.socks.indexOf(s);
					if (i >= 0)
						this.socks.splice(i, 1);
				};
				this.socks.push(s);
			}
		}
	},
	close() {
		this.socks.forEach(s => s.close());
		if (this.timer)
			this.timer.close();
		require.busy = false;
	},
	start(host, domain) {
		this.domain = domain;
		this.host = host;
		if (this.init) {
			this.init();
			delete this.init;
			require.busy = true;
			this.register(undefined, host, 0, undefined, 255);
		}
	},
	stop() {
		if (this.services.length > 0) {
			var running = 0;
			this.services.forEach(svc => {
				if (svc.removing)
					running++;
				else
					svc.deannounce(this.socks, () => {
						this.remove(svc);
						if (this.services.length == 0)
							this.close();
					});
			});
			if (running) {
				this.timer = new TimeInterval(() => {
					this.timer.close();
					delete this.timer;
					this.stop();
				}, 250);
				this.timer.start();
			}
		}
		else
			this.close();
	},
	register(service, servname, port, txt, ttl, i = 0) {
		if (i > 0) {
			let dot = servname.indexOf('.');
			if (dot >= 0) {
				let nn = servname.slice(0, dot);
				nn += ` (${i})`;
				var uniqname = nn + slice(dot);
			}
			else
				var uniqname = `${servname} (${i})`;
		}
		else
			var uniqname = servname;
		var svc = new Service(service, uniqname, port, txt, ttl);
		svc.onFound = res => {
			if (res) {
				if (this.socks.some(s => res.addr == s.addr)) {	// multicast_loop is false so this should not happen...
					// trace("mdns: from myself!\n");
					return;
				}
				if (this.services.some(svc => res.name == svc.name)) {
					// trace(`mdns.register: conflict! ${service} / ${servname} (${i})\n`);
					this.remove(svc);
					this.register(service, servname, port, txt, ttl, i + 1);
				}
			}
		};
		svc.probe(this.socks, () => svc.announce(this.socks, () => svc.probed = true));
		this.services.push(svc);
	},
	update(service, txt, ttl) {
		for (let svc of this.services) {
			if (svc.match(service)) {	// could be a wild card
				if (txt !== undefined)
					svc.txt = txt;
				if (ttl)
					svc.ttl = ttl;
				svc.announce(this.socks, () => {});
			}
		}
	},
	unregister(service) {
		this.services.forEach(svc => {
			if (svc.match(service))	 {	// could be a wild card
				svc.removing = true;
				svc.deannounce(this.socks, () => this.remove(svc));
			}
		});
	},
	remove(svc) {
		svc.close();
		let i = this.services.indexOf(svc);
		if (i >= 0)
			this.services.splice(i, 1);
	},
	query(service, cb) {
		if (!cb) {	// stop the query
			let svc = this.services.find(e => e.match(service) && e.querying);
			if (svc)
				this.remove(svc);
			return;
		}
		let svc = new Service(service);
		svc.onFound = res => cb(res);
		svc.query(this.socks);
		svc.querying = true;
		this.services.push(svc);
	},

	findService(name) {
		return this.services.find(e => e.match(name));
	},

	handler(pkt, sock) {
		if (!pkt)
			return;
		let message = require.weak("mdns/message");
		let res = message.parse(pkt);
		// check if it's a query for a registered service
		for (let qd of res.qds) {
			// trace(`mdns: ${Date()}: query: ${qd.name}\n`);
			let svc = this.findService(qd.name);
			if (svc) {
				if (svc.service) {
					// `Known answer suppression'
					if (res.ans.some(an => an.type == DNS_RRTYPE_PTR && an.domain == svc.name)) {
						// trace(`mdns: known-name: ${svc.domain}\n`);
						return;
					}
				}
				// trace(`mdns: responding to ${svc.name}\n`);
				if (svc.probed)
					svc.announce([sock], () => {}, false, 0, qd.unicast);
			}
		}
		// keep it in the cache, anyway
		let cache = require.weak("mdns/cache");
		cache.put(res);
		// check if it's a response to the probe or query
		for (let an of res.ans) {
			// trace(`mdns: answer: ${an.name}\n`);
			this.services.forEach(svc => {
				if (svc.match(an.name))
					svc.answer(an);
			});
		}
	},
};

export default server;
