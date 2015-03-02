<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package script="true">
	<program c="xs_Arith_init"/>

	<object name="Arith">
		<object name="z" c="xs_z_destructor">
			<function name="add" params="a, b" c="xs_z_add"/>
			<function name="sub" params="a, b" c="xs_z_sub"/>
			<function name="mul" params="a, b" c="xs_z_mul"/>
			<function name="div2" params="a, b" c="xs_z_div2"/>
			<function name="div" params="a, b" c="xs_z_div"/>
			<function name="mod" params="a, b" c="xs_z_mod"/>
			<function name="square" params="a" c="xs_z_square"/>
			<function name="xor" params="a, b" c="xs_z_xor"/>
			<function name="or" params="a, b" c="xs_z_or"/>
			<function name="and" params="a, b" c="xs_z_and"/>
			<function name="lsl" params="a, b" c="xs_z_lsl"/>
			<function name="lsr" params="a, b" c="xs_z_lsr"/>
			<function name="_init" params="" c="xs_z_init"/>
			<function name="inc" params="a, d">
				return this.add(a, new Arith.Integer(d));
			</function>
		</object>
		<function name="Z" params="" prototype="Arith.z">
			this._init();
		</function>

		<object name="module" c="xs_mod_destructor">
			<null name="z"/>
			<null name="m"/>
			<function name="add" params="a, b" c="xs_mod_add"/>
			<function name="inv" params="a" c="xs_mod_inv"/>
			<function name="sub" params="a, b" c="xs_mod_sub"/>
			<function name="mul" params="a, b" c="xs_mod_mul"/>
			<function name="square" params="a" c="xs_mod_square"/>
			<function name="mulinv" params="a" c="xs_mod_mulinv"/>
			<function name="exp" params="a, e" c="xs_mod_exp"/>
			<function name="exp2" params="a1, e1, a2, e2" c="xs_mod_exp2"/>
			<function name="mod" params="a" c="xs_mod_mod"/>
			<function name="_init" params="options" c="xs_mod_init"/>
		</object>
		<function name="Module" params="z, m, options" prototype="Arith.module">
			this.z = z;
			this.m = m;
			this._init(options);
		</function>

		<!--
		  --  arithmetics objects
		  -->
		<object name="integer" c="xs_integer_destructor">
			<function name="toChunk" params="minBytes, signess" c="xs_integer_toChunk"/>
			<function name="toString" params="radix" c="xs_integer_toString"/>
			<function name="toNumber" params="" c="xs_integer_toNumber"/>
			<function name="negate" params="" c="xs_integer_negate"/>
			<function name="isZero" params="" c="xs_integer_isZero"/>
			<function name="isNaN" params="" c="xs_integer_isNaN"/>
			<function name="comp" params="a" c="xs_integer_comp"/>
			<function name="sign" params="" c="xs_integer_sign"/>
			<function name="sizeof" params="" c="xs_integer_sizeof"/>
			<function name="setNumber" params="n" c="xs_integer_setNumber"/>
			<function name="setString" params="string" c="xs_integer_setString"/>
			<function name="setChunk" params="chunk" c="xs_integer_setChunk"/>
			<function name="setInteger" params="int" c="xs_integer_setInteger"/>
			<function name="setNaN" params="" c="xs_integer_setNaN"/>
			<function name="free" params="" c="xs_integer_free"/>
			<function name="_init" params="" c="xs_integer_init"/>
			<function name="serialize" params="o">
				if (o instanceof Arith.Integer)
					return (o.toChunk(0, true)).toString();	// make it a base64 encoded string, not using this.toString, because of efficiency
			</function>
			<function name="parse" params="txt">
				if (typeof txt == "string" && txt)
					return new Arith.Integer(new Chunk(txt), true);
				else
					return new Arith.Integer();	// NaN
			</function>
		</object>
		<function name="Integer" params="a, opt" prototype="Arith.integer">
			this._init();
			switch (typeof a) {
			case "undefined":
				this.setNaN();
				break;
			case "number":
				this.setNumber(a);
				break;
			case "string":
				this.setString(a);
				break;
			case "object":
				if (a instanceof Chunk)
					this.setChunk(a, opt);
				else if (a instanceof Arith.Integer)
					this.setInteger(a);
				else
					throw new Crypt.Error(-7, "invalid parameter in Integer");
				break;
			default:
				throw new Crypt.Error(-7, "invalid parameter in Integer");
				break;
			}
		</function>

		<object name="vector">
			<array name="vector" contents="Arith.integer"/>
			<function name="toString" params="">
				return this.vector.serialize(this);
			</function>
			<function name="serialize" params="o">
				return o.vector.join(",");	// same as toString()
			</function>
			<function name="parse" params="txt">
				var a = txt.split(",");
				var v = new Arith.Vector();
				for (var i = 0; i < a.length; i++)
					v.vector.push(new Arith.Integer(a[i]));
				return v;
			</function>
		</object>
		<function name="Vector" params="a" prototype="Arith.vector">
			this.vector = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++)
				this.vector[i] = arguments[i];
		</function>

		<object name="pair" prototype="Arith.vector">
			<function name="get q" params="">
				return this.vector[0];
			</function>
			<function name="get r" params="">
				return this.vector[1];
			</function>
			<function name="set q" params="q">
				this.vector[0] = q;
			</function>
			<function name="set r" params="r">
				this.vector[1] = r;
			</function>
		</object>
		<function name="Pair" params="q, r" prototype="Arith.pair">
			Arith.Vector.call(this, q, r);
		</function>
	</object>
</package>