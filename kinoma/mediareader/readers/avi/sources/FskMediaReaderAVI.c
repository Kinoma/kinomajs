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
	other codecs (e.g. non-divx content)
	seeking on http (not easy!)

	index cache to handle key frame flag better - unnecesary for now, as Bryan can sniff MPEG-4 and AVC bitstreams to determine frame type

	done:
	bitrate, frame rate info
	frame rate
	http support (ignore index, no seek)
	kFskDecompressorSlop
	scrub mode
*/

#define __FSKMEDIAREADER_PRIV__
#include "FskMediaReader.h"
#include "FskDIDLGenMedia.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "QTReader.h"

static Boolean aviReaderCanHandle(const char *mimeType);
static FskErr aviReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr aviReaderDispose(FskMediaReader reader, void *readerState);
static FskErr aviReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr aviReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr aviReaderStop(FskMediaReader reader, void *readerState);
static FskErr aviReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr aviReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr aviReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr aviReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord aviReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			aviReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			aviReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		aviReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		aviReaderGetState,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						aviReaderSetScrub},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	aviReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderAVI = {aviReaderCanHandle, aviReaderNew, aviReaderDispose, aviReaderGetTrack, aviReaderStart, aviReaderStop, aviReaderExtract, aviReaderGetMetadata, aviReaderProperties, aviReaderSniff};

static FskErr aviReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr aviReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord aviReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		aviReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		aviReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		aviReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		aviReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		aviReaderTrackGetBitRate,	NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskMediaPropertyEntryRecord aviReaderVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		aviReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		aviReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			aviReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			aviReaderTrackGetFrameRate,			NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		aviReaderTrackGetDimensions,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		aviReaderTrackGetBitRate,	NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gAVIReaderAudioTrack = {aviReaderAudioTrackProperties};
FskMediaReaderTrackDispatchRecord gAVIReaderVideoTrack = {aviReaderVideoTrackProperties};

Boolean aviReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("video/avi", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/msvideo", mimeType))
		return true;

	return false;
}

enum {
	kAVIMediaTypeUnknown = 0,
	kAVIMediaTypeAudio,
	kAVIMediaTypeVideo,
	kAVIMediaTypeImageJFIF
};

typedef struct aviReaderRecord aviReaderRecord;
typedef struct aviReaderRecord *aviReader;

typedef struct aviReaderTrackRecord aviReaderTrackRecord;
typedef struct aviReaderTrackRecord *aviReaderTrack;

struct aviReaderTrackRecord {
	aviReaderTrack						next;

	FskMediaReaderTrackRecord			readerTrack;
	aviReader							avi;

	UInt32						aviScale;
	UInt32						aviRate;
	UInt32						dwLength;
	UInt32						timeOffset;
	UInt32						aviSampleSize;
	UInt32						aviPosition;

	FskInt64					playDuration;

	char						*format;
	UInt32						mediaType;
	union {
		struct {
			UInt16				formatTag;
			UInt16				numChannels;
			UInt32				samplesPerSecond;
			UInt32				averageBytesPerSecond;
			UInt16				blockAlign;
			UInt16				bitsPerSample;
			UInt16				codecSpecificDataSize;
			UInt8				*codecSpecificData;

			unsigned char		*mp3Data;
			UInt32				mp3DataSize;
			UInt32				mp3BytesAvailable;
		} audio;

		struct {
			UInt32				encodedImageWidth;
			UInt32				encodedImageHeight;

			UInt32				width;
			UInt32				height;
			UInt16				reserved;
			UInt16				bitsPerPixel;
			UInt32				compressionID;
			UInt32				imageSize;
			UInt32				hPixelsPerMeter;
			UInt32				vPixelsPerMeter;
			UInt32				colorsUsed;
			UInt32				importantColors;
			UInt32				codecSpecificDataSize;
			UInt8				*codecSpecificData;
		} video;
	} media;

	union {
		struct {
			UInt32						sampleRate;
			UInt32						channelCount;
		} audio;

		struct {
			FskDimensionRecord			dimensions;
			Boolean						isMP4;
			FskImageDecompress			deco;
		} video;
	};
};

 struct aviReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;
	Boolean					dontSeekIfExpensive;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;
	Boolean					loaded;
	Boolean					scrub;

	UInt32					fileSize;
	FskInt64				dataObject_fileOffset;
	FskInt64				dataObject_fileSize;
	FskInt64				aviOffset;
	FskInt64				simpleIndex_fileOffset;
	FskInt64				simpleIndex_count;
	FskInt64				playDuration;
	UInt32					scale;
	UInt32					duration;

	FskMediaReader			reader;

	aviReaderTrack			tracks;
};

static FskErr aviInstantiate(aviReader avi);
static FskErr instantiateAVI(aviReader avi);
static FskErr aviSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static FskErr doRead(aviReader avi, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr AVIDemuxerNextFrame(aviReader avi, void **data, UInt32 *dataSize, aviReaderTrack *trackOut, FskInt64 *presentationTime);

typedef FskErr (*aviChunkWalker)(aviReader avi, FskInt64 offset, FskInt64 size);

typedef struct {
	UInt32				chunkType;
	UInt32				listType;
	aviChunkWalker		walker;
} aviChunkWalkersRecord, *aviChunkWalkers;

static void disposeAVI(aviReader avi);
static FskErr aviWalkChunks(aviReader avi, FskInt64 offset, FskInt64 size, aviChunkWalkers walkers);

#define DeclareAVIChunkProc(foo) static FskErr foo(aviReader avi, FskInt64 chunkOffset, FskInt64 chunkSize)

DeclareAVIChunkProc(aviHandlersListChunk);
DeclareAVIChunkProc(aviMovieListChunk);
DeclareAVIChunkProc(aviIndexChunk);
DeclareAVIChunkProc(aviHeaderChunk);
DeclareAVIChunkProc(aviStreamListChunk);
DeclareAVIChunkProc(aviStreamHeaderChunk);
DeclareAVIChunkProc(aviStreamFormatChunk);

static UInt32 aviReadFourCC(unsigned char **p);
static UInt32 aviRead32(unsigned char **p);
static UInt16 aviRead16(unsigned char **p);

static FskErr aviPostProcessStreams(aviReader avi);
static SInt32 bytesToFrameStart(unsigned char *p, SInt32 count);

#define kMicroToNanoSec (10)
#define kNanoSec (10000000L)

FskErr AVIDemuxerNextFrame(aviReader avi, void **data, UInt32 *dataSize, aviReaderTrack *trackOut, FskInt64 *presentationTime)
{
	FskErr err;
	unsigned char chunkHeader[8];
	UInt32 streamIndex, chunkSize = 0;
	aviReaderTrack track = avi->tracks;
	unsigned char *p;
	
	err = doRead(avi, avi->aviOffset, 8, chunkHeader);
	BAIL_IF_ERR(err);

	streamIndex = ((chunkHeader[0] - '0') * 10) + (chunkHeader[1] - '0');
	p = &chunkHeader[4];
	chunkSize = aviRead32(&p);

	while (streamIndex-- && track)
		track = track->next;

	*dataSize = chunkSize;
	*trackOut = track;

	if (NULL == track)
		goto bail;

	err = FskMemPtrNew(chunkSize + kFskDecompressorSlop, data);
	BAIL_IF_ERR(err);

	err = doRead(avi, avi->aviOffset + 8, chunkSize, *data);
	if (err) {
		chunkSize = 0;
		goto bail;
	}

	if (kAVIMediaTypeVideo == track->mediaType) {
		if (presentationTime)
			*presentationTime = (FskInt64)((double)kNanoSec * ((track->aviPosition * (float)track->aviScale) / (float)track->aviRate));

		track->aviPosition += 1;
	}
	else if (kAVIMediaTypeAudio == track->mediaType) {
		if (presentationTime)
			*presentationTime = (FskInt64)((double)kNanoSec * ((float)track->aviPosition / (float)track->media.audio.averageBytesPerSecond));

		track->aviPosition += chunkSize;
	}

bail:
	if (chunkSize)
		avi->aviOffset += chunkSize + (chunkSize & 1) + 8;

	return err;
}

FskErr aviReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	aviReader avi = NULL;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(aviReaderRecord), &avi);
	BAIL_IF_ERR(err);

	*readerState = avi;			// must be set before anything that might issue a callback
	avi->spooler = spooler;
	avi->reader = reader;

	avi->spooler->onSpoolerCallback = aviSpoolerCallback;
	avi->spooler->clientRefCon = avi;
	avi->spooler->flags |= kFskMediaSpoolerForwardOnly;

	avi->scale = 1000;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		avi->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = aviInstantiate(avi);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != avi)) {
		aviReaderDispose(reader, avi);
		avi = NULL;
	}

	*readerState = avi;

	return err;
}

FskErr aviReaderDispose(FskMediaReader reader, void *readerState)
{
	aviReader avi = readerState;

	disposeAVI(avi);

	if (avi->spoolerOpen && avi->spooler->doClose)
		(avi->spooler->doClose)(avi->spooler);

	FskMemPtrDispose(avi);

	return kFskErrNone;
}

FskErr aviReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	aviReader avi = readerState;
	aviReaderTrack walker = avi->tracks;

	while ((index > 0) && walker) {
		walker = walker->next;
		index -= 1;
	}

	if (walker) {
		*track = &walker->readerTrack;
		return kFskErrNone;
	}
	else
		return kFskErrNotFound;
}

FskErr aviReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	FskErr err = kFskErrNone;
	aviReader avi = readerState;
	aviReaderTrack walker;

	avi->aviOffset = avi->dataObject_fileOffset;

	for (walker = avi->tracks; NULL != walker; walker = walker->next) {
		walker->aviPosition = 0;

		if (kAVIMediaTypeAudio == walker->mediaType) {
			FskMemPtrDisposeAt(&walker->media.audio.mp3Data);
			walker->media.audio.mp3DataSize = 0;
			walker->media.audio.mp3BytesAvailable = 0;
		}
	}

	if (avi->simpleIndex_count && startTime && *startTime) {
		double duration = avi->playDuration / (kMicroToNanoSec * (double)avi->scale);
		double fraction = *startTime / duration;
		UInt32 target = (UInt32)(fraction * avi->simpleIndex_count), index;
		UInt32 indexEntry[4], offset;
		FskInt64 bias;
		UInt32 bufferCount = 512, count;
		UInt32 buffer[512 * 4], *b;

		// check 0 entry to determine if index is absolute or relative
		err = doRead(avi, avi->simpleIndex_fileOffset, 16, indexEntry);
		BAIL_IF_ERR(err);

		offset = FskEndianU32_LtoN(indexEntry[2]);
		if (offset >= avi->dataObject_fileOffset)
			bias = 0;
		else
			bias = avi->dataObject_fileOffset - 4;

		// back-up a bit to get a running start
		index = target;
		if (index < bufferCount)
			index = 0;
		else
			index -= bufferCount;
		count = bufferCount;
		if ((index + count) > avi->simpleIndex_count)
			count = avi->simpleIndex_count - index;
		err = doRead(avi, avi->simpleIndex_fileOffset + (index * 16), count * 16, buffer);
		BAIL_IF_ERR(err);
		for (b = buffer; count--; index++, b += 4) {
			unsigned char *chunkHeader = (unsigned char *)b;
			UInt32 streamIndex = ((chunkHeader[0] - '0') * 10) + (chunkHeader[1] - '0');
			UInt32 flags = FskEndianU32_LtoN(b[1]);
			aviReaderTrack walker;

			for (walker = avi->tracks; streamIndex && walker; walker = walker->next, streamIndex -= 1)
				walker = walker->next;

			if (walker && (kAVIMediaTypeVideo == walker->mediaType)) {
				if (flags & 0x010)		// AVIIF_INDEX
					target = index;
			}
		}
		fraction = (double)target / (double)avi->simpleIndex_count;

		// (re)read this entry
		err = doRead(avi, avi->simpleIndex_fileOffset + (target * 16), 16, indexEntry);
		BAIL_IF_ERR(err);

		offset = FskEndianU32_LtoN(indexEntry[2]);
		avi->aviOffset = offset + bias;

		for (walker = avi->tracks; NULL != walker; walker = walker->next) {
			if (kAVIMediaTypeVideo == walker->mediaType)
				walker->aviPosition = (UInt32)(walker->dwLength * fraction);
			else if (kAVIMediaTypeAudio == walker->mediaType)
				walker->aviPosition = (UInt32)(walker->media.audio.averageBytesPerSecond * (duration / (double)avi->scale) * fraction);
		}
	}

bail:
	return err;
}

FskErr aviReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;		//@@
}

FskErr aviReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **data)
{
	FskErr err;
	aviReader avi = readerState;
	UInt32 dataSize;
	aviReaderTrack track;
	FskInt64 presoTime;
	FskMediaReaderSampleInfo info;

	while (true) {
		err = AVIDemuxerNextFrame(avi, (void **)data, &dataSize, &track, &presoTime);
		BAIL_IF_ERR(err);

		if (NULL == track) continue;

		if (kAVIMediaTypeVideo == track->mediaType) {
			SInt32 offset;

			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) {
				FskMemPtrDisposeAt((void **)data);
				goto bail;
			}

			if (track->video.isMP4) {
				offset = bytesToFrameStart(*data, dataSize);
				if (0 != offset) {
					FskMemMove(*data, *data + offset, dataSize - offset);
					dataSize -= offset;
				}
			}

			info->flags = kFskImageFrameTypeSync;
			if (track->video.deco) {
				FskMediaPropertyValueRecord value;
				FskImageDecompressSetData(track->video.deco, *data, dataSize);
				if (kFskErrNone == FskImageDecompressGetMetaData(track->video.deco, kFskImageDecompressMetaDataFrameType, 0, &value, NULL))
					info->flags = value.value.integer;
			}

			if (avi->scrub && (kFskImageFrameTypeSync != info->flags)) {
				FskMemPtrDisposeAt((void **)data);
				FskMemPtrDisposeAt((void **)&info);
				continue;
			}

			info->samples = 1;
			info->sampleSize = dataSize;
			info->decodeTime = presoTime / (kMicroToNanoSec * (double)avi->scale);
			info->compositionTime = -1;

			*infoCountOut = 1;
			*infoOut = info;
			*trackOut = &track->readerTrack;
			break;
		}
		else
		if (kAVIMediaTypeAudio == track->mediaType) {
			if (85 == track->media.audio.formatTag) {
				UInt32 space;
				unsigned char *d, *e, *dUsed;
				UInt32 dataSizeOut;
				const int kEndSlop = 16;

				if (NULL == track->media.audio.mp3Data) {
					track->media.audio.mp3DataSize = 32768;
					err = FskMemPtrNew(track->media.audio.mp3DataSize, (FskMemPtr *)&track->media.audio.mp3Data);
					BAIL_IF_ERR(err);

					track->media.audio.mp3BytesAvailable = 0;
				}

				space = track->media.audio.mp3DataSize - track->media.audio.mp3BytesAvailable;
				if (space < dataSize) {
					UInt32 need = dataSize - space;
					FskMemMove(track->media.audio.mp3Data, track->media.audio.mp3Data + need, need);
					track->media.audio.mp3BytesAvailable = need;
				}
				FskMemMove(track->media.audio.mp3Data + track->media.audio.mp3BytesAvailable, *data, dataSize);
				track->media.audio.mp3BytesAvailable += dataSize;

				FskMemPtrDisposeAt(data);

				*data = NULL;
				dataSizeOut = 0;
				*infoOut = NULL;
				*infoCountOut = 0;

				d = track->media.audio.mp3Data;
				e = track->media.audio.mp3Data + track->media.audio.mp3BytesAvailable - kEndSlop;
				dUsed = NULL;
				while (d < e) {
					DIDLMusicItemRecord mi1;
					FskMediaReaderSampleInfo i;

					if (false == parseMP3Header(d, &mi1)) {
						d += 1;
						continue;
					}

					if ((SInt32)mi1.frameLength > (SInt32)((e + kEndSlop) - d))
						break;

					err = FskMemPtrRealloc(dataSizeOut + mi1.frameLength, (FskMemPtr *)data);
					BAIL_IF_ERR(err);

					err = FskMemPtrRealloc((*infoCountOut + 1) * sizeof(FskMediaReaderSampleInfoRecord), (FskMemPtr *)infoOut);
					BAIL_IF_ERR(err);

					FskMemMove(*data + dataSizeOut, d, mi1.frameLength);
					dataSizeOut += mi1.frameLength;

					i = (*infoOut) + *infoCountOut;
					FskMemSet(i, 0, sizeof(FskMediaReaderSampleInfoRecord));
					i->samples = 1;
					i->sampleSize = mi1.frameLength;
					i->flags = kFskImageFrameTypeSync;
					i->decodeTime = presoTime / (kMicroToNanoSec * (double)avi->scale);
					i->compositionTime = -1;

					*infoCountOut += 1;
					d += mi1.frameLength;

					dUsed = d;
				}

				if (dUsed) {
					FskMemMove(track->media.audio.mp3Data, dUsed, (e + kEndSlop) - dUsed);
					track->media.audio.mp3BytesAvailable = (e + kEndSlop) - dUsed;
				}

				if (0 == *infoCountOut)
					continue;

				*trackOut = &track->readerTrack;

				goto bail;
			}

			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) {
				FskMemPtrDisposeAt((void **)data);
				goto bail;
			}

			info->samples = 1;
			info->sampleSize = dataSize;
			info->flags = kFskImageFrameTypeSync;
			info->decodeTime = presoTime / (kMicroToNanoSec * (double)avi->scale);
			info->compositionTime = -1;

			*infoCountOut = 1;
			*infoOut = info;
			*trackOut = &track->readerTrack;
			break;
		}
	}

bail:
	return err;
}

FskErr aviReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	return kFskErrUnknownElement;
}

FskErr aviReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if (dataSize >= 12) {
		if (('R' == data[0]) && ('I' == data[1]) && ('F' == data[2]) && ('F' == data[3]) &&
			('A' == data[8]) && ('V' == data[9]) && ('I' == data[10]) && (' ' == data[11])) {
			*mime = FskStrDoCopy("video/avi");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr aviReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReader avi = state;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = avi->playDuration / (kMicroToNanoSec * (double)avi->scale);

	return kFskErrNone;
}

FskErr aviReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return -1;
}

FskErr aviReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReader avi = state;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = avi->scale;

	return kFskErrNone;
}

FskErr aviReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	return -1;
}

FskErr aviReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReader avi = state;

	avi->scrub = property->value.b;

	return kFskErrNone;
}

FskErr aviReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/avi\000video/msvideo\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

FskErr aviReaderTrackGetMediaType(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeString;
	if (kAVIMediaTypeAudio == track->mediaType)
		property->value.str = FskStrDoCopy("audio");
	else
	if (kAVIMediaTypeVideo == track->mediaType)
		property->value.str = FskStrDoCopy("video");
	else
		property->value.str = FskStrDoCopy("unknown");

	return kFskErrNone;
}

FskErr aviReaderTrackGetFormat(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);

	return kFskErrNone;
}

FskErr aviReaderTrackGetFormatInfo(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	if (kAVIMediaTypeVideo != track->mediaType)
		return kFskErrUnimplemented;

	if (0 == track->media.video.codecSpecificDataSize)
		return kFskErrUnimplemented;

	if (0 == FskStrCompare(track->format, "x-video-codec/mp4")) {
		static unsigned char prefix[] = {0x00, 0x00, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x01, 0xb5, 0x09};
		FskErr err;
		UInt32 s = FskMisaligned32_GetBtoN(&track->media.video.codecSpecificData);
		unsigned char *desc, *esds;
		UInt32 descSize, esdsSize;
		Boolean needPatch = false;
		UInt32 constesds = 'esds', constmp4v = 'mp4v';

		if ((s >> 10) == 0x20)
			;		// do nothing, short headers
		else if (s == 0x000001b0)
			;		// do nothing, data is fine
		else
			needPatch = true;		// special case for MPEG-4 in ASF as per the ASF specification section 11.2 - may need to add a prefix to the esds

		esdsSize = 8 + (needPatch ? 10 : 0) + track->media.video.codecSpecificDataSize;
		descSize = sizeof(QTImageDescriptionRecord) + esdsSize;

		err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
		if (err) return err;

		FskMemMove(desc, &descSize, 4);			// *(UInt32 *)desc = (descSize);
		FskMemMove(desc + 4, &constmp4v, 4);		// *(UInt32 *)(desc + 4) = ('mp4v');
		esds = desc + sizeof(QTImageDescriptionRecord);
		FskMemMove(esds, &esdsSize, 4);			// *(UInt32 *)esds = (esdsSize);
		FskMemMove(esds + 4, &constesds, 4);		// *(UInt32 *)(esds + 4) = ('esds');
		if (!needPatch)
			FskMemMove(esds + 8, track->media.video.codecSpecificData, track->media.video.codecSpecificDataSize);
		else {
			FskMemMove(esds + 8, prefix, 10);
			FskMemMove(esds + 8 + 10, track->media.video.codecSpecificData, track->media.video.codecSpecificDataSize);
		}

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = descSize;
		property->value.data.data = desc;

		return kFskErrNone;
	}

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = track->media.video.codecSpecificDataSize;
	return FskMemPtrNewFromData(track->media.video.codecSpecificDataSize, track->media.video.codecSpecificData, (FskMemPtr *)&property->value.data.data);
}

FskErr aviReaderTrackGetFrameRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = track->aviRate;
	property->value.ratio.denom = track->aviScale;

	return kFskErrNone;
}

FskErr aviReaderTrackGetSampleRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->media.audio.samplesPerSecond;

	return kFskErrNone;
}

FskErr aviReaderTrackGetChannelCount(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->media.audio.numChannels;

	return kFskErrNone;
}

FskErr aviReaderTrackGetDimensions(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width = track->media.video.width;
	property->value.dimension.height = track->media.video.height;

	return kFskErrNone;
}

FskErr aviReaderTrackGetBitRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;
	aviReader avi = track->avi;
	aviReaderTrack walker;

	property->type = kFskMediaPropertyTypeInteger;
	if (kAVIMediaTypeVideo == track->mediaType) {
		UInt32 audioRate = 0;
		double duration = (double)avi->playDuration / (double)kNanoSec;

		for (walker = avi->tracks; NULL != walker; walker = walker->next) {
			if (kAVIMediaTypeAudio == walker->mediaType)
				audioRate += walker->media.audio.averageBytesPerSecond;
		}
		property->value.integer = (SInt32)((avi->dataObject_fileSize / duration) - audioRate) * 8;
	}
	else
	if (kAVIMediaTypeAudio == track->mediaType)
		property->value.integer = track->media.audio.averageBytesPerSecond * 8;

	return kFskErrNone;
}

FskErr asfReaderTrackGetFrameRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	aviReaderTrack track = state;

	if (kAVIMediaTypeVideo != track->mediaType)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = track->aviScale;
	property->value.ratio.denom = track->aviRate;

	return kFskErrNone;
}

FskErr aviInstantiate(aviReader state)
{
	FskErr err;

	err = instantiateAVI(state);
	BAIL_IF_ERR(err);

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}

FskErr doRead(aviReader state, FskInt64 offset, UInt32 size, void *bufferIn)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;

	while (0 != size) {
		unsigned char *readBuffer;
		UInt32 bytesRead;

		if (state->dontSeekIfExpensive)
			state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;

		err = FskMediaSpoolerRead(state->spooler, offset, size, &readBuffer, &bytesRead);
		if (state->dontSeekIfExpensive)
			state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		if (err) return err;

		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

	return err;
}


FskErr aviSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	aviReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationGetHeaders:
			state->spooler->flags |= kFskMediaSpoolerCantSeek;
			break;

		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = aviInstantiate(state);
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

FskErr instantiateAVI(aviReader avi)
{
	FskErr err = kFskErrNone;
	unsigned char header[12];
	unsigned char *p;
	aviChunkWalkersRecord walkers[] = {
		{'LIST', 'hdrl', NULL},
		{'LIST', 'movi', NULL},
		{'idx1', 0, NULL},
		{0, 0, NULL}
		};

	walkers[0].walker = aviHandlersListChunk;
	walkers[1].walker = aviMovieListChunk;
	walkers[2].walker = aviIndexChunk;

	// check file signature (riff avi header must be first)
	err = doRead(avi, 0, sizeof(header), header);
	BAIL_IF_ERR(err);

	if (('R' != header[0]) || ('I' != header[1]) || ('F' != header[2]) || ('F' != header[3]) ||
		('A' != header[8]) || ('V' != header[9]) || ('I' != header[10]) || (' ' != header[11]))
        BAIL(kFskErrBadData);

	p = &header[4];
	avi->fileSize = aviRead32(&p);

	avi->dontSeekIfExpensive = false;
	err = aviWalkChunks(avi, sizeof(header), avi->fileSize - sizeof(header), walkers);
	if (avi->aviOffset && (kFskErrNeedMoreTime == err))
		err = 0;
	avi->dontSeekIfExpensive = false;
	BAIL_IF_ERR(err);

	if (0 == avi->dataObject_fileOffset)
        BAIL(-1);       // movi chunk is required

	err = aviPostProcessStreams(avi);
	BAIL_IF_ERR(err);

	avi->loaded = true;

bail:
	if (err)
		disposeAVI(avi);

	return err;
}

/*
	AVI utils
*/

void disposeAVI(aviReader avi)
{
	while (avi->tracks) {
		aviReaderTrack track = avi->tracks;
		avi->tracks = track->next;
		if (kAVIMediaTypeVideo == track->mediaType) {
			FskMemPtrDispose(track->media.video.codecSpecificData);
			FskImageDecompressDispose(track->video.deco);
		}
		else
		if (kAVIMediaTypeAudio == track->mediaType)
			FskMemPtrDispose(track->media.audio.mp3Data);
		FskMemPtrDispose(track->format);
		FskMemPtrDispose(track);
	}
}

FskErr aviWalkChunks(aviReader avi, FskInt64 offset, FskInt64 size, aviChunkWalkers walkers)
{
	FskErr err = kFskErrNone;

	// scan those objecs
	while ((size >= 8) && (false == avi->loaded)) {
		aviChunkWalkers w = walkers;
		unsigned char chunkHeader[12];
		unsigned char *p;
		UInt32 chunkType, listType, chunkSize, headerSize;

		err = doRead(avi, offset, 12, chunkHeader);
		BAIL_IF_ERR(err);

		p = chunkHeader;
		chunkType = aviReadFourCC(&p);
		chunkSize = aviRead32(&p);
		if ('LIST' == chunkType) {
			listType = aviReadFourCC(&p);
			headerSize = 12;
			chunkSize -= 4;
		}
		else {
			listType = 0;
			headerSize = 8;
		}

		for (w = walkers; NULL != w->walker; w++) {
			if ((chunkType != w->chunkType) || (listType != w->listType))
				continue;

			err = (w->walker)(avi, offset + headerSize, chunkSize);
			BAIL_IF_ERR(err);

			break;
		}

		chunkSize += (chunkSize & 1);
		offset += chunkSize + headerSize;
		size -= chunkSize + headerSize;
	}

bail:
	return err;
}

DeclareAVIChunkProc(aviHandlersListChunk)
{
	aviChunkWalkersRecord walkers[] = {
		{'avih', 0, NULL},
		{'LIST', 'strl', NULL},
		{0, 0, NULL}
		};

	walkers[0].walker = aviHeaderChunk;
	walkers[1].walker = aviStreamListChunk;

	return aviWalkChunks(avi, chunkOffset, chunkSize, walkers);
}

DeclareAVIChunkProc(aviMovieListChunk)
{
	avi->dataObject_fileOffset = chunkOffset;
	avi->dataObject_fileSize = chunkSize;
	avi->aviOffset = chunkOffset;

	avi->dontSeekIfExpensive = true;

	return 0;
}

DeclareAVIChunkProc(aviIndexChunk)
{
	avi->simpleIndex_fileOffset = chunkOffset;
	avi->simpleIndex_count = (UInt32)(chunkSize / 16);

	return 0;
}

DeclareAVIChunkProc(aviHeaderChunk)
{
	FskErr err;
	unsigned char header[0x038];
	unsigned char *p;
	FskInt64 totalFrames;
	FskInt64 microsecondsPerFrame;

	if (chunkSize < sizeof(header))
		return -1;

	err = doRead(avi, chunkOffset, sizeof(header), header);
	BAIL_IF_ERR(err);

	// AVIMAINHEADER structure
	p = header;
	microsecondsPerFrame = aviRead32(&p);	// dwMicroSecPerFrame
	p += 12;
	totalFrames = aviRead32(&p);		// dwTotalFrames
	avi->playDuration = totalFrames * microsecondsPerFrame * kMicroToNanoSec;

bail:
	return err;
}

DeclareAVIChunkProc(aviStreamListChunk)
{
	aviChunkWalkersRecord walkers[] = {
		{'strh', 0, NULL},
		{'strf', 0, NULL},
		{0, 0, NULL}
		};

	walkers[0].walker = aviStreamHeaderChunk;
	walkers[1].walker = aviStreamFormatChunk;

	return aviWalkChunks(avi, chunkOffset, chunkSize, walkers);
}

DeclareAVIChunkProc(aviStreamHeaderChunk)
{
	FskErr err;
	unsigned char header[0x038];
	unsigned char *p;
	aviReaderTrack track;
	UInt32 streamType;

	if (chunkSize < sizeof(header))
		return -1;

	err = doRead(avi, chunkOffset, sizeof(header), header);
	BAIL_IF_ERR(err);

	err = FskMemPtrNewClear(sizeof(aviReaderTrackRecord), &track);
	BAIL_IF_ERR(err);

	track->avi = avi;

	FskListAppend(&avi->tracks, track);

	// AVISTREAMHEADER structure
	p = header;
	streamType = aviReadFourCC(&p);		// fccType
	if ('vids' == streamType)
		track->mediaType = kAVIMediaTypeVideo;
	else if ('auds' == streamType)
		track->mediaType = kAVIMediaTypeAudio;
	else
		goto bail;						// unrecognized

	p += 16;							// fccHandler, dwFlags, wPriority, wLanguage, dwInitialFrames
	track->aviScale = aviRead32(&p);	// dwScale
	track->aviRate = aviRead32(&p);	// dwRate

	track->timeOffset = (aviRead32(&p) * track->aviRate) / track->aviScale;	// dwStart

	track->dwLength = aviRead32(&p);			// dwLength;
	if (track->aviRate && track->dwLength && track->aviScale)
		track->playDuration = (FskInt64)((((double)track->aviScale / (double)track->aviRate) * (double)track->dwLength) * kNanoSec);

	p += 8;								// dwSuggestedBufferSize, dwQuality
	track->aviSampleSize = aviRead32(&p);	// dwSampleSize
	p += 4;								// rcFrame left/top
	if (kAVIMediaTypeVideo == track->mediaType) {
		track->media.video.encodedImageWidth = aviRead16(&p);
		track->media.video.encodedImageHeight = aviRead16(&p);
	}

bail:
	return err;
}

DeclareAVIChunkProc(aviStreamFormatChunk)
{
	FskErr err;
	aviReaderTrack track = avi->tracks;
	unsigned char *data, *p;

	while (track->next)
		track = track->next;

	err = FskMemPtrNew((UInt32)chunkSize, &data);
	BAIL_IF_ERR(err);

	err = doRead(avi, chunkOffset, (UInt32)chunkSize, data);
	BAIL_IF_ERR(err);

	p = data;

	if (kAVIMediaTypeAudio == track->mediaType) {
		track->media.audio.formatTag = aviRead16(&p);
		track->media.audio.numChannels = aviRead16(&p);
		track->media.audio.samplesPerSecond = aviRead32(&p);
		track->media.audio.averageBytesPerSecond = aviRead32(&p);
		track->media.audio.blockAlign = aviRead16(&p);
		track->media.audio.bitsPerSample = aviRead16(&p);

		track->readerTrack.dispatch = &gAVIReaderAudioTrack;
		track->readerTrack.state = track;
	}
	else
	if (kAVIMediaTypeVideo == track->mediaType) {
		p += 4;
		track->media.video.width = aviRead32(&p);
		track->media.video.height = aviRead32(&p);
		track->media.video.reserved = aviRead16(&p);
		track->media.video.bitsPerPixel = aviRead16(&p);
		track->media.video.compressionID = aviRead32(&p);
		track->media.video.imageSize = aviRead32(&p);
		track->media.video.hPixelsPerMeter = aviRead32(&p);
		track->media.video.vPixelsPerMeter = aviRead32(&p);
		track->media.video.colorsUsed = aviRead32(&p);
		track->media.video.importantColors = aviRead32(&p);

		track->readerTrack.dispatch = &gAVIReaderVideoTrack;
		track->readerTrack.state = track;
	}

bail:
	FskMemPtrDispose(data);

	return err;
}

UInt32 aviReadFourCC(unsigned char **pp)
{
	unsigned char *p = *pp;
	UInt32 result;

	result = p[3];
	result |= ((UInt32)p[2]) <<   8;
	result |= ((UInt32)p[1]) <<  16;
	result |= ((UInt32)p[0]) <<  24;

	*pp += 4;
	return result;
}

UInt32 aviRead32(unsigned char **pp)
{
	unsigned char *p = *pp;
	UInt32 result;

	result = p[0];
	result |= ((UInt32)p[1]) <<   8;
	result |= ((UInt32)p[2]) <<  16;
	result |= ((UInt32)p[3]) <<  24;

	*pp += 4;
	return result;
}

UInt16 aviRead16(unsigned char **pp)
{
	unsigned char *p = *pp;
	UInt16 result = (p[0] <<  0) | (p[1] <<  8);
	*pp += 2;
	return	result;
}

FskErr aviPostProcessStreams(aviReader avi)
{
	FskErr err = kFskErrNone;
	aviReaderTrack track;

	for (track = avi->tracks; NULL != track; track = track->next) {
		if (track->playDuration > avi->playDuration)
			avi->playDuration = track->playDuration;

		if (kAVIMediaTypeVideo == track->mediaType) {
			if (('XVID' == track->media.video.compressionID) ||
				('05XD' == track->media.video.compressionID) ||
				('DIVX' == track->media.video.compressionID)) {
				unsigned char *data;
				UInt32 dataSize;
				aviReaderTrack aTrack;

				track->media.video.compressionID = '2S4M';
				track->format = FskStrDoCopy("x-video-codec/mp4");
				track->video.isMP4 = true;
				while (true) {
					err = AVIDemuxerNextFrame(avi, (void **)&data, &dataSize, &aTrack, NULL);
					BAIL_IF_ERR(err);

					if (aTrack == track) {
						SInt32 esdsSize = bytesToFrameStart(data, dataSize);
						if (esdsSize > 0) {
							err = FskMemPtrNew(esdsSize, &track->media.video.codecSpecificData);
							BAIL_IF_ERR(err);

							track->media.video.codecSpecificDataSize = esdsSize;
							FskMemMove(track->media.video.codecSpecificData, data, esdsSize);
						}
						FskMemPtrDispose(data);
						break;
					}
					FskMemPtrDispose(data);
				}
			}
			else
			if ('462H' == track->media.video.compressionID) {
				track->format = FskStrDoCopy("x-video-codec/avc");
			}

			if (track->format)
				FskImageDecompressNew(&track->video.deco, 0, track->format, NULL);		// instance for determining frame type
		}
		else
		if (kAVIMediaTypeAudio == track->mediaType) {
			if (85 == track->media.audio.formatTag)
				track->format = FskStrDoCopy("x-audio-codec/mp3");
		}
	}

bail:
	return err;
}

SInt32 bytesToFrameStart(unsigned char *p, SInt32 count)
{
	SInt32 offset = 0;
	UInt32 start = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	count -= 4;
	p += 4;

	if (count > 1024)
		count = 1024;

	while (count >= 4) {
		if (0x01b6 == start)
			return offset;

		offset++;
		start = (start << 8) | *p++;
		count--;
	}

	return 0;
}
