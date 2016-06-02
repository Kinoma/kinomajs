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
import Environment from "env";
import Bin from "bin";

export default class CacheManager {
	constructor() {
		this.cache = new Environment("ssl", true, true);	// both autosave / encryption = true
	};
	getByHost(hostname) {
		var cache = this.cache.get(hostname);
		if (cache) {
			var o = JSON.parse(cache);
			return {id: Bin.decode(o.id), secret: Bin.decode(o.secret)};
		}
	};
	getByID(id) {
		var eid = Bin.encode(id);
		for (let name of this.cache) {
			var cache = this.cache.get(name);
			var o = JSON.parse(cache);
			if (o && typeof o == "object" && o.id == eid)
				return {id: id, secret: Bin.decode(o.secret), name: name};
		}
	};
	saveSession(host, id, secret) {
		this.cache.set(host, JSON.stringify({id: Bin.encode(id), secret: Bin.encode(secret)}));
	};
	deleteSessionID(id) {
		var o = this.getByID(id);
		if (o)
			this.cache.set(o.name);
	};
};
