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

static Boolean pcmReaderCanHandle(const char *mimeType);
static FskErr pcmReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr pcmReaderDispose(FskMediaReader reader, void *readerState);
static FskErr pcmReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr pcmReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr pcmReaderStop(FskMediaReader reader, void *readerState);
static FskErr pcmReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr pcmReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr pcmReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderSetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord pcmReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			pcmReaderGetDuration,		pcmReaderSetDuration},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			pcmReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		pcmReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		pcmReaderGetState,			NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	pcmReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderPCM = {pcmReaderCanHandle, pcmReaderNew, pcmReaderDispose, pcmReaderGetTrack, pcmReaderStart, pcmReaderStop, pcmReaderExtract, NULL, pcmReaderProperties, pcmReaderSniff};

static FskErr pcmReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr pcmReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord pcmReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		pcmReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		pcmReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		pcmReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		pcmReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		pcmReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gPCMReaderAudioTrack = {pcmReaderAudioTrackProperties};

Boolean pcmReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitiveWithLength("audio/L16", mimeType, 9))
		return true;

	return false;
}

typedef struct pcmReaderRecord pcmReaderRecord;
typedef struct pcmReaderRecord *pcmReader;

typedef struct pcmReaderTrackRecord pcmReaderTrackRecord;
typedef struct pcmReaderTrackRecord *pcmReaderTrack;

struct pcmReaderTrackRecord {
	FskMediaReaderTrackRecord			readerTrack;
	pcmReader							state;

	char								*format;

	UInt32								bitRate;

	union {
		struct {
			UInt32						sampleRate;
			UInt32						channelCount;
		} audio;
	};
};

 struct pcmReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	UInt32					scale;
	UInt32					duration;

	FskMediaReader			reader;

	pcmReaderTrack			tracks;

	UInt32					headerSize;
	FskInt64				fileOffset;
	UInt32                  firstSampleOffset;
     
	pcmReaderTrackRecord	audio;
};

static FskErr doRead(pcmReader state, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr pcmSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static void parseContentType(pcmReader state, const char *contentType);

 FskErr pcmReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	pcmReader state = NULL;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(pcmReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	state->spooler->onSpoolerCallback = pcmSpoolerCallback;
	state->spooler->clientRefCon = state;

	state->scale = 1000;
	state->duration = kFskUInt32Max;

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

    if (mimeType) {
        parseContentType(state, mimeType);

        if (state->audio.audio.channelCount) {
			FskInt64 totalBytes;

			if (kFskErrNone == state->spooler->doGetSize(state->spooler, &totalBytes)) {
				state->duration = totalBytes / (2 * state->audio.audio.channelCount);
				(reader->doSetState)(reader, kFskMediaPlayerStateStopped);
			}
        }
    }

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		pcmReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr pcmReaderDispose(FskMediaReader reader, void *readerState)
{
	pcmReader state = readerState;

	if (NULL != state) {

		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr pcmReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	pcmReader state = readerState;

	if (0 != index)
		return kFskErrNotFound;

	*track = &state->audio.readerTrack;

	return kFskErrNone;
}

FskErr pcmReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	pcmReader state = readerState;

	state->atTime = startTime ? (UInt32)*startTime : 0;

	if (kFskUInt32Max != state->duration) {
		if (endTime) {
			if (*endTime > state->duration)
				state->endTime = (UInt32)state->duration;
			else
				state->endTime = (UInt32)*endTime;
			state->hasEndTime = true;
		}
		else
			state->hasEndTime = false;
    }

    if (state->spooler->flags & kFskMediaSpoolerTimeSeekSupported) {
        state->spooler->flags |= kFskMediaSpoolerUseTimeSeek;
        state->fileOffset = state->firstSampleOffset;
    }
    else
        state->fileOffset = state->atTime * 2 * state->audio.audio.channelCount;

	return kFskErrNone;
}

FskErr pcmReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr pcmReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	pcmReader state = readerState;
	FskErr err = kFskErrNone;
	UInt32 bytesNeeded = (state->audio.audio.sampleRate * state->audio.audio.channelCount * 2) / 4;		// quarter second
	unsigned char *data;
	FskMediaReaderSampleInfo info;

	err = FskMemPtrNew(bytesNeeded, &data);
	BAIL_IF_ERR(err);

	err = doRead(state, state->fileOffset, bytesNeeded, data);
	if (kFskErrNone != err) {
		FskMemPtrDispose(data);
		goto bail;
	}

	*infoCountOut = 1;
	err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
	BAIL_IF_ERR(err);

	info->samples = state->audio.audio.sampleRate / 4;
	info->sampleSize = state->audio.audio.channelCount * 2;
	info->flags = kFskImageFrameTypeSync;
	info->decodeTime = state->atTime;
	info->sampleDuration = 1;
	info->compositionTime = -1;

	*infoOut = info;
	*trackOut = &state->audio.readerTrack;
	*dataOut = data;

	state->atTime += info->samples;
	state->fileOffset += bytesNeeded;

bail:
	return err;
}

FskErr pcmReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (NULL != headers) {
		char *contentType = FskHeaderFind("Content-Type", headers);

		if (contentType && (0 == FskStrCompareCaseInsensitiveWithLength("audio/L16;", contentType, 10))) {
			*mime = FskStrDoCopy("audio/L16");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr pcmReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (kFskUInt32Max != state->duration) ? state->duration : -1.0;

	return kFskErrNone;
}

FskErr pcmReaderSetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReader state = readerState;

	state->duration = (UInt32)property->value.number;

	return kFskErrNone;
}

FskErr pcmReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr pcmReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

FskErr pcmReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr pcmReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "audio/L16\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr pcmReaderTrackGetMediaType(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("audio");

	return kFskErrNone;
}

FskErr pcmReaderTrackGetFormat(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("x-audio-codec/pcm-16-be");

	return kFskErrNone;
}

FskErr pcmReaderTrackGetSampleRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.sampleRate;

	return kFskErrNone;
}

FskErr pcmReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 16 * track->audio.channelCount * track->audio.sampleRate;

	return kFskErrNone;
}

FskErr pcmReaderTrackGetChannelCount(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	pcmReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channelCount;

	return kFskErrNone;
}

/*
	local
*/

FskErr doRead(pcmReader state, FskInt64 offset, UInt32 size, void *bufferIn)
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

FskErr pcmSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	pcmReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationGetHeaders: {
			const char *contentType = FskHeaderFind("Content-Type", (FskHeaders *)param), *timeSeekRange;
            
            state->firstSampleOffset = 0;
			if (!contentType || (0 != FskStrCompareCaseInsensitiveWithLength("audio/L16;", contentType, 10)))
				(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
			else {
                if (0 == state->audio.audio.sampleRate)
                    parseContentType(state, contentType);

                if (kFskUInt32Max == state->duration) {
                    char *contentLength = FskHeaderFind("Content-Length", (FskHeaders *)param);
                    if (contentLength) {
                        UInt32 totalBytes = state->fileOffset + FskStrToNum(contentLength);
                        state->duration = totalBytes / (2 * state->audio.audio.channelCount);
                    }
                }

				timeSeekRange = FskHeaderFind("TimeSeekRange.dlna.org", (FskHeaders *)param);
				if (timeSeekRange) {
					char* bytes = FskStrStr(timeSeekRange, "bytes=");
					if (bytes) {
						FskInt64 offset = FskStrToFskInt64(bytes + 6);
						if (offset)
							state->fileOffset = state->firstSampleOffset = offset % (state->audio.audio.channelCount * 2);
					}
				}
			}
			}
			break;

		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

void parseContentType(pcmReader state, const char *contentType)
{
    const char *p = contentType + 10;
    
    while (*p) {
        const char *next = FskStrChr(p, ';');
        if (!next)
            next = p + FskStrLen(p);
        else
            next += 1;
        
        p = FskStrStripHeadSpace(p);
        if (0 == FskStrCompareWithLength(p, "rate=", 5))
            state->audio.audio.sampleRate = FskStrToNum(p + 5);
        else
            if (0 == FskStrCompareWithLength(p, "channels=", 9))
                state->audio.audio.channelCount = FskStrToNum(p + 9);
        
        p = next;
    }
    
    if (((1 == state->audio.audio.channelCount) || (2 == state->audio.audio.channelCount)) && (0 != state->audio.audio.sampleRate)) {
        state->scale = state->audio.audio.sampleRate;
        
        state->audio.readerTrack.dispatch = &gPCMReaderAudioTrack;
        state->audio.readerTrack.state = &state->audio.readerTrack;
        state->audio.state = state;
        
        state->spooler->flags |= kFskMediaSpoolerDontDownload;
    }
    else
        (state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
}
