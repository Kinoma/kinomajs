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
#include "crypt_digest.h"
#include <string.h>

void
xs_digest_constructor(xsMachine *the)
{
	crypt_digest_t *digest;

	if ((digest = crypt_malloc(sizeof(crypt_digest_t))) == NULL)
		crypt_throw_error(the, "digest: nomem");
	memset(digest, 0, sizeof(crypt_digest_t));
	xsSetHostData(xsThis, digest);
}

void
xs_digest_destructor(void *data)
{
	if (data != NULL) {
		crypt_digest_t *digest = data;
		if (digest->ctx != NULL)
			(*digest->finish)(digest->ctx);
		crypt_free(digest);
	}
}

void
xs_digest_update(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc), i;
	int len;
	void *data;

	for (i = 0; i < ac; i++) {
		if (xsTypeOf(xsArg(i)) == xsStringType) {
			data = xsToString(xsArg(i));
			len = strlen(data);
		}
		else {
			len = xsGetArrayBufferLength(xsArg(i));
			data = xsToArrayBuffer(xsArg(i));
		}
		(*digest->update)(digest->ctx, data, len);
	}
}

void
xs_digest_reset(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);

	(*digest->init)(digest->ctx);
}

void
xs_digest_close(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);

	xsResult = xsArrayBuffer(NULL, digest->outputSize);
	(*digest->result)(digest->ctx, xsToArrayBuffer(xsResult));
}

void
xs_digest_getBlockSize(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);

	xsResult = xsInteger(digest->blockSize);
}

void
xs_digest_getOutputSize(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);

	xsResult = xsInteger(digest->outputSize);
}
