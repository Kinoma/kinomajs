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
#include "FskFiles.h"
#include "FskMediaReader.h"
#include "FskEndian.h"

static Boolean waveReaderCanHandle(const char *mimeType);
static FskErr waveReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr waveReaderDispose(FskMediaReader reader, void *readerState);
static FskErr waveReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr waveReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr waveReaderStop(FskMediaReader reader, void *readerState);
static FskErr waveReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr waveReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr waveReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr waveReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderGetSeekableSegment(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord waveReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			waveReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			waveReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		waveReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		waveReaderGetState,			NULL},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			waveReaderGetSeekableSegment, NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	waveReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderWave = {waveReaderCanHandle, waveReaderNew, waveReaderDispose, waveReaderGetTrack, waveReaderStart, waveReaderStop, waveReaderExtract, waveReaderGetMetadata, waveReaderProperties, waveReaderSniff};

static FskErr waveReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr waveReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord waveReaderTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		waveReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		waveReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		waveReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		waveReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		waveReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gMediaReaderWaveTrack = {waveReaderTrackProperties};

Boolean waveReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("audio/wav", mimeType))
		return true;
	if (0 == FskStrCompareCaseInsensitive("audio/x-wav", mimeType))
		return true;

	return false;
}

typedef struct {
	UInt16 audioFormat;
	UInt16 numChannels;
	UInt32 sampleRate;
	UInt16 bitsPerSample;
	UInt32 dataSize;
} waveInfoRecord, *waveInfo;

typedef struct {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	FskInt64				position;

	unsigned char			*readBufferPtr;
	unsigned char			*readBufferEnd;

	FskMediaReader			reader;
	FskMediaReaderTrackRecord
							track;

	UInt32					channelCount;
	UInt32					sampleRate;
	double					duration;		// in seconds
	UInt32					bytesPerSample;
	UInt32					dataOffset;

	UInt32					readBufferSize;
	unsigned char			*readBuffer;

	FskInt64				spoolerPosition;
} waveReaderRecord, *waveReader;

static FskErr waveInstantiate(waveReader state);
static FskErr waveRefillReadBuffer(waveReader state, UInt32 minimumBytesNeeded);
static FskErr waveSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);

FskErr waveReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	waveReader state = NULL;

	if ((NULL == spooler) || (NULL == spooler->doGetSize)) {
		err = kFskErrUnimplemented;
		goto bail;
	}
	
	err = FskMemPtrNewClear(sizeof(waveReaderRecord), &state);
	if (err) goto bail;

	state->spooler = spooler;
	state->reader = reader;

	*readerState = state;

	state->spooler->onSpoolerCallback = waveSpoolerCallback;
	state->spooler->clientRefCon = state;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		if (err) goto bail;

		state->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = waveInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		waveReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr waveReaderDispose(FskMediaReader reader, void *readerState)
{
	waveReader state = readerState;

	if (NULL != state) {
		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);
		FskMemPtrDispose(state->readBuffer);
		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr waveReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	waveReader state = readerState;
	FskErr err = kFskErrNone;

	if (0 != index)
		return kFskErrInvalidParameter;

	state->track.state = state;
	state->track.dispatch = &gMediaReaderWaveTrack;

	*track = &state->track;

	return err;
}

FskErr waveReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	waveReader state = readerState;
	double sampleTime, duration;

	state->atTime = startTime ? (UInt32)*startTime : 0;

	duration = (double)state->duration * (double)state->sampleRate;
	sampleTime = (double)state->atTime;
	if (sampleTime > duration)
		sampleTime = duration;

	if (endTime) {
		if (*endTime > duration)
			state->endTime = (UInt32)duration;
		else
			state->endTime = (UInt32)*endTime;
		state->hasEndTime = true;
	}
	else
		state->hasEndTime = false;

	state->position = (FskInt64)(sampleTime * (state->bytesPerSample * state->channelCount));
	state->position += state->dataOffset;

	// reset buffers
	state->readBufferPtr = state->readBuffer;
	state->readBufferEnd = state->readBuffer;

	return kFskErrNone;
}

FskErr waveReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr waveReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **dataOut)
{
	waveReader state = readerState;
	FskErr err = kFskErrNone;
	unsigned char *data;
	UInt32 sampleSize = state->bytesPerSample * state->channelCount;

	*track = &state->track;
	*infoCount = 0;
	*info = NULL;

	// We return audio in one second chunks
	err = FskMemPtrNew(state->readBufferSize, (FskMemPtr *)&data);
	if (err) return err;

	*dataOut = data;

	while (true) {
		UInt32 samplesRequested, samplesAvailable;

		if (state->hasEndTime && (state->atTime >= state->endTime)) {
			err = kFskErrEndOfFile;
			goto bail;
		}

		if ((UInt32)(state->readBufferEnd - state->readBufferPtr) < sampleSize) {
			err = waveRefillReadBuffer(state, sampleSize - (state->readBufferEnd - state->readBufferPtr));
			if (err) goto bail;
		}
					
		samplesRequested = state->sampleRate;
		samplesAvailable = ((UInt32)state->readBufferEnd - (UInt32)state->readBufferPtr) / sampleSize;
		if (samplesRequested > samplesAvailable)
			samplesRequested = samplesAvailable;

		*infoCount = 1;
		err = FskMemPtrNewClear(*infoCount * sizeof(FskMediaReaderSampleInfoRecord), (FskMemPtr *)info);
		if (err) goto bail;

		(*info)->samples = samplesRequested;
		(*info)->sampleSize = sampleSize;
		(*info)->flags = kFskImageFrameTypeSync;
		(*info)->decodeTime = state->atTime;
		(*info)->sampleDuration = 1;
		(*info)->compositionTime = -1;

		FskMemMove(data, state->readBufferPtr, (*info)->sampleSize * (*info)->samples);
		state->readBufferPtr += (*info)->sampleSize * (*info)->samples;
		state->atTime += samplesRequested;
		break;
	}

bail:
	if (kFskErrNone != err) {
		FskMemPtrDisposeAt((void **)info);
		FskMemPtrDisposeAt((void **)dataOut);
	}

	return err;
}

FskErr waveReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	return kFskErrUnknownElement;
}

FskErr waveReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	unsigned char *header = (unsigned char *)data;
	UInt32 offset, chunk_size;
	
	if (dataSize < 64) 
		goto bail;
	
	if (('RIFF' != FskMisaligned32_GetBtoN(&header[0])) || 
		('WAVE' != FskMisaligned32_GetBtoN(&header[8])) ||
		('fmt ' != FskMisaligned32_GetBtoN(&header[12])))
		goto bail;
	
	offset = 16;
	chunk_size = FskMisaligned32_GetLtoN(&header[offset]);
	offset += 4;
	offset += chunk_size;
	
	if ('data' == FskMisaligned32_GetBtoN(&header[offset])) {
		*mime = FskStrDoCopy("audio/wav");
		return kFskErrNone;
	}
	
	if ('LIST' == FskMisaligned32_GetBtoN(&header[offset])) {
		offset += 4;
		chunk_size = FskMisaligned32_GetLtoN(&header[offset]);
		offset += 4;
		offset += chunk_size;
		
		if (dataSize < offset + 4)
			goto bail;	//no enough data
		
		if ('data' == FskMisaligned32_GetBtoN(&header[offset])) {
			*mime = FskStrDoCopy("audio/wav");
			return kFskErrNone;
		}
	}
	
bail:	
	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr waveReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = state->duration * state->sampleRate;

	return kFskErrNone;
}

FskErr waveReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr waveReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->sampleRate;

	return kFskErrNone;
}

FskErr waveReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr waveReaderGetSeekableSegment(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	if (!(kFskMediaSpoolerDownloading & state->spooler->flags))
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)(state->spoolerPosition / state->bytesPerSample) / (double)state->sampleRate;

	return kFskErrNone;
}

FskErr waveReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "audio/wav\000audio/x-wav\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr waveReaderTrackGetMediaType(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("audio");

	return kFskErrNone;
}

FskErr waveReaderTrackGetFormat(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy((2 == state->bytesPerSample) ? "x-audio-codec/pcm-16-le" : "x-audio-codec/pcm-8-offset");

	return kFskErrNone;
}

FskErr waveReaderTrackGetSampleRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->sampleRate;

	return kFskErrNone;
}

FskErr waveReaderTrackGetBitRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->bytesPerSample * state->channelCount * state->sampleRate * 8;

	return kFskErrNone;
}

FskErr waveReaderTrackGetChannelCount(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	waveReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->channelCount;

	return kFskErrNone;
}

/*
	local
*/

FskErr waveInstantiate(waveReader state)
{
	FskErr err;
	unsigned char *fmtP;
	unsigned char *header;
	UInt32 offset, chunk_size;
	waveInfoRecord wi;
	
	err = FskMediaSpoolerRead(state->spooler, 0, 64, (void *)&header, NULL);
	if (err) return err;

	offset = 16;
	chunk_size = FskMisaligned32_GetLtoN(&header[offset]);
	offset += 4;
	offset += chunk_size;
	
	if ('data' == FskMisaligned32_GetBtoN(&header[offset])) {
		offset += 4;
		chunk_size = FskMisaligned32_GetLtoN(&header[offset]);		
		offset += 4;
		state->dataOffset = offset;
		
		wi.dataSize = chunk_size;
		fmtP = &header[20];
	}
	else if ('LIST' == FskMisaligned32_GetBtoN(&header[offset])) {
		offset += 4;
		chunk_size = FskMisaligned32_GetLtoN(&header[offset]);
		offset += 4;
		offset += chunk_size;

		//re-spool extened data
		err = FskMediaSpoolerRead(state->spooler, 0, offset + 8, (void *)&header, NULL);
		if (err) return err;
		
		if ('data' != FskMisaligned32_GetBtoN(&header[offset]))
			return kFskErrUnimplemented;

		offset += 4;
		chunk_size = FskMisaligned32_GetLtoN(&header[offset]);		
		offset += 4;
		state->dataOffset = offset;
		
		wi.dataSize = chunk_size;
		fmtP = &header[20];
	}
	else
		return kFskErrUnimplemented;
	
	wi.audioFormat = FskMisaligned16_GetLtoN(fmtP); fmtP += 2;
	wi.numChannels = FskMisaligned16_GetLtoN(fmtP); fmtP += 2;
	wi.sampleRate = FskMisaligned32_GetLtoN(fmtP); fmtP += (4 + 4 + 2);
	wi.bitsPerSample = FskMisaligned16_GetLtoN(fmtP);

	if ((1 != wi.audioFormat) || ((16 != wi.bitsPerSample) && (8 != wi.bitsPerSample)) || (wi.numChannels < 1) || (wi.numChannels > 2))
		return kFskErrUnimplemented;

	state->readBufferSize = wi.sampleRate * (wi.bitsPerSample / 8) * wi.numChannels;
	state->readBufferSize /= 2;		//*** half second chunks, need to revist this later  --bine, peter  1/19/07
	err = FskMemPtrNew(state->readBufferSize, (FskMemPtr*)&state->readBuffer);
	if (err) return err;

	state->channelCount = wi.numChannels;
	state->sampleRate = wi.sampleRate;
	state->bytesPerSample = wi.bitsPerSample / 8;
	state->duration = ((double)wi.dataSize) / (state->bytesPerSample * state->channelCount) / state->sampleRate;
	state->position = state->dataOffset;

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

	return kFskErrNone;
}

FskErr waveRefillReadBuffer(waveReader state, UInt32 minimumBytesNeeded)
{
	FskErr err = kFskErrNone;
	UInt32 bytesInBuffer = state->readBufferEnd - state->readBufferPtr;
	UInt32 bytesRead;
	void *buffer;

	FskMemMove(state->readBuffer, state->readBufferPtr, bytesInBuffer);
	state->readBufferPtr = state->readBuffer;
	state->readBufferEnd = state->readBufferPtr + bytesInBuffer;

	err = FskMediaSpoolerRead(state->spooler, state->position, state->readBufferSize - bytesInBuffer, &buffer, &bytesRead);
	if (err) return err;

	FskMemMove(state->readBufferEnd, buffer, bytesRead);

	state->position += bytesRead;
	state->readBufferEnd += bytesRead;

	if (bytesRead < minimumBytesNeeded) {
		err = kFskErrOperationFailed;
		goto bail;
	}

bail:
	return err;
}

FskErr waveSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	waveReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			state->spoolerPosition += (UInt32)param;
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = waveInstantiate(state);
			break;

		case kFskMediaSpoolerOperationGetURI:
			state->spoolerPosition = ((FskMediaSpoolerGetURI)param)->position;
			break;
	}

	return err;
}
