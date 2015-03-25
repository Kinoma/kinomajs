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
#include "FskEndian.h"
#include "FskDIDLGenMedia.h"
#include "FskFiles.h"
#include "FskMediaReader.h"
#include "QTReader.h"

#include "kinoma_ipp_lib.h"
#include "kinoma_avc_header_parser.h"

FskInstrumentedSimpleType(MediaReaderFLV, mediareaderflv);

static Boolean flvReaderCanHandle(const char *mimeType);
static FskErr flvReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr flvReaderDispose(FskMediaReader reader, void *readerState);
static FskErr flvReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr flvReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr flvReaderStop(FskMediaReader reader, void *readerState);
static FskErr flvReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr flvReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr flvReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr flvReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderSetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderGetSeekableSegment(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord flvReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			flvReaderGetDuration,		flvReaderSetDuration},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			flvReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		flvReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		flvReaderGetState,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						flvReaderSetScrub},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			flvReaderGetSeekableSegment,	NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	flvReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderFLV = {flvReaderCanHandle, flvReaderNew, flvReaderDispose, flvReaderGetTrack, flvReaderStart, flvReaderStop, flvReaderExtract, flvReaderGetMetadata, flvReaderProperties, flvReaderSniff};

static FskErr flvReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvReaderTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord flvReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		flvReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		flvReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			flvReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		flvReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		flvReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		flvReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskMediaPropertyEntryRecord flvReaderVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		flvReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		flvReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			flvReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		flvReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			flvReaderTrackGetFrameRate,			NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		flvReaderTrackGetDimensions,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gFLVReaderAudioTrack = {flvReaderAudioTrackProperties};
FskMediaReaderTrackDispatchRecord gFLVReaderVideoTrack = {flvReaderVideoTrackProperties};

static const UInt32 kFLVLiveDuration = kFskUInt32Max;

Boolean flvReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("video/flv", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/x-flv", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("audio/flv", mimeType))
		return true;

	return false;
}

typedef struct flvReaderRecord flvReaderRecord;
typedef struct flvReaderRecord *flvReader;

typedef struct flvReaderTrackRecord flvReaderTrackRecord;
typedef struct flvReaderTrackRecord *flvReaderTrack;

struct flvReaderTrackRecord {
	FskMediaReaderTrackRecord			readerTrack;
	flvReader							state;

	char								*format;

	UInt32								bitRate;
	UInt32								profile;
	UInt32								level;
	UInt8								*codecSpecificDataRef;
	UInt32								codecSpecificDataSize;

	union {
		struct {
			UInt32						sampleRate;
			UInt32						channelCount;
			UInt8						codec;
		} audio;

		struct {
			FskDimensionRecord			dimensions;
			double						frameRate;
		} video;
	};
};

 struct flvReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;
    UInt32                  decodeTimeOffset;

	UInt32					scale;
	UInt32					duration;

	FskMediaReader			reader;

	flvReaderTrack			tracks;

	UInt32					headerSize;
	FskInt64				fileOffset;

	Boolean					hasVideo;
	Boolean					hasAudio;
	Boolean					scrub;
	Boolean					seeking;
	Boolean					flvSeeking;
	Boolean					canSeekOnTime;
	SInt32					seekTableInterval;
	SInt32					seekTableLength;
	UInt32					*seekTable;

	UInt32					bytesToSkip;

	flvReaderTrackRecord	video;
	flvReaderTrackRecord	audio;

    FskMediaMetaData        meta;

    struct {
		UInt32							parseState;
		UInt32							nextParseState;
		UInt32							bytesToSkip;
		UInt32							bytesToCollect;
		unsigned char					*collectionBuffer;
		unsigned char					buffer[16];
		UInt32							bytesParsed;
		UInt32							maximumDownloadedTime;
		Boolean							cantSeek;
	} http;
};

enum {
	kFLVParseNone = 0,
	kFLVParseInitialize = 1,
	kFLVParseCollect,
	kFLVParseSkip,
	kFLVParseFileHeader,
	kFLVParseFrameHeaderStart,
	kFLVParseFrameHeaderDone
};

static FskErr instantiateFLV(flvReader state);
static FskErr demuxNextDLV(flvReader state, UInt32 *timeStampOut, UInt8 *tagTypeOut, UInt8 *tagFlagsOut, unsigned char **dataOut, UInt32 *dataSizeOut, UInt8 *AVCFlagsOut);
static void getDimensions(unsigned char *bits, UInt32 *width, UInt32 *height);
static void scanMetadata(flvReader state, unsigned char *meta, unsigned char *metaEnd);
static FskErr growSeekTable(flvReader state, UInt32 length);
static void setSeekTableIndex(flvReader state, UInt32 timeStamp, UInt32 value);
static FskErr doRead(flvReader state, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr flvInstantiate(flvReader state);
static FskErr flvSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static void creat_aac_esds_config(UInt8 *config, UInt32 frequency, UInt32 channel);

 FskErr flvReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	flvReader state = NULL;

	if (NULL == spooler) {
		err = kFskErrUnimplemented;
		goto bail;
	}
	
	err = FskMemPtrNewClear(sizeof(flvReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;

	state->spooler->onSpoolerCallback = flvSpoolerCallback;
	state->spooler->clientRefCon = state;
	state->spooler->flags |= kFskMediaSpoolerForwardOnly;

	state->scale = 1000;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = flvInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		flvReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr flvReaderDispose(FskMediaReader reader, void *readerState)
{
	flvReader state = readerState;

	if (NULL != state) {
		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		FskMemPtrDispose(state->seekTable);
        FskMediaMetaDataDispose(state->meta);

		if (state->video.codecSpecificDataRef != NULL)
			FskMemPtrDispose(state->video.codecSpecificDataRef);
		if (state->audio.codecSpecificDataRef != NULL)
			FskMemPtrDispose(state->audio.codecSpecificDataRef);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr flvReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	flvReader state = readerState;

	if (state->hasAudio && state->hasVideo) {
		if (0 == index)
			*track = &state->video.readerTrack;
		else
		if (1 == index)
			*track = &state->audio.readerTrack;
		else
			return kFskErrNotFound;
	}
	else
	if (state->hasAudio && (0 == index))
		*track = &state->audio.readerTrack;
	else
	if (state->hasVideo && (0 == index))
		*track = &state->video.readerTrack;
	else
		return kFskErrNotFound;

	return kFskErrNone;
}

FskErr flvReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	flvReader state = readerState;
	UInt32 sampleTime;
	Boolean firstTime = true;

	state->atTime = startTime ? (UInt32)*startTime : 0;

    state->decodeTimeOffset = 0;
	if (kFLVLiveDuration != state->duration) {
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

        if ((kFLVLiveDuration != state->duration) && (state->spooler->flags & kFskMediaSpoolerTimeSeekSupported)) {
            state->spooler->flags |= kFskMediaSpoolerUseTimeSeek;
            state->fileOffset = state->headerSize + 4;
            state->decodeTimeOffset = state->atTime;
            return kFskErrNone;
        }

		if (state->canSeekOnTime) {
			state->fileOffset += 4000000;	// offset is fake - just want to convince http cache it needs to seek
			if (state->fileOffset >= 0x70000000)
				state->fileOffset = 1024;	// be careful of overflow
			return kFskErrNone;		
		}

	again:
		state->fileOffset = state->headerSize + 4;
		if (state->seekTable && state->seekTableInterval) {
			SInt32 thisSeekTableIndex = sampleTime / state->seekTableInterval;
			SInt32 backUpCount = 0;
			if (thisSeekTableIndex >= state->seekTableLength)
				thisSeekTableIndex = state->seekTableLength - 1;
			for ( ; thisSeekTableIndex >= 0; thisSeekTableIndex -= 1, backUpCount += 1) {
				if (0 != state->seekTable[thisSeekTableIndex]) {
					state->fileOffset = state->seekTable[thisSeekTableIndex];
					break;
				}
			}

			if (firstTime && (backUpCount > (10000 / state->seekTableInterval))) {
				// fast scan to try to get there
				while (true) {
					UInt32 thisTime;

					if (kFskErrNone != demuxNextDLV(state, &thisTime, NULL, NULL, NULL, NULL, NULL))
						break;

					if (thisTime >= sampleTime)
						break;
				}

				firstTime = false;
				goto again;
			}
		}
	}
    else
		state->fileOffset = state->headerSize + 4;

	return kFskErrNone;
}

FskErr flvReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr flvReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	flvReader state = readerState;
	FskErr err = kFskErrNone;

	while (true) {
		UInt32 timeStamp, dataSize;
		UInt8 tagType, tagFlags, AVCAACFlags = 1;
		FskMediaReaderSampleInfo info;

		err = demuxNextDLV(state, &timeStamp, &tagType, &tagFlags, dataOut, &dataSize, &AVCAACFlags);
		if (err) break;

		if (0 == AVCAACFlags) //skip config data!
			continue;

        timeStamp += state->decodeTimeOffset;

		if ((8 == tagType) && state->hasAudio) {
			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) {
				FskMemPtrDisposeAt((void **)dataOut);
				goto bail;
			}

			if (3 != state->audio.audio.codec) {
				info->samples = 1;
				info->sampleSize = dataSize;
				FskMediaReaderFLVPrintfDebug( "audio data, codec: %d, samples: 1, size: %d", state->audio.audio.codec, dataSize );
			}
			else {
				info->sampleSize = state->audio.audio.channelCount * 2;
				info->samples = dataSize / info->sampleSize;
				FskMediaReaderFLVPrintfDebug( "audio data, codec: 3, samples: %d, size: %d", info->samples, dataSize );
			}

 			info->flags = kFskImageFrameTypeSync;
			info->decodeTime = timeStamp;
			info->compositionTime = -1;

			*infoCountOut = 1;
			*infoOut = info;
			*trackOut = &state->audio.readerTrack;

            if (0xffffffff==state->audio.level && 10 == state->audio.audio.codec) {
                UInt32 sample_rate_ext = 0;
                SInt32 sbr_flag = 0;
                get_extended_aac_profile_level(*dataOut, state->audio.audio.sampleRate, state->audio.audio.channelCount, &sample_rate_ext, &sbr_flag, &state->audio.profile, &state->audio.level);
                if( state->audio.audio.sampleRate != sample_rate_ext && sample_rate_ext != 0 ) {
                    FskMediaReaderFLVPrintfDebug( "update aac sample rate, old/new: %d, %d", state->audio.audio.sampleRate, sample_rate_ext );
                    //state->audio.audio.sampleRate = sample_rate_ext;
                }
            }
			break;
		}
		else
		if ((9 == tagType) && state->hasVideo) {
			UInt32 flags = (1 == ((tagFlags & 0xf0) >> 4)) ? kFskImageFrameTypeSync : kFskImageFrameTypeDifference;
			if (3 == ((tagFlags & 0xf0) >> 4))
				flags = kFskImageFrameTypeDroppable;

			if (state->scrub && (kFskImageFrameTypeSync != flags)) {
				FskMemPtrDisposeAt((void **)dataOut);
				continue;
			}

			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) {
				FskMemPtrDisposeAt((void **)dataOut);
				goto bail;
			}

			info->samples = 1;
			info->sampleSize = dataSize;
			info->flags = flags;
			info->decodeTime = timeStamp;
			info->compositionTime = -1;

			*infoCountOut = 1;
			*infoOut = info;
			*trackOut = &state->video.readerTrack;
			break;
		}
        else if (18 == tagType)
			scanMetadata(state, *dataOut, *dataOut + dataSize);

		FskMemPtrDisposeAt((void **)dataOut);
	}

bail:

	return err;
}

FskErr flvReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	flvReader state = readerState;

    if (NULL == state->meta)
        return kFskErrNotFound;

	return FskMediaMetaDataGetForMediaPlayer(state->meta, metaDataType, index, value, flags);
}

FskErr flvReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize >= 9) {
		if (('F' == data[0]) && ('L' == data[1]) && ('V' == data[2])) {
			UInt32 headerSize = FskMisaligned32_GetBtoN((data + 5)), timeStamp;
			if (dataSize >= (headerSize + 16)) {
				data += headerSize;

				timeStamp = ((UInt32)data[8] << 16) | ((UInt32)data[9] << 8) | (UInt32)data[10];
				if (timeStamp < 1250) {		// try to weed out bad time stamps... it really should start at 0
					*mime = FskStrDoCopy("video/x-flv");
					return kFskErrNone;
				}
			}
		}
	}

	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr flvReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (kFLVLiveDuration != state->duration) ? state->duration : -1.0;

	return kFskErrNone;
}

FskErr flvReaderSetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	state->duration = (UInt32)property->value.number;

	return kFskErrNone;
}

FskErr flvReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr flvReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

FskErr flvReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr flvReaderSetScrub(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	state->scrub = property->value.b;

	return kFskErrNone;
}

FskErr flvReaderGetSeekableSegment(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReader state = readerState;

	if (!(kFskMediaSpoolerDownloading & state->spooler->flags) || (kFLVLiveDuration == state->duration))
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = state->http.maximumDownloadedTime / 1000.0;


	return kFskErrNone;
}

FskErr flvReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/flv\000video/x-flv\000audio/flv\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr flvReaderTrackGetMediaType(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;
	flvReader state = track->state;

	property->type = kFskMediaPropertyTypeString;
	if (track == &state->audio)
		property->value.str = FskStrDoCopy("audio");
	else
	if (track == &state->video)
		property->value.str = FskStrDoCopy("video");
	else
		property->value.str = FskStrDoCopy("unknown");

	return kFskErrNone;
}

FskErr flvReaderTrackGetFormat(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);

	return kFskErrNone;
}

FskErr flvReaderTrackGetFormatInfo(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	if (0 == FskStrCompare(track->format, "x-video-codec/avc") || 0 == FskStrCompare(track->format, "x-audio-codec/aac")) {
		if (NULL == track->codecSpecificDataRef)
			return kFskErrUnimplemented;

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = track->codecSpecificDataSize;
		return FskMemPtrNewFromData(track->codecSpecificDataSize, track->codecSpecificDataRef, (FskMemPtr *)&property->value.data.data);
	}
	else
		return kFskErrUnimplemented;
}

FskErr flvReaderTrackGetSampleRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.sampleRate;

	return kFskErrNone;
}

FskErr flvReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->bitRate;

	return kFskErrNone;
}

FskErr flvReaderTrackGetFrameRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	if (0 == track->video.frameRate)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = (SInt32)(1000 * track->video.frameRate);
	property->value.ratio.denom = 1000;

	return kFskErrNone;
}

FskErr flvReaderTrackGetChannelCount(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channelCount;

	return kFskErrNone;
}

FskErr flvReaderTrackGetDimensions(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvReaderTrack track = readerState;

	if ((0 == track->video.dimensions.width) && (0 == track->video.dimensions.height))
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width = track->video.dimensions.width;
	property->value.dimension.height = track->video.dimensions.height;

	return kFskErrNone;
}

/*
	local
*/

UInt32 aac_samplerate_table[16] =
{
	96000,	//0x0
	88200,	//0x1
	64000,	//0x2
	48000,	//0x3
	44100,	//0x4
	32000,	//0x5
	24000,	//0x6
	22050,	//0x7
	16000,	//0x8
	12000,	//0x9
	11025,	//0xa
	8000,	//0xb
	0,		//0xc		reserved
	0,		//0xd		reserved
	0,		//0xe		reserved
	0		//0xf		reserved
};

FskErr instantiateFLV(flvReader state)
{
	FskErr err = kFskErrNone;
	UInt8 header[9], bytes[12];
	FskInt64 totalSize;
	UInt32 timeStamp = 0;
	UInt32 audioFrames = 0, audioBitRate = 0;
	UInt32 videoFrames = 0, videoLastTime = 0;

	state->hasAudio = state->hasVideo = false;

	err = doRead(state, 0, 9, header);
	if (err) return err;

	if (('F' != header[0]) || ('L' != header[1]) || ('V' != header[2]) || (1 != header[3])) {
		err = kFskErrBadData;
		goto bail;
	}

	state->headerSize = FskMisaligned32_GetBtoN((header + 5));

	// look at the first part of the stream to see what's really there (header flags don't seem to be so accurate)
	state->fileOffset = state->headerSize + 4;

	while (true) {
		UInt32 dataSize;
		Boolean tagType, flags, AVCAACflags = 1;
		unsigned char *data;

		err = demuxNextDLV(state, &timeStamp, &tagType, &flags, &data, &dataSize, &AVCAACflags);
		if (err) {
			if (kFskErrEndOfFile == err)
				break;
			goto bail;
		}

		if (8 == tagType) {
			UInt8 codec = (flags & 0xf0) >> 4;

			if (false == state->hasAudio) {
				static const UInt32 sampleRates[] = {5500, 11025, 22050, 44100};

				if (1 == codec)
					state->audio.format = "x-audio-codec/adpcm";
				else if (2 == codec)
					state->audio.format = "x-audio-codec/mp3";
				else if ((4 == codec) || (5 == codec) || (6 == codec))
					state->audio.format = "x-audio-codec/nellymoser";
				else if (11 == codec)
					state->audio.format = "x-audio-codec/speex-wb";
				else if (3 == codec)
					state->audio.format = "x-audio-codec/pcm-16-le";
				else if (10 == codec)
					state->audio.format = "x-audio-codec/aac";
				state->audio.audio.codec = codec;

				if (10 != codec) {
					if (11 != codec)
						state->audio.audio.sampleRate = sampleRates[(flags & 0x0c) >> 2];
					else
						state->audio.audio.sampleRate = 16000;
					state->audio.audio.channelCount = (flags & 0x01) + 1;
				}
				else if (10 == codec && AVCAACflags == 0) {
					UInt32 smpling_frequency_index, channel_configuration;
					UInt32 esds_size = 43;

					state->audio.codecSpecificDataSize = esds_size;

					err = FskMemPtrNewClear(state->audio.codecSpecificDataSize, (FskMemPtr *)&state->audio.codecSpecificDataRef);
					if (err != kFskErrNone)
						return kFskErrMemFull;

					//AudioSpecificConfig
					smpling_frequency_index = ((data[0]&0x7)<<1) | ((data[1]>>7)&0x1);
					if (0xf == smpling_frequency_index)
					{
						state->audio.audio.sampleRate = smpling_frequency_index = ((data[1]&0x7f)<<17) | (data[2]<<9) | (data[3]<<1) | ((data[4]>>7)&0x1);
						state->audio.audio.channelCount = channel_configuration = (data[4]>>3)&0xf;
					}
					else 
						state->audio.audio.channelCount = channel_configuration = (data[1]>>3)&0xf;
					state->audio.audio.sampleRate = aac_samplerate_table[smpling_frequency_index];
                    state->audio.profile = 2;			//default
                    state->audio.level = 0xffffffff;	//invalid
					creat_aac_esds_config(state->audio.codecSpecificDataRef, smpling_frequency_index, channel_configuration);
					goto doNext;
				}

				state->hasAudio = true;
				state->audio.readerTrack.dispatch = &gFLVReaderAudioTrack;
				state->audio.readerTrack.state = &state->audio.readerTrack;
				state->audio.state = state;
			}

			if (2 == codec) {
				DIDLMusicItemRecord mi;

				if (parseMP3Header(data, &mi)) {
					audioFrames += 1;
					audioBitRate += mi.bitrate;
                    if(state->audio.audio.sampleRate != mi.frequency) {
                        FskMediaReaderFLVPrintfDebug( "update mp3 sample rate, old/new: %d, %d", state->audio.audio.sampleRate, mi.frequency );
                        state->audio.audio.sampleRate = mi.frequency;
                    }
				}
			}
            
 		}
		else
		if (9 == tagType) {
			if (false == state->hasVideo) {
				UInt8 codec = (flags & 0x0f);

				if (1 == codec) {                // this is a Kinoma extension, not part of the FLV format
                    FskImageDecompress dt;

                    if (kFskErrNone == FskImageDecompressNew(&dt, 0, "image/jpeg", NULL)) {
                        FskMediaPropertyValueRecord prop;

                        FskImageDecompressSetData(dt, data, dataSize);
                        if (kFskErrNone == FskImageDecompressGetMetaData(dt, kFskImageDecompressMetaDataDimensions, 0, &prop, NULL)) {
                            state->video.video.dimensions.height = prop.value.dimension.height;
                            state->video.video.dimensions.width = prop.value.dimension.width;

                            state->video.format = "image/jpeg";
                        }
                        
                        FskImageDecompressDispose(dt);
                    }
                }
                else
				if (2 == codec) {
					state->video.format = "x-video-codec/h263-flash";

					getDimensions(data, (UInt32 *)&state->video.video.dimensions.width, (UInt32 *)&state->video.video.dimensions.height);
				}
				else
				if (4 == codec)
					state->video.format = "x-video-codec/truemotion-vp6";
				else
				if (7 == codec) {
					state->video.format = "x-video-codec/avc";
					if (AVCAACflags == 0)
					{
						UInt32 const_avc1 = 'avc1', const_avcC = 'avcC';
						UInt8 *FormatInfo;
						UInt32 avcC_Size;
						QTImageDescriptionRecord *image_desc;
						int profile = 0, level = 0, sps_size;
						SInt32 width_clip, height_clip;
						UInt8 *spspps;

						if (0 == dataSize)
							return kFskErrUnimplemented;

						avcC_Size = dataSize + 8;
						state->video.codecSpecificDataSize = avcC_Size + sizeof(QTImageDescriptionRecord);

						err = FskMemPtrNewClear(state->video.codecSpecificDataSize, (FskMemPtr *)&state->video.codecSpecificDataRef);
						if (err != kFskErrNone)
							return kFskErrMemFull;

						err = FskMemPtrNewClear(dataSize, (FskMemPtr *)&spspps);
						if (err != kFskErrNone)
						{
							if (state->video.codecSpecificDataRef != NULL)
								FskMemPtrDispose(state->video.codecSpecificDataRef);
							return kFskErrMemFull;
						}

						sps_size = dataSize-6;
						FskMemCopy(spspps+2, data+6, sps_size); //add double 0 here!!

						sps_size = spspps[2]<<8|spspps[3];
						err = parse_avc_header( spspps+4, sps_size, (int *)&width_clip, (int *)&height_clip, &profile, &level );
                        FskMemPtrDispose(spspps);
						if (err)
							return kFskErrBadData;

						state->video.profile = profile;
						state->video.level   = level;
						state->video.video.dimensions.width = width_clip;
						state->video.video.dimensions.height = height_clip;

						FormatInfo = state->video.codecSpecificDataRef;
						//QTDR
						FskMemCopy(FormatInfo+0, &state->video.codecSpecificDataSize, 4);
						FskMemCopy(FormatInfo+4, &const_avc1, 4);
						
						//avcC
						FormatInfo += sizeof(QTImageDescriptionRecord);
						FskMemCopy(FormatInfo+0, &avcC_Size, 4);
						FskMemCopy(FormatInfo+4, &const_avcC, 4);
						FskMemCopy(FormatInfo+8, data, dataSize);

						image_desc = (QTImageDescriptionRecord *)state->video.codecSpecificDataRef;
						//set extra info
						//image_desc->idSize;						//83 00 00 00
						//image_desc->cType;						//31 63 76 61
						//image_desc->resvd1;						//00 00 00 00
						//image_desc->resvd2;						//00 00
						image_desc->dataRefIndex	= 1;			//01 00
						image_desc->version			= 0;			//00 00 
						image_desc->revisionLevel	= 0;			//00 00 
						image_desc->vendor			= 0;			//00 00 00 00
						image_desc->temporalQuality = 0x00000200;	//00 02 00 00
						image_desc->spatialQuality  = 0x00000200;	//00 02 00 00
						image_desc->width			= (UInt16)state->video.video.dimensions.width;
						image_desc->height			= (UInt16)state->video.video.dimensions.height;
						image_desc->hRes			= 0x00480000;	//00 00 48 00 
						image_desc->vRes			= 0x00480000;	//00 00 48 00 
						image_desc->dataSize		= 0;			//00 00 00 00
						image_desc->frameCount		= 1;			//01 00 
						//image_desc->name[32];	
						image_desc->depth			= 0x0018;		//18 00
						image_desc->clutID			= 0xffff;		//ff ff
						goto doNext;
					}
				}

				state->hasVideo = true;
				state->video.readerTrack.dispatch = &gFLVReaderVideoTrack;
				state->video.readerTrack.state = &state->video.readerTrack;
				state->video.state = state;
			}

			videoFrames += 1;
			videoLastTime = timeStamp;
		}
		else
		if (18 == tagType)
			scanMetadata(state, data, data + dataSize);

		FskMemPtrDispose(data);

doNext:
        if (state->hasVideo && state->hasAudio)
            break;

		if (timeStamp >= 1250)			// depend on both audio & video streams to show a frame in the first 1.25 seconds
			break;
	}

	// calculate duration by looking at the last time stamp in the file
	err = (state->spooler->doGetSize)(state->spooler, &totalSize);
	BAIL_IF_ERR(err);

	totalSize -= 4;
	state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;
	err = doRead(state, totalSize, 4, bytes);
	state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
	if (kFskErrNone == err) {
		totalSize -= FskMisaligned32_GetBtoN(bytes);
		err = doRead(state, totalSize, 12, bytes);
		BAIL_IF_ERR(err);

		bytes[3] = 0;
		state->duration = FskMisaligned32_GetBtoN(&bytes[3]);
	}
	else
	if (kFskErrNeedMoreTime == err) {
		if (0 == state->duration)                   // non-zero if we got the duration in the metadata
			state->duration = kFLVLiveDuration;		// treat it as live

		err = kFskErrNone;
	}
	else
		goto bail;

	// set-up seek table
	if (0 == state->seekTableLength) {
		state->seekTableInterval = 1 * 1000;

		if ((0 != state->duration) && (kFLVLiveDuration != state->duration))
			err = growSeekTable(state, (state->duration / state->seekTableInterval) + 10);		// +10 to have a little slop at the end to avoid a surprise grow when reaching the end
		else
			err = growSeekTable(state, ((15 * 60) / state->seekTableInterval) + 10);		// +10 to have a little slop at the end to avoid a surprise grow when reaching the end
		BAIL_IF_ERR(err);
		
		state->seekTable[0] = state->headerSize + 4;
	}

	if ((0 == state->audio.bitRate) && (0 != audioFrames))
		state->audio.bitRate = (audioBitRate * 1000) / audioFrames;

	if ((0 != videoLastTime) && (videoFrames > 1))
		state->video.video.frameRate = (double)(videoFrames - 1) / ((double)videoLastTime / 1000.0);

	// set position to start, so we're correctly positioned for live case
	state->fileOffset = state->headerSize + 4;

	// if we are streaming, don't have a seek table or time seeking server, force download mode
	if (!(kFskMediaSpoolerCantSeek & state->spooler->flags) && (kFskMediaSpoolerIsNetwork & state->spooler->flags) && !state->flvSeeking && !(kFskMediaSpoolerTimeSeekSupported & state->spooler->flags) && (kFLVLiveDuration != state->duration)) {
		// re-open for download
		state->http.cantSeek = true;

		if (state->spoolerOpen && state->spooler->doClose) {
			(state->spooler->doClose)(state->spooler);
			state->spoolerOpen = false;
		}

		err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

bail:
	return err;
}

FskErr demuxNextDLV(flvReader state, UInt32 *timeStampOut, UInt8 *tagTypeOut, UInt8 *tagFlagsOut, unsigned char **dataOut, UInt32 *dataSizeOut,  UInt8 *AVCFlagsOut)
{
	FskErr err = kFskErrNone;
#define TAG_HEADER_SIZE 16
	while (true) {
		UInt8 tagHeader[TAG_HEADER_SIZE];
		UInt8 tagType, tagFlags;
		UInt32 tagBodySize, timeStamp;
		UInt8 AVCAACPacketType = 1;
		UInt32 VideoisAVC = 0, AudioisAAC = 0;

		err = doRead(state, state->fileOffset, TAG_HEADER_SIZE, tagHeader);
		BAIL_IF_ERR(err);

		tagType = tagHeader[0];
		tagBodySize = ((UInt32)tagHeader[1] << 16) | ((UInt32)tagHeader[2] << 8) | (UInt32)tagHeader[3];
		timeStamp = ((UInt32)tagHeader[4] << 16) | ((UInt32)tagHeader[5] << 8) | (UInt32)tagHeader[6];
		tagFlags = tagHeader[11];

		if (9 == tagType && (7 == (tagFlags&0xf))) //AVC video
		{
			VideoisAVC = 1;
			AVCAACPacketType = tagHeader[12];
		}
		else if (8 == tagType && (0xa0 == (tagFlags&0xf0))) //AAC audio
		{
			AudioisAAC = 1;
			AVCAACPacketType = tagHeader[12];
		}
        else if (18 == tagType) {
            err = kFskErrNone;
        }

		if ((8 != tagType) && (9 != tagType) && (18 != tagType)) {
			// unknown tag type
			state->fileOffset += 15 + tagBodySize;
			continue;
		}

		if (0 == tagBodySize) {
			state->fileOffset += 15 + tagBodySize;
			continue;
		}

		if (NULL != dataOut) {
			err = FskMemPtrNew(tagBodySize - 1 + kFskDecompressorSlop, (FskMemPtr *)dataOut);
			BAIL_IF_ERR(err);
			
			if (1 == VideoisAVC)
			{
				err = doRead(state, state->fileOffset + 16, tagBodySize - 5, *dataOut);
			}
			else if (1 == AudioisAAC)
			{
				err = doRead(state, state->fileOffset + 13, tagBodySize - 2, *dataOut);
			}
			else
				err = doRead(state, state->fileOffset + 12, tagBodySize - 1, *dataOut);

			if (err) {
				FskMemPtrDisposeAt((void **)dataOut);
				goto bail;
			}
		}

		if (NULL != dataSizeOut)
			*dataSizeOut = VideoisAVC == 1 ? tagBodySize - 5 : (AudioisAAC == 1 ? tagBodySize - 2 : tagBodySize - 1);

		if (timeStampOut)
			*timeStampOut = timeStamp;

		if (tagTypeOut)
			*tagTypeOut = tagType;

		if (tagFlagsOut)
			*tagFlagsOut = tagFlags;

		if (AVCFlagsOut)
			*AVCFlagsOut = AVCAACPacketType;

		// update seek table
		if (0 != state->seekTableInterval) {
			Boolean indexIt;
			if (state->hasVideo)
				indexIt = (9 == tagType) && (1 == ((tagFlags & 0xf0) >> 4));
			else
				indexIt = (8 == tagType);
			if (indexIt)
				setSeekTableIndex(state, timeStamp, (UInt32)state->fileOffset);
		}

		state->fileOffset += 15 + tagBodySize;

		break;
	}

bail:
	return err;
}


typedef struct {
	unsigned char *data;
	int position;
} bitScanRecord, *bitScan;

static unsigned long getBits(bitScan bits, int count);

unsigned long getBits(bitScan bits, int count)
{
	unsigned long result = 0;

	while (count--) {
		result <<= 1;
		if (*(bits->data) & (1L << (7 - bits->position)))
			result |= 1;
		bits->position += 1;
		if (8 == bits->position) {
			bits->position = 0;
			bits->data += 1;
		}
	}

	return result;
}

void getDimensions(unsigned char *bits, UInt32 *width, UInt32 *height)
{
	bitScanRecord scan;
	unsigned long flvMarker;

	scan.data = bits;
	scan.position = 0;

	(void)getBits(&scan, 17);       // start code
	flvMarker = getBits(&scan, 5) + 1;
	if (flvMarker <= 1) {
		// not flash
	}
	else {
		unsigned long format;
		getBits(&scan, 8);		//temporalReference
		format = getBits(&scan, 3);
		switch (format) {
			case 0:
				*width = getBits(&scan, 8);
				*height = getBits(&scan, 8);
				break;

			case 1:
				*width = getBits(&scan, 16);
				*height = getBits(&scan, 16);
				break;

			case 2:
				*width = 352;
				*height = 288;
				break;

			case 3:
				*width = 176;
				*height = 144;
				break;

			case 4:
				*width = 128;
				*height = 96;
				break;

			case 5:
				*width = 320;
				*height = 240;
				break;

			case 6:
				*width = 160;
				*height = 120;
				break;

			case 7:
				*width = 0;
				*height = 0;
				break;
		}
	}
}


enum {
	kFlvMetaTypeUnknown = 0,
	kFlvMetaTypeRoot,
	kFlvMetaTypeDouble,
	kFlvMetaTypeBoolean,
	kFlvMetaTypeString,
	kFlvMetaTypeObject,
	kFlvMetaTypeArray
};

typedef struct FskFlvMetaItemRecord FskFlvMetaItemRecord;
typedef struct FskFlvMetaItemRecord *FskFlvMetaItem;

typedef union {
	Boolean			b;
	double			number;
	char			*str;

	FskFlvMetaItem	object;

	struct {
		UInt32						length;
		void						*values;		// array of FskFlvMetaItem (only value field used though)
	} anArray;
} FskFlvMetaItemValue;

struct FskFlvMetaItemRecord {
	FskFlvMetaItem		next;

	UInt8				type;
	char				*name;
	FskFlvMetaItemValue	value;

	char				data[1];
};

static	FskFlvMetaItem findMetadata(FskFlvMetaItem root, char *path);
static void metadataDispose(FskFlvMetaItem item, Boolean disposeItem);
static unsigned char *parseOneMetadata(flvReader state, unsigned char *meta, unsigned char *metaEnd, FskFlvMetaItem container);

void scanMetadata(flvReader state, unsigned char *meta, unsigned char *metaEnd)
{
	FskFlvMetaItem root, duration, keyframeTimes, keyframeFilePositions, dataRate, canseekontime;
	FskFlvMetaItem title, artist, album;

	if ((metaEnd - meta) >= 12) {
		if ((0 == meta[0]) && (10 == meta[1]) && (0 == FskStrCompareWithLength((char *)(meta + 2), "onMetaData", 10)))
			meta += 12;
	}

	FskMemPtrNewClear(sizeof(FskFlvMetaItemRecord) + FskStrLen("onMetaData") + 1, &root);
	root->type = kFlvMetaTypeRoot;
	root->name = root->data;
	FskStrCopy(root->name, "onMetaData");

	parseOneMetadata(state, meta, metaEnd, root);

	duration = findMetadata(root, "duration");
	if (duration && (kFlvMetaTypeDouble == duration->type)) {
		if (0 == state->duration)
			state->duration = (UInt32)(duration->value.number * state->scale);
	}

	dataRate = findMetadata(root, "videodatarate");
	if (dataRate && (kFlvMetaTypeDouble == dataRate->type))
		state->video.bitRate = (UInt32)(dataRate->value.number * 1000.0);

	dataRate = findMetadata(root, "audiodatarate");
	if (dataRate && (kFlvMetaTypeDouble == dataRate->type))
		state->audio.bitRate = (UInt32)(dataRate->value.number * 1000.0);

	if (kFLVParseNone != state->http.parseState) {
		canseekontime = findMetadata(root, "canseekontime");
		if (canseekontime && (kFlvMetaTypeBoolean == canseekontime->type)) {
			state->canSeekOnTime = canseekontime->value.b;

			if (state->canSeekOnTime)
				state->flvSeeking = true;
		}
	}

    // refresh metadata
    title = findMetadata(root, "FullName");
    artist = findMetadata(root, "Artist");
    album = findMetadata(root, "Album");
    if (title || artist || album) {
        FskMediaPropertyValueRecord prop;
        prop.type = kFskMediaPropertyTypeString;

        FskMediaMetaDataDispose(state->meta);
        FskMediaMetaDataNew(&state->meta);
        if (title && (kFlvMetaTypeString == title->type)) {
            prop.value.str = title->value.str;
            FskMediaMetaDataAdd(state->meta, "FullName", NULL, &prop, 0);
        }
        if (artist && (kFlvMetaTypeString == artist->type)) {
            prop.value.str = artist->value.str;
            FskMediaMetaDataAdd(state->meta, "Artist", NULL, &prop, 0);
        }
        if (album && (kFlvMetaTypeString == album->type)) {
            prop.value.str = album->value.str;
            FskMediaMetaDataAdd(state->meta, "Album", NULL, &prop, 0);
        }

        if (state->reader->mediaState >= kFskMediaPlayerStateStopped)
            FskMediaReaderSendEvent(state->reader, kFskEventMediaPlayerMetaDataChanged);
    }
    
	keyframeTimes = findMetadata(root, "keyframes.times");
	keyframeFilePositions = findMetadata(root, "keyframes.filepositions");
	if (keyframeTimes && keyframeFilePositions && (kFlvMetaTypeArray == keyframeTimes->type) && (kFlvMetaTypeArray == keyframeFilePositions->type) && (keyframeFilePositions->value.anArray.length == keyframeTimes->value.anArray.length)) {
		UInt32 i;

		state->seekTableInterval = 1 * 1000;

		for (i = 0; i < keyframeTimes->value.anArray.length; i++) {
			FskFlvMetaItem time = i + (FskFlvMetaItem)keyframeTimes->value.anArray.values;
			FskFlvMetaItem position = i + (FskFlvMetaItem)keyframeFilePositions->value.anArray.values;
			if ((kFlvMetaTypeDouble == time->type) && (kFlvMetaTypeDouble == position->type))
				setSeekTableIndex(state, (UInt32)(time->value.number * 1000), (UInt32)position->value.number);
		}

		state->flvSeeking = true;
	}

	metadataDispose(root, true);
}

unsigned char *parseOneMetadata(flvReader state, unsigned char *meta, unsigned char *metaEnd, FskFlvMetaItem container)
{
	unsigned char type = *meta++;

	switch (type) {
		case 0:	{ // double, big endian
			double d;

#if TARGET_RT_LITTLE_ENDIAN
			unsigned char *dst = (unsigned char *)&d + 7, *src = meta;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst-- = *src++;
			*dst = *src++;
#else
			FskMemMove(&d, meta, 8);
#endif
			meta += 8;

			container->type = kFlvMetaTypeDouble;
			container->value.number = d;
			}
			break;
			
		case 1:	// boolean
			container->type = kFlvMetaTypeBoolean;
			container->value.b = *meta;
			meta += 1;
			break;

		case 2: { //string
            UInt16 nameLen = FskMisaligned16_GetBtoN(meta);
			container->type = kFlvMetaTypeString;
			FskMemPtrNewClear(nameLen + 1, &container->value.str);
			FskMemMove(container->value.str, meta + 2, nameLen);

			meta += nameLen + 2;
            }
			break;

		case 3:	// object (?) - name/value pairs without length
			container->type = kFlvMetaTypeObject;
			while (meta < metaEnd) {
				FskFlvMetaItem entry;
				UInt32 nameLen = meta[1];       //@@ endian?
				unsigned char *name = meta + 2;
				meta = name;
				if ((0 == nameLen) && (0 == meta[0]))
					break;	//@@ need test case for this
				meta += nameLen;

				FskMemPtrNewClear(sizeof(FskFlvMetaItemRecord) + nameLen + 1, &entry);
				entry->name = entry->data;
				FskStrNCopy(entry->name, (char *)name, nameLen);

				meta = parseOneMetadata(state, meta, metaEnd, entry);

				FskListAppend(&container->value.object, entry);
			}
			break;

		case 8:	{ // associative array (name/value pairs)
			UInt32 length = FskMisaligned32_GetBtoN(meta);
			meta += 4;
			container->type = kFlvMetaTypeObject;
			while ((meta < metaEnd) && (length > 0)) {
				FskFlvMetaItem entry;
				UInt32 nameLen = FskMisaligned16_GetBtoN(meta);
				unsigned char *name = &meta[2];

				FskMemPtrNewClear(sizeof(FskFlvMetaItemRecord) + nameLen + 1, &entry);
				entry->name = entry->data;
				FskStrNCopy(entry->name, (char *)name, nameLen);

				meta = parseOneMetadata(state, meta + nameLen + 2, metaEnd, entry);
				length -= 1;

				FskListAppend(&container->value.object, entry);
			}
			}
			break;

		case 10: {	// array
			UInt32 length = FskMisaligned32_GetBtoN(meta), i  = 0;
			meta += 4;
			FskMemPtrNewClear(sizeof(FskFlvMetaItemRecord) * length, &container->value.anArray.values);
			container->type = kFlvMetaTypeArray;
			container->value.anArray.length = length;
			while ((meta < metaEnd) && (length > 0)) {
				meta = parseOneMetadata(state, meta, metaEnd, ((FskFlvMetaItem)container->value.anArray.values) + i);
				length -= 1;
				i += 1;
			}
			}
			break;

		default:	// unknown, surrender
			meta = metaEnd;
	}

	return meta;
}

FskFlvMetaItem findMetadata(FskFlvMetaItem root, char *path)
{
	while (*path) {
		char *dot = FskStrChr(path, '.');
		UInt32 len;
		FskFlvMetaItem walker;

		if (kFlvMetaTypeObject != root->type)
			return NULL;

		if (NULL == dot)
			dot = path + FskStrLen(path);
		len = dot - path;

		for (walker = root->value.object; NULL != walker; walker = walker->next) {
			if ((FskStrLen(walker->name) == len) && (0 == FskStrCompareWithLength(path, walker->name, len)))
				break;;
		}

		if (NULL == walker)
			return NULL;

		root = walker;

		path = dot;
		if (0 == *path)
			return root;			// done!

		path += 1;
	}

	return NULL;
}

void metadataDispose(FskFlvMetaItem item, Boolean disposeItem)
{
	switch (item->type) {
		case kFlvMetaTypeRoot:
		case kFlvMetaTypeDouble:
		case kFlvMetaTypeBoolean:
			break;

		case kFlvMetaTypeString:
			FskMemPtrDispose(item->value.str);
			break;

		case kFlvMetaTypeObject:
			while (item->value.object) {
				void *next = item->value.object->next;
				metadataDispose(item->value.object, true);
				item->value.object = next;
			}
			break;

		case kFlvMetaTypeArray: {
			UInt32 i;

			for (i = 0; i < item->value.anArray.length; i++)
				metadataDispose(((FskFlvMetaItem)item->value.anArray.values) + i, false);

			FskMemPtrDispose(item->value.anArray.values);
			}
			break;
	}

	if (disposeItem)
		FskMemPtrDispose(item);
}


FskErr growSeekTable(flvReader state, UInt32 length)
{
	FskErr err;
	UInt32 *seekTable;

	err = FskMemPtrNew(sizeof(UInt32) * (length + 1), &seekTable);
	BAIL_IF_ERR(err);

	if (0 != state->seekTableLength)
		FskMemMove(seekTable, state->seekTable, sizeof(UInt32) * state->seekTableLength);
	FskMemSet(seekTable + state->seekTableLength, 0, (length - state->seekTableLength) * sizeof(UInt32));
	FskMemPtrDispose(state->seekTable);
	state->seekTable = seekTable;
	state->seekTableLength = length;

bail:
	return err;
}

void setSeekTableIndex(flvReader state, UInt32 timeStamp, UInt32 value)
{
	SInt32 index = (timeStamp + state->seekTableInterval - 1) / state->seekTableInterval;

	if (index >= state->seekTableLength) {
		if (kFskErrNone != growSeekTable(state, index + (index / 10)))
			return;
	}

	if (0 != state->seekTable[index])
		return;

	state->seekTable[index] = value;
}

FskErr doRead(flvReader state, FskInt64 offset, UInt32 size, void *bufferIn)
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

FskErr flvInstantiate(flvReader state)
{
	FskErr err;

	err = instantiateFLV(state);
	BAIL_IF_ERR(err);

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}

FskErr flvSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	flvReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = flvInstantiate(state);
			break;

		case kFskMediaSpoolerOperationGetHeaders: {
			FskHeaders *headers = param;
			char *value = FskHeaderFind("Server", headers);
			if ((NULL != value) && (0 == FskStrCompareWithLength("Orbiter", value, 7)))
				state->http.cantSeek = true;

			if (state->http.cantSeek)
				state->spooler->flags |= kFskMediaSpoolerCantSeek;

            value = FskHeaderFind("availableSeekRange.dlna.org", headers);
            if (value && (0 == FskStrCompareWithLength("1 npt=", value, 6))) {
                char *dash = FskStrChr(value + 6, '-');
                double duration;
                if (dash && (kFskErrNone == FskMediaParseNPT(dash + 1, &duration)))
                    state->duration = (UInt32)(duration * state->scale);
            }

			state->spooler->flags |= kFskMediaSpoolerTransformReceivedData;		// this is unnecessary when not downloading, but difficult to set later...

			state->http.parseState = kFLVParseInitialize;
			}
			break;

		case kFskMediaSpoolerOperationGetURI:
			if (state->flvSeeking) {
				if (!state->canSeekOnTime) {
					FskMediaSpoolerGetURI getURI = param;
					char position[64];
					FskStrCopy(position, "&start=");
					FskStrNumToStr((UInt32)getURI->position, position + FskStrLen(position), sizeof(position) - FskStrLen(position));
					getURI->uriOut = FskStrDoCat(getURI->uriIn, position);
					getURI->doSetPosition = false;

	//@@				state->spooler->flags |= kFskMediaSpoolerTransformReceivedData;
					state->bytesToSkip = 13;		// optimally this would be calculated from the data stream
				}
				else {
					FskMediaSpoolerGetURI getURI = param;
					char begin[64];		// time in milliseconds
					FskStrCopy(begin, "&begin=");
					FskStrNumToStr(state->atTime, begin + FskStrLen(begin), sizeof(begin) - FskStrLen(begin));
					getURI->uriOut = FskStrDoCat(getURI->uriIn, begin);
					getURI->doSetPosition = false;

	//@@				state->spooler->flags |= kFskMediaSpoolerTransformReceivedData;
					state->bytesToSkip = 13;		// optimally this would be calculated from the data stream
				}
			}
			else {
//@@				state->spooler->flags &= ~kFskMediaSpoolerTransformReceivedData;
				FskMediaSpoolerGetURI getURI = param;

				// if backing up to near the start of the file, just go to the start to accomodate servers that don't seek (including YouTube clips encoded without onMetaData key frame list)
				if (getURI->position < 4096) {
					getURI->doSetPosition = false;		// go to start of the file
					state->bytesToSkip = (UInt32)(getURI->position);	/* 4294967295 bytes max */
				}
				else
					state->bytesToSkip = 0;
			}
			break;

		case kFskMediaSpoolerOperationTransformData: {
			FskMediaSpoolerTransformData xform = param;
			if (state->bytesToSkip <= xform->dataSize) {
				xform->data += state->bytesToSkip;
				xform->dataSize -= state->bytesToSkip;
				state->bytesToSkip = 0;
//@@				state->spooler->flags &= ~kFskMediaSpoolerTransformReceivedData;
			}
			else {
				state->bytesToSkip -= xform->dataSize;
				xform->dataSize = 0;
			}

			if (kFskMediaSpoolerDownloading & state->spooler->flags) {
				UInt32 bufferSize = xform->dataSize;
				unsigned char *buffer = (unsigned char*)xform->data;

				while (bufferSize) {
					switch (state->http.parseState) {
						case kFLVParseInitialize:
							state->http.parseState = kFLVParseCollect;
							state->http.bytesToCollect = 9;
							state->http.collectionBuffer = state->http.buffer;
							state->http.nextParseState = kFLVParseFileHeader;
							state->http.maximumDownloadedTime = 0;
							break;

						case kFLVParseFileHeader:
							state->http.parseState = kFLVParseFrameHeaderStart;
							break;

						case kFLVParseFrameHeaderStart:
							state->http.parseState = kFLVParseCollect;
							state->http.bytesToCollect = 16;
							state->http.collectionBuffer = state->http.buffer;
							state->http.nextParseState = kFLVParseFrameHeaderDone;
							break;

						case kFLVParseFrameHeaderDone: {
							const UInt32 kBufferInterval = 1500;
							unsigned char *tagHeader = state->http.buffer;
//							UInt8 tagType = tagHeader[4];
							UInt32 tagBodySize = ((UInt32)tagHeader[5] << 16) | ((UInt32)tagHeader[6] << 8) | (UInt32)tagHeader[7];
							UInt32 timeStamp = ((UInt32)tagHeader[8] << 16) | ((UInt32)tagHeader[9] << 8) | (UInt32)tagHeader[10];

							if (timeStamp > kBufferInterval)
								state->http.maximumDownloadedTime = timeStamp - kBufferInterval;

							state->http.parseState = kFLVParseSkip;
							state->http.bytesToSkip = tagBodySize - 1;
							state->http.nextParseState = kFLVParseFrameHeaderStart;
							}
							break;

						case kFLVParseCollect: {
							UInt32 bytesToCopy = state->http.bytesToCollect;
							if (bytesToCopy > bufferSize)
								bytesToCopy = bufferSize;
							FskMemMove(state->http.collectionBuffer, buffer, bytesToCopy);
							bufferSize -= bytesToCopy;
							buffer += bytesToCopy;
							state->http.collectionBuffer += bytesToCopy;
							state->http.bytesToCollect -= bytesToCopy;
							state->http.bytesParsed += bytesToCopy;
							if (0 == state->http.bytesToCollect)
								state->http.parseState = state->http.nextParseState;
							}
							break;

						case kFLVParseSkip: {
							UInt32 bytesToSkip = state->http.bytesToSkip;
							if (bytesToSkip > bufferSize)
								bytesToSkip = bufferSize;
							bufferSize -= bytesToSkip;
							buffer += bytesToSkip;
							state->http.bytesToSkip -= bytesToSkip;
							state->http.bytesParsed += bytesToSkip;
							if (0 == state->http.bytesToSkip)
								state->http.parseState = state->http.nextParseState;
							}
							break;
					}
				}
			}
			}
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

static void creat_aac_esds_config(UInt8 *config, UInt32 frequency, UInt32 channel)
{
	UInt8 *p = config;

	*p = 0;					p++;		//version
	*p = 0;					p++;		//flags
	*p = 0;					p++;		
	*p = 0;					p++;		

	//esd starts here
	*p = 0x03;				p++;		//tag
	*p = 0x80;				p++;		//size
	*p = 0x80;				p++;		//size
	*p = 0x80;				p++;		//size
	*p = 0x22;				p++;		//size

	*p = 0;					p++;		//ES_ID
	*p = 0;					p++;		
	*p = 0;					p++;		//flags&priority: 

	//decoder config descriptor
	*p = 0x04;				p++;		//object type
	*p = 0x80;				p++;		//size
	*p = 0x80;				p++;		//size
	*p = 0x80;				p++;		//size
	*p = 0x14;				p++;		//size

	*p = 0x40;			p++;		//objectTypeId
	*p = 0x15;			p++;		//streamType

	*p = 0;				p++;		//bufferSizeDB, 24 bits: 0x1800
	*p = 0x18;			p++;		
	*p = 0;				p++;		

	*p = 0;				p++;		//maxBitRate, 32 bits:
	*p = 0x01;			p++;		
	*p = 0xf4;			p++;		
	*p = 0;				p++;		

	*p = 0;				p++;		//avgBitRate, 32 bits:
	*p = 0x01;			p++;		
	*p = 0xf4;			p++;		
	*p = 0;				p++;		

	//decoder specific info
	*p = 0x05;			p++;		//tag
	*p = 0x80;			p++;		//size
	*p = 0x80;			p++;		//size
	*p = 0x80;			p++;		//size
	*p = 0x02;			p++;		//size

	*p = (unsigned char )(0x10 |(frequency>>1));
	p++;//0x12;			p++;		//decoderConfig
	*p = (unsigned char )(((frequency&0x01)<<7)| ((channel&0x0f)<<3));
	p++;//0x10;			p++;

	//decoder specific info
	*p = 0x06;			p++;		//tag
	*p = 0x80;			p++;		//size
	*p = 0x80;			p++;		//size
	*p = 0x80;			p++;		//size
	*p = 0x01;			p++;		//size
	*p = 0x02;			p++;		//predefined: use timestamps from mp4 stbl

	//no IPI_DescrPointer
	//no IP_IdentificationDataSet
	//no IPMP_DescriptorPointer
	//no LanguageDescriptor
	//no QoS_Descriptor
	//no RegistrationDescriptor
	//no ExtensionDescriptor

	return;
}
