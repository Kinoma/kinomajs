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
#ifndef __KPRUTILITIES__
#define __KPRUTILITIES__

#include "expat.h"

#include "kpr.h"
#include "FskNetUtils.h"
#include "FskHeaders.h"
#include "kprHTTPServer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* UTILITIES */

FskAPI(void) KprAspectApply(UInt32 flags, FskRectangle srcRect, FskRectangle dstRect);
FskAPI(void) KprSerializeColor(xsMachine* the, FskConstColorRGBA color, xsSlot* slot);

FskAPI(UInt32) KprDateNow(void);
FskAPI(FskErr) KprDateFromHTTP(char* text, UInt32* date);
FskAPI(UInt32) KprEnvironmentGetUInt32(char* key, UInt32 it);

#if kprDumpMemory
#define KprMemPtrNew(size, newMemory) KprMemPtrNew_(size, (FskMemPtr *)(void *)(newMemory), __FILE__, __LINE__)
#define KprMemPtrNewClear(size, newMemory) KprMemPtrNewClear_(size, (FskMemPtr *)(void *)(newMemory), __FILE__, __LINE__)
#define KprMemPtrRealloc(size, newMemory) KprMemPtrRealloc_(size, (FskMemPtr *)(void *)(newMemory), __FILE__, __LINE__)
#define KprMemPtrNewFromData(size, data, newMemory) KprMemPtrNewFromData_(size, data, (FskMemPtr *)(void *)(newMemory), __FILE__, __LINE__)
#define KprMemPtrDispose(ptr) KprMemPtrDispose_((void *)(ptr), __FILE__, __LINE__)
#define KprMemPtrDisposeAt(ptr) KprMemPtrDisposeAt_((void **)(void *)(ptr), __FILE__, __LINE__)
#define KprStrDoCopy(str) KprStrDoCopy_(str, __FILE__, __LINE__)
#else /* !kprDumpMemory */
#define KprMemPtrNew(size, newMemory) FskMemPtrNew(size, newMemory)
#define KprMemPtrNewClear(size, newMemory) FskMemPtrNewClear(size, newMemory)
#define KprMemPtrRealloc(size, newMemory) FskMemPtrRealloc(size, newMemory)
#define KprMemPtrNewFromData(size, data, newMemory) FskMemPtrNewFromData(size, data, newMemory)
#define KprMemPtrDispose(ptr) FskMemPtrDispose(ptr)
#define KprMemPtrDisposeAt(ptr) FskMemPtrDisposeAt(ptr)
#define KprStrDoCopy(str) FskStrDoCopy(str)
#endif /* !kprDumpMemory */

FskAPI(FskErr) KprMemPtrNew_(UInt32 size, FskMemPtr *newMemory, char* file, int line);
FskAPI(FskErr) KprMemPtrNewClear_(UInt32 size, FskMemPtr *newMemory, char* file, int line);
FskAPI(FskErr) KprMemPtrRealloc_(UInt32 size, FskMemPtr *newMemory, char* file, int line);
FskAPI(FskErr) KprMemPtrNewFromData_(UInt32 size, const void *data, FskMemPtr *newMemory, char* file, int line);
FskAPI(void) KprMemPtrDispose_(void *ptr, char* file, int line);
FskAPI(void) KprMemPtrDisposeAt_(void **ptr, char* file, int line);
FskAPI(char*) KprStrDoCopy_(const char *str, char* file, int line);

FskAPI(FskErr) FskImageDecompressDataWithOrientation(const void *data, UInt32 dataSize, const char *mimeType, const char *extension, UInt32 targetWidth, UInt32 targetHeight, FskImageDecompressComplete completion, void *completionRefcon, FskBitmap *bits);
FskAPI(FskErr) FskImageDecompressBitmap(const void *data, UInt32 dataSize, const char *mimeType, const char *extension, UInt32 width, UInt32 height, FskBitmap *bits);
FskAPI(FskErr) FskPortBitmapProject(FskPort port, FskBitmap srcBits, FskRectangle srcRect, float transform[3][3]);

FskAPI(FskErr) FskStrB64Decode(const char *src, UInt32 srcSize, char **pDst, UInt32 *pDstSize);
FskAPI(char*) FskStrStrCaseInsensitive(const char *s1, const char *s2);
FskAPI(void) FskRectangleScaleToFill(FskConstRectangle containing, FskConstRectangle containee, FskRectangle fillOut);
FskAPI(Boolean) FskRectangleIsIntersectionNotEmpty(FskConstRectangle a, FskConstRectangle b);

FskAPI(FskErr) KprCryptMD5(void* input, UInt32 size, UInt8* binary, char* output);
//FskAPI(FskErr) KprCryptSHA1(void* input, UInt32 size, UInt8* binary, char* output);
//
//#define KPR_CRYPT_SHA1_BLOCK_SIZE SHA1_BLKSIZE
//#define KPR_CRYPT_SHA1_HASH_SIZE SHA1_DGSTSIZE
//
//typedef struct {
//	struct sha1 state;
//	unsigned char k_opad[KPR_CRYPT_SHA1_BLOCK_SIZE];
//} KprCryptHMAC_SHA1ContextRecord, *KprCryptHMAC_SHA1Context;
//
//FskAPI(FskErr) KprCryptHMAC_SHA1(const void *key, UInt32 keySize, const void *data, UInt32 dataSize, void *output);
//FskAPI(void) KprCryptHMACInput_SHA1(KprCryptHMAC_SHA1Context context, const void *data, UInt32 dataSize);
//FskAPI(void) KprCryptHMACReset_SHA1(KprCryptHMAC_SHA1Context context, const void *key, UInt32 keySize);
//FskAPI(void) KprCryptHMACResult_SHA1(KprCryptHMAC_SHA1Context context, void *output);
//
//FskAPI(FskErr) KprCryptPKCS5_PBKDF2_HMAC_SHA1(const char *password, UInt32 passwordSize, const unsigned char *salt, UInt32 saltSize, UInt32 iteration, UInt32 size, unsigned char* output);
//
//FskAPI(void) KprCryptXOR(void* input1, void* input2, UInt32 size, char* output);

FskAPI(char*) KprMachineName(void);
FskAPI(void) KprEnsureDirectory(char* path);

// XML Helper

typedef struct KprXMLParserStruct KprXMLParserRecord, *KprXMLParser;
typedef struct KprXMLElementStruct KprXMLElementRecord, *KprXMLElement;
typedef struct KprXMLAttributeStruct KprXMLAttributeRecord, *KprXMLAttribute;

//--------------------------------------------------
// XML Parser
//--------------------------------------------------

struct KprXMLParserStruct {
	XML_Parser expat;
	KprXMLElement element;
	KprXMLElement root;
};

FskAPI(FskErr) KprXMLParse(KprXMLElement* root, unsigned char *data, FskInt64 size);
FskAPI(FskErr) KprXMLSerialize(KprXMLElement element, char** data, UInt32* size);

//--------------------------------------------------
// XML Element
//--------------------------------------------------

struct KprXMLElementStruct {
	KprXMLElement next;
	KprXMLElement owner;
	char* name;
	KprXMLAttribute attribute;
	KprXMLElement element;
	KprXMLAttribute nameSpace;
	char* value;
	UInt32 valueSize;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprXMLElementNew(KprXMLElement *it, KprXMLElement owner, const char* name, const char** attributes);
FskAPI(void) KprXMLElementDispose(KprXMLElement self);
FskAPI(char*) KprXMLElementGetAttribute(KprXMLElement self, const char* name);
KprXMLElement KprXMLElementGetFirstElement(KprXMLElement self, const char* nameSpace, const char* name);
FskAPI(SInt32) KprXMLElementGetIntegerValue(KprXMLElement self);
KprXMLAttribute KprXMLElementGetNamespace(KprXMLElement self, const char* name);
KprXMLElement KprXMLElementGetNextElement(KprXMLElement self, const char* nameSpace, const char* name);
FskAPI(char*) KprXMLElementGetProperty(KprXMLElement self, const char* nameSpace, const char* name);
FskAPI(char*) KprXMLElementGetValue(KprXMLElement self);
FskAPI(Boolean) KprXMLElementIsEqual(KprXMLElement self, const char* nameSpace, const char* name);
FskAPI(FskErr) KprXMLElementSetAttributeValue(KprXMLElement self, const char* name, const char* value);

//--------------------------------------------------
// XML Attribute
//--------------------------------------------------

struct KprXMLAttributeStruct {
	KprXMLAttribute next;
	Boolean isNamespace;
	char* name;
	KprXMLAttribute nameSpace;
	char* value;
	FskInstrumentedItemDeclaration
};

/** --------------------------------------------------
 * KprRetainable
 * --------------------------------------------------- */

typedef struct KprRetainableRecord KprRetainableRecord, *KprRetainable;

struct KprRetainableRecord {
	UInt32 retainCount;
};

FskAPI(FskErr) KprRetainableNew(KprRetainable *it);
FskAPI(FskErr) KprRetainableDispose(KprRetainable self);
FskAPI(void) KprRetainableRetain(KprRetainable self);
FskAPI(Boolean) KprRetainableRelease(KprRetainable self);

#define KprRetain(x) (KprRetainableRetain((x)->retainable), x)

/** --------------------------------------------------
 * KprMemoryBlock
 * --------------------------------------------------- */

typedef struct KprMemoryBlockRecord KprMemoryBlockRecord, *KprMemoryBlock;

struct KprMemoryBlockRecord {
	KprMemoryBlock next;
	UInt32 size;
	KprRetainable retainable;
};

FskAPI(FskErr) KprMemoryBlockNew(UInt32 size, const void *data, KprMemoryBlock *it);
FskAPI(FskErr) KprMemoryBlockDispose(KprMemoryBlock self);
FskAPI(FskErr) KprMemoryBlockDisposeAt(KprMemoryBlock *it);

FskAPI(KprMemoryBlock) KprMemoryBlockRetain(KprMemoryBlock self);

FskAPI(void) *KprMemoryBlockStart(KprMemoryBlock self);
FskAPI(void) *KprMemoryBlockEnd(KprMemoryBlock self);

FskAPI(void *) KprMemoryBlockCopyTo(KprMemoryBlock self, void *dest);
FskAPI(FskErr) KprMemoryBlockToChunk(KprMemoryBlock self, xsMachine *the, xsSlot *ref);

FskAPI(Boolean) KprMemoryBlockIsSame(KprMemoryBlock a, KprMemoryBlock b);

/** --------------------------------------------------
 * KprSocketUtilities
 * --------------------------------------------------- */

typedef enum {
	kKprSocketErrorOnRead = 1,
	kKprSocketErrorOnWrite = 2,
	kKprSocketErrorOnListen = 3,
} KprSocketErrorContext;

typedef void (*KprSocketErrorCallback)(KprSocketErrorContext context, FskErr err, void *refcon);

/** --------------------------------------------------
 * KprSocketReader
 *
 * This class is for asynchronous event based socket
 * reading. Whole reading is non-bocking and callback
 * of proper state will be called if some data is ready.
 *
 * @author Basuke Suzuki
 * @see KprSocketWriter, KprWebSocketEndpoint
 * --------------------------------------------------- */

typedef struct KprSocketReaderRecord KprSocketReaderRecord, *KprSocketReader;
typedef struct KprSocketReaderState KprSocketReaderState;

typedef FskErr (*KprSocketReaderCallback)(KprSocketReader self, void *refcon);

struct KprSocketReaderRecord {
	FskSocket socket;
	void *refcon;
	Boolean inThreadDataHandler;
	Boolean disposeRequested;
	Boolean closed;
	Boolean stateInitialized;

	KprSocketErrorCallback errorCallback;

	FskThreadDataHandler handler;
	UInt32 stateCount;
	KprSocketReaderState *states;
	int state;

	UInt32 bufferLength;
	UInt8 *leftover;
	UInt32 leftoverLength;

	FskTimeRecord lastDataArrived;
};

struct KprSocketReaderState {
	int state;
	KprSocketReaderCallback callback;
};

FskAPI(FskErr) KprSocketReaderNew(KprSocketReader *reader, FskSocket skt, KprSocketReaderState *states, UInt32 stateCount, void *refcon);
FskAPI(FskErr) KprSocketReaderDispose(KprSocketReader reader);

FskAPI(void) KprSocketReaderSetState(KprSocketReader self, int state);
FskAPI(void) KprSocketReaderResetRead(KprSocketReader self);

FskAPI(FskErr) KprSocketReaderReadBytes(KprSocketReader self, void *buffer, size_t targetSize);
FskAPI(FskErr) KprSocketReaderReadHTTPHeaders(KprSocketReader self, FskHeaders *headers);
FskAPI(FskErr) KprSocketReaderUnreadBytes(KprSocketReader self, void *buffer, size_t targetSize);

FskAPI(FskErr) KprSocketReaderReadDataFrom(KprSocketReader self, void *buffer, UInt32 *size, UInt32 *remoteIP, UInt16 *remotePort);

/** --------------------------------------------------
 * KprSocketWriter
 *
 * This class is for asynchronous socket sender with
 * automatic socket saturation support, which means
 * if connection is sending too much data and socket
 * can not send such amount of data right now, it will
 * be stored in the buffer and resend later when socket
 * can accept more data.
 *
 * @author Basuke Suzuki
 * @see KprSocketReader, KprWebSocketEndpoint
 * --------------------------------------------------- */

typedef struct KprSocketWriterRecord KprSocketWriterRecord, *KprSocketWriter;

struct KprSocketWriterRecord {
	FskSocket socket;
	void *refcon;
	Boolean inThreadDataHandler;
	Boolean disposeRequested;

	KprSocketErrorCallback errorCallback;

	FskThreadDataHandler handler;
	UInt8 *pendingData;
	UInt32 pendingLength;

	UInt32 targetIP;
	UInt16 targetPort;

	FskTimeRecord lastDataSent;
};

FskAPI(FskErr) KprSocketWriterNew(KprSocketWriter *writer, FskSocket skt, void *refcon);
FskAPI(FskErr) KprSocketWriterDispose(KprSocketWriter writer);

FskAPI(void) KprSocketWriterSetDestination(KprSocketWriter writer, UInt32 ip, UInt16 port); // For UDP

FskAPI(void) KprSocketWriterSendBytes(KprSocketWriter writer, void *bytes, UInt32 size);
FskAPI(void) KprSocketWriterSendChunk(KprSocketWriter writer, KprMemoryBlock chunk);

/** --------------------------------------------------
 * KprSocketServer
 *
 * This class open the listing socket for each network
 * interface. Managing up/down of interfaces.
 *
 * @author Basuke Suzuki
 * @see KprMQTTBroker
 * --------------------------------------------------- */

typedef struct KprSocketServerRecord KprSocketServerRecord, *KprSocketServer;
typedef struct KprPortListenerRecord KprPortListenerRecord, *KprPortListener;

typedef FskErr (*KprSocketServerAcceptCallback)(KprSocketServer server, FskSocket skt, const char *interfaceName, UInt32 ip, void *refcon);
typedef void (*KprSocketServerInterfaceDroppedCallback)(KprSocketServer server, const char *interfaceName, UInt32 ip, void *refcon);

struct KprSocketServerRecord {
	KprPortListener listeners;
	KprNetworkInterfaceNotifier notifier;
	UInt16 port;
	Boolean all;
	void *refcon;
	char /* @weak */ *debugName;

	KprSocketServerAcceptCallback acceptCallback;
	KprSocketServerInterfaceDroppedCallback interfaceDroppedCallback;
	KprSocketErrorCallback errorCallback;
};

struct KprPortListenerRecord {
	KprPortListener next;
	KprSocketServer /* @weak */ server;
	FskSocket socket;
	char *interfaceName;
	int ip;
	FskThreadDataHandler dataHandler;
};

FskAPI(FskErr) KprSocketServerNew(KprSocketServer *server, void *refcon);
FskAPI(FskErr) KprSocketServerDispose(KprSocketServer server);
FskAPI(FskErr) KprSocketServerListen(KprSocketServer server, UInt16 port, const char *interfaceName);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPRUTILITIES__ */


