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
#define __FSKMUXER_PRIV__
#include "FskAudio.h"
#include "FskEndian.h"
#include "FskExtensions.h"
#include "FskMuxer.h"
#include "FskUtilities.h"

static Boolean flvMuxerCanHandle(const char *mimeType);
static FskErr flvMuxerNew(FskMuxer muxer, void **muxerState);
static FskErr flvMuxerDispose(FskMuxer muxer, void *muxerState);
static FskErr flvMuxerStart(FskMuxer muxer, void *muxerState);
static FskErr flvMuxerStop(FskMuxer muxer, void *stateIn);
static FskErr flvMuxerSetMetaData(FskMuxer muxer, void *muxerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 flags);
static FskErr flvMuxerNewTrack(FskMuxer muxer, void *muxerState, FskMuxerTrack track, FskMuxerTrackDispatch *dispatch, void **trackState);

FskMediaPropertyEntryRecord gFLVMuxerProperties[] = {
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMuxerDispatchRecord gFLVMuxer = {flvMuxerCanHandle, flvMuxerNew, flvMuxerDispose, flvMuxerStart, flvMuxerStop, flvMuxerSetMetaData, flvMuxerNewTrack, gFLVMuxerProperties};

FskExport(FskErr) flvmuxer_fskLoad(FskLibrary library)
{
	FskExtensionInstall(kFskExtensionMuxer, &gFLVMuxer);
	return kFskErrNone;
}

FskExport(FskErr) flvmuxer_fskUnload(FskLibrary library)
{
	FskExtensionUninstall(kFskExtensionMuxer, &gFLVMuxer);
	return kFskErrNone;
}

// muxer

typedef struct flvMuxerRecord flvMuxerRecord;
typedef struct flvMuxerRecord *flvMuxer;

typedef struct {
	FskMuxerTrack					muxerTrack;
	FskMuxer						muxer;
	struct flvMuxerRecord			*state;
    Boolean                         isSound;
    Boolean                         isVideo;
} flvMuxerTrackRecord, *flvMuxerTrack;

struct flvMuxerRecord {
	FskMuxer					muxer;

	UInt32						sampleRate;
	SInt32						channelCount;
	UInt32						audioFormat;
    UInt32                      aacSampleRateIndex;

	UInt8						audioFormatBits;
	UInt8						sampleRateBits;
	UInt8						sizeBits;
	unsigned char				audioHeader[12];
	UInt32						audioSamples;
	UInt32						audioTime;

    FskMuxerTrack               soundTrack;
    FskMuxerTrack               videoTrack;
    
    UInt8                       videoFormat;
	unsigned char				videoHeader[12];
	UInt32						videoTime;
    
	FskMediaMetaData			meta;
    Boolean                     metaChanged;

	FskFileOffset				nextOffset;
};

// track

static FskErr flvMuxerTrackDispose(FskMuxerTrack track, void *trackState);
static FskErr flvMuxerTrackAdd(FskMuxerTrack track, void *trackState, const void *data, UInt32 infoCount, FskMuxerSampleInfo info);

static FskErr flvMuxerTrackSetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvMuxerTrackSetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvMuxerTrackSetFormatInfo(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr flvMuxerTrackSetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

static FskErr doAppend(flvMuxer state, const void *data, UInt32 size);

FskMediaPropertyEntryRecord gFLVMuxerSoundTrackProperties[] = {
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeFloat,			NULL,								flvMuxerTrackSetSampleRate},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		NULL,								flvMuxerTrackSetFormat},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		NULL,								flvMuxerTrackSetChannelCount},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaPropertyEntryRecord gFLVMuxerVideoTrackProperties[] = {
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		NULL,								flvMuxerTrackSetFormat},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,		NULL,								flvMuxerTrackSetFormatInfo},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMuxerTrackDispatchRecord gFLVMuxerSoundTrack = {flvMuxerTrackDispose, flvMuxerTrackAdd, gFLVMuxerSoundTrackProperties};

FskMuxerTrackDispatchRecord gFLVMuxerVideoTrack = {flvMuxerTrackDispose, flvMuxerTrackAdd, gFLVMuxerVideoTrackProperties};

/*
	muxer functions
*/

Boolean flvMuxerCanHandle(const char *mimeType)
{
	return	(0 == FskStrCompareCaseInsensitive("video/flv", mimeType)) || (0 == FskStrCompareCaseInsensitive("audio/flv", mimeType));
}

FskErr flvMuxerNew(FskMuxer muxer, void **muxerState)
{
	FskErr err;
	flvMuxer state;

	err = FskMemPtrNewClear(sizeof(flvMuxerRecord), (FskMemPtr *)&state);
	BAIL_IF_ERR(err);

	state->muxer = muxer;

	err = FskMediaMetaDataNew(&state->meta);
	BAIL_IF_ERR(err);

bail:
	if (err) {
		flvMuxerDispose(muxer, state);
		state = NULL;
	}
	*muxerState = state;

	return err;
}

FskErr flvMuxerDispose(FskMuxer muxer, void *stateIn)
{
	flvMuxer state = stateIn;

	FskMediaMetaDataDispose(state->meta);
    
	FskMemPtrDispose(state);

	return kFskErrNone;
}

FskErr flvMuxerStart(FskMuxer muxer, void *stateIn)
{
	flvMuxer state = stateIn;
	FskErr err;
	unsigned char header[9] = {'F', 'L', 'V', 1, 0, 0, 0, 0, 9};
	UInt32 zero = 0;

#if 0
	if ((0.0 == state->sampleRate) || (0 == state->channelCount) || (kFskAudioFormatUndefined == state->audioFormat))
		return kFskErrInvalidParameter;
#endif

	if (kFskAudioFormatUndefined != state->audioFormat) {
		header[4] |= 4;
		state->audioHeader[0] = 8;
        if (10 != state->audioFormatBits)
            state->audioHeader[11] = (UInt8)((state->audioFormatBits << 4) | (state->sampleRateBits << 2) | (state->sizeBits << 1) | (state->channelCount - 1));
        else
            state->audioHeader[11] = (UInt8)((state->audioFormatBits << 4) | (3 << 2) | (state->sizeBits << 1) | (2 - 1));      // AAC
	}

	if (0 != state->videoFormat) {
		header[4] |= 1;
		state->videoHeader[0] = 9;
		state->videoHeader[11] = (1 << 4) | state->videoFormat;
    }

	state->nextOffset = 0;
	state->audioTime = 0;
	state->audioSamples = 0;
	state->videoTime = 0;

	err = doAppend(state, header, sizeof(header));
	BAIL_IF_ERR(err);

	// size of previous tag (0!)
	err = doAppend(state, &zero, sizeof(zero));
	BAIL_IF_ERR(err);
    
    if (kFskAudioFormatAAC == state->audioFormat) {
        unsigned char header[12];
        UInt32 dataSize = 2 + 1;
        unsigned char packetType = 0;       // sequence header
        unsigned char packet[2];
        UInt32 s32;

        FskMemMove(header, state->audioHeader, sizeof(header));
        header[1] = (UInt8)(((dataSize + 1) >> 16) & 0xff);
        header[2] = (UInt8)(((dataSize + 1) >>  8) & 0xff);
        header[3] = (UInt8)((dataSize + 1) & 0xff);
        header[4] = (UInt8)((0 >> 16) & 0xff);
        header[5] = (UInt8)((0 >>  8) & 0xff);
        header[6] = (UInt8)((0 >>  0) & 0xff);

        err = doAppend(state, &header, sizeof(header));
        BAIL_IF_ERR(err);

        err = doAppend(state, &packetType, sizeof(packetType));
        BAIL_IF_ERR(err);

        packet[0] = packet[1] = 0;
        packet[0] |= (state->aacSampleRateIndex >> 1) & 0x07;
        packet[1] |= (state->aacSampleRateIndex << 7) & 0x80;
        packet[1] |= (state->channelCount << 3) & 0x78;
        
        err = doAppend(state, &packet, sizeof(packet));
        BAIL_IF_ERR(err);

        // write previous tag size
        s32 = sizeof(header) + dataSize;
        s32 = FskEndianU32_NtoB(s32);
        err = doAppend(state, &s32, sizeof(s32));
        BAIL_IF_ERR(err);
    }

bail:
	return err;
}

FskErr flvMuxerStop(FskMuxer muxer, void *stateIn)
{
	return kFskErrNone;
}

// note: we don't use any of this metadata, so this function is optimistically in place for future use
FskErr flvMuxerSetMetaData(FskMuxer muxer, void *stateIn, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 flags)
{
	flvMuxer state = stateIn;

	if (1 != index)
		return kFskErrUnknownElement;

	while (kFskErrNone == FskMediaMetaDataRemove(state->meta, metaDataType, 0))
		;

    state->metaChanged = true;

	return FskMediaMetaDataAdd(state->meta, metaDataType, NULL, value, 0);
}

FskErr flvMuxerNewTrack(FskMuxer muxer, void *stateIn, FskMuxerTrack track, FskMuxerTrackDispatch *dispatch, void **trackStateOut)
{
	flvMuxer state = stateIn;
	FskErr err = kFskErrNone;
	flvMuxerTrack trackState = NULL;

	if ((0 != FskStrCompare(track->trackType, "sound")) && (0 != FskStrCompare(track->trackType, "video")))
		return kFskErrInvalidParameter;

	err = FskMemPtrNewClear(sizeof(flvMuxerTrackRecord), (FskMemPtr *)&trackState);
	BAIL_IF_ERR(err);

	trackState->muxerTrack = track;
	trackState->muxer = muxer;
	trackState->state = state;
    
    if (0 == FskStrCompare(track->trackType, "sound")) {
        if (state->soundTrack) {
            err = kFskErrInvalidParameter;
            goto bail;
        }
        state->soundTrack = track;
        trackState->isSound = true;
        track->dispatch = &gFLVMuxerSoundTrack;
    }
    else {
        if (state->videoTrack) {
            err = kFskErrInvalidParameter;
            goto bail;
        }
        state->videoTrack = track;
        trackState->isVideo = true;
        track->dispatch = &gFLVMuxerVideoTrack;
    }

bail:
	if (kFskErrNone != err) {
		FskMemPtrDispose(trackState);
		trackState = NULL;
	}
	*trackStateOut = trackState;

	return err;
}

/*
	dispatched track functions
*/

FskErr flvMuxerTrackDispose(FskMuxerTrack track, void *stateIn)
{
	flvMuxerTrack trackState = stateIn;
	FskMemPtrDispose(trackState);
	return kFskErrNone;
}

FskErr flvMuxerTrackAdd(FskMuxerTrack track, void *stateIn, const void *data, UInt32 infoCount, FskMuxerSampleInfo info)
{
	flvMuxerTrack trackState = stateIn;
	flvMuxer state = trackState->state;
	FskErr err = kFskErrNone;
	UInt32 i, dataSize = 0, sampleCount = 0;
	unsigned char header[12];
	UInt32 s32;

    if (state->metaChanged) {
        unsigned char onMetaDataPlusArray[] = {0, 10, 'o', 'n', 'M', 'e', 't', 'a', 'D', 'a', 't', 'a', 8};
        unsigned char emptyString[] = {0, 0};
        unsigned char terminator[] = {0, 0, 9};
        UInt32 arrayLength, textSize = 0;
        UInt16 nameLength, nameLengthOut;
        FskMediaPropertyValueRecord nameValue, albumValue, artistValue;
        Boolean hasName, hasAlbum, hasArtist;
        UInt8 two = 2;
        UInt32 flags;

        hasName = kFskErrNone == FskMediaMetaDataGet(state->meta, "FullName", 1, &nameValue, &flags);
        hasArtist = kFskErrNone == FskMediaMetaDataGet(state->meta, "Artist", 1, &artistValue, &flags);
        hasAlbum = kFskErrNone == FskMediaMetaDataGet(state->meta, "Album", 1, &albumValue, &flags);
        
        if (hasName) textSize += FskStrLen(nameValue.value.str);
        if (hasArtist) textSize += FskStrLen(artistValue.value.str);
        if (hasAlbum) textSize += FskStrLen(albumValue.value.str);

        dataSize += sizeof(onMetaDataPlusArray);
        dataSize += 4;      // array length
        dataSize += textSize + (3 * 1) + (3 * 2) + (3 * 2) + 8 + 6 + 5;      // all string length bytes, all strings
        dataSize += sizeof(terminator);

        header[0] = 18;
        header[1] = (UInt8)(((dataSize + 1) >> 16) & 0xff);
        header[2] = (UInt8)(((dataSize + 1) >>  8) & 0xff);
        header[3] = (UInt8)((dataSize + 1) & 0xff);
        header[4] = (UInt8)((state->audioTime >> 16) & 0xff);
        header[5] = (UInt8)((state->audioTime >>  8) & 0xff);
        header[6] = (UInt8)((state->audioTime >>  0) & 0xff);
        header[7] = header[8] = header[9] = header[10] = 0;
        header[11] = 8;     // ecmascript array tag

        err = doAppend(state, &header, sizeof(header));
        BAIL_IF_ERR(err);
        
        err = doAppend(state, &onMetaDataPlusArray, sizeof(onMetaDataPlusArray));
        BAIL_IF_ERR(err);

        arrayLength = FskEndianU32_NtoB(3);
        err = doAppend(state, &arrayLength, sizeof(arrayLength));
        BAIL_IF_ERR(err);

        // name
        nameLength = 8;
        nameLengthOut = FskEndianU16_NtoB(nameLength);
        err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
        BAIL_IF_ERR(err);

        err = doAppend(state, "FullName", nameLength);
        BAIL_IF_ERR(err);
    
        err = doAppend(state, &two, 1);
        BAIL_IF_ERR(err);

        if (hasName) {
            nameLength = (UInt16)FskStrLen(nameValue.value.str);
            nameLengthOut = FskEndianU16_NtoB(nameLength);

            err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
            BAIL_IF_ERR(err);

            err = doAppend(state, nameValue.value.str, nameLength);
            BAIL_IF_ERR(err);
        }
        else {
            err = doAppend(state, emptyString, sizeof(emptyString));
            BAIL_IF_ERR(err);
        }

        // artist
        nameLength = 6;
        nameLengthOut = FskEndianU16_NtoB(nameLength);
        err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
        BAIL_IF_ERR(err);

        err = doAppend(state, "Artist", nameLength);
        BAIL_IF_ERR(err);
                
        err = doAppend(state, &two, 1);
        BAIL_IF_ERR(err);

        if (hasArtist) {
            nameLength = (UInt16)FskStrLen(artistValue.value.str);
            nameLengthOut = FskEndianU16_NtoB(nameLength);
            
            err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
            BAIL_IF_ERR(err);
            
            err = doAppend(state, artistValue.value.str, nameLength);
            BAIL_IF_ERR(err);
        }
        else {
            err = doAppend(state, emptyString, sizeof(emptyString));
            BAIL_IF_ERR(err);
        }
        
        // album
        nameLength = 5;
        nameLengthOut = FskEndianU16_NtoB(nameLength);
        err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
        BAIL_IF_ERR(err);

        err = doAppend(state, "Album", nameLength);
        BAIL_IF_ERR(err);

        err = doAppend(state, &two, 1);
        BAIL_IF_ERR(err);

        if (hasAlbum) {
            nameLength = (UInt16)FskStrLen(albumValue.value.str);
            nameLengthOut = FskEndianU16_NtoB(nameLength);
            
            err = doAppend(state, &nameLengthOut, sizeof(nameLengthOut));
            BAIL_IF_ERR(err);
            
            err = doAppend(state, albumValue.value.str, nameLength);
            BAIL_IF_ERR(err);
        }
        else {
            err = doAppend(state, emptyString, sizeof(emptyString));
            BAIL_IF_ERR(err);
        }

        // terminator
        err = doAppend(state, terminator, sizeof(terminator));
        BAIL_IF_ERR(err);

        // write previous tag size
        s32 = sizeof(header) + dataSize;
        s32 = FskEndianU32_NtoB(s32);
        err = doAppend(state, &s32, sizeof(s32));
        BAIL_IF_ERR(err);

        state->metaChanged = false;
        FskMediaMetaDataDispose(state->meta);
        state->meta = NULL;
        err = FskMediaMetaDataNew(&state->meta);
        BAIL_IF_ERR(err);
    }

    if (trackState->isSound) {
        if ((kFskAudioFormatMP3 != state->audioFormat) && (kFskAudioFormatAAC != state->audioFormat)) {
            for (i=0; i<infoCount; i++) {
                dataSize += info[i].sampleCount * info[i].sampleSize;
                sampleCount += info[i].sampleCount * info[i].sampleDuration;
            }

            // header
            FskMemMove(header, state->audioHeader, sizeof(header));
            header[1] = (UInt8)(((dataSize + 1) >> 16) & 0xff);
            header[2] = (UInt8)(((dataSize + 1) >>  8) & 0xff);
            header[3] = (UInt8)((dataSize + 1) & 0xff);
            header[4] = (UInt8)((state->audioTime >> 16) & 0xff);
            header[5] = (UInt8)((state->audioTime >>  8) & 0xff);
            header[6] = (UInt8)((state->audioTime >>  0) & 0xff);

            err = doAppend(state, &header, sizeof(header));
            BAIL_IF_ERR(err);

            // data
            err = doAppend(state, data, dataSize);
            BAIL_IF_ERR(err);

            // update clock
            state->audioSamples += sampleCount;
            state->audioTime = (UInt32)(((double)state->audioSamples / (double)state->sampleRate) * 1000.0);

            // write previous tag size
            s32 = sizeof(header) + dataSize;
            s32 = FskEndianU32_NtoB(s32);
            err = doAppend(state, &s32, sizeof(s32));
            BAIL_IF_ERR(err);
        }
        else {
            UInt32 aac = (kFskAudioFormatAAC == state->audioFormat) ? 1 : 0;

            for (i=0; i<infoCount; i++) {
                UInt32 j;

                for (j = 0; j < info[i].sampleCount; j++) {
                    dataSize = info[i].sampleSize + aac;
                    sampleCount = info[i].sampleDuration;

                    // header
                    FskMemMove(header, state->audioHeader, sizeof(header));
                    header[1] = (UInt8)(((dataSize + 1) >> 16) & 0xff);
                    header[2] = (UInt8)(((dataSize + 1) >>  8) & 0xff);
                    header[3] = (UInt8)((dataSize + 1) & 0xff);
                    header[4] = (UInt8)((state->audioTime >> 16) & 0xff);
                    header[5] = (UInt8)((state->audioTime >>  8) & 0xff);
                    header[6] = (UInt8)((state->audioTime >>  0) & 0xff);
                    
                    err = doAppend(state, &header, sizeof(header));
                    BAIL_IF_ERR(err);

                    if (aac) {
                        unsigned char packetType = 1;
                        
                        err = doAppend(state, &packetType, sizeof(packetType));
                        BAIL_IF_ERR(err);
                    }

                    // data
                    err = doAppend(state, data, dataSize - aac);
                    BAIL_IF_ERR(err);
                    
                    data = (const void*)((const char*)data + dataSize - aac);
                    
                    // update clock
                    state->audioSamples += sampleCount;
                    state->audioTime = (UInt32)(((double)state->audioSamples / (double)track->scale) * 1000.0);

                    // write previous tag size
                    s32 = sizeof(header) + dataSize;
                    s32 = FskEndianU32_NtoB(s32);
                    err = doAppend(state, &s32, sizeof(s32));
                    BAIL_IF_ERR(err);
                }
            }
        }
    }
    else {
        // header
        UInt32 videoTime = (UInt32)(((double)state->videoTime / (double)track->scale) * 1000.0);

        dataSize = info[0].sampleSize;
        FskMemMove(header, state->videoHeader, sizeof(header));
        header[1] = (UInt8)(((dataSize + 1) >> 16) & 0xff);
        header[2] = (UInt8)(((dataSize + 1) >>  8) & 0xff);
        header[3] = (UInt8)((dataSize + 1) & 0xff);
        header[4] = (UInt8)((videoTime >> 16) & 0xff);
        header[5] = (UInt8)((videoTime >>  8) & 0xff);
        header[6] = (UInt8)((videoTime >>  0) & 0xff);

        err = doAppend(state, &header, sizeof(header));
        BAIL_IF_ERR(err);
        
        // data
        err = doAppend(state, data, dataSize);
        BAIL_IF_ERR(err);

        // update clock
        state->videoTime += info[0].sampleDuration;

        // write previous tag size
        s32 = sizeof(header) + dataSize;
        s32 = FskEndianU32_NtoB(s32);
        err = doAppend(state, &s32, sizeof(s32));
        BAIL_IF_ERR(err);
    }

bail:
	return err;
}

FskErr flvMuxerTrackSetSampleRate(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvMuxerTrack trackState = stateIn;
	flvMuxer state = trackState->state;
	const UInt32 aacSampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

    state->sampleRate = (kFskMediaPropertyTypeInteger == property->type) ? (UInt32)property->value.integer : (UInt32)property->value.number;

	if (5500 == state->sampleRate)
		state->sampleRateBits = 0;
	else if (11025 == state->sampleRate)
		state->sampleRateBits = 1;
	else if (22050 == state->sampleRate)
		state->sampleRateBits = 2;
	else if (44100 == state->sampleRate)
		state->sampleRateBits = 3;
	// no error if no match, because Speex uses 8k and 16k, which are not expressable in the 2 bits allocated to sample rate in FLV

    for (state->aacSampleRateIndex = 0; state->aacSampleRateIndex < 12; state->aacSampleRateIndex++) {
		if (aacSampleRates[state->aacSampleRateIndex] == state->sampleRate)
            break;
	}

	return kFskErrNone;
}

FskErr flvMuxerTrackSetFormat(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvMuxerTrack trackState = stateIn;
	flvMuxer state = trackState->state;
	FskErr err = kFskErrNone;

    if (trackState->isSound) {
        if ((0 == FskStrCompare(property->value.str, "format:speexnb")) || (0 == FskStrCompare(property->value.str, "x-audio-codec/speex-nb"))) {
            state->audioFormat = kFskAudioFormatSPEEXNB;
            state->audioFormatBits = 11;
            state->sizeBits = 1;
        }
        else if ((0 == FskStrCompare(property->value.str, "format:pcm16le")) || (0 == FskStrCompare(property->value.str, "x-audio-codec/pcm-16-le"))) {
            state->audioFormat = kFskAudioFormatPCM16BitLittleEndian;
            state->audioFormatBits = 3;
            state->sizeBits = 1;
        }
        else if (0 == FskStrCompare(property->value.str, "x-audio-codec/mp3")) {
            state->audioFormat = kFskAudioFormatMP3;
            state->audioFormatBits = 2;
            state->sizeBits = 1;
        }
        else if (0 == FskStrCompare(property->value.str, "x-audio-codec/aac")) {
            state->audioFormat = kFskAudioFormatAAC;
            state->audioFormatBits = 10;
            state->sizeBits = 1;
        }
        else
            err = kFskErrInvalidParameter;
    }
    else {
        if (0 == FskStrCompare("image/jpeg", property->value.str))
            state->videoFormat = 1;
        else
            err = kFskErrInvalidParameter;
    }

	return err;
}

FskErr flvMuxerTrackSetFormatInfo(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    return kFskErrUnimplemented;        //@@ eventually needed for AVC?
}

FskErr flvMuxerTrackSetChannelCount(void *stateIn, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	flvMuxerTrack trackState = stateIn;
	flvMuxer state = trackState->state;

	if ((property->value.integer <= 0) || (property->value.integer > 2))
		return kFskErrInvalidParameter;

	state->channelCount = property->value.integer;

	return kFskErrNone;
}


FskErr doAppend(flvMuxer state, const void *data, UInt32 size)
{
    FskErr err = (state->muxer->write)(state->muxer, state->muxer->writeRefCon, data, state->nextOffset, size);
    if (kFskErrNone == err)
        state->nextOffset += size;
    return err;
}
