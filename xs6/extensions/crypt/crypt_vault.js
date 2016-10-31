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

import Crypt from "crypt";
import Files from "files";

const nonce = "vault";

export default class Vault extends Map {
	constructor(path, key = undefined, autosave = true) {
		super();
		this.path = path;
		this.key = key;
		this.autosave = autosave;
		if (path)
			this.load();
	};
	set(key, value) {
		super.set(key, value);
		if (this.autosave)
			this.save();
	};
	save() {
		let s = JSON.stringify([...this]);
		if (this.key) {
			let aead = new Crypt.AEAD(this.key, nonce, this.path, Crypt.Chacha, Crypt.Poly1305);
			s = aead.process(s, true);
		}
		Files.write(this.path, s);
	};
	load() {
		let data = Files.read(this.path);
		if (!data)
			return;
		let s = String.fromArrayBuffer(data);
		if (this.key) {
			let aead = new Crypt.AEAD(this.key, nonce, this.path, Crypt.Chacha, Crypt.Poly1305);
			s = aead.process(s, false);
			if (!s)
				return;
		}
		this.clear();
		let o = JSON.parse(s);
		for (let k of Object.keys(o))
			this.set(k, o[k]);
	};
};
