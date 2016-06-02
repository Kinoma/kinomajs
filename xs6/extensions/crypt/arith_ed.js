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
import Arith from "arith";

export default class Ed @ "xs_ed_destructor" {
	constructor(q, d) {
		var Module = require.weak("Arith").Module;
		this.q = q;
		this.d = d;
		this.mod = new Module(null, this.q);
		this._proto_int = Arith.Integer.prototype;
		this._proto_ecp = Arith.ECPoint.prototype;
		this._init();
	};
	add(P, Q) @ "xs_ed_add";
	mul(P, k) @ "xs_ed_mul";
	_init() @ "xs_ed_init";
};
