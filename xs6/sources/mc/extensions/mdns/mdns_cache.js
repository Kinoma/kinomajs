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

const MAX_CACHE = 24;

var cache = {
	data: [],

	get(name, type) {
		let ctime = Date.now() / 1000;
		let data = this.data;
		if (name.endsWith("."))
			name = name.slice(0, -1);
		for (let i = 0; i < data.length; i++) {
			if (data[i].name == name && (type !== undefined ? data[i].type == type : true)) {
				if (data[i].lifetime < ctime) {	// already dead
					this.data.splice(i, 1);
					return null;
				}
				return data[i];
			}
		}
	},
	_push(arr) {
		for (let a of arr) {
			if (a.name.endsWith("."))
				a.name = a.name.slice(0, -1);
			this.remove(a.name, a.type);	// always replace it with the new one
			if (a.ttl == 0)
				continue;		// should not be cached
			if (this.data.length >= MAX_CACHE)
				this.data.shift();
			a.lifetime = Date.now() / 1000 + a.ttl;
			this.data.push(a);
		}
	},
	put(res) {
		if (!res)
			return;
		this._push(res.ans);
		this._push(res.ars);	// some implementations use the additional record?
		if (this.data.length > 0)
			require.busy = true;
	},
	remove(name, type) {
		let data = this.data;
		for (let i = 0; i < data.length; i++) {
			if (data[i].name == name && data[i].type == type) {
				this.data.splice(i, 1);
				if (this.data.length == 0)
					require.busy = false;
			}
		}
	},
	close() {
		this.data.length = 0;
		require.busy = false;
	},
};

export default cache;
