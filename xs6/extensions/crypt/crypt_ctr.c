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
#include "kcl_symmetric.h"
#include <stdint.h>
#include <string.h>

static size_t
ctr_process(crypt_mode_t *mode, const void *indata, void *outdata, size_t n, int eof)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = cipher->blockSize;
	const uint8_t *src = indata;
	uint8_t *dst = outdata;
	uint8_t *ctrbuf = mode->em_buf;
	int i, c;
	uint8_t tbuf[CRYPT_MAX_BLOCKSIZE];

	while (n != 0) {
		(*cipher->process)(cipher->ctx, ctrbuf, tbuf);
		for (i = mode->offset; n != 0 && (unsigned)i < blksz; i++, --n)
			*dst++ = *src++ ^ tbuf[i];
		if (n != 0) {
			mode->offset = 0;
			/* increment the counter */
			for (i = blksz, c = 1; --i >= 0 && c != 0;) {
				uint8_t t = ctrbuf[i];
				ctrbuf[i]++;
				c = ctrbuf[i] < t;
			}
		}
		else
			mode->offset = i;
	}
	return dst - (uint8_t *)outdata;
}

static void
ctr_setIV(crypt_mode_t *mode, const void *iv, size_t ivsize)
{
	crypt_cipher_t *cipher = mode->cipher;
	size_t blksz = cipher->blockSize;

	memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ivsize > blksz)
		ivsize = blksz;
	memcpy(&mode->em_buf[blksz - ivsize], iv, ivsize);
}

void
xs_ctr_init(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);

	mode->encrypt = ctr_process;
	mode->decrypt = ctr_process;
	mode->setIV = ctr_setIV;
	mode->maxSlop = 0;
	memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ac > 0 && xsTest(xsArg(0))) {
		/* iv */
		void *iv = xsToArrayBuffer(xsArg(0));
		size_t ivsize = xsGetArrayBufferLength(xsArg(0));
		ctr_setIV(mode, iv, ivsize);
	}
	(*mode->cipher->keysched)(mode->cipher->ctx, KCL_DIRECTION_ENCRYPTION);	/* CTR uses encryption only */
}
