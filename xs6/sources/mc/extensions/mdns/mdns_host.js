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

class Service @ "xs_mdns_destructor" {
	constructor(params) @ "xs_mdns_constructor";
	close() @ "xs_mdns_close";
	update(txt) @ "xs_mdns_update";
};

var mdns = {
	start(host, domain = "local") {
		if (!host)
			host = require.weak("system").hostname;
		if (host.endsWith(".local"))
			host = host.slice(0, -6);
		this.host = host;
		this.domain = domain;
		require.busy = true;
	},
	stop() {
		this.services.forEach((svc, key) => svc.close());
		require.busy = false;
	},
	add(service, servname, port, txt) {
		let svc = new Service({type: service, name: servname, port: port, key: txt, domain: this.domain, host: this.host});
		this.services.set(service, svc);
	},
	update(service, txt) {
		let svc = this.services.get(service);
		if (svc)
			svc.update(txt);
	},
	remove(service) {
		let svc = this.services.get(service);
		if (svc) {
			svc.close();
			this.services.delete(service);
		}
	},
	query(service, cb = res => console.log("query:", res)) {
		if (cb) {
			let svc = new Service({type: service, query: true});
			svc.callback = (fullname, name, target, port, txt) => {
				if (fullname)
					this.resolv(target, addr => cb({service: service, name: name, addr: addr, port: port, keys: txt, status: "found"}));
				else
					cb({service: service, name: name, status: "lost"});
			};
			this.services.set(service, svc);
		}
		else
			this.remove(service);
	},
	resolv(name, cb) {
		let resolver = require.weak("mdns/resolver");
		resolver(name, cb || addr => console.log("resolved: " + addr));
	},
};

mdns.services = new Map();

export default mdns;
