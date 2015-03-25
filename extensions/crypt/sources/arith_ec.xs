<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<package script="true">
	<patch prototype="Arith">
		<object name="ecpoint" c="xs_ecpoint_destructor">
			<function name="get identity" c="xs_ecpoint_getIdentity"/>
			<function name="get x" c="xs_ecpoint_getX"/>
			<function name="get y" c="xs_ecpoint_getY"/>
			<function name="set x" c="xs_ecpoint_setX"/>
			<function name="set y" c="xs_ecpoint_setY"/>
			<function name="free" params="" c="xs_ecpoint_free"/>
			<function name="_init" params="x, y" c="xs_ecpoint_init"/>
			<function name="isZero" params="">
				return this.identity;
			</function>
			<function name="toString" params="">
				return this.x + "," + this.y;
			</function>
			<function name="serialize" params="o">
				if (o instanceof Arith.ECPoint)
					return o.toString();
			</function>
			<function name="parse" params="txt">
				if (typeof txt == "string" && txt) {
					var a = txt.split(",");
					if (a.length == 2)
						return new Arith.ECPoint(new Arith.Integer(a[0]), new Arith.Integer(a[1]));
				}
			</function>
		</object>
		<function name="ECPoint" params="x, y" prototype="Arith.ecpoint">
			this._init(x, y);
		</function>

		<object name="ec" c="xs_ec_destructor">
			<null name="a"/>
			<null name="b"/>
			<null name="m"/>
			<function name="inv" params="a" c="xs_ec_inv"/>
			<function name="add" params="a, b" c="xs_ec_add"/>
			<function name="mul" params="a, k" c="xs_ec_mul"/>
			<function name="mul2" params="a1, k1, a2, k2" c="xs_ec_mul2"/>
			<function name="_init" params="" c="xs_ec_init"/>
		</object>
		<function name="EC" params="a, b, m" prototype="Arith.ec">
			this.a = a;
			this.b = b;
			this.m = m;
			this._init();
		</function>
	</patch>
</package>