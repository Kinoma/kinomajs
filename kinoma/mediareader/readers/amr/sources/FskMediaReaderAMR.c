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
#define __FSKMEDIAREADER_PRIV__
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskMediaReader.h"

static Boolean amrReaderCanHandle(const char *mimeType);
static FskErr amrReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr amrReaderDispose(FskMediaReader reader, void *readerState);
static FskErr amrReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr amrReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr amrReaderStop(FskMediaReader reader, void *readerState);
static FskErr amrReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr amrReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr amrReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderSetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord amrReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			amrReaderGetDuration,		amrReaderSetDuration},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			amrReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		amrReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		amrReaderGetState,			NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	amrReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderAMR = {amrReaderCanHandle, amrReaderNew, amrReaderDispose, amrReaderGetTrack, amrReaderStart, amrReaderStop, amrReaderExtract, NULL, amrReaderProperties, amrReaderSniff};

static FskErr amrReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr amrReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr amrReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord amrReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		amrReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		amrReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		amrReaderTrackGetSampleRate,		NULL},
//	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		amrReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		amrReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gAMRReaderAudioTrack = {amrReaderAudioTrackProperties};

Boolean amrReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("audio/AMR", mimeType))
		return true;

	return false;
}

typedef struct amrReaderRecord amrReaderRecord;
typedef struct amrReaderRecord *amrReader;

typedef struct amrReaderTrackRecord amrReaderTrackRecord;
typedef struct amrReaderTrackRecord *amrReaderTrack;

struct amrReaderTrackRecord {
	FskMediaReaderTrackRecord			readerTrack;
	amrReader							state;

	char								*format;

	UInt32								bitRate;
};

 struct amrReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	UInt32					duration;

	FskMediaReader			reader;

	FskInt64				fileOffset;

	amrReaderTrackRecord	audio;
};

static FskErr doRead(amrReader state, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr amrSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static FskErr amrInstantiate(amrReader state);

static unsigned char gBlockSizes[16]= { 13, 14, 16, 18, 20, 21, 27, 32, 6, 1, 1, 1, 1, 1, 1, 1 };

 FskErr amrReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	amrReader state = NULL;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(amrReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	state->spooler->onSpoolerCallback = amrSpoolerCallback;
	state->spooler->clientRefCon = state;
	state->spooler->flags |= kFskMediaSpoolerForwardOnly;

	state->duration = kFskUInt32Max;

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = amrInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		amrReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr amrReaderDispose(FskMediaReader reader, void *readerState)
{
	amrReader state = readerState;

	if (NULL != state) {

		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr amrReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	amrReader state = readerState;

	if (0 != index)
		return kFskErrNotFound;

	*track = &state->audio.readerTrack;

	return kFskErrNone;
}

FskErr amrReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	amrReader state = readerState;
	FskErr err = kFskErrNone;
	UInt32 sampleTime;

	state->atTime = startTime ? (UInt32)*startTime : 0;

	if (kFskUInt32Max != state->duration) {
		sampleTime = state->atTime;
		if (sampleTime > state->duration)
			sampleTime = state->duration;

		if (endTime) {
			if (*endTime > state->duration)
				state->endTime = (UInt32)state->duration;
			else
				state->endTime = (UInt32)*endTime;
			state->hasEndTime = true;
		}
		else
			state->hasEndTime = false;

		state->fileOffset = 6;
		state->atTime = 0;

		while (sampleTime > 160) {
			char *data, *p, *end;
			UInt32 bytes = 1024;

			err = FskMediaSpoolerRead(state->spooler, state->fileOffset, bytes, &p, &bytes);
			if (err) break;

			data = p;
			end = data + bytes;

			while ((p < end) && (sampleTime > 160)) {
				int mode = (p[0] >> 3) & 0x0f;
				int frameSize = (int)gBlockSizes[mode];

				p += frameSize;
				sampleTime -= 160;
				state->atTime += 160;
			}

			state->fileOffset += (p - data);
		}

		if ((kFskErrNone == err) || (kFskErrEndOfFile == err))
			err = kFskErrNone;
	}

	return err;
}

FskErr amrReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr amrReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	amrReader state = readerState;
	FskErr err = kFskErrNone;
	unsigned char header, *data;
	FskMediaReaderSampleInfo info;
	int mode, frameSize = 0;
	UInt32 frameCount = 0;

	do {
		err = doRead(state, state->fileOffset + (frameSize * frameCount), 1, &header);
		if (err) {
			if (0 != frameCount) break;
			goto bail;
		}

		mode = (header >> 3) & 0x0f;
		if (0 == frameSize)
			frameSize = (int)gBlockSizes[mode];
		else if (frameSize != (int)gBlockSizes[mode])
			break;

		frameCount += 1;
	} while (frameCount < 10);

	err = FskMemPtrNew(frameSize * frameCount, &data);
	BAIL_IF_ERR(err);

	err = doRead(state, state->fileOffset, frameSize * frameCount, data);
	if (kFskErrNone != err) {
		FskMemPtrDispose(data);
		goto bail;
	}

	*infoCountOut = 1;
	err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
	BAIL_IF_ERR(err);

	info->samples = frameCount;
	info->sampleSize = frameSize;
	info->flags = kFskImageFrameTypeSync;
	info->decodeTime = state->atTime;
	info->sampleDuration = 160;
	info->compositionTime = -1;

	*infoOut = info;
	*trackOut = &state->audio.readerTrack;
	*dataOut = data;

	state->atTime += info->samples * info->sampleDuration;
	state->fileOffset += frameSize * frameCount;

bail:
	return err;
}

FskErr amrReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if ((dataSize > 6) && (0 == FskStrCompareWithLength((const char *)data, "#!AMR", 5)) && (0x0a == data[5])) {
		*mime = FskStrDoCopy("audio/AMR");
		return kFskErrNone;
	}

	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr amrReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	amrReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (kFskUInt32Max != state->duration) ? state->duration : -1.0;

	return kFskErrNone;
}

FskErr amrReaderSetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	amrReader state = readerState;

	state->duration = (UInt32)property->value.number;

	return kFskErrNone;
}

FskErr amrReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	amrReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr amrReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 8000;

	return kFskErrNone;
}

FskErr amrReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	amrReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr amrReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "audio/AMR\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr amrReaderTrackGetMediaType(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("audio");

	return kFskErrNone;
}

FskErr amrReaderTrackGetFormat(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("x-audio-codec/amr-nb");

	return kFskErrNone;
}

FskErr amrReaderTrackGetSampleRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 8000;

	return kFskErrNone;
}

/*
FskErr amrReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	amrReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 16 * track->audio.channelCount * track->audio.sampleRate;

	return kFskErrNone;
}
*/

FskErr amrReaderTrackGetChannelCount(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 1;

	return kFskErrNone;
}

/*
	local
*/

FskErr doRead(amrReader state, FskInt64 offset, UInt32 size, void *bufferIn)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;

	while (0 != size) {
		unsigned char *readBuffer;
		UInt32 bytesRead;

		err = FskMediaSpoolerRead(state->spooler, offset, size, &readBuffer, &bytesRead);
		if (err) return err;

		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

	return err;
}

FskErr amrSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	amrReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = amrInstantiate(state);
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

FskErr amrInstantiate(amrReader state)
{
	FskErr err = kFskErrNone;
	FskInt64 totalSize;
	char header[6];

	if (NULL != state->spooler->doGetSize) {
		err = (state->spooler->doGetSize)(state->spooler, &totalSize);
		BAIL_IF_ERR(err);
	}
	else
		totalSize = 0;

	err = doRead(state, 0, 6, header);
	BAIL_IF_ERR(err);
	
	if ((0 != FskStrCompareWithLength(header, "#!AMR", 5)) || (0x0a != header[5]))
        BAIL(kFskErrBadData);

	if (0 == totalSize)
		state->duration = kFskUInt32Max;
	else {
		FskInt64 offset = 6;
		UInt32 duration = 0;

		while (true) {
			char *data, *p, *end;
			UInt32 bytes = 1024;

			err = FskMediaSpoolerRead(state->spooler, offset, bytes, &p, &bytes);
			if (err) break;

			data = p;
			end = data + bytes;

			while (p < end) {
				int mode = (p[0] >> 3) & 0x0f;
				int frameSize = (int)gBlockSizes[mode];

				p += frameSize;
				duration += 160;
			}

			offset += (p - data);
		}

		if ((kFskErrNone == err) || (kFskErrEndOfFile == err)) {
			state->duration = duration;
			err = kFskErrNone;
		}
		else
		if (kFskErrNeedMoreTime == err) {
			state->duration = kFskUInt32Max;
			err = kFskErrNone;
		}
		else
			goto bail;
	}

	state->audio.readerTrack.dispatch = &gAMRReaderAudioTrack;
	state->audio.readerTrack.state = &state->audio.readerTrack;
	state->audio.state = state;

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}
