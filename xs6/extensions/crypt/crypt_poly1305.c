/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#include "crypt.h"
#include "poly1305-donna.h"

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
		sz = c_strlen(msg);
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
