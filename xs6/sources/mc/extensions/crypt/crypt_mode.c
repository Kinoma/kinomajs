/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
#include "crypt_common.h"
#include "crypt_mode.h"
#include <string.h>

void
xs_mode_constructor(xsMachine *the)
{
	crypt_mode_t *mode;

	if ((mode = crypt_malloc(sizeof(crypt_mode_t))) == NULL)
		crypt_throw_error(the, "mode: nomem");
	memset(mode, 0, sizeof(crypt_mode_t));
	mode->cipher = xsGetHostData(xsArg(0));
	if (mode->cipher->blockSize > CRYPT_MAX_BLOCKSIZE)
		crypt_throw_error(the, "mode: wrong blocksize");
	xsSet(xsThis, xsID("_cipher"), xsArg(0));
	xsSetHostData(xsThis, mode);
}

void
xs_mode_destructor(void *data)
{
	if (data != NULL)
		crypt_free(data);
}

void
xs_mode_encrypt(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);
	size_t n = xsGetArrayBufferLength(xsArg(0));
	void *indata, *outdata;

	xsResult = xsToInteger(xsArgc) > 1 && xsTest(xsArg(1)) ? xsArg(1) : xsArrayBuffer(NULL, n + mode->maxSlop);
	indata = xsToArrayBuffer(xsArg(0));
	outdata = xsToArrayBuffer(xsResult);
	n = (*mode->encrypt)(mode, indata, outdata, n, 0);
	xsSetArrayBufferLength(xsResult, n);
}

void
xs_mode_decrypt(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);
	size_t n = xsGetArrayBufferLength(xsArg(0));
	void *indata, *outdata;

	xsResult = xsToInteger(xsArgc) > 1 && xsTest(xsArg(1)) ? xsArg(1) : xsArrayBuffer(NULL, n);
	indata = xsToArrayBuffer(xsArg(0));
	outdata = xsToArrayBuffer(xsResult);
	n = (*mode->decrypt)(mode, indata, outdata, n, 0);
	xsSetArrayBufferLength(xsResult, n);
}

void
xs_mode_setIV(xsMachine *the)
{
	crypt_mode_t *mode = xsGetHostData(xsThis);

	if (mode->setIV != NULL) {
		size_t ivsize = xsGetArrayBufferLength(xsArg(0));
		void *iv = xsToArrayBuffer(xsArg(0));
		(*mode->setIV)(mode, iv, ivsize);
	}
}
