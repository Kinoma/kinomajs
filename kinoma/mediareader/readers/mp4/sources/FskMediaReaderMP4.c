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
/*
	to do:

		edit list
		album art for memory stick video
		choose time scale based on content (maybe)

		X bitrate calculations
		X stream
		X when seeking on stream, determine earliest offset and read that first to avoid multiple seeks
*/

#define __FSKMEDIAREADER_PRIV__
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskMediaReader.h"
#include "FskTextConvert.h"
#include "QTReader.h"

static Boolean mp4ReaderCanHandle(const char *mimeType);
static FskErr mp4ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr mp4ReaderDispose(FskMediaReader reader, void *readerState);
static FskErr mp4ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr mp4ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr mp4ReaderStop(FskMediaReader reader, void *readerState);
static FskErr mp4ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr mp4ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr mp4ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr mp4ReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetFlags(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetRedirect(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetError(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetSeekableSegment(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderSetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord mp4ReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			mp4ReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			mp4ReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		mp4ReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		mp4ReaderGetState,			NULL},
	{kFskMediaPropertyFlags,				kFskMediaPropertyTypeInteger,		mp4ReaderGetFlags,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						mp4ReaderSetScrub},
	{kFskMediaPropertyRedirect,				kFskMediaPropertyTypeString,		mp4ReaderGetRedirect,		NULL},
	{kFskMediaPropertyError,				kFskMediaPropertyTypeInteger,		mp4ReaderGetError,			NULL},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			mp4ReaderGetSeekableSegment, NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		NULL,						mp4ReaderSetBitRate},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	mp4ReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderMP4 = {mp4ReaderCanHandle, mp4ReaderNew, mp4ReaderDispose, mp4ReaderGetTrack, mp4ReaderStart, mp4ReaderStop, mp4ReaderExtract, mp4ReaderGetMetadata, mp4ReaderProperties, mp4ReaderSniff};

static FskErr mp4ReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetProfile(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mp4ReaderTrackGetRotation(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord mp4ReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mp4ReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		mp4ReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		mp4ReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mp4ReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			mp4ReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		mp4ReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyProfile,				kFskMediaPropertyTypeString,		mp4ReaderTrackGetProfile,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskMediaPropertyEntryRecord mp4ReaderVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mp4ReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		mp4ReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mp4ReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			mp4ReaderTrackGetFrameRate,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			mp4ReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		mp4ReaderTrackGetDimensions,		NULL},
	{kFskMediaPropertyProfile,				kFskMediaPropertyTypeString,		mp4ReaderTrackGetProfile,			NULL},
	{kFskMediaPropertyRotation,				kFskMediaPropertyTypeFloat,			mp4ReaderTrackGetRotation,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskMediaPropertyEntryRecord mp4ReaderUnknownTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mp4ReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mp4ReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gMP4ReaderAudioTrack = {mp4ReaderAudioTrackProperties};
FskMediaReaderTrackDispatchRecord gMP4ReaderVideoTrack = {mp4ReaderVideoTrackProperties};
FskMediaReaderTrackDispatchRecord gMP4ReaderUnknownTrack = {mp4ReaderUnknownTrackProperties};


Boolean mp4ReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("audio/mp4", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/mp4", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/quicktime", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("audio/3gpp", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/3gpp", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/3gpp2", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("audio/3gpp2", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("audio/mp4-alac", mimeType))
		return true;

	return false;
}

typedef struct mp4ReaderTrackRecord mp4ReaderTrackRecord;
typedef struct mp4ReaderTrackRecord *mp4ReaderTrack;

typedef struct QTTimeToOffsetTableRecord QTTimeToOffsetTableRecord;
typedef struct QTTimeToOffsetTableRecord *QTTimeToOffsetTable;

typedef struct {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	QTMovie					movie;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	UInt32					scale;
	double					duration;

	FskMediaReader			reader;

	SInt32					availableBitRate;
    UInt32					bitRate;

	Boolean					scrub;
	Boolean					isM4B;
	Boolean					hasDRM;
	Boolean					forceDownload;

	FskErr					theError;

	mp4ReaderTrack			tracks;

	char					*redirect;
	char					*uri;

	QTTimeToOffsetTable		timeToOffset;
	FskInt64				spoolerPosition;
	double					downloadMaxTime;
	UInt32					*downloadTimeToOffsetEntry;

	struct {
		UInt32				size;			// total size
		UInt32				bytes;			// bytes so far
		QTFileOffset		offset;
		unsigned char		*data;
	} moov;
} mp4ReaderRecord, *mp4Reader;

typedef enum {
	kFskMediaReaderMP4Unknown = 0,
	kFskMediaReaderMP4Audio = 1,
	kFskMediaReaderMP4Video = 2,
	kFskMediaReaderMP4Pano = 3,
	kFskMediaReaderMP4Object = 4,
	kFskMediaReaderMP4Hint = 5
} mp4MediaTypes;

struct mp4ReaderTrackRecord {
	mp4ReaderTrack						next;

	FskMediaReaderTrackRecord			readerTrack;
	QTTrack								qtTrack;
	mp4Reader							state;

	mp4MediaTypes						mediaType;

	char								*format;
	unsigned char						*formatInfo;
	UInt32								formatInfoSize;

	UInt32								profile;
	UInt32								level;

	Boolean								unplayable;
	UInt8								atEnd;		// 0 = not at end, 1 = at end but haven't reported that through extract, 2 = at end and have reported it
	Boolean								missingCodec;

	UInt32								lastExtractedTime;
	UInt32								sampleToExtract;		// next one to get
	UInt32								editMediaTime;

	FskDimensionRecord					dimensions;

	QTFileOffset						dataSize;

	union {
		struct {
			QTSoundDescription			soundDesc;
			UInt32						sampleRate;
			UInt32						channelCount;
			UInt32						durationPerFrame;
			Boolean						compressed;
		} audio;

		struct {
			QTImageDescription			videoDesc;
		} video;
	};
};

struct QTTimeToOffsetTableRecord {
	UInt32	secondsPerEntry;
	UInt32	maxGap;
	Boolean	hasMediaSamplesOutOfOrder;

	UInt32	entryCount;
	UInt32	entry[1];
};

static FskErr mp4Instantiate(mp4Reader state);
static FskErr mp4SpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static FskErr buildTimeToOffsetTable(mp4Reader state);

static QTErr qtRead(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize);
static QTErr qtMemRead(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize);
static QTErr qtAlloc(void *refCon, Boolean clear, UInt32 size, void **data);
static void qtFree(void *refCon, void *data);

FskErr mp4ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	mp4Reader state = NULL;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);

	err = FskMemPtrNewClear(sizeof(mp4ReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;		// must occur before any callback
	state->spooler = spooler;
	state->reader = reader;

	state->spooler->onSpoolerCallback = mp4SpoolerCallback;
	state->spooler->clientRefCon = state;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	if (uri && (FskStrLen(uri) > 5) && (0 == FskStrCompareCaseInsensitive(".m4b", uri + FskStrLen(uri) - 4)))
		state->isM4B = true;

	state->uri = FskStrDoCopy(uri);

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = mp4Instantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		mp4ReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr mp4ReaderDispose(FskMediaReader reader, void *readerState)
{
	mp4Reader state = readerState;

	if (NULL != state) {
		while (state->tracks) {
			mp4ReaderTrack track = FskListRemoveFirst((FskList*)(void*)(&state->tracks));
			FskMemPtrDispose(track->format);
			FskMemPtrDispose(track);
		}

		QTMovieDispose(state->movie);

		FskMemPtrDispose(state->moov.data);
		FskMemPtrDispose(state->redirect);
		FskMemPtrDispose(state->uri);
		FskMemPtrDispose(state->timeToOffset);

		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr mp4ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	mp4Reader state = readerState;
	mp4ReaderTrack walker;

	if (state->theError)
		return state->theError;

	for (walker = state->tracks; NULL != walker; walker = walker->next) {
		if (walker->unplayable)
			continue;

		if (0 == index) {
			*track = &walker->readerTrack;
			return kFskErrNone;
		}

		index -= 1;
	}

	return kFskErrNotFound;
}

FskErr mp4ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	mp4Reader state = readerState;
	UInt32 movieTime;
	mp4ReaderTrack walker;
	QTFileOffset minOffset = ~0;

	if (state->hasDRM) {
		if (kFskErrNone == state->theError) {
			state->theError = kFskErrCannotDecrypt;
			(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
		}
		return kFskErrCannotDecrypt;
	}

	state->atTime = startTime ? (UInt32)*startTime : 0;

	if (endTime) {
		if (*endTime > state->duration)
			state->endTime = (UInt32)state->duration;
		else
			state->endTime = (UInt32)*endTime;
		state->hasEndTime = true;
	}
	else
		state->hasEndTime = false;

	movieTime = (UInt32)(((double)state->atTime / (double)state->scale) * (double)state->movie->scale);

	for (walker = state->tracks; NULL != walker; walker = walker->next) {
		UInt32 requestedMediaTime;
		UInt32 mediaTime, duration, compositionTimeOffset, chunkNumber, firstChunkSample;
		QTFileOffset thisOffset, offsetInChunk;
		UInt32 editIndex;

		// skip over any initial empty edits
		walker->editMediaTime = 0;
		for (editIndex = 0; editIndex < walker->qtTrack->editsCount; editIndex++) {
			if ((-1 == walker->qtTrack->edits[editIndex].mediaTime) || (0 == walker->qtTrack->edits[editIndex].trackDuration))
				continue;
			walker->editMediaTime = walker->qtTrack->edits[editIndex].mediaTime;
		}

		requestedMediaTime = QTMovieToTrackDuration(movieTime, walker->qtTrack) + walker->editMediaTime;

		if (kFskErrNone != QTTrackTimeToSample(walker->qtTrack, requestedMediaTime, &walker->sampleToExtract, NULL, NULL)) {
			walker->atEnd = true;
			continue;
		}

		QTTrackSampleToSyncSamples(walker->qtTrack, walker->sampleToExtract, &walker->sampleToExtract, NULL);
		QTTrackGetSampleTemporalInfo(walker->qtTrack, walker->sampleToExtract, &mediaTime, &compositionTimeOffset, &duration);
		walker->lastExtractedTime = (UInt32)(((double)mediaTime / (double)walker->qtTrack->media->scale) * (double)state->scale);

		walker->atEnd = walker->unplayable;

		// determine minimum read offset for this start
		if ((kFskErrNone == QTTrackSampleToChunk(walker->qtTrack, walker->sampleToExtract, &chunkNumber, &thisOffset, &firstChunkSample, NULL)) &&
			(firstChunkSample != walker->sampleToExtract)) {
			QTTrackGetSampleSizes(walker->qtTrack, firstChunkSample, walker->sampleToExtract - firstChunkSample, &offsetInChunk);
			thisOffset += offsetInChunk;
		}
		if ((~0 == minOffset) || (thisOffset < minOffset))
			minOffset = thisOffset;
	}

	if (~0 != minOffset) {
		// get the spooler started at the minimm offset we'll need.
		unsigned char *buffer;
		UInt32 bytesRead;

		FskMediaSpoolerRead(state->spooler, minOffset, 1, &buffer, &bytesRead);
		bytesRead += 1;
	}

	return kFskErrNone;
}

FskErr mp4ReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr mp4ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	mp4Reader state = readerState;
	FskErr err = kFskErrNone;
	mp4ReaderTrack walker, track = NULL;
	UInt32 lastExtractedTime = 0, infoCount;
	void *data = NULL;
	UInt32 dataSize;
	UInt32 mediaTime, compositionTimeOffset, duration, prevSyncSample;
	FskMediaReaderSampleInfo info = NULL;

	for (walker = state->tracks; NULL != walker; walker = walker->next) {
		if (walker->atEnd) {
			if (1 == walker->atEnd) {
				walker->atEnd = 2;

				*trackOut = &walker->readerTrack;;
				*infoCountOut = 1;
				*dataOut = NULL;
				FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), infoOut);
				(*infoOut)->flags |= kFskImageFrameEndOfMedia;
				goto bail;

			}
			continue;
		}

		if ((NULL == track) || (walker->lastExtractedTime <= lastExtractedTime)) {
			track = walker;
			lastExtractedTime = walker->lastExtractedTime;
		}
	}

	if (NULL == track)
		return kFskErrEndOfFile;

	if (kFskMediaReaderMP4Audio != track->mediaType) {
		double t;
		if (false == state->scrub) {
			err = QTTrackSampleToSyncSamples(track->qtTrack, track->sampleToExtract, &prevSyncSample, NULL);
			BAIL_IF_ERR(err);
		}
		else {
			UInt32 nextSyncSample;

			err = QTTrackSampleToSyncSamples(track->qtTrack, track->sampleToExtract, &prevSyncSample, &nextSyncSample);
			BAIL_IF_ERR(err);

			if ((0 == nextSyncSample) && (track->sampleToExtract > prevSyncSample))
                BAIL(kFskErrEndOfFile);

			if ((track->sampleToExtract != prevSyncSample) && nextSyncSample) {
				track->sampleToExtract = nextSyncSample;
				prevSyncSample = nextSyncSample;
			}
			else
				track->sampleToExtract = prevSyncSample;
		}

		err = QTTrackGetSampleTemporalInfo(track->qtTrack, track->sampleToExtract, &mediaTime, &compositionTimeOffset, &duration);
		if (err)
            BAIL(kFskErrEndOfFile);

		err = QTTrackLoadSample(track->qtTrack, track->sampleToExtract, &data, &dataSize);
		BAIL_IF_ERR(err);

		err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
		BAIL_IF_ERR(err);

		info->samples = 1;
		info->sampleSize = dataSize;
		info->flags = (track->sampleToExtract == prevSyncSample) ? kFskImageFrameTypeSync : kFskImageFrameTypeDifference;
		t = mediaTime;
		t -= track->editMediaTime;
		info->decodeTime = (FskInt64)((t / (double)track->qtTrack->media->scale) * (double)state->scale);
		info->sampleDuration = (UInt32)(((double)duration / (double)track->qtTrack->media->scale) * (double)state->scale);
		if (0 == compositionTimeOffset)
			info->compositionTime = -1;
		else {
			t = mediaTime;
			t += (SInt32)compositionTimeOffset;
			t -= track->editMediaTime;
			info->compositionTime = (FskInt64)((t / (double)track->qtTrack->media->scale) * (double)state->scale);
		}

		infoCount = 1;

		track->lastExtractedTime = (UInt32)(((double)(mediaTime + duration) / (double)track->qtTrack->media->scale) * (double)state->scale);
		track->sampleToExtract += 1;
	}
	else {
		UInt32 chunkNumber, firstChunkSample, samplesIntoChunk, uniformSizeCount;
		QTSampleToChunkRecord stc;
		QTFileOffset chunkOffset, chunkSize;

		err = QTTrackGetSampleTemporalInfo(track->qtTrack, track->sampleToExtract, &mediaTime, NULL, &duration);
		if (err)
            BAIL(kFskErrEndOfFile);

		err = QTTrackSampleToChunk(track->qtTrack, track->sampleToExtract, &chunkNumber, &chunkOffset, &firstChunkSample, &stc);
		BAIL_IF_ERR(err);

		samplesIntoChunk = track->sampleToExtract - firstChunkSample;
		if (0 != samplesIntoChunk) {
			QTFileOffset offsetIntoChunk;

			stc.samplesPerChunk -= samplesIntoChunk;
			err = QTTrackGetSampleSizes(track->qtTrack, firstChunkSample, samplesIntoChunk, &offsetIntoChunk);
			BAIL_IF_ERR(err);

			chunkOffset += offsetIntoChunk;
		}

		while (true) {
			err = QTTrackGetSampleSizes(track->qtTrack, track->sampleToExtract, stc.samplesPerChunk, &chunkSize);
			BAIL_IF_ERR(err);

			if (track->audio.compressed) {
				if (((chunkSize < 64L * 1024L) && (stc.samplesPerChunk < 32)) || (1 == stc.samplesPerChunk))
					break;
			}
			else {
				if ((chunkSize < 64L * 1024L) || (1 == stc.samplesPerChunk))
					break;
			}

			stc.samplesPerChunk >>= 1;
		}

		dataSize = (UInt32)chunkSize;

		err = QTTrackGetChunkSamplesOfSameSize(track->qtTrack, track->sampleToExtract, &uniformSizeCount);
		BAIL_IF_ERR(err);

		err = FskMemPtrNew(dataSize + kFskDecompressorSlop, &data);
		BAIL_IF_ERR(err);

		err = qtRead(state, data, chunkOffset, (UInt32)dataSize);
		BAIL_IF_ERR(err);

		if( (0xffffffff==track->level) && (0 == FskStrCompare(track->format, "x-audio-codec/aac") || 0 == FskStrCompare(track->format, "x-audio-codec/qcelp") ) )
		{
		    UInt32 sample_rate_ext = 0;
            SInt32 sbr_flag = 0;
		    get_extended_aac_profile_level(data, track->audio.sampleRate, track->audio.channelCount, &sample_rate_ext, &sbr_flag, &track->profile, &track->level);
			if( track->audio.sampleRate != sample_rate_ext && sample_rate_ext != 0 ) {
				track->audio.sampleRate = sample_rate_ext;
			}
		}

		if (uniformSizeCount < stc.samplesPerChunk) {
			// not all the same
			UInt32 i;

			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord) * stc.samplesPerChunk, &info);
			BAIL_IF_ERR(err);

			for (i = 0; i < stc.samplesPerChunk; i++) {
				QTFileOffset size64;
				QTTrackGetSampleSizes(track->qtTrack, track->sampleToExtract + i, 1, &size64);

				info[i].samples = 1;
				info[i].sampleSize = (UInt32)size64;
				info[i].flags = kFskImageFrameTypeSync;
				QTTrackGetSampleTemporalInfo(track->qtTrack, track->sampleToExtract + i, &mediaTime, NULL, &duration);
				info[i].decodeTime = (UInt32)(((double)mediaTime / (double)track->qtTrack->media->scale) * (double)state->scale);
				info[i].sampleDuration = (UInt32)(((double)track->audio.durationPerFrame / (double)track->qtTrack->media->scale) * (double)state->scale);;
				info[i].compositionTime = -1;
			}
			infoCount = stc.samplesPerChunk;
		}
		else {
			// all the same
			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			BAIL_IF_ERR(err);

			info->samples = stc.samplesPerChunk;
			info->sampleSize = dataSize / stc.samplesPerChunk;
			info->flags = kFskImageFrameTypeSync;
			info->decodeTime = (UInt32)(((double)mediaTime / (double)track->qtTrack->media->scale) * (double)state->scale);
			info->sampleDuration = (UInt32)(((double)track->audio.durationPerFrame / (double)track->qtTrack->media->scale) * (double)state->scale);;
			info->compositionTime = -1;

			infoCount = 1;
		}

		if (kFskErrNone != QTTrackSampleToTime(track->qtTrack, track->sampleToExtract + stc.samplesPerChunk, &mediaTime))
			mediaTime = track->qtTrack->media->duration;

		track->lastExtractedTime = (UInt32)(((double)(mediaTime) / (double)track->qtTrack->media->scale) * (double)state->scale);
		track->sampleToExtract += stc.samplesPerChunk;
	}

	if ((track->sampleToExtract >= track->qtTrack->media->sampleCount) ||
        (state->hasEndTime && (lastExtractedTime >= state->endTime)))  {
		info[infoCount - 1].flags |= kFskImageFrameEndOfMedia;
		track->atEnd = 2;
	}

	*infoOut = info;
	*infoCountOut = infoCount;
	*dataOut = data;
	*trackOut = &track->readerTrack;

bail:
	if (kFskErrNone != err) {
		if (NULL != track) {
			FskMemPtrDispose(data);
			FskMemPtrDispose(info);
			if ((kFskErrNeedMoreTime != err) && !track->atEnd)
				track->atEnd = true;

			// only return end of file error when all tracks are done.
			if (err == kFskErrEndOfFile) {
				for (walker = state->tracks; NULL != walker; walker = walker->next) {
					if (!state->scrub || (kFskMediaReaderMP4Audio != walker->mediaType)) {
						if (!walker->atEnd)
							err = kFskErrNeedMoreTime;
					}
				}
			}
		}
	}

	return err;
}

FskErr mp4ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue valueOut, UInt32 *flags)
{
	mp4Reader state = readerState;
	FskErr err = kFskErrUnknownElement;
	QTUserData userData;
	UInt32 tag;
	FskMediaPropertyValueRecord meta;

	if (!state->movie)
		goto bail;

	if (1 != index)
		return -1;

	if (flags) *flags = 0;

	userData = state->movie->userData;
	if (userData) {
		// quicktime
		if (kFskErrNone == FskMediaMetaDataFskTagToFormatTag(metaDataType, kFskMetaDataFormatQuickTime, (void **)(void *)&tag, &meta.type, NULL)) {
			if (kFskMediaPropertyTypeString == meta.type) {
				if (0 == QTUserDataGetText(userData, tag, (void **)(void *)&meta.value.str))
					goto done;
			}
			else {
				if (0 == QTUserDataGet(userData, tag, 1, &meta.value.data.data, &meta.value.data.dataSize))
					goto done;
			}
		}

		// mp4
		if (kFskErrNone == FskMediaMetaDataFskTagToFormatTag(metaDataType, kFskMetaDataFormatMP4, (void **)(void *)&tag, &meta.type, NULL)) {
			if (0 == QTUserDataGetTextMP4(userData, tag, (void **)(void *)&meta.value.str))
				goto done;
		}

		// iTunes
		if (kFskErrNone == FskMediaMetaDataFskTagToFormatTag(metaDataType, kFskMetaDataFormatiTunes, (void **)(void *)&tag, &meta.type, NULL)) {
			FskErr tErr;

			if (kFskMediaPropertyTypeString == meta.type) {
				tErr = QTUserDataGetTextiTunes(userData, tag, (void **)(void *)&meta.value.str);
				if (0 == tErr) {
					// make sure this is UTF-8 text. if not, truncate at first bad character.
					unsigned char *p = (unsigned char *)meta.value.str;
					while (*p) {
						SInt32 advance = 0;
						unsigned char c = *p;

						if (c < 0x080)
							advance = 1;
						else if (c < 0xc0)
							advance = 0;
						else if (c < 0xe0) {
							if (0x80 == (p[1] & 0xc0))
								advance = 2;
						}
						else if ((0x80 == (p[1] & 0xc0)) && (0x80 == (p[2] & 0xc0)))
							advance = 3;

						if (0 == advance) {
							*p = 0;
							break;
						}

						p += advance;
					}
				}
				else
				if (kiTunesUserDataGenre == tag) {
					tag = kiTunesUserDataGenreAsInteger;
					tErr = QTUserDataGetiTunes(userData, tag, (void **)(void *)&meta.value.data.data, &meta.value.data.dataSize);		// try the other genre tag
				}
			}
			else
				tErr = QTUserDataGetiTunes(userData, tag, (void **)(void *)&meta.value.data.data, &meta.value.data.dataSize);

			if (0 == tErr) {
				if ('trkn' == tag) {
					UInt32 trackNum = FskEndianU32_BtoN(*(UInt32 *)meta.value.str);
					FskMemPtrDispose(meta.value.str);
					FskMemPtrNew(128, (FskMemPtr*)(void*)(&meta.value.str));
					FskStrNumToStr(trackNum, (char *)meta.value.str, 128);
				}
				else if ('covr' == tag) {
					unsigned char *cap = (unsigned char *)meta.value.data.data;
					char *mime;
					UInt32 mimeLen;

					if ((0x89 == cap[0]) && ('P' == cap[1]) && ('N' == cap[2]) && ('G' == cap[3]))
						mime = "image/png";
					else
						mime = "image/jpeg";
					mimeLen = FskStrLen(mime) + 1;

					err = FskMemPtrRealloc(meta.value.data.dataSize + mimeLen, (FskMemPtr*)(void*)(&meta.value.data.data));
					if (err) {
						FskMemPtrDispose(meta.value.data.data);
						goto bail;
					}

					FskMemMove((char *)meta.value.data.data + mimeLen, meta.value.data.data, meta.value.data.dataSize);
					FskMemMove(meta.value.data.data, mime, mimeLen);
					meta.value.data.dataSize += mimeLen;
				}
				else if ('tmpo' == tag) {
					UInt32 bpm = FskEndianU16_BtoN(*(UInt16 *)meta.value.str);
					FskMemPtrDispose(meta.value.str);
					FskMemPtrNew(128, (FskMemPtr*)(void*)(&meta.value.str));
					FskStrNumToStr(bpm, (char*)(void*)(meta.value.str), 128);
				}
				else if (kiTunesUserDataGenreAsInteger == tag) {
					UInt32 genre = FskEndianU16_BtoN(*(UInt16 *)meta.value.str);
					if (genre)
						genre -= 1;		// looks like itunes is off-by-one
					FskMemPtrDispose(meta.value.str);
					FskMemPtrNew(128, (FskMemPtr*)(void*)(&meta.value.str));
					FskStrCopy(meta.value.str, "(");
					FskStrNumToStr(genre, (char *)meta.value.str + 1, 128);
					FskStrCat(meta.value.str, ")");
					meta.type = kFskMediaPropertyTypeString;
				}
				goto done;
			}
		}

		// id3-in-mp4
		if (kFskErrNone == FskMediaMetaDataFskTagToFormatTag(metaDataType, kFskMetaDataFormatID3v23, (void **)(void *)&tag, &meta.type, NULL) &&
				(kFskMediaPropertyTypeString == meta.type)) {
			UInt32 uuidIndex = 0;
			const unsigned char uuid[] = {0xa0, 0x5b, 0x10, 0xe2, 0x4e, 0xa6, 0x47, 0x19, 0x9d, 0x69, 0x08, 0x61, 0xe6, 0x10, 0xff, 0x2d};

			while (true) {
				unsigned char *udData;
				UInt32 udSize, i;
				Boolean match = true;

				if (0 != QTUserDataGet(userData, 'uuid', ++uuidIndex, (void **)(void *)&udData, &udSize))
					break;

				for (i=0; i<16; i++) {
					if (uuid[i] != udData[i]) {
						match = false;
						break;
					}
				}

				if ((false == match) || (udSize < 23) || (FskEndianU32_BtoN(*(UInt32 *)&udData[16]) != tag) || (3 != udData[22])) {
					FskMemPtrDispose(udData);
					continue;
				}

				FskMemMove(udData, udData + 23, udSize - 23);
				udData[udSize - 23] = 0;
				meta.value.str = (char *)udData;

				goto done;
			}
		}
	}

	// memory stick video
	if (state->movie->mtdt && (kFskErrNone == FskMediaMetaDataFskTagToFormatTag(metaDataType, kFskMetaDataFormatMemoryStickVideo, (void **)(void *)&tag, &meta.type, NULL))) {
		QTMTDT mtdt;

#if 0		//@@
		if (-1 == tag) {
			// album art
			Boolean foundArt = false;
			if (!state->isURL) {
				char *dot = FskStrRChr(state->dataSource, '.');
				if (dot && ((FskStrLen(state->dataSource) - (dot - state->dataSource)) == 4)) {
					char *tmp = FskStrDoCopy(state->dataSource);
					FskMemPtr file;
					FskInt64 fileSize;
					UInt32 artSize;
					FskStrCopy(&tmp[dot - state->dataSource], ".THM");
					if (kFskErrNone == FskFileLoad(tmp, &file, &fileSize)) {
						artSize = (UInt32)fileSize + 11;
						if (kFskErrNone == FskMemPtrNew(artSize, (FskMemPtr *)&meta.value.data.data)) {
							FskStrCopy(meta.value.data.data, "image/jpeg");
							FskMemMove(11 + (char *)meta.value.data.data, file, (UInt32)fileSize);
							meta.value.data.dataSize = artSize;
							foundArt = true;
						}
						FskMemPtrDispose(file);
					}
					FskMemPtrDispose(tmp);
				}
			}
			if (foundArt)
				goto done;
		}
#endif

		for (mtdt = state->movie->mtdt; NULL != mtdt; mtdt = mtdt->next) {
			if (mtdt->dataType == tag) {
				if (1 == mtdt->encoding) {
					FskTextUnicode16BEToUTF8((UInt16 *)mtdt->data, mtdt->dataSize, (char **)&meta.value.str, NULL);
					meta.type = kFskMediaPropertyTypeString;
				}
				else {
					FskMemPtrNewFromData(mtdt->dataSize, mtdt->data, (FskMemPtr*)(void*)(&meta.value.data.data));
					meta.value.data.dataSize = mtdt->dataSize;
					meta.type = kFskMediaPropertyTypeData;
				}
				goto done;
			}
		}
	}

bail:
	return err;

done:
	*valueOut = meta;
	return kFskErrNone;
}

FskErr mp4ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	UInt32 *atom = (UInt32 *)data;
	UInt32 size, type;
	Boolean _3gpp = false, _3gpp2 = false;

	if (dataSize < 8)
		return kFskErrUnknownElement;

	size = FskEndianU32_BtoN(atom[0]);
	type  = FskEndianU32_BtoN(atom[1]);
	if ((8 == size) && ('wide' == type))
		;													// the classic wide atom
	else {
		if ((size != 1) && (size < 8))
			return kFskErrUnknownElement;					// size of 1 could be 64 bit atom header, otherwise there is no valid size under 8... we check against 9 because the atom types we are looking for must have some content

		if (('free' != type) && (size == 8))				// only 'free' atom can be just 8 bytes
			return kFskErrUnknownElement;					
		
		if (('moov' != type) && ('ftyp' != type) && ('mdat' != type) && ('free' != type) && ('pnot' != type))
			return kFskErrUnknownElement;

		if (('ftyp' == type) && (dataSize >= size)) {
			SInt32 count = (size / 4) - 4;
			UInt32 *t = &atom[4];
			while (count > 0) {
				UInt32 ftype = *t++;
				ftype = FskEndianU32_BtoN(ftype);
				if (('3gp4' == ftype) || ('3gp5' == ftype) || ('3gp6' == ftype) || ('3gp7' == ftype) || ('3gp6' == ftype)) {
					_3gpp = true;
					break;
				}
				else if (('3g2a' == ftype) ||  ('3g2b' == ftype) ||  ('3g2c' == ftype) ||  ('3gp6' == ftype)) {
					_3gpp2 = true;
					break;
				}
				else if (('iso2' == ftype) ||('isom' == ftype) ||('mmp4' == ftype) ||('mp41' == ftype) ||('mp42' == ftype) ||('avc1' == ftype))
					break;		// video/mp4
				count -= 1;
			}

		}
	}

	if (_3gpp)
		*mime = FskStrDoCopy("video/3gpp");
	else if (_3gpp2)
		*mime = FskStrDoCopy("video/3gpp2");
	else
		*mime = FskStrDoCopy("video/mp4");

	return kFskErrNone;
}

/*
	reader properties
*/

FskErr mp4ReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = state->duration;

	return kFskErrNone;
}

FskErr mp4ReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr mp4ReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

FskErr mp4ReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr mp4ReaderGetFlags(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;
	mp4ReaderTrack walker;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 0;

	for (walker = state->tracks; NULL != walker; walker = walker->next) {
		UInt32 mediaType = walker->mediaType;

		if ((kFskMediaReaderMP4Video == mediaType) && walker->qtTrack->enabled)
			property->value.integer |= kFskMediaPlayerFlagHasVideo;
		else
		if ((kFskMediaReaderMP4Audio == mediaType) && walker->qtTrack->enabled)
			property->value.integer |= kFskMediaPlayerFlagHasAudio;
		else
		if (kFskMediaReaderMP4Pano == mediaType)
			property->value.integer |= kFskMediaPlayerFlagHasQTPanorama;
		else
		if (kFskMediaReaderMP4Object == mediaType)
			property->value.integer |= kFskMediaPlayerFlagHasQTObject;
	}

	if (state->isM4B)
		property->value.integer |= kFskMediaPlayerFlagHasAudioBook;

	return kFskErrNone;
}

FskErr mp4ReaderSetScrub(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	state->scrub = property->value.b;

	return kFskErrNone;
}

FskErr mp4ReaderGetRedirect(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	if (!state->redirect)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(state->redirect);

	return kFskErrNone;
}

FskErr mp4ReaderGetError(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->theError;

	return kFskErrNone;
}

FskErr mp4ReaderGetSeekableSegment(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	if (!(kFskMediaSpoolerDownloading & state->spooler->flags) || !state->downloadTimeToOffsetEntry)
		return kFskErrUnimplemented;

	while (state->downloadTimeToOffsetEntry < &state->timeToOffset->entry[state->timeToOffset->entryCount]) {
		if (state->spoolerPosition < *state->downloadTimeToOffsetEntry)
			break;

		state->downloadTimeToOffsetEntry += 1;
		if (state->timeToOffset->entryCount == (UInt32)(state->downloadTimeToOffsetEntry - state->timeToOffset->entry))
			state->downloadMaxTime = state->duration / (double)state->scale;
		else
			state->downloadMaxTime = state->timeToOffset->secondsPerEntry *
							(state->downloadTimeToOffsetEntry - state->timeToOffset->entry - 1);
	}

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = state->downloadMaxTime;

	return kFskErrNone;
}


FskErr mp4ReaderSetBitRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4Reader state = readerState;

	state->availableBitRate = property->value.integer;

	return kFskErrNone;
}

FskErr mp4ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "audio/mp4\000video/mp4\000video/quicktime\000video/3gpp\000audio/3gpp\000video/3gpp2\000audio/3gpp2\000audio/mp4-alac\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr mp4ReaderTrackGetMediaType(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeString;
	switch (track->mediaType) {
		case kFskMediaReaderMP4Audio:
			property->value.str = FskStrDoCopy("audio");
			break;

		case kFskMediaReaderMP4Video:
			property->value.str = FskStrDoCopy("video");
			break;

		case kFskMediaReaderMP4Hint:
			property->value.str = FskStrDoCopy("hint");
			break;

		case kFskMediaReaderMP4Pano:
			property->value.str = FskStrDoCopy("qtpano");
			break;

		case kFskMediaReaderMP4Object:
			property->value.str = FskStrDoCopy("qtobject");
			break;

		default:
			property->value.str = FskStrDoCopy("unknown");
			break;
	}

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetFormat(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetSampleRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.sampleRate;

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	if (!track->state->duration || !track->qtTrack->media || !track->qtTrack->media->scale)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = (SInt32)((track->dataSize / ((double)track->qtTrack->media->duration / (double)track->qtTrack->media->scale)) * 8.0);

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetFrameRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;
	double fps;

	if (!track->state->duration || !track->qtTrack->media || !track->qtTrack->media->sampleCount)
		return kFskErrUnimplemented;

	fps = ((double)track->qtTrack->media->sampleCount) / ((double)track->qtTrack->duration / (double)track->state->movie->scale);

	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = (SInt32)(1000 * fps);
	property->value.ratio.denom = 1000;

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetFormatInfo(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	if (NULL == track->formatInfo)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = track->formatInfoSize;
	return FskMemPtrNewFromData(track->formatInfoSize, track->formatInfo, (FskMemPtr*)(void*)(&property->value.data.data));
}

FskErr mp4ReaderTrackGetChannelCount(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channelCount;

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetDimensions(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension = track->dimensions;

	return kFskErrNone;
}


FskErr mp4ReaderTrackGetProfile(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = state;
	int profile = track->profile;
	int level   = track->level;
	const char *profile_str;
	char level_str[16];

	if (kFskMediaReaderMP4Video == track->mediaType)
	{
		if( 0 == FskStrCompare(track->format, "x-video-codec/avc"))
		{
				 if( profile == 66 )		profile_str = "baseline,";
			else if( profile == 77 )		profile_str = "main,";
			else if( profile == 88 )		profile_str = "extended,";
			else if( profile == 100 )		profile_str = "high,";
			else if( profile == 110 )		profile_str = "high 10,";
			else if( profile == 122 )		profile_str = "high 4:2:2,";
			else if( profile == 244 )		profile_str = "high 4:4:4,";
			else
				profile_str = "unknown,";
		}
		else if( 0 == FskStrCompare(track->format, "x-video-codec/mp4"))
		{
			if( profile == 0x08 )
			{
				profile_str = "simple,";
				level = 0;
			}
			else if( (profile>>2)== 0 && (profile&0x03)!= 0 )
			{
				profile_str = "simple,";
				level = profile&0x03;
			}
			else if( profile == 0x32 )
			{
				profile_str = "main,";
				level = 2;
			}
			else if( profile == 0x33 )
			{
				profile_str = "main,";
				level = 3;
			}
			else if( profile == 0x34 )
			{
				profile_str = "main,";
				level = 4;
			}
			else if( (profile>>3)== 0x1e && (profile&0x07)<= 5 )
			{
				profile_str = "advance simple,";
				level = profile&0x07;
			}
			else
				profile_str = "unknown,";
		}
		else
			profile_str = "unknown,";
	}
	else if (kFskMediaReaderMP4Audio == track->mediaType)
	{
		if( 0 == FskStrCompare(track->format, "x-audio-codec/aac") || 0 == FskStrCompare(track->format, "x-audio-codec/qcelp") )
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
	}
	else
		profile_str = "unknown,";

	FskStrNumToStr( level, level_str, sizeof(level_str));
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCat( profile_str, level_str );

	return kFskErrNone;
}

FskErr mp4ReaderTrackGetRotation(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	mp4ReaderTrack track = readerState;
	FskFixed *matrix = (FskFixed *)track->qtTrack->matrix;

	property->type = kFskMediaPropertyTypeFloat;
	if ((matrix[0] == -FskIntToFixed(1)) && (matrix[4] == -FskIntToFixed(1)) && (matrix[8] == (1 << 30)))
		property->value.number = 180;
	else
		property->value.number = 0;

	return kFskErrNone;
}

/*
	local
*/

FskErr mp4Instantiate(mp4Reader state)
{
	FskErr err;
	QTTrack walker;
	SInt32 playableVideoCount = 0;

	if (0 == state->moov.size) {
		err = QTMovieNewFromReader(&state->movie, false, qtRead, state, qtAlloc, qtFree, NULL);
		BAIL_IF_ERR(err);
	}
	else {
		err = QTMovieNewFromReader(&state->movie, false, qtMemRead, state, qtAlloc, qtFree, NULL);
		BAIL_IF_ERR(err);

		state->movie->reader = qtRead;
	}

	if (NULL != state->movie->refMovieDescriptor) {
		QTReferenceMovieDescriptor desc = NULL, walker;
		UInt32 availableDataRate = (state->availableBitRate ? state->availableBitRate : 384000) / 10;		// data rates in Descritor are all divided by 10
		Boolean wantsMobile = false;

		// check to see if mobile is set. if so, we prefer that. The iPhone exporter (export to web) in QuickTime 7.7 doesn't properly set the dataRate and this is the workaround to mimic their behavior.
		for (walker = state->movie->refMovieDescriptor; NULL != walker; walker = walker->next)
			wantsMobile = wantsMobile || walker->wantsMobile;

		for (walker = state->movie->refMovieDescriptor; NULL != walker; walker = walker->next) {
			if (wantsMobile && !walker->wantsMobile)
				continue;

			if (!desc)
				desc = walker;
			else
			if (walker->dataRate <= availableDataRate) {
				if ((walker->dataRate > desc->dataRate) || (desc->dataRate > availableDataRate))
					desc = walker;
			}
			else if (walker->dataRate < desc->dataRate)
				desc = walker;
		}

		if (desc && desc->dataRef) {
			err = FskMemPtrNewFromData(desc->dataRefSize, desc->dataRef, &state->redirect);
			BAIL_IF_ERR(err);

			state->redirect[desc->dataRefSize] = 0;

			if (state->uri && (NULL == FskStrChr(state->redirect, ':'))) {
				// relative
				char *end = state->uri + FskStrLen(state->uri);
				char *c, save, *slash;

				c = FskStrChr(state->uri, '?');
				if (NULL != c)
					end = c;
				save = *end;
				*end = 0;

				slash = FskStrRChr(state->uri, '/');
				*end = save;

				if (NULL != slash) {
					char *redirect;
					save = slash[1];
					slash[1] = 0;
					redirect = FskStrDoCat(state->uri, state->redirect);
					slash[1] = save;
					FskMemPtrDispose(state->redirect);
					state->redirect = redirect;
				}
			}

			goto doneTracks;
		}

		return kFskErrUnimplemented;
	}

	for (walker = state->movie->tracks; NULL != walker; walker = walker->next) {
		mp4ReaderTrack track;

		err = FskMemPtrNewClear(sizeof(mp4ReaderTrackRecord), &track);
		BAIL_IF_ERR(err);

		track->readerTrack.dispatch = &gMP4ReaderUnknownTrack;
		track->readerTrack.state = track;
		track->qtTrack = walker;
		track->state = state;

		if (kQTSoundType == walker->media->mediaType) {
			track->mediaType = kFskMediaReaderMP4Audio;
			track->readerTrack.dispatch = &gMP4ReaderAudioTrack;

			track->audio.soundDesc = QTTrackGetIndSampleDescription(track->qtTrack, 1);
			track->audio.sampleRate = track->audio.soundDesc->sampleRate >> 16;
			track->audio.channelCount = track->audio.soundDesc->numChannels;
			track->audio.compressed = true;
			switch (track->audio.soundDesc->dataFormat) {
				case 'twos':
					if (16 == track->audio.soundDesc->sampleSize)
						 track->format = "x-audio-codec/pcm-16-be";
					 else
						 track->format = "x-audio-codec/pcm-8-twos";
					track->audio.compressed = false;
					break;
				case 'sowt':
					track->format = "x-audio-codec/pcm-16-le";
					track->audio.compressed = false;
					break;
				case 'raw ':
					track->format = "x-audio-codec/pcm-8-offset";
					track->audio.compressed = false;
					break;
				case '.mp3':
					track->format = "x-audio-codec/mp3";
					break;
				case 'mp4a':
					track->format = "x-audio-codec/aac";
					track->formatInfo = QTAudioSampleDescriptionGetESDS(track->audio.soundDesc, &track->formatInfoSize);
					if (NULL != track->formatInfo) {
						UInt8 esdsCodec;
						UInt32 audioType, esdsSampleRate, esdsNumChannels;

						if (false == QTESDSScanAudio(track->formatInfo, track->formatInfoSize, &esdsCodec, &audioType, &esdsSampleRate, &esdsNumChannels))
							track->format = NULL;
						else {
							//http://www.mp4ra.org/object.html
							if (0x040 == esdsCodec || 0x067 == esdsCodec) {
								if (esdsSampleRate)
									track->audio.sampleRate = esdsSampleRate;
								if (esdsNumChannels)
									track->audio.channelCount = esdsNumChannels;

								track->audio.durationPerFrame = 1024;			// at this level, we don't ever know if it is aac+
							}
							else if (0x0e1 == esdsCodec) {
								track->audio.sampleRate = 8000;
								track->audio.channelCount = 1;
								track->format = "x-audio-codec/qcelp";
							}
							else
								track->format = NULL;

							track->profile = audioType;
							track->level = 0xffffffff;	//invalid
						}
					}
					break;
				case 'samr': {
					unsigned char *ext = QTAudioSampleDescriptionGetExtension(track->audio.soundDesc, 'wave');

					track->format = NULL;
					if (ext) {
						// amr in a QuickTime movie container
						SInt32 count = FskMisaligned32_GetN(ext) - 8;
						UInt32 *p = (UInt32 *)(ext + 8);

						while (count > 8) {
							UInt32 c = FskMisaligned32_GetBtoN(&p[0]);
							UInt32 tag = FskMisaligned32_GetBtoN(&p[1]);
							if (!c || !tag)
								break;

							if (('samr' == tag) && (c >= 16)) {
								track->format = "x-audio-codec/amr-nb";
								track->audio.durationPerFrame = 160 * *(16 + (UInt8 *)p);	// frames_per_sample from AMR decoder specific box
								track->audio.channelCount = 1;
								track->audio.sampleRate = 8000;
								break;
							}
							count -= c;
							p = (UInt32 *)(c + (char *)p);

						}
					}
					else if (track->audio.soundDesc->descSize >= (36 + 8 + 9)) {
						// amr in a 3gp/mp4 container
						track->format = "x-audio-codec/amr-nb";
						track->audio.durationPerFrame = 160 * *(UInt8 *)((36 + 8 + 8) + (char *)track->audio.soundDesc);	// frames_per_sample from AMR decoder specific box
						track->audio.channelCount = 1;
						track->audio.sampleRate = 8000;
					}
					}
					break;
				case 'sawb':
					if (track->audio.soundDesc->descSize >= (36 + 8 + 9)) {
						track->format = "x-audio-codec/amr-wb";
						track->audio.durationPerFrame = 160 * *(UInt8 *)((36 + 8 + 8) + (char *)track->audio.soundDesc);	// frames_per_sample from AMR decoder specific box
						track->audio.channelCount = 1;
						track->audio.sampleRate = 16000;
					}
					else
						track->format = NULL;
					break;

				case 'drms':
				case 'drmv':
				case 'drmi':
					track->unplayable = true;
					state->hasDRM = true;
					break;

				case 'QDM2':
					track->format = "x-audio-codec/qdesign-music";
					break;

				case 'alac':
					track->format = "x-audio-codec/apple-lossless";
					track->formatInfo = QTAudioSampleDescriptionGetALAC(track->audio.soundDesc, &track->formatInfoSize);
					if (NULL != track->formatInfo) {
                        UInt8	bitDepth, numChannels;
                        UInt32	frameLength, maxFrameBytes,avgBitRate,sampleRate;

						if (false == QTALACScanAudio(track->formatInfo, track->formatInfoSize, &frameLength, &bitDepth, &numChannels, &maxFrameBytes, &avgBitRate, &sampleRate))
							track->format = NULL;
						else {
							track->audio.sampleRate = sampleRate;
							track->audio.channelCount = numChannels;
						}
					}
					break;
			}

			if (NULL == track->format)
				track->unplayable = true;
			else
				track->format = FskStrDoCopy(track->format);
		}
		else
		if (kQTVideoType == walker->media->mediaType) {
			track->mediaType = kFskMediaReaderMP4Video;
			track->readerTrack.dispatch = &gMP4ReaderVideoTrack;

			track->video.videoDesc = QTTrackGetIndSampleDescription(track->qtTrack, 1);
			track->dimensions.width = track->video.videoDesc->width;
			track->dimensions.height = track->video.videoDesc->height;

			switch (track->video.videoDesc->cType) {
				case 'jpeg':
					track->format = "image/jpeg";
					break;
				case 'png ':
					track->format = "image/png";
					break;
				case 'mp4v': {
					unsigned char *esds;

					track->format = "x-video-codec/mp4";
					track->formatInfo = (unsigned char *)track->video.videoDesc;
					track->formatInfoSize = track->video.videoDesc->idSize;

					esds = QTVideoSampleDescriptionGetExtension(track->video.videoDesc, 'esds');
					if (NULL != esds) {
						UInt32 esdsWidth, esdsHeight;
						UInt8 profile_level;

						if (QTESDSScanVideo(esds + 8, FskMisaligned32_GetN(esds), &esdsWidth, &esdsHeight, &profile_level))
						{
							track->dimensions.width = esdsWidth;
							track->dimensions.height = esdsHeight;
							track->profile = profile_level;
						}
					}
					}
					break;
				case 'avc1':
					track->format = "x-video-codec/avc";
					track->formatInfo = (unsigned char *)track->video.videoDesc;
					track->formatInfoSize = track->video.videoDesc->idSize;
					{
						unsigned char	*avcc_data = NULL;
						avcc_data = (unsigned char *)QTVideoSampleDescriptionGetExtension((QTImageDescription)track->video.videoDesc, 'avcC');
						if( avcc_data != NULL )
						{
							track->profile = avcc_data[9];
							track->level   = avcc_data[11];
						}
					}
					break;
				case 'h263':
				case 's263':
					track->format = "x-video-codec/263";
					break;
				case 'drms':
				case 'drmv':
				case 'drmi':
					track->format = "x-video-codec/drm-protected-video";
					track->unplayable = true;
					state->hasDRM = true;
					break;
				case 'SVQ1':
					track->format = "x-video-codec/sorenson-video-1";
					break;
				case 'SVQ3':
					track->format = "x-video-codec/sorenson-video-3";
					break;
				case 'cvid':
					track->format = "x-video-codec/cinepak";
					break;
				case 'smc ':
					track->format = "x-video-codec/apple-graphics";
					break;
				case 'rle ':
					track->format = "x-video-codec/apple-animation";
					break;
				default:
					track->format = "x-video-codec/unknown";
					break;
			}

			if (kFskErrNone != FskImageDecompressNew(NULL, 0, track->format, NULL))
				track->missingCodec = true;

			if (!track->unplayable && !track->missingCodec)
				playableVideoCount += 1;

			track->format = FskStrDoCopy(track->format);
		}
		else if ((kQTPanoramaType == walker->media->mediaType) || ('STpn' == walker->media->mediaType))
			track->mediaType = kFskMediaReaderMP4Pano;
		else if ('obje' == walker->media->mediaType)
			track->mediaType = kFskMediaReaderMP4Object;
		else if ('hint' == walker->media->mediaType)
			track->mediaType = kFskMediaReaderMP4Hint;
		else if ('strm' == walker->media->mediaType) {
			UInt32 urlSize;

			track->mediaType = kFskMediaReaderMP4Unknown;

			FskMemPtrDisposeAt(&state->redirect);

			err = QTTrackLoadSample(track->qtTrack, 1, (void **)(void *)&state->redirect, &urlSize);
			BAIL_IF_ERR(err);

			err = FskMemPtrRealloc(urlSize + 1, (FskMemPtr*)(void*)(&state->redirect));
			if (err) {
				FskMemPtrDispose(state->redirect);
				goto bail;
			}
			state->redirect[urlSize] = 0;
		}
		else {
			FskMemPtrDispose(track);
			continue;
		}

		QTTrackGetSampleSizes(walker, 1, walker->media->sampleCount, &track->dataSize);

		FskListAppend((FskList*)(void*)(&state->tracks), track);
	}

	if (playableVideoCount > 1) {
		// we can really only handle a single video track at a time - so find the best one (as defined by being playable and having the most samples)
		UInt32 maxSamples = 0;
		mp4ReaderTrack best = NULL, walker, next;

		for (walker = state->tracks; NULL != walker; walker = walker->next) {
			QTMedia media = walker->qtTrack->media;

			if ((kQTVideoType != media->mediaType) || walker->unplayable || walker->missingCodec)
				continue;

			if (media->sampleCount < maxSamples)
				continue;

			best = walker;
			maxSamples = media->sampleCount;
		}

		for (walker = state->tracks; NULL != walker; walker = next) {
			QTMedia media = walker->qtTrack->media;

			next = walker->next;

			if (kQTVideoType != media->mediaType)
				continue;

			if (walker == best)
				continue;

			FskListRemove(&state->tracks, walker);
			FskMemPtrDispose(walker->format);
			FskMemPtrDispose(walker);
		}
	}

doneTracks:
	state->scale = 2997;
	state->duration = (double)state->scale * ((double)state->movie->duration / (double)state->movie->scale);

    {
    mp4ReaderTrack walker;
    double dataSize = 0;

	for (walker = state->tracks; NULL != walker; walker= walker->next)
        dataSize += (double)walker->dataSize;
        
    state->bitRate = (UInt32)((dataSize * 8) / (((double)state->movie->duration / (double)state->movie->scale)));
    }

	if (kFskMediaSpoolerIsNetwork & state->spooler->flags) {
		err = buildTimeToOffsetTable(state);
		BAIL_IF_ERR(err);

#define skewLimit 8	
		if (state->timeToOffset->hasMediaSamplesOutOfOrder || (state->timeToOffset->maxGap > (skewLimit * (state->bitRate / 8)))) {		// 5 second skew limit
			state->forceDownload = true;

			if (!(kFskMediaSpoolerDownloading & state->spooler->flags)) {
				// re-open for download

				if (state->spoolerOpen && state->spooler->doClose) {
					(state->spooler->doClose)(state->spooler);
					state->spoolerOpen = false;
				}

				err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
				BAIL_IF_ERR(err);

				state->spoolerOpen = true;
			}
			else
				state->downloadTimeToOffsetEntry = &state->timeToOffset->entry[0];
		}
	}

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	if (kFskErrNone != err) {
		QTMovieDispose(state->movie);
		state->movie = NULL;

		if (kFskErrNeedMoreTime != err) {
			state->theError = err;
			(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
		}
	}

	return err;
}

FskErr mp4SpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	mp4Reader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationGetURI:
			state->spoolerPosition = ((FskMediaSpoolerGetURI)param)->position;
			break;

		case kFskMediaSpoolerOperationGetHeaders:
			if (state->forceDownload)
				state->spooler->flags |= kFskMediaSpoolerDownloadPreferred;

			state->downloadMaxTime = 0;
			if (state->timeToOffset)
				state->downloadTimeToOffsetEntry = &state->timeToOffset->entry[0];
			break;

		case kFskMediaSpoolerOperationDataReady:
			state->spoolerPosition += (UInt32)param;

			if (state->reader->mediaState < kFskMediaPlayerStateStopped) {
				while (0 == state->moov.size) {
					unsigned char *header;
					QTFileOffset size;

					err = FskMediaSpoolerRead(state->spooler, state->moov.offset, 16, &header, NULL);
					BAIL_IF_ERR(err);

					size = FskMisaligned32_GetBtoN(&header[0]);
					if (1 == size) {
						size = FskMisaligned64_GetN(&header[8]);
						size = FskEndianU64_BtoN(size);
					}

					if (size < 8) {
						state->theError = kFskErrBadData;
						(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
						goto bail;
					}

					if (0 == FskStrCompareWithLength("moov", (const char *)&header[4], 4)) {
						if (size == (UInt32)size) {
							state->moov.size = (UInt32)size;

							err = FskMemPtrNew(state->moov.size, (FskMemPtr *)&state->moov.data);
							if (kFskErrNone != err) {
								state->theError = err;
								(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
							}
						}
						else {
							state->theError = kFskErrBadData;
							(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
						}
					}
					else
						state->moov.offset += size;
				}

				if (0 != state->moov.size) {
					// collect the entire movie resource. it can be larger than the streaming buffers.
					UInt32 bytesRead;
					unsigned char *data;

					if (kFskErrNone == FskMediaSpoolerRead(state->spooler, state->moov.offset + state->moov.bytes,
						state->moov.size - state->moov.bytes, &data, &bytesRead)) {
						FskMemMove(&state->moov.data[state->moov.bytes], data, bytesRead);
						state->moov.bytes += bytesRead;
						if (state->moov.bytes == state->moov.size) {
							mp4Instantiate(state);
							FskMemPtrDisposeAt(&state->moov.data);
							state->moov.size = 0;			// we're done trying to instantiate
						}
						else {
							UInt32 percent = (UInt32)((((float)state->moov.bytes) / ((float)state->moov.size)) * 100.0);
							(state->reader->doSetState)(state->reader, kFskMediaPlayerStateInstantiatingProgress + percent);
						}
					}
				}
			}
			break;
	}

bail:
	return err;
}

FskErr buildTimeToOffsetTable(mp4Reader state)
{
	FskErr err = kFskErrNone;
	QTMovie movie = state->movie;
	double duration = (movie->duration + movie->scale - 1) / movie->scale;
	UInt32 secondsPerEntry, entryCount, time, *entry, trackIndex;
	QTTimeToOffsetTable table;
	mp4ReaderTrack walker;

	// we don't want a giant table (too much memory, too much time to construct) so we reduce the resolution to keep the size reasonable
	for (secondsPerEntry = 1; ((duration + secondsPerEntry - 1) / secondsPerEntry) > 1024; secondsPerEntry++)
		;

	entryCount = (UInt32)((duration + secondsPerEntry - 1) / secondsPerEntry);

	err = FskMemPtrNewClear(sizeof(QTTimeToOffsetTableRecord) + (entryCount * sizeof(UInt32)), (FskMemPtr*)(void*)(&table));
	if (err) return err;

	state->timeToOffset = table;
	table->entryCount = entryCount;
	table->secondsPerEntry = secondsPerEntry;

	for (walker = state->tracks, trackIndex = 0; NULL != walker; walker= walker->next) {
		QTFileOffset previousOffset = 0;
		QTTrack track = walker->qtTrack;
		QTMedia media = track->media;
		UInt32 count;

		if (!media || walker->unplayable) continue;

		if ((kQTVideoType != media->mediaType) && (kQTSoundType != media->mediaType))
			continue;

		for (time = 0, entry = table->entry, count = 0; time < media->duration && count < entryCount; time += (secondsPerEntry * media->scale), entry++, count++) {
			UInt32 sampleNumber, chunkNumber, firstChunkSample;
			QTFileOffset sampleOffset, chunkOffset, endOfSampleOffset, sampleSize;

			if (0 != QTTrackTimeToSample(track, time, &sampleNumber, NULL, NULL))
				break;			// QTTrackTimeToSample fails because there are no more samples; some media have a duration greater than the max sample time (mistake when authored), so this is just another valid exit condition

			if (0 != QTTrackSampleToChunk(track, sampleNumber, &chunkNumber, &chunkOffset, &firstChunkSample, NULL))
				goto fail;

			if (firstChunkSample == sampleNumber)
				sampleOffset = chunkOffset;
			else {
				QTFileOffset offset;

				if (0 != QTTrackGetSampleSizes(track, firstChunkSample, sampleNumber - firstChunkSample, &offset))
					goto fail;

				sampleOffset = chunkOffset + (UInt32)offset;
			}

			if (0 != QTTrackGetSampleSizes(track, sampleNumber, 1, &sampleSize))
				goto fail;

			endOfSampleOffset = sampleOffset + sampleSize;
			if (endOfSampleOffset < previousOffset) {
				endOfSampleOffset = previousOffset;
				table->hasMediaSamplesOutOfOrder = true;
			}

			if (endOfSampleOffset > *entry) {
				if (0 != trackIndex) {
					UInt32 gap = (UInt32)(endOfSampleOffset - *entry);
					if (gap > table->maxGap)
						table->maxGap = gap;
				}
				*entry = (UInt32)endOfSampleOffset;
			}
			else {
				if (0 != trackIndex) {
					UInt32 gap = (UInt32)(*entry - endOfSampleOffset);
					if (gap > table->maxGap)
						table->maxGap = gap;
				}
			}

			previousOffset = endOfSampleOffset;
		}

		if (0 == trackIndex) {
			// smear last value to the end
			while (entry < (table->entry + table->entryCount)) {
				entry[0] = entry[-1];
				entry++;
			}
		}

		trackIndex += 1;
	}

	return kFskErrNone;

fail:
	FskMemPtrDisposeAt(&state->timeToOffset);

	return kFskErrOperationFailed;
}

QTErr qtRead(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize)
{
	mp4Reader state = refCon;
	FskErr err = kFskErrNone;

	while (0 != dataSize) {
		unsigned char *buffer;
		UInt32 bytesRead;

		err = FskMediaSpoolerRead(state->spooler, offset, dataSize, &buffer, &bytesRead);
		if (err) return err;

		FskMemMove(data, buffer, bytesRead);

		data = bytesRead + (char *)data;
		dataSize -= bytesRead;
		offset += bytesRead;
	}

	return err;
}

QTErr qtMemRead(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize)
{
	mp4Reader state = refCon;

	if (offset + dataSize > state->moov.size)
		return kFskErrEndOfFile;

	FskMemMove(data, state->moov.data + offset, dataSize);

	return kFskErrNone;
}

QTErr qtAlloc(void *refCon, Boolean clear, UInt32 size, void **data)
{
	if (clear)
		BAIL_IF_ERR(FskMemPtrNewClear(size, (FskMemPtr *)data));
	else
		BAIL_IF_ERR(FskMemPtrNew(size, (FskMemPtr *)data));
		
bail:
	return (*data == NULL) ? -1 : 0;
}

void qtFree(void *refCon, void *data)
{
	FskMemPtrDispose(data);
}
