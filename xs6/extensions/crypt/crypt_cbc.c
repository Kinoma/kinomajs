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
#include "crypt.h"
#include "crypt_mode.h"
#include <string.h>

static void
cbc_xor(uint8_t *t, const uint8_t *x, size_t n)
{
	while (n-- != 0)
		*t++ ^= *x++;
}

static size_t
cbc_encrypt(crypt_mode_t *mode, const void *indata, void *outdata, size_t n, int eof)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = mode->cipher->blockSize;
	const uint8_t *src = indata;
	uint8_t *dst = outdata;
	uint8_t *prev = mode->em_buf;

	if (n == 0)
		return 0;
	if (mode->direction != KCL_DIRECTION_ENCRYPTION) {
		(*cipher->keysched)(cipher->ctx, KCL_DIRECTION_ENCRYPTION);
		mode->direction = KCL_DIRECTION_ENCRYPTION;
	}
	while (n >= blksz) {
		cbc_xor(prev, src, blksz);
		(*cipher->process)(cipher->ctx, prev, dst);
		memcpy(prev, dst, blksz);
		n -= blksz;
		src += blksz;
		dst += blksz;
	}
	if (eof && mode->padding) {
		/* process the padding method defined in RFC2630 */
		uint8_t pad[CRYPT_MAX_BLOCKSIZE];
		uint8_t padbyte = blksz - n;
		unsigned int i;
		for (i = 0; i < n; i++)
			pad[i] = src[i];
		for (; i < blksz; i++)
			pad[i] = padbyte;
		cbc_xor(prev, pad, blksz);
		(*cipher->process)(cipher->ctx, prev, dst);
		dst += blksz;
	}
	return dst - (uint8_t *)outdata;
}

static size_t
cbc_decrypt(crypt_mode_t *mode, const void *indata, void *outdata, size_t n, int eof)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = mode->cipher->blockSize;
	const uint8_t *src = indata;
	uint8_t *dst = outdata;
	unsigned int i;

	if (n == 0)
		return 0;
	if (mode->direction != KCL_DIRECTION_DECRYPTION) {
		(*cipher->keysched)(cipher->ctx, KCL_DIRECTION_DECRYPTION);
		mode->direction = KCL_DIRECTION_DECRYPTION;
	}
	while (n >= blksz) {
		uint8_t *prev = mode->em_buf;
		uint8_t tbuf[CRYPT_MAX_BLOCKSIZE], *t = tbuf;
		(*cipher->process)(cipher->ctx, src, tbuf);
		for (i = blksz; i-- != 0;) {
			uint8_t c = *prev;
			*prev++ = *src++;
			*dst++ = c ^ *t++;
		}
		n -= blksz;
	}
	if (eof && mode->padding) {
		/* process RFC2630 */
		uint8_t padlen = *(dst - 1);
		if (padlen > blksz)
			padlen = 0;	/* what should we do?? */
		dst -= padlen;
	}
	return dst - (uint8_t *)outdata;
}

static void
cbc_setIV(crypt_mode_t *mode, const void *iv, size_t ivsize)
{
	size_t blksz = mode->cipher->blockSize;

	memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ivsize > blksz)
		ivsize = blksz;
	memcpy(mode->em_buf, iv, ivsize);
}

void
xs_cbc_init(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);

	mode->encrypt = cbc_encrypt;
	mode->decrypt = cbc_decrypt;
	mode->setIV = cbc_setIV;
	mode->direction = -1;
	mode->maxSlop = mode->cipher->blockSize;
	memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ac > 0 && xsTest(xsArg(0))) {
		/* iv */
		size_t ivsize = xsGetArrayBufferLength(xsArg(0));
		void *iv = xsToArrayBuffer(xsArg(0));
		cbc_setIV(mode, iv, ivsize);
	}
	mode->padding = ac > 1 && xsToBoolean(xsArg(1));
}
