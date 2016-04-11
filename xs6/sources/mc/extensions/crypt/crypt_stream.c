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
#include "crypt_common.h"
#include "crypt_stream.h"
#include <string.h>

void
xs_stream_constructor(xsMachine *the)
{
	crypt_stream_t *stream;

	if ((stream = crypt_malloc(sizeof(crypt_stream_t))) == NULL)
		crypt_throw_error(the, "stream: nomem");
	memset(stream, 0, sizeof(crypt_stream_t));
	xsSetHostData(xsThis, stream);
}

void
xs_stream_destructor(void *data)
{
	if (data != NULL) {
		crypt_stream_t *stream = data;
		if (stream->ctx != NULL)
			(*stream->finish)(stream->ctx);
		crypt_free(stream);
	}
}

void
xs_stream_encrypt(xsMachine *the)
{
	crypt_stream_t *stream = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	size_t len;
	void *indata, *outdata;

	if (xsTypeOf(xsArg(0)) == xsStringType)
		len = strlen(xsToString(xsArg(0)));
	else
		len = xsGetArrayBufferLength(xsArg(0));
	if (ac > 2 && xsTypeOf(xsArg(2)) != xsUndefinedType) {
		size_t n = xsToInteger(xsArg(2));
		if (n < len)
			len = n;
	}
	if (ac > 1 && xsTest(xsArg(1))) {
		if (xsGetArrayBufferLength(xsArg(1)) < (xsIntegerValue)len)
			crypt_throw_error(the, "too small buffer");
		xsResult = xsArg(1);
	}
	else
		xsResult = xsArrayBuffer(NULL, len);
	if (xsTypeOf(xsArg(0)) == xsStringType)
		indata = xsToString(xsArg(0));
	else
		indata = xsToArrayBuffer(xsArg(0));
	outdata = xsToArrayBuffer(xsResult);
	(*stream->process)(stream->ctx, indata, outdata, len);
}

void
xs_stream_decrypt(xsMachine *the)
{
	xs_stream_encrypt(the);
}
