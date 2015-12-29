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
