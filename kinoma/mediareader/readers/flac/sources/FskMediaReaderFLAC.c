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
#include "FskFiles.h"
#include "FskDIDLGenMedia.h"
#include "FskTextConvert.h"

#include "stream_decoder.h"

#if SUPPORT_INSTRUMENTATION
#include "FskPlatformImplementation.h"
FskInstrumentedTypeRecord gFskMediaReaderFLACTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "FskMediaReaderFLAC"};
#define klog(...)  do { FskInstrumentedTypePrintfDebug  (&gFskMediaReaderFLACTypeInstrumentation, __VA_ARGS__); } while(0)
#else
#define klog(...) do{}while(0)
#endif

static Boolean flacReaderCanHandle(const char *mimeType);
static FskErr flacReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr flacReaderDispose(FskMediaReader reader, void *readerState);
static FskErr flacReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr flacReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr flacReaderStop(FskMediaReader reader, void *readerState);
static FskErr flacReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr flacReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr flacReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr flacReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderGetError(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord flacReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			flacReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			flacReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		flacReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		flacReaderGetState,			NULL},
	{kFskMediaPropertyError,				kFskMediaPropertyTypeInteger,		flacReaderGetError,			NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	flacReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderFLAC = {flacReaderCanHandle, flacReaderNew, flacReaderDispose, flacReaderGetTrack, flacReaderStart, flacReaderStop, flacReaderExtract, flacReaderGetMetadata, flacReaderProperties, flacReaderSniff};

static FskErr flacReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flacReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord flacReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		flacReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		flacReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		flacReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		flacReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		flacReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaReaderTrackDispatchRecord gFLACReaderAudioTrack = {flacReaderAudioTrackProperties};

Boolean flacReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("application/flac", mimeType))
		return true;
	
	if (0 == FskStrCompareCaseInsensitive("audio/flac", mimeType))
		return true;
	
	return false;
}

typedef struct flacReaderRecord flacReaderRecord;
typedef struct flacReaderRecord *flacReader;

typedef struct flacReaderTrackRecord flacReaderTrackRecord;
typedef struct flacReaderTrackRecord *flacReaderTrack;

struct flacReaderTrackRecord {
	flacReaderTrackRecord				*next;
	
	FskMediaReaderTrackRecord			readerTrack;
	flacReader							state;
	
	UInt32								sampleRate;
	UInt32								channelCount;
	UInt32								bitsPerSample;
};

struct flacReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;
	FskInt64				spoolerSize;
	FskInt64				spoolerPosition;
	FskErr					readError;
	
	FLAC__StreamDecoder		*flacStream;
	
	
	FskInt64				atTime;
	FskInt64				endTime;
	Boolean					hasEndTime;
	
	UInt16					*samples;
	UInt32					sampleCount;
	
	FskMediaMetaData		metadata;
	FskMediaMetaData		id3Metadata;
	
	UInt32					scale;
	FskInt64				duration;
	
	FskMediaReader			reader;
	
	flacReaderTrack			tracks;
	
	FskErr					theError;
};

static FskErr instantiateFLAC(flacReader state);
static FskErr doRead(flacReader state, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr flacInstantiate(flacReader state);
static FskErr flacSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static FLAC__StreamDecoderReadStatus doStreamDecoderReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus doStreamDecoderSeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus doStreamDecoderTellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus doStreamDecoderLengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool doStreamDecoderEofCallback(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus doStreamDecoderWriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void doStreamDecoderMetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void doStreamDecoderErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FskErr flacReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	flacReader state = NULL;
	
	klog( "into flacReaderNew");
    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(flacReaderRecord), &state);
	BAIL_IF_ERR(err);
	
	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;
	
	state->spooler->onSpoolerCallback = flacSpoolerCallback;
	state->spooler->clientRefCon = state;
	state->spooler->flags |= kFskMediaSpoolerForwardOnly;
	
	state->scale = 1000;		// overwritten later
	
	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);
		
		state->spoolerOpen = true;
	}
	
	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);
	
	err = FskMemPtrNewClear(sizeof(flacReaderTrackRecord), (FskMemPtr *)&state->tracks);
	BAIL_IF_ERR(err);
	
	state->tracks->state = state;
	state->tracks->readerTrack.dispatch = &gFLACReaderAudioTrack;
	state->tracks->readerTrack.state = state->tracks;
	
	err = flacInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}
	
bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		flacReaderDispose(reader, state);
		state = NULL;
	}
	
	*readerState = state;
	
	return err;
}

FskErr flacReaderDispose(FskMediaReader reader, void *readerState)
{
	flacReader state = readerState;
	
	klog( "into flacReaderDispose");
	if (NULL != state) {
		FskMediaMetaDataDispose(state->metadata);
		FskMediaMetaDataDispose(state->id3Metadata);
		
		if (state->flacStream)
			FLAC__stream_decoder_delete(state->flacStream);
		
		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);
		
		while (state->tracks) {
			flacReaderTrack track = FskListRemoveFirst((void **)&state->tracks);
			FskMemPtrDispose(track);
		}
		
		FskMemPtrDispose(state);
	}
	
	return kFskErrNone;
}

FskErr flacReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *trackOut)
{
	flacReader state = readerState;
	flacReaderTrack track = state->tracks;
	
	klog( "into flacReaderGetTrack");
	while ((NULL != track) && (0 != index))
		track = track->next;
	
	if (NULL == track)
		return kFskErrNotFound;
	
	*trackOut = &track->readerTrack;
	
	return kFskErrNone;
}

FskErr flacReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	flacReader state = readerState;
	FskErr err = kFskErrNone;
	
	klog( "into flacReaderStart");
	state->atTime = startTime ? (UInt32)*startTime : 0;
	
	if (state->atTime > state->duration)
		state->atTime = state->duration;
	
	if (endTime) {
		if (*endTime > state->duration)
			state->endTime = (UInt32)state->duration;
		else
			state->endTime = (UInt32)*endTime;
		state->hasEndTime = true;
	}
	else
		state->hasEndTime = false;
	
	if (false == FLAC__stream_decoder_seek_absolute(state->flacStream, state->atTime)) {
		return kFskErrNeedMoreTime;		//@@ not always true
	}
	
	return err;
}

FskErr flacReaderStop(FskMediaReader reader, void *readerState)
{
	flacReader state = readerState;
	
	klog( "into flacReaderStop");
	FskMemPtrDisposeAt((void **)&state->samples);
	state->sampleCount = 0;
	
	return kFskErrNone;
}

FskErr flacReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **dataOut)
{
	flacReader state = readerState;
	FskErr err = kFskErrNone;
	
	klog( "into flacReaderExtract");
	state->readError = kFskErrNone;
	
	if (NULL == state->samples) {
		FskInt64 position;
		unsigned char *readBuffer;
		//@@ only need this check if network
		
		FLAC__stream_decoder_get_decode_position(state->flacStream, (FLAC__uint64 *)&position);
		
		position += 32768;		//@@ generous estimate of a single frame size
		
		if (position >= state->spoolerSize)
			position = state->spoolerSize - 1;
		
		state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;
		err = FskMediaSpoolerRead(state->spooler, position, 1, &readBuffer, NULL);
		state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		if (err) return err;
		
		FLAC__stream_decoder_process_single(state->flacStream);
		
		if (kFskErrNeedMoreTime == state->readError)
			return kFskErrNeedMoreTime;		//@@ not always true
		if (NULL == state->samples)
			return kFskErrNeedMoreTime;
	}
	
	*infoCount = 1;
	err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), (FskMemPtr *)info);
	BAIL_IF_ERR(err);
	
	(*info)->samples = state->sampleCount;
	(*info)->sampleSize = 2 * ((state->tracks->channelCount >= 2) ? 2 : 1);
	(*info)->flags = kFskImageFrameTypeSync;
	(*info)->decodeTime = state->atTime;
	(*info)->sampleDuration = 1;
	(*info)->compositionTime = -1;
	
	*dataOut = (unsigned char *)state->samples;
	
	state->atTime += state->sampleCount;
	
	state->samples = NULL;
	state->sampleCount = 0;
	
	*trackOut = &state->tracks->readerTrack;
	
bail:
	return err;
}

FskErr flacReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetMetadata");
	if (state->id3Metadata && (kFskErrNone == FskMediaMetaDataGetForMediaPlayer(state->id3Metadata, metaDataType, index, value, flags)))
		return kFskErrNone;
	
	return FskMediaMetaDataGetForMediaPlayer(state->metadata, metaDataType, index, value, flags);
}

FskErr flacReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	klog( "into flacReaderSniff");
	if ((dataSize >= 4) && (0 == FskStrCompareWithLength((const char *)data, "fLaC", 4))) {
		*mime = FskStrDoCopy("application/flac");
		return kFskErrNone;
	}
	
	if ((dataSize >= 4) && (0 == FskStrCompareWithLength((const char *)data, "ID3", 3)) && (NULL != uri)) {
		if ((0 == FskStrCompareWithLength(uri, "file://", 7)) && (FskStrLen(uri) > 10) && (0 == FskStrCompareCaseInsensitive(uri + FskStrLen(uri) - 5, ".flac"))) {
			*mime = FskStrDoCopy("application/flac");
			return kFskErrNone;
		}
	}
	
	return kFskErrUnknownElement;
}

/*
 reader properties
 */

FskErr flacReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetDuration");
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (-1 != state->duration) ? state->duration : -1.0;
	
	return kFskErrNone;
}

FskErr flacReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetTime");
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;
	
	return kFskErrNone;
}

FskErr flacReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetTimeScale");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;
	
	return kFskErrNone;
}

FskErr flacReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetState");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;
	
	return kFskErrNone;
}

FskErr flacReaderGetError(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReader state = readerState;
	
	klog( "into flacReaderGetError");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->theError;
	
	return kFskErrNone;
}

FskErr flacReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "application/flac\000audio/flac\000";
	
	klog( "into flacReaderGetDLNASinks");
	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);
	
	return kFskErrNone;
}

/*
 track properties
 */

FskErr flacReaderTrackGetMediaType(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	klog( "into flacReaderTrackGetMediaType");
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("audio");
	
	return kFskErrNone;
}

FskErr flacReaderTrackGetFormat(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	klog( "into flacReaderTrackGetFormat");
	property->type = kFskMediaPropertyTypeString;
#if TARGET_RT_LITTLE_ENDIAN
	property->value.str = FskStrDoCopy("x-audio-codec/pcm-16-le");
#else
	property->value.str = FskStrDoCopy("x-audio-codec/pcm-16-be");
#endif
	
	return kFskErrNone;
}

FskErr flacReaderTrackGetSampleRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReaderTrack track = trackState;
	
	klog( "into flacReaderTrackGetSampleRate");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->sampleRate;
	
	return kFskErrNone;
}

FskErr flacReaderTrackGetBitRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReaderTrack track = trackState;
	flacReader state = track->state;
	
	klog( "into flacReaderTrackGetBitRate");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = (SInt32)(((double)state->spoolerSize / (double)state->duration) * state->scale * 8.0);
	
	return kFskErrNone;
}

FskErr flacReaderTrackGetChannelCount(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flacReaderTrack track = trackState;
	
	klog( "into flacReaderTrackGetChannelCount");
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = (track->channelCount >= 2) ? 2 : 1;
	
	return kFskErrNone;
}

/*
 local
 */

FskErr instantiateFLAC(flacReader state)
{
	FskErr err = kFskErrNone;
	FLAC__StreamDecoderInitStatus status = 0;
	unsigned char *id3 = NULL;
	char buffer[10];
	
	klog( "into instantiateFLAC");
	err = (state->spooler->doGetSize)(state->spooler, &state->spoolerSize);
	BAIL_IF_ERR(err);
	
	err = doRead(state, 0, sizeof(buffer), buffer);
	BAIL_IF_ERR(err);
	
	if (0 == FskStrCompareWithLength(buffer, "ID3", 3)) {
		UInt32 id3TagSize = (((buffer[6] & 0x7f) << 21) | ((buffer[7] & 0x7f) << 14) | ((buffer[8] & 0x7f) << 7) | (buffer[9] & 0x7f)) + 10;
		DIDLMusicItemRecord mi = {0};
		
		err = FskMemPtrNew(id3TagSize, &id3);
		BAIL_IF_ERR(err);
		
		err = doRead(state, 0, id3TagSize, id3);
		BAIL_IF_ERR(err);
		
		err = FskMediaMetaDataNew(&mi.meta);
		BAIL_IF_ERR(err);
		
		if (scanMP3ID3v2(id3, id3 + id3TagSize, &mi)) {
			FskMediaMetaDataDispose(state->id3Metadata);
			state->id3Metadata = mi.meta;
			mi.meta = NULL;
			scanMP3Dispose(&mi);
		}
		
		FskMediaMetaDataDispose(mi.meta);
	}
	
	if (NULL == state->flacStream) {
		state->flacStream = FLAC__stream_decoder_new();
		
        BAIL_IF_NULL(state->flacStream, err, kFskErrMemFull);
		
		FLAC__stream_decoder_set_metadata_respond_all(state->flacStream);
		
		status = FLAC__stream_decoder_init_stream(state->flacStream,
												  doStreamDecoderReadCallback,
												  doStreamDecoderSeekCallback,
												  doStreamDecoderTellCallback,
												  doStreamDecoderLengthCallback,
												  doStreamDecoderEofCallback,
												  doStreamDecoderWriteCallback,
												  doStreamDecoderMetadataCallback,
												  doStreamDecoderErrorCallback,
												  state);
		if (FLAC__STREAM_DECODER_INIT_STATUS_OK != status)
            BAIL(kFskErrOperationFailed);
	}
	
	if (false == FLAC__stream_decoder_process_until_end_of_metadata(state->flacStream)) {
		if (kFskErrNeedMoreTime == state->readError) {
			err = kFskErrNeedMoreTime;
			FLAC__stream_decoder_reset(state->flacStream);
		}
		else
			err = kFskErrBadData;
		goto bail;
	}
	
bail:
	FskMemPtrDispose(id3);
	
	return err;
}

FskErr doRead(flacReader state, FskInt64 offset, UInt32 size, void *bufferIn)
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

FskErr flacInstantiate(flacReader state)
{
	FskErr err;
	
	err = instantiateFLAC(state);
	BAIL_IF_ERR(err);
	
	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);
	
bail:
	return err;
}

FskErr flacSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	flacReader state = clientRefCon;
	FskErr err = kFskErrNone;
	
	switch (operation) {
		case kFskMediaSpoolerOperationGetHeaders:
			state->spooler->flags |= kFskMediaSpoolerDownloadPreferred;		// force download because we cannot seek on network
			break;
			
		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = flacInstantiate(state);
			break;
	}
	
	return err;
}

FLAC__StreamDecoderReadStatus doStreamDecoderReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	flacReader state = client_data;
	
	FskErr err;
	
	err = doRead(state, state->spoolerPosition, *bytes, buffer);
	state->readError = err;
	if (kFskErrNone == err) {
		state->spoolerPosition += *bytes;
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	if (kFskErrEndOfFile == err)
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	if (kFskErrNeedMoreTime == err)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus doStreamDecoderSeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	flacReader state = client_data;
	state->spoolerPosition = absolute_byte_offset;
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus doStreamDecoderTellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	flacReader state = client_data;
	
	*absolute_byte_offset = state->spoolerPosition;
	
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus doStreamDecoderLengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	flacReader state = client_data;
	
	*stream_length = state->spoolerSize;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool doStreamDecoderEofCallback(const FLAC__StreamDecoder *decoder, void *client_data)
{
	flacReader state = client_data;
	
	return (state->spoolerPosition >= state->spoolerSize);
}

FLAC__StreamDecoderWriteStatus doStreamDecoderWriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	flacReader state = client_data;
	UInt16 *out;
	UInt32 sampleCount = frame->header.blocksize;
	UInt32 channelCount = state->tracks->channelCount;
	
	if (NULL != state->samples)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	
	if (kFskErrNone != FskMemPtrNew(sampleCount * 2 * ((channelCount >= 2) ? 2 : 1), &out))
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	
	state->samples = out;
	state->sampleCount = sampleCount;
	
	if (0 == channelCount)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	
	if (16 == state->tracks->bitsPerSample) {	
		if (1 == state->tracks->channelCount) {
			const FLAC__int32 *in = buffer[0];
			
			while (sampleCount--)
				*out++ = (SInt16)*in++;
		}
		else {
			const FLAC__int32 *in0 = buffer[0], *in1 = buffer[1];
			
			while (sampleCount--) {
				*out++ = (SInt16)*in0++;
				*out++ = (SInt16)*in1++;
			}
		}
	}
	else if (24 == state->tracks->bitsPerSample) {
		if (1 == state->tracks->channelCount) {
			const FLAC__int32 *in = buffer[0];
			
			while (sampleCount--)
				*out++ = (SInt16)(*in++ >> 8);
		}
		else {
			const FLAC__int32 *in0 = buffer[0], *in1 = buffer[1];
			
			while (sampleCount--) {
				*out++ = (SInt16)(*in0++ >> 8);
				*out++ = (SInt16)(*in1++ >> 8);
			}
		}
	}
	else if (32 == state->tracks->bitsPerSample) {
		if (1 == state->tracks->channelCount) {
			const FLAC__int32 *in = buffer[0];
			
			while (sampleCount--)
				*out++ = (SInt16)(*in++ >> 16);
		}
		else {
			const FLAC__int32 *in0 = buffer[0], *in1 = buffer[1];
			
			while (sampleCount--) {
				*out++ = (SInt16)(*in0++ >> 16);
				*out++ = (SInt16)(*in1++ >> 16);
			}
		}
	}
	
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void doStreamDecoderMetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	flacReader state = client_data;
	
	if (FLAC__METADATA_TYPE_STREAMINFO == metadata->type) {
		state->scale = metadata->data.stream_info.sample_rate;
		
		state->tracks->sampleRate = metadata->data.stream_info.sample_rate;
		state->tracks->channelCount = metadata->data.stream_info.channels;
		state->tracks->bitsPerSample = metadata->data.stream_info.bits_per_sample;
		
		state->duration = metadata->data.stream_info.total_samples;
	}
	else
		if (FLAC__METADATA_TYPE_VORBIS_COMMENT == metadata->type) {
			const char *fields[] = {"TITLE", "FullName", "ARTIST", "Artist", "ALBUM", "Album", "GENRE", "Genre", "DATE", "Year", "TRACKNUMBER", "TrackNumber", NULL, NULL};
			UInt32 i, j;
			
			FskMediaMetaDataDispose(state->metadata);
			state->metadata = NULL;
			if (kFskErrNone != FskMediaMetaDataNew(&state->metadata))
				return;
			
			for (i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
				for (j = 0; NULL != fields[j]; j += 2) {
					UInt32 len = FskStrLen(fields[j]);
					FskMediaPropertyValueRecord value;
					
					if (len >= metadata->data.vorbis_comment.comments[i].length)
						continue;
					if ('=' != metadata->data.vorbis_comment.comments[i].entry[len])
						continue;
					if (0 != FskStrCompareWithLength((const char *)metadata->data.vorbis_comment.comments[i].entry, fields[j], len))
						continue;
					
					value.type = kFskMediaPropertyTypeString;
					value.value.str = (char *)metadata->data.vorbis_comment.comments[i].entry + len + 1;
					FskMediaMetaDataAdd(state->metadata, fields[j + 1], NULL, &value, 0);
					break;
				}
			}
		}
}

void doStreamDecoderErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//	flacReader state = client_data;
}
