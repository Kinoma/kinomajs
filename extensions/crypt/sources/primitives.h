/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
/*
 * digest instance
 */

typedef void (*cryptDigestUpdateProc)(void *ctx, const void *data, UInt32 size);
typedef void (*cryptDigestCloseProc)(void *ctx, UInt8 *data);
typedef void (*cryptDigestCreateProc)(void *ctx);
typedef struct {
	void *ctx;
	UInt32 blockSize;
	UInt32 outputSize;
	cryptDigestUpdateProc update;
	cryptDigestCloseProc close;
	cryptDigestCreateProc create;
	FskInstrumentedItemDeclaration
} cryptDigest;


/*
 * block cipher instance
 */
enum cipher_direction {cipher_encryption, cipher_decryption};

typedef void (*cryptCipherProcessProc)(void *inData, void *outData, void *ctx);
typedef struct cryptBlockCipher {
	void *ctx;
	int keySize;
	int blockSize;
	cryptCipherProcessProc encrypt, decrypt;
	FskInstrumentedItemDeclaration
} cryptBlockCipher;


/*
 * stream cipher instance
 */

typedef void (*cryptStreamCipherProcessProc)(void *inout, UInt32 inoutLen, void *ctx);
typedef struct {
	void *ctx;
	int keySize;
	cryptStreamCipherProcessProc process;
	FskInstrumentedItemDeclaration
} cryptStreamCipher;

/*
 * encryption modes instance
 */

typedef int (*cryptEncryptionModeProc)(void *ctx, void *inData, void *outData, int size, int eofFlag);
typedef void (*cryptEncryptionModeSetIVProc)(void *ctx, void *data, UInt32 size, void *opt);
typedef void (*cryptEncryptionModeGetIVProc)(void *ctx, void **datap, UInt32 *sizep, void **optp);
typedef struct {
	cryptBlockCipher *blkcphr;
	SInt32 maxSlop;
	void *em_buf;
	UInt8 *em_tbuf1, *em_tbuf2, *em_tbuf3;
	cryptEncryptionModeProc encrypt, decrypt;
	cryptEncryptionModeSetIVProc setIV;
	cryptEncryptionModeGetIVProc getIV;
	/* mode depending value */
	SInt32 padLen;	/* for CBC and ECB mode */
	UInt32 ctr_counter, ctr_offset;	/* for CTR mode */
	FskInstrumentedItemDeclaration
} cryptEncryptionMode;

#if SUPPORT_INSTRUMENTATION

enum {
	kDigestInstrMsgUpdate = kFskInstrumentedItemFirstCustomMessage,
	kDigestInstrMsgClose,
	kDigestInstrMsgReset,
	kDigestInstrMsgGetBlockSize,
	kDigestInstrMsgGetOutputSize,
};

enum {
	kBlockCipherInstrMsgEncrypt = kFskInstrumentedItemFirstCustomMessage,
	kBlockCipherInstrMsgDecrypt,
	kBlockCipherInstrMsgGetKeySize,
	kBlockCipherInstrMsgGetBlockSize,
};

enum {
	kStreamCipherInstrMsgProcess = kFskInstrumentedItemFirstCustomMessage,
	kStreamCipherInstrMsgGetKeySize,
};

enum {
	kEncryptionModeInstrMsgEncrypt = kFskInstrumentedItemFirstCustomMessage,
	kEncryptionModeInstrMsgDecrypt,
	kEncryptionModeInstrMsgSetIV,
	kEncryptionModeInstrMsgGetIV,
};

#endif
