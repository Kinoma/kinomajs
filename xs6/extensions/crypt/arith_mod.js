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

export default class Module @ "xs_mod_destructor" {
	constructor(z, m) {
		this._proto_int = Arith.Integer.prototype;
		this.z = z || new Arith.Z();
		this.m = m;
		this._init(this.z, m);
	};
	add(a, b) @ "xs_mod_add";
	inv(a) @ "xs_mod_inv";
	sub(a, b) @ "xs_mod_sub";
	mul(a, b) @ "xs_mod_mul";
	square(a) @ "xs_mod_square";
	mulinv(a) @ "xs_mod_mulinv";
	exp(a, e) @ "xs_mod_exp";
	exp2(a1, e1, a2, e2) @ "xs_mod_exp2";
	mod(a) @ "xs_mod_mod";
	_init(z, m) @ "xs_mod_init";
};
