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

#define GHASH_BLOCK_SIZE	(128/8)
#define GHASH_DEFAULT_OUTPUT_SIZE	(128/8)

typedef union {
	uint8_t _8[16];
	uint64_t _64[2];
} uint128_t;

typedef struct {
	uint128_t h;
	uint128_t y;
	uint128_t y0;
	size_t len;
	size_t aad_len;
} ghash_t;

#define SWAP64(v)	((v >> 56) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) | ((v >> 8) & 0xff000000) | ((v << 8) & 0xff00000000) | ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) | ((v << 56) & 0xff00000000000000))

static void
fix128(uint128_t *v)
{
#if mxLittleEndian
	uint64_t t;

	t = v->_64[1];
	v->_64[1] = SWAP64(v->_64[0]);
	v->_64[0] = SWAP64(t);
#endif
}

static void
rightshift(uint128_t *v)
{
#if mxLittleEndian
	v->_64[0] >>= 1;
	v->_64[0] |= (v->_64[1] & 1) << 63;
	v->_64[1] >>= 1;
#else
	v->_64[1] >>= 1;
	v->_64[1] |= (v->_64[0] & 1) << 63;
	v->_64[0] >>= 1;
#endif
}

static int
isset128(const uint128_t *v, int i)
{
#if mxLittleEndian
	return (v->_8[15 - (i / 8)] >> (7 - (i % 8))) & 1;
#else
	return (v->_8[i / 8] >> (7 - (i % 8))) & 1;
#endif
}

static void
xor128(uint128_t *r, const uint128_t *t)
{
	r->_64[0] ^= t->_64[0];
	r->_64[1] ^= t->_64[1];
}

static void
ghash_mul(const uint128_t *x, const uint128_t *y, uint128_t *z)
{
	uint128_t v;
	int i;
#define R	0xe1

	/* take into consideration x = z */
	c_memcpy(&v, x, sizeof(uint128_t));
	c_memset(z, 0, sizeof(uint128_t));
	for (i = 0; i < 128; i++) {
		if (isset128(y, i))
			xor128(z, &v);
		if (isset128(&v, 127)) {
			rightshift(&v);
#if mxLittleEndian
			v._8[15] ^= R;
#else
			v._8[0] ^= R;
#endif
		}
		else
			rightshift(&v);
	}
}

static void
ghash_finish(void *ctx)
{
	if (ctx)
		crypt_free(ctx);
}

static void
ghash_update(void *ctx, const void *data, size_t sz)
{
	ghash_t *ghash = ctx;
	const uint8_t *p = data;
	uint128_t c;

	ghash->len += sz;
	while (sz != 0) {
		size_t len = sz;
		if (len > sizeof(uint128_t))
			len = sizeof(uint128_t);
		else if (len < sizeof(uint128_t))
			c_memset(&c, 0, sizeof(uint128_t));
		c_memcpy(&c, p, len);
		fix128(&c);
		xor128(&ghash->y, &c);
		ghash_mul(&ghash->y, &ghash->h, &ghash->y);
		p += len;
		sz -= len;
	}
}

static void
ghash_init(void *ctx)
{
	ghash_t *ghash = ctx;

	c_memcpy(&ghash->y, &ghash->y0, sizeof(ghash->y));
	ghash->len = 0;
}

static void
ghash_result(void *ctx, void *result)
{
	ghash_t *ghash = ctx;
	uint128_t l;

	/* len(A) || len(C) */
#if mxLittleEndian
	l._64[1] = ghash->aad_len * 8;
	l._64[0] = ghash->len * 8;
#else
	l._64[0] = ghash->aad_len * 8;
	l._64[1] = ghash->len * 8;
#endif
	xor128(&ghash->y, &l);
	ghash_mul(&ghash->y, &ghash->h, &ghash->y);
	fix128(&ghash->y);
	c_memcpy(result, &ghash->y, GHASH_DEFAULT_OUTPUT_SIZE);
}

void
xs_ghash_init(xsMachine *the)
{
	crypt_digest_t *digest = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	ghash_t *ghash;
	size_t len;

	if ((ghash = crypt_malloc(sizeof(ghash_t))) == NULL)
		crypt_throw_error(the, "ghash: nomem");
	len = xsGetArrayBufferLength(xsArg(0));
	if (len > sizeof(ghash->h))
		len = sizeof(ghash->h);
	c_memcpy(&ghash->h, xsToArrayBuffer(xsArg(0)), len);
	fix128(&ghash->h);
	if (ac > 1 && xsTest(xsArg(1))) {
		void *aad = xsToArrayBuffer(xsArg(1));
		len = xsGetArrayBufferLength(xsArg(1));
		c_memset(&ghash->y, 0, sizeof(ghash->y));
		ghash_update(ghash, aad, len);
		c_memcpy(&ghash->y0, &ghash->y, sizeof(ghash->y));
		ghash->aad_len = len;
	}
	else {
		c_memset(&ghash->y0, 0, sizeof(ghash->y0));
		ghash->aad_len = 0;
	}
	ghash->len = 0;
	digest->ctx = ghash;
	digest->init = ghash_init;
	digest->update = ghash_update;
	digest->result = ghash_result;
	digest->finish = ghash_finish;
	digest->blockSize = GHASH_BLOCK_SIZE;
	digest->outputSize = GHASH_DEFAULT_OUTPUT_SIZE;
	ghash_init(digest->ctx);
}
