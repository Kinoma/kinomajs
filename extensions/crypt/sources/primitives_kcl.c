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
#include "primitives.h"

#if !FSK_NO_SHA1 || !FSK_NO_SHA256 || !FSK_NO_SHA512
#include "fips180.h"
#endif
#if !FSK_NO_MD5
#include "rfc1321.h"
#endif
#if !FSK_NO_RC4
#include "rc.h"
#endif
#if !FSK_NO_DES || !FSK_NO_TDES
#include "fips46.h"
struct des_context {
	int direction;
	des_subkey subkey;
	UInt8 key[8];
};

static void kcl_des_encrypt(void *in, void *out, void *ctx)
{
	struct des_context *desc = ctx;

	if (desc->direction != des_cipher_encryption) {
		desc->direction = des_cipher_encryption;
		des_keysched(desc->key, des_cipher_encryption, desc->subkey);
	}
	des_process(in, out, desc->subkey);
}

static void kcl_des_decrypt(void *in, void *out, void *ctx)
{
	struct des_context *desc = ctx;

	if (desc->direction != des_cipher_decryption) {
		desc->direction = des_cipher_decryption;
		des_keysched(desc->key, des_cipher_decryption, desc->subkey);
	}
	des_process(in, out, desc->subkey);
}
#if !FSK_NO_TDES
struct tdes_context {
	int direction;
	des_subkey subkey[3];
	UInt8 key[8*3];
};

static void kcl_tdes_encrypt(void *in, void *out, void *ctx)
{
	struct tdes_context *tdes = ctx;

	if (tdes->direction != des_cipher_encryption) {
		tdes->direction = des_cipher_encryption;
		des_keysched(tdes->key, des_cipher_encryption, tdes->subkey[0]);
		des_keysched(tdes->key + 8, des_cipher_decryption, tdes->subkey[1]);
		des_keysched(tdes->key + 16, des_cipher_encryption, tdes->subkey[2]);
	}
	des_process(in, out, tdes->subkey[0]);
	des_process(out, out, tdes->subkey[1]);
	des_process(out, out, tdes->subkey[2]);
}

static void kcl_tdes_decrypt(void *in, void *out, void *ctx)
{
	struct tdes_context *tdes = ctx;

	if (tdes->direction != des_cipher_decryption) {
		tdes->direction = des_cipher_decryption;
		des_keysched(tdes->key, des_cipher_decryption, tdes->subkey[2]);
		des_keysched(tdes->key + 8, des_cipher_encryption, tdes->subkey[1]);
		des_keysched(tdes->key + 16, des_cipher_decryption, tdes->subkey[0]);
	}
	des_process(in, out, tdes->subkey[0]);
	des_process(out, out, tdes->subkey[1]);
	des_process(out, out, tdes->subkey[2]);
}
#endif	/* !FSK_NO_TDES */
#endif	/* !FSK_NO_DES */

#if !FSK_NO_AES
#include "fips197.h"

struct aes_context {
	int Nk, Nb, Nr;
	int direction;
	UInt32 subkey[(14+1)*4];	/* (Nr + 1) * Nb, Nr = MAX(Nk, Nb) + 6, Nb is fixed to 4 */
	UInt8 key[32];
};

static inline void kcl_aes_encrypt(void *inData, void *outData, void *ctx)
{
	struct aes_context *aes = ctx;

	if (aes->direction != aes_cipher_encryption) {
		aes->direction = aes_cipher_encryption;
		aes_keysched(aes->key, aes->Nk, aes->Nb, aes->Nr, aes_cipher_encryption, aes->subkey);
	}
	aes_encrypt4(inData, outData, aes->Nr, aes->subkey);
}
static inline void kcl_aes_decrypt(void *inData, void *outData, void *ctx)
{
	struct aes_context *aes = ctx;

	if (aes->direction != aes_cipher_decryption) {
		aes->direction = aes_cipher_decryption;
		aes_keysched(aes->key, aes->Nk, aes->Nb, aes->Nr, aes_cipher_decryption, aes->subkey);
	}
	aes_decrypt4(inData, outData, aes->Nr, aes->subkey);
}
#endif	/* !FSK_NO_AES */

void
xs_sha1_constructor(xsMachine *the)
{
#if !FSK_NO_SHA1
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct sha1), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha1_update;
	dgst->close = (cryptDigestCloseProc)sha1_fin;
	dgst->create = (cryptDigestCreateProc)sha1_create;
	dgst->blockSize = SHA1_BLKSIZE;
	dgst->outputSize = SHA1_DGSTSIZE;
	(*dgst->create)(dgst->ctx);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_sha256_constructor(xsMachine *the)
{
#if !FSK_NO_SHA256
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct sha256), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha256_update;
	dgst->close = (cryptDigestCloseProc)sha256_fin;
	dgst->create = (cryptDigestCreateProc)sha256_create;
	dgst->blockSize = SHA256_BLKSIZE;
	dgst->outputSize = SHA256_DGSTSIZE;
	(*dgst->create)(dgst->ctx);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_sha512_constructor(xsMachine *the)
{
#if !FSK_NO_SHA512
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct sha512), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha512_update;
	dgst->close = (cryptDigestCloseProc)sha512_fin;
	dgst->create = (cryptDigestCreateProc)sha512_create;
	dgst->blockSize = SHA512_BLKSIZE;
	dgst->outputSize = SHA512_DGSTSIZE;
	(*dgst->create)(dgst->ctx);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_md5_constructor(xsMachine *the)
{
#if !FSK_NO_MD5
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct md5), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)md5_update;
	dgst->close = (cryptDigestCloseProc)md5_fin;
	dgst->create = (cryptDigestCreateProc)md5_create;
	dgst->blockSize = MD5_BLKSIZE;
	dgst->outputSize = MD5_DGSTSIZE;
	(*dgst->create)(dgst->ctx);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_des_constructor(xsMachine *the)
{
#if !FSK_NO_DES
	void *key;
	xsIntegerValue keySize;
	cryptBlockCipher *blkcphr;
	struct des_context *desc;
	FskErr err;

	getChunkData(the, &xsArg(0), &key, &keySize);
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNewClear(sizeof(struct des_context), (FskMemPtr *)&desc)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	desc->direction = -1;
	if (keySize > 8) keySize = 8;
	FskMemCopy(desc->key, key, keySize);
	blkcphr->ctx = desc;
	blkcphr->keySize = 8;
	blkcphr->blockSize = 8;
	blkcphr->encrypt = kcl_des_encrypt;
	blkcphr->decrypt = kcl_des_decrypt;
	xsSetHostData(xsThis, blkcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_tdes_constructor(xsMachine *the)
{
#if !FSK_NO_TDES
	void *key;
	int keySize;
	cryptBlockCipher *blkcphr;
	struct tdes_context *tdes;
	FskErr err;

	getChunkData(the, &xsArg(0), &key, (xsIntegerValue *)&keySize);
	if (xsToInteger(xsArgc) > 1) {
		int tkeySize = xsToInteger(xsArg(1));
		if (tkeySize > keySize)
			cryptThrow("kCryptRangeError");
		keySize = tkeySize;
	}
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNewClear(sizeof(struct tdes_context), (FskMemPtr *)&tdes)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	tdes->direction = -1;
	if (keySize > 8*3)
		keySize = 8*3;
	FskMemCopy(tdes->key, key, keySize);
	if (keySize <= 8) {
		FskMemCopy(tdes->key + 8, key, keySize);
		FskMemCopy(tdes->key + 16, key, keySize);
	}
	else if (keySize <= 16)
		FskMemCopy(tdes->key + 16, key, 8);
	blkcphr->ctx = tdes;
	blkcphr->keySize = keySize;
	blkcphr->blockSize = 8;
	blkcphr->encrypt = kcl_tdes_encrypt;
	blkcphr->decrypt = kcl_tdes_decrypt;
	xsSetHostData(xsThis, blkcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_aes_constructor(xsMachine *the)
{
#if !FSK_NO_AES
	void *key;
	xsIntegerValue keySize;
	int blockSize = 16;
	cryptBlockCipher *blkcphr;
	struct aes_context *aes;
	FskErr err;

	getChunkData(the, &xsArg(0), &key, &keySize);
	if (xsToInteger(xsArgc) > 1) {
		xsIntegerValue tkeySize = xsToInteger(xsArg(1));
		if (tkeySize > keySize)
			cryptThrow("kCryptRangeError");
		keySize = tkeySize;
	}
	if (xsToInteger(xsArgc) > 2)
		blockSize = xsToInteger(xsArg(2));
	if ((keySize != 16 && keySize != 24 && keySize != 32) || (blockSize != 16))	/* the key scheduler can take 16, 24, 32 bytes block size but enc/dec processes are fixed to 16 for both time and size efficiency */
		cryptThrow("kCryptRangeError");
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNewClear(sizeof(struct aes_context), (FskMemPtr *)&aes)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	aes->Nk = keySize / 4;
	aes->Nb = blockSize / 4;
	aes->Nr = aes->Nk >= aes->Nb ? aes->Nk + 6 : aes->Nb + 6;
	aes->direction = -1;
	FskMemCopy(aes->key, key, keySize);
	blkcphr->ctx = aes;
	blkcphr->encrypt = kcl_aes_encrypt;
	blkcphr->decrypt = kcl_aes_decrypt;
	blkcphr->keySize = keySize;
	blkcphr->blockSize = blockSize;
	xsSetHostData(xsThis, blkcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_rc4_constructor(xsMachine *the)
{
#if !FSK_NO_RC4
	void *key;
	xsIntegerValue keySize;
	cryptStreamCipher *strmcphr;
	rc4_state *rc4;
	FskErr err;

	getChunkData(the, &xsArg(0), &key, &keySize);
	if (xsToInteger(xsArgc) > 1) {
		xsIntegerValue tkeySize = xsToInteger(xsArg(1));
		if (tkeySize > keySize)
			cryptThrow("kCryptRangeError");
		keySize = tkeySize;
	}
	if (keySize > 256)
		cryptThrow("kCryptRangeError");
	if ((err = FskMemPtrNew(sizeof(cryptStreamCipher), (FskMemPtr *)&strmcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(rc4_state), (FskMemPtr *)&rc4)) != kFskErrNone) {
		FskMemPtrDispose(strmcphr);
		cryptThrowFSK(err);
	}
	rc4_init(rc4, key, keySize);
	strmcphr->ctx = rc4;
	strmcphr->keySize = keySize;
	strmcphr->process = (cryptStreamCipherProcessProc)rc4_process;
	xsSetHostData(xsThis, strmcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}
