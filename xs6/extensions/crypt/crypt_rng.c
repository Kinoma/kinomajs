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

static uint8_t rng_state[256];
static int rng_inited = 0;

static void
rng_init(uint8_t *seed, size_t seedsize)
{
	int i;
	uint8_t j, t;

	for (i = j = 0; i < 256; i++) {
		j += rng_state[i] + seed[i % seedsize];
		t = rng_state[i];
		rng_state[i] = rng_state[j];
		rng_state[j] = t;
	}
}

void
xs_rng_get(xsMachine *the)
{
	size_t n = xsToInteger(xsArg(0));
	uint8_t *bp, *bufend;
	uint8_t i, j, t;

	if (!rng_inited) {
		/* use srand */
#define DEFAULT_SEED_SIZE	16
		uint8_t seed[DEFAULT_SEED_SIZE];
		for (i = 0; i < DEFAULT_SEED_SIZE; i++)
			seed[i] = ((uint64_t)c_rand() * 256) / C_RAND_MAX;
		rng_init(seed, DEFAULT_SEED_SIZE);
		rng_inited++;
	}
	xsResult = xsArrayBuffer(NULL, n);
	bp = xsToArrayBuffer(xsResult);
	for (i = j = 0, bufend = bp + n; bp < bufend;) {
		++i;
		j += rng_state[i];
		t = rng_state[i];
		rng_state[i] = rng_state[j];
		rng_state[j] = t;
		t = rng_state[i] + rng_state[j];
		*bp++ = rng_state[t];
	}
}

void
xs_rng_init(xsMachine *the)
{
	int i;

	for (i = 0; i < 256; i++)
		rng_state[i] = i;
	if (xsToInteger(xsArgc) > 0 && xsTest(xsArg(0))) {
		uint8_t *seed = xsToArrayBuffer(xsArg(0));
		size_t seedsize = xsGetArrayBufferLength(xsArg(0));
		rng_init(seed, seedsize);
	}
	rng_inited++;
}
