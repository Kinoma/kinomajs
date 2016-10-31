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
#include "FskManifest.xs.h"
#define FskECMAScriptThrowIf(THE, ERROR) xsThrowIfFskErr(ERROR)
#include "FskFiles.h"
#include "FskNetUtils.h"
#include "FskNetInterface.h"
#include "FskUtilities.h"
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
	StreamBufferPtr aStreamPtr;
	FskErr err = FskMemPtrNewClear(sizeof(StreamBuffer), &aStreamPtr);
	FskECMAScriptThrowIf(the, err);

	FskMemSet(aStreamPtr, 0, sizeof(StreamBuffer));
	xsSetHostData(xsThis, aStreamPtr);

	aStreamPtr->the = the;
	aStreamPtr->obj = xsThis;
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
		if (!theStreamPtr->wbuf || theSize > theStreamPtr->wsize - theStreamPtr->wcount)
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

