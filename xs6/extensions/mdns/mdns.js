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

import server from "mdns/server";

var mdns = {
	start(host, domain = "local") {
		if (!host)
			host = require.weak("system").hostname;
		if (host.endsWith(".local"))
			host = host.slice(0, -6);
		server.start(host, domain);
	},
	stop() {
		let cache = require.weak("mdns/cache");
		server.stop();
		cache.close();
	},
	add(service, servname, port, txt = undefined, ttl = 255) {
		if (txt && typeof txt == 'string')
			txt = this.parse(txt);
		server.register(service, servname, port, txt, ttl);
	},
	update(service, txt = undefined, ttl = undefined) {	// set txt = null if you want to unset the TXT record
		if (txt && typeof txt == 'string')
			txt = this.parse(txt);
		server.update(service, txt, ttl);
	},
	remove(service, servname = "*") {
		server.unregister(servname + "." + service);
	},
	query(service, cb = res => console.log("query:", res)) {
		server.query(service, cb);
	},
	// use unregister to stop query
	resolv(name, cb) {
		let resolver = require.weak("mdns/resolver");
		resolver(name, cb || addr => console.log("resolved: " + addr));
	},
	dump() {
		let cache = require.weak("mdns/cache");
		cache.data.forEach(res => console.log(res));
	},
	parse(txt) {
		txt = {};
		txt.split(':').forEach(e => {
			let a = e.split('=');
			if (a.length > 1)
				txt[a[0]] = a[1];
		});
		return txt;
	},
	get services() {
		return server.services;
	},
};

export default mdns;
