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
#include <stddef.h>	/* for size_t in edsign.h */
#include <string.h>
#include "xs.h"
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
		sz = strlen(msg);
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
		sz = strlen(msg);
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
