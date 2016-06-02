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
#include "xs.h"
#include "crypt_mode.h"

static size_t
ecb_encrypt(crypt_mode_t *mode, const void *indata, void *outdata, size_t n, int eof)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = cipher->blockSize;
	const uint8_t *src = indata;
	uint8_t *dst = outdata;

	if (mode->direction != KCL_DIRECTION_ENCRYPTION) {
		(*cipher->keysched)(cipher->ctx, KCL_DIRECTION_ENCRYPTION);
		mode->direction = KCL_DIRECTION_ENCRYPTION;
	}
	while (n >= blksz) {
		(*cipher->process)(cipher->ctx, src, dst);
		n -= blksz;
		src += blksz;
		dst += blksz;
	}
	if (eof && mode->padding) {
		/* process padding in the RFC2630 compatible way */
		uint8_t pad = blksz - n;
		uint8_t tbuf[CRYPT_MAX_BLOCKSIZE];
		unsigned int i;
		for (i = 0; i < n; i++)
			tbuf[i] = src[i];
		for (; i < blksz; i++)
			tbuf[i] = pad;
		(*cipher->process)(cipher->ctx, tbuf, dst);
		dst += blksz;
	}
	return dst - (uint8_t *)outdata;
}

static size_t
ecb_decrypt(crypt_mode_t *mode, const void *indata, void *outdata, size_t n, int eof)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = cipher->blockSize;
	const uint8_t *src = indata;
	uint8_t *dst = outdata;

	if (mode->direction != KCL_DIRECTION_DECRYPTION) {
		(*cipher->keysched)(cipher->ctx, KCL_DIRECTION_DECRYPTION);
		mode->direction = KCL_DIRECTION_DECRYPTION;
	}
	while (n >= blksz) {
		(*cipher->process)(cipher->ctx, src, dst);
		n -= blksz;
		src += blksz;
		dst += blksz;
	}
	if (eof && mode->padding) {
		size_t padlen = *(dst - 1);
		if (padlen > blksz)
			padlen = 0;	/* what should we do?? */
		dst -= padlen;
	}
	return dst - (uint8_t *)outdata;
}

void
xs_ecb_init(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);

	mode->encrypt = ecb_encrypt;
	mode->decrypt = ecb_decrypt;
	mode->setIV = NULL;
	mode->maxSlop = mode->cipher->blockSize;
	mode->direction = -1;
	mode->padding = xsToInteger(xsArgc) > 0 && xsToBoolean(xsArg(0));
}
