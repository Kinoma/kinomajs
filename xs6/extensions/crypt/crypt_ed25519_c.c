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
#include "edsign.h"

void
xs_ed25519_getPK(xsMachine *the)
{
	void *sk;

	xsResult = xsArrayBuffer(NULL, EDSIGN_PUBLIC_KEY_SIZE);
	sk = xsToArrayBuffer(xsArg(0));
	edsign_sec_to_pub(xsToArrayBuffer(xsResult), sk);
}

void
xs_ed25519_sign(xsMachine *the)
{
	void *msg, *sk, *pk;
	xsIntegerValue sz;

	xsResult = xsArrayBuffer(NULL, EDSIGN_SIGNATURE_SIZE);
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		msg = xsToString(xsArg(0));
		sz = c_strlen(msg);
	}
	else {
		msg = xsToArrayBuffer(xsArg(0));
		sz = xsGetArrayBufferLength(xsArg(0));
	}
	sk = xsToArrayBuffer(xsArg(1));
	pk = xsToArrayBuffer(xsArg(2));
	edsign_sign(xsToArrayBuffer(xsResult), pk, sk, msg, sz);
}

void
xs_ed25519_verify(xsMachine *the)
{
	void *msg, *sig, *pk;
	xsIntegerValue sz;

	if (xsTypeOf(xsArg(0)) == xsStringType) {
		msg = xsToString(xsArg(0));
		sz = c_strlen(msg);
	}
	else {
		msg = xsToArrayBuffer(xsArg(0));
		sz = xsGetArrayBufferLength(xsArg(0));
	}
	sig = xsToArrayBuffer(xsArg(1));
	pk = xsToArrayBuffer(xsArg(2));
	if (edsign_verify(sig, pk, msg, sz))
		xsResult = xsBoolean(1);
	else
		xsResult = xsBoolean(0);
}

void
xs_ed25519_constructor(xsMachine *the)
{
}

void
xs_ed25519_destructor(void *data)
{
}
