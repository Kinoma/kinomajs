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

/*
 * LibTomCrypt version
 */

#include "tomcrypt.h"
#include "md5.h"

typedef union {
	hash_state tomcrypt_hash_state;
	md5_hash_state md5_hash_state;
} crypt_hash_state;

/*
 * Digest algorithm constructors
 */
#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageDigest(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gDigestTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"crypt.digest",
	FskInstrumentationOffset(cryptDigest),
	NULL,
	0,
	NULL,
	doFormatMessageDigest
};

static char *gSha1Name = "sha1";
static char *gSha256Name = "sha256";
static char *gSha512Name = "sha512";
static char *gSha384Name = "sha384";
static char *gSha224Name = "sha224";
static char *gMd5Name = "md5";
#endif

void
xs_sha1_constructor(xsMachine *the)
{
#if !FSK_NO_SHA1
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha1_process;
	dgst->close = (cryptDigestCloseProc)sha1_done;
	dgst->create = (cryptDigestCreateProc)sha1_init;
	dgst->blockSize = 512/8;
	dgst->outputSize = 160/8;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gSha1Name, &gDigestTypeInstrumentation);
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
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha256_process;
	dgst->close = (cryptDigestCloseProc)sha256_done;
	dgst->create = (cryptDigestCreateProc)sha256_init;
	dgst->blockSize = 64;
	dgst->outputSize = 256/8;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gSha256Name, &gDigestTypeInstrumentation);
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
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha512_process;
	dgst->close = (cryptDigestCloseProc)sha512_done;
	dgst->create = (cryptDigestCreateProc)sha512_init;
	dgst->blockSize = 64;
	dgst->outputSize = 512/8;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gSha512Name, &gDigestTypeInstrumentation);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_sha384_constructor(xsMachine *the)
{
#if !FSK_NO_SHA384
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha384_process;
	dgst->close = (cryptDigestCloseProc)sha384_done;
	dgst->create = (cryptDigestCreateProc)sha384_init;
	dgst->blockSize = 64;
	dgst->outputSize = 384/8;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gSha384Name, &gDigestTypeInstrumentation);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_sha224_constructor(xsMachine *the)
{
#if !FSK_NO_SHA224
	cryptDigest *dgst;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(cryptDigest), (FskMemPtr *)&dgst)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)sha224_process;
	dgst->close = (cryptDigestCloseProc)sha224_done;
	dgst->create = (cryptDigestCreateProc)sha224_init;
	dgst->blockSize = 64;
	dgst->outputSize = 224/8;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gSha224Name, &gDigestTypeInstrumentation);
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
	if ((err = FskMemPtrNew(sizeof(crypt_hash_state), (FskMemPtr *)&dgst->ctx)) != kFskErrNone) {
		FskMemPtrDispose(dgst);
		cryptThrowFSK(err);
	}
	dgst->update = (cryptDigestUpdateProc)md5_process;
	dgst->close = (cryptDigestCloseProc)md5_done;
	dgst->create = (cryptDigestCreateProc)md5_init;
	dgst->blockSize = 64;
	dgst->outputSize = 16;
	(*dgst->create)(dgst->ctx);
	FskInstrumentedItemNew(dgst, gMd5Name, &gDigestTypeInstrumentation);
	xsSetHostData(xsThis, dgst);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}


/*
 * block cipher algorithms
 */
#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageBlockCipher(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gBlockCipherTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"crypt.blockCipher",
	FskInstrumentationOffset(cryptBlockCipher),
	NULL,
	0,
	NULL,
	doFormatMessageBlockCipher
};

static char *gDesName = "des";
static char *gTDesName = "tdes";
static char *gAesName = "aes";
#endif

void
xs_des_constructor(xsMachine *the)
{
#if !FSK_NO_DES
	void *key;
	xsIntegerValue keySize;
	cryptBlockCipher *blkcphr;
	FskErr err;

	getChunkData(the, &xsArg(0), &key, &keySize);
	if (keySize < 8)
		cryptThrow("kCryptRangeError");
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct des_key), (FskMemPtr *)&blkcphr->ctx)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	blkcphr->encrypt = (cryptCipherProcessProc)des_ecb_encrypt;
	blkcphr->decrypt = (cryptCipherProcessProc)des_ecb_decrypt;
	blkcphr->keySize = 8;
	blkcphr->blockSize = 8;
	des_setup(key, 8, 0, blkcphr->ctx);
	FskInstrumentedItemNew(blkcphr, gDesName, &gBlockCipherTypeInstrumentation);
	xsSetHostData(xsThis, blkcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

void
xs_tdes_constructor(xsMachine *the)
{
#if !FSK_NO_TDES
	UInt8 *key;
	xsIntegerValue keySize;
	cryptBlockCipher *blkcphr;
	FskErr err;
	UInt8 rkey[8*3];

	getChunkData(the, &xsArg(0), (void **)&key, &keySize);
	if (xsToInteger(xsArgc) > 1) {
		int tkeySize = xsToInteger(xsArg(1));
		if (tkeySize > keySize)
			cryptThrow("kCryptRangeError");
		keySize = tkeySize;
	}
	if (keySize < 8 * 2)
		cryptThrow("kCryptRangeError");
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct des3_key), (FskMemPtr *)&blkcphr->ctx)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	blkcphr->encrypt = (cryptCipherProcessProc)des3_ecb_encrypt;
	blkcphr->decrypt = (cryptCipherProcessProc)des3_ecb_decrypt;
	blkcphr->keySize = keySize;
	blkcphr->blockSize = 8;
	if (keySize < 8 * 3) {
		/* duplicate the first key to the last */
		FskMemCopy(rkey, key, 8*2);
		FskMemCopy(&rkey[8*2], key, 8);
		key = rkey;
	}
	des3_setup(key, 8*3, 0, blkcphr->ctx);
	FskInstrumentedItemNew(blkcphr, gTDesName, &gBlockCipherTypeInstrumentation);
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
	if ((keySize != 16 && keySize != 24 && keySize != 32) || blockSize != 16)
		cryptThrow("kCryptRangeError");
	if ((err = FskMemPtrNew(sizeof(cryptBlockCipher), (FskMemPtr *)&blkcphr)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(struct rijndael_key), (FskMemPtr *)&blkcphr->ctx)) != kFskErrNone) {
		FskMemPtrDispose(blkcphr);
		cryptThrowFSK(err);
	}
	blkcphr->encrypt = (cryptCipherProcessProc)rijndael_ecb_encrypt;
	blkcphr->decrypt = (cryptCipherProcessProc)rijndael_ecb_decrypt;
	blkcphr->keySize = keySize;
	blkcphr->blockSize = blockSize;
	rijndael_setup(key, keySize, 0, blkcphr->ctx);
	FskInstrumentedItemNew(blkcphr, gAesName, &gBlockCipherTypeInstrumentation);
	xsSetHostData(xsThis, blkcphr);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}


/*
 * stream cipher algorithms
 */
#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageStreamCipher(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gStreamCipherTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"crypt.streamCipher",
	FskInstrumentationOffset(cryptStreamCipher),
	NULL,
	0,
	NULL,
	doFormatMessageStreamCipher
};

static char *gRc4Name = "rc4";
#endif

void
xs_rc4_constructor(xsMachine *the)
{
#if !FSK_NO_RC4
	void *key;
	xsIntegerValue keySize;
	cryptStreamCipher *strmcipher;
	prng_state *prng;
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
	if ((err = FskMemPtrNew(sizeof(cryptStreamCipher), &strmcipher)) != kFskErrNone)
		cryptThrowFSK(err);
	if ((err = FskMemPtrNew(sizeof(prng_state), (FskMemPtr *)&prng)) != kFskErrNone) {
		FskMemPtrDispose(strmcipher);
		cryptThrowFSK(err);
	}
	rc4_start(prng);
	rc4_add_entropy(key, keySize, prng);
	rc4_ready(prng);
	strmcipher->ctx = prng;
	strmcipher->keySize = keySize;
	strmcipher->process = (cryptStreamCipherProcessProc)rc4_read;
	FskInstrumentedItemNew(strmcipher, gRc4Name, &gStreamCipherTypeInstrumentation);
	xsSetHostData(xsThis, strmcipher);
#else
	cryptThrow("kCryptUnimplemented");
#endif
}

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageDigest(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kDigestInstrMsgUpdate:
			snprintf(buffer, bufferSize, "Crypt.digest.update");
		   return true;

		case kDigestInstrMsgClose:
			snprintf(buffer, bufferSize, "Crypt.digest.close");
		   return true;

		case kDigestInstrMsgReset:
			snprintf(buffer, bufferSize, "Crypt.digest.reset");
			return true;

		case kDigestInstrMsgGetBlockSize:
			snprintf(buffer, bufferSize, "Crypt.digest.get blockSize");
			return true;

		case kDigestInstrMsgGetOutputSize:
			snprintf(buffer, bufferSize, "Crypt.digest.get outputSize");
			return true;
	}

	return false;
}

static Boolean doFormatMessageBlockCipher(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kBlockCipherInstrMsgEncrypt:
			snprintf(buffer, bufferSize, "Crypt.blockCipher.encrypt");
		   return true;

		case kBlockCipherInstrMsgDecrypt:
			snprintf(buffer, bufferSize, "Crypt.blockCipher.decrypt");
		   return true;

		case kBlockCipherInstrMsgGetKeySize:
			snprintf(buffer, bufferSize, "Crypt.blockCipher.get keySize");
			return true;

		case kBlockCipherInstrMsgGetBlockSize:
			snprintf(buffer, bufferSize, "Crypt.blockCipher.get blockSize");
			return true;
	}

	return false;
}

static Boolean doFormatMessageStreamCipher(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kStreamCipherInstrMsgProcess:
			snprintf(buffer, bufferSize, "Crypt.streamCipher.process");
		   return true;

		case kStreamCipherInstrMsgGetKeySize:
			snprintf(buffer, bufferSize, "Crypt.streamCipher.get keySize");
		   return true;
	}

	return false;
}
#endif
