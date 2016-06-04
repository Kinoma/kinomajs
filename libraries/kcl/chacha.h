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
#include <stdint.h>

#define CHACHA_NSTATE	16

typedef struct {
	uint32_t state[CHACHA_NSTATE];
	uint8_t pad[CHACHA_NSTATE * sizeof(uint32_t)];
	uint32_t npad;
} chacha_ctx;

extern int chacha_keysetup(chacha_ctx *x, const uint8_t *k, uint32_t kbytes);
extern void chacha_ivsetup(chacha_ctx *x, const uint8_t *iv, uint32_t ivSize, uint64_t counter);
extern void chacha_process(const void *in, void *out, uint32_t nbytes, chacha_ctx *x);
