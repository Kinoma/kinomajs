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
#include "poly1305-donna.h"
#include <string.h>

#define POLY1305_KEYSIZE	32
#define POLY1305_OUTPUTSIZE	16

void
xs_poly1305_constructor(xsMachine *the)
{
	void *key;
	xsIntegerValue keySize;
	poly1305_context *ctx;

	key = xsToArrayBuffer(xsArg(0));
	keySize = xsGetArrayBufferLength(xsArg(0));
	if (keySize < POLY1305_KEYSIZE)
		xsRangeError("bad arg");
	if ((ctx = crypt_malloc(sizeof(poly1305_context))) == NULL)
		xsUnknownError("no mem");
	poly1305_init(ctx, key);
	xsSetHostData(xsThis, ctx);
}

void
xs_poly1305_destructor(void *data)
{
	if (data != NULL)
		crypt_free(data);
}

void
xs_poly1305_update(xsMachine *the)
{
	poly1305_context *ctx = xsGetHostData(xsThis);
	void *msg;
	xsIntegerValue sz;

	if (xsTypeOf(xsArg(0)) == xsStringType) {
		msg = xsToString(xsArg(0));
		sz = strlen(msg);
	}
	else {
		msg = xsToArrayBuffer(xsArg(0));
		sz = xsGetArrayBufferLength(xsArg(0));
	}
	poly1305_update(ctx, msg, sz);
}

void
xs_poly1305_close(xsMachine *the)
{
	poly1305_context *ctx;

	xsResult = xsArrayBuffer(NULL, POLY1305_OUTPUTSIZE);
	ctx = xsGetHostData(xsThis);
	poly1305_finish(ctx, xsToArrayBuffer(xsResult));
}

void
xs_poly1305_keySize(xsMachine *the)
{
	xsResult = xsInteger(POLY1305_KEYSIZE);
}

void
xs_poly1305_outputSize(xsMachine *the)
{
	xsResult = xsInteger(POLY1305_OUTPUTSIZE);
}
