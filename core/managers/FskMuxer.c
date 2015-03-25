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
#define __FSKMUXER_PRIV__
#include "FskMuxer.h"

#include "FskExtensions.h"
#include "FskUtilities.h"
#include "FskList.h"

FskErr FskMuxerNew(FskMuxer *muxerOut, const char *mimeType, UInt32 scale, FskMuxerWriteProc write, void *writeRefCon)
{
	FskErr err;
	FskMuxer muxer = NULL;
	UInt32 i = 0;
	FskMuxerDispatch dispatch;

	while (true) {
		dispatch = (FskMuxerDispatch)FskExtensionGetByIndex(kFskExtensionMuxer, i++);
		if (NULL == dispatch)
			return kFskErrExtensionNotFound;

		if (dispatch->doCanHandle(mimeType))
			break;
	}

	err = FskMemPtrNewClear(sizeof(FskMuxerRecord) + FskStrLen(mimeType) + 1, &muxer);
	BAIL_IF_ERR(err);

	muxer->mimeType = (char *)(muxer + 1);
	FskStrCopy(muxer->mimeType, mimeType);
	muxer->scale = scale;
	muxer->write = write;
	muxer->writeRefCon = writeRefCon;
	muxer->dispatch = dispatch;

	err = (muxer->dispatch->doNew)(muxer, &muxer->state);
	BAIL_IF_ERR(err);

bail:
	if (kFskErrNone != err) {
		FskMuxerDispose(muxer);
		muxer = NULL;
	}
	*muxerOut = muxer;

	return err;
}

FskErr FskMuxerDispose(FskMuxer muxer)
{
	if (NULL == muxer)
		goto bail;

	while (muxer->tracks)
		FskMuxerTrackDispose(muxer->tracks);

	if (muxer->dispatch->doDispose)
		(muxer->dispatch->doDispose)(muxer, muxer->state);

	FskMemPtrDispose(muxer);

bail:
	return kFskErrNone;
}

FskErr FskMuxerHasProperty(FskMuxer muxer, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(muxer->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskMuxerSetProperty(FskMuxer muxer, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(muxer->dispatch->properties, muxer->state, muxer, propertyID, property);
}

FskErr FskMuxerGetProperty(FskMuxer muxer, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(muxer->dispatch->properties, muxer->state, muxer, propertyID, property);
}

FskErr FskMuxerStart(FskMuxer muxer)
{
	return muxer->dispatch->doStart ? (muxer->dispatch->doStart)(muxer, muxer->state) : kFskErrNone;
}

FskErr FskMuxerStop(FskMuxer muxer)
{
	return muxer->dispatch->doStop ? (muxer->dispatch->doStop)(muxer, muxer->state) : kFskErrNone;
}

FskErr FskMuxerSetMetaData(FskMuxer muxer, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 flags)
{
	return muxer->dispatch->doSetMetaData ? (muxer->dispatch->doSetMetaData)(muxer, muxer->state, metaDataType, index, value, flags) : kFskErrNone;
}

FskErr FskMuxerTrackNew(FskMuxer muxer, FskMuxerTrack *muxerTrackOut, const char *trackType, UInt32 scale)
{
	FskErr err;
	FskMuxerTrack track = NULL;

	err = FskMemPtrNewClear(sizeof(FskMuxerTrackRecord) + FskStrLen(trackType) + 1, &track);
	BAIL_IF_ERR(err);

	track->trackType = (char *)(track + 1);
	FskStrCopy(track->trackType, trackType);
	track->scale = scale;
	track->muxer = muxer;

	err = (muxer->dispatch->doNewTrack)(muxer, muxer->state, track, &track->dispatch, &track->state);
	BAIL_IF_ERR(err);

	FskListAppend((FskList*)(void*)&muxer->tracks, track);

bail:
	if (kFskErrNone != err) {
		FskMuxerTrackDispose(track);
		track = NULL;
	}

	*muxerTrackOut = track;

	return err;
}

FskErr FskMuxerTrackDispose(FskMuxerTrack track)
{
	if (NULL == track)
		goto bail;

	if (track->dispatch && track->dispatch->doDispose)
		(track->dispatch->doDispose)(track, track->state);

	FskListRemove((FskList*)(void*)&track->muxer->tracks, track);

	FskMemPtrDispose(track);

bail:
	return kFskErrNone;
}

FskErr FskMuxerTrackHasProperty(FskMuxerTrack track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(track->dispatch->properties, propertyID, get, set, dataType);
}

FskErr FskMuxerTrackSetProperty(FskMuxerTrack track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(track->dispatch->properties, track->state, track, propertyID, property);
}

FskErr FskMuxerTrackGetProperty(FskMuxerTrack track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(track->dispatch->properties, track->state, track, propertyID, property);
}

FskErr FskMuxerTrackAdd(FskMuxerTrack track, const void *data, UInt32 infoCount, FskMuxerSampleInfo info)
{
	return (track->dispatch->doAdd)(track, track->state, data, infoCount, info);
}
