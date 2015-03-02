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
	<patch prototype="Crypt">
		<object name="hmac">
			<null name="ipad" script="false"/>
			<null name="opad" script="false"/>
			<null name="h" script="false"/>
			<null name="hashLen" script="false"/>

			<function name="init" params="key" script="false">
				var h = this.h;
				if (key.length > h.blockSize) {
					// truncate the key
					h.reset();
					h.update(key);
					key = h.close();
				}
				var n = h.blockSize;
				var l = key.length;
				this.ipad = new Chunk(n);
				this.opad = new Chunk(n);
				if (l > n)
					l = n;
				var i = 0;
				for (; i < l; i++) {
					var c = key.peek(i);
					this.ipad.poke(i, c ^ 0x36);
					this.opad.poke(i, c ^ 0x5c);
				}
				for (; i < n; i++) {
					this.ipad.poke(i, 0x36);
					this.opad.poke(i, 0x5c);
				}
				h.reset();
				h.update(this.ipad);
			</function>

			<function name="update" params="text">
				this.h.update(text);
			</function>

			<function name="close" params="">
				var h = this.h;
				var digest = h.close();
				h.reset();
				h.update(this.opad);
				h.update(digest);
				return(h.close());
			</function>

			<function name="reset" params="">
				this.h.reset();
				this.h.update(this.ipad);
			</function>

			<function name="sign" params="H">
				this.update(H);
				var sig = this.close();
				if (this.hashLen)
					sig = sig.slice(0, this.hashLen);
				return(sig);
				// if there is possibility to be called sign or verify again, needs reset() here.
			</function>

			<function name="verify" params="H, sig">
				this.update(H);
				var t = this.close();
				if (this.hashLen)
					return(t.ncomp(sig, this.hashLen) == 0);
				else
					return(t.comp(sig) == 0);
			</function>
		</object>
		<function name="HMAC" params="h, key, bitLen" prototype="Crypt.hmac">
			this.h = h;
			this.init(key);
			if (bitLen) {
				if (bitLen % 8)
					throw new Crypt.Error(Crypt.error.kCryptParameterError);
				this.hashLen = bitLen / 8;	// in byte
			}
		</function>
	</patch>
</package>