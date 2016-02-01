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
		</object>
		<function name="SHA1" params="" prototype="Crypt.digest" c="xs_sha1_constructor"/>
		<function name="SHA256" params="" prototype="Crypt.digest" c="xs_sha256_constructor"/>
		<function name="SHA512" params="" prototype="Crypt.digest" c="xs_sha512_constructor"/>
		<function name="SHA384" params="" prototype="Crypt.digest" c="xs_sha384_constructor"/>
		<function name="SHA224" params="" prototype="Crypt.digest" c="xs_sha224_constructor"/>
		<function name="MD5" params ="" prototype="Crypt.digest" c="xs_md5_constructor"/>

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

		<object name="rng">
			<!-- prototype -->
			<function name="get" params="nbytes"/>
			<!-- for convenience of RNGs -->
			<function name="getTimeVariable" params="chunk" c="xs_rng_getTimeVariable"/>
		</object>
	</patch>
</package>
