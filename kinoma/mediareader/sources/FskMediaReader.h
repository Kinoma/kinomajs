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
#ifndef __FSKMEDIAREADER__
#define __FSKMEDIAREADER__

#include "FskEvent.h"
#include "FskHeaders.h"
#include "FskImage.h"
#include "FskMedia.h"
#include "FskMediaPlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FskMediaReaderInstall(a) (FskExtensionInstall(kFskExtensionMediaReader, a))
#define FskMediaReaderUninstall(a) (FskExtensionUninstall(kFskExtensionMediaReader, a))

typedef struct FskMediaReaderRecord FskMediaReaderRecord;
typedef FskMediaReaderRecord *FskMediaReader;

typedef struct FskMediaReaderTrackRecord FskMediaReaderTrackRecord;
typedef FskMediaReaderTrackRecord *FskMediaReaderTrack;

typedef struct {
	UInt32		samples;
	UInt32		sampleSize;
	UInt32		flags;						// kFskImageFrameType enum
	FskInt64	decodeTime;
	UInt32		sampleDuration;				// 0 == unknown
	FskInt64	compositionTime;			// -1 == unknown (maybe should define a flag that says this is valid)
} FskMediaReaderSampleInfoRecord, *FskMediaReaderSampleInfo;

typedef FskErr (*FskMediaReaderEvent)(FskMediaReader reader, void *refCon, FskEventCodeEnum eventCode, FskEvent event);

#if defined(__FSKMEDIAREADER_PRIV__) || SUPPORT_INSTRUMENTATION
	typedef struct {
		Boolean		(*doCanHandle)(const char *mimeType);

		FskErr		(*doNew)(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
		FskErr		(*doDispose)(FskMediaReader reader, void *readerState);

		FskErr		(*doGetTrack)(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);

		FskErr		(*doStart)(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
		FskErr		(*doStop)(FskMediaReader reader, void *readerState);

		FskErr		(*doExtract)(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);

		FskErr		(*doGetMetadata)(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

		FskMediaPropertyEntry	properties;

		FskErr		(*doSniff)(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);
	} FskMediaReaderDispatchRecord, *FskMediaReaderDispatch;

	typedef struct {
		FskMediaPropertyEntry	properties;
	} FskMediaReaderTrackDispatchRecord, *FskMediaReaderTrackDispatch;

	struct FskMediaReaderRecord {
		FskMediaReaderDispatch			dispatch;
		void							*state;

		FskMediaSpooler					spooler;

		FskMediaReaderEvent				eventHandler;
		void							*eventHandlerRefCon;

		SInt32							useCount;
		SInt32							mediaState;
		Boolean							instantiating;
		Boolean							needsIdle;

		FskErr (*doSetState)(FskMediaReader reader, SInt32 state);

		FskErr (*doUsing)(void *refCon, Boolean inUse);
		void							*doUsingRefCon;

		FskInstrumentedItemDeclaration
	};

	struct FskMediaReaderTrackRecord {
		FskMediaReaderTrackDispatch		dispatch;
		void							*state;
	};

#endif

FskExport(FskErr) FskMediaReaderNew(FskMediaReader *reader, const char *mimeType, const char *uri, FskMediaSpooler source, FskMediaReaderEvent eventHandler, void *refCon);
FskExport(FskErr) FskMediaReaderDispose(FskMediaReader reader);

FskExport(FskErr) FskMediaReaderHasProperty(FskMediaReader reader, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskExport(FskErr) FskMediaReaderSetProperty(FskMediaReader reader, UInt32 propertyID, FskMediaPropertyValue property);
FskExport(FskErr) FskMediaReaderGetProperty(FskMediaReader reader, UInt32 propertyID, FskMediaPropertyValue property);

FskExport(FskErr) FskMediaReaderGetTrack(FskMediaReader reader, SInt32 index, FskMediaReaderTrack *track);
FskExport(FskErr) FskMediaReaderGetMetadata(FskMediaReader reader, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

FskExport(FskErr) FskMediaReaderStart(FskMediaReader reader, double *startTime, double *endTime);
FskExport(FskErr) FskMediaReaderStop(FskMediaReader reader);

FskExport(FskErr) FskMediaReaderExtract(FskMediaReader reader, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);

FskExport(FskErr) FskMediaReaderUsing(FskMediaReader reader, Boolean inUse);

#ifdef __FSKMEDIAREADER_PRIV__
	FskExport(FskErr) FskMediaReaderSendEvent(FskMediaReader reader, FskEventCodeEnum eventCode);		// only for use by extensions
#endif

FskExport(FskErr) FskMediaReaderTrackHasProperty(FskMediaReaderTrack track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskExport(FskErr) FskMediaReaderTrackSetProperty(FskMediaReaderTrack track, UInt32 propertyID, FskMediaPropertyValue property);
FskExport(FskErr) FskMediaReaderTrackGetProperty(FskMediaReaderTrack track, UInt32 propertyID, FskMediaPropertyValue property);

FskExport(FskErr) FskMediaReaderSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

#ifdef __cplusplus
}
#endif

#endif
