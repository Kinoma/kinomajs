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

export default class ECPoint @ "xs_ecpoint_destructor" {
	constructor(x, y) {
		this._proto_int = Arith.Integer.prototype;
		this._init(x, y);
	};
	_init(x, y) @ "xs_ecpoint_init";
	get identity() @ "xs_ecpoint_getIdentity";
	get x() @ "xs_ecpoint_getX";
	get y() @ "xs_ecpoint_getY";
	set x(x) @ "xs_ecpoint_setX";
	set y(y) @ "xs_ecpoint_setY";
	isZero() {
		return this.identify;
	};
	toString() {
		return this.x + "," + this.y;
	}
	static serialize(o) {
		o.toString();
	};
	static parse(txt) {
		var a = txt.split(",");
		if (a.length == 2) {
			var Integer = require.weak("Arith").Integer;
			return new ECPoint(new Integer(a[0]), new Integer(a[1]));
		}
	};
};
