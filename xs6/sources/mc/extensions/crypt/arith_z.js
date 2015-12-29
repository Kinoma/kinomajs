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
import Arith from "arith";

export default class Z @ "xs_z_destructor" {
	constructor() {
		this._proto_int = Arith.Integer.prototype;
		this._init();
	};
	add(a, b) @ "xs_z_add";
	sub(a, b) @ "xs_z_sub";
	mul(a, b) @ "xs_z_mul";
	div2(a, b) @ "xs_z_div2";
	div(a, b) @ "xs_z_div";
	mod(a, b) @ "xs_z_mod";
	square(a) @ "xs_z_square";
	xor(a, b) @ "xs_z_xor";
	or(a, b) @ "xs_z_or";
	and(a, b) @ "xs_z_and";
	lsl(a, b) @ "xs_z_lsl";
	lsr(a, b) @ "xs_z_lsr";
	_init() @ "xs_z_init";
	inc(a, d) {
		return this.add(a, new Arith.Integer(d));
	};
	toString(i, radix) @ "xs_z_toString";
	toInteger(digits, radix) @ "xs_z_toInteger";
};
