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
#ifndef __FSKMUXER__
#define __FSKMUXER__

#include "FskMedia.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskMuxerRecord FskMuxerRecord;
typedef FskMuxerRecord *FskMuxer;

typedef struct FskMuxerTrackRecord FskMuxerTrackRecord;
typedef FskMuxerTrackRecord *FskMuxerTrack;

typedef struct {
	UInt32		sampleCount;
	UInt32		sampleSize;
	UInt32		sampleDuration;
	UInt32		compositionOffset;
	UInt32		flags;					// 1 bit: 0 = i frame, 1 = diff
} FskMuxerSampleInfoRecord, *FskMuxerSampleInfo;

typedef FskErr (*FskMuxerWriteProc)(FskMuxer muxer, void *refCon, const void *data, FskFileOffset offset, UInt32 dataSize);

typedef struct {
	FskErr		(*doDispose)(FskMuxerTrack track, void *trackState);
	FskErr		(*doAdd)(FskMuxerTrack track, void *trackState, const void *data, UInt32 infoCount, FskMuxerSampleInfo info);

	FskMediaPropertyEntry	properties;
} FskMuxerTrackDispatchRecord, *FskMuxerTrackDispatch;

typedef struct {
	Boolean		(*doCanHandle)(const char *mimeType);

	FskErr		(*doNew)(FskMuxer muxer, void **muxerState);
	FskErr		(*doDispose)(FskMuxer muxer, void *muxerState);

	FskErr		(*doStart)(FskMuxer muxer, void *muxerState);
	FskErr		(*doStop)(FskMuxer muxer, void *muxerState);

	FskErr		(*doSetMetaData)(FskMuxer muxer, void *muxerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 flags);

	FskErr		(*doNewTrack)(FskMuxer muxer, void *muxerState, FskMuxerTrack track, FskMuxerTrackDispatch *dispatch, void **trackState);

	FskMediaPropertyEntry	properties;
} FskMuxerDispatchRecord, *FskMuxerDispatch;

#ifdef __FSKMUXER_PRIV__

	struct FskMuxerRecord {
		FskMuxerTrack			tracks;

		char					*mimeType;
		UInt32					scale;

		FskMuxerWriteProc		write;
		void					*writeRefCon;

		FskMuxerDispatch		dispatch;
		void					*state;
	};

	struct FskMuxerTrackRecord {
		FskMuxerTrack			next;

		FskMuxer				muxer;
		char					*trackType;
		UInt32					scale;

		FskMuxerTrackDispatch	dispatch;
		void					*state;
	};

#endif

// instance management
FskAPI(FskErr) FskMuxerNew(FskMuxer *muxer, const char *mimeType, UInt32 scale, FskMuxerWriteProc write, void *writeRefCon);
FskAPI(FskErr) FskMuxerDispose(FskMuxer muxer);

// configure
FskAPI(FskErr) FskMuxerHasProperty(FskMuxer muxer, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskMuxerSetProperty(FskMuxer muxer, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskMuxerGetProperty(FskMuxer muxer, UInt32 propertyID, FskMediaPropertyValue property);

FskAPI(FskErr) FskMuxerSetMetaData(FskMuxer muxer, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 flags);

// go, go, go
FskAPI(FskErr) FskMuxerStart(FskMuxer muxer);
FskAPI(FskErr) FskMuxerStop(FskMuxer muxer);

// track
FskAPI(FskErr) FskMuxerTrackNew(FskMuxer muxer, FskMuxerTrack *muxerTrack, const char *trackType, UInt32 scale);
FskAPI(FskErr) FskMuxerTrackDispose(FskMuxerTrack muxerTrack);

// configure
FskAPI(FskErr) FskMuxerTrackHasProperty(FskMuxerTrack muxerTrack, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskMuxerTrackSetProperty(FskMuxerTrack muxerTrack, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskMuxerTrackGetProperty(FskMuxerTrack muxerTrack, UInt32 propertyID, FskMediaPropertyValue property);

// adding samples
FskAPI(FskErr) FskMuxerTrackAdd(FskMuxerTrack muxerTrack, const void *data, UInt32 infoCount, FskMuxerSampleInfo info);

#ifdef __cplusplus
}
#endif

#endif
