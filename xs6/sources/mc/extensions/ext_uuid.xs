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
<package>
	<program>
		var Connection = require("wifi");
	</program>
	<object name="uuid">
		<number name="clockSequence" value="0"/>
		<function name="getMacAddress">
			var interfaces = Connection.getInterfaces();
			for (var i in interfaces) {
				if (interfaces[i].mac)
					return interfaces[i].mac;
			}
			// needs to return something...
			return new Chunk(6);
		</function>
		<function name="create_v1">
			if (uuid.clockSequence == 0)
				uuid.clockSequence = Math.random() * 32767;
			var t = (new Date()).getTime() * 10;	// in 100-nanosec
			this.time_low = t & 0xffffffff;
			this.time_mid = (t >> 32) & 0xffff;
			this.time_high = ((t >> 48) & 0x0fff) | (1 << 12);
			this.clock_seq = uuid.clockSequence++ | 0x8000;
			this.node = Connection.mac;
		</function>
		<function name="toHex" params="v, columns">
			if (typeof v == "number")
				var x = Number(v).toString(16);
			else {	// assume it's a chunk
				var x = "", l = v.length, s;
				for (var i = 0; i < l; i++) {
					s = Number(v.peek(i)).toString(16);
					if (s.length < 2) s = "0" + s;
					x += s;
				}
			}
			while (x.length < columns)
				x = "0" + x;
			return x;
		</function>
		<function name="toString" params="toupper">
			var s = "";
			s = this.toHex(this.time_low, 8) + "-" +
			    this.toHex(this.time_mid, 4) + "-" +
			    this.toHex(this.time_high, 4) + "-" +
			    this.toHex(this.clock_seq, 4) + "-" +
			    this.node;
			if (toupper)
				s = s.toUpperCase();
			return s;
		</function>

		<string name="__uuid__" value=""/>
		<function name="getUUID">
			if (uuid.__uuid__ == "") {
				var Environment = require("env");
				var env = new Environment();
				var v = env.get("UUID");
				if (!v) {
					v = (new UUID()).toString();
					env.set("UUID", v);
					env.save();
				}
				uuid.__uuid__ = v;
			}
			return uuid.__uuid__;
		</function>
	</object>
	<function name="UUID" prototype="uuid">
		this.create_v1();
	</function>
</package>
