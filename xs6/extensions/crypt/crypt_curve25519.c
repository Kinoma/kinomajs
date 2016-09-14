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
#include "c25519.h"

void
xs_curve25519_dh(xsMachine *the)
{
	void *secret, *basepoint;

	xsResult = xsArrayBuffer(NULL, C25519_EXPONENT_SIZE);
	secret = xsToArrayBuffer(xsArg(0));
	if (xsToInteger(xsArgc) > 1) {
		if (xsGetArrayBufferLength(xsArg(1)) != 32)
			xsRangeError("bad arg");
		basepoint = xsToArrayBuffer(xsArg(1));
	}
	else
		basepoint = (void *)c25519_base_x;
	c25519_prepare(secret);
	c25519_smult(xsToArrayBuffer(xsResult), basepoint, secret);
}

void
xs_curve25519_constructor(xsMachine *the)
{
}

void
xs_curve25519_destructor(void *data)
{
}
