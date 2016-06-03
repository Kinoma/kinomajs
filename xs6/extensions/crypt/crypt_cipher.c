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

#include "crypt.h"
#include "crypt_common.h"
#include "crypt_cipher.h"

void
xs_cipher_constructor(xsMachine *the)
{
	crypt_cipher_t *cipher;

	if ((cipher = crypt_malloc(sizeof(crypt_cipher_t))) == NULL)
		crypt_throw_error(the, "cipher: nomem");
	c_memset(cipher, 0, sizeof(crypt_cipher_t));
	cipher->direction = -1;
	xsSetHostData(xsThis, cipher);
}

void
xs_cipher_destructor(void *data)
{
	if (data != NULL) {
		crypt_cipher_t *cipher = data;
		if (cipher->ctx != NULL)
			(*cipher->finish)(cipher->ctx);
		crypt_free(cipher);
	}
}

static void
cipher_process(xsMachine *the, kcl_symmetric_direction_t direction)
{
	crypt_cipher_t *cipher = xsGetHostData(xsThis);
	size_t len;
	void *indata, *outdata;

	if (cipher->keysched != NULL) {
		if (cipher->direction != direction) {
			(*cipher->keysched)(cipher->ctx, direction);
			cipher->direction = direction;
		}
	}
	if (xsToInteger(xsArgc) > 1 && xsTest(xsArg(1))) {
		if (xsGetArrayBufferLength(xsArg(1)) < (xsIntegerValue)cipher->blockSize)
			crypt_throw_error(the, "cipher: too small buffer");
		xsResult = xsArg(1);
	}
	else
		xsResult = xsArrayBuffer(NULL, cipher->blockSize);
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		indata = xsToString(xsArg(0));
		len = c_strlen(indata);
	}
	else {
		indata = xsToArrayBuffer(xsArg(0));
		len = xsGetArrayBufferLength(xsArg(0));
	}
	if (len < cipher->blockSize)
		crypt_throw_error(the, "cipher: wrong size");
	outdata = xsToArrayBuffer(xsResult);
	(*cipher->process)(cipher->ctx, indata, outdata);
}

void
xs_cipher_encrypt(xsMachine *the)
{
	cipher_process(the, KCL_DIRECTION_ENCRYPTION);
}


void
xs_cipher_decrypt(xsMachine *the)
{
	cipher_process(the, KCL_DIRECTION_DECRYPTION);
}

void
xs_cipher_getKeySize(xsMachine *the)
{
	crypt_cipher_t *cipher = xsGetHostData(xsThis);

	xsResult = xsInteger(cipher->keySize);
}

void
xs_cipher_getBlockSize(xsMachine *the)
{
	crypt_cipher_t *cipher = xsGetHostData(xsThis);

	xsResult = xsInteger(cipher->blockSize);
}
