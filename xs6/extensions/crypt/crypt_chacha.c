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
#include "crypt_stream.h"
#include "crypt_common.h"
#include "kcl_symmetric.h"

void
xs_chacha_init(xsMachine *the)
{
	crypt_stream_t *stream = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	void *key, *iv;
	size_t keysize, ivsize;
	uint64_t counter = 0;
	kcl_err_t err;

	key = xsToArrayBuffer(xsArg(0));
	keysize = xsGetArrayBufferLength(xsArg(0));
	if (ac > 1 && xsTest(xsArg(1))) {
		iv = xsToArrayBuffer(xsArg(1));
		ivsize = xsGetArrayBufferLength(xsArg(1));
	}
	else {
		iv = NULL;
		ivsize = 0;
	}
	if (ac > 2) {
		switch (xsTypeOf(xsArg(2))) {
		case xsIntegerType:
		case xsNumberType:
			counter = xsToInteger(xsArg(2));	/* @@ take only 32bit */
			break;
		}
	}
	if ((err = kcl_chacha_init(&stream->ctx, key, keysize, iv, ivsize, counter)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	stream->process = kcl_chacha_process;
	stream->finish = kcl_chacha_finish;
}

void
xs_chacha_setIV(xsMachine *the)
{
	crypt_stream_t *stream = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	void *iv;
	size_t ivsize;
	uint64_t counter = 0;

	if (ac > 0 && xsTest(xsArg(0))) {
		iv = xsToArrayBuffer(xsArg(0));
		ivsize = xsGetArrayBufferLength(xsArg(0));
	}
	else {
		iv = NULL;
		ivsize = 0;
	}
	if (ac > 1) {
		switch (xsTypeOf(xsArg(1))) {
		case xsIntegerType:
		case xsNumberType:
			counter = xsToInteger(xsArg(1));	/* @@ take only 32bit */
			break;
		}
	}
	kcl_chacha_setIV(stream->ctx, iv, ivsize, counter);
}
