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

export default class EC @ "xs_ec_destructor" {
	constructor(a, b, m) {
		this.a = a;
		this.b = b;
		this.m = m;
		this._proto_ecpoint = Arith.ECPoint.prototype;
		this._proto_int = Arith.Integer.prototype;
		this._init();
	};
	inv(a) @ "xs_ec_inv";
	add(a, b) @ "xs_ec_add";
	mul(a, k) @ "xs_ec_mul";
	mul2(a1, k1, a2, k2) @ "xs_ec_mul2";
	_init() @ "xs_ec_init";
};
