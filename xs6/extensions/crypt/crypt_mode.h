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
#include "crypt_cipher.h"

typedef struct crypt_mode {
	crypt_cipher_t *cipher;
	size_t (*encrypt)(struct crypt_mode *, const void *indata, void *outdata, size_t n, int eof);
	size_t (*decrypt)(struct crypt_mode *, const void *indata, void *outdata, size_t n, int eof);
	void (*setIV)(struct crypt_mode *, const void *iv, size_t ivsize);
	kcl_symmetric_direction_t direction;
	size_t maxSlop;
	int padding;
	int eof;
	size_t offset;
	uint8_t em_buf[CRYPT_MAX_BLOCKSIZE];
} crypt_mode_t;
