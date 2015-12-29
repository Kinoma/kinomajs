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
#include "kcl_symmetric.h"
#include <string.h>	/* for mem... */

/*
 * DES
 */
#include "fips46.h"
struct des_context {
	des_subkey subkey;
	uint8_t key[8];
};

kcl_err_t
kcl_des_init(void **ctxp, const void *key, size_t keysize)
{
	struct des_context *desc;

	if (keysize < 8)
		return KCL_ERR_BAD_ARG;
	if ((desc = kcl_malloc(sizeof(struct des_context))) == NULL)
		return KCL_ERR_NOMEM;
	if (keysize > 8) keysize = 8;
	memset(desc->key, 0, sizeof(desc->key));
	memcpy(desc->key, key, keysize);
	*ctxp = desc;
	return KCL_ERR_NONE;
}

void
kcl_des_keysched(void *ctx, kcl_symmetric_direction_t direction)
{
	struct des_context *desc = ctx;

	des_keysched(desc->key, direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, desc->subkey);
}

void
kcl_des_process(void *ctx, const void *in, void *out)
{
	struct des_context *desc = ctx;

	des_process(in, out, desc->subkey);
}

void
kcl_des_size(void *ctx, size_t *blksz, size_t *keysz)
{
	*blksz = 8;
	*keysz = 8;
}

void
kcl_des_finish(void *ctx)
{
	kcl_free(ctx);
}

struct tdes_context {
	des_subkey subkey[3];
	uint8_t key[8*3];
};

kcl_err_t
kcl_tdes_init(void **ctxp, const void *key, size_t keysize)
{
	struct tdes_context *tdes;

	if ((tdes = kcl_malloc(sizeof(struct tdes_context))) == NULL)
		return KCL_ERR_NOMEM;
	if (keysize > 8 * 3)
		keysize = 8 * 3;
	memset(tdes->key, 0, sizeof(tdes->key));
	if (keysize <= 8) {
		memcpy(tdes->key, key, keysize);
		memcpy(&tdes->key[8], key, keysize);
		memcpy(&tdes->key[16], key, keysize);
	}
	else if (keysize <= 16) {
		memcpy(tdes->key, key, keysize);
		memcpy(&tdes->key[16], key, 8);
	}
	else
		memcpy(tdes->key, key, keysize);
	*ctxp = tdes;
	return KCL_ERR_NONE;
}

void
kcl_tdes_keysched(void *ctx, kcl_symmetric_direction_t direction)
{
	struct tdes_context *tdes = ctx;

	des_keysched(tdes->key, direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, tdes->subkey[0]);
	des_keysched(&tdes->key[8], direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_decryption : des_cipher_encryption, tdes->subkey[1]);
	des_keysched(&tdes->key[16], direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, tdes->subkey[2]);
}

void
kcl_tdes_process(void *ctx, const void *in, void *out)
{
	struct tdes_context *tdes = ctx;

	des_process(in, out, tdes->subkey[0]);
	des_process(out, out, tdes->subkey[1]);
	des_process(out, out, tdes->subkey[2]);
}

void
kcl_tdes_size(void *ctx, size_t *blksz, size_t *keysz)
{
	*blksz = 8;
	*keysz = 8*3;
}

void
kcl_tdes_finish(void *ctx)
{
	kcl_free(ctx);
}

/*
 * AES
 */
#include "fips197.h"

struct aes_context {
	uint32_t Nk, Nb, Nr;
	uint32_t subkey[(14+1)*4];	/* (Nr + 1) * Nb, Nr = MAX(Nk, Nb) + 6, Nb is fixed to 4 */
	uint8_t key[32];
	enum aes_cipher_direction direction;
};

kcl_err_t
kcl_aes_init(void **ctxp, const void *key, size_t keysize, size_t blocksize)
{
	struct aes_context *aes;

	if ((keysize != 16 && keysize != 24 && keysize != 32) || (blocksize != 16))	/* the key scheduler can take 16, 24, 32 bytes block size but enc/dec processes are fixed to 16 for both time and size efficiency */
		return KCL_ERR_BAD_ARG;
	if ((aes = kcl_malloc(sizeof(struct aes_context))) == NULL)
		return KCL_ERR_NOMEM;
	aes->Nk = keysize / 4;
	aes->Nb = blocksize / 4;
	aes->Nr = aes->Nk >= aes->Nb ? aes->Nk + 6 : aes->Nb + 6;
	memcpy(aes->key, key, keysize);
	*ctxp = aes;
	return KCL_ERR_NONE;
}

void
kcl_aes_keysched(void *ctx, kcl_symmetric_direction_t direction)
{
	struct aes_context *aes = ctx;

	aes->direction = direction == KCL_DIRECTION_ENCRYPTION ? aes_cipher_encryption : aes_cipher_decryption;
	aes_keysched(aes->key, aes->Nk, aes->Nb, aes->Nr, aes->direction, aes->subkey);
}

void
kcl_aes_process(void *ctx, const void *in, void *out)
{
	struct aes_context *aes = ctx;

	if (aes->direction == aes_cipher_encryption)
		aes_encrypt4(in, out, aes->Nr, aes->subkey);
	else
		aes_decrypt4(in, out, aes->Nr, aes->subkey);
}

void
kcl_aes_size(void *ctx, size_t *blksz, size_t *keysz)
{
	struct aes_context *aes = ctx;

	*blksz = aes->Nb * 4;
	*keysz = aes->Nk * 4;
}

void
kcl_aes_finish(void *ctx)
{
	kcl_free(ctx);
}

/*
 * RC4
 */
#include "rc.h"

kcl_err_t
kcl_rc4_init(void **ctxp, const void *key, size_t keysize)
{
	rc4_state_t *rc4;

	if ((rc4 = kcl_malloc(sizeof(rc4_state_t))) == NULL)
		return KCL_ERR_NOMEM;
	rc4_init(rc4, key, keysize);
	*ctxp = rc4;
	return KCL_ERR_NONE;
}

void
kcl_rc4_process(void *ctx, const void *in, void *out, size_t n)
{
	rc4_process(in, out, n, ctx);
}

void
kcl_rc4_finish(void *ctx)
{
	kcl_free(ctx);
}

/*
 * chacha
 */
#include "chacha.h"

kcl_err_t
kcl_chacha_init(void **ctxp, const void *key, size_t keysize, const void *iv, size_t ivsize, uint64_t counter)
{
	chacha_ctx *chacha;

	if ((chacha = kcl_malloc(sizeof(chacha_ctx))) == NULL)
		return KCL_ERR_NOMEM;
	chacha_keysetup(chacha, key, keysize);
	chacha_ivsetup(chacha, iv, ivsize, counter);
	*ctxp = chacha;
	return KCL_ERR_NONE;
}

void
kcl_chacha_process(void *ctx, const void *in, void *out, size_t n)
{
	chacha_process(in, out, n, ctx);
}

void
kcl_chacha_finish(void *ctx)
{
	kcl_free(ctx);
}

void
kcl_chacha_setIV(void *ctx, const void *iv, size_t ivsize, uint64_t counter)
{
	chacha_ivsetup(ctx, iv, ivsize, counter);
}

/*
 * SHA
 */
#include "fips180.h"

kcl_err_t
kcl_sha1_create(void **ctxp)
{
	struct sha1 *sha;

	if ((sha = kcl_malloc(sizeof(struct sha1))) == NULL)
		return KCL_ERR_NOMEM;
	*ctxp = sha;
	return KCL_ERR_NONE;
}

void
kcl_sha1_init(void *ctx)
{
	sha1_create(ctx);
}

void
kcl_sha1_update(void *ctx, const void *data, size_t sz)
{
	sha1_update(ctx, data, sz);
}

void
kcl_sha1_result(void *ctx, void *result)
{
	sha1_fin(ctx, result);
}

void
kcl_sha1_finish(void *ctx)
{
	kcl_free(ctx);
}

void
kcl_sha1_size(void *ctx, size_t *blksz, size_t *outsz)
{
	*blksz = SHA1_BLKSIZE;
	*outsz = SHA1_DGSTSIZE;
}

kcl_err_t
kcl_sha256_create(void **ctxp)
{
	struct sha256 *sha;

	if ((sha = kcl_malloc(sizeof(struct sha256))) == NULL)
		return KCL_ERR_NOMEM;
	*ctxp = sha;
	return KCL_ERR_NONE;
}

void
kcl_sha256_init(void *ctx)
{
	sha256_create(ctx);
}

void
kcl_sha256_update(void *ctx, const void *data, size_t sz)
{
	sha256_update(ctx, data, sz);
}

void
kcl_sha256_result(void *ctx, void *result)
{
	sha256_fin(ctx, result);
}

void
kcl_sha256_finish(void *ctx)
{
	kcl_free(ctx);
}

void
kcl_sha256_size(void *ctx, size_t *blksz, size_t *outsz)
{
	*blksz = SHA256_BLKSIZE;
	*outsz = SHA256_DGSTSIZE;
}

kcl_err_t
kcl_sha512_create(void **ctxp)
{
	struct sha512 *sha;

	if ((sha = kcl_malloc(sizeof(struct sha512))) == NULL)
		return KCL_ERR_NOMEM;
	*ctxp = sha;
	return KCL_ERR_NONE;
}

void
kcl_sha512_init(void *ctx)
{
	sha512_create(ctx);
}

void
kcl_sha512_update(void *ctx, const void *data, size_t sz)
{
	sha512_update(ctx, data, sz);
}

void
kcl_sha512_result(void *ctx, void *result)
{
	sha512_fin(ctx, result);
}

void
kcl_sha512_finish(void *ctx)
{
	kcl_free(ctx);
}

void
kcl_sha512_size(void *ctx, size_t *blksz, size_t *outsz)
{
	*blksz = SHA512_BLKSIZE;
	*outsz = SHA512_DGSTSIZE;
}

/*
 * MD5
 */
#include "rfc1321.h"

kcl_err_t
kcl_md5_create(void **ctxp)
{
	struct md5 *md;

	if ((md = kcl_malloc(sizeof(struct md5))) == NULL)
		return KCL_ERR_NOMEM;
	*ctxp = md;
	return KCL_ERR_NONE;
}

void
kcl_md5_init(void *ctx)
{
	md5_create(ctx);
}

void
kcl_md5_update(void *ctx, const void *data, size_t sz)
{
	md5_update(ctx, data, sz);
}

void
kcl_md5_result(void *ctx, void *data)
{
	md5_fin(ctx, data);
}

void
kcl_md5_finish(void *ctx)
{
	kcl_free(ctx);
}

void
kcl_md5_size(void *ctx, size_t *blksz, size_t *outsz)
{
	*blksz = MD5_BLKSIZE;
	*outsz = MD5_DGSTSIZE;
}
