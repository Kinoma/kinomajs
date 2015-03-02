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
<package>
	<patch prototype="Crypt">
		<object name="pem"/>
	</patch>

	<patch prototype="Crypt.pem">
		<object name="header">
			<string name="name"/>
			<string name="value"/>
		</object>
		<function name="Header" params="name, value" prototype="Crypt.pem.header">
			if (name)
				this.name = name;
			if (value)
				this.value = value;
		</function>

		<object name="message">
			<string name="keyword"/>
			<array name="headers" contents="Crypt.pem.header"/>
			<chunk name="body"/>
		</object>
		<function name="Message" params="keyword" prototype="Crypt.pem.message">
			if (keyword)
				this.keyword = keyword;
		</function>

		<function name="puts" params="c, s" script="false">
			var b = new Chunk(s.length);
			for (var i = 0; i < s.length; i++)
				b.poke(i, s[i]);
			c.append(b);
			b.free();
		</function>

		<function name="putb" params="c, b" script="false">
			var maxline = 80;
			var s = b.toString();
			for (var offset = 0, ss; ss = s.slice(offset, offset + maxline); offset += maxline)
				this.puts(c, ss + "\n");
		</function>

		<function name="encodeMessage" params="r, m" script="false">
			this.puts(r, "-----BEGIN " + m.keyword + "-----\n");
			if (m.hasOwnProperty("headers") && m.headers) {
				for (var i in m.headers)
					this.puts(r, i + ": " + m.headers[i] + "\n");
				this.puts(r, "\n");
			}
			this.putb(r, m.body);
			this.puts(r, "-----END " + m.keyword + "-----\n");
		</function>

		<function name="encode" params="msgs">
			var r = new Chunk();
			for (var i = 0; i < msgs.length; i++) {
				this.encodeMessage(r, msgs[i]);
			}
			return(r);
		</function>

		<function name="decode" params="chunk" c="xs_pem_decode"/>
	</patch>
</package>