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
/*
	to do:

		X ICY metadata support
		X Chunky allocation of FskMediaReaderSampleInfoRecord in extract rather than lots of reallocs
		X AAC stream support
*/

#define __FSKMEDIAREADER_PRIV__
#include "FskDIDLGenMedia.h"
#include "FskFiles.h"
#include "FskHeaders.h"
#include "FskMediaReader.h"
#include "FskTextConvert.h"

static Boolean mp3ReaderCanHandle(const char *mimeType);
static FskErr mp3ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr mp3ReaderDispose(FskMediaReader reader, void *readerState);
static FskErr mp3ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr mp3ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr mp3ReaderStop(FskMediaReader reader, void *readerState);
static FskErr mp3ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr mp3ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr mp3ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr mp3ReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetFlags(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetSeekableSegment(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord mp3ReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			mp3ReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			mp3ReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		mp3ReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		mp3ReaderGetState,			NULL},
	{kFskMediaPropertyFlags,				kFskMediaPropertyTypeInteger,		mp3ReaderGetFlags,			NULL},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			mp3ReaderGetSeekableSegment, NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	mp3ReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderMP3 = {mp3ReaderCanHandle, mp3ReaderNew, mp3ReaderDispose, mp3ReaderGetTrack, mp3ReaderStart, mp3ReaderStop, mp3ReaderExtract, mp3ReaderGetMetadata, mp3ReaderProperties, mp3ReaderSniff};

static FskErr mp3ReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp3ReaderTrackGetProfile(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord mp3ReaderTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mp3ReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		mp3ReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		mp3ReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mp3ReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			mp3ReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		mp3ReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		mp3ReaderTrackGetTimeScale,			NULL},
	{kFskMediaPropertyProfile,				kFskMediaPropertyTypeString,		mp3ReaderTrackGetProfile,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gMediaReaderMP3Track = {mp3ReaderTrackProperties};

Boolean mp3ReaderCanHandle(const char *mimeType)
{
	const char *semi = FskStrChr(mimeType, ';');
	UInt32 length = semi ? (UInt32)(semi - mimeType) : FskStrLen(mimeType);

	if (0 == FskStrCompareCaseInsensitiveWithLength("audio/mpeg", mimeType, length))
		return true;

	if (0 == FskStrCompareCaseInsensitiveWithLength("audio/aac", mimeType, length))
		return true;

	if (0 == FskStrCompareCaseInsensitiveWithLength("audio/vnd.dlna.adts", mimeType, length))
		return true;

	return false;
}

// about 1 second of data at 128kps
#define kMP3ReadBufferSize (16384)

typedef struct {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;
	FskInt64				spoolerPosition;
	FskInt64				spoolerSize;

	DIDLMusicItemRecord		mi;

    double                  dlnaDuration;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	FskInt64				position;

	unsigned char			*readBufferPtr;
	unsigned char			*readBufferEnd;
	Boolean					resync;

	FskMediaReader				reader;
	FskMediaReaderTrackRecord	track;

	FskTimeRecord			orbStart;				// wall-clock time when we started
	Boolean					isOrbiter;				// true when Orb, can seek backwards from max time seen, but never forward

	struct {
		Boolean				isProtocol;				// true if ICY protocol instead of HTTP
		Boolean				isNanocaster;			// true when live365 (need bigger buffers because they are relatively bursty)

		UInt32				metaInt;
		FskInt64			nextMetaPosition;
		UInt32				metaBytesToCollect;
		UInt32				metaBytesCollected;
		unsigned char		*metaBytes;

		char				*title;
	} icy;

	struct {
		UInt32				size;			// total size
		FskInt64			offset;
		unsigned char		*data;
	} id3;

	Boolean					(*doHeaderParse)(unsigned char *scan, DIDLMusicItem mi);

	unsigned char			readBuffer[kMP3ReadBufferSize];
} mp3ReaderRecord, *mp3Reader;

static FskErr mp3ScanProc(void *refCon, UInt32 bytesToRead, void *data, UInt32 *bytesRead, FskInt64 offset, Boolean dontSeekIfExpensive);
static FskErr mp3RefillReadBuffer(mp3Reader state, UInt32 minimumBytesNeeded);
static FskErr mp3Instantiate(mp3Reader state);
static FskErr mp3SpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static void mp3TimeToPosition(mp3Reader state, double sampleTime, FskInt64 *position);
static char *icyString(char *str);

#if SUPPORT_MP3_LOG_TO_FILE
	static FskFile gFref = NULL;
#endif

FskErr mp3ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	mp3Reader state = NULL;

	if (NULL == spooler) {
		err = kFskErrUnimplemented;
		goto bail;
	}

	err = FskMemPtrNewClear(sizeof(mp3ReaderRecord), &state);
	if (err) goto bail;

	*readerState = state;				// must occur before doSetState
	state->spooler = spooler;
	state->reader = reader;

	state->spooler->onSpoolerCallback = mp3SpoolerCallback;
	state->spooler->clientRefCon = state;
	state->spooler->flags |= kFskMediaSpoolerForwardOnly;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		if (err) goto bail;

		state->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	state->icy.isNanocaster = (NULL != FskStrStr(mimeType, "live365=true"));

	err = mp3Instantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		mp3ReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr mp3ReaderDispose(FskMediaReader reader, void *readerState)
{
	mp3Reader state = readerState;

	if (NULL != state) {
		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);
		scanMP3Dispose(&state->mi);
		FskMemPtrDispose(state->icy.metaBytes);
		FskMemPtrDispose(state->icy.title);
		FskMemPtrDispose(state->id3.data);
		FskMemPtrDispose(state);
	}

#if SUPPORT_MP3_LOG_TO_FILE
	FskFileClose(gFref);
	gFref = NULL;
#endif

	return kFskErrNone;
}

FskErr mp3ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	mp3Reader state = readerState;
	FskErr err = kFskErrNone;

	if (0 != index)
		return kFskErrInvalidParameter;

	state->track.state = state;
	state->track.dispatch = &gMediaReaderMP3Track;

	*track = &state->track;

	return err;
}

FskErr mp3ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	mp3Reader state = readerState;
	double sampleTime, duration;

	state->atTime = startTime ? (UInt32)*startTime : 0;

	sampleTime = (double)state->atTime;
	state->hasEndTime = false;
    state->position = 0;
	if (-1 != state->mi.duration) {
		duration = (double)state->mi.duration * (double)state->mi.frequency;
		if (sampleTime > duration)
			sampleTime = duration;

		if (endTime) {
			if (*endTime > duration)
				state->endTime = (UInt32)duration;
			else
				state->endTime = (UInt32)*endTime;
			state->hasEndTime = true;
		}

        if (state->spooler->flags & kFskMediaSpoolerTimeSeekSupported)
            state->spooler->flags |= kFskMediaSpoolerUseTimeSeek;
        else
            mp3TimeToPosition(state, sampleTime, &state->position);
	}

	// reset buffers
	state->readBufferPtr = state->readBuffer;
	state->readBufferEnd = state->readBuffer;
	state->resync = true;

	return kFskErrNone;
}

FskErr mp3ReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr mp3ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **dataOut)
{
	mp3Reader state = readerState;
	FskErr err = kFskErrNone;
	UInt32 spaceInOutput;
	unsigned char *data;
	FskMediaReaderSampleInfo lastInfo = NULL;
	UInt32 infoCountFree;
#if SUPPORT_MP3_LOG_TO_FILE
	UInt32 outputSize;
#endif

	spaceInOutput = (state->mi.bitrate * 128)  / 2;		// about half a second
	if (spaceInOutput < 4096)
		spaceInOutput = 4096;
#if SUPPORT_MP3_LOG_TO_FILE
	outputSize = spaceInOutput;
#endif

	*track = &state->track;
	*infoCount = 0;
	*info = NULL;

	infoCountFree = state->mi.frequency >> 11;		// quick divide by 2048... allocate enough entries for about half a second of frames
	err = FskMemPtrNew(infoCountFree * sizeof(FskMediaReaderSampleInfoRecord), (FskMemPtr*)(void*)info);
	if (err) return err;

	err = FskMemPtrNew(spaceInOutput + kFskDecompressorSlop, (FskMemPtr *)&data);
	if (err) {
		FskMemPtrDisposeAt((void **)info);
		return err;
	}

	*dataOut = data;

	while (0 != infoCountFree) {
		if (state->hasEndTime && (state->atTime >= state->endTime)) {
			if (0 == *infoCount )
				err = kFskErrEndOfFile;
			goto bail;
		}

		if ((state->readBufferEnd - state->readBufferPtr) < 6) {
			err = mp3RefillReadBuffer(state, 6);
			if (err) goto bail;
		}

		if ((0xff == state->readBufferPtr[0]) && (0xe0 == (state->readBufferPtr[1] & 0xe0))) {
			DIDLMusicItemRecord mi;

			mi.doExtendedParsing = 0;
			if (true == (state->doHeaderParse)(state->readBufferPtr, &mi)) {
				if (spaceInOutput < mi.frameLength)
					goto bail;

				if ((state->readBufferEnd - state->readBufferPtr) < (SInt32)mi.frameLength) {
					err = mp3RefillReadBuffer(state, mi.frameLength);
					if (err) goto bail;
				}

				// When starting from a new location, make extra sure we're really sitting on a valid frame,
				// by validating the next frame.
				if (state->resync) {
					if (state->readBufferEnd - (state->readBufferPtr + mi.frameLength) > 6) {
						DIDLMusicItemRecord mi2;
						UInt8 *nextFramePtr = state->readBufferPtr + mi.frameLength;
						if ((0xff != nextFramePtr[0]) || (0xe0 != (nextFramePtr[1] & 0xe0))) {
							state->readBufferPtr += 1;
							continue;
						}

						mi2.doExtendedParsing = 0;
						if (false == (state->doHeaderParse)(nextFramePtr, &mi2)) {
							state->readBufferPtr += 1;
							continue;
						}
					}

					state->resync = false;
				}

				if (kFskAudioFormatAACADTS == mi.codec) {	// skip adts header to get to raw aac frame. @@ if frame Count != 1 this isn't going to work
					mi.frameLength -= 7;
					state->readBufferPtr += 7;
				}

				if ((NULL != lastInfo) && (lastInfo->sampleDuration = mi.samplesPerFrame) && (lastInfo->sampleSize == mi.frameLength))
					lastInfo->samples += 1;
				else {
					lastInfo = &(*info)[*infoCount];
					*infoCount += 1;
					infoCountFree -= 1;

					FskMemSet(lastInfo, 0, sizeof(FskMediaReaderSampleInfoRecord));
					lastInfo->samples = 1;
					lastInfo->sampleSize = mi.frameLength;
					lastInfo->flags = kFskImageFrameTypeSync;
					lastInfo->decodeTime = state->atTime;
					lastInfo->sampleDuration = mi.samplesPerFrame;
					lastInfo->compositionTime = -1;
				}

				FskMemMove(data, state->readBufferPtr, mi.frameLength);
				data += mi.frameLength;
				state->readBufferPtr += mi.frameLength;
				spaceInOutput -= mi.frameLength;

				state->atTime += mi.samplesPerFrame;

				continue;
			}
		}

		state->readBufferPtr += 1;
		state->resync = true;
	}

bail:
	if (kFskErrNone != err) {
		if ((kFskErrNeedMoreTime == err) || (kFskErrEndOfFile == err)) {
			if (0 != *infoCount)
				err = kFskErrNone;
		}

		if (kFskErrNone != err) {
			FskMemPtrDisposeAt((void **)info);
			FskMemPtrDisposeAt((void **)dataOut);
			*infoCount = 0;
		}
	}

#if SUPPORT_MP3_LOG_TO_FILE
	if (kFskErrNone == err) {
		if (NULL == gFref) {
			FskFileDelete("c:/dump.mp3");
			FskFileCreate("c:/dump.mp3");
			FskFileOpen("c:/dump.mp3", kFskFilePermissionReadWrite, &gFref);
		}
		FskFileWrite(gFref, outputSize - spaceInOutput, *dataOut, NULL);
	}
#endif

	return err;
}

FskErr mp3ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	mp3Reader state = readerState;
	return FskMediaMetaDataGetForMediaPlayer(state->mi.meta, metaDataType, index, value, flags);
}

FskErr mp3ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	UInt32 i;

	// check data for ID3 tag (as per section 3.1 of id3 spec)
	if (dataSize >= 10) {
		if (('I' == data[0]) & ('D' == data[1]) & ('3' == data[2])) {
			if ((data[3] != 0xff) && (data[4] != 0xff)) {
				if (0 == (data[5] & 0x1f)) {
					if ((data[6] < 0x80) && (data[7] < 0x80) && (data[8] < 0x80) && (data[9] < 0x80)) {
						*mime = FskStrDoCopy("audio/mpeg");		// could also be AAC depending on codec tag or actual bitstream. but this is enough to get it routed to this media player.
						return kFskErrNone;
					}
				}
			}
		}
	}

	for (i = 0; (i < 2) && (dataSize >= 10); i++) {
		const unsigned char *d = data, *e = data + dataSize - 7;		// - 7 to account for the max header size of mp3 & aac/adts
		UInt32 ds = dataSize;
		Boolean	(*doHeaderParse)(unsigned char *scan, DIDLMusicItem mi);

		doHeaderParse = (0 == i) ? parseMP3Header : parseADTSHeader;

		while (d < e) {
			DIDLMusicItemRecord mi0, mi1, mi2;
			mi0.doExtendedParsing = 0;
			mi1.doExtendedParsing = 0;
			mi2.doExtendedParsing = 0;
			if (true == (doHeaderParse)((unsigned char *)d, &mi0)) {
				if ((d + mi0.frameLength + 7) < e) {
					if (true == (doHeaderParse)((unsigned char *)(d + mi0.frameLength), &mi1)) {
						if ((d + mi0.frameLength + mi1.frameLength + 7) < e) {
							if (true == (doHeaderParse)((unsigned char *)(d + mi0.frameLength + mi1.frameLength), &mi2)) {
								if ((mi0.frequency == mi1.frequency) && (mi0.channelCount == mi1.channelCount) &&
									(mi0.frequency == mi2.frequency) && (mi0.channelCount == mi2.channelCount)) {
									*mime = FskStrDoCopy((0 == i) ? "audio/mpeg" : "audio/vnd.dlna.adts");
									return kFskErrNone;
								}
							}
						}
					}
				}
			}

			d += 1;
			ds -= 1;
		}
	}

	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr mp3ReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	if (-1 != state->mi.duration)
		property->value.number = state->mi.duration * state->mi.frequency;
	else
		property->value.number = -1;

	return kFskErrNone;
}

FskErr mp3ReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr mp3ReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->mi.frequency;

	return kFskErrNone;
}

FskErr mp3ReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr mp3ReaderGetFlags(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;
	FskMediaPropertyValueRecord value;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = kFskMediaPlayerFlagHasAudio;

	if (kFskErrNone == FskMediaMetaDataGet(state->mi.meta, "Genre", 0, &value, NULL)) {
		if (0 == FskStrCompareCaseInsensitive(value.value.str, "Audiobooks"))
			property->value.integer |= kFskMediaPlayerFlagHasAudioBook;
	}

	if (kFskErrNone == FskMediaMetaDataGet(state->mi.meta, "Album", 0, &value, NULL)) {
		if (NULL != FskStrStr(value.value.str, "Librivox"))
			property->value.integer |= kFskMediaPlayerFlagHasAudioBook;
	}

	return kFskErrNone;
}

FskErr mp3ReaderGetSeekableSegment(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;

	if (state->isOrbiter) {
		FskTimeRecord now;
		FskTimeGetNow(&now);
		FskTimeSub(&state->orbStart, &now);
		property->value.number = FskTimeInMS(&now) / 500.0;	// assume it can transcode at twice real time
		if (property->value.number > state->mi.duration) {
			property->value.number = state->mi.duration;
			state->reader->needsIdle = false;
		}
		return kFskErrNone;
	}

	if (!(kFskMediaSpoolerDownloading & state->spooler->flags) || (-1 == state->mi.duration))
		return kFskErrUnimplemented;

	if ((state->spoolerPosition <= state->mi.dataOffset) || (0 == state->mi.dataSize))
		property->value.number = 0;
	else {
		FskInt64 position = state->spoolerPosition - state->mi.dataOffset;
		double time;

		time = (((double)position) / ((double)state->mi.dataSize)) * (double)(state->mi.duration * state->mi.frequency);

		while (state->mi.xingToc) {
			FskInt64 xingPos;

			mp3TimeToPosition(state, time, &xingPos);
			xingPos -= state->mi.dataOffset;
			if (xingPos <= position)
				break;

			if (0 == time)
				break;

			if (time < state->mi.frequency)
				time = 0;
			else
				time -= state->mi.frequency;
		}

		property->value.number = time / ((double)state->mi.frequency);
	}

	return kFskErrNone;
}

FskErr mp3ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "audio/mpeg\000audio/aac\000audio/vnd.dlna.adts\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr mp3ReaderTrackGetMediaType(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("audio");

	return kFskErrNone;
}

FskErr mp3ReaderTrackGetFormat(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeString;
	if (kFskAudioFormatMP3 == state->mi.codec)
		property->value.str = FskStrDoCopy("x-audio-codec/mp3");
	else if (kFskAudioFormatMP2 == state->mi.codec)
		property->value.str = FskStrDoCopy("x-audio-codec/mp2");
	else if (kFskAudioFormatMP1 == state->mi.codec)
		property->value.str = FskStrDoCopy("x-audio-codec/mp1");
	else if (kFskAudioFormatAACADTS == state->mi.codec)
		property->value.str = FskStrDoCopy("x-audio-codec/aac");
	else
		return kFskErrBadState;

	return kFskErrNone;
}

FskErr mp3ReaderTrackGetSampleRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->mi.frequency;

	return kFskErrNone;
}

FskErr mp3ReaderTrackGetBitRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->mi.bitrate * 1000;

	return kFskErrNone;
}

FskErr mp3ReaderTrackGetFormatInfo(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;
	UInt8 esds[] = {	0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x80, 0x80, 0x22, 0x00, 0x00, 0x00, 0x04, 0x80, 0x80, 0x80,
						0x14, 0x40, 0x15, 0x00, 0x18, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x05, 0x80,
						0x80, 0x80, 0x02, 0x10, 0x00, 0x06, 0x80, 0x80, 0x80, 0x01, 0x02};
	const UInt32 kESDSPatchOffset = 35;
	const UInt32 sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};
	UInt32 sampleRateIndex = 0;

	if (kFskAudioFormatAACADTS != state->mi.codec)
		return kFskErrUnimplemented;

	for (sampleRateIndex = 0; sampleRateIndex < 12; sampleRateIndex++) {
		if (sampleRates[sampleRateIndex] == state->mi.frequency)
			break;
	}
	if (sampleRateIndex > 11)
		return state->mi.frequency;

	esds[kESDSPatchOffset] |= (UInt8)(sampleRateIndex >> 1);
	esds[kESDSPatchOffset + 1] |= (UInt8)((sampleRateIndex & 1) << 7);
	if (1 == state->mi.channelCount)
		esds[kESDSPatchOffset + 1] |= 0x08;
	else if (2 == state->mi.channelCount)
		esds[kESDSPatchOffset + 1] |= 0x10;

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = sizeof(esds);
	return FskMemPtrNewFromData(sizeof(esds), esds, (FskMemPtr*)(void*)(&property->value.data.data));
}


FskErr mp3ReaderTrackGetChannelCount(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->mi.channelCount;

	return kFskErrNone;
}

FskErr mp3ReaderTrackGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->mi.frequency;

	return kFskErrNone;
}


FskErr mp3ReaderTrackGetProfile(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp3Reader state = readerState;
	int profile = state->mi.profile;
	int level   = state->mi.level;
	const char *profile_str;
	char level_str[16];

	if (kFskAudioFormatAACADTS == state->mi.codec )
	{
			 if( profile == 1 )		profile_str = "AAC Main,";
		else if( profile == 2 )		profile_str = "AAC LC,";
		else if( profile == 3 )		profile_str = "AAC SSR,";
		else if( profile == 4 )		profile_str = "AAC LTP,";
		else if( profile == 5 )		profile_str = "SBR,";
		else if( profile == 6 )		profile_str = "AAC Scalable,";
		else if( profile == 7 )		profile_str = "TwinVQ,";
		else if( profile == 8 )		profile_str = "CELP,";
		else if( profile == 9 )		profile_str = "HXVC,";
		else
			profile_str = "unknown,";
	}
	else
		profile_str = "unknown,";

	FskStrNumToStr( level, level_str, sizeof(level_str));
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCat( profile_str, level_str );

	return kFskErrNone;
}


/*
	local
*/

FskErr mp3ScanProc(void *refCon, UInt32 bytesToRead, void *data, UInt32 *bytesReadOut, FskInt64 offset, Boolean dontSeekIfExpensive)
{
	mp3Reader state = refCon;
	FskErr err = kFskErrNone;
	UInt32 bytesRead = 0;

	if (state->id3.data && (offset < state->id3.offset)) {
		UInt32 bytesToUse = (UInt32)(state->id3.offset - offset);
		if (bytesToUse > bytesToRead)
			bytesToUse = bytesToRead;
		FskMemMove(data, (char *)state->id3.data + offset, bytesToUse);
		if (bytesReadOut)
			*bytesReadOut = bytesToUse;
		else
		if (bytesToUse != bytesToRead)
			return kFskErrNeedMoreTime;
		return kFskErrNone;
	}

	while (0 != bytesToRead) {
		void *buffer;
		UInt32 readCount;

		if (dontSeekIfExpensive)
			state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;
		err = FskMediaSpoolerRead(state->spooler, offset, bytesToRead, &buffer, &readCount);
		state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		if (err) {
			if (0 == bytesRead)
				return err;

			err = kFskErrNone;		// return what we have
			break;
		}

		FskMemMove(data, buffer, readCount);

		bytesRead += readCount;
		bytesToRead -= readCount;
		offset += readCount;
		data = readCount + (char *)data;
	}

	if (NULL != bytesReadOut)
		*bytesReadOut = bytesRead;
	else
	if (0 != bytesToRead)
		err = kFskErrNeedMoreTime;

	return err;
}

FskErr mp3RefillReadBuffer(mp3Reader state, UInt32 minimumBytesNeeded)
{
	FskErr err = kFskErrNone;
	UInt32 bytesInBuffer = state->readBufferEnd - state->readBufferPtr;
	UInt32 bytesRead;
	void *buffer;
	Boolean firstTime = true;

	FskMemMove(state->readBuffer, state->readBufferPtr, bytesInBuffer);
	state->readBufferPtr = state->readBuffer;
	state->readBufferEnd = state->readBufferPtr + bytesInBuffer;

	while (true) {
		UInt32 bytesToRead = kMP3ReadBufferSize - bytesInBuffer;

		if (state->spoolerSize && ((state->position + bytesToRead) > state->spoolerSize)) {
			bytesToRead = (UInt32)(state->spoolerSize - state->position);
			if (0 == bytesToRead) {
				err = kFskErrEndOfFile;
				goto bail;
			}
		}

		if (0 != state->icy.metaInt) {
			if (0 != state->icy.metaBytesToCollect) {
				err = FskMediaSpoolerRead(state->spooler, state->position, state->icy.metaBytesToCollect, &buffer, &bytesRead);
				if (kFskErrNone != err) goto readErr;

				state->position += bytesRead;

				err = FskMemPtrRealloc(state->icy.metaBytesCollected + bytesRead + 1, &state->icy.metaBytes);
				if (err) return err;

				FskMemMove(state->icy.metaBytes + state->icy.metaBytesCollected, buffer, bytesRead);
				state->icy.metaBytes[state->icy.metaBytesCollected + bytesRead] = 0;

				state->icy.metaBytesCollected += bytesRead;
				state->icy.metaBytesToCollect -= bytesRead;
				if (0 == state->icy.metaBytesToCollect) {
					if (0 == FskStrCompareCaseInsensitiveWithLength((char *)state->icy.metaBytes, "StreamTitle=", 12)) {
						char *start = (char *)state->icy.metaBytes + 13;
						char *end = start;
						char *dash;
						FskMediaPropertyValueRecord prop;

						while (true) {
							end = FskStrChr(end, start[-1]);
							if (NULL == end) break;
							if ((0 != end[1]) && (';' != end[1])) {
								end += 1;
								continue;
							}
							break;
						}
						if (end)
							*end = 0;

						while (true) {
							if (kFskErrNone != FskMediaMetaDataRemove(state->mi.meta, "FullName", 0))
								break;
						}

						dash = FskStrStr(start, " - ");
						if (NULL != dash) {
							while (true) {
								if (kFskErrNone != FskMediaMetaDataRemove(state->mi.meta, "Artist", 0))
									break;
							}

							*dash = 0;

							prop.type = kFskMediaPropertyTypeString;
							prop.value.str = icyString(start);
							FskMediaMetaDataAdd(state->mi.meta, "Artist", NULL, &prop, kFskMediaMetaDataFlagOwnIt);

							prop.type = kFskMediaPropertyTypeString;
							prop.value.str = icyString(dash + 3);
							FskMediaMetaDataAdd(state->mi.meta, "FullName", NULL, &prop, kFskMediaMetaDataFlagOwnIt);
						}
						else {
							prop.type = kFskMediaPropertyTypeString;
							prop.value.str = icyString(start);
							FskMediaMetaDataAdd(state->mi.meta, "FullName", NULL, &prop, kFskMediaMetaDataFlagOwnIt);
						}

						FskMediaReaderSendEvent(state->reader, kFskEventMediaPlayerMetaDataChanged);
					}
					FskMemPtrDisposeAt((void**)(void*)(&state->icy.metaBytes));
				}
				continue;
			}
			else
			if (state->position == state->icy.nextMetaPosition) {
				err = FskMediaSpoolerRead(state->spooler, state->position, 1, &buffer, &bytesRead);
				if (kFskErrNone != err) goto readErr;

				state->position += 1;
				state->icy.metaBytesToCollect = ((unsigned char *)buffer)[0] * 16;
				state->icy.metaBytesCollected = 0;
				state->icy.nextMetaPosition += 1 + state->icy.metaBytesToCollect + state->icy.metaInt;
				continue;
			}
			else
			if ((state->position <= state->icy.nextMetaPosition) && (state->icy.nextMetaPosition < (state->position + bytesToRead)))
				bytesToRead = (UInt32)(state->icy.nextMetaPosition - state->position);
		}

		err = FskMediaSpoolerRead(state->spooler, state->position, bytesToRead, &buffer, &bytesRead);
readErr:
		if (err) {
			if (false == firstTime) {
				err = kFskErrNone;
				break;
			}
			goto bail;
		}

		FskMemMove(state->readBufferEnd, buffer, bytesRead);

		state->position += bytesRead;
		state->readBufferEnd += bytesRead;
		bytesInBuffer = state->readBufferEnd - state->readBufferPtr;

		if ((kMP3ReadBufferSize == bytesInBuffer) || (bytesInBuffer >= minimumBytesNeeded))
			break;

		firstTime = false;
	}

	if (bytesInBuffer < minimumBytesNeeded) {
		err = kFskErrNeedMoreTime;
		goto bail;
	}

bail:
	return err;
}

FskErr mp3Instantiate(mp3Reader state)
{
	FskErr err = kFskErrNone;
	FskInt64 totalSize;

	if ((NULL != state->spooler->doGetSize) && (false == state->icy.isProtocol)) {
		err = (state->spooler->doGetSize)(state->spooler, &totalSize);
		if (err) goto bail;
	}
	else
		totalSize = 0;

	state->spoolerSize = totalSize;

	state->mi.id3TagSize = 0;
	state->mi.doExtendedParsing = 1;
	if (false == scanMP3FromCallback(&state->mi, mp3ScanProc, state, totalSize, true)) {
		if (0 != state->mi.id3TagSize) {
			if (NULL == state->id3.data) {
				state->id3.size = state->mi.id3TagSize;
				err = FskMemPtrNew(state->id3.size, &state->id3.data);
				if (err) goto bail;
			} else if (state->id3.size < state->mi.id3TagSize) {
				state->id3.size = state->mi.id3TagSize;
				err = FskMemPtrRealloc(state->id3.size, &state->id3.data);
				if (err) goto bail;
			}
		}

		err = kFskErrBadData;
		goto bail;
	}

	if (state->icy.isProtocol || state->icy.isNanocaster)
		state->mi.duration = -1;
    else if ((-1 == state->mi.duration) && (0 != state->dlnaDuration))
		state->mi.duration = state->dlnaDuration;

	if (NULL == state->mi.meta)
		FskMediaMetaDataNew(&state->mi.meta);			// so we can reliably have meta available for updates

	if (NULL != state->icy.title) {
		FskMediaPropertyValueRecord prop;

		prop.type = kFskMediaPropertyTypeString;
		prop.value.str = state->icy.title;
		FskMediaMetaDataAdd(state->mi.meta, "FullName", NULL, &prop, 0);
	}

	if ((kFskAudioFormatMP3 == state->mi.codec) || (kFskAudioFormatMP2 == state->mi.codec) || (kFskAudioFormatMP1 == state->mi.codec) )
		state->doHeaderParse = parseMP3Header;
	else if (kFskAudioFormatAACADTS == state->mi.codec)
		state->doHeaderParse = parseADTSHeader;
	else {
		err = kFskErrUnimplemented;
		goto bail;
	}

	FskMemPtrDisposeAt(&state->id3.data);

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}

FskErr mp3SpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	mp3Reader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			state->spoolerPosition += (UInt32)param;
			if (state->reader->mediaState < kFskMediaPlayerStateStopped) {
				if (state->id3.data) {
					UInt32 percent, bytesRead;
					void *buffer;

					if (state->id3.size != state->id3.offset) {
						err = FskMediaSpoolerRead(state->spooler, state->id3.offset, (UInt32)(state->id3.size - state->id3.offset), &buffer, &bytesRead);
						if (kFskErrNone == err) {
							FskMemMove((char *)state->id3.data + state->id3.offset, buffer, bytesRead);
							state->id3.offset += bytesRead;
						}
					}

					percent = (UInt32)((((float)state->id3.offset) / ((float)state->id3.size)) * 100.0);
					if (percent < 100)
						(state->reader->doSetState)(state->reader, kFskMediaPlayerStateInstantiatingProgress + percent);
					else
						err = mp3Instantiate(state);
				}
				else
					err = mp3Instantiate(state);
			}
			break;

		case kFskMediaSpoolerOperationSetHeaders: {
			FskHeaders *headers = param;
			FskHeaderAddString("icy-metadata", "1", headers);

			}
			break;

		case kFskMediaSpoolerOperationGetHeaders: {
			FskHeaders *headers = param;
			char *value = FskHeaderFind("icy-metaint", headers);
			state->icy.metaInt = (NULL != value) ? FskStrToNum(value) : 0;
			state->icy.nextMetaPosition = state->icy.metaInt;

			if ((NULL == state->mi.meta) || (kFskErrNone != FskMediaMetaDataGet(state->mi.meta, "FullName", 0, NULL, NULL))) {
				value = FskHeaderFind("icy-name", headers);
				if (NULL == value)
					value = FskHeaderFind("x-audiocast-name", headers);
				state->icy.title = FskStrDoCopy(value);
			}

			state->icy.isProtocol = 0 == FskStrCompare(headers->protocol, "ICY");

			value = FskHeaderFind("Server", headers);
			if (NULL != value) {
				if (0 == FskStrCompareWithLength("Orbiter", value, 7)) {
					if (!state->isOrbiter) {
						state->isOrbiter = true;
						state->reader->needsIdle = true;
						FskTimeGetNow(&state->orbStart);
					}
				}
			}

			if (state->icy.isNanocaster) {
				value = FskHeaderFind("icy-genre", headers);
				if ((NULL != value) && (0 == FskStrCompareCaseInsensitive(value, "error")))
					(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
			}

            value = FskHeaderFind("availableSeekRange.dlna.org", headers);
            if (value && (0 == FskStrCompareWithLength("1 npt=", value, 6))) {
                char *dash = FskStrChr(value + 6, '-');
                double duration;
                if (dash && (kFskErrNone == FskMediaParseNPT(dash + 1, &duration)))
                    state->dlnaDuration = duration;
            }
			}
			break;

		case kFskMediaSpoolerOperationGetURI:
			state->spoolerPosition = ((FskMediaSpoolerGetURI)param)->position;
			break;
	}

	return err;
}

void mp3TimeToPosition(mp3Reader state, double sampleTime, FskInt64 *position)
{
	double duration = (double)state->mi.duration * (double)state->mi.frequency;

	if (NULL == state->mi.xingToc)
		*position = (FskInt64)(state->mi.dataSize * (sampleTime / duration));
	else {
		double percent = ((double)sampleTime / (double)duration) * 100.0;
		double interp;
		SInt32 fa, fb;
		SInt32 p = (SInt32)percent;
		if (p < 0)
			p = 0;
		else
		if (p > 99)
			p = 99;

		fa = state->mi.xingToc[p];
		fb = (99 == p) ? 255 : state->mi.xingToc[p + 1];
		interp = fa + (fb - fa) * (percent - p);
		interp /= 256.0;
		*position = (FskInt64)(interp * state->mi.dataSize);
	}

	*position += state->mi.dataOffset;
}

char *icyString(char *str)
{
	char *result;
	UInt32 len = FskStrLen(str);
	if (FskTextUTF8IsValid((unsigned char *)str, len))
		return FskStrDoCopy(str);

	FskTextLatin1ToUTF8(str, len, &result, NULL);
	return result;
}
