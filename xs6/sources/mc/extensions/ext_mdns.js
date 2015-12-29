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
class Service @ "xs_mdns_service_destructor" {
	constructor(fqst, port, name) @ "xs_mdns_service_constructor";
	_configure(txt, sep) @ "xs_mdns_service_configure";
	configure(keys) {
		var arr = [];
		for (var i in keys)
			arr.push(i + "=" + keys[i]);
		this._configure(arr.join('|'), '|');
		mdns.reannounce();
	}
	close() @ "xs_mdns_service_close";
};

var mdns = {
	_services: {},

	start(domain, hostname) @ "xs_mdns_start",
	_stop() @ "xs_mdns_stop",
	stop() {
		if (Object.keys(this._services).length == 0)
			this._stop();
	},

	newService(fqst, port, name) {
		var svc = new Service(fqst, port, name);
		this._services[fqst] = svc;
		this._newService(svc);
		return svc;
	},
	_newService(svc) @ "xs_mdns_newService",
	getService(fqst) {
		return this._services[fqst];
	},
	removeService(fqst) {
		var svc = this._services[fqst];
		if (svc) {
			this._removeService(svc);
			svc.close();
			delete this._services[fqst];	// instance will be GC'ed
		}
	},
	_removeService(svc) @ "xs_mdns_removeService",
	reannounce() @ "xs_mdns_reannounce",

	// query APIs
	query(fqst) @ "xs_mdns_query",
	// callbacks
	onFound(fqst, addr, port, keys, name) {},
	onLost(fqst, addr, port, keys, name) {},
};
export default mdns;
