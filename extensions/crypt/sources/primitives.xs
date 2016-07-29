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
		<object name="digest" c="xs_digest_destructor">
			<function name="update" params="chunk" c="xs_digest_update"/>
			<function name="close" params="" c="xs_digest_close"/>
			<function name="reset" params="" c="xs_digest_reset"/>
			<function name="get blockSize" c="xs_digest_getBlockSize"/>
			<function name="get outputSize" c="xs_digest_getOutputSize"/>
			<function name="process">
				this.reset();
				for (var i = 0; i < arguments.length; i++)
					this.update(arguments[i]);
				return this.close();
			</function>
		</object>
		<function name="SHA1" params="" prototype="Crypt.digest" c="xs_sha1_constructor"/>
		<function name="SHA256" params="" prototype="Crypt.digest" c="xs_sha256_constructor"/>
		<function name="SHA512" params="" prototype="Crypt.digest" c="xs_sha512_constructor"/>
		<function name="SHA384" params="" prototype="Crypt.digest" c="xs_sha384_constructor"/>
		<function name="SHA224" params="" prototype="Crypt.digest" c="xs_sha224_constructor"/>
		<function name="MD5" params ="" prototype="Crypt.digest" c="xs_md5_constructor"/>
		<function name="GHASH" params="h, aad" prototype="Crypt.digest" c="xs_ghash_constructor"/>

		<object name="blockCipher" c="xs_blockCipher_destructor">
			<function name="encrypt" params="inChunk, outChunk" c="xs_blockCipher_encrypt"/>
			<function name="decrypt" params="inChunk, outChunk" c="xs_blockCipher_decrypt"/>
			<function name="get keySize" c="xs_blockCipher_getKeySize"/>
			<function name="get blockSize" c="xs_blockCipher_getBlockSize"/>
		</object>
		<function name="DES" params="key" prototype="Crypt.blockCipher" c="xs_des_constructor"/>
		<function name="TDES" params="key, keySize" prototype="Crypt.blockCipher" c="xs_tdes_constructor"/>
		<function name="AES" params="key, keySize, blockSize" prototype="Crypt.blockCipher" c="xs_aes_constructor"/>

		<object name="encryptionMode" c="xs_encryptionMode_destructor">
			<function name="encrypt" params="inChunk, outChunk" c="xs_encryptionMode_encrypt"/>
			<function name="decrypt" params="inChunk, outChunk" c="xs_encryptionMode_decrypt"/>
			<function name="setIV" params="chunk" c="xs_encryptionMode_setIV"/>
			<function name="getIV" params="" c="xs_encryptionMode_getIV"/>
		</object>
		<function name="CBC" params="blockCipher, iv, padLen" prototype="Crypt.encryptionMode" c="xs_cbc_constructor"/>
		<function name="CTR" params="blockCipher, iv" prototype="Crypt.encryptionMode" c="xs_ctr_constructor"/>
		<function name="ECB" params="blockCipher, padLen" prototype="Crypt.encryptionMode" c="xs_ecb_constructor"/>

		<object name="streamCipher" c="xs_streamCipher_destructor">
			<function name="process" params="inChunk, outChunk" c="xs_streamCipher_process"/>
			<function name="get keySize" c="xs_streamCipher_getKeySize"/>
			<function name="encrypt" params="inChunk, outChunk">
				return this.process(inChunk, outChunk);
			</function>
			<function name="decrypt" params="inChunk, outChunk">
				return this.process(inChunk, outChunk);
			</function>
		</object>
		<function name="RC4" params="key, keySize" prototype="Crypt.streamCipher" c="xs_rc4_constructor"/>

		<object name="gcm">
			<function name="init" params="iv, aad">
				let h = new Chunk(this.block.blockSize);
				this.block.encrypt(new Chunk(this.block.blockSize), h);
				this.ghash = new Crypt.GHASH(h, aad);
				if (iv.length == 12) {
					let one = new Chunk(4);
					one.poke32(0, 1);
					iv = new Chunk(iv);
					iv.append(one);
				}
				else {
					let ghash = new Crypt.GHASH(h);
					iv = ghash.process(iv);
				}
				this.y0 = iv;
				// start with y1
				let y1 = new Arith.Integer(iv);
				let z = new Arith.Z();
				y1 = z.inc(y1, 1);
				this.ctr.setIV(y1.toChunk());
			</function>
			<function name="encrypt" params="data, buf">
				buf = this.ctr.encrypt(data, buf);
				this.ghash.update(buf);
				return buf;
			</function>
			<function name="decrypt" params="data, buf">
				this.ghash.update(data);
				return this.ctr.decrypt(data, buf);
			</function>
			<function name="close">
				let t = this.ghash.close();
				let result = new Chunk(this.y0.length);
				this.block.encrypt(this.y0, result);
				return Crypt.bin.chunk.xor(t, result);
			</function>
			<function name="process" params="data, buf, iv, aad, encFlag">
				if (encFlag) {
					this.init(iv, aad);
					buf = this.encrypt(data, buf);
					let tag = this.close();
					if (tag.length > this.tagLength)
						tag = tag.slice(0, this.tagLength);
					buf.append(tag);
					return buf;
				}
				else {
					this.init(iv, aad);
					buf = this.decrypt(data.slice(0, data.length - this.tagLength), buf);
					let tag = this.close();
					if (tag.ncomp(data.slice(data.length - this.tagLength), this.tagLength) == 0)
						return buf;
				}
			</function>
		</object>
		<function name="GCM" params="cipher, tagLength" prototype="Crypt.gcm">
			this.block = cipher;
			this.ctr = new Crypt.CTR(this.block);
			this.tagLength = tagLength || 16;
		</function>

		<object name="rng">
			<!-- prototype -->
			<function name="get" params="nbytes"/>
			<!-- for convenience of RNGs -->
			<function name="getTimeVariable" params="chunk" c="xs_rng_getTimeVariable"/>
		</object>
	</patch>
</package>
