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
	<patch prototype="Crypt">
		<null name="defaultRNG"/>

		<object name="x917" prototype="Crypt.rng">
			<null name="I"/>
			<null name="s"/>
			<null name="cipher"/>

			<function name="get" params="nbytes">
				var blockSize = this.cipher.blockSize;
				var r = new Chunk();
				for (var n = nbytes; n > 0; n -= blockSize) {
					var x = new Chunk(blockSize);
					this.cipher.encrypt(Crypt.bin.chunk.xor(this.I, this.s), x);
					r.append(x);
					this.cipher.encrypt(Crypt.bin.chunk.xor(x, this.I), this.s);
					x.free();
				}
				if (r.length != nbytes)
					r = r.slice(0, nbytes);
				return(r);
			</function>
		</object>
		<function name="X917" params="seed, cipher" prototype="Crypt.x917">
			if (!cipher) {
				// this is an easy setting just for convenience
				if (!seed)
					seed = new Chunk("9+Z2ZCp0HR/ZILmE7FHQJA==");	// a random key for two-key EDE (16B)
				var sha1 = new Crypt.SHA1();
				sha1.update(seed);
				var key = (sha1.close()).slice(0, 16);
				cipher = new Crypt.TDES(key);
				seed = seed.slice(16);
			}
			this.cipher = cipher;
			var d = new Chunk(this.cipher.blockSize);
			this.getTimeVariable(d);
			this.I = new Chunk(this.cipher.blockSize);
			this.cipher.encrypt(d, this.I);
			// set up the seed
			if (!seed || seed.length < cipher.blockSize) {
				seed = new Chunk(8);
				this.getTimeVariable(seed);
			}
			this.s = seed;
		</function>
	</patch>

	<program>
		Crypt.defaultRNG = new Crypt.X917();
	</program>
</package>