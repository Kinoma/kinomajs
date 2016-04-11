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
export default {
	create() {
		let Connection = require.weak("wifi");
		let random = (Math.random() * 32767) | 0;
		let now = (Date.now() * 10) | 0;	// in 100-nanosec, as integer
//@@ not enough precision to make this work!
		return [	this.toHex(now & 0xffffffff, 8),
					this.toHex((now >> 32) & 0xffff, 4),
					this.toHex(((now >> 48) & 0x0fff) | (1 << 12), 4),
					this.toHex(random | 0x8000, 4),		// clockSequence - not persistent - could write to env
					Connection.mac
				].join("-");
	},
	toHex(v, length) {
		v = Math.abs(v).toString(16);
		while (v.length < length)
			v = "0" + v;
		return v;
	},
	get(name) {
		let Environment = require.weak("env");
		let env = new Environment();
		name = "UUID" + (name ? name : "");
		let v = env.get(name);
		if (!v) {
			v = this.create();
			env.set(name, v);
			env.save();
		}
		return v;
	}
};
