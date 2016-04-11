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

import cache from "mdns/cache";
import {Socket} from "socket";
import TimeInterval from "timeinterval";

const BROADCAST_ADDR = "224.0.0.251";
const BROADCAST_PORT = 5353;
const DNS_PORT = 53;
const DEFAULT_DNS_SERVER = "208.67.222.222";	// resolver1.opendns.com

const DEFAULT_TIMEOUT = 1000;
const DEFAULT_RETRY_COUNT = 3;

/*
 * 5.1 one-shot queries in RFC 6762
 */

class MSocket {
	constructor(params) {
		let Connection = require.weak("wifi");
		let interfaces = Connection.getInterfaces();
		this.socks = [];
		if (params.multicast) {
			for (let i in interfaces) {
				let nif = interfaces[i];
				if (nif.UP && nif.MULTICAST && !nif.LOOPBACK && nif.addr) {
					let s = new Socket({host: BROADCAST_ADDR, port: BROADCAST_PORT, proto: "udp", multicast: nif.addr, ttl: 255});
					s.onConnect = () => s.send(this.data);
					s.onMessage = n => this.handler(s, n);
					this.socks.push(s);
				}
			}
		}
		else {
			for (let i in interfaces) {
				let nif = interfaces[i];
				if (nif.dns) {
					let s = new Socket({host: nif.dns, port: DNS_PORT, proto: "udp"});
					s.onConnect = () => s.send(this.data);
					s.onMessage = n => this.handler(s, n);
					this.socks.push(s);
				}
			}
		}
		this.data = params.data;
		this.onData = params.onData;
		var retry = params.retry || DEFAULT_RETRY_COUNT;
		this.timer = new TimeInterval(() => {
			if (--retry > 0)
				this.send();
			else {
				this.close();
				this.onData();
			}
		}, params.timeout || DEFAULT_TIMEOUT);
		this.timer.start();
	};
	close() {
		if (this.timer) {
			this.timer.close();
			delete this.timer;
		}
		this.socks.forEach(s => s.close());
		this.socks.length = 0;
	};

	send() {
		this.socks.forEach(s => s.send(this.data));
	};
	handler(s, n) {
		if (n > 0) {
			this.onData(s.recv(n));
			this.close();
		}
	};
};

var socks = [];

function remove(s)
{
	let i = socks.indexOf(s);
	if (i >= 0)
		socks.splice(i, 1);
	if (socks.length == 0)
		require.busy = false;
}

function _resolv(name, cb)
{
	let message = require.weak("mdns/message");

	// shortcut for localhost
	if (name == "localhost")
		name = "127.0.0.1";
	// check if the name is already in the dot notation
	if (Socket.aton(name))
		return name;
	// check if the name is in the cache
	let res = cache.get(name);
	if (res && res.addr)
		return res.addr;
	// ask the name server or broadcast to the local network
	if (name.endsWith(".local."))
		name = name.slice(0, -1);
	let multicast = name.endsWith(".local");
	let q = message.query(0, name, multicast);		// not using the ID
	let s = new MSocket({multicast: multicast, data: q, onData: pkt => {
		let addr;
		if (pkt) {
			let res = message.parse(pkt);
			if (res.rcode == 0) {
				for (let i = 0, ans = res.ans; i < ans.length; i++) {
					if (ans[i].addr) {
						addr = ans[i].addr;
						break;
					}
				}
				cache.put(res);
			}
		}
		cb(addr);
		remove(s);
	}});
	socks.push(s);
	require.busy = true;
};

function resolv(name, cb)
{
	var res = _resolv(name, cb);
	if (res) {
		let System = require.weak("system");
		System.sched(cb, res);	// call the callback in the next event cycle
	}
}

export default resolv;
