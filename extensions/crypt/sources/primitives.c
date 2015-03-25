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
 * Digest algorithms
 */

void
xs_digest_destructor(void *data)
{
	if (data) {
		cryptDigest *dgst = data;
		FskMemPtrDispose(dgst->ctx);
		FskInstrumentedItemDispose(dgst);
		FskMemPtrDispose(dgst);
	}
}

void
xs_digest_update(xsMachine *the)
{
	cryptDigest *dgst = xsGetHostData(xsThis);
	void *data;
	xsIntegerValue dataLen;

	FskInstrumentedItemSendMessageNormal(dgst, kDigestInstrMsgUpdate, NULL);
	getInputData(the, &xsArg(0), &data, &dataLen);
	if (data != NULL && dataLen > 0)
		(*dgst->update)(dgst->ctx, data, (UInt32)dataLen);
}

void
xs_digest_close(xsMachine *the)
{
	cryptDigest *dgst = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);

	FskInstrumentedItemSendMessageNormal(dgst, kDigestInstrMsgClose, NULL);
	if (ac >= 1 && xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		xsResult = xsArg(0);
		xsSet(xsResult, xsID("length"), xsInteger(dgst->outputSize));
		(*dgst->close)(dgst->ctx, xsGetHostData(xsResult));
	}
	else if (ac >= 1 && xsTest(xsArg(0))) {
		unsigned char *out, *op;
		char *hex, *hp;
		int n;
		FskErr err;
		if ((err = FskMemPtrNew(dgst->outputSize, (FskMemPtr *)&out)) != kFskErrNone)
			cryptThrowFSK(err);
		if ((err = FskMemPtrNew(dgst->outputSize * 2 + 1, (FskMemPtr *)&hex)) != kFskErrNone) {
			FskMemPtrDispose(out);
			cryptThrowFSK(err);
		}
		(*dgst->close)(dgst->ctx, out);
		for (n = dgst->outputSize, hp = hex, op = out; --n >= 0;) {
			unsigned char c;
#define TOHEX(h)	(c = (h) & 0xf, c < 10 ? c + '0': c - 10 + 'a')
			*hp++ = TOHEX(*op >> 4);
			*hp++ = TOHEX(*op++);
		}
		*hp = '\0';
		xsResult = xsString(hex);
		FskMemPtrDispose(hex);
		FskMemPtrDispose(out);
	}
	else {
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(dgst->outputSize));
		(*dgst->close)(dgst->ctx, xsGetHostData(xsResult));
	}
}

void
xs_digest_reset(xsMachine *the)
{
	cryptDigest *dgst = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(dgst, kDigestInstrMsgReset, NULL);
	(*dgst->create)(dgst->ctx);
}

void
xs_digest_getBlockSize(xsMachine *the)
{
	cryptDigest *dgst = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(dgst, kDigestInstrMsgGetBlockSize, NULL);
	xsResult = xsInteger(dgst->blockSize);
}

void
xs_digest_getOutputSize(xsMachine *the)
{
	cryptDigest *dgst = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(dgst, kDigestInstrMsgGetOutputSize, NULL);
	xsResult = xsInteger(dgst->outputSize);
}


/*
 * block cipher algorithms
 */

void
xs_blockCipher_destructor(void *data)
{
	if (data) {
		cryptBlockCipher *blkcphr = data;
		FskMemPtrDispose(blkcphr->ctx);
		FskInstrumentedItemDispose(blkcphr);
		FskMemPtrDispose(blkcphr);
	}
}

void
xs_blockCipher_encrypt(xsMachine *the)
{
	cryptBlockCipher *blkcphr = xsGetHostData(xsThis);
	void *inData, *outData;
	xsIntegerValue inSize, outSize;

	FskInstrumentedItemSendMessageNormal(blkcphr, kBlockCipherInstrMsgEncrypt, NULL);
	getInputData(the, &xsArg(0), &inData, &inSize);
	getChunkData(the, &xsArg(1), &outData, &outSize);
	if (inSize < blkcphr->blockSize || outSize < blkcphr->blockSize)
		cryptThrow("kCryptRangeError");
	(*blkcphr->encrypt)(inData, outData, blkcphr->ctx);
}

void
xs_blockCipher_decrypt(xsMachine *the)
{
	cryptBlockCipher *blkcphr = xsGetHostData(xsThis);
	void *inData, *outData;
	xsIntegerValue inSize, outSize;

	FskInstrumentedItemSendMessageNormal(blkcphr, kBlockCipherInstrMsgDecrypt, NULL);
	getInputData(the, &xsArg(0), &inData, &inSize);
	getChunkData(the, &xsArg(1), &outData, &outSize);
	if (inSize < blkcphr->blockSize || outSize < blkcphr->blockSize)
		cryptThrow("kCryptRangeError");
	(*blkcphr->decrypt)(inData, outData, blkcphr->ctx);
}

void
xs_blockCipher_getKeySize(xsMachine *the)
{
	cryptBlockCipher *blkcphr = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(blkcphr, kBlockCipherInstrMsgGetKeySize, NULL);
	xsResult = xsInteger(blkcphr->keySize);
}

void
xs_blockCipher_getBlockSize(xsMachine *the)
{
	cryptBlockCipher *blkcphr = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(blkcphr, kBlockCipherInstrMsgGetBlockSize, NULL);
	xsResult = xsInteger(blkcphr->blockSize);
}


/*
 * stream cipher
 */

void
xs_streamCipher_destructor(void *data)
{
	if (data) {
		cryptStreamCipher *strmcipher = data;
		FskMemPtrDispose(strmcipher->ctx);
		FskInstrumentedItemDispose(strmcipher);
		FskMemPtrDispose(strmcipher);
	}
}

void
xs_streamCipher_process(xsMachine *the)
{
	cryptStreamCipher *strmcipher = xsGetHostData(xsThis);
	UInt8 *inData, *outData;
	xsIntegerValue inSize, outSize;

	FskInstrumentedItemSendMessageNormal(strmcipher, kStreamCipherInstrMsgProcess, NULL);
	getInputData(the, &xsArg(0), (void **)&inData, &inSize);
	if (xsToInteger(xsArgc) > 1 && xsIsInstanceOf(xsArg(1), xsChunkPrototype)) {
		getChunkData(the, &xsArg(1), (void **)&outData, &outSize);
		xsResult = xsArg(1);
		if (inSize > outSize)
			inSize = outSize;
	}
	else {
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(inSize));
		outData = xsGetHostData(xsResult);
		outSize = inSize;
	}
	FskMemCopy(outData, inData, inSize);
	(*strmcipher->process)(outData, inSize, strmcipher->ctx);
}

void
xs_streamCipher_getKeySize(xsMachine *the)
{
	cryptStreamCipher *strmcipher = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(strmcipher, kStreamCipherInstrMsgGetKeySize, NULL);
	xsResult = xsInteger(strmcipher->keySize);
}


/*
 * encryption modes
 */
#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageEncryptionMode(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gEncryptionModeTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"crypt.encryptionMode",
	FskInstrumentationOffset(cryptEncryptionMode),
	NULL,
	0,
	NULL,
	doFormatMessageEncryptionMode
};

static char *gCbcName = "cbc";
static char *gCtrName = "ctr";
static char *gEcbName = "ecb";
#endif

void
xs_encryptionMode_destructor(void *data)
{
	if (data) {
		cryptEncryptionMode *encmode = data;
		FskMemPtrDispose(encmode->em_buf);
		FskInstrumentedItemDispose(encmode);
		FskMemPtrDispose(encmode);
	}
}

static void
processMode(xsMachine *the, cryptEncryptionMode *encmode, cryptEncryptionModeProc proc, int maxSlop)
{
	int ac = xsToInteger(xsArgc);
	UInt8 *inData, *outData;
	xsIntegerValue inSize, outSize;
	Boolean eofFlag;
	int n;

	if (ac < 1)
		cryptThrow("kCryptTypeError");
	getInputData(the, &xsArg(0), (void **)&inData, &inSize);
	if (ac > 1 && xsIsInstanceOf(xsArg(1), xsChunkPrototype)) {
		/* NOTE: padding size should be taken into account */
		getChunkData(the, &xsArg(1), (void **)&outData, &outSize);
		if (outSize < inSize) {
			inSize = outSize;
			eofFlag = false;
		}
		else if (outSize < inSize + maxSlop)
			eofFlag = false;
		else if (ac > 2)
			eofFlag = xsToBoolean(xsArg(2));
		else
			eofFlag = true;
		xsResult = xsArg(1);
	}
	else if (ac > 1 && xsTest(xsArg(1))) {
		FskErr err = FskMemPtrNew(inSize + maxSlop + 1, (FskMemPtr *)&outData);
		if (err != kFskErrNone)
			cryptThrowFSK(err);
		eofFlag = true;
	}
	else {
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(inSize + maxSlop));
		outData = xsGetHostData(xsResult);
		eofFlag = true;
	}
	n = (*proc)(encmode, inData, outData, inSize, eofFlag);
	if (n < 0)
		cryptThrow("kCryptMalformedInput");
	if (xsIsInstanceOf(xsResult, xsChunkPrototype))
		xsSet(xsResult, xsID("length"), xsInteger(n));
	else {
		outData[n] = '\0';
		xsResult = xsString((char *)outData);
		FskMemPtrDispose(outData);
	}
}

void
xs_encryptionMode_encrypt(xsMachine *the)
{
	cryptEncryptionMode *encmode = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(encmode, kEncryptionModeInstrMsgEncrypt, NULL);
	processMode(the, encmode, encmode->encrypt, encmode->maxSlop);
}

void
xs_encryptionMode_decrypt(xsMachine *the)
{
	cryptEncryptionMode *encmode = xsGetHostData(xsThis);

	FskInstrumentedItemSendMessageNormal(encmode, kEncryptionModeInstrMsgDecrypt, NULL);
	processMode(the, encmode, encmode->decrypt, 0);
}

void
xs_encryptionMode_setIV(xsMachine *the)
{
	cryptEncryptionMode *encmode = xsGetHostData(xsThis);
	void *data;
	xsIntegerValue dataSize;

	FskInstrumentedItemSendMessageNormal(encmode, kEncryptionModeInstrMsgSetIV, NULL);
	if (encmode->setIV == NULL)
		cryptThrow("kCryptUnimplemented");
	getInputData(the, &xsArg(0), &data, &dataSize);
	(*encmode->setIV)(encmode, data, dataSize, NULL);
}

void
xs_encryptionMode_getIV(xsMachine *the)
{
	cryptEncryptionMode *encmode = xsGetHostData(xsThis);
	void *data;
	UInt32 dataSize;

	FskInstrumentedItemSendMessageNormal(encmode, kEncryptionModeInstrMsgGetIV, NULL);
	if (encmode->getIV == NULL)
		cryptThrow("kCryptUnimplemented");
	(*encmode->getIV)(encmode, &data, &dataSize, NULL);
	xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(dataSize));
	FskMemCopy(xsGetHostData(xsResult), data, dataSize);
}

static cryptEncryptionMode *
cryptEncryptionModeNew(xsMachine *the)
{
	cryptEncryptionMode *encmode;
	FskErr err;

	if (!xsIsInstanceOf(xsArg(0), xsGet(xsGet(xsGlobal, xsID("Crypt")), xsID("blockCipher"))))
		cryptThrow("kCryptTypeError");
	if ((err = FskMemPtrNew(sizeof(cryptEncryptionMode), (FskMemPtr *)&encmode)) != kFskErrNone)
		cryptThrowFSK(err);
	xsSet(xsThis, xsID("blkenc"), xsArg(0));	/* bind the argument to this object to avoid GC */
	encmode->blkcphr = xsGetHostData(xsArg(0));
	/* allocate the temporary buffer for convenience of each mode */
	if ((err = FskMemPtrNew(encmode->blkcphr->blockSize * 3, (FskMemPtr *)&encmode->em_buf)) != kFskErrNone) {
		FskMemPtrDispose(encmode);
		cryptThrowFSK(err);
	}
	return(encmode);
}

static UInt8 *
cryptXOR(UInt8 *t, UInt8 *x, UInt8 *y, int n)
{
	UInt8 *d = t;

	while (--n >= 0)
		*t++ = *x++ ^ *y++;
	return(d);
}

/*
 * CBC mode
 */
#define cbc_iv		em_tbuf1
#define cbc_tmpbuf	em_tbuf2
#define cbc_prevbuf	em_tbuf3

static int
cryptCBCModeEncrypt(cryptEncryptionMode *ctx, UInt8 *din, UInt8 *dout, SInt32 n, Boolean eofFlag)
{
	cryptCipherProcessProc proc = ctx->blkcphr->encrypt;
	UInt8 *prev = ctx->cbc_prevbuf;
	int blockSize = ctx->blkcphr->blockSize;
	UInt8 *doutSave = dout;

	if (n <= 0)
		return(0);
	while (n >= blockSize) {
		cryptXOR(prev, prev, din, blockSize);
		(*proc)(prev, dout, ctx->blkcphr->ctx);
		FskMemCopy(prev, dout, blockSize);
		n -= blockSize;
		din += blockSize;
		dout += blockSize;
	}
	if (eofFlag && ctx->padLen != 0) {
		/* process RFC2630 */
		UInt8 *pad = ctx->cbc_tmpbuf;
		int padbyte = blockSize - n;
		int i;
		for (i = 0; i < n; i++)
			pad[i] = din[i];
		for (; i < blockSize; i++)
			pad[i] = padbyte;
		cryptXOR(prev, prev, pad, blockSize);
		(*proc)(prev, dout, ctx->blkcphr->ctx);
		/* safe to write over given 'n' at EOF -- because it is guranteed that the chunk size is multiple of the block size, and the case of no padding (i.e. full block of padding) is taken into account even if the chunk buffer is full */
		dout += blockSize;
	}
	else if (eofFlag && n > 0) {
		/* just remain as is */
		FskMemMove(dout, din, n);
		dout += n;
	}
	return(dout - doutSave);
}

static int
cryptCBCModeDecrypt(cryptEncryptionMode *ctx, UInt8 *din, UInt8 *dout, SInt32 n, Boolean eofFlag)
{
	cryptCipherProcessProc proc = ctx->blkcphr->decrypt;
	int blockSize = ctx->blkcphr->blockSize;
	int i;
	UInt8 *doutSave = dout;

	if (n < blockSize)
		return(0);
	while (n >= blockSize) {
		UInt8 *prev = ctx->cbc_prevbuf;
		UInt8 *tmpbuf = ctx->cbc_tmpbuf;
		(*proc)(din, tmpbuf, ctx->blkcphr->ctx);
		for (i = blockSize; --i >= 0;) {
			int c = *prev;
			*prev++ = *din++;
			*dout++ = c ^ *tmpbuf++;
		}
		n -= blockSize;
	}
	if (eofFlag) {
		int padLen;
		if (ctx->padLen < 0) {
			/* process RFC2630 */
			padLen = *(dout-1);
			if (padLen > blockSize)
				return(-1);
		}
		else if (ctx->padLen > 0)
			padLen = ctx->padLen;
		else {
			if (n > 0) {
				FskMemMove(dout, din, n);
				dout += n;
			}
			padLen = 0;
		}
		dout -= padLen;
	}
	return(dout - doutSave);
}

static void
cryptCBCModeSetIV(cryptEncryptionMode *ctx, void *data, UInt32 size, void *opt)
{
	UInt32 blockSize = ctx->blkcphr->blockSize;
	UInt32 n = MIN(size, blockSize);

	if (n < size) {
		FskMemSet(ctx->cbc_iv, 0, blockSize);
		FskMemSet(ctx->cbc_prevbuf, 0, blockSize);
	}
	FskMemCopy(ctx->cbc_iv, data, n);
	FskMemCopy(ctx->cbc_prevbuf, ctx->cbc_iv, n);
}

static void
cryptCBCModeGetIV(cryptEncryptionMode *ctx, void **datap, UInt32 *sizep, void **optp)
{
	*datap = ctx->cbc_iv;
	*sizep = ctx->blkcphr->blockSize;
}

void
xs_cbc_constructor(xsMachine *the)
{
	cryptEncryptionMode *encmode = cryptEncryptionModeNew(the);
	UInt32 blockSize = encmode->blkcphr->blockSize;

	encmode->cbc_iv = encmode->em_buf;
	encmode->cbc_tmpbuf = encmode->cbc_iv + blockSize;
	encmode->cbc_prevbuf = encmode->cbc_tmpbuf + blockSize;
	FskMemSet(encmode->cbc_iv, 0, blockSize);
	FskMemSet(encmode->cbc_prevbuf, 0, blockSize);
	encmode->encrypt = (cryptEncryptionModeProc)cryptCBCModeEncrypt;
	encmode->decrypt = (cryptEncryptionModeProc)cryptCBCModeDecrypt;
	encmode->setIV = (cryptEncryptionModeSetIVProc)cryptCBCModeSetIV;
	encmode->getIV = (cryptEncryptionModeGetIVProc)cryptCBCModeGetIV;
	encmode->maxSlop = blockSize;
	encmode->padLen = xsToInteger(xsArgc) > 2 && xsTypeOf(xsArg(2)) == xsIntegerType ? xsToInteger(xsArg(2)): 0;
	FskInstrumentedItemNew(encmode, gCbcName, &gEncryptionModeTypeInstrumentation);
	xsSetHostData(xsThis, encmode);
	if (xsToInteger(xsArgc) > 1 && xsTest(xsArg(1)))
		(void)xsCall1(xsThis, xsID("setIV"), xsArg(1));
}

/*
 * CTR mode
 */
#define ctr_ctrbuf	em_tbuf1
#define ctr_outbuf	em_tbuf2

#if TARGET_RT_BIG_ENDIAN
/* must be aligned */
#define load32(p)	(*(UInt32 *)(p))
#define store32(p, v)	(*(UInt32 *)(p) = (v))
#else
#define _UL(x)		((UInt32)(x))
#define load32(p)	((_UL((p)[0]) << 24) | (_UL((p)[1]) << 16) | (_UL((p)[2]) << 8) | _UL((p)[3]))
#define _UB(x)		((UInt8)(x))
#define store32(p, v)	((p)[0] = _UB((v) >> 24), (p)[1] = _UB((v) >> 16), (p)[2] = _UB((v) >> 8), (p)[3] = _UB(v))
#endif

static int
cryptCTRModeProcess(cryptEncryptionMode *ctx, UInt8 *din, UInt8 *dout, SInt32 n, Boolean eofFlag)
{
	UInt8 *ctrbuf = ctx->ctr_ctrbuf;
	UInt8 *outbuf = ctx->ctr_outbuf;
	cryptCipherProcessProc proc = ctx->blkcphr->encrypt;	/* CTR mode uses encryption only */
	int blockSize = ctx->blkcphr->blockSize;
	int i;
	UInt8 *doutSave = dout;

	while (n > 0) {
		(*proc)(ctrbuf, outbuf, ctx->blkcphr->ctx);
		for (i = ctx->ctr_offset; n > 0 && i < blockSize; i++, --n)
			*dout++ = *din++ ^ outbuf[i];
		if (i == blockSize) {
			ctx->ctr_offset = 0;
			ctx->ctr_counter++;
			store32(&ctrbuf[blockSize - sizeof(UInt32)], ctx->ctr_counter);
			if (ctx->ctr_counter == 0) {
				/* round up */
				int j = blockSize - sizeof(UInt32);
				while ((j -= sizeof(UInt32)) >= 0) {
					UInt32 k;
					k = load32(&ctrbuf[j]);
					k++;
					store32(&ctrbuf[j], k);
					if (k != 0)
						break;
				}
			}
		}
		else
			ctx->ctr_offset = i;
	}
	return(dout - doutSave);
}

static void
cryptCTRModeSetIV(cryptEncryptionMode *ctx, UInt8 *data, UInt32 size, void *opt)
{
	UInt32 blockSize = ctx->blkcphr->blockSize;

	FskMemSet(ctx->ctr_ctrbuf, 0, blockSize);
	FskMemCopy(ctx->ctr_ctrbuf, data, MIN(size, blockSize));
	ctx->ctr_counter = load32(&ctx->ctr_ctrbuf[blockSize - sizeof(UInt32)]);
	if (opt != NULL)
		ctx->ctr_offset = *(UInt32 *)opt;
}

static void
cryptCTRModeGetIV(cryptEncryptionMode *ctx, UInt8 **datap, UInt32 *sizep, void **optp)
{
	*datap = ctx->ctr_ctrbuf;
	*sizep = ctx->blkcphr->blockSize;
	if (optp != NULL)
		*optp = (void **)&ctx->ctr_offset;
}

void
xs_ctr_constructor(xsMachine *the)
{
	cryptEncryptionMode *encmode = cryptEncryptionModeNew(the);
	UInt32 blockSize = encmode->blkcphr->blockSize;

	encmode->ctr_ctrbuf = encmode->em_buf;
	encmode->ctr_outbuf = encmode->ctr_ctrbuf + blockSize;
	FskMemSet(encmode->ctr_ctrbuf, 0, blockSize);
	FskMemSet(encmode->ctr_outbuf, 0, blockSize);
	encmode->encrypt = (cryptEncryptionModeProc)cryptCTRModeProcess;
	encmode->decrypt = (cryptEncryptionModeProc)cryptCTRModeProcess;
	encmode->setIV = (cryptEncryptionModeSetIVProc)cryptCTRModeSetIV;
	encmode->getIV = (cryptEncryptionModeGetIVProc)cryptCTRModeGetIV;
	encmode->ctr_counter = 0;
	encmode->ctr_offset = 0;
	encmode->maxSlop = 0;
	FskInstrumentedItemNew(encmode, gCtrName, &gEncryptionModeTypeInstrumentation);
	xsSetHostData(xsThis, encmode);
	if (xsToInteger(xsArgc) > 1 && xsTest(xsArg(1)))
		(void)xsCall1(xsThis, xsID("setIV"), xsArg(1));
}

/*
 * ECB mode
 */
#define ecb_pad	em_tbuf1

static int
cryptECBModeEncrypt(cryptEncryptionMode *ctx, UInt8 *din, UInt8 *dout, SInt32 n, Boolean eofFlag)
{
	cryptCipherProcessProc proc = ctx->blkcphr->encrypt;
	int blockSize = ctx->blkcphr->blockSize;
	UInt8 *doutSave = dout;

	if (n <= 0)
		return(0);
	while (n >= blockSize) {
		(*proc)(din, dout, ctx->blkcphr->ctx);
		n -= blockSize;
		din += blockSize;
		dout += blockSize;
	}
	if (eofFlag && ctx->padLen != 0) {
		/* process padding with a RFC2630 compatible way */
		UInt8 *pad = ctx->ecb_pad;
		int padByte = blockSize - n;
		int i;
		for (i = 0; i < n; i++)
			pad[i] = din[i];
		for (; i < blockSize; i++)
			pad[i] = padByte;
		(*proc)(pad, dout, ctx->blkcphr->ctx);
		dout += blockSize;
	}
	else if (eofFlag && n > 0) {
		/* just remian as is */
		FskMemMove(dout, din, n);
		dout += n;
	}
	return(dout - doutSave);
}

static int
cryptECBModeDecrypt(cryptEncryptionMode *ctx, UInt8 *din, UInt8 *dout, SInt32 n, Boolean eofFlag)
{
	cryptCipherProcessProc proc = ctx->blkcphr->decrypt;
	int blockSize = ctx->blkcphr->blockSize;
	UInt8 *doutSave = dout;

	if (n < blockSize)
		return(0);
	while (n >= blockSize) {
		(*proc)(din, dout, ctx->blkcphr->ctx);
		n -= blockSize;
		din += blockSize;
		dout += blockSize;
	}
	if (eofFlag) {
		int padLen;
		if (ctx->padLen < 0) {
			/* process padding */
			padLen = *(dout-1);
			if (padLen > blockSize)
				return(-1);
		}
		else if (ctx->padLen > 0)
			padLen = ctx->padLen;
		else {
			if (n > 0) {
				FskMemMove(dout, din, n);
				dout += n;
			}
			padLen = 0;
		}
		dout -= padLen;
	}
	return(dout - doutSave);
}

void
xs_ecb_constructor(xsMachine *the)
{
	cryptEncryptionMode *encmode = cryptEncryptionModeNew(the);
	UInt32 blockSize = encmode->blkcphr->blockSize;

	encmode->encrypt = (cryptEncryptionModeProc)cryptECBModeEncrypt;
	encmode->decrypt = (cryptEncryptionModeProc)cryptECBModeDecrypt;
	encmode->setIV = NULL;
	encmode->getIV = NULL;
	encmode->maxSlop = blockSize;
	encmode->padLen = xsToInteger(xsArgc) > 1 && xsTypeOf(xsArg(1)) == xsIntegerType ? xsToInteger(xsArg(1)): 0;
	encmode->ecb_pad = encmode->em_buf;
	FskInstrumentedItemNew(encmode, gEcbName, &gEncryptionModeTypeInstrumentation);
	xsSetHostData(xsThis, encmode);
}


/*
 * Random number generator
 */

#if !WM_FSK
#include "FskTime.h"
#endif

void
xs_rng_getTimeVariable(xsMachine *the)
{
	FskTimeRecord now;
	UInt8 *c;
	UInt32 t;
	xsIntegerValue size, i;
	XS_FNAME("Crypt.rng.getTimeVariable");

	FskTimeGetNow(&now);
	getInputData(the, &xsArg(0), (void **)&c, &size);
	t = now.useconds;
	for (i = 0; i < size && i < (xsIntegerValue)sizeof(t); i++, t >>= 8)
		c[i] = (UInt8)t;
	t = now.seconds;
	for (; i < size; i++, t >>= 8)
		c[i] = (UInt8)t;
}

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageEncryptionMode(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
	case kEncryptionModeInstrMsgEncrypt:
		snprintf(buffer, bufferSize, "Crypt.encryptionMode.encrypt");
		return true;

	case kEncryptionModeInstrMsgDecrypt:
		snprintf(buffer, bufferSize, "Crypt.encryptionMode.decrypt");
		return true;

	case kEncryptionModeInstrMsgSetIV:
		snprintf(buffer, bufferSize, "Crypt.encryptionMode.setIV");
		return true;

	case kEncryptionModeInstrMsgGetIV:
		snprintf(buffer, bufferSize, "Crypt.encryptionMode.getIV");
		return true;
	}

	return false;
}
#endif
