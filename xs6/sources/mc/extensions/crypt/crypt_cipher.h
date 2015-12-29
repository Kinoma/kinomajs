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
#include <stddef.h>	/* for size_t */
#include "kcl_symmetric.h"

#define CRYPT_MAX_BLOCKSIZE	16

typedef struct {
	void *ctx;
	void (*keysched)(void *ctx, kcl_symmetric_direction_t direction);
	void (*process)(void *ctx, const void *in, void *out);
	void (*finish)(void *ctx);
	kcl_symmetric_direction_t direction;
	size_t keySize;
	size_t blockSize;
} crypt_cipher_t;
