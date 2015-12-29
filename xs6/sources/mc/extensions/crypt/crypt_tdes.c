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
#include "crypt_cipher.h"
#include "crypt_common.h"
#include "kcl_symmetric.h"

void
xs_tdes_init(xsMachine *the)
{
	crypt_cipher_t *cipher = xsGetHostData(xsThis);
	void *key;
	size_t keysize;
	kcl_err_t err;

	key = xsToArrayBuffer(xsArg(0));
	keysize = xsGetArrayBufferLength(xsArg(0));
	if ((err = kcl_tdes_init(&cipher->ctx, key, keysize)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	cipher->keysched = kcl_tdes_keysched;
	cipher->process = kcl_tdes_process;
	cipher->finish = kcl_tdes_finish;
	kcl_tdes_size(cipher->ctx, &cipher->blockSize, &cipher->keySize);
}
