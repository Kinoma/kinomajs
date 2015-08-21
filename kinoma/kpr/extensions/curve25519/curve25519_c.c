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
#include "cryptTypes.h"
#include "common.h"
#include "c25519.h"
#include "FskExtensions.h"

void
xs_curve25519_dh(xsMachine *the)
{
	void *secret, *basepoint;
	xsIntegerValue sz;

	xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(32));
	getChunkData(the, &xsArg(0), &secret, &sz);
	if (sz != 32)
		cryptThrowFSK(kFskErrInvalidParameter);
	if (xsToInteger(xsArgc) > 1) {
		getChunkData(the, &xsArg(1), &basepoint, &sz);
		if (sz != 32)
			cryptThrowFSK(kFskErrInvalidParameter);
	}
	else
		basepoint = (void *)c25519_base_x;
	c25519_prepare(secret);
	c25519_smult(xsGetHostData(xsResult), basepoint, secret);
}

void
xs_curve25519_constructor(xsMachine *the)
{
}

void
xs_curve25519_destructor(void *data)
{
}

FskErr curve25519_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}
FskErr curve25519_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}
