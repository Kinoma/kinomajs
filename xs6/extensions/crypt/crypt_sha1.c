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

#include "crypt.h"
#include "crypt_common.h"
#include "crypt_digest.h"
#include "kcl_symmetric.h"

void
xs_sha1_init(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);
	kcl_err_t err;

	if ((err = kcl_sha1_create(&digest->ctx)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	digest->update = kcl_sha1_update;
	digest->init = kcl_sha1_init;
	digest->result = kcl_sha1_result;
	digest->finish = kcl_sha1_finish;
	kcl_sha1_size(digest->ctx, &digest->blockSize, &digest->outputSize);
	kcl_sha1_init(digest->ctx);
}
