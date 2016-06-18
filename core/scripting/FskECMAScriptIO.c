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
#define SUPPORT_SOCKET_STREAM	1

#define __FSKECMASCRIPT_PRIV__
#define __FSKNETUTILS_PRIV__
#include "FskECMAScript.h"
#ifdef KPR_CONFIG
#include "FskManifest.xs.h"
#define FskECMAScriptThrowIf(THE, ERROR) xsThrowIfFskErr(ERROR)
#else
#include "FskCore.xs.h"
#endif
#include "FskFiles.h"
#include "FskNetUtils.h"
#include "FskNetInterface.h"
#include "FskUtilities.h"
#include "FskHTTPClient.h"
#include "FskHTTPAuth.h"
#include "FskDIDLGenMedia.h"
#include "FskImage.h"
#include "FskMediaPlayer.h"
#include "FskThread.h"
#include "FskTextConvert.h"
#include "zlib.h"
#include "FskSSL.h"

#if TARGET_OS_WIN32
	#include "FskPlatformImplementation.h"
#endif

#define kXSMaxInt (0x80000000)

/*
	Stream
*/

typedef struct {
	union {
		FskFile fref;	/* cFileStream */
		long m;			/* cMemoryStream */
#if SUPPORT_SOCKET_STREAM
		FskSocket s;	/* cSocketStream */
#endif
	} u;
	char *wdata;
	char *wbuf;
	long wcount;
	long wsize;
	long tabcount;
	char *rdata;
	long rcount;
	long rindex;
	long rsize;
	FskInt64 bytesAvailable;
	unsigned long bytesWritten;
	Boolean	disableCache;

#if SUPPORT_SOCKET_STREAM
	// socket excitement
	Boolean socketConnected;
	Boolean socketWantsReadable;
	Boolean socketWantsWritable;
	Boolean	isSocket;
	Boolean keepalive;
	FskThreadDataHandler	handler;
	void *callbackData;
#if CLOSED_SSL
	void *ssl;
	Boolean isRaw;
#endif

	FskNetInterfaceNotifier netInterfaceNotifier;
#endif

	xsMachine *the;
	xsSlot obj;
	xsIndex id_buffer;
	xsIndex id_filename;
	xsIndex id_free;
	xsIndex id_mime;
	xsIndex id_length;
	xsIndex id_readChunk;	
	xsIndex id_readData;	
	xsIndex id_writeData;	
	xsIndex id_chunk;	
	xsIndex id_Chunk;	
	xsIndex id_FileSystem;	

	SInt32 bufferChunkSize;

	// gz stream
	z_stream			zlib;
	Boolean				initializedZlib;
	unsigned char		*gzInputBuffer;

	// chunk stream
	int incrementSize;
} StreamBuffer, *StreamBufferPtr;

#define kStreamBufferChunk (1024 * 4)
#define kStreamMaxWriteBuffer (1024 * 24)

static long StreamFromMemory(xsMachine* the, xsSlot thiss, StreamBufferPtr theStreamPtr, char *thePtr, long theSize);
static long StreamToMemory(xsMachine* the, xsSlot thiss, StreamBufferPtr theStreamPtr, char *thePtr, long theSize);
static Boolean StreamFlushData(xsMachine *the, xsSlot theStream, StreamBufferPtr theStreamPtr);

void xscStreamConstructor(xsMachine* the)
{
#ifndef KPR_CONFIG
	FskECMAScript vm = (FskECMAScript)xsGetContext(the);
#endif	
	StreamBufferPtr aStreamPtr;
	FskErr err = FskMemPtrNewClear(sizeof(StreamBuffer), &aStreamPtr);
	FskECMAScriptThrowIf(the, err);

	FskMemSet(aStreamPtr, 0, sizeof(StreamBuffer));
	xsSetHostData(xsThis, aStreamPtr);

	aStreamPtr->the = the;
	aStreamPtr->obj = xsThis;
#ifdef KPR_CONFIG
	aStreamPtr->id_buffer = xsID_buffer;
	aStreamPtr->id_filename = xsID_filename;
	aStreamPtr->id_free = xsID_free;
	aStreamPtr->id_mime = xsID_mime;
	aStreamPtr->id_length = xsID_length;
	aStreamPtr->id_readChunk = xsID_readChunk;	
	aStreamPtr->id_readData = xsID_readData;	
	aStreamPtr->id_writeData = xsID_writeData;	
	aStreamPtr->id_chunk = xsID_chunk;	
	aStreamPtr->id_Chunk = xsID_Chunk;	
	aStreamPtr->id_FileSystem = xsID_FileSystem;	
#else
	aStreamPtr->id_buffer = vm->id_buffer;
	aStreamPtr->id_filename = vm->id_filename;
	aStreamPtr->id_free = vm->id_free;
	aStreamPtr->id_mime = vm->id_mime;
	aStreamPtr->id_length = vm->id_length;
	aStreamPtr->id_readChunk = vm->id_readChunk;	
	aStreamPtr->id_readData = vm->id_readData;	
	aStreamPtr->id_writeData = vm->id_writeData;	
	aStreamPtr->id_chunk = vm->id_chunk;	
	aStreamPtr->id_Chunk = vm->id_Chunk;	
	aStreamPtr->id_FileSystem = vm->id_FileSystem;	
#endif	
	aStreamPtr->bufferChunkSize = kStreamBufferChunk;
	aStreamPtr->wsize = kStreamMaxWriteBuffer;

	xscStreamOpen(the);
}

void xscStreamAttach(xsMachine* the)
{
	xsTry {
		xsIndex id_attachData = xsID("attachData");
		StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
		if (!aStreamPtr)
			xsThrow(xsNewInstanceOf(xsErrorPrototype));
		switch (xsToInteger(xsArgc)) {
			case 0:
				xsCall0_noResult(xsThis, id_attachData);
				break;
			case 1:
				xsCall1_noResult(xsThis, id_attachData, xsArg(0));
				break;
			case 2:
				xsCall2_noResult(xsThis, id_attachData, xsArg(0), xsArg(1));
				break;
		}
	}
	xsCatch {		
		xsThrow(xsException);
	}
}

void xscStreamClose(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	if (aStreamPtr) {
		xsTry {
			xsCall0_noResult(xsThis, aStreamPtr->id_writeData);
			xsCall0_noResult(xsThis, xsID("closeData"));
			aStreamPtr->bytesAvailable = 0;

			xscStreamDestructor(aStreamPtr);
			xsSetHostData(xsThis, NULL);
		}
		xsCatch {
			xsCall0_noResult(xsThis, xsID("closeData"));
			xsThrow(xsException);
		}
	}
}

void xscStreamDestructor(void* theData)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)theData;

	if (aStreamPtr) {
		FskMemPtrDispose(aStreamPtr->wbuf);
		FskMemPtrDispose(aStreamPtr->rdata);
		FskMemPtrDispose(aStreamPtr);
	}
}

void xscStreamDetach(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	xsTry {
		if (!aStreamPtr)
			xsThrow(xsNewInstanceOf(xsErrorPrototype));
		
		xsCall0_noResult(xsThis, aStreamPtr->id_writeData);
		xsCall0_noResult(xsThis, xsID("detachData"));
	}
	xsCatch {
		if (aStreamPtr)
			xsCall0_noResult(xsThis, xsID("detachData"));
		xsThrow(xsException);
	}
}

void xscStreamFlush(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	Boolean ret;

	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	ret = StreamFlushData(the, xsThis, aStreamPtr);
	/* if we couldn't flush all data, what should we do? */
	xsResult = xsBoolean(ret);
}

void xscStreamReadCacheDispose(xsMachine* the)
{
    StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

    FskMemPtrDisposeAt((void **)(void*)&aStreamPtr->rdata);
    aStreamPtr->rcount = 0;
    aStreamPtr->rsize = 0;
    aStreamPtr->rindex = 0;
}

void xscStreamOpen(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	xsTry {
		if (!aStreamPtr)
			xsThrow(xsNewInstanceOf(xsErrorPrototype));
		aStreamPtr->bytesAvailable = 0;
		switch (xsToInteger(xsArgc)) {
			case 0:
				xsCall0_noResult(xsThis, xsID("openData"));
				break;
			case 1:
				xsCall1_noResult(xsThis, xsID("openData"), xsArg(0));
				break;
			case 2:
				xsCall2_noResult(xsThis, xsID("openData"), xsArg(0), xsArg(1));
				break;
			case 3:
				xsCall3_noResult(xsThis, xsID("openData"), xsArg(0), xsArg(1), xsArg(2));
				break;
			default:
				xsCall4_noResult(xsThis, xsID("openData"), xsArg(0), xsArg(1), xsArg(2), xsArg(3));
				break;
		}
	}
	xsCatch {
		xsThrow(xsException);
	}
}

void xscStreamReadChar(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aResult;
	UInt8 aChar = 0;

	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));

	aResult = StreamToMemory(the, xsThis, aStreamPtr, (char*)(&aChar), 1);
	if (1 == aResult)
		xsResult = xsInteger(aChar);
}

void xscStreamReadLine(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char aString[256];		// most lines will fit on the stack. for those that don't, we can overflow.
	long length = 0;
	char *overflow = NULL;
	long overflowSize = 0;
	Boolean done = false;
	Boolean eol = xsToInteger(xsArgc) > 0 && xsTest(xsArg(0));

	do {
		long aSize = sizeof(aString);
		char *anAddress = aString;
		while (aSize > 0) {
			long aCount = StreamToMemory(the, xsThis, aStreamPtr, anAddress, 1);
			if (aCount == 0) {
				done = true;
				break;
			}
			aSize--;
			length++;

			if (*anAddress == 0x0A) {
				*(anAddress--) = '\0';
				if (*anAddress == 0x0D)
					*anAddress-- = '\0';
				if (eol) *++anAddress = '\n';
				done = true;
				break;
			}
			anAddress++;
		}

		if (overflow && (sizeof(aString) != aSize)) {
			long newSize = (sizeof(aString) - aSize) + overflowSize;
			FskMemPtrRealloc(newSize + 1, &overflow);
			FskMemMove(overflow + overflowSize, aString, sizeof(aString) - aSize);
			overflowSize = newSize;
		}
		if (!overflow && (0 == aSize)) {
			FskECMAScriptThrowIf(the, FskMemPtrNew(sizeof(aString) + 1, &overflow));
			FskMemMove(overflow, aString, sizeof(aString));
			overflowSize = sizeof(aString);
		}
	} while (!done);

	if (0 != length) {
		if (NULL == overflow) {
			aString[length] = 0;
			xsResult = xsString(aString);
		}
		else {
			overflow[length] = 0;
			xsResult = xsString(overflow);
		}
	}
	else
		xsResult = xsNull;

	if (overflow)
		FskMemPtrDispose(overflow);
}

void xscStreamReadString(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char* aString = NULL;

	xsTry {
		SInt32 argc = xsToInteger(xsArgc);
		FskErr err;
		if (0 != argc) {
			long aSize = xsToInteger(xsArg(0));

			err = FskMemPtrNew(aSize + 1, &aString);
			FskECMAScriptThrowIf(the, err);
			
			aSize = StreamToMemory(the, xsThis, aStreamPtr, aString, aSize);
			if (0 != aSize) {
				aString[aSize] = '\0';
				if (!FskTextUTF8IsValid((unsigned char*)aString, aSize))
					xsThrow(xsNewInstanceOf(xsErrorPrototype));
				xsResult = xsString(aString);
			}
			else
				xsResult = xsUndefined;
			FskMemPtrDispose(aString);
		}
		else {
			UInt32 totalSize = 0, remaining = 0;
			long aSize;
			const UInt32 increment = 32768;

			while (true) {
				if (0 == remaining) {
					err = FskMemPtrRealloc(totalSize + increment + 1, &aString);
					FskECMAScriptThrowIf(the, err);
					remaining = increment;
				}
				aSize = StreamToMemory(the, xsThis, aStreamPtr, aString + totalSize, remaining);
				if (0 == aSize)
					break;
				totalSize += aSize;
				remaining -= aSize;
			}
			if (0 != totalSize) {
				aString[totalSize] = 0;
				if (!FskTextUTF8IsValid((unsigned char*)aString, totalSize))
					xsThrow(xsNewInstanceOf(xsErrorPrototype));
				xsResult = xsString(aString);
			}
			FskMemPtrDispose(aString);
		}
	}
	xsCatch {
		FskMemPtrDispose(aString);
		xsThrow(xsException);
	}
}

void xscStreamReset(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	aStreamPtr->wcount = 0;	/* @@ what to do for sockets? */
}

void xscStreamReturn(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char aChar;
	long aTabIndex;

	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));

	aChar = 0x0D;
	StreamFromMemory(the, xsThis, aStreamPtr, &aChar, 1);
	aChar = 0x0A;
	StreamFromMemory(the, xsThis, aStreamPtr, &aChar, 1);
	aChar = 0x09;
	for (aTabIndex = 0; aTabIndex < aStreamPtr->tabcount; aTabIndex++)
		StreamFromMemory(the, xsThis, aStreamPtr, &aChar, 1);
}

void xscStreamTab(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	aStreamPtr->tabcount++;
}

void xscStreamUntab(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	if (aStreamPtr->tabcount > 0)
		aStreamPtr->tabcount--;
}

void xscStreamWriteChar(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char aChar;
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));

	aChar = (char)xsToInteger(xsArg(0));
	xsResult = xsInteger(StreamFromMemory(the, xsThis, aStreamPtr, &aChar, 1));
}

void xscStreamWriteLine(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char* aString = NULL;
	char anEOF[3] = {0x0D, 0x0A, 0x00};
	int aResult, aStringLen;

	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
		
	aString = xsToStringCopy(xsArg(0));
	aStringLen = FskStrLen(aString);

	aResult = StreamFromMemory(the, xsThis, aStreamPtr, aString, aStringLen);
	aResult += StreamFromMemory(the, xsThis, aStreamPtr, anEOF, 2);
	xsResult = xsInteger(aResult);

	FskMemPtrDispose(aString);
}

void xscStreamWriteString(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char* aString = NULL;
	int aResult, aStringLen;
	
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	
	aString = xsToStringCopy(xsArg(0));
	aStringLen = FskStrLen(aString);
	aResult = StreamFromMemory(the, xsThis, aStreamPtr, aString, aStringLen);
	xsResult = xsInteger(aResult);
	FskMemPtrDispose(aString);
}

void xscStreamReadChunk(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aSize = xsToInteger(xsArg(0));
	long bSize;
	xsVars(1);
		
	if ((xsToInteger(xsArgc) >= 2) && (xsReferenceType == xsTypeOf(xsArg(1)))) {
		xsVar(0) = xsArg(1);
		xsSet(xsVar(0), aStreamPtr->id_length, xsInteger(aSize));
	}
	else
		xsVar(0) = xsNew1(xsGlobal, aStreamPtr->id_Chunk, xsInteger(aSize));

	bSize = StreamToMemory(the, xsThis, aStreamPtr, (char *)xsGetHostData(xsVar(0)), aSize);
	if (0 == bSize)
		xsResult = xsUndefined;
	else {
		xsResult = xsVar(0);
		if (bSize != aSize)
			xsSet(xsVar(0), aStreamPtr->id_length, xsInteger(bSize));
	}
}

void xscStreamWriteChunk(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aSize;
	int aResult;
	
	if (!aStreamPtr)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
		
	aSize = xsToInteger(xsGet(xsArg(0), aStreamPtr->id_length));
	aResult = StreamFromMemory(the, xsThis, aStreamPtr, (char *)xsGetHostData(xsArg(0)), aSize);
	xsResult = xsInteger(aResult);
}

void xscStreamGetBytesAvailable(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (aStreamPtr->bytesAvailable < kXSMaxInt)
		xsResult = xsInteger((xsIntegerValue)aStreamPtr->bytesAvailable);
	else
		xsResult = xsNumber((xsNumberValue)aStreamPtr->bytesAvailable);
}

void xscStreamGetBytesWritable(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	xsResult = xsInteger((xsIntegerValue)(aStreamPtr->wsize - aStreamPtr->wcount));
}

void xscStreamGetBytesWritten(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	xsResult = xsInteger(aStreamPtr->bytesWritten);
}

void xscStreamSeek(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	FskErr err;
	FskMemPtr temp = NULL;
	long seek = xsToInteger(xsArg(0));

	if (seek < 0)
		xsThrow(xsNewInstanceOf(xsErrorPrototype));

	err = FskMemPtrNew(1024, (FskMemPtr *)&temp);
	FskECMAScriptThrowIf(the, err);
	
	while (seek > 0) {
		long readSize = seek;
		if (readSize > 1024)
			readSize = 1024;

		if (readSize != StreamToMemory(the, xsThis, aStreamPtr, (char *)temp, readSize))
			break;

		seek -= readSize;
	}

	FskMemPtrDispose(temp);
}

static Boolean StreamFlushData(xsMachine *the, xsSlot theStream, StreamBufferPtr theStreamPtr)
{
	while (theStreamPtr->wcount > 0) {
		long aCount = theStreamPtr->wcount;
		xsCall0_noResult(theStream, theStreamPtr->id_writeData);
		aCount -= theStreamPtr->wcount;
		if (aCount <= 0)
			return false;
	}
	if (theStreamPtr->wbuf != NULL) {
		FskMemPtrDispose(theStreamPtr->wbuf);
		theStreamPtr->wbuf = NULL;
	}
	return true;
}

// private memory stream function
static long StreamFromMemory(xsMachine* the, xsSlot theStream, StreamBufferPtr theStreamPtr, char *thePtr, long theSize)
{
	FskErr err;

	/* try to flush the remaining data anyway */
	StreamFlushData(the, theStream, theStreamPtr);
	if (theStreamPtr->wcount > 0) {
		/* couldn't flush all data */
		if (theSize > theStreamPtr->wsize - theStreamPtr->wcount)
			xsThrow(xsNewInstanceOf(xsErrorPrototype));	/* sorry if you've expected the buffered data to be able to be flushed by now, but it's your fault not listen to me that we don't have much space! :< */
		FskMemMove(theStreamPtr->wbuf, theStreamPtr->wdata, theStreamPtr->wcount);
		theStreamPtr->wdata = theStreamPtr->wbuf;
		FskMemCopy(theStreamPtr->wdata + theStreamPtr->wcount, thePtr, theSize);
		theStreamPtr->wcount += theSize;
		goto bail;
	}

	theStreamPtr->wdata = thePtr;
	theStreamPtr->wcount = theSize;
	while (theStreamPtr->wcount > 0) {
		long aCount = theStreamPtr->wcount;
		xsCall0_noResult(theStream, theStreamPtr->id_writeData);		//@@
		aCount -= theStreamPtr->wcount;	
		if (aCount <= 0) {
			/* couldn't write even a byte */
			if (theStreamPtr->disableCache)
				break;	/* you really don't like cache? it's not going be so expensive only when writeData fails */
			if (theStreamPtr->wcount > theStreamPtr->wsize)
				xsThrow(xsNewInstanceOf(xsErrorPrototype));	/* I told you you can't put data more than that!!! :< */
			/* save the data into the write buffer and pretend it could successfully write the whole data */
			err = FskMemPtrNew(theStreamPtr->wsize, (FskMemPtr *)(void*)&theStreamPtr->wbuf);
			FskECMAScriptThrowIf(the, err);
			FskMemCopy(theStreamPtr->wbuf, theStreamPtr->wdata, theStreamPtr->wcount);
			theStreamPtr->wdata = theStreamPtr->wbuf;
			break;
		}
	}

bail:
	theStreamPtr->bytesWritten += theSize;
#if SUPPORT_SOCKET_STREAM
	if (!theStreamPtr->isSocket)
#endif
	theStreamPtr->bytesAvailable += theSize;	/* really necessary??? */
	return theSize;
}

static long StreamToMemory(xsMachine* the, xsSlot theStream, StreamBufferPtr theStreamPtr, char *thePtr, long theSize)
{
	FskErr err;
	long aResult;
	long aSize;

	if (theStreamPtr->disableCache) {
		theStreamPtr->rdata = thePtr;
		theStreamPtr->rcount = 0;
		theStreamPtr->rsize = theSize;

		{
			xsTry {
				xsResult = xsCall1(theStream, theStreamPtr->id_readData, xsInteger(theSize));
			}
			xsCatch {
				theStreamPtr->rdata = NULL;
				theStreamPtr->rcount = 0;
				xsThrow(xsException);
			}
		}
		if (xsTypeOf(xsResult) == xsReferenceType) {
			aSize = xsToInteger(xsGet(xsResult, theStreamPtr->id_length));
			if (aSize < theSize)
				theSize = aSize;
			FskMemCopy(thePtr, xsGetHostData(xsResult), theSize);
			xsCall0_noResult(xsResult, theStreamPtr->id_free);
		}
		else
			theSize = theStreamPtr->rcount;
		theStreamPtr->rdata = NULL;
		theStreamPtr->rcount = 0;
		theStreamPtr->rsize = 0;
#if SUPPORT_SOCKET_STREAM
		if (theStreamPtr->isSocket)
			theStreamPtr->bytesAvailable = !theStreamPtr->u.s || theStreamPtr->u.s->isEof ? -1: 0;
		else
#endif
		theStreamPtr->bytesAvailable -= theSize;
		return theSize;
	}

	if (theStreamPtr->rdata == NULL) {
		err = FskMemPtrNew(theStreamPtr->bufferChunkSize, (FskMemPtr *)(void*)&theStreamPtr->rdata);
		FskECMAScriptThrowIf(the, err);
		theStreamPtr->rsize = theStreamPtr->bufferChunkSize;
	}
	aResult = 0;
	while (theSize > 0) {
		if (theStreamPtr->rindex == theStreamPtr->rcount) {
			int typeOf;
			theStreamPtr->rcount = 0;
			theStreamPtr->rindex = 0;
			xsResult = xsCall1(theStream, theStreamPtr->id_readData, xsInteger(theStreamPtr->rsize));
			typeOf = xsTypeOf(xsResult);
			if (xsReferenceType == typeOf) {
				void* data = xsGetHostData(xsResult);
				UInt32 length = xsToInteger(xsGet(xsResult, theStreamPtr->id_length));
				UInt32 bsize = (length > (UInt32)theStreamPtr->rsize) ?
					(UInt32)theStreamPtr->rsize : length;
				FskMemCopy(theStreamPtr->rdata, data, bsize);
				theStreamPtr->rcount = bsize;
				theStreamPtr->rindex = 0;
				xsCall0_noResult(xsResult, theStreamPtr->id_free);
			}
			else if ((xsIntegerType == typeOf) || (xsNumberType == typeOf)) {
				goto bail;			// no data available at the moment
			}

			if (theStreamPtr->rcount == 0) /* EOF */ {
				// flush and try again
				xsCall0_noResult(theStream, xsID("flush"));
				xsCall0_noResult(theStream, theStreamPtr->id_readData);
				if (theStreamPtr->rcount == 0) /* EOF */
					break;
			}
		}
		aSize = theStreamPtr->rcount - theStreamPtr->rindex;
		if (aSize > theSize)
			aSize = theSize;
		FskMemMove(thePtr, theStreamPtr->rdata + theStreamPtr->rindex, aSize);
		theStreamPtr->rindex += aSize;
		thePtr += aSize;
		theSize -= aSize;
		aResult += aSize;
	}
#if SUPPORT_SOCKET_STREAM
	if (theStreamPtr->isSocket)
		theStreamPtr->bytesAvailable = !theStreamPtr->u.s || theStreamPtr->u.s->isEof ? -1: (theStreamPtr->rcount - theStreamPtr->rindex);
	else
#endif
	theStreamPtr->bytesAvailable -= aResult;
bail:
	xsResult = xsUndefined;

	return aResult;
}

typedef struct  {
	xsMachine* the;
	xsSlot thiss;
	StreamBufferPtr streamPtr;
	long size;
	xsSlot exception;
	int failed;
} sStreamGetC;

static int StreamGetC(sStreamGetC *theStream)
{
	char aChar = 0;
	int aResult = EOF;
	
	xsBeginHost(theStream->the);
	xsTry {
		if (theStream->size == -1) {
			if (StreamToMemory(theStream->the, theStream->thiss, theStream->streamPtr, &aChar, 1) == 1)
				aResult = (int)aChar;
		}
		else if ((theStream->size) > 0) {
			theStream->size--;
			if (StreamToMemory(theStream->the, theStream->thiss, theStream->streamPtr, &aChar, 1) == 1)
				aResult = (int)aChar;
		}
	}
	xsCatch {
		theStream->exception = xsException;
		xsRemember(theStream->exception);
		theStream->failed = 1;
	}
	xsEndHost(theStream->the);
	return aResult;
}

static int StreamPutC(char *theString, sStreamGetC *theStream)
{
	long aSize = (long)FskStrLen(theString);
	int aResult = EOF;

	xsBeginHost(theStream->the);
	xsTry {
		if (!theStream->failed)
			if (StreamFromMemory(theStream->the, theStream->thiss, theStream->streamPtr, theString, aSize) == aSize)
				aResult = 0;
	}
	xsCatch {
		theStream->exception = xsException;
		xsRemember(theStream->exception);
		theStream->failed = 1;
	}
	xsEndHost(theStream->the);
	return aResult;
}

void xscStreamSerialize(xsMachine* the)
{
	sStreamGetC aStream;

	aStream.the = the;
	aStream.thiss = xsThis;
	aStream.streamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStream.size = -1;
	aStream.exception = xsNull;
	aStream.failed = 0;
    if (NULL == aStream.streamPtr)
        xsThrowIfFskErr(kFskErrBadState);
	xsResult = xsSerialize(xsArg(0), &aStream, (xsPutter)StreamPutC);
	if (aStream.failed) {
		xsForget(aStream.exception);
		xsThrow(aStream.exception);
	}
}

void xscStreamParse(xsMachine* the)
{
	sStreamGetC aStream;

	aStream.the = the;
	aStream.thiss = xsThis;
	aStream.streamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
    if (NULL == aStream.streamPtr)
        xsThrowIfFskErr(kFskErrBadState);
	aStream.size = xsToInteger(xsArg(0));
	aStream.exception = xsNull;
	aStream.failed = 0;

	switch (xsToInteger(xsArgc) ) {
		case 1:
			xsResult = xsParse0(&aStream, (xsGetter)StreamGetC, "", 1, 0);
		break;
		case 2:
			xsResult = xsParse0(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)));
		break;
		case 3:
			xsResult = xsParse1(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)), xsArg(2));
		break;
		case 4:
			xsResult = xsParse2(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)), xsArg(2), xsArg(3));
		break;
		case 5:
			xsResult = xsParse3(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)), xsArg(2), xsArg(3), xsArg(4));
		break;
		case 6:
			xsResult = xsParse4(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)), xsArg(2), xsArg(3), xsArg(4), xsArg(5));
		break;
		case 7:
			xsResult = xsParse5(&aStream, (xsGetter)StreamGetC, "", 1, (xsFlag)xsToInteger(xsArg(1)), xsArg(2), xsArg(3), xsArg(4), xsArg(5), xsArg(6));
		break;
	}

	if (aStream.failed) {
		xsForget(aStream.exception);
		xsThrow(aStream.exception);
	}
}

// chunk stream

#ifndef roundup
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif

void xscChunkStreamAttachData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);

	aStreamPtr->disableCache = true;
	aStreamPtr->incrementSize = 1;

	if (ac > 0 && xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		xsSet(xsThis, aStreamPtr->id_chunk, xsArg(0));
		aStreamPtr->bytesAvailable = xsToInteger(xsGet(xsArg(0), aStreamPtr->id_length));
	}
	else {
		if (ac > 0 && xsTypeOf(xsArg(0)) == xsIntegerType)
			aStreamPtr->incrementSize = xsToInteger(xsArg(0));
		xsSet(xsThis, aStreamPtr->id_chunk, xsNew0(xsGlobal, aStreamPtr->id_Chunk));
		aStreamPtr->bytesAvailable = 0;
	}
}

void xscChunkStreamReadData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aSize;
	char *anAddress;

	xsVars(1);
	xsVar(0) = xsGet(xsThis, aStreamPtr->id_chunk);
	anAddress = (char *)xsGetHostData(xsVar(0));
	aSize = xsToInteger(xsGet(xsVar(0), aStreamPtr->id_length));

	aSize -= aStreamPtr->u.m;
	if (aSize > 0) {
		if (aSize > aStreamPtr->rsize)
			aSize = aStreamPtr->rsize;
		FskMemCopy(aStreamPtr->rdata, anAddress + aStreamPtr->u.m, aSize);
	}
	aStreamPtr->u.m += aSize;
	aStreamPtr->rcount = aSize;
}

void xscChunkStreamRewind(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStreamPtr->bytesAvailable = xsToInteger(xsGet(xsGet(xsThis, aStreamPtr->id_chunk), aStreamPtr->id_length));
	aStreamPtr->u.m = 0;
	aStreamPtr->rcount = 0;
	aStreamPtr->rindex = 0;
}

void xscChunkStreamWriteData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	if (aStreamPtr->wcount > 0) {
		char *anAddress;
		long aSize;

		xsVars(1);
		xsVar(0) = xsGet(xsThis, aStreamPtr->id_chunk);		//@@
		aSize = xsToInteger(xsGet(xsVar(0), aStreamPtr->id_length));
		if (aSize < (aStreamPtr->u.m + aStreamPtr->wcount))
			xsSet(xsVar(0), aStreamPtr->id_length, xsInteger(roundup(aStreamPtr->u.m + aStreamPtr->wcount, aStreamPtr->incrementSize)));
		anAddress = (char *)xsGetHostData(xsVar(0));

		FskMemCopy(anAddress + aStreamPtr->u.m, aStreamPtr->wdata, aStreamPtr->wcount);
		aStreamPtr->u.m += aStreamPtr->wcount;
		aStreamPtr->wcount = 0;
	}
}

void xscChunkStreamSeek(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aSize = xsToInteger(xsGet(xsGet(xsThis, aStreamPtr->id_chunk), aStreamPtr->id_length));
	SInt32 delta = xsToInteger(xsArg(0));
	UInt32 position = aStreamPtr->u.m + delta;

	if (((SInt32)position < 0) || (position > (UInt32)aSize))
		xsThrow(xsNewInstanceOf(xsErrorPrototype));

	aStreamPtr->u.m = position;
	aStreamPtr->bytesAvailable = aSize - position;
}

/* only intended to be called from an override of writeData */
void xscChunkGetWriteBuffer(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	if (aStreamPtr->wcount <= 0)
		return;

	xsResult = xsNew1(xsGlobal, aStreamPtr->id_Chunk, xsInteger(aStreamPtr->wcount));
	FskMemMove(xsGetHostData(xsResult), aStreamPtr->wdata, aStreamPtr->wcount);

	aStreamPtr->wcount = 0;
}

// file stream

void xscFileStreamAttachData(xsMachine* the)
{
	FskInt64 bytesAvailable;
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStreamPtr->u.fref = (FskFile)xsGetHostData(xsArg(0));
	FskFileGetSize(aStreamPtr->u.fref, &bytesAvailable);
	aStreamPtr->bytesAvailable = (UInt32)bytesAvailable;
}

void xscFileStreamCloseData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	FskFileClose(aStreamPtr->u.fref);
	aStreamPtr->u.fref = NULL;
}

void xscFileStreamDetachData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStreamPtr->u.fref = NULL;
}

void xscFileStreamOpenData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	char *aFilename = NULL, *name;
	FskErr err;

	xsTry  {
		UInt32 permissions = kFskFilePermissionReadOnly;
		SInt32 argc = xsToInteger(xsArgc);

		if (0 == argc)
			goto done;		// likely called from constructor

		xsVars(1);
		aFilename = xsToStringCopy(xsArg(0));
		xsVar(0) = xsString(aFilename);
		xsSet(xsThis, aStreamPtr->id_filename, xsVar(0));		//@@

		if (argc > 1) {
			SInt32 aKeyMode = xsToInteger(xsArg(1));
			if (1 & aKeyMode)
				permissions = kFskFilePermissionReadWrite;
			if (2 & aKeyMode)
				aStreamPtr->disableCache = true;
			if (4 & aKeyMode)
				permissions |= kFskFilePermissionRecovery;

			if (argc > 3) {
				aStreamPtr->bufferChunkSize = xsToInteger(xsArg(3));
				if (aStreamPtr->bufferChunkSize <= 0)
					aStreamPtr->bufferChunkSize = kStreamBufferChunk;
			}
		}

		err = FskFileOpen(aFilename, permissions, &aStreamPtr->u.fref);
		if ((kFskErrFileNotFound == err) && (kFskFilePermissionReadWrite & permissions)) {
			err = FskFileCreate(aFilename);
			FskECMAScriptThrowIf(the, err);

			err = FskFileOpen(aFilename, permissions, &aStreamPtr->u.fref);
		}
		FskECMAScriptThrowIf(the, err);
#ifndef KPR_CONFIG
		FskInstrumentedItemSetOwner(aStreamPtr->u.fref, (FskECMAScript)xsGetContext(the));
#endif
		FskFileGetSize(aStreamPtr->u.fref, &aStreamPtr->bytesAvailable);
		if (argc > 2) {
			if (xsUndefinedType != xsTypeOf(xsArg(2))) {
				FskInt64 size = xsToInteger(xsArg(2));
				err = FskFileSetSize(aStreamPtr->u.fref, &size);
				if (kFskErrNone == err) {		
					if (size < aStreamPtr->bytesAvailable)
						aStreamPtr->bytesAvailable = size;
				}
				else if (kFskErrUnimplemented == err)
					;	// don't require call to succeed because some file systems may not be able to do this
				else {
					if ((0 != size) && (0 == aStreamPtr->bytesAvailable)) {
						FskFileClose(aStreamPtr->u.fref);
						aStreamPtr->u.fref = NULL;
						FskFileDelete(aFilename);
					}
					FskECMAScriptThrowIf(the, err);
				}
			}
		}

		xsSet(xsThis, aStreamPtr->id_mime, xsCall1(xsGet(xsGlobal, aStreamPtr->id_FileSystem), xsID("getMIMEType"), xsVar(0)));

		name = FskStrRChr(aFilename, '/');
		if (!name)
			name = aFilename;
		else
			name += 1;
		xsSet(xsThis, xsID("name"), xsString(name));

done:
		;
	}
	xsCatch {
		FskMemPtrDispose(aFilename);
		FskFileClose(aStreamPtr->u.fref);
		aStreamPtr->u.fref = NULL;
		xsThrow(xsException);
	}
	FskMemPtrDispose(aFilename);
}

void xscFileStreamReadData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	UInt32 aCount;
	FskErr err;

	err = FskFileRead(aStreamPtr->u.fref, aStreamPtr->rsize, aStreamPtr->rdata, &aCount);
	if (err != kFskErrEndOfFile)
		FskECMAScriptThrowIf(the, err);

	aStreamPtr->rcount = aCount;
	aStreamPtr->rindex = 0;
}

void xscFileStreamRewind(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	FskInt64 zero = 0;

	xsCall0_noResult(xsThis, xsID("flush"));

	FskFileSetPosition(aStreamPtr->u.fref, &zero);
	FskFileGetSize(aStreamPtr->u.fref, &aStreamPtr->bytesAvailable);

	aStreamPtr->rcount = 0;
	aStreamPtr->rindex = 0;
}

void xscFileStreamWriteData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	if (0 != aStreamPtr->wcount) {
		FskErr err = FskFileWrite(aStreamPtr->u.fref, aStreamPtr->wcount, aStreamPtr->wdata, NULL);
		FskECMAScriptThrowIf(the, err);
		aStreamPtr->wcount = 0;
	}
}

void xscFileStreamSeek(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	FskErr err;
	xsNumberValue deltaF = xsToNumber(xsArg(0));
	FskInt64 delta = (FskInt64)deltaF;

	xsCall0_noResult(xsThis, xsID("flush"));

	if (delta) {
		FskInt64 position;

		FskFileGetPosition(aStreamPtr->u.fref, &position);
		position -= (aStreamPtr->rcount - aStreamPtr->rindex);
		FskMemPtrDisposeAt((void **)(void*)&aStreamPtr->rdata);
		aStreamPtr->rcount = 0;
		aStreamPtr->rsize = 0;
		aStreamPtr->rindex = 0;

		position += delta;
		err = FskFileSetPosition(aStreamPtr->u.fref, &position);
		FskECMAScriptThrowIf(the, err);
		aStreamPtr->bytesAvailable -= delta;
	}
}


// string stream

void xscStringStreamAttachData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	xsSet(xsThis, aStreamPtr->id_buffer, xsArg(0));
	aStreamPtr->disableCache = true;
	aStreamPtr->bytesAvailable = FskStrLen(xsToString(xsArg(0)));
}

void xscStringStreamCloseData(xsMachine* the)
{
}

void xscStringStreamDetachData(xsMachine* the)
{
}

void xscStringStreamOpenData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (0 == xsToInteger(xsArgc))
		xsSet(xsThis, aStreamPtr->id_buffer, xsString(""));
	else {
		char *str = xsToString(xsArg(0));
		aStreamPtr->bytesAvailable = FskStrLen(str);
		xsSet(xsThis, aStreamPtr->id_length, xsNumber((xsNumberValue)aStreamPtr->bytesAvailable));
		xsSet(xsThis, aStreamPtr->id_buffer, xsArg(0));
	}
}

void xscStringStreamReadData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	long aSize;
	char* anAddress;

	xsResult = xsGet(xsThis, aStreamPtr->id_buffer);
	anAddress = xsToString(xsResult);
	aSize = FskStrLen(anAddress);
	aSize -= aStreamPtr->u.m;
	if (aSize > 0) {
		if (aSize > aStreamPtr->rsize)
			aSize = aStreamPtr->rsize;
		FskMemCopy(aStreamPtr->rdata, anAddress + aStreamPtr->u.m, aSize);
	}
	aStreamPtr->u.m += aSize;
	aStreamPtr->rcount = aSize;
	aStreamPtr->rindex = 0;
}

void xscStringStreamRewind(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStreamPtr->bytesAvailable = FskStrLen(xsToString(xsGet(xsThis, xsID("buffer"))));
	aStreamPtr->u.m = 0;
	aStreamPtr->rcount = 0;
	aStreamPtr->rindex = 0;
}

void xscStringStreamWriteData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	if (aStreamPtr->wcount > 0) {
		char* aString;
		xsResult = xsGet(xsThis, aStreamPtr->id_buffer);
		FskECMAScriptThrowIf(the, FskMemPtrNew(aStreamPtr->wcount + 1, &aString));
		if (aString) {
			long aLength;
			FskMemCopy(aString, aStreamPtr->wdata, aStreamPtr->wcount);
			aString[aStreamPtr->wcount] = '\0';
			xsResult = xsCall1(xsResult, xsID("concat"), xsString(aString));
			FskMemPtrDispose(aString);
			aLength = FskStrLen(xsToString(xsResult));
			xsSet(xsThis, aStreamPtr->id_buffer, xsResult);
			xsSet(xsThis, aStreamPtr->id_length, xsInteger(aLength));
		}
		aStreamPtr->wcount = 0;
	}
}

static void *gzAlloc(void *state, uInt items, uInt size);
static void gzFree(void *state, void *mem);
static void gzipStreamReset(StreamBufferPtr aStreamPtr);

void *gzAlloc(void *state, uInt items, uInt size)
{
	return FskMemPtrAlloc(items * size);
}

void gzFree(void *state, void *mem)
{
	FskMemPtrDispose(mem);
}

void xscGZipStreamReadData(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	int result;

	if (false == aStreamPtr->initializedZlib) {
		aStreamPtr->zlib.zalloc = gzAlloc;
		aStreamPtr->zlib.zfree = gzFree;
		aStreamPtr->zlib.opaque = NULL;
		if (Z_OK != inflateInit2(&aStreamPtr->zlib, -MAX_WBITS))		// header-free
			FskECMAScriptThrowIf(the, kFskErrOperationFailed);
		aStreamPtr->initializedZlib = true;
		aStreamPtr->zlib.total_in	= 0;
	}

	if (0 == aStreamPtr->zlib.avail_in) {
		xsResult = xsCall1(xsGet(xsThis, xsID("source")), xsID("readChunk"), xsInteger(aStreamPtr->bufferChunkSize));
		if (!xsTest(xsResult)) {
			aStreamPtr->rcount = 0;
			aStreamPtr->rindex = 0;

			xsResult = xsNull;
			return;
		}

		aStreamPtr->gzInputBuffer = (unsigned char *)xsGetHostData(xsResult);
		aStreamPtr->zlib.next_in = aStreamPtr->gzInputBuffer;
		aStreamPtr->zlib.avail_in = xsToInteger(xsGet(xsResult, xsID("length")));
		xsSetHostData(xsResult, NULL);
	}

	aStreamPtr->zlib.next_out	= (Bytef *)aStreamPtr->rdata;
	aStreamPtr->zlib.avail_out	= aStreamPtr->rsize;
	aStreamPtr->zlib.total_out	= 0;
	result = inflate(&aStreamPtr->zlib, Z_PARTIAL_FLUSH);
	if ((Z_OK != result) && (Z_STREAM_END != result))
		FskECMAScriptThrowIf(the, kFskErrOperationFailed);

	if (0 == aStreamPtr->zlib.avail_in)
		FskMemPtrDisposeAt((void **)(void*)&aStreamPtr->gzInputBuffer);

	aStreamPtr->rcount = aStreamPtr->zlib.total_out;
	aStreamPtr->rindex = 0;

	xsResult = xsNull;
}

void xscGZipStreamRewind(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	gzipStreamReset(aStreamPtr);
	xsCall0_noResult(xsThis, xsID("flush"));
	xsCall0_noResult(xsGet(xsThis, xsID("source")), xsID("flush"));
	xsCall0_noResult(xsGet(xsThis, xsID("source")), xsID("rewind"));
	xsCall1_noResult(xsGet(xsThis, xsID("source")), xsID("seek"), xsInteger(10));		// skip gz header

	aStreamPtr->bytesAvailable = 0;			// we don't really know
	aStreamPtr->rcount = 0;
	aStreamPtr->rindex = 0;
}

void xscGZipStreamCloseData(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

	gzipStreamReset(aStreamPtr);
	if (xsTest(xsGet(xsThis, xsID("source")))) {
		xsCall0_noResult(xsGet(xsThis, xsID("source")), xsID("close"));
		xsDelete(xsThis, xsID("source"));
	}
}

void gzipStreamReset(StreamBufferPtr aStreamPtr)
{
	if (aStreamPtr->initializedZlib) {
		aStreamPtr->initializedZlib = false;
		inflateEnd(&aStreamPtr->zlib);
		FskMemSet(&aStreamPtr->zlib, 0, sizeof(aStreamPtr->zlib));
	}

	FskMemPtrDisposeAt((void **)(void*)&aStreamPtr->gzInputBuffer);
}

#if SUPPORT_SOCKET_STREAM

// socket stream
typedef struct {
	unsigned int props;
#define kSocketStreamPropertyBlocking	0x01
#define kSocketStreamPropertyPriority	0x02
#define kSocketStreamPropertySynchronous	0x04
#define kSocketStreamPropertySSL	0x08
#define kSocketStreamPropertyCert	0x10
#define kSocketStreamPropertyKeepalive	0x20
#define kSocketStreamPropertyRaw	0x40
	Boolean blocking;
	Boolean keepalive;
	int priority;
	long flags;
	FskSocketCertificateRecord cert;
} SocketStreamProperties;

typedef struct {
	StreamBufferPtr aStreamPtr;
} SocketStreamConnectCallbackData;

static void xscSocketStreamConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
static void xscSocketStreamReady(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

extern FskThreadDataHandler FskThreadFindDataHandlerBySourceNode(UInt32 dataNode);

static void xscSocketStreamAddDataHandler(xsMachine *the, StreamBufferPtr aStreamPtr)
{
	if (!((aStreamPtr->socketWantsReadable && xsTest(xsGet(aStreamPtr->obj, xsID("onReadable")))) ||
	      (aStreamPtr->socketWantsWritable && xsTest(xsGet(aStreamPtr->obj, xsID("onWritable")))) ||
	      (aStreamPtr->wcount != 0)))
		return;	// nothing to register

	if (aStreamPtr->handler != NULL)
		FskThreadRemoveDataHandler(&aStreamPtr->handler);
	FskThreadAddDataHandler(&aStreamPtr->handler, (FskThreadDataSource)aStreamPtr->u.s, xscSocketStreamReady, aStreamPtr->socketWantsReadable, aStreamPtr->socketWantsWritable, aStreamPtr);
}

void xscSocketStreamAttachData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	aStreamPtr->u.s = (FskSocket)xsGetHostData(xsArg(0));
	if (aStreamPtr->u.s == NULL)
		return;
	if (aStreamPtr->bytesAvailable <= 0)
		aStreamPtr->bytesAvailable = FskNetSocketIsReadable(aStreamPtr->u.s) ? (aStreamPtr->u.s->isEof ? -1: 1): 0;
}

void xscSocketStreamCloseData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

#if CLOSED_SSL
	if (aStreamPtr->ssl != NULL) {
		/* handshaking */
		FskSSLClose(aStreamPtr->ssl);
		FskSSLDispose(aStreamPtr->ssl);
		aStreamPtr->ssl = NULL;
	}
#endif
	if (aStreamPtr->netInterfaceNotifier != NULL) {
		FskNetInterfaceRemoveNotifier(aStreamPtr->netInterfaceNotifier);
		aStreamPtr->netInterfaceNotifier = NULL;
	}
	if (aStreamPtr->handler != NULL)
		FskThreadRemoveDataHandler(&aStreamPtr->handler);
	if (aStreamPtr->u.s != 0) {
		FskNetSocketClose(aStreamPtr->u.s);
		aStreamPtr->u.s = 0;
	}
	if (aStreamPtr->callbackData != NULL) {
		SocketStreamConnectCallbackData *callbackData = aStreamPtr->callbackData;
		callbackData->aStreamPtr = NULL;	/* tell them stream.socket has been closed */
	}
}

void xscSocketStreamDetachData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	if (aStreamPtr->handler != NULL) FskThreadRemoveDataHandler(&aStreamPtr->handler);
	xsResult = xsNewHostObject(NULL);
	xsSetHostData(xsResult, aStreamPtr->u.s);
	aStreamPtr->u.s = 0;
}

static FskErr xscSocketStreamOpenComplete(FskSocket skt, void *refCon)
{
	SocketStreamConnectCallbackData *callbackData = refCon;
	StreamBufferPtr aStreamPtr = callbackData->aStreamPtr;

	FskMemPtrDispose(callbackData);
	if (aStreamPtr == NULL) {
		/* "this" has been already closed */
		if (skt && skt->lastErr == kFskErrNone)
			FskNetSocketClose(skt);
		return kFskErrNone;
	}

	aStreamPtr->callbackData = NULL;
	if (skt && skt->lastErr == kFskErrNone) {
		aStreamPtr->u.s = skt;
		if (aStreamPtr->keepalive)
			FskNetSocketSetKeepAlive(skt, 1);

//		if (FskNetSocketIsWritable(skt))
//			xscSocketStreamConnected(aStreamPtr->u.s, aStreamPtr);
//		else
			FskThreadAddDataHandler(&aStreamPtr->handler, (FskThreadDataSource)aStreamPtr->u.s, xscSocketStreamConnected, true, true, aStreamPtr);
	}
	else {
		xsBeginHost(aStreamPtr->the);
		xsSet(aStreamPtr->obj, xsID("connectionError"), xsInteger(skt ? skt->lastErr : kFskErrUnknown));
		xsCall0_noResult(aStreamPtr->obj, xsID("onConnected"));	// "connected" is not set
		xsEndHost(aStreamPtr->the);
		if (skt)
			FskNetSocketClose(skt);
	}
	return kFskErrNone;
}

static void xscSocketStreamSetCert(xsMachine *the, FskSocketCertificateRecord *certs, xsSlot arg)
{
	FskMemSet(certs, 0, sizeof(FskSocketCertificateRecord));
	/* the certificates option */
	if (!xsTest(arg))
		return;
	else if (xsTypeOf(arg) == xsStringType) {
		certs->certificates = xsToStringCopy(arg);
		certs->certificatesSize = FskStrLen(certs->certificates);
	}
	else if (xsIsInstanceOf(arg, xsChunkPrototype)) {
		certs->certificatesSize = xsToInteger(xsGet(arg, xsID("length")));
		FskMemPtrNewFromData(certs->certificatesSize, xsGetHostData(arg), (FskMemPtr *)&certs->certificates);
	}
	/* else ignore it and go ahead */
	certs->policies = xsHas(arg, xsID("policies")) ? xsToStringCopy(xsGet(arg, xsID("policies"))): NULL;
}

static void xscSocketStreamDisposeCert(FskSocketCertificateRecord *certs)
{
	if (certs == NULL)
		return;
	if (certs->certificates != NULL && certs->certificatesSize > 0)
		FskMemPtrDispose(certs->certificates);
	if (certs->policies != NULL)
		FskMemPtrDispose(certs->policies);
	if (certs->key != NULL && certs->keySize > 0)
		FskMemPtrDispose(certs->key);
}

static void xscSocketStreamArgToProperties(xsMachine *the, SocketStreamProperties *props, xsSlot arg)
{
	props->props = 0;
	props->blocking = false;
    props->keepalive = false;
	props->flags = 0;
	props->priority = kFskNetSocketDefaultPriority;

	if (!xsTest(arg))
		return;
	if (xsHas(arg, xsID("ssl")) && xsToBoolean(xsGet(arg, xsID("ssl")))) {
		props->flags |= kConnectFlagsSSLConnection;
		props->props |= kSocketStreamPropertySSL;
	}
	if (xsHas(arg, xsID("cert"))) {
		xscSocketStreamSetCert(the, &props->cert, xsGet(arg, xsID("cert")));
		props->props |= kSocketStreamPropertyCert;
	}
	if (xsHas(arg, xsID("synchronous"))) {
		if (xsToBoolean(xsGet(arg, xsID("synchronous"))))
			props->flags |= kConnectFlagsSynchronous;
		else
			props->flags &= ~kConnectFlagsSynchronous;
		props->props |= kSocketStreamPropertySynchronous;
	}
	if (xsHas(arg, xsID("querytype"))) {
		const char *value = xsToString(xsGet(arg, xsID("querytype")));
		if (FskStrCompareCaseInsensitive(value, "SRV") == 0)
			props->flags |= kConnectFlagsServerSelection;
	}
	if (xsHas(arg, xsID("blocking"))) {
		props->blocking = xsToBoolean(xsGet(arg, xsID("blocking")));
		props->props |= kSocketStreamPropertyBlocking;
	}
	if (xsHas(arg, xsID("keepalive"))) {
		props->keepalive = xsToBoolean(xsGet(arg, xsID("keepalive")));
		props->props |= kSocketStreamPropertyKeepalive;
	}
	if (xsHas(arg, xsID("priority"))) {
		const char *value = xsToString(xsGet(arg, xsID("priority")));
		props->props |= kSocketStreamPropertyPriority;
		if (FskStrStr(value, "lowest"))
			props->priority = kFskNetSocketLowestPriority;
		else if (FskStrStr(value, "low"))
			props->priority = kFskNetSocketLowPriority;
		else if (FskStrStr(value, "medium"))
			props->priority = kFskNetSocketMediumPriority;
		else if (FskStrStr(value, "highest"))
			props->priority = kFskNetSocketHighestPriority;
		else if (FskStrStr(value, "high"))
			props->priority = kFskNetSocketHighPriority;
	}
	if (xsHas(arg, xsID("raw")))
		props->props |= kSocketStreamPropertyRaw;
}

void xscSocketStreamOpenData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	SocketStreamConnectCallbackData *callbackData;
	char* aHostname;
	short aPort;
	SocketStreamProperties props;
	FskSocketCertificateRecord *cert;
	FskErr err;
	int ac = xsToInteger(xsArgc);

	aStreamPtr->isSocket = true;

	if (ac < 1)
		goto done;		// likely called from constructor
	else if (ac < 2) {
		// attach the socket
		FskSocket skt = (FskSocket)xsGetHostData(xsArg(0));
		aStreamPtr->u.s = skt;
		aStreamPtr->socketConnected = true;
		aStreamPtr->socketWantsReadable = true;
		aStreamPtr->socketWantsWritable = true;
		xsSet(xsThis, xsID("hostname"), xsString(skt->hostname == NULL ? "" : skt->hostname));
		xsSet(xsThis, xsID("connected"), xsTrue);
		goto done;
	}

	aHostname = xsToStringCopy(xsArg(0));
	aPort = (short)xsToInteger(xsArg(1));
	xscSocketStreamArgToProperties(the, &props, ac >= 3 ? xsArg(2) : xsUndefined);
	cert = props.props & kSocketStreamPropertyCert ? &props.cert : NULL;
	aStreamPtr->keepalive = props.keepalive;
	aStreamPtr->isRaw = (props.props & kSocketStreamPropertyRaw) != 0;

	xsSet(xsThis, xsID("hostname"), xsArg(0));
	xsSet(xsThis, xsID("connected"), xsFalse);

	err = FskMemPtrNew(sizeof(SocketStreamConnectCallbackData), &callbackData);
	FskECMAScriptThrowIf(the, err);
	callbackData->aStreamPtr = aStreamPtr;
	aStreamPtr->callbackData = callbackData;
	err = FskNetConnectToHostPrioritized(aHostname, aPort, props.blocking, xscSocketStreamOpenComplete, callbackData, props.flags, props.priority, cert, "xscSocketStreamOpenData");
	if (cert) xscSocketStreamDisposeCert(cert);
	FskMemPtrDispose(aHostname);
	FskECMAScriptThrowIf(the, err);
done:;
}

void xscSocketStreamReadData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	int aCount = 0;
	FskErr err;

	if (false == aStreamPtr->socketConnected)
		goto done;

	if (!aStreamPtr->u.s)	/* probably detached */
		goto done;


	aStreamPtr->socketWantsReadable = false;
	FskThreadRemoveDataHandler(&aStreamPtr->handler);

	if (aStreamPtr->isRaw)
		err = FskNetSocketRecvRawTCP(aStreamPtr->u.s, aStreamPtr->rdata, aStreamPtr->rsize, &aCount);
	else
		err = FskNetSocketRecvTCP(aStreamPtr->u.s, aStreamPtr->rdata, aStreamPtr->rsize, &aCount);
	if (kFskErrNone == err) {
		aStreamPtr->socketWantsReadable = true;
		xscSocketStreamAddDataHandler(the, aStreamPtr);
	}
	else if (kFskErrNoData == err) {
		aCount = 0;

		aStreamPtr->socketWantsReadable = true;
		xscSocketStreamAddDataHandler(the, aStreamPtr);
	}
	else if (kFskErrSocketNotConnected == err || kFskErrConnectionClosed == err) {
		aCount = 0;
		aStreamPtr->u.s->isEof = true;
	}
	else {
		xsThrow(xsNewInstanceOf(xsErrorPrototype));
	}

done:
	aStreamPtr->rcount = aCount;
	aStreamPtr->rindex = 0;
}

void xscSocketStreamWriteData(xsMachine* the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	int aCount;
	FskErr err;

	if (false == aStreamPtr->socketConnected)
		return;

	if (!aStreamPtr->u.s)	/* probably detached */
		return;

	aStreamPtr->socketWantsWritable = false;
	FskThreadRemoveDataHandler(&aStreamPtr->handler);

	if (aStreamPtr->isRaw)
		err = FskNetSocketSendRawTCP(aStreamPtr->u.s, aStreamPtr->wdata, aStreamPtr->wcount, &aCount);
	else
		err = FskNetSocketSendTCP(aStreamPtr->u.s, aStreamPtr->wdata, aStreamPtr->wcount, &aCount);
	if (kFskErrNoData == err)
		aCount = 0;
	else
		FskECMAScriptThrowIf(the, err);

	aStreamPtr->wdata += aCount;
	aStreamPtr->wcount -= aCount;
//	aStreamPtr->socketWantsWritable = (0 != aStreamPtr->wcount);
	aStreamPtr->socketWantsWritable = true;
	xscSocketStreamAddDataHandler(the, aStreamPtr);
}

#if CLOSED_SSL
static FskErr xscSocketStreamSSLCallback(FskSocket skt, void *refCon)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)refCon;

	aStreamPtr->ssl = NULL;
	xsBeginHost(aStreamPtr->the);
	{
		xsVars(1);
		xsVar(0) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(0), skt);
		xsCall1_noResult(aStreamPtr->obj, xsID("attachData"), xsVar(0));
		xsCall1_noResult(aStreamPtr->obj, xsID("onTLSStarted"), xsBoolean(skt != NULL && skt->lastErr == kFskErrNone));
	}
	xsEndHost(aStreamPtr->the);
	return kFskErrNone;
}
#endif

static void xscSocketStreamStartTLS(xsMachine *the, FskSocketCertificateRecord *cert)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);

#if CLOSED_SSL
	FskSocket skt;
	FskErr err;

	/* detach the socket from "this" */
	if (aStreamPtr->handler != NULL) FskThreadRemoveDataHandler(&aStreamPtr->handler);
	skt = aStreamPtr->u.s;
	aStreamPtr->u.s = 0;
	/* and attach it to the SSL socket stream */
	err = FskSSLAttach(&aStreamPtr->ssl, skt);
	if (err == kFskErrNone) {
		if (cert != NULL)
			FskSSLLoadCerts(aStreamPtr->ssl, cert);
		err = FskSSLHandshake(aStreamPtr->ssl, xscSocketStreamSSLCallback, aStreamPtr, true, 0);
	}
	else
		/* reattach anyway */
		aStreamPtr->u.s = skt;
	FskECMAScriptThrowIf(the, err);
#else
	FskECMAScriptThrowIf(the, kFskErrUnimplemented);
#endif
}

void xscSocketStreamSetProperties(xsMachine *the)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsThis);
	FskSocket skt = aStreamPtr->u.s;
	SocketStreamProperties props;

	if (xsToInteger(xsArgc) < 1)
		return;

	xscSocketStreamArgToProperties(the, &props, xsArg(0));
	if (props.props & kSocketStreamPropertySSL) {
		/* ignore the value -- it must be always "true" */
		/* even if skt->ssl is already true, do it anyway */
		FskSocketCertificateRecord *cert = props.props & kSocketStreamPropertyCert ? &props.cert: NULL;
		xscSocketStreamStartTLS(the, cert);
		if (cert) xscSocketStreamDisposeCert(cert);
	}
	if (props.props & kSocketStreamPropertySynchronous)
		/* changing synchronous flag doesn't make sense */;
	if (props.props & kSocketStreamPropertyBlocking) {
		if (!props.blocking && !skt->nonblocking)
			FskNetSocketMakeNonblocking(skt);
		/* no way to go back to the blocking mode... */
	}
	if (props.props & kSocketStreamPropertyPriority) {
		skt->priority = props.priority;
		/* @@ should notify? */
	}
	if (props.props & kSocketStreamPropertyKeepalive)
		FskNetSocketSetKeepAlive(skt, props.keepalive);
	aStreamPtr->isRaw = (props.props & kSocketStreamPropertyRaw) != 0;
}

static int xscSocketStreamNetInterfaceChanged(struct FskNetInterfaceRecord *iface, UInt32 status, void *params)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)params;
	UInt32 netmask = (UInt32)iface->netmask;
	FskSocket skt = aStreamPtr->u.s;
	UInt32 localAddr;
	char *ss;
	Boolean flag;

	if (skt == NULL)
		return 0;
	FskNetSocketGetLocalAddress(skt, &localAddr, NULL);
	if ((localAddr & netmask) == ((UInt32)iface->ip & netmask)) {
		switch (status) {
		case kFskNetInterfaceStatusNew:
			ss = "new";
			flag = iface->status;
			break;
		case kFskNetInterfaceStatusChanged:
			ss = "changed";
			flag = iface->status;
			break;
		case kFskNetInterfaceStatusRemoved:
			ss = "removed";
			flag = 0;
			break;
		default:
			return 0;
		}
		xsBeginHost(aStreamPtr->the);
		if (xsHas(aStreamPtr->obj, xsID("onInterfaceChanged")))
			xsCall2_noResult(aStreamPtr->obj, xsID("onInterfaceChanged"), xsString(ss), xsBoolean(flag));
		xsEndHost(aStreamPtr->the);
	}
	return 0;
}

void xscSocketStreamConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)refCon;

	aStreamPtr->socketConnected = true;

	FskThreadRemoveDataHandler(&aStreamPtr->handler);
	aStreamPtr->socketWantsReadable = true;
	aStreamPtr->socketWantsWritable = true;

	aStreamPtr->netInterfaceNotifier = FskNetInterfaceAddNotifier(xscSocketStreamNetInterfaceChanged, refCon, "stream socket");

	xsBeginHost(aStreamPtr->the);
	{
	xsSet(aStreamPtr->obj, xsID("connected"), xsTrue);
	xsCall0_noResult(aStreamPtr->obj, xsID("onConnected"));
	xscSocketStreamAddDataHandler(aStreamPtr->the, aStreamPtr);
	}
	xsEndHost(aStreamPtr->the);
}

void xscSocketStreamReady(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)refCon;
	unsigned long initialByteWritten = aStreamPtr->bytesWritten;

	FskThreadRemoveDataHandler(&aStreamPtr->handler);

	if (!aStreamPtr->u.s) return;

	xsBeginHost(aStreamPtr->the);
	{
	if (FskNetSocketIsReadable(aStreamPtr->u.s)) {
		aStreamPtr->socketWantsReadable = false;	// true only when it really wants to read
		aStreamPtr->bytesAvailable = aStreamPtr->u.s->isEof ? -1: 1;	// we must be able to read at least 1 byte
		if (xsTest(xsGet(aStreamPtr->obj, xsID("onReadable")))) xsCall0_noResult(aStreamPtr->obj, xsID("onReadable"));
		if (!aStreamPtr->u.s) goto bail;	// detached in callback
	}

	if (FskNetSocketIsWritable(aStreamPtr->u.s)) {
		aStreamPtr->socketWantsWritable = false;
		if (0 != aStreamPtr->wcount)
			xsCall0_noResult(aStreamPtr->obj, aStreamPtr->id_writeData);	// first output what we have
		if (0 == aStreamPtr->wcount && xsTest(xsGet(aStreamPtr->obj, xsID("onWritable"))))
			xsCall0_noResult(aStreamPtr->obj, xsID("onWritable"));	// let client write more
		if (!aStreamPtr->u.s) goto bail;	// detached in callback
	}

	if (initialByteWritten != aStreamPtr->bytesWritten)
		xsCall0_noResult(aStreamPtr->obj, aStreamPtr->id_writeData);		// auto flush if they wrote during EITHER read or write callback

	xscSocketStreamAddDataHandler(aStreamPtr->the, aStreamPtr);
	}

bail:
	xsEndHost(aStreamPtr->the);
}
#endif

FskErr readStreamIntoMemory(xsMachine *the, xsSlot *stream, xsSlot *scratch, unsigned char **data, UInt32 *dataSize)
{
	StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(*stream);

	*scratch = xsCall1(*stream, aStreamPtr->id_readChunk, xsInteger((UInt32)aStreamPtr->bytesAvailable));

	*dataSize = xsToInteger(xsGet(*scratch, aStreamPtr->id_length));
	*data = (unsigned char *)xsGetHostData(*scratch);
	xsSetHostData(*scratch, NULL);

	return kFskErrNone;
}

#ifndef KPR_CONFIG

/*
	File System
*/

void xs_FileSystem_getFileInfo(xsMachine *the)
{
	FskFileInfo itemInfo;

	if (xsToInteger(xsArgc) >= 1) {
		FskECMAScript vm = (FskECMAScript)xsGetContext(the);
		xsVars(1);

		if (kFskErrNone == FskFileGetFileInfo(xsToString(xsArg(0)), &itemInfo)) {
			char *fType;

			xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, vm->id_FileSystem), vm->id_moreFileInfo));
			if (itemInfo.filesize < kXSMaxInt)
				xsSet(xsResult, vm->id_size, xsInteger((xsIntegerValue)itemInfo.filesize));
			else
				xsSet(xsResult, vm->id_size, xsNumber((xsNumberValue)itemInfo.filesize));
			if (itemInfo.filetype == kFskDirectoryItemIsFile)
				fType = NULL;			// default value in prototype is "file"
			else if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
				fType = "directory";
			else if (itemInfo.filetype == kFskDirectoryItemIsLink)
				fType = "link";
			else
				fType = NULL;

			if (NULL != fType)
				xsSet(xsResult, vm->id_type, xsGet(xsResult, xsID(fType)));

			// The ECMAScript date object constructor expects UTC values in milliseconds
			xsVar(0) = xsNew1(xsGlobal, vm->id_Date, xsNumber(itemInfo.fileCreationDate * 1000.0));
			xsSet(xsResult, vm->id_creationDate, xsVar(0));
			xsVar(0) = xsNew1(xsGlobal, vm->id_Date, xsNumber(itemInfo.fileModificationDate * 1000.0));
			xsSet(xsResult, vm->id_modificationDate, xsVar(0));

			if (kFileFileHidden & itemInfo.flags)
				xsSet(xsResult, vm->id_hidden, xsTrue);
			if (kFileFileLocked & itemInfo.flags)
				xsSet(xsResult, vm->id_locked, xsTrue);
		}
	}
}

void xs_FileSystem_simplifyPath(xsMachine* the)
{
#if TARGET_OS_WIN32
	WCHAR platform_simplified[1024];
	char* original = NULL;
	char* simplified = NULL;
	UInt16 *platform_original = NULL;

	xsResult = xsArg(0);
	original = xsToString(xsArg(0));
	if (!FskFilePathToNative(original, (char **)&platform_original)) {
		if (GetFullPathNameW((LPCWSTR)platform_original, sizeof(platform_simplified) / 2, platform_simplified, NULL)) {
			simplified = fixUpPathForFsk(platform_simplified);
			if (simplified) {
				xsResult = xsString(simplified);
				FskMemPtrDispose(simplified);
			}
			FskMemPtrDispose(platform_original);
		}
	}
#elif TARGET_OS_KPL
	xsResult = xsArg(0);	// @@
#else 
	int c0, c1;
	char* path0;
	char path1[PATH_MAX + 1];
	path0 = xsToString(xsArg(0));

	xsResult = xsArg(0);
	if (realpath(path0, path1)) {
		c0 = strlen(path0);
		c1 = strlen(path1);
		if ((path0[c0 - 1] == '/') && (path1[c1 - 1] != '/')) {
			path1[c1] = '/';
			path1[c1 + 1] = 0;
		}
		xsResult = xsString(path1);
	}
#endif
}

void xs_FileSystem_setFileInfo(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 1) {
		char *path;
		FskFileInfo itemInfo;

		path = xsToStringCopy(xsArg(0));

		if (kFskErrNone == FskFileGetFileInfo(path, &itemInfo)) {
			FskErr err;

			if (xsToBoolean(xsGet(xsArg(1), xsID("locked"))))
				itemInfo.flags |= kFileFileLocked;
			else
				itemInfo.flags &= ~kFileFileLocked;

			if (xsToBoolean(xsGet(xsArg(1), xsID("hidden"))))
				itemInfo.flags |= kFileFileHidden;
			else
				itemInfo.flags &= ~kFileFileHidden;

			itemInfo.fileCreationDate = (UInt32)(xsToNumber(xsGet(xsArg(1), xsID("creationDate"))) / 1000.0);
			itemInfo.fileModificationDate = (UInt32)(xsToNumber(xsGet(xsArg(1), xsID("modificationDate"))) / 1000.0);

			err = FskFileSetFileInfo(path, &itemInfo);
			xsResult = xsBoolean(kFskErrNone == err);
		}

		FskMemPtrDispose(path);
	}
}

void xs_FileSystem_deleteFile(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileDelete(xsToString(xsArg(0)));
		xsResult = xsBoolean(kFskErrNone == err);
		if (kFskErrFileNotFound != err)
			FskECMAScriptThrowIf(the, err);
	}
}

void xs_FileSystem_renameFile(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 2) {
		FskErr err;
		char *to = xsToStringCopy(xsArg(1));
		char *from = xsToString(xsArg(0));

		err = FskFileRename(from ,to);
		FskMemPtrDispose(to);
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_resolveLink(xsMachine *the)
{
	char *path = xsToString(xsArg(0));
	char *resolved;

	if (kFskErrNone == FskFileResolveLink(path, &resolved)) {
		xsResult = xsString(resolved);
		FskMemPtrDispose(resolved);
	}
}

void xs_FileSystem_createDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileCreateDirectory(xsToString(xsArg(0)));
		if (kFskErrFileExists == err)
			err = kFskErrNone;
		FskECMAScriptThrowIf(the, err);
	}
}

void xs_FileSystem_deleteDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileDeleteDirectory(xsToString(xsArg(0)));
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_renameDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 2) {
		FskErr err;
		char *to = xsToStringCopy(xsArg(1));
		char *from = xsToString(xsArg(0));

		err = FskFileRenameDirectory(from, to);
		FskMemPtrDispose(to);
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_getVolumeInfo(xsMachine *the)
{
	FskErr err;
	Boolean haveID = false;
	UInt32 id;
	char *path = NULL, *name = NULL;
	UInt32 volumeType;
	Boolean isRemovable;
	FskInt64 free, capacity;
	Boolean requestDeviceInfo = false, requestVolumeType = true, requestIsRemovable = true;
	
	if (xsToInteger(xsArgc) >= 2) {
		UInt32 flags = xsToInteger(xsArg(1));
		if (1 & flags)
			requestDeviceInfo = true;
		
		requestVolumeType = 0 == (2 & flags);
		requestIsRemovable = 0 == (4 & flags);
	}

	if (xsStringType == xsTypeOf(xsArg(0))) {
		err = FskVolumeGetInfoFromPath(xsToString(xsArg(0)), &path, &name,
				requestVolumeType ? &volumeType : NULL, requestIsRemovable ? &isRemovable : NULL, &capacity, &free);
		if (kFskErrNone == err)
			haveID = kFskErrNone == FskVolumeGetID(path, &id);
	}
	else {
		id = xsToInteger(xsArg(0));
		haveID = true;
		err = FskVolumeGetInfo(id, &path, &name,
				requestVolumeType ? &volumeType : NULL, requestIsRemovable ? &isRemovable : NULL, &capacity, &free);
	}
	FskECMAScriptThrowIf(the, err);

	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FileSystem")), xsID("volumeInfo")));
	xsSet(xsResult, xsID("path"), xsString(path));
	xsSet(xsResult, xsID("name"), xsString(name));
	if (haveID)
		xsSet(xsResult, xsID("id"), xsInteger(id));
	if (requestIsRemovable)
		xsSet(xsResult, xsID("removable"), xsBoolean(isRemovable));
	xsSet(xsResult, xsID("capacity"), xsNumber((xsNumberValue)capacity));
	xsSet(xsResult, xsID("free"), xsNumber((xsNumberValue)free));
	if (requestVolumeType && (volumeType <= kFskVolumeTypeDirectory)) {
		static char *kinds[] = {"", "unknown", "fixed", "floppy", "optical", "CD", "DVD", "network", "Memory Stick", "MMC", "SD Memory", "Compact Flash", "SmartMedia", "directory"};
		xsSet(xsResult, xsID("kind"), xsString(kinds[volumeType]));
	}

	FskMemPtrDispose(path);
	FskMemPtrDispose(name);

	if (requestDeviceInfo && haveID) {
		char *vendor, *product, *revision, *vendorSpecific;

		err = FskVolumeGetDeviceInfo(id, &vendor, &product, &revision, &vendorSpecific);
		if (err) {
			xsSet(xsResult, xsID("device"), xsNull);	// set null rather than throwing an error
			return;
		}

		xsVars(1);
		xsVar(0) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FileSystem")), xsID("deviceInfo")));
		if (vendor) {
			xsSet(xsVar(0), xsID("vendor"), xsString(vendor));
			FskMemPtrDispose(vendor);
		}
		if (product) {
			xsSet(xsVar(0), xsID("product"), xsString(product));
			FskMemPtrDispose(product);
		}
		if (revision) {
			xsSet(xsVar(0), xsID("revision"), xsString(revision));
			FskMemPtrDispose(revision);
		}
		if (vendorSpecific) {
			xsSet(xsVar(0), xsID("vendorSpecific"), xsString(vendorSpecific));
			FskMemPtrDispose(vendorSpecific);
		}

		xsSet(xsResult, xsID("device"), xsVar(0));
	}
}

void xs_FileSystem_ejectVolume(xsMachine *the)
{
	FskErr err = FskVolumeEject(xsToString(xsArg(0)), 0);
	xsResult = err ? xsFalse : xsTrue;
}

/*
	Thumbnail
*/

void xs_FileSystem_getThumbnail(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	FskErr err;
	UInt32 width, height;
	FskBitmap thumbnail;

	if (argc >= 3) {
		width = xsToInteger(xsArg(1));
		height = xsToInteger(xsArg(2));
	}
	else
		width = height = 0;

	err = FskFileGetThumbnail(xsToString(xsArg(0)), width, height, &thumbnail);
	FskECMAScriptThrowIf(the, err);

	fskBitmapToXSBitmap(the, thumbnail, true, &xsResult);
}

/*
	Directory Iterator
*/

typedef struct {
	FskDirectoryIterator	dirIt;
	FskVolumeIterator		volIt;

	xsIndex					id_FileSystem;
	xsIndex					id_fileInfo;
	xsIndex					id_path;
	xsIndex					id_type;
	xsIndex					id_directory;
	xsIndex					id_link;
	xsIndex					id_name;
	xsIndex					id_volume;
	xsIndex					id_id;
} xsNativeFSIteratorRecord, *xsNativeFSIterator;

void xs_FileSystem_Iterator(xsMachine* the)
{
	xsNativeFSIterator fsit;
	FskErr err;
	const char *path;

	err = FskMemPtrNewClear(sizeof(xsNativeFSIteratorRecord), &fsit);
	FskECMAScriptThrowIf(the, err);
	xsSetHostData(xsResult, fsit);
	
	fsit->id_FileSystem = xsID("FileSystem");
	fsit->id_fileInfo = xsID("fileInfo");
	fsit->id_path = xsID("path");
	fsit->id_type = xsID("type");
	fsit->id_directory = xsID("directory");
	fsit->id_link = xsID("link");
	fsit->id_name = xsID("name");
	fsit->id_volume = xsID("volume");
	fsit->id_id = xsID("id");

	path = xsToInteger(xsArgc) ? xsToString(xsArg(0)) : NULL;
	if ((NULL == path) || (0 == *path)) {
		err = FskVolumeIteratorNew(&fsit->volIt);
		FskECMAScriptThrowIf(the, err);
//		FskInstrumentedItemSetOwner(fsit->volIt, (FskECMAScript)xsGetContext(the));
	}
	else {
		UInt32 flags = 0;
		if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
			flags = 1;
		err = FskDirectoryIteratorNew(path, &fsit->dirIt, flags);
		FskECMAScriptThrowIf(the, err);
		FskInstrumentedItemSetOwner(fsit->dirIt, (FskECMAScript)xsGetContext(the));
	}
}

void xs_FileSystem_iterator_destructor(void *it)
{
	if (it) {
		xsNativeFSIterator fsit = (xsNativeFSIterator)it;
		FskDirectoryIteratorDispose(fsit->dirIt);
		FskVolumeIteratorDispose(fsit->volIt);
		FskMemPtrDispose(fsit);
	}
}

void xs_FileSystem_iterator_getNext(xsMachine* the)
{
	xsNativeFSIterator fsit = (xsNativeFSIterator)xsGetHostData(xsThis);
	char *name;
	FskErr err;

	if (!fsit)
		;
	else
	if (fsit->dirIt) {
		UInt32 itemType;

		err = FskDirectoryIteratorGetNext(fsit->dirIt, &name, &itemType);
		if (kFskErrNone == err) {
			xsVars(1);
			xsVar(0) = xsGet(xsGet(xsGlobal, fsit->id_FileSystem), fsit->id_fileInfo);
			xsResult = xsNewInstanceOf(xsVar(0));
			xsSet(xsResult, fsit->id_path, xsString(name));
			if (kFskDirectoryItemIsDirectory == itemType)
				xsSet(xsResult, fsit->id_type, xsGet(xsVar(0), fsit->id_directory));
			else if (kFskDirectoryItemIsLink == itemType)
				xsSet(xsResult, fsit->id_type, xsGet(xsVar(0), fsit->id_link));
			FskMemPtrDispose(name);
		}
		else {
			xs_FileSystem_iterator_close(the);
			xsResult = xsNull;
			if (kFskErrIteratorComplete != err)
				FskECMAScriptThrowIf(the, err);
		}
	}
	else if (fsit->volIt) {
		UInt32 id;
		char *path;

		err = FskVolumeIteratorGetNext(fsit->volIt, &id, &path, &name);
		if (kFskErrNone == err) {
			xsVars(1);

			xsVar(0) = xsGet(xsGet(xsGlobal, fsit->id_FileSystem), fsit->id_fileInfo);
			xsResult = xsNewInstanceOf(xsVar(0));
			xsSet(xsResult, fsit->id_path, xsString(path));
			if (name)
				xsSet(xsResult, fsit->id_name, xsString(name));
			xsSet(xsResult, fsit->id_type, xsString("volume"));
			xsSet(xsResult, fsit->id_id, xsInteger(id));
			FskMemPtrDispose(name);
			FskMemPtrDispose(path);
		}
		else {
			xs_FileSystem_iterator_close(the);
			xsResult = xsNull;
			if (kFskErrIteratorComplete != err)
				FskECMAScriptThrowIf(the, err);
		}
	}
}

void xs_FileSystem_iterator_close(xsMachine* the)
{
	xsNativeFSIterator fsit = (xsNativeFSIterator)xsGetHostData(xsThis);
	if (fsit) {
		FskDirectoryIteratorDispose(fsit->dirIt);
		FskVolumeIteratorDispose(fsit->volIt);
		FskMemPtrDispose(fsit);
		xsSetHostData(xsThis, NULL);
	}
}

/*
	Special Directory
*/

void xs_FileSystem_getSpecialDirectory(xsMachine *the)
{
	FskErr err;
	char *path;
	char *typeString = xsToString(xsArg(0));
	UInt32 type, typeFlags = 0;
	Boolean create;
	char *volume;
	SInt32 argc = xsToInteger(xsArgc);

	if (0 == FskStrCompareWithLength(typeString, "Shared/", 7)) {
		typeString += 7;
		typeFlags = kFskDirectorySpecialTypeSharedFlag;
	}

	create = argc >= 2 ? xsTest(xsArg(1)) : false;
	volume = argc >= 3 ? xsToString(xsArg(2)) : NULL;

	if (FskStrCompare(typeString, "Preferences") == 0) {
		type = kFskDirectorySpecialTypeApplicationPreference;
		if ((argc < 2) || (xsUndefinedType == xsTypeOf(xsArg(1))))
			create = true;
	}
	else if (FskStrCompare(typeString, "Document") == 0)
		type = kFskDirectorySpecialTypeDocument;
	else if (FskStrCompare(typeString, "Photo") == 0)
		type = kFskDirectorySpecialTypePhoto;
	else if (FskStrCompare(typeString, "Music") == 0)
		type = kFskDirectorySpecialTypeMusic;
	else if (FskStrCompare(typeString, "Video") == 0)
		type = kFskDirectorySpecialTypeVideo;
	else if (FskStrCompare(typeString, "TV") == 0)
		type = kFskDirectorySpecialTypeTV;
	else if (FskStrCompare(typeString, "PreferencesRoot") == 0)
		type = kFskDirectorySpecialTypeApplicationPreferenceRoot;
	else if (FskStrCompare(typeString, "Temporary") == 0)
		type = kFskDirectorySpecialTypeTemporary;
	else if (FskStrCompare(typeString, "Download") == 0)
		type = kFskDirectorySpecialTypeDownload;
	else if (FskStrCompare(typeString, "Start") == 0)
		type = kFskDirectorySpecialTypeStartMenu;
	else if (FskStrCompare(typeString, "MusicSync") == 0)
		type = kFskDirectorySpecialTypeMusicSync;
	else if (FskStrCompare(typeString, "PhotoSync") == 0)
		type = kFskDirectorySpecialTypePhotoSync;
	else if (FskStrCompare(typeString, "VideoSync") == 0)
		type = kFskDirectorySpecialTypeVideoSync;
	else if (FskStrCompare(typeString, "PlaylistSync") == 0)
		type = kFskDirectorySpecialTypePlaylistSync;
	else
		return;

	err = FskDirectoryGetSpecialPath(type | typeFlags, create, volume, &path);
	if (kFskErrNone == err) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}
	else if (kFskErrInvalidParameter == err)
		;		// unknown directory
	else
		FskECMAScriptThrowIf(the, err);
}

/*
	volume notifier
*/

typedef struct {
	FskVolumeNotifier	notifier;
	FskECMAScript		vm;
	xsSlot				obj;
} xsNativeVolumeNotifierRecord, *xsNativeVolumeNotifier;

static FskErr volumeNotifier(UInt32 status, UInt32 volumeID, void *refCon);

void xs_FileSystem_Notifier_Volume(xsMachine *the)
{
	xsNativeVolumeNotifier nvn;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(xsNativeVolumeNotifierRecord), &nvn);
	FskECMAScriptThrowIf(the, err);

	nvn->obj = xsThis;
	nvn->vm = (FskECMAScript)xsGetContext(the);
	err = FskVolumeNotifierNew(volumeNotifier, nvn, &nvn->notifier);
	if (err) {
		FskMemPtrDispose(nvn);
		FskECMAScriptThrowIf(the, err);
	}
	xsSetHostData(xsThis, nvn);
	FskInstrumentedItemSetOwner(nvn->notifier, nvn->vm);
}

void xs_FileSystem_notifier_volume_destructor(void *data)
{
	if (data) {
		xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)data;
		FskVolumeNotifierDispose(nvn->notifier);
		FskMemPtrDispose(nvn);
	}
}

void xs_FileSystem_notifier_volume_close(xsMachine *the)
{
	xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)xsGetHostData(xsThis);
	FskVolumeNotifierDispose(nvn->notifier);
	nvn->notifier = NULL;
}

FskErr volumeNotifier(UInt32 status, UInt32 volumeID, void *refCon)
{
	xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)refCon;

	xsBeginHost(nvn->vm->the);
		xsCall1_noResult(nvn->obj, xsID((kFskVolumeHello == status) ? "onFound" : "onLost"), xsInteger(volumeID));
	xsEndHost(nvn->vm->the);

	return kFskErrNone;
}

/*
	directory change notifier
*/

typedef struct {
	FskDirectoryChangeNotifier	notifier;
	FskECMAScript				vm;
	xsSlot						obj;
} xsNativeDirectoryChangeNotifierRecord, *xsNativeDirectoryChangeNotifier;

static FskErr directoryChangeNotifier(UInt32 flags, const char *path, void *refCon);

void xs_FileSystem_change_Notifier(xsMachine *the)
{
	FskErr err;
	xsNativeDirectoryChangeNotifier ndc;
	UInt32 flags = 0;

	err = FskMemPtrNewClear(sizeof(xsNativeDirectoryChangeNotifierRecord), &ndc);
	BAIL_IF_ERR(err);

	ndc->obj = xsThis;
	ndc->vm = (FskECMAScript)xsGetContext(the);
	xsSetHostData(xsThis, ndc);

	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
		flags |= kFskDirectoryChangeMonitorSubTree;

	err = FskDirectoryChangeNotifierNew(xsToString(xsArg(0)), flags, directoryChangeNotifier, ndc, &ndc->notifier);

bail:
	FskECMAScriptThrowIf(the, err);
}

void xs_FileSystem_change_notifier_destructor(void *data)
{
	if (NULL != data) {
		FskDirectoryChangeNotifierDispose(((xsNativeDirectoryChangeNotifier)data)->notifier);
		FskMemPtrDispose(data);
	}
}

void xs_FileSystem_change_notifier_close(xsMachine *the)
{
	xs_FileSystem_change_notifier_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

FskErr directoryChangeNotifier(UInt32 flags, const char *path, void *refCon)
{
	xsNativeDirectoryChangeNotifier ndc = (xsNativeDirectoryChangeNotifier)refCon;

	xsBeginHost(ndc->vm->the);
		if (path)
			xsCall2_noResult(ndc->obj, xsID("onUpdate"), xsString((char *)path), xsInteger(flags));
		else
			xsCall2_noResult(ndc->obj, xsID("onUpdate"), xsNull, xsInteger(flags));
	xsEndHost(ndc->vm->the);

	return kFskErrNone;
}

void xs_FileSystem_load(xsMachine *the)
{
	void *data;
	FskInt64 dataSize;

	if (kFskErrNone == FskFileLoad(xsToString(xsArg(0)), (FskMemPtr *)(void*)&data, &dataSize)) {		// always puts a trailing null on result, so we can safely treat it as a string
		int argc = xsToInteger(xsArgc);
		if ((argc > 1) && xsTest(xsArg(1)))
			xsMemPtrToChunk(the, &xsResult, (FskMemPtr)data, (UInt32)dataSize, false);
		else {
			if (FskTextUTF8IsValid(data, (SInt32)dataSize))
				xsResult = xsString(data);
			FskMemPtrDispose(data);
		}
	}
}

/*
	HTTP Client
*/

static FskErr xs_HTTPClient_finished(FskHTTPClient client, void *refCon);

void xs_HTTPClient(xsMachine* the)
{
	FskErr err;
	xsNativeHTTPClient nht;
	SInt32 priority;

 	err = FskMemPtrNewClear(sizeof(xsNativeHTTPClientRecord), &nht);
	BAIL_IF_ERR(err);

	priority = kFskNetSocketDefaultPriority;
	if (xsToInteger(xsArgc) > 0)
		priority = xsToInteger(xsArg(0));
	err = FskHTTPClientNewPrioritized(&nht->client, priority, "xs_HTTPClient");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(nht->client, nht);

	nht->obj = xsThis;
	nht->vm = (FskECMAScript)xsGetContext(the);
	nht->useCount = 1;

	xsSetHostData(xsThis, nht);

	FskHTTPClientSetFinishedCallback(nht->client, xs_HTTPClient_finished);

bail:
	if (err) {
		xs_HTTPClient_destructor(nht);
		FskECMAScriptThrowIf(the, err);
	}
}

void xs_HTTPClient_destructor(void *ref)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)ref;

	if (nht) {
		nht->useCount -= 1;
		if (0 == nht->useCount) {
			FskHTTPClientDispose(nht->client);
			FskMemPtrDispose(nht);
		}
	}
}

void xs_HTTPClient_close(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);

	if (!nht)
		return;

	FskHTTPClientCancel(nht->client);

	xs_HTTPClient_destructor(nht);
	xsSetHostData(xsThis, NULL);
}

void xs_HTTPClient_suspend(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	FskHTTPClientSuspend(nht->client);
}

void xs_HTTPClient_resume(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	FskHTTPClientResume(nht->client);
}

FskErr xs_HTTPClient_finished(FskHTTPClient client, void *refCon)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)refCon;
	xsMachine *the = nht->vm->the;

	xsBeginHost(the);
		xsCall1_noResult(nht->obj, xsID("onComplete"), xsInteger(client->status.lastErr));
	xsEndHost(the);

	return kFskErrNone;		// please don't dispose us
}
	
void xs_HTTPClient_isFinished(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);

	if (FskHTTPClientIsIdle(nht->client))
		xsResult = xsTrue;
	else {
		xsResult = xsFalse;
		FskThreadYield();
		FskThreadRunloopCycle(100);
	}
}

#define kHTTPClientPostBufferSize (8192)

static FskErr xs_HTTPRequest_sendRequestData(FskHTTPClientRequest req, char **buffer, int *bufferSize, FskInt64 position, void *refCon);
static FskErr xs_HTTPRequest_responseHeaderCallback(FskHTTPClientRequest req, FskHeaders *responseHeaders, void *refCon);
static FskErr xs_HTTPRequest_receiveDataCallback(FskHTTPClientRequest req, char *buffer, int bufferSize, void *refCon);
static FskErr xs_HTTPRequest_requestFinishedCallback(FskHTTPClient client, FskHTTPClientRequest req, void *refCon);

static void xs_HTTPRequest_empty(xsNativeHTTPRequest nhr);

void xs_HTTPClient_setUserPass(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	char *user, *pass;
	int passType = kFskHTTPAuthCredentialsTypeString;
	int argc = xsToInteger(xsArgc);

	if (0 == argc) {
		FskHTTPClientSetCredentials(nht->client, NULL, NULL, 0, kFskHTTPAuthCredentialsTypeNone);
		return;
	}

	if (argc > 1) {
		user = xsToString(xsArg(0));
		pass = xsToString(xsArg(1));
		if (argc > 2) {
			passType = xsToInteger(xsArg(2));
		}
/* Note: - if type isn't TypeString, then the size should be specified below */
		FskHTTPClientSetCredentials(nht->client, user, pass, 0, passType);
	}
}

void xs_HTTPClient_setCertificates(xsMachine *the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	void *certs = NULL, *certsString = NULL;
	int certsSize = 0;
	char *policies = NULL;

	if (ac < 1)
		return;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		certs = certsString = xsToStringCopy(xsArg(0));
		certsSize = FskStrLen(certs);
	}
	else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		certs = xsGetHostData(xsArg(0));
		certsSize = xsToInteger(xsGet(xsArg(0), xsID("length")));
	}
	if (ac >= 2 && xsTypeOf(xsArg(1)) == xsStringType)
		policies = xsToString(xsArg(1));
	FskHTTPClientSetCertificates(nht->client, certs, certsSize, policies);
	if (certsString != NULL)
		FskMemPtrDispose(certsString);
}

void xs_HTTPClient_setTimeout(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	if (!nht)
		FskECMAScriptThrowIf(the, kFskErrOutOfSequence);
	FskHTTPClientSetIdleTimeout(nht->client, xsToInteger(xsArg(0)));
}

void xs_HTTPClient_request(xsMachine* the)
{	
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	xsNativeHTTPRequest nhr;
	FskErr err;
	char *url;

	xsVars(1);

	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FskHTTP")), xsID("request")));

	url = xsToString(xsArg(0));

	err = FskMemPtrNewClear(sizeof(xsNativeHTTPRequestRecord), &nhr);
	BAIL_IF_ERR(err);

	xsSetHostData(xsResult, nhr);

	nhr->nht = nht;
	nhr->nht->useCount += 1;
	nhr->vm = nht->vm;
	nhr->obj = xsResult;

	err = FskHTTPClientRequestNew(&nhr->req, url);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(nhr->req, nhr);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(nhr->req, xs_HTTPRequest_responseHeaderCallback, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(nhr->req, xs_HTTPRequest_receiveDataCallback, NULL, 32768, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(nhr->req, xs_HTTPRequest_requestFinishedCallback);

	xsSet(xsResult, nht->vm->id_stream, xsNew0(xsGet(xsGlobal, xsID("Stream")), nht->vm->id_Chunk));
	xsVar(0) = xsNew0(xsGlobal, nht->vm->id_Chunk);
	xsSetHostDestructor(xsVar(0), NULL);
	xsSet(xsResult, nht->vm->id_buffer, xsVar(0));

bail:
	FskECMAScriptThrowIf(the, err);

	xsRemember(nhr->obj);
	nhr->needsForget = true;
}

void xs_HTTPRequest_destructor(void *data)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)data;

	xs_HTTPRequest_empty(nhr);

	FskMemPtrDispose(nhr);
}

void xs_HTTPRequest_empty(xsNativeHTTPRequest nhr)
{
	if (NULL == nhr)
		return;

	if (0 == nhr->stage) {
		FskHTTPClientRequestDispose(nhr->req);
		nhr->req = NULL;
	}
	FskMemPtrDisposeAt((void **)(void*)&nhr->postBuffer);

	xs_HTTPClient_destructor(nhr->nht);
	nhr->nht = NULL;
}

void xs_HTTPRequest_close(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	if (NULL == nhr)
		return;
	if (nhr->needsForget) {
		nhr->needsForget = false;
		xsForget(nhr->obj);
	}
	if (1 == nhr->stage)
		nhr->stage = 0;			// force to not-yet-added stage to force us to call dispose
	xs_HTTPRequest_empty(nhr);
}

void xs_HTTPRequest_setRequestMethod(xsMachine* the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	FskHTTPClientRequestSetMethod(nhr->req, xsToString(xsArg(0)));
}

void xs_HTTPRequest_addHeader(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	char *name = xsToStringCopy(xsArg(0));
	char *value = xsToString(xsArg(1));
	if (0 == FskStrCompareCaseInsensitive(name, "Content-Length"))
		nhr->hasContentLength = true;
	else
	if (0 == FskStrCompareCaseInsensitive(name, "User-Agent"))
		nhr->hasUserAgent = true;
	FskHTTPClientRequestAddHeader(nhr->req, name, value);
	FskMemPtrDispose(name);
}

void xs_HTTPRequest_getHeader(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	char *header;

	header = FskHeaderFind(xsToString(xsArg(0)), nhr->req->responseHeaders);
	if (header)
		xsResult = xsString(header);
}

void xs_HTTPRequest_start(xsMachine* the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	FskErr err;
	SInt32 argc = xsToInteger(xsArgc);
	Boolean havePostStream = false;
	Boolean doStart = true;

	if (0 != nhr->stage)
		FskECMAScriptThrowIf(the, kFskErrOutOfSequence);

	if (argc > 0) {
		if (xsReferenceType == xsTypeOf(xsArg(0)))
			havePostStream = true;
		else if (1 == argc)
			doStart = xsToBoolean(xsArg(0));

		if (argc >= 2)
			doStart = xsToBoolean(xsArg(1));
	}

	if (havePostStream) {
		// request message body stream
		xsSet(xsThis, xsID("postStream"), xsArg(0));
		FskHTTPClientRequestSetSendDataCallback(nhr->req, xs_HTTPRequest_sendRequestData);

		if (false == nhr->hasContentLength) {
			StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsArg(0));
			if (0 != aStreamPtr->bytesAvailable) {
				char str[40];
				FskStrNumToStr((UInt32)aStreamPtr->bytesAvailable, str, sizeof(str));
				FskHTTPClientRequestAddHeader(nhr->req, "Content-Length", str);
			}
		}
	}

	if (false == nhr->hasUserAgent) {
		char *userAgent = FskEnvironmentGet("http-user-agent");
		if (NULL != userAgent) {
			FskHTTPClientRequestRemoveHeader(nhr->req, "User-Agent");
			FskHTTPClientRequestAddHeader(nhr->req, "User-Agent", userAgent);
		}
	}

	err = FskHTTPClientAddRequest(nhr->nht->client, nhr->req);
	FskECMAScriptThrowIf(the, err);

	nhr->stage = 1;

	if (doStart) {
		err = FskHTTPClientBegin(nhr->nht->client);
		FskECMAScriptThrowIf(the, err);
	}
}

FskErr xs_HTTPRequest_sendRequestData(FskHTTPClientRequest req, char **buffer, int *bufferSize, FskInt64 position, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	{
		xsBeginHost(nhr->vm->the);
		{
			xsVars(3);

			// out with the old
			FskMemPtrDisposeAt((void **)(void*)&nhr->postBuffer);

			// in with the new
			xsVar(0) = xsGet(nhr->obj, xsID("postStream"));
			if (0 == position) {
				xsCall0_noResult(xsVar(0), xsID("rewind"));
			}
			else {
				// what to do when position isn't 0?
			}

			xsVar(1) = xsCall1(xsVar(0), nhr->vm->id_readChunk, xsInteger(kHTTPClientPostBufferSize));
			if (xsReferenceType == xsTypeOf(xsVar(1))) {
				*bufferSize = xsToInteger(xsGet(xsVar(1), nhr->vm->id_length));
				nhr->postBuffer = (char *)xsGetHostData(xsVar(1));

				xsSetHostData(xsVar(1), NULL);
				xsSet(xsVar(1), nhr->vm->id_length, xsInteger(0));

				*buffer = nhr->postBuffer;
			}
			else {
				xsCall0_noResult(xsVar(0), xsID("rewind"));
				err = kFskErrEndOfFile;	// all done
			}
			
			xsCall1_noResult(nhr->obj, xsID("onDataSent"), xsInteger((xsIntegerValue)req->status.bytesSent));
		}
		xsEndHost(nhr->vm->the);
	}

	return err;
}

FskErr xs_HTTPRequest_responseHeaderCallback(FskHTTPClientRequest req, FskHeaders *responseHeaders, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	UInt32 typeOf;
	FskECMAScript vm = nhr->vm;

	xsBeginHost(vm->the);
	{
		int response = FskHeaderResponseCode(responseHeaders);
		Boolean isRedirect = (301 == response) || (302 == response) || (305 == response) || (307 == response);
		if (false == isRedirect) {
			// if there's a MIME type in the header, attach it to the stream object
			char *mimeType = FskHeaderFind(kFskStrContentType, responseHeaders);
			if (mimeType)
				xsSet(xsGet(nhr->obj, vm->id_stream), xsID("mime"), xsString(mimeType));
		}
		else
			nhr->hasUserAgent = false;

		xsSet(nhr->obj, xsID("statusCode"), xsInteger(FskHeaderResponseCode(req->responseHeaders)));
		xsResult = xsCall0(nhr->obj, xsID("onHeaders"));

		typeOf = xsTypeOf(xsResult);
		if ((xsNumberType == typeOf) || (xsIntegerType == typeOf))
			err = xsToInteger(xsResult);

		if (isRedirect && nhr->req && (false == nhr->hasUserAgent)) {
			char *userAgent = FskEnvironmentGet("http-user-agent");
			if (NULL != userAgent) {
				FskHTTPClientRequestRemoveHeader(nhr->req, "User-Agent");
				FskHTTPClientRequestAddHeader(req, "User-Agent", userAgent);
			}
		}
	}
	xsEndHost(vm->the);

	return err;
}

FskErr xs_HTTPRequest_receiveDataCallback(FskHTTPClientRequest req, char *buffer, int bufferSize, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	xsBeginHost(nhr->vm->the);
	{
		xsVars(1);

		xsVar(0) = xsGet(nhr->obj, nhr->vm->id_buffer);
		xsSetHostData(xsVar(0), buffer);
		xsSet(xsVar(0), nhr->vm->id_length, xsInteger(bufferSize));

		xsResult = xsCall1(nhr->obj, xsID("onDataReady"), xsVar(0));
		err = xsToInteger(xsResult);
	}
	xsEndHost(nhr->vm->the);

	return err;
}

FskErr xs_HTTPRequest_requestFinishedCallback(FskHTTPClient client, FskHTTPClientRequest req, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;

	nhr->stage = 2;

	xsBeginHost(nhr->vm->the);
	{
		xsTry {
			xsIndex id_status = xsID("status");

			xsForget(nhr->obj);
			nhr->needsForget = false;

			xsSet(nhr->obj, id_status, xsInteger(client->status.lastErr));
			if (req->responseHeaders->headersParsed)
				xsSet(nhr->obj, xsID("statusCode"), xsInteger(FskHeaderResponseCode(req->responseHeaders)));

			xsVars(1);
			xsVar(0) = xsGet(nhr->obj, nhr->vm->id_stream);
			xsCall0_noResult(xsVar(0), xsID("flush"));
			xsCall0_noResult(xsVar(0), xsID("rewind"));
			xsResult = xsCall0(nhr->obj, xsID("onTransferComplete"));

			xsVar(0) = xsGet(nhr->obj, nhr->vm->id_stream);
			xsCall0_noResult(xsVar(0), nhr->vm->id_close);
		}
		xsCatch {
		}
	}

	xsEndHost(nhr->vm->the);

	return kFskErrNone;
}

#if TARGET_OS_WIN32
	#include "FskTextConvert.h"
	#include <wininet.h>

	enum
	{
		kHTTPSStateIdle = 0,
		kHTTPSStateConnecting,
		kHTTPSStateWaitingForResponse,
		kHTTPSStateReading,
		kHTTPSStateClosing,
		kHTTPSStateClosed
	};

	const UInt16 *kHTTPSVersion = L"HTTPS/1.1";

	struct xsNativeHTTPSRecord {
		struct xsNativeHTTPSRecord *next;
		xsMachine	*the;
		xsSlot		obj;
		FskMutex	mutex;
		FskThread	thread;
		FskResolver	resolver;
		HINTERNET	internetOpenHandle;
		HINTERNET	internetConnectHandle;
		HINTERNET	httpRequestHandle;
		DWORD		statusCode;
		UInt16		*pathW;
		UInt16		*methodW;
		UInt16		*userAgentW;
		xsSlot		stream;
		xsSlot		postStream;
		void		*postChunk;
		UInt32		postChunkLength;
		UInt32		state;
		char		**requestHeaders;
		UInt32		requestHeaderCount;
		Boolean		hasContentLength;
		Boolean		didResolve;
		Boolean		gotHeaders;
		SInt16		count;
	};

	typedef struct xsNativeHTTPSRecord xsNativeHTTPSRecord;
	typedef xsNativeHTTPSRecord *xsNativeHTTPS;

	FskListMutex gHTTPS;

	void HTTPSResolverCallback(FskResolver rr);
	void CALLBACK HTTPSStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	void HTTPSOpenRequest(void *param1, void *param2, void *param3, void *param4);
	void HTTPSSendRequest(void *param1, void *param2, void *param3, void *param4);
	void HTTPSResponseReceived(void *param1, void *param2, void *param3, void *param4);
	void HTTPSRequestComplete(void *param1, void *param2, void *param3, void *param4);
	void HTTPSClosingConnection(void *param1, void *param2, void *param3, void *param4);

	void HTTPSClose(xsNativeHTTPS nativeHTTPS);

	FskErr HTTPSAddHeader(xsNativeHTTPS nativeHTTPS, char *header, char *value);
	FskErr HTTPSGetHeaderValue(xsNativeHTTPS nativeHTTPS, char *header, char **value);
#endif

void xs_HTTPS_init(xsMachine *the)
{
#if OPEN_SSL
	xsResult = xsGet(xsGlobal, xsID("HTTP"));
	xsSet(xsResult, xsID("SSL"), xsTrue);
#endif
}

void xs_HTTPS_initialize(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;

	if (!gHTTPS) {
		err = FskListMutexNew(&gHTTPS, "https");
		FskECMAScriptThrowIf(the, err);
	}

	err = FskMemPtrNewClear(sizeof(xsNativeHTTPSRecord), &nativeHTTPS);
	BAIL_IF_ERR(err);

	nativeHTTPS->the = the;
	nativeHTTPS->obj = xsThis;
	nativeHTTPS->thread = FskThreadGetCurrent();
	nativeHTTPS->state = kHTTPSStateIdle;
	nativeHTTPS->count = 1;

	nativeHTTPS->stream = xsNew0(xsGet(xsGlobal, xsID("Stream")), xsID("Chunk"));
	xsSet(nativeHTTPS->obj, xsID("stream"), nativeHTTPS->stream);

	err = FskMutexNew(&nativeHTTPS->mutex, "nativeHTTPS");
	BAIL_IF_ERR(err);

	FskListMutexAppend(gHTTPS, nativeHTTPS);

	xsSetHostData(xsThis, nativeHTTPS);

bail:
	if (err && nativeHTTPS) {
		FskMutexDispose(nativeHTTPS->mutex);
		FskMemPtrDispose(nativeHTTPS);
	}

	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_destructor(void *hostData)
{
#if TARGET_OS_WIN32
	xsNativeHTTPS nativeHTTPS = hostData;

	if (nativeHTTPS)
		HTTPSClose(nativeHTTPS);
#endif
}

void xs_HTTPS_start(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS = NULL;
	char			*path, *method, *userAgent, contentLength[40];
	StreamBufferPtr streamBufferPtr;
	UInt32			bytesToRead;

	nativeHTTPS = xsGetHostData(xsThis);

	if (nativeHTTPS->state != kHTTPSStateIdle) {
		BAIL(kFskErrBadState);
	}

	nativeHTTPS->state = kHTTPSStateConnecting;

	if (xsToInteger(xsArgc) >= 1) {
		streamBufferPtr = (StreamBufferPtr)xsGetHostData(xsArg(0));

		xsSet(xsThis, xsID("_postStream"), xsArg(0));

		xsCall0_noResult(xsArg(0), xsID("rewind"));
		xsResult = xsCall0(xsArg(0), xsID("toChunk"));
		nativeHTTPS->postChunk = xsGetHostData(xsResult);
		bytesToRead = xsToInteger(xsGet(xsResult, xsID("length")));
		nativeHTTPS->postChunkLength = bytesToRead;
		xsSetHostData(xsResult, NULL);

		if (!nativeHTTPS->hasContentLength && bytesToRead) {
			FskStrNumToStr(bytesToRead, contentLength, sizeof(contentLength));

			HTTPSAddHeader(nativeHTTPS, "Content-Length", contentLength);
		}
	}

	path = xsToString(xsGet(xsThis, xsID("_path")));
	method = xsToString(xsGet(xsThis, xsID("_method")));
	userAgent = FskEnvironmentGet("http-user-agent");

	nativeHTTPS->statusCode = 0;

	if (path) {
		err = FskTextUTF8ToUnicode16LE(path, FskStrLen(path), &nativeHTTPS->pathW, NULL);
		BAIL_IF_ERR(err);
	}

	if (method) {
		err = FskTextUTF8ToUnicode16LE(method, FskStrLen(method), &nativeHTTPS->methodW, NULL);
		BAIL_IF_ERR(err);
	}
	else {
		err = FskMemPtrNewFromData(8, L"GET", &nativeHTTPS->methodW);
		BAIL_IF_ERR(err);
	}

	err = FskTextUTF8ToUnicode16LE(userAgent, FskStrLen(userAgent), &nativeHTTPS->userAgentW, NULL);
	BAIL_IF_ERR(err);

	nativeHTTPS->count += 1;
	err = FskNetHostnameResolveAsync(NULL, HTTPSResolverCallback, nativeHTTPS, &nativeHTTPS->resolver);
	BAIL_IF_ERR(err);

bail:
	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_addHeader(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS = NULL;
	char			*header, *value;

	nativeHTTPS = xsGetHostData(xsThis);

	if (xsToInteger(xsArgc) < 2) {
		BAIL(kFskErrInvalidParameter);
	}

	header = xsToStringCopy(xsArg(0));
	value = xsToStringCopy(xsArg(1));

	err = HTTPSAddHeader(nativeHTTPS, header, value);
	FskMemPtrDispose(header);
	FskMemPtrDispose(value);
	BAIL_IF_ERR(err);

bail:
	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_getHeader(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	char			*header, *value = NULL;

	nativeHTTPS = xsGetHostData(xsThis);

	if (xsToInteger(xsArgc) < 1) {
		xsResult = xsNull;

		return;
	}

	header = xsToString(xsArg(0));

	err = HTTPSGetHeaderValue(nativeHTTPS, header, &value);
	BAIL_IF_ERR(err);

	if (value)
		xsResult = xsString(value);
	else
		xsResult = xsNull;

bail:
	FskMemPtrDispose(value);
#endif
}

void xs_HTTPS_close(xsMachine *the)
{
#if TARGET_OS_WIN32
	xsNativeHTTPS nativeHTTPS;

	nativeHTTPS = xsGetHostData(xsThis);
	xsSetHostData(xsThis, NULL);

	xs_HTTPS_destructor(nativeHTTPS);
#endif
}

#if TARGET_OS_WIN32
void HTTPSResolverCallback(FskResolver rr)
{
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	char			*host;
	UInt16			*hostW = NULL;

	nativeHTTPS = rr->ref;

	if (nativeHTTPS->state == kHTTPSStateClosing) {
		HTTPSClose(nativeHTTPS);
		return;
	}
	else
		nativeHTTPS->count -= 1;

	if (rr->err == kFskErrNone) {
		xsBeginHost(nativeHTTPS->the);
			host = xsToString(xsGet(nativeHTTPS->obj, xsID("_host")));
			err = FskTextUTF8ToUnicode16LE(host, FskStrLen(host), &hostW, NULL);
		xsEndHost(nativeHTTPS->the);

		BAIL_IF_ERR(err);

		nativeHTTPS->internetOpenHandle = InternetOpenW(nativeHTTPS->userAgentW, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);

		InternetSetStatusCallback(nativeHTTPS->internetOpenHandle, HTTPSStatusCallback);

		nativeHTTPS->state = kHTTPSStateWaitingForResponse;
		nativeHTTPS->didResolve = true;

		InternetConnectW(nativeHTTPS->internetOpenHandle, hostW, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD)nativeHTTPS);
	}
	else
		nativeHTTPS->state = kHTTPSStateClosed;

	nativeHTTPS->resolver = NULL;

bail:
	FskMemPtrDispose(hostW);
}

void CALLBACK HTTPSStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	xsNativeHTTPS nativeHTTPS;

	nativeHTTPS = (xsNativeHTTPS)dwContext;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;

	FskMutexAcquire(nativeHTTPS->mutex);

	switch (dwInternetStatus) {
		case INTERNET_STATUS_HANDLE_CREATED:
			if ((hInternet == nativeHTTPS->internetOpenHandle) && (nativeHTTPS->internetConnectHandle == NULL)) {
				nativeHTTPS->internetConnectHandle = (HINTERNET)(((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult);
				nativeHTTPS->count += 1;

				FskThreadPostCallback(nativeHTTPS->thread, HTTPSOpenRequest, nativeHTTPS, NULL, NULL, NULL);
			}
			else if ((hInternet == nativeHTTPS->internetConnectHandle) && (nativeHTTPS->httpRequestHandle == NULL)) {
				nativeHTTPS->httpRequestHandle = (HINTERNET)(((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult);
				nativeHTTPS->count += 1;

				FskThreadPostCallback(nativeHTTPS->thread, HTTPSSendRequest, nativeHTTPS, NULL, NULL, NULL);
			}
			break;
		case INTERNET_STATUS_RESPONSE_RECEIVED:
			if (nativeHTTPS->state == kHTTPSStateWaitingForResponse) {
				nativeHTTPS->state = kHTTPSStateReading;
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSResponseReceived, nativeHTTPS, NULL, NULL, NULL);
			}
			break;
		case INTERNET_STATUS_REQUEST_COMPLETE:
			if (nativeHTTPS->state != kHTTPSStateClosing) {
				INTERNET_ASYNC_RESULT *iar = lpvStatusInformation;
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSRequestComplete, nativeHTTPS, (void *)(iar ? iar->dwError : ERROR_SUCCESS), NULL, NULL);
			}
			break;

		case INTERNET_STATUS_HANDLE_CLOSING:
			FskMutexRelease(nativeHTTPS->mutex);
			HTTPSClose(nativeHTTPS);
			return;

		case INTERNET_STATUS_CLOSING_CONNECTION:
			if ((nativeHTTPS->state != kHTTPSStateReading) && (nativeHTTPS->state != kHTTPSStateClosing))
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSClosingConnection, nativeHTTPS, NULL, NULL, NULL);
			break;
	}

	FskMutexRelease(nativeHTTPS->mutex);
}

void HTTPSOpenRequest(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	Boolean			close = false;

	nativeHTTPS = param1;

	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;

	HttpOpenRequestW(nativeHTTPS->internetConnectHandle, nativeHTTPS->methodW, nativeHTTPS->pathW, kHTTPSVersion, NULL, NULL, (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_CACHE_WRITE), (DWORD)nativeHTTPS);
}

void HTTPSSendRequest(void *param1, void *param2, void *param3, void *param4)
{
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	DWORD			securityFlags, bufferLength;
	char			*headers = NULL;
	UInt16			*headersW = NULL;
	UInt32			headersLength = 0, headerLength, valueLength, index;
	Boolean			close = false;

	nativeHTTPS = param1;
	bufferLength = sizeof(securityFlags);

	InternetQueryOption(nativeHTTPS->httpRequestHandle, INTERNET_OPTION_SECURITY_FLAGS, &securityFlags, &bufferLength);
	securityFlags |= (SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID);
	InternetSetOption(nativeHTTPS->httpRequestHandle, INTERNET_OPTION_SECURITY_FLAGS, &securityFlags, bufferLength);

	if (nativeHTTPS->requestHeaderCount > 0) {
		for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
			headerLength = FskStrLen(nativeHTTPS->requestHeaders[index * 2]);
			valueLength = FskStrLen(nativeHTTPS->requestHeaders[(index * 2) + 1]);

			err = FskMemPtrRealloc(headersLength + headerLength + valueLength + 4 + 1, &headers);
			BAIL_IF_ERR(err);

			if (headersLength == 0)
				headers[0] = 0;

			FskStrCat(headers, nativeHTTPS->requestHeaders[index * 2]);
			FskStrCat(headers, ": ");
			FskStrCat(headers, nativeHTTPS->requestHeaders[(index * 2) + 1]);
			FskStrCat(headers, "\r\n");

			headersLength += headerLength + valueLength + 4;
		}

		err = FskTextUTF8ToUnicode16LE(headers, FskStrLen(headers), &headersW, NULL);
		BAIL_IF_ERR(err);

		HttpAddRequestHeadersW(nativeHTTPS->httpRequestHandle, headersW, wcslen(headersW), (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));
	}

	if (nativeHTTPS->postChunk) {
		xsBeginHost(nativeHTTPS->the);
			HttpSendRequestW(nativeHTTPS->httpRequestHandle, NULL, 0, nativeHTTPS->postChunk, nativeHTTPS->postChunkLength);

			xsCall1_noResult(nativeHTTPS->obj, xsID("onDataSent"), xsInteger(nativeHTTPS->postChunkLength));
		xsEndHost(nativeHTTPS->the);
	}
	else
		HttpSendRequestW(nativeHTTPS->httpRequestHandle, NULL, 0, NULL, 0);

bail:
	FskMemPtrDispose(headers);
	FskMemPtrDispose(headersW);
}

void HTTPSResponseReceived(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	xsMachine		*the;
	DWORD			statusCodeSize;
	char			*value = NULL;
	Boolean			close = false;

	nativeHTTPS = param1;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;
	the = nativeHTTPS->the;

	xsBeginHost(the);
	FskMutexAcquire(nativeHTTPS->mutex);

		statusCodeSize = sizeof(nativeHTTPS->statusCode);

		if (HttpQueryInfo(nativeHTTPS->httpRequestHandle, (HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER), &nativeHTTPS->statusCode, &statusCodeSize, NULL)) {
			xsSet(nativeHTTPS->obj, xsID("statusCode"), xsInteger(nativeHTTPS->statusCode));

			if ((HTTPSGetHeaderValue(nativeHTTPS, "Content-Type", &value) == kFskErrNone) && value)
				xsSet(xsGet(nativeHTTPS->obj, xsID("stream")), xsID("mime"), xsString(value));

			nativeHTTPS->gotHeaders = true;
		}
	FskMutexRelease(nativeHTTPS->mutex);

	if (nativeHTTPS->gotHeaders)
		xsCall0_noResult(nativeHTTPS->obj, xsID("onHeaders"));

	xsEndHost(the);

	FskMemPtrDispose(value);
}

void HTTPSRequestComplete(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS		nativeHTTPS;
	char				*value = NULL;
	DWORD				statusCodeSize;
	Boolean				done = false;
	xsMachine			*the;
	char				data[1024];
	UInt32				totalBufferSize = 0;
	DWORD				result = (DWORD)param2;

	nativeHTTPS = param1;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;
	the = nativeHTTPS->the;

	xsBeginHost(the);

	if (ERROR_SUCCESS != result) {
		FskErr status;

		if (12007 == result)	// ERROR_WINHTTP_NAME_NOT_RESOLVED 
			status = kFskErrNameLookupFailed;
		else
			status = kFskErrNetworkErr;

		done = true;

		xsSet(nativeHTTPS->obj, xsID("status"), xsInteger(status));

		goto done;
	}

	if (!nativeHTTPS->gotHeaders) {
		statusCodeSize = sizeof(nativeHTTPS->statusCode);

		if (HttpQueryInfo(nativeHTTPS->httpRequestHandle, (HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER), &nativeHTTPS->statusCode, &statusCodeSize, NULL)) {
			xsSet(nativeHTTPS->obj, xsID("statusCode"), xsInteger(nativeHTTPS->statusCode));

			if ((HTTPSGetHeaderValue(nativeHTTPS, "Content-Type", &value) == kFskErrNone) && value)
				xsSet(xsGet(nativeHTTPS->obj, xsID("stream")), xsID("mime"), xsString(value));

			nativeHTTPS->gotHeaders = true;

			xsCall0_noResult(nativeHTTPS->obj, xsID("onHeaders"));
		}
	}

	while (true) {
		INTERNET_BUFFERSA	buffers = {0};

		buffers.dwStructSize = sizeof(buffers);
		buffers.lpvBuffer = data;
		buffers.dwBufferLength = sizeof(data);
		if (!InternetReadFileExA(nativeHTTPS->httpRequestHandle, &buffers, (IRF_ASYNC | IRF_USE_CONTEXT | IRF_NO_WAIT), (DWORD_PTR)nativeHTTPS))
			break;

		if (buffers.dwBufferLength <= 0) {
			done = true;
			break;
		}
		if (totalBufferSize == 0)
			xsResult = xsNew0(xsGlobal, xsID("Chunk"));

		xsSet(xsResult, xsID("length"), xsInteger(totalBufferSize + buffers.dwBufferLength));
		FskMemMove((char *)xsGetHostData(xsResult) + totalBufferSize, data, buffers.dwBufferLength);
		totalBufferSize += buffers.dwBufferLength;
	}

	if (totalBufferSize > 0)
		xsCall1_noResult(nativeHTTPS->obj, xsID("onDataReady"), xsResult);

done:
	if (done && (kHTTPSStateClosing != nativeHTTPS->state)) {
		xsCall0_noResult(nativeHTTPS->stream, xsID("rewind"));
		xsCall0_noResult(nativeHTTPS->obj, xsID("onTransferComplete"));
	}

	xsEndHost(the);

	FskMemPtrDispose(value);
}

void HTTPSClosingConnection(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	xsMachine		*the;

	nativeHTTPS = param1;
	the = nativeHTTPS->the;

	xsBeginHost(the);
		xsSet(nativeHTTPS->obj, xsID("status"), xsInteger(-1));

		xsCall0_noResult(nativeHTTPS->obj, xsID("onTransferComplete"));
	xsEndHost(the);
}

void HTTPSClose(xsNativeHTTPS nativeHTTPS)
{
	UInt32 index;

#if TARGET_OS_WIN32

	FskMutexAcquire(nativeHTTPS->mutex);

	if (kHTTPSStateClosing != nativeHTTPS->state) {
		nativeHTTPS->state = kHTTPSStateClosing;

		if (nativeHTTPS->httpRequestHandle) {
			InternetCloseHandle(nativeHTTPS->httpRequestHandle);
			nativeHTTPS->httpRequestHandle = NULL;
		}

		if (nativeHTTPS->internetConnectHandle) {
			InternetCloseHandle(nativeHTTPS->internetConnectHandle);
			nativeHTTPS->internetConnectHandle = NULL;
		}

		InternetSetStatusCallback(nativeHTTPS->internetOpenHandle, NULL);
		InternetCloseHandle(nativeHTTPS->internetOpenHandle);
		nativeHTTPS->internetOpenHandle = NULL;
	}

	nativeHTTPS->count -= 1;
	if (nativeHTTPS->count > 0) {
		FskMutexRelease(nativeHTTPS->mutex);
		return;
	}

	FskMutexRelease(nativeHTTPS->mutex);

	FskListMutexRemove(gHTTPS, nativeHTTPS);

	FskMemPtrDispose(nativeHTTPS->postChunk);
	FskMemPtrDispose(nativeHTTPS->pathW);
	FskMemPtrDispose(nativeHTTPS->methodW);
	FskMemPtrDispose(nativeHTTPS->userAgentW);
	for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
		FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2]);
		FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2 + 1]);
	}
	FskMemPtrDispose(nativeHTTPS->requestHeaders);
	FskMutexDispose(nativeHTTPS->mutex);
	FskMemPtrDispose(nativeHTTPS);
#endif
}

FskErr HTTPSAddHeader(xsNativeHTTPS nativeHTTPS, char *header, char *value)
{
	FskErr	err = kFskErrNone;
	UInt32	index;
	Boolean	replaced = false;

	if (FskStrCompareCaseInsensitive(header, "Content-Length") == 0)
		nativeHTTPS->hasContentLength = true;

	for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
		if (FskStrCompare(nativeHTTPS->requestHeaders[index * 2], header) == 0) {
			FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2 + 1]);
			nativeHTTPS->requestHeaders[index * 2 + 1] = FskStrDoCopy(value);
			replaced = true;
			break;
		}
	}

	if (!replaced) {
		err = FskMemPtrRealloc(sizeof(char *) * 2 * (nativeHTTPS->requestHeaderCount + 1), &nativeHTTPS->requestHeaders);
		BAIL_IF_ERR(err);

		nativeHTTPS->requestHeaders[nativeHTTPS->requestHeaderCount * 2] = FskStrDoCopy(header);
		nativeHTTPS->requestHeaders[(nativeHTTPS->requestHeaderCount * 2) + 1] = FskStrDoCopy(value);
		nativeHTTPS->requestHeaderCount++;
	}

bail:
	return err;
}

FskErr HTTPSGetHeaderValue(xsNativeHTTPS nativeHTTPS, char *header, char **value)
{
	FskErr	err = kFskErrNone;
	UInt16	*headerW = NULL;
	DWORD	valueSize;
	char	*h = NULL;
	DWORD	i = 0, j;

	*value = NULL;

	while (true) {
		FskMemPtrDisposeAt(&headerW);

		err = FskTextUTF8ToUnicode16LE(header, FskStrLen(header), &headerW, NULL);
		BAIL_IF_ERR(err);

		valueSize = wcslen(headerW);

		j = i;
		if (!HttpQueryInfoW(nativeHTTPS->httpRequestHandle, HTTP_QUERY_CUSTOM, headerW, &valueSize, &j)) {
			DWORD winErr = GetLastError();

			if (winErr == 0) {
				goto bail;
			}
			if (winErr != ERROR_INSUFFICIENT_BUFFER) goto bail;

			err = FskMemPtrRealloc(valueSize << 1, &headerW);			// contrary to documentation, valueSize in this case is characters not bytes.
			BAIL_IF_ERR(err);

			j = i;
			if (!HttpQueryInfoW(nativeHTTPS->httpRequestHandle, HTTP_QUERY_CUSTOM, headerW, &valueSize, &j))
				goto bail;
		}
		i = j;

		err = FskTextUnicode16LEToUTF8(headerW, valueSize, &h, NULL);
		BAIL_IF_ERR(err);

		if (*value) {
			char *tmp = *value;
			*value = FskStrDoCat(tmp, ",");
			FskMemPtrDispose(tmp); tmp = *value;
			*value = FskStrDoCat(tmp, h);
			FskMemPtrDisposeAt(&h);
		}
		else {
			*value = h;
			h = NULL;
		}
	}

bail:
	FskMemPtrDispose(headerW);
	FskMemPtrDispose(h);

	if ((kFskErrNone == err) && (NULL == *value))
		err = kFskErrNotFound;

	return err;
}
#endif

void xs_Network_set(xsMachine *the)
{
	do_setMediaProperty(the, NULL, FskNetworkHasProperty, FskNetworkSetProperty);
}

void xs_Network_get(xsMachine *the)
{
	do_getMediaProperty(the, NULL, FskNetworkGetProperty);
}

typedef struct {
	xsMachine		*the;
	xsSlot			obj;
	FskNetInterfaceNotifier netInterfaceNotifier;
} xsNativeNetworkNotifierRecord, *xsNativeNetworkNotifier;

static void networkNotifier(int what, int message, void *refCon);
static int networkInterfaceNotifier(struct FskNetInterfaceRecord *iface, UInt32 status, void *params);

void xs_Network_Notifier(xsMachine *the)
{
	FskErr err;
	xsNativeNetworkNotifier nn;
	int what = xsToInteger(xsArg(0));

	err = FskMemPtrNew(sizeof(xsNativeNetworkNotifierRecord), &nn);
	FskECMAScriptThrowIf(the, err);

	xsSetHostData(xsThis, nn);
	nn->the = the;
	nn->obj = xsThis;

	if (what == kFskNetNotificationNetworkInterfaceChanged)
		nn->netInterfaceNotifier = FskNetInterfaceAddNotifier(networkInterfaceNotifier, nn, "network notifier");
	else {
		FskNetNotificationNew(what, networkNotifier, nn);
		nn->netInterfaceNotifier = NULL;
	}
}

void xs_Network_notifier_destructor(void *data)
{
	if (data) {
		xsNativeNetworkNotifier nn = data;
		if (nn->netInterfaceNotifier)
			FskNetInterfaceRemoveNotifier(nn->netInterfaceNotifier);
		else
			FskNetNotificationDispose(networkNotifier, data);
		FskMemPtrDispose(data);
	}
}

void xs_Network_notifier_close(xsMachine *the)
{
	xs_Network_notifier_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void networkNotifier(int what, int message, void *refCon)
{
	xsNativeNetworkNotifier nn = refCon;

	xsBeginHost(nn->the);
		xsCall0_noResult(nn->obj, xsID("onUpdate"));
	xsEndHost(nn->the);
}

int networkInterfaceNotifier(struct FskNetInterfaceRecord *iface, UInt32 status, void *params)
{
	xsNativeNetworkNotifier nn = params;
	char *ss;
	Boolean flag;

	switch (status) {
	case kFskNetInterfaceStatusNew:
		ss = "new";
		flag = iface->status;
		break;
	case kFskNetInterfaceStatusChanged:
		ss = "changed";
		flag = iface->status;
		break;
	case kFskNetInterfaceStatusRemoved:
		ss = "removed";
		flag = 0;
		break;
	default:
		return 0;
	}
	xsBeginHost(nn->the);
		xsCall2_noResult(nn->obj, xsID("onUpdate"), xsString(ss), xsBoolean(flag));
	xsEndHost(nn->the);
	return 0;
}

static char *getStringCopy(xsMachine *the, xsSlot *slot);

void xs_FileSystem_chooseFile(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	Boolean allowMultiple = (argc >= 3) ? xsToBoolean(xsArg(2)) : false;
	char *initialPath = (argc >= 4) ? getStringCopy(the, &xsArg(3)) : NULL;
	char *files;
	FskFileChooseEntry fileTypes = NULL;

	xsVars(3);

#if TARGET_OS_WIN32 || TARGET_OS_MAC
	if ((argc >= 1) && (xsTypeOf(xsArg(0)) != xsNullType)) {
		SInt32 length = xsToInteger(xsGet(xsArg(0), xsID("length"))), i;

		err = FskMemPtrNewClear(sizeof(FskFileChooseEntryRecord) * (length + 1), (FskMemPtr *)&fileTypes);
		BAIL_IF_ERR(err);

		for (i = 0; i < length; i += 2) {
			char *extension;
			UInt32 typeOf;

			xsVar(0) = xsGet(xsArg(0), (xsIndex)i);
			typeOf = xsTypeOf(xsVar(0));
			if (xsStringType == typeOf) {
				if (0 != FskStrCompare(xsToString(xsVar(0)), "*/*"))
					extension = xsToString(xsCall1(xsGet(xsGlobal, xsID("FileSystem")), xsID("getExtensionFromMIMEType"), xsVar(0)));
				else
					extension = "*";

				err = FskMemPtrNewClear(FskStrLen(extension) + 2, (FskMemPtr *)&fileTypes[i / 2].extension);
				BAIL_IF_ERR(err);

				FskStrCopy(fileTypes[i / 2].extension, extension);
			}
			else if ((xsReferenceType == typeOf) && (xsIsInstanceOf(xsArg(0), xsArrayPrototype))) {
				UInt32 extensionLength = 0, count, j;

				count = xsToInteger(xsGet(xsVar(0), xsID("length")));
				for (j=0; j < count; j++) {
					xsVar(1) = xsGet(xsVar(0), (xsIndex)j);
					if (0 != FskStrCompare(xsToString(xsVar(1)), "*/*"))
						extension = xsToString(xsCall1(xsGet(xsGlobal, xsID("FileSystem")), xsID("getExtensionFromMIMEType"), xsVar(1)));
					else
						extension = "*";

					err = FskMemPtrRealloc(extensionLength + 2 + FskStrLen(extension), (FskMemPtr *)&fileTypes[i / 2].extension);
					BAIL_IF_ERR(err);

					FskMemMove(fileTypes[i / 2].extension + extensionLength, extension, FskStrLen(extension) + 1);
					extensionLength += FskStrLen(extension) + 1;
					fileTypes[i / 2].extension[extensionLength] = 0;
				}
			}

			if ((i + 1) < length)
				fileTypes[i / 2].label = xsToStringCopy(xsGet(xsArg(0), (xsIndex)(i + 1)));
		}
	}

#endif

	err = FskFileChoose(fileTypes, prompt, allowMultiple, initialPath, &files);
	if ((kFskErrNone == err) && (NULL != files)) {
		char *walker;
		xsIndex i = 0;
		xsResult = xsNewInstanceOf(xsArrayPrototype);
		for (walker = files; 0 != *walker; walker += (FskStrLen(walker) + 1))
			xsSet(xsResult, i++, xsString(walker));
		FskMemPtrDispose(files);
	}

#if TARGET_OS_WIN32 || TARGET_OS_MAC
bail:
	if (NULL != fileTypes) {
		FskFileChooseEntry walker;

		for (walker = fileTypes; NULL != walker->extension; walker++) {
			FskMemPtrDispose(walker->extension);
			FskMemPtrDispose(walker->label);
		}
		FskMemPtrDispose(fileTypes);
	}
#endif

	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialPath);
}

void xs_FileSystem_chooseFileSave(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *defaultName = (argc >= 1) ? getStringCopy(the, &xsArg(0)) : NULL;
	char *prompt = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	char *initialDirectory = (argc >= 3) ? getStringCopy(the, &xsArg(2)) : NULL;
	char *file;

	err = FskFileChooseSave(defaultName, prompt, initialDirectory, &file);
	if ((kFskErrNone == err) && (NULL != file)) {
		xsResult = xsString(file);
		FskMemPtrDispose(file);
	}

	FskMemPtrDispose(defaultName);
	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialDirectory);
}

void xs_FileSystem_chooseDirectory(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = (argc >= 1) ? getStringCopy(the, &xsArg(0)) : NULL;
	char *initialPath = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	char *path;

	err = FskDirectoryChoose(prompt, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}

	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialPath);
}

char *getStringCopy(xsMachine *the, xsSlot *slot)
{
	int slotType = xsTypeOf(*slot);
	if ((xsUndefinedType == slotType) || (xsNullType == slotType))
		return NULL;
	return xsToStringCopy(*slot);
}

/*
	in-memory http cache
*/

typedef struct {
	UInt32		flags;

	SInt32		id;
	char		*uri;
	double		last;
	SInt32		count;
	char		*ETag;
	char		*lastModified;
	double		expires;
	char		*mime;
	SInt32		size;
	char		*compress;
} xsHTTPCacheEntryRecord, *xsHTTPCacheEntry;

void xsHTTPCacheEntryConstructor(xsMachine *the)
{
	xsHTTPCacheEntry entry;

	FskMemPtrNewClear(sizeof(xsHTTPCacheEntryRecord), &entry);
	xsSetHostData(xsThis, entry);
}

void xsHTTPCacheEntryDestructor(void *data)
{
	xsHTTPCacheEntry entry = data;
	
	if (!data) return;

	FskMemPtrDispose(entry->uri);
	FskMemPtrDispose(entry->ETag);
	FskMemPtrDispose(entry->lastModified);
	FskMemPtrDispose(entry->mime);
	FskMemPtrDispose(entry->compress);
	FskMemPtrDispose(data);
}

void xsHTTPCacheEntrySet_id(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->id = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_id(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->id);
}

void xsHTTPCacheEntrySet_last(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->last = xsToNumber(xsArg(0));
}

void xsHTTPCacheEntryGet_last(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsNumber(entry->last);
}

void xsHTTPCacheEntrySet_expires(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->expires = xsToNumber(xsArg(0));
}

void xsHTTPCacheEntryGet_expires(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsNumber(entry->expires);
}

void xsHTTPCacheEntrySet_count(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->count = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_count(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->count);
}

void xsHTTPCacheEntrySet_size(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->size = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_size(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->size);
}


void xsHTTPCacheEntrySet_uri(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->uri);
	entry->uri = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_uri(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->uri) xsResult = xsString(entry->uri);
}

void xsHTTPCacheEntrySet_ETag(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->ETag);
	entry->ETag = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_ETag(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->ETag) xsResult = xsString(entry->ETag);
}

void xsHTTPCacheEntrySet_lastModified(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->lastModified);
	entry->lastModified = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_lastModified(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->lastModified) xsResult = xsString(entry->lastModified);
}

void xsHTTPCacheEntrySet_mime(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->mime);
	entry->mime = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_mime(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->mime) xsResult = xsString(entry->mime);
}

void xsHTTPCacheEntrySet_compress(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->compress);
	entry->compress = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_compress(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->compress) xsResult = xsString(entry->compress);
}

void xsHTTPCacheFind(xsMachine *the)
{
	SInt32 i, length;
	char *uri = xsToString(xsArg(0));

	xsVars(4);
	
	xsVar(0) = xsGet(xsThis, xsID("entries"));
	length = xsToInteger(xsGet(xsVar(0), xsID("length")));
	for (i = 0; i < length; i++) {
		xsHTTPCacheEntry entry;
		xsVar(1) = xsGet(xsVar(0), i);
		entry = xsGetHostData(xsVar(1));
		if (FskStrCompare(entry->uri, uri))
			continue;

		xsResult = xsVar(1);
		break;
	}
}

#endif
