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
#define __FSKMEDIAREADER_PRIV__
#include "FskMediaReader.h"

#include "FskExtensions.h"
#include "FskUtilities.h"

static FskErr mediaReaderSetState(FskMediaReader reader, SInt32 state);

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gMediaReaderTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"mediareader",
		FskInstrumentationOffset(FskMediaPlayerRecord),
		NULL,
		0,
		NULL,
		NULL
	};
#endif

FskErr FskMediaReaderNew(FskMediaReader *readerOut, const char *mimeType, const char *uri, FskMediaSpooler spooler, FskMediaReaderEvent eventHandler, void *refCon)
{
	FskErr err = kFskErrNone;
	UInt32 i = 0;
	FskMediaReaderDispatch dispatch = NULL;
	FskMediaReader reader = NULL;

	while (true) {
		FskMediaReaderDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionMediaReader, i++);
		if (NULL == aDispatch)
			break;

		if (kFskErrNone != (aDispatch->doCanHandle)(mimeType)) {
			dispatch = aDispatch;
			break;
		}
	}

	if (NULL == dispatch)
        BAIL(kFskErrExtensionNotFound);

	if (NULL == readerOut)
		goto bail;					// canHandle case

	err = FskMemPtrNewClear(sizeof(FskMediaReaderRecord), &reader);
	BAIL_IF_ERR(err);

	reader->dispatch = dispatch;
	*readerOut = reader;			// must occur before doNew, so reader instance is available to eventHandler

	FskInstrumentedItemNew(reader, uri, &gMediaReaderTypeInstrumentation);

	reader->spooler = spooler;
	reader->eventHandler = eventHandler;
	reader->eventHandlerRefCon = refCon;
	reader->useCount = 1;
	reader->mediaState = kFskMediaPlayerStateInitializing;
	reader->doSetState = mediaReaderSetState;

	err = (reader->dispatch->doNew)(reader, &reader->state, mimeType, uri, reader->spooler);
	BAIL_IF_ERR(err);

bail:
	if (kFskErrNone != err) {
		FskMediaReaderDispose(reader);
		reader = NULL;
	}
	if (readerOut)
		*readerOut = reader;

	return err;
}

FskErr FskMediaReaderDispose(FskMediaReader reader)
{
	if (NULL == reader)
		return kFskErrNone;

	reader->useCount -= 1;
	if (0 != reader->useCount)
		return kFskErrNone;

	(reader->dispatch->doDispose)(reader, reader->state);

	FskInstrumentedItemDispose(reader);

	FskMemPtrDispose(reader);

	return kFskErrNone;
}

FskErr FskMediaReaderHasProperty(FskMediaReader reader, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	FskInstrumentedItemPrintfVerbose(reader, "hasProperty %u", (unsigned int)propertyID);
	return FskMediaHasProperty(reader->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskMediaReaderSetProperty(FskMediaReader reader, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskInstrumentedItemPrintfVerbose(reader, "setProperty %u", (unsigned int)propertyID);
	return FskMediaSetProperty(reader->dispatch->properties, reader->state, reader, propertyID, property);
}

FskErr FskMediaReaderGetProperty(FskMediaReader reader, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskInstrumentedItemPrintfVerbose(reader, "getProperty %u", (unsigned int)propertyID);
	return FskMediaGetProperty(reader->dispatch->properties, reader->state, reader, propertyID, property);
}

FskErr FskMediaReaderGetTrack(FskMediaReader reader, SInt32 index, FskMediaReaderTrack *track)
{
	FskInstrumentedItemPrintfVerbose(reader, "getTrack %d", (int)index);
	if (NULL == reader->dispatch->doGetTrack)
		return kFskErrUnimplemented;
	return (reader->dispatch->doGetTrack)(reader, reader->state, index, track);
}

FskErr FskMediaReaderGetMetadata(FskMediaReader reader, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskInstrumentedItemPrintfVerbose(reader, "getMetdata %s", metaDataType);
	if (NULL == reader->dispatch->doGetMetadata)
		return kFskErrUnimplemented;
	return (reader->dispatch->doGetMetadata)(reader, reader->state, metaDataType, index, value, flags);
}

FskErr FskMediaReaderStart(FskMediaReader reader, double *startTime, double *endTime)
{
	FskInstrumentedItemPrintfVerbose(reader, "start");
	if (NULL == reader->dispatch->doStart)
		return kFskErrUnimplemented;
	return (reader->dispatch->doStart)(reader, reader->state, startTime, endTime);
}

FskErr FskMediaReaderStop(FskMediaReader reader)
{
	FskInstrumentedItemPrintfVerbose(reader, "stop");
	if (NULL == reader->dispatch->doStop)
		return kFskErrUnimplemented;
	return (reader->dispatch->doStop)(reader, reader->state);
}

FskErr FskMediaReaderExtract(FskMediaReader reader, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data)
{
	if (NULL == reader->dispatch->doExtract)
		return kFskErrUnimplemented;
#if SUPPORT_INSTRUMENTATION
    {
    FskErr err = (reader->dispatch->doExtract)(reader, reader->state, track, infoCount, info, data);
    if (err) {
        FskInstrumentedItemPrintfVerbose(reader, "extract - error %s (%d)", FskInstrumentationGetErrorString(err), (int)err);
    }
    else {
        UInt32 i, count, dataSize = 0, duration = 0;
        FskInt64 lastTime = 0;
        FskMediaReaderSampleInfo inf;
        for (i = 0, count = *infoCount, inf = *info; i < count; i++, inf++) {
            dataSize += inf->samples + inf->sampleSize;
            duration = inf->samples * inf->sampleDuration;
            lastTime = inf->decodeTime;
        }
        FskInstrumentedItemPrintfVerbose(reader, "extract - to time %d (%d duration), %d bytes", (int)(duration + lastTime), (int)duration, (int)dataSize);
    }
    return err;
    }
#else
	return (reader->dispatch->doExtract)(reader, reader->state, track, infoCount, info, data);
#endif
}

FskErr FskMediaReaderUsing(FskMediaReader reader, Boolean inUse)
{
	if (inUse) {
		reader->useCount += 1;
		if (reader->doUsing)
			(reader->doUsing)(reader->doUsingRefCon, inUse);
	}
	else {
		Boolean alive = reader->useCount > 1;

		FskMediaReaderDispose(reader);
		if (alive && reader->doUsing)
			(reader->doUsing)(reader->doUsingRefCon, inUse);
	}

	return kFskErrNone;
}

FskErr FskMediaReaderSendEvent(FskMediaReader reader, FskEventCodeEnum eventCode)
{
	FskErr err = kFskErrNone;
	FskEvent event;

	FskInstrumentedItemPrintfVerbose(reader, "sendEvent %d", eventCode);

	if (NULL == reader->eventHandler)
		goto bail;

	err = FskEventNew(&event, eventCode, NULL, 0);
	BAIL_IF_ERR(err);

	FskMediaReaderUsing(reader, true);

	(reader->eventHandler)(reader, reader->eventHandlerRefCon, eventCode, event);
	FskEventDispose(event);

	FskMediaReaderUsing(reader, false);

bail:
	return err;
}

FskErr FskMediaReaderTrackHasProperty(FskMediaReaderTrack track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(track->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskMediaReaderTrackSetProperty(FskMediaReaderTrack track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(track->dispatch->properties, track->state, track, propertyID, property);
}

FskErr FskMediaReaderTrackGetProperty(FskMediaReaderTrack track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(track->dispatch->properties, track->state, track, propertyID, property);
}

FskErr FskMediaReaderSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	UInt32 i = 0;

	FskInstrumentedTypePrintfVerbose(&gMediaReaderTypeInstrumentation, "sniff %u bytes, headers=%p, uri=%s", (unsigned int)dataSize, headers, uri);
	while (true) {
		FskMediaReaderDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionMediaReader, i++);
		if (NULL == aDispatch)
			break;

		if (NULL == aDispatch->doSniff)
			continue;

		if (kFskErrNone == (aDispatch->doSniff)(data, dataSize, headers, uri, mime))
			return kFskErrNone;
	}

	return kFskErrExtensionNotFound;
}

/*
	local
*/

FskErr mediaReaderSetState(FskMediaReader reader, SInt32 state)
{
	if (state != reader->mediaState) {
		SInt32 previousState = reader->mediaState;

        FskInstrumentedTypePrintfVerbose(&gMediaReaderTypeInstrumentation, "set state %d", (int)state);

		FskMediaReaderUsing(reader, true);

		reader->mediaState = state;
		if ((previousState < kFskMediaPlayerStateStopped) && (state >= kFskMediaPlayerStateStopped))
			FskMediaReaderSendEvent(reader, kFskEventMediaPlayerInitialize);

		FskMediaReaderSendEvent(reader, kFskEventMediaPlayerStateChange);

		FskMediaReaderUsing(reader, false);
	}

	return kFskErrNone;
}

