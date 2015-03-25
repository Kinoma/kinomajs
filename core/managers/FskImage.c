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
#define __FSKIMAGE_PRIV__

#include "FskFiles.h"
#include "FskImage.h"
#include "FskExtensions.h"

FskErr FskImageCodecInitialize(void)
{
	static FskImageDecompressorRecord imageDecompressors[] = {
		{NULL, NULL, NULL, NULL}
	};
	static FskImageCompressorRecord imageCompressors[] = {
		{NULL, NULL, NULL, NULL}
	};
	FskImageDecompressor walkerD;
	FskImageCompressor walkerC;

	for (walkerD = imageDecompressors; NULL != walkerD->doCanHandle; walkerD++)
		FskImageDecompressorInstall(walkerD);

	for (walkerC = imageCompressors; NULL != walkerC->doCanHandle; walkerC++)
		FskImageCompressorInstall(walkerC);

	return kFskErrNone;
}

/*
	Decompress
*/

#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
	typedef struct {
		void						*next;

		FskImageDecompress			deco;
		FskImageDecompressComplete	completionFunction;
		void						*completionRefcon;

		const void					*data;
		UInt32						dataSize;

		Boolean						ownIt;

		FskInt64					decodeTime;
		FskInt64					*decodeTimePtr;
		UInt32						compositionTimeOffset;
		UInt32						*compositionTimeOffsetPtr;
		FskInt64					*compositionTime;

		UInt32						frameType;

		UInt32						propertyID;
		FskMediaPropertyValueRecord	property;

		void						*ref;
	} FskImageDecompressAsyncDecompressRecord, *FskImageDecompressAsyncDecompress;

	static FskThread		gImageDecompressThread;
	static FskSemaphore	gImageDecompressSemaphore;
	static FskListMutex	gImageDecompressAsyncList;
	static Boolean		gImageDecompressThreadDone;

	static void doDecompressThread(void *refcon);
#endif

static FskErr doDecompressFrame (FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bits, Boolean ownIt, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType);
static FskErr doSetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property);

FskErr FskImageCodecTerminate(void)
{
#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
	if (NULL != gImageDecompressThread) {
		gImageDecompressThreadDone = true;
		FskSemaphoreRelease(gImageDecompressSemaphore);
		while (gImageDecompressThreadDone)
			FskThreadYield();

		while (true) {
			FskImageDecompressAsyncDecompress async = (FskImageDecompressAsyncDecompress)FskListMutexRemoveFirst(gImageDecompressAsyncList);
			if (NULL == async) break;

			if (async->completionFunction)
				((FskImageDecompressComplete)async->completionFunction)(NULL, async->completionRefcon, kFskErrShutdown, NULL);
			FskMemPtrDispose(async);
		}
	}

	FskSemaphoreDispose(gImageDecompressSemaphore);
	FskListMutexDispose(gImageDecompressAsyncList);
#endif
	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageImageDecompress(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gImageDecompressTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"imagedecompress",
		FskInstrumentationOffset(FskImageDecompressRecord),
		NULL,
		0,
		NULL,
		doFormatMessageImageDecompress
	};
#endif

FskErr FskImageDecompressSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	UInt32 i = 0;

	while (true) {
		FskImageDecompressor aDispatch = (FskImageDecompressor)FskExtensionGetByIndex(kFskExtensionImageDecompressor, i++);
		if (NULL == aDispatch)
			break;

		if (NULL == aDispatch->doSniff)
			continue;

		if ((kFskErrNone == (aDispatch->doSniff)(data, dataSize, headers, uri, mime)) && (NULL != *mime))
			return kFskErrNone;
	}

	return kFskErrExtensionNotFound;
}

// can call with NULL for decoOut to determine if parameters are supported
FskErr FskImageDecompressNew(FskImageDecompress *decoOut, UInt32 format, const char *mimeType, const char *extension)
{
	FskErr err = kFskErrNone;
	FskImageDecompressor decoder;
	FskImageDecompress deco = NULL;
	UInt32 i = 0;

	while (true) {
		Boolean canHandle = false;
		decoder = (FskImageDecompressor)FskExtensionGetByIndex(kFskExtensionImageDecompressor, i++);
		if (!decoder) {
			if (NULL != decoOut)
				*decoOut = NULL;
			return kFskErrExtensionNotFound;
		}

		if ((kFskErrNone == decoder->doCanHandle(format, mimeType, extension, &canHandle)) && canHandle)
			break;
	}

	if (NULL == decoOut) goto bail;

	err = FskMemPtrNewClear(sizeof(FskImageDecompressRecord), &deco);
	BAIL_IF_ERR(err);

	deco->decoder = decoder;

	FskInstrumentedItemNew(deco, mimeType, &gImageDecompressTypeInstrumentation);

	if (decoder->doNew) {
		err = decoder->doNew(deco, format, mimeType, extension);
		BAIL_IF_ERR(err);
	}

bail:
	if (err) {
		FskImageDecompressDispose(deco);
		deco = NULL;
	}
	if (decoOut)
		*decoOut = deco;

	return err;
}

FskErr FskImageDecompressDispose(FskImageDecompress deco)
{
	if (deco) {
#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
		FskImageDecompressFlush(deco);
#endif
		if (deco->decoder->doDispose)
			deco->decoder->doDispose(deco->state, deco);
		if (false == deco->privateBits)
			FskBitmapDispose(deco->bits);
		FskMemPtrDispose(deco->spooler);
		FskInstrumentedItemDispose(deco);
		FskMemPtrDispose(deco);
	}
	return kFskErrNone;
}

FskErr FskImageDecompressRequestSize(FskImageDecompress deco, UInt32 width, UInt32 height)
{
	deco->requestedWidth = width;
	deco->requestedHeight = height;
	return kFskErrNone;
}

FskErr FskImageDecompressRequestedOutputFormat(FskImageDecompress deco, FskBitmapFormatEnum pixelFormat)
{
	if (deco->initializeComplete)
		return kFskErrOutOfSequence;

	deco->requestedPixelFormat = pixelFormat;
	return kFskErrNone;
}

FskErr FskImageDecompressSetImageDescription(FskImageDecompress deco, void *desc)
{
	FskMediaPropertyValueRecord value;
	value.type = kFskMediaPropertyTypeData;
	value.value.data.data = desc;
	value.value.data.dataSize = *(UInt32 *)desc;
	return FskImageDecompressSetProperty(deco, kFskMediaPropertySampleDescription, &value);
}

FskErr FskImageDecompressHasProperty(FskImageDecompress deco, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(deco->decoder->properties, propertyID, get, set, dataType);
}

FskErr FskImageDecompressSetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property)
{
#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
	FskErr err;

	if (NULL != gImageDecompressAsyncList) {
		FskImageDecompressAsyncDecompress walker;

		FskMutexAcquire(gImageDecompressAsyncList->mutex);

		for (walker = (FskImageDecompressAsyncDecompress)gImageDecompressAsyncList->list; NULL != walker; walker = (FskImageDecompressAsyncDecompress)walker->next) {
			if (walker->deco == deco)
				break;
		}

		if ((NULL != walker) || deco->decompressing) {
			FskImageDecompressAsyncDecompress async;

			err = FskMemPtrNewClear(sizeof(FskImageDecompressAsyncDecompressRecord), &async);
			BAIL_IF_ERR(err);

			async->deco = deco;
			async->propertyID = propertyID & 0x7fffffff;
			async->property = *property;

			if (!(propertyID & 0x80000000))
				FskListAppend(&gImageDecompressAsyncList->list, async);
			else
				FskListPrepend(&gImageDecompressAsyncList->list, async);

			// get it started
			FskSemaphoreRelease(gImageDecompressSemaphore);

bail:
			FskMutexRelease(gImageDecompressAsyncList->mutex);
			return err;
		}

		FskMutexRelease(gImageDecompressAsyncList->mutex);

		// fall through to synchronous setproperty
	}
#endif
	return doSetProperty(deco, propertyID & 0x7fffffff, property);
}

FskErr FskImageDecompressGetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(deco->decoder->properties, deco->state, deco, propertyID, property);
}

FskErr FskImageDecompressFrame(FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bits, Boolean ownIt, FskImageDecompressComplete completion, void *completionRefcon, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, void *asyncRef, UInt32 frameType)
{
	deco->initializeComplete = true;

	if (compositionTime) {
		if (decodeTime)
			*compositionTime = *decodeTime;
		else
			*compositionTime = -1;
	}

#if SUPPORT_ASYNC_IMAGE_DECOMPRESS
	{
	FskErr err;
	FskImageDecompressAsyncDecompress async;

	if (NULL == completion) {
		return doDecompressFrame(deco, data, dataSize, bits, ownIt, decodeTime, compositionTimeOffset, compositionTime, frameType);
	}

	// create parameter block
	err = FskMemPtrNewClear(sizeof(FskImageDecompressAsyncDecompressRecord), &async);
	if (err) return err;

	async->deco = deco;
	async->completionFunction = completion;
	async->completionRefcon = completionRefcon;
	async->data = data;
	async->dataSize = dataSize;
	async->ownIt = ownIt;
	if (decodeTime) {
		async->decodeTime = *decodeTime;
		async->decodeTimePtr = &async->decodeTime;
	}
	if (compositionTimeOffset) {
		async->compositionTimeOffset = *compositionTimeOffset;
		async->compositionTimeOffsetPtr = &async->compositionTimeOffset;
	}
	async->compositionTime = compositionTime;
	async->frameType = frameType;
	async->ref = asyncRef;

	// add to decompress queue
	if (NULL == gImageDecompressAsyncList)
		FskListMutexNew(&gImageDecompressAsyncList, "gImageDecompressList");

	FskListMutexAppend(gImageDecompressAsyncList, async);

	// if no thread, create thread
	if (NULL == gImageDecompressThread) {
		if (NULL == gImageDecompressSemaphore)
			FskSemaphoreNew(&gImageDecompressSemaphore, 0);

		gImageDecompressThreadDone = false;
//		FskThreadCreate(&gImageDecompressThread, doDecompressThread, kFskThreadFlagsDefault, NULL, "ImageDecompressThread");
		FskThreadCreate(&gImageDecompressThread, doDecompressThread, kFskThreadFlagsWaitForInit, NULL, "ImageDecompressThread");
	}

	FskInstrumentedItemSendMessage(deco, kFskImageDecompressInstrMsgDecompressQueue, (void *)data);

	// get it started
	FskSemaphoreRelease(gImageDecompressSemaphore);

	return kFskErrNone;
	}
#else
	{
	FskErr err;

	err = doDecompressFrame(deco, data, dataSize, bits, ownIt, decodeTime, compositionTimeOffset, compositionTime, frameType);
	if (NULL != completion)
		((FskImageDecompressComplete)completion)(deco, completionRefcon, err, *bits);
	return err;
	}
#endif
}

FskErr doDecompressFrame(FskImageDecompress deco, const void *data, UInt32 dataSize, FskBitmap *bitsOut, Boolean ownIt, FskInt64 *decodeTime, UInt32 *compositionTimeOffset, FskInt64 *compositionTime, UInt32 frameType)
{
	FskErr err;
	FskBitmap bits = NULL;
	Boolean spoolerOpen = false;

	FskInstrumentedItemSendMessage(deco, kFskImageDecompressInstrMsgDecompressFrameBegin, (void *)data);

	deco->frameNumber += 1;

	if (((void *)-1 == data) && (NULL != deco->spooler)) {
		if (NULL == deco->spooler->doRead)
            BAIL(kFskErrOperationFailed);

		if (deco->spooler->doOpen) {
			err = (deco->spooler->doOpen)(deco->spooler, kFskFilePermissionReadOnly);
			BAIL_IF_ERR(err);
		}

		spoolerOpen = true;

		err = (deco->spooler->doRead)(deco->spooler, 0, dataSize, (void **)&data, NULL);
		BAIL_IF_ERR(err);
	}

	err = (deco->decoder->doDecompressFrame)(deco->state, deco, data, dataSize, decodeTime, compositionTimeOffset, compositionTime, frameType);
	BAIL_IF_ERR(err);

	if (ownIt) {
		if (false == deco->privateBits) {
			bits = deco->bits;
			deco->bits = NULL;
		}
		else {
			// clone bit map
			FskBitmapFormatEnum pixelFormat;
			FskRectangleRecord bounds;

			FskBitmapGetPixelFormat(deco->bits, &pixelFormat);
			FskBitmapGetBounds(deco->bits, &bounds);
			err = FskBitmapNew(bounds.width, bounds.height, pixelFormat, &bits);
			BAIL_IF_ERR(err);

			FskInstrumentedItemSetOwner(deco->bits, bits);
			FskBitmapDraw(deco->bits, &bounds, bits, &bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);
		}
	}
	else
		bits = deco->bits;

bail:
	FskInstrumentedItemSendMessage(deco, kFskImageDecompressInstrMsgDecompressFrameComplete, (void *)data);

	if (bitsOut)
		*bitsOut = bits;

	if (spoolerOpen && deco->spooler->doClose)
		(deco->spooler->doClose)(deco->spooler);

	return err;
}

FskErr doSetProperty(FskImageDecompress deco, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err;

	err = FskMediaSetProperty(deco->decoder->properties, deco->state, deco, propertyID, property);
	if ((kFskMediaPropertySpooler == propertyID) && (kFskErrUnimplemented == err)) {
		FskMemPtrDisposeAt((void**)(void*)&deco->spooler);
		if (kFskMediaSpoolerValid & property->value.spooler.flags)
			err = FskMemPtrNewFromData(sizeof(FskMediaSpoolerRecord), &property->value.spooler, (FskMemPtr*)(void*)&deco->spooler);
	}

	return err;
}

#if SUPPORT_ASYNC_IMAGE_DECOMPRESS

FskErr FskImageDecompressFlush(FskImageDecompress deco)
{
	FskImageDecompressAsyncDecompress async;

	if (NULL == gImageDecompressAsyncList) {
		deco->flushing += 1;
		if (deco->decoder->doFlush)
			(deco->decoder->doFlush)(deco->state, deco);
		deco->flushing -= 1;
		return kFskErrNone;
	}

	FskInstrumentedItemSendMessage(deco, kFskImageDecompressInstrMsgDecompressFlush, NULL);

	deco->flushing += 1;		// let the async queue know not to start anything new on this instance

	while (deco->decompressing)	// wait for the frame currently being processed to finish
		FskThreadYield();

	FskMutexAcquire(gImageDecompressAsyncList->mutex);

	for (async = (FskImageDecompressAsyncDecompress)gImageDecompressAsyncList->list; NULL != async; ) {
		FskImageDecompressAsyncDecompress next = (FskImageDecompressAsyncDecompress)async->next;

		if (async->deco == deco) {
			if (async->completionFunction)
				((FskImageDecompressComplete)async->completionFunction)(NULL, async->completionRefcon, kFskErrShutdown, NULL);
			FskListRemove(&gImageDecompressAsyncList->list, async);
			FskMemPtrDispose(async);
		}

		async = next;
	}

	if (deco->decoder->doFlush)
		(deco->decoder->doFlush)(deco->state, deco);

	deco->flushing -= 1;

	FskMutexRelease(gImageDecompressAsyncList->mutex);

	return kFskErrNone;
}

void doDecompressThread(void *refcon)
{
	FskThreadInitializationComplete(FskThreadGetCurrent());

	while (!gImageDecompressThreadDone) {
		FskImageDecompressAsyncDecompress async;
		FskImageDecompress deco = NULL;

		// wait for something to do
		FskSemaphoreAcquire(gImageDecompressSemaphore);
		if (gImageDecompressThreadDone)
			break;

		// find next eligible element
		FskMutexAcquire(gImageDecompressAsyncList->mutex);
			for (async = (FskImageDecompressAsyncDecompress)gImageDecompressAsyncList->list; NULL != async; async = (FskImageDecompressAsyncDecompress)async->next) {
				deco = async->deco;
				if (deco->flushing)
					continue;

				deco->decompressing += 1;
				FskListRemove(&gImageDecompressAsyncList->list, async);
				break;
			}
		FskMutexRelease(gImageDecompressAsyncList->mutex);

		if (!async)
			continue;

		if (kFskMediaPropertyUndefined == async->propertyID) {
			FskBitmap bits = NULL;
			FskErr err;

			if (!deco->flushing) {
				deco->completionFunction = async->completionFunction;
				deco->completionRefcon = async->completionRefcon;

				err = doDecompressFrame(deco, async->data, async->dataSize, &bits, async->ownIt,
					async->decodeTimePtr, async->compositionTimeOffsetPtr, async->compositionTime, async->frameType);

				if (NULL != deco->completionFunction) {
					deco->completionFunction = NULL;
					deco->completionRefcon = NULL;

					if (deco->flushing) {
						// aborted during decompress. requeue so flush code can handle it.
						if (async->ownIt)
							FskBitmapDispose(bits);
						FskMutexAcquire(gImageDecompressAsyncList->mutex);
							FskListPrepend(&gImageDecompressAsyncList->list, async);
						FskMutexRelease(gImageDecompressAsyncList->mutex);
						deco->decompressing -= 1;
						continue;
					}
				}
				else {
					deco->decompressing -= 1;
					FskMemPtrDispose(async);
					continue;
				}
			}
			else
				err = kFskErrOperationCancelled;

			deco->decompressing -= 1;
			if (NULL != async->completionFunction)
				((FskImageDecompressComplete)async->completionFunction)(deco, async->completionRefcon, err, bits);
		}
		else if (!deco->flushing) {
			doSetProperty(deco, async->propertyID, &async->property);
			deco->decompressing -= 1;
		}
		else {
			deco->decompressing -= 1;
		}

		// all done
		FskMemPtrDispose(async);
	}

	gImageDecompressThread = NULL;
	gImageDecompressThreadDone = false;
}
#else

FskErr FskImageDecompressFlush(FskImageDecompress deco)
{
	return kFskErrNone;
}

#endif

FskErr FskImageDecompressSetData(FskImageDecompress deco, void *data, UInt32 dataSize)
{
	FskErr err = kFskErrNone;

	if (NULL != deco->spooled) {
		if (deco->spooler->doClose)
			(deco->spooler->doClose)(deco->spooler);
		deco->spooled = NULL;
	}

	deco->data = data;
	deco->dataSize = dataSize;
	if (deco->decoder->doSetData)
		err = (deco->decoder->doSetData)(deco->state, deco, data, dataSize);

	return err;
}

FskErr FskImageDecompressGetMetaData(FskImageDecompress deco, UInt32 metaData, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskErr err;

	if (NULL == deco->decoder->doGetMetaData)
		return kFskErrUnimplemented;

	if (((void *)-1 == deco->data) && (NULL == deco->spooled) && (NULL != deco->spooler)) {
		if (NULL == deco->spooler->doRead)
			return kFskErrOperationFailed;

		if (deco->spooler->doOpen) {
			err = (deco->spooler->doOpen)(deco->spooler, kFskFilePermissionReadOnly);
			if (err) return err;
		}

		err = (deco->spooler->doRead)(deco->spooler, 0, deco->dataSize, (void **)&deco->spooled, NULL);
		if (err) {
			if (deco->spooler->doClose)
				(deco->spooler->doClose)(deco->spooler);
			return err;
		}

		deco->data = deco->spooled;
		if (deco->decoder->doSetData)
            (deco->decoder->doSetData)(deco->state, deco, deco->data, deco->dataSize);
	}

	return (deco->decoder->doGetMetaData)(deco->state, deco, metaData, index, value, flags);
}

typedef struct {
	FskImageDecompress			deco;

	FskImageDecompressComplete	completion;
	void						*completionRefcon;
} imageOneShotRecord, *imageOneShot;

static void imageOneShotCompletion(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits);

void imageOneShotCompletion(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits)
{
	imageOneShot params = (imageOneShot)refcon;

	(params->completion)(deco, params->completionRefcon, result, bits);

	FskImageDecompressDispose(params->deco);
	FskMemPtrDispose(params);
}

FskErr FskImageDecompressData(const void *data, UInt32 dataSize, const char *mimeType, const char *extension, UInt32 targetWidth, UInt32 targetHeight, FskImageDecompressComplete completion, void *completionRefcon, FskBitmap *bits)
{
	FskErr err;
	FskImageDecompress deco;

	err = FskImageDecompressNew(&deco, 0, mimeType, extension);
	BAIL_IF_ERR(err);

	FskImageDecompressRequestSize(deco, targetWidth, targetHeight);

	if (NULL == completion) {
		err = FskImageDecompressFrame(deco, data, dataSize, bits, true, NULL, NULL, NULL, NULL, NULL, NULL, kFskImageFrameTypeSync);
		BAIL_IF_ERR(err);
	}
	else {
		imageOneShot params;
		FskMemPtrNew(sizeof(imageOneShotRecord), &params);
		params->deco = deco;
		params->completion = completion;
		params->completionRefcon = completionRefcon;
		return FskImageDecompressFrame(deco, data, (UInt32)dataSize, bits, true, imageOneShotCompletion, params, NULL, NULL, NULL, NULL, kFskImageFrameTypeSync);
	}

bail:
	FskImageDecompressDispose(deco);

	return err;
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageImageDecompress(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskImageDecompressInstrMsgDecompressQueue:
			snprintf(buffer, bufferSize, "queue %p", msgData);
			return true;

		case kFskImageDecompressInstrMsgDecompressFrameBegin:
			snprintf(buffer, bufferSize, "begin decompress %p", msgData);
			return true;

		case kFskImageDecompressInstrMsgDecompressFrameComplete:
			snprintf(buffer, bufferSize, "end decompress %p", msgData);
			return true;

		case kFskImageDecompressInstrMsgDecompressFlush:
			snprintf(buffer, bufferSize, "flush");
			return true;
	}

	return false;

}

#endif

/*
	Compress
*/

#if SUPPORT_ASYNC_IMAGE_COMPRESS
	typedef struct FskImageCompressAsyncCompressRecord FskImageCompressAsyncCompressRecord;
	typedef struct FskImageCompressAsyncCompressRecord *FskImageCompressAsyncCompress;

	struct FskImageCompressAsyncCompressRecord {
		FskImageCompressAsyncCompress
									next;

		FskImageCompress			comp;
		FskImageCompressComplete	completionFunction;
		void						*completionRefcon;

		FskBitmap					bits;

		const void					**dataOut;
		UInt32						*dataSizeOut;
		UInt32						*frameDurationOut;
		UInt32						*compositionTimeOffsetOut;
		UInt32						*frameTypeOut;

		UInt32						propertyID;
		FskMediaPropertyValueRecord	property;
	};

	static FskThread		gImageCompressThread;
	static FskSemaphore	gImageCompressSemaphore;
	static FskListMutex	gImageCompressAsyncList;
	static Boolean		gImageCompressThreadDone;

	static void doCompressThread(void *refcon);
#endif

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gImageCompressTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"imagecompress",
		FskInstrumentationOffset(FskImageCompressRecord),
		NULL,
		0,
		NULL,
		NULL
	};
#endif

static FskErr doCompressFrame(FskImageCompress comp, FskBitmap bits, void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType, FskImageCompressComplete completion, void *completionRefcon);

FskErr FskImageCompressNew(FskImageCompress *compOut, UInt32 format, const char *mimeType, UInt32 width, UInt32 height)
{
	FskErr err;
	FskImageCompressor encoder;
	FskImageCompress comp;
	UInt32 i = 0;

	while (true) {
		Boolean canHandle = false;
		encoder = (FskImageCompressor)FskExtensionGetByIndex(kFskExtensionImageCompressor, i++);
		if (NULL == encoder) {
			*compOut = NULL;
			return kFskErrExtensionNotFound;
		}

		if ((kFskErrNone == encoder->doCanHandle(format, mimeType, &canHandle)) && canHandle)
			break;
	}

	err = FskMemPtrNewClear(sizeof(FskImageCompressRecord), &comp);
	BAIL_IF_ERR(err);

	comp->format = format;
	comp->mimeType = FskStrDoCopy(mimeType);
	comp->width = width;
	comp->height = height;
	comp->timeScale = 90000;

	comp->encoder = encoder;

	FskInstrumentedItemNew(comp, comp->mimeType, &gImageCompressTypeInstrumentation);

	err = comp->encoder->doNew(comp);
	BAIL_IF_ERR(err);

bail:
	if (err) {
		FskImageCompressDispose(comp);
		comp = NULL;
	}
	*compOut = comp;

	return err;
}

FskErr FskImageCompressDispose(FskImageCompress comp)
{
	if (comp) {
#if SUPPORT_ASYNC_IMAGE_COMPRESS
		FskImageCompressFlush(comp);
#endif
		comp->encoder->doDispose(comp->state, comp);
		FskMemPtrDispose((void *)comp->mimeType);
		FskMemPtrDispose((void *)comp->esds);
		FskInstrumentedItemDispose(comp);
		FskMemPtrDispose(comp);
	}
	return kFskErrNone;
}

FskErr FskImageCompressSetBitrate(FskImageCompress comp, UInt32 bitrate)
{
	if (comp->frameNumber)
		return kFskErrOutOfSequence;

	comp->bitrate = bitrate;
	return kFskErrNone;
}

FskErr FskImageCompressSetFrameDuration(FskImageCompress comp, UInt32 microseconds)
{
	comp->frameDuration = microseconds;
	return kFskErrNone;
}

FskErr FskImageCompressSetTimeScale(FskImageCompress comp, UInt32 scale)
{
	if (comp->frameNumber)
		return kFskErrOutOfSequence;

	comp->timeScale = scale;
	return kFskErrNone;
}

FskErr FskImageCompressGetElementaryStreamDescriptor(FskImageCompress comp, void **esds, UInt32 *esdsSize)
{
	if (NULL == comp->esds) {
		*esds = NULL;
		*esdsSize = 0;
		return kFskErrNone;
	}

	*esdsSize = comp->esdsSize;
	return FskMemPtrNewFromData(comp->esdsSize, comp->esds, (FskMemPtr *)esds);
}

FskErr FskImageCompressFrame(FskImageCompress comp, FskBitmap bits, void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType, FskImageCompressComplete completion, void *completionRefcon)
{
	FskErr err;

	if (data) *data = NULL;
	if (dataSize) *dataSize = 0;
	if (frameType) *frameType = 0;
	if (compositionTimeOffset) *compositionTimeOffset = 0;

#if SUPPORT_ASYNC_IMAGE_COMPRESS
	if (NULL != completion) {
		FskImageCompressAsyncCompress async;

		err = FskMemPtrNewClear(sizeof(FskImageCompressAsyncCompressRecord), &async);
		BAIL_IF_ERR(err);

		async->comp = comp;
		async->completionFunction = completion;
		async->completionRefcon = completionRefcon;
		async->bits = bits;
		async->dataSizeOut = dataSize;
		async->frameDurationOut = frameDuration;
		async->compositionTimeOffsetOut = compositionTimeOffset;
		async->frameTypeOut = frameType;

		if (NULL == gImageCompressAsyncList)
			FskListMutexNew(&gImageCompressAsyncList, "compressList");

		FskListMutexAppend(gImageCompressAsyncList, async);

		// if no thread, create thread
		if (NULL == gImageCompressThread) {
			if (NULL == gImageCompressSemaphore)
				FskSemaphoreNew(&gImageCompressSemaphore, 0);

			gImageCompressThreadDone = false;
			FskThreadCreate(&gImageCompressThread, doCompressThread, kFskThreadFlagsDefault, NULL, "ImageCompressThread");
		}

		// get it started
		FskSemaphoreRelease(gImageCompressSemaphore);

		return kFskErrNone;
	}
	else {
		FskImageCompressFlush(comp);

		err = doCompressFrame(comp, bits, data, dataSize, frameDuration, compositionTimeOffset, frameType, NULL, NULL);
	}
bail:
#else
		err = doCompressFrame(comp, bits, data, dataSize, frameDuration, compositionTimeOffset, frameType, NULL, NULL);
#endif
	return err;
}

FskErr doCompressFrame(FskImageCompress comp, FskBitmap bits, void **data, UInt32 *dataSize, UInt32 *frameDuration, UInt32 *compositionTimeOffset, UInt32 *frameType, FskImageCompressComplete completion, void *completionRefcon)
{
	comp->frameNumber += 1;

	return (comp->encoder->doCompressFrame)(comp->state, comp, bits, (const void **)data, dataSize, frameDuration, compositionTimeOffset, frameType);
}

FskErr FskImageCompressFlush(FskImageCompress comp)
{
#if !SUPPORT_ASYNC_IMAGE_COMPRESS
	return kFskErrNone;
#else
	FskImageCompressAsyncCompress async;

	if (NULL == gImageCompressAsyncList)
		return kFskErrNone;

	comp->flushing += 1;		// let the async queue know not to start anything new on this instance

	while (comp->compressing)	// wait for the frame currently being processed to finish
		FskThreadYield();

	FskMutexAcquire(gImageCompressAsyncList->mutex);

	for (async = (FskImageCompressAsyncCompress)gImageCompressAsyncList->list; NULL != async; ) {
		FskImageCompressAsyncCompress next = (FskImageCompressAsyncCompress)async->next;

		if (async->comp == comp) {
			if (async->completionFunction)
				((FskImageCompressComplete)async->completionFunction)(comp, async->completionRefcon, kFskErrShutdown, NULL, 0);
			FskListRemove(&gImageCompressAsyncList->list, async);
			FskMemPtrDispose(async);
		}

		async = next;
	}

	comp->flushing -= 1;

	FskMutexRelease(gImageCompressAsyncList->mutex);

	return kFskErrNone;
#endif
}

FskErr FskImageCompressHasProperty(FskImageCompress comp, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(comp->encoder->properties, propertyID, get, set, dataType);
}

FskErr FskImageCompressGetProperty(FskImageCompress comp, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(comp->encoder->properties, comp->state, comp, propertyID, property);
}

FskErr FskImageCompressSetProperty(FskImageCompress comp, UInt32 propertyID, FskMediaPropertyValue property)
{
#if SUPPORT_ASYNC_IMAGE_COMPRESS
	FskErr err;

	if (NULL != gImageCompressAsyncList) {
		FskImageCompressAsyncCompress walker;

		FskMutexAcquire(gImageCompressAsyncList->mutex);

		for (walker = (FskImageCompressAsyncCompress)gImageCompressAsyncList->list; NULL != walker; walker = (FskImageCompressAsyncCompress)walker->next) {
			if (walker->comp == comp)
				break;
		}

		if (NULL != walker) {
			FskImageCompressAsyncCompress async;

			err = FskMemPtrNewClear(sizeof(FskImageCompressAsyncCompressRecord), &async);
			BAIL_IF_ERR(err);

			async->comp = comp;
			async->propertyID = propertyID;
			async->property = *property;

			FskListAppend(&gImageCompressAsyncList->list, async);

			// get it started
			FskSemaphoreRelease(gImageCompressSemaphore);

bail:
			FskMutexRelease(gImageCompressAsyncList->mutex);
			return err;
		}

		FskMutexRelease(gImageCompressAsyncList->mutex);

		// fall through to synchronous setproperty
	}
#endif
	return FskMediaSetProperty(comp->encoder->properties, comp->state, comp, propertyID, property);
}

#if SUPPORT_ASYNC_IMAGE_COMPRESS

void doCompressThread(void *refcon)
{
	while (!gImageCompressThreadDone) {
		FskImageCompressAsyncCompress async;
		FskImageCompress comp = NULL;

		// wait for something to do
		FskSemaphoreAcquire(gImageCompressSemaphore);
		if (gImageCompressThreadDone)
			break;

		// find next eligible element
		FskMutexAcquire(gImageCompressAsyncList->mutex);
			for (async = (FskImageCompressAsyncCompress)gImageCompressAsyncList->list; NULL != async; async = (FskImageCompressAsyncCompress)async->next) {
				comp = async->comp;
				if (comp->flushing)
					continue;

				comp->compressing += 1;
				FskListRemove(&gImageCompressAsyncList->list, async);
				break;
			}
		FskMutexRelease(gImageCompressAsyncList->mutex);

		if (!async)
			continue;

		if (kFskMediaPropertyUndefined == async->propertyID) {
			FskErr err;
			void *data = NULL;
			UInt32 dataSize = 0;

			if (!comp->flushing) {
				err = doCompressFrame(async->comp, async->bits, &data, &dataSize,
							async->frameDurationOut, async->compositionTimeOffsetOut, async->frameTypeOut, NULL, NULL);
				if (kFskErrNone == err) {
					if (async->dataOut) *(async->dataOut) = data;
					if (async->dataSizeOut) *(async->dataSizeOut) = dataSize;
				}
			}
			else
				err = kFskErrOperationCancelled;

			comp->compressing -= 1;
			if (NULL != async->completionFunction)
				(async->completionFunction)(comp, async->completionRefcon, err, data, dataSize);
		}
		else if (!comp->flushing) {
			comp->compressing -= 1;
			FskMediaSetProperty(comp->encoder->properties, comp->state, comp, async->propertyID, &async->property);
		}
		else {
			comp->compressing -= 1;
		}

		// all done
		FskMemPtrDispose(async);
	}

	gImageCompressThread = NULL;
	gImageCompressThreadDone = false;
}

#endif
