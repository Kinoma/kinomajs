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
#include "FskDIDLGenMedia.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskHTTPClient.h"
#include "FskMediaReader.h"
#include "FskTextConvert.h"
#include "ASFReader.h"
#include "QTReader.h"

#define kMilliToNano ((ASFFileOffset)10000)
#define kNSPlayerUserAgent "NSPlayer/9.0.0.3646"

static Boolean asfReaderCanHandle(const char *mimeType);
static FskErr asfReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr asfReaderDispose(FskMediaReader reader, void *readerState);
static FskErr asfReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr asfReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr asfReaderStop(FskMediaReader reader, void *readerState);
static FskErr asfReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr asfReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr asfReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static Boolean asfReaderMMSCanHandle(const char *mimeType);
static FskErr asfReaderMMSNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr asfReaderMMSStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr asfReaderMMSStop(FskMediaReader reader, void *readerState);
static FskErr asfReaderMMSSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr asfReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetRedirect(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetError(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetSeekableSegment(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderSetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetBufferDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord asfReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			asfReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			asfReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		asfReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		asfReaderGetState,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						asfReaderSetScrub},
	{kFskMediaPropertyRedirect,				kFskMediaPropertyTypeString,		asfReaderGetRedirect,		NULL},
	{kFskMediaPropertyError,				kFskMediaPropertyTypeInteger,		asfReaderGetError,			NULL},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			asfReaderGetSeekableSegment, NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		NULL,						asfReaderSetBitRate},
	{kFskMediaPropertyBufferDuration,		kFskMediaPropertyTypeFloat,			asfReaderGetBufferDuration,	NULL},

	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	asfReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

static FskErr asfReaderMMSGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord asfReaderMMSProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			asfReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			asfReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		asfReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		asfReaderGetState,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						asfReaderSetScrub},
	{kFskMediaPropertyRedirect,				kFskMediaPropertyTypeString,		asfReaderGetRedirect,		NULL},
	{kFskMediaPropertyError,				kFskMediaPropertyTypeInteger,		asfReaderGetError,			NULL},
	{kFskMediaPropertySeekableSegment,		kFskMediaPropertyTypeFloat,			asfReaderGetSeekableSegment, NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		NULL,						asfReaderSetBitRate},
	{kFskMediaPropertyBufferDuration,		kFskMediaPropertyTypeFloat,			asfReaderGetBufferDuration,	NULL},

	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	asfReaderMMSGetDLNASinks,	NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderASFFile = {asfReaderCanHandle, asfReaderNew, asfReaderDispose, asfReaderGetTrack, asfReaderStart, asfReaderStop, asfReaderExtract, asfReaderGetMetadata, asfReaderProperties, asfReaderSniff};
FskMediaReaderDispatchRecord gMediaReaderASFMMSOverHTTP = {asfReaderMMSCanHandle, asfReaderMMSNew, asfReaderDispose, asfReaderGetTrack, asfReaderMMSStart, asfReaderMMSStop, asfReaderExtract, asfReaderGetMetadata, asfReaderMMSProperties, asfReaderMMSSniff};

static FskErr asfReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr asfReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord asfReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		asfReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		asfReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		asfReaderTrackGetSampleRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		asfReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		asfReaderTrackGetChannelCount,		NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			asfReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

static FskMediaPropertyEntryRecord asfReaderVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		asfReaderTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		asfReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		asfReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			asfReaderTrackGetFrameRate,			NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		asfReaderTrackGetDimensions,		NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			asfReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaReaderTrackDispatchRecord gASFReaderAudioTrack = {asfReaderAudioTrackProperties};
FskMediaReaderTrackDispatchRecord gASFReaderVideoTrack = {asfReaderVideoTrackProperties};

static const UInt32 kASFLiveDuration = kFskUInt32Max;

Boolean asfReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("video/x-ms-asf", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("video/x-ms-wmv", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("audio/x-ms-wma", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("application/vnd.ms-asf", mimeType))
		return true;

	return false;
}

Boolean asfReaderMMSCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("application/vnd.ms.wms-hdr.asfv1", mimeType))
		return true;

	return false;
}

typedef struct ASFPacketRecord ASFPacketRecord;
typedef ASFPacketRecord *ASFPacket;

struct ASFPacketRecord {
	ASFPacket						next;
	UInt32							number;
	UInt32							dataBytes;			// total bytes expected
	UInt32							bytesInPacket;		// bytes received so far
	char							data[1];
};

typedef struct asfReaderRecord asfReaderRecord;
typedef struct asfReaderRecord *asfReader;

typedef struct asfReaderTrackRecord asfReaderTrackRecord;
typedef struct asfReaderTrackRecord *asfReaderTrack;

struct asfReaderTrackRecord {
	asfReaderTrackRecord				*next;

	FskMediaReaderTrackRecord			readerTrack;
	asfReader							state;
	ASFStream							stream;

	char								*format;

	union {
		struct {
			UInt32						sampleRate;
			UInt32						channelCount;

			unsigned char				*mp3Data;
			UInt32						mp3DataSize;
			UInt32						mp3BytesAvailable;
		} audio;

		struct {
			FskDimensionRecord			dimensions;
		} video;
	};
};

 struct asfReaderRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;
	UInt32					endTime;
	Boolean					hasEndTime;

	UInt32					scale;
	UInt32					duration;

	FskMediaReader			reader;
	ASFDemuxer				demuxer;
	SInt32					availableBitRate;

	asfReaderTrack			tracks;

	UInt32					asfHeaderSize;

	Boolean					isMMS;
	Boolean					scrub;

	FskErr					theError;

	FskInt64				spoolerPosition;

	char					*redirect;

	struct {
		FskHTTPClient						client;
		FskHTTPClientRequest				request;
		FskHTTPClientRequest				stopRequest;

		char								*uri;

		unsigned char						*asfHeader;
		UInt32								asfHeaderSize;
		UInt32								asfHeaderSizeAllocated;

		unsigned char						*asfMeta;
		UInt32								asfMetaSize;
		char								*metaTitle;

		UInt32								packetParseState;
		UInt32								packetParseStateNext;
		UInt32								bytesToCollect;
		UInt32								bytesToSkip;
		UInt32								partial;
		unsigned char						partialBuffer[16];
		ASFPacket							packets;
		ASFPacket							workingPacket;
		UInt32								packetCount;
		UInt32								bytesInWorkingPacket;
		Boolean								endOfPackets;
		Boolean								suspended;
		Boolean								hadUnderflow;
		Boolean								initializeZeroTime;
		ASFFileOffset						timeOffset;
		ASFFileOffset						metaStartOffset;

		char								*playlistSeekID;
		char								*clientID;
		char								clientGUID[64];

		Boolean								starting;
		Boolean								stopping;
		Boolean								ignorePackets;

		Boolean								cantReuseConnection;
		Boolean								doThinning;
		Boolean								isOrb;
	} mms;
};

static FskErr doInstantiateMMS(asfReader state);
static FskErr doStartMMS(asfReader state);
static FskErr instantiateASF(asfReader state);
static FskErr demuxerInstantiated(asfReader state);
static FskErr doRead(asfReader state, FskInt64 offset, UInt32 size, void *bufferIn);
static FskErr asfInstantiate(asfReader state);
static FskErr asfSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static void flushPackets(asfReader state);
static void resolveAdvancedMutualExclusions(asfReader state, UInt32 availableBitRate);

// for mms over http

enum {
	kPacketParseStateInitialize = 1,
	kPacketParseStateHeaderBegin,
	kPacketParseStateHeaderEnd,
	kPacketParseStateDataPacketBegin,
	kPacketParseStateDataPacketFill,
	kPacketParseStateASFHeaderFill,
	kPacketParseStateASFMetaFill,
	kPacketParseStateSkip,
	kPacketParseStateData,
	kPacketParseStateCollect,
	kPacketParseStateEndEnd
};

static FskErr asfHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon);
static FskErr asfDataReceived(FskHTTPClientRequest request, char *buffer, int bufferSize, void *refCon);
static FskErr asfFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);
static FskErr asfFinishedStop(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);
static void ensureClientGUID(asfReader state);

static const unsigned char gAsfGUID[] = {0x30, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c};

FskErr asfReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	asfReader state = NULL;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);

	err = FskMemPtrNewClear(sizeof(asfReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;

	state->spooler->onSpoolerCallback = asfSpoolerCallback;
	state->spooler->clientRefCon = state;
	state->spooler->flags |= kFskMediaSpoolerForwardOnly;

	state->scale = 1000;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	state->mms.isOrb = (NULL != FskStrStr(uri, "/live.wma")) || (NULL != FskStrStr(uri, "/live.wmv"));

	err = asfInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		asfReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr asfReaderMMSNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	asfReader state = NULL;

	err = FskMemPtrNewClear(sizeof(asfReaderRecord), &state);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->reader = reader;

	state->scale = 1000;

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	state->mms.uri = FskStrDoCopy(uri);
	state->isMMS = true;

	state->mms.isOrb = (NULL != FskStrStr(uri, "/live.wma")) || (NULL != FskStrStr(uri, "/live.wmv"));

	//@@ if uri is "mms://" we should make it into "http://" - though conveniently the http client doesn't seem to care

	// let's get connected
	err = doInstantiateMMS(state);
	BAIL_IF_ERR(err);

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		asfReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr doInstantiateMMS(asfReader state)
{
	FskErr err;

	err = FskHTTPClientNew(&state->mms.client, "asfReaderMMSOverHTTP");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(state->mms.client, state);

	err = FskHTTPClientRequestNew(&state->mms.request, (char *)state->mms.uri);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(state->mms.request, state);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(state->mms.request, asfHeadersReceived, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(state->mms.request, asfDataReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(state->mms.request, asfFinished);

	FskHTTPClientRequestAddHeader(state->mms.request, "User-Agent", kNSPlayerUserAgent);
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "version11-enabled=1");		// seems necessary to allow multiple requests on a single tcp connection

	ensureClientGUID(state);
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", state->mms.clientGUID);
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "xPlayStrm=0");
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "max-duration=0");

	err = FskHTTPClientBeginRequest(state->mms.client, state->mms.request);
	BAIL_IF_ERR(err);

bail:
	return err;
}

FskErr asfReaderDispose(FskMediaReader reader, void *readerState)
{
	asfReader state = readerState;

	if (NULL != state) {
		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		while (state->tracks) {
			asfReaderTrack track = FskListRemoveFirst((void **)&state->tracks);
			if (kASFMediaTypeAudio == track->stream->mediaType)
				FskMemPtrDispose(track->audio.mp3Data);
			FskMemPtrDispose(track);
		}

		ASFDemuxerDispose(state->demuxer);
		FskMemPtrDispose(state->redirect);

		FskHTTPClientDispose(state->mms.client);
		FskMemPtrDispose(state->mms.uri);
		FskMemPtrDispose(state->mms.playlistSeekID);
		FskMemPtrDispose(state->mms.clientID);
		flushPackets(state);

		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr asfReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *trackOut)
{
	asfReader state = readerState;
	asfReaderTrack track = state->tracks;

	while ((NULL != track) && ((0 != index) || (true == track->stream->disabled))) {
		if (false == track->stream->disabled)
			index -= 1;
		track = track->next;
	}

	if ((NULL == track) || (track->stream->disabled))
		return kFskErrNotFound;

	*trackOut = &track->readerTrack;

	return kFskErrNone;
}

FskErr asfReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	asfReader state = readerState;
	FskErr err;
	ASFStream stream;

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

	for (stream = state->demuxer->streams; NULL != stream; stream = stream->next) {
		if (stream->encrypted) {
			if (kFskErrNone == state->theError) {
				state->theError = kFskErrCannotDecrypt;
				(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
			}
            BAIL(kFskErrCannotDecrypt);
		}
	}

	state->mms.endOfPackets = false;

	err = ASFDemuxerSeek(state->demuxer, state->atTime * kMilliToNano, NULL);
	if (kFskErrNeedMoreTime == err) err = kFskErrNone;
	BAIL_IF_ERR(err);

	state->mms.initializeZeroTime = 0 == state->demuxer->playDuration;

	if (state->mms.isOrb && (kASFLiveDuration == state->duration))
		state->spooler->flags |= kFskMediaSpoolerCantSeek;

bail:
	return err;
}

FskErr asfReaderMMSStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	asfReader state = readerState;
	FskErr err;

	state->atTime = startTime ? (UInt32)*startTime : 0;

	state->mms.initializeZeroTime = true;
	state->mms.endOfPackets = false;

	err = doStartMMS(state);
	BAIL_IF_ERR(err);
bail:
	return err;
}

FskErr doStartMMS(asfReader state)
{
	FskErr err;
	ASFStream walker;
	UInt32 streamCount;
	char scratch[64], num[64];
	char *value = NULL;

	if (state->mms.cantReuseConnection || (NULL == state->mms.client)) {
		FskHTTPClientDispose(state->mms.client);

		err = FskHTTPClientNew(&state->mms.client, "asfReaderMMSOverHTTP/2");
		BAIL_IF_ERR(err);

		FskHTTPClientSetRefCon(state->mms.client, state);
	}

	err = FskHTTPClientRequestNew(&state->mms.request, (char *)state->mms.uri);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(state->mms.request, state);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(state->mms.request, asfHeadersReceived, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(state->mms.request, asfDataReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(state->mms.request, asfFinished);

	FskHTTPClientRequestAddHeader(state->mms.request, "User-Agent", kNSPlayerUserAgent);
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "version11-enabled=1");		// seems necessary to allow multiple requests on a single tcp connection

	if (state->mms.clientID)
		FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", state->mms.clientID);
	ensureClientGUID(state);
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", state->mms.clientGUID);

	if (state->mms.playlistSeekID) {
		FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", state->mms.playlistSeekID);
		FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "pl-offset=0"); 	//@@
	}

	if (kASFLiveDuration != state->duration) {
		FskStrCopy(scratch, "stream-time=");
		FskStrNumToStr(state->atTime, scratch + 12, sizeof(scratch) - 12);
		FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", scratch);
	}

	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "xPlayStrm=1");
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "max-duration=0"); 		// we support live content
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "rate=1.000");

	// stream switch count (total stream count)

	for (walker = state->demuxer->streams, streamCount = 0; NULL != walker; walker = walker->next, streamCount++)
		;

	FskStrCopy(scratch, "stream-switch-count=");
	FskStrNumToStr(streamCount, scratch + FskStrLen(scratch), sizeof(scratch) - FskStrLen(scratch));
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", scratch);

	value = FskStrDoCopy("stream-switch-entry=");
	for (walker = state->demuxer->streams; NULL != walker; walker = walker->next) {
		char *temp;

		scratch[0] = 0;
		if (walker != state->demuxer->streams)
			FskStrCat(scratch, " ");
		FskStrCat(scratch, "ffff:");
		FskStrNumToHex(walker->number, num, (walker->number < 16) ? 1 : 2);
		FskStrCat(scratch, num);
		if (walker->disabled)
			FskStrCat(scratch, ":2");
		else if (state->mms.doThinning && (kASFMediaTypeVideo == walker->mediaType))		// we don't try thinning on audio, since it seems to cause bad things to happen
			FskStrCat(scratch, ":1");
		else
			FskStrCat(scratch, ":0");

		temp = FskStrDoCat(value, scratch);
		FskMemPtrDispose(value);
		value = temp;
	}

	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", value);
	FskMemPtrDispose(value);

	//@@
/*
	if (state->mms.xPlayNextEntry) {
		FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "xPlayNextEntry=1");
		state->mms.xPlayNextEntry = false;
	}
*/

	//@@ stream switching selection

	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "AccelDuration=9000");		// accelerate streaming for the first 3 seconds to build up buffer
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "AccelBW=2147483647");		// we're running over TCP, so we can request infinite bandwidth when accelerating (0x7fffffff == 2147483647)
	FskHTTPClientRequestAddHeader(state->mms.request, "Pragma", "no-cache");

	// start
	err = FskHTTPClientBeginRequest(state->mms.client, state->mms.request);
	BAIL_IF_ERR(err);

bail:
	return err;
}

FskErr asfReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;
}

FskErr asfReaderMMSStop(FskMediaReader reader, void *readerState)
{
	asfReader state = readerState;
	FskErr err = kFskErrNone;

	state->mms.starting = false;

	state->mms.ignorePackets = true;		// until we receive a new header

	// toss anything we have pending (but not the working packet... setting ignorePackets will get that)
	while (state->mms.packets) {
		ASFPacket packet = FskListRemoveFirst((void **)&state->mms.packets);
		FskMemPtrDispose(packet);
		state->mms.packetCount -= 1;
	}

	if (state->mms.cantReuseConnection) {
//@@ toss and/or null out request too

		FskHTTPClientDispose(state->mms.client);
		state->mms.client = NULL;
	}
	else {
#if 1
		state->mms.stopping = true;

		if (NULL == state->mms.client) {
			asfFinishedStop(NULL, NULL, state);
			goto bail;
		}

		if (state->mms.request) {
			// don't want to know about any more data received on this request
			FskHTTPClientRequestSetReceivedResponseHeadersCallback(state->mms.request, NULL, 0);
			FskHTTPClientRequestSetReceivedDataCallback(state->mms.request, NULL, NULL, 0, kFskHTTPClientReadAnyData);
			FskHTTPClientRequestSetFinishedCallback(state->mms.request, NULL);
		}

		err = FskHTTPClientRequestNew(&state->mms.stopRequest, (char *)state->mms.uri);
		BAIL_IF_ERR(err);

		FskHTTPClientRequestSetRefCon(state->mms.stopRequest, state);
		FskHTTPClientRequestSetFinishedCallback(state->mms.stopRequest, asfFinishedStop);

		FskHTTPClientRequestAddHeader(state->mms.stopRequest, "User-Agent", kNSPlayerUserAgent);
		FskHTTPClientRequestAddHeader(state->mms.stopRequest, "Accept", "*/*");

		if (state->mms.clientID)
			FskHTTPClientRequestAddHeader(state->mms.stopRequest, "Pragma", state->mms.clientID);
		FskHTTPClientRequestAddHeader(state->mms.stopRequest, "Pragma", "xStopStrm=1");
		FskHTTPClientRequestAddHeader(state->mms.stopRequest, "Content-Length", "0");

		FskHTTPClientRequestSetMethod(state->mms.stopRequest, "POST");

		// stop
		err = FskHTTPClientBeginRequest(state->mms.client, state->mms.stopRequest);
		BAIL_IF_ERR(err);
#endif
	}

bail:
	return err;
}

FskErr asfReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	asfReader state = readerState;
	ASFErr rtCode = kASFErrNone;
	FskErr err = kFskErrNone;

	while (true) {
		void *data;
		UInt32 dataSize;
		ASFStream stream;
		ASFFileOffset presentationTime;
		Boolean keyFrame;
		asfReaderTrack track;
		FskMediaReaderSampleInfo info;

		rtCode = ASFDemuxerNextFrame(state->demuxer, &data, &dataSize, &stream, &presentationTime, &keyFrame);
		if (rtCode) {
			if (kASFErrEndOfPackets == rtCode)
				err = kFskErrEndOfFile;
			goto bail;
		}

		for (track = state->tracks; NULL != track; track = track->next) {
			if (track->stream == stream)
				break;
		}

		if (NULL == track) {
			FskMemPtrDispose(data);
			continue;
		}

		if (state->mms.initializeZeroTime) {
			UInt32 startTime = state->atTime;
			if (kASFLiveDuration != state->duration) {
				UInt32 preroll = (UInt32)state->demuxer->preroll;
				if (startTime < preroll)
					startTime = 0;
				else
					startTime -= preroll;
			}
			state->mms.timeOffset = (startTime * kMilliToNano) - presentationTime;
			state->mms.initializeZeroTime = false;
		}
		presentationTime += state->mms.timeOffset;

		if (state->scrub && !keyFrame) {
			FskMemPtrDispose(data);
			continue;
		}

		if ((kASFMediaTypeAudio == track->stream->mediaType) && (85 == stream->media.audio.formatTag)) {
			UInt32 space;
			unsigned char *d, *e, *dUsed;
			UInt32 dataSizeOut;
			const int kEndSlop = 16;

			if (NULL == track->audio.mp3Data) {
				track->audio.mp3DataSize = 32768;
				err = FskMemPtrNew(track->audio.mp3DataSize, (FskMemPtr *)&track->audio.mp3Data);
				BAIL_IF_ERR(err);

				track->audio.mp3BytesAvailable = 0;
			}

			space = track->audio.mp3DataSize - track->audio.mp3BytesAvailable;
			if (space < dataSize) {
				UInt32 need = dataSize - space;
				FskMemMove(track->audio.mp3Data, track->audio.mp3Data + need, need);
				track->audio.mp3BytesAvailable = need;
			}
			FskMemMove(track->audio.mp3Data + track->audio.mp3BytesAvailable, data, dataSize);
			track->audio.mp3BytesAvailable += dataSize;

			FskMemPtrDisposeAt(&data);

			*dataOut = NULL;
			dataSizeOut = 0;
			*infoOut = NULL;
			*infoCountOut = 0;

			d = track->audio.mp3Data;
			e = track->audio.mp3Data + track->audio.mp3BytesAvailable - kEndSlop;
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

				err = FskMemPtrRealloc(dataSizeOut + mi1.frameLength, (FskMemPtr *)dataOut);
				BAIL_IF_ERR(err);

				err = FskMemPtrRealloc((*infoCountOut + 1) * sizeof(FskMediaReaderSampleInfoRecord), (FskMemPtr *)infoOut);
				BAIL_IF_ERR(err);

				FskMemMove(*dataOut + dataSizeOut, d, mi1.frameLength);
				dataSizeOut += mi1.frameLength;

				i = (*infoOut) + *infoCountOut;
				FskMemSet(i, 0, sizeof(FskMediaReaderSampleInfoRecord));
				i->samples = 1;
				i->sampleSize = mi1.frameLength;
				i->flags = kFskImageFrameTypeSync;
				i->decodeTime = (FskInt64)(presentationTime / 10000.0);
				i->compositionTime = -1;

				*infoCountOut += 1;
				d += mi1.frameLength;

				dUsed = d;
			}

			if (dUsed) {
				FskMemMove(track->audio.mp3Data, dUsed, (e + kEndSlop) - dUsed);
				track->audio.mp3BytesAvailable = (e + kEndSlop) - dUsed;
			}

			if (0 == *infoCountOut)
				continue;

			*trackOut = &track->readerTrack;

			goto bail;
		}

		err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
		if (err) {
			FskMemPtrDisposeAt(&data);
			goto bail;
		}

		info->samples = 1;
		info->sampleSize = dataSize;
		info->flags = keyFrame ? kFskImageFrameTypeSync : kFskImageFrameTypeDifference;
		info->decodeTime = (FskInt64)(presentationTime / 10000.0);
		info->compositionTime = -1;

		*infoCountOut = 1;
		*infoOut = info;
		*trackOut = &track->readerTrack;
		*dataOut = data;

		break;
	}

bail:
	return err;
}

FskErr asfReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	asfReader state = readerState;
	FskErr err = kFskErrUnknownElement;
	ASFDemuxer demuxer = state->demuxer;
	ASFContentDescriptor desc;
	UInt16 *unicodeStr = NULL;
	UInt16 *unicodeType = NULL;

	if (NULL == demuxer)
		return kFskErrNeedMoreTime;

	if (0 == FskStrCompare(metaDataType, "FullName"))
		unicodeStr = demuxer->contentDesc_title;
	else if (0 == FskStrCompare(metaDataType, "Artist")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/AlbumArtist", FskStrLen("WM/AlbumArtist"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc)
			unicodeStr = (UInt16 *)desc->value;
		else
			unicodeStr = demuxer->contentDesc_author;
	}
	else if (0 == FskStrCompare(metaDataType, "Album")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/AlbumTitle", FskStrLen("WM/AlbumTitle"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc)
			unicodeStr = (UInt16 *)desc->value;
	}
	else if (0 == FskStrCompare(metaDataType, "Genre")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/Genre", FskStrLen("WM/Genre"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc)
			unicodeStr = (UInt16 *)desc->value;
	}
	else if (0 == FskStrCompare(metaDataType, "Composer")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/Composer", FskStrLen("WM/Composer"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc)
			unicodeStr = (UInt16 *)desc->value;
	}
	else if (0 == FskStrCompare(metaDataType, "Publisher")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/Publisher", FskStrLen("WM/Publisher"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc)
			unicodeStr = (UInt16 *)desc->value;
	}
	else if (0 == FskStrCompare(metaDataType, "WM/WMCollectionID")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/WMCollectionID", FskStrLen("WM/WMCollectionID"), &unicodeType, NULL);
		desc = ASFDemuxerGetMetadataLibrary(demuxer, 0, unicodeType);
		if (NULL != desc) {
			value->type = kFskMediaPropertyTypeData;
			value->value.data.dataSize = desc->valueLength;
			err = FskMemPtrNewFromData(value->value.data.dataSize, desc->value, (FskMemPtr *)&value->value.data.data);
			BAIL_IF_ERR(err);
		}
	}
	else if (0 == FskStrCompare(metaDataType, "TrackNumber")) {
		FskTextUTF8ToUnicode16NE((const unsigned char *)"WM/TrackNumber", FskStrLen("WM/TrackNumber"), &unicodeType, NULL);
		desc = ASFDemuxerGetExtendedContentDescriptor(demuxer, unicodeType);
		if (NULL != desc) {
			value->type = kFskMediaPropertyTypeInteger;
			FskMemMove(&value->value.integer, desc->value, 4);		// using memmove because desc->value may be misaligned
			err = kFskErrNone;
			goto bail;
		}
	}

	if (NULL != unicodeStr) {
		value->type = kFskMediaPropertyTypeString;
        err = FskTextUnicode16LEToUTF8(unicodeStr, FskUnicodeStrLen(unicodeStr) * 2, (char **)&value->value.str, NULL);
        BAIL_IF_ERR(err);
	}

bail:
	FskMemPtrDispose(unicodeType);

	return err;
}

FskErr asfReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if ((dataSize >= sizeof(gAsfGUID)) && ASFEqualGUIDS(gAsfGUID, data)) {
		*mime = FskStrDoCopy("video/x-ms-asf");
		return kFskErrNone;
	}

	return kFskErrUnknownElement;
}

FskErr asfReaderMMSSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	if ((NULL != uri) && (0 == FskStrCompareWithLength("mms://", uri, 6))) {
		*mime = FskStrDoCopy("application/vnd.ms.wms-hdr.asfv1");
		return kFskErrNone;
	}

	if ((dataSize > 4) && headers && ('$' == data[0]) && (('H' == data[1]) || ('M' == data[1]))) {
		char *mimeIn = FskHeaderFind("Content-Type", headers);
		if ((NULL != mimeIn) && ((0 == FskStrCompareCaseInsensitive(mimeIn, "application/octet-stream")) || (0 == FskStrCompareCaseInsensitive(mimeIn, "application/vnd.ms.wms-hdr.asfv1")) || (0 == FskStrCompareCaseInsensitive(mimeIn, "application/x-mms-framed")))) {
			*mime = FskStrDoCopy("application/vnd.ms.wms-hdr.asfv1");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr asfReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (kASFLiveDuration != state->duration) ? state->duration : -1.0;

	return kFskErrNone;
}

FskErr asfReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr asfReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

FskErr asfReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr asfReaderSetScrub(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	state->scrub = property->value.b;

	return kFskErrNone;
}

FskErr asfReaderGetRedirect(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	if (!state->redirect)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(state->redirect);

	return kFskErrNone;
}

FskErr asfReaderGetError(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->theError;

	return kFskErrNone;
}

FskErr asfReaderGetSeekableSegment(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;
	FskInt64 spoolerPosition;

	if (!state->spooler || !(kFskMediaSpoolerDownloading & state->spooler->flags) || (kASFLiveDuration == state->duration) || !state->demuxer)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeFloat;

	spoolerPosition = state->spoolerPosition;
	if (spoolerPosition < state->demuxer->dataObject_fileOffset)
		property->value.number = 0;
	else {
		UInt32 packet;

		spoolerPosition -= state->demuxer->dataObject_fileOffset;
		packet = (UInt32)(spoolerPosition / state->demuxer->maxPacketSize);
		property->value.number = ((double)packet / (double)state->demuxer->dataPacketsCount) * state->duration;
		if (property->value.number > state->duration)
			property->value.number = state->duration;
		property->value.number /= (double)state->scale;
	}

	return kFskErrNone;
}

FskErr asfReaderSetBitRate(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	state->availableBitRate = property->value.integer;

	return kFskErrNone;
}

FskErr asfReaderGetBufferDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReader state = readerState;

	if (false == state->mms.isOrb)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 4000;

	return kFskErrNone;
}

FskErr asfReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/x-ms-asf\000video/x-ms-wmv\000audio/x-ms-wma\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

FskErr asfReaderMMSGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "application/vnd.ms.wms-hdr.asfv1\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr asfReaderTrackGetMediaType(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeString;
	switch (track->stream->mediaType) {
		case kASFMediaTypeAudio:
			property->value.str = FskStrDoCopy("audio");
			break;

		case kASFMediaTypeVideo:
			property->value.str = FskStrDoCopy("video");
			break;

		default:
			property->value.str = FskStrDoCopy("unknown");
			break;
	}

	return kFskErrNone;
}

FskErr asfReaderTrackGetFormat(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	if (!track->format)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);

	return kFskErrNone;
}

#if 1
void make_esds( unsigned char *codec_data_in, int codec_data_size, unsigned char *esds, int *esds_size )
{
	static unsigned char codec_data[256] = { 0x00, 0x00, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x01, 0xb5, 0x09 };	//9 bytes are patch
	UInt32			s = FskMisaligned32_GetBtoN(&codec_data);
	unsigned char	*d = esds;
	int				size;

	if( codec_data_size > 94 )
		codec_data_size = 94;//this only works when codec_data_size is less than 94 bytes

	if( s != 0x000001b0 && (s >> 10) != 0x20 ) // special case for MPEG-4 in ASF as per the ASF specification section 11.2 - may need to add a prefix to the esds
	{
		memcpy( codec_data + 10, codec_data_in, codec_data_size );
		codec_data_size += 10;
	}
	else
		memcpy( codec_data, codec_data_in, codec_data_size );

	*d++ = 0x00;		//version
	*d++ = 0x00;
	*d++ = 0x00;
	*d++ = 0x00;

	size = 23 + codec_data_size;
	*d++ = 0x03;
	*d++ = size;

	*d++ = 0; *d++ = 0;	//track ID
	*d++ = 0;

	size = 15 + codec_data_size;
	*d++ = 0x04;
	*d++ = size;

	*d++ = 0x20;		//object type indicator
	*d++ = 0x11;		//video

	*d++ = 0x04;		//buffer size DB
	*d++ = 0x07;
	*d++ = 0x40;

	*d++ = 0x00;		//max bitrate
	*d++ = 0x0a;
	*d++ = 0xbe;
	*d++ = 0x00;

	*d++ = 0x00;		//avg bitrate
	*d++ = 0x0a;
	*d++ = 0xbe;
	*d++ = 0x00;

	size = codec_data_size;
	*d++ = 0x05;
	*d++ = size;
	memcpy( (void *)d, (void *)codec_data, codec_data_size );
	d += codec_data_size;

	*d++ = 0x06;
	*d++ = 0x01;
	*d++ = 0x02;

	*esds_size = 4 + 25 + codec_data_size;
}

FskErr asfReaderTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	if (0 == track->stream->typeSpecificDataSize)
		return kFskErrUnimplemented;

	// for mpeg-4, need to convert format info into an image description
	if ((0 == FskStrCompare(track->format, "x-video-codec/mp4")) && (track->stream->media.video.codecSpecificDataSize > 4)) {
		FskErr err;
		unsigned char *desc;
		UInt32 descSize;
		//Boolean needPatch = false;
		UInt32 constesds = 'esds', constmp4v = 'mp4v';

		unsigned char *esds_dst;
		unsigned char esds_src[256];
		int		esds_size;


		make_esds( 	track->stream->media.video.codecSpecificData, track->stream->media.video.codecSpecificDataSize, esds_src, &esds_size );

		esds_size += 8;	//+ box size + 4cc
		descSize = sizeof(QTImageDescriptionRecord) + esds_size;
		err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
		if (err) return err;

		FskMemMove(desc, &descSize, 4);						// *(UInt32 *)desc = (descSize);
		FskMemMove(desc + 4, &constmp4v, 4);				// *(UInt32 *)(desc + 4) = ('mp4v');
		esds_dst = desc + sizeof(QTImageDescriptionRecord);
		FskMemMove(esds_dst, &esds_size, 4);				// *(UInt32 *)esds = (esdsSize);
		FskMemMove(esds_dst + 4, &constesds, 4);			// *(UInt32 *)(esds + 4) = ('esds');
		FskMemMove(esds_dst + 8, esds_src, esds_size-8);

		{//set some extra info
			QTImageDescriptionRecord *image_desc = (QTImageDescriptionRecord *)desc;

			//image_desc->resvd1;							//00 00 00 00
			//image_desc->resvd2;							//00 00
			image_desc->dataRefIndex	= 1;			//01 00
			image_desc->version			= 0;			//00 00
			image_desc->revisionLevel	= 0;			//00 00
			image_desc->vendor			= 0;			//00 00 00 00
			image_desc->temporalQuality = 0x00000200;	//00 02 00 00
			image_desc->spatialQuality  = 0x00000200;	//00 02 00 00
			image_desc->width			= (UInt16)track->video.dimensions.width;
			image_desc->height			= (UInt16)track->video.dimensions.height;
			image_desc->hRes			= 0x00480000;	//00 00 48 00
			image_desc->vRes			= 0x00480000;	//00 00 48 00
			image_desc->dataSize		= 0;			//00 00 00 00
			image_desc->frameCount		= 1;			//01 00
//			image_desc->name[0];						//
			image_desc->depth			= 0x0018;		//18 00
			image_desc->clutID			= 0xffff;		//ff ff
		}

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = descSize;
		property->value.data.data = desc;

		return kFskErrNone;
	}

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = track->stream->typeSpecificDataSize;
	return FskMemPtrNewFromData(track->stream->typeSpecificDataSize, track->stream->typeSpecificData, (FskMemPtr *)&property->value.data.data);
}

#else
FskErr asfReaderTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	if (0 == track->stream->typeSpecificDataSize)
		return kFskErrUnimplemented;

	// for mpeg-4, need to convert format info into an image description
	if ((0 == FskStrCompare(track->format, "x-video-codec/mp4")) && (track->stream->media.video.codecSpecificDataSize > 4)) {
		static unsigned char prefix[] = {0x00, 0x00, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x01, 0xb5, 0x09};
		FskErr err;
		UInt32 s = FskMisaligned32_GetBtoN(&track->stream->media.video.codecSpecificData);
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

		esdsSize = 8 + (needPatch ? 10 : 0) + track->stream->media.video.codecSpecificDataSize;
		descSize = sizeof(QTImageDescriptionRecord) + esdsSize;

		err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
		if (err) return err;

		FskMemMove(desc, &descSize, 4);			// *(UInt32 *)desc = (descSize);
		FskMemMove(desc + 4, &constmp4v, 4);		// *(UInt32 *)(desc + 4) = ('mp4v');
		esds = desc + sizeof(QTImageDescriptionRecord);
		FskMemMove(esds, &esdsSize, 4);			// *(UInt32 *)esds = (esdsSize);
		FskMemMove(esds + 4, &constesds, 4);		// *(UInt32 *)(esds + 4) = ('esds');
		if (!needPatch)
			FskMemMove(esds + 8, track->stream->media.video.codecSpecificData, track->stream->media.video.codecSpecificDataSize);
		else {
			FskMemMove(esds + 8, prefix, 10);
			FskMemMove(esds + 8 + 10, track->stream->media.video.codecSpecificData, track->stream->media.video.codecSpecificDataSize);
		}

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = descSize;
		property->value.data.data = desc;

		return kFskErrNone;
	}

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = track->stream->typeSpecificDataSize;
	return FskMemPtrNewFromData(track->stream->typeSpecificDataSize, track->stream->typeSpecificData, (FskMemPtr *)&property->value.data.data);
}
#endif
FskErr asfReaderTrackGetSampleRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.sampleRate;

	return kFskErrNone;
}

FskErr asfReaderTrackGetBitRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	if (NULL != track->stream->extendedProperties) {
		property->type = kFskMediaPropertyTypeInteger;
		property->value.integer = track->stream->extendedProperties->bitRate;
		return kFskErrNone;
	}

	return kFskErrUnimplemented;
}

FskErr asfReaderTrackGetFrameRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	if ((NULL != track->stream->extendedProperties) && (track->stream->extendedProperties->averageTimePerFrame / 100)) {
		property->type = kFskMediaPropertyTypeRatio;
		property->value.ratio.numer = 100000;
		property->value.ratio.denom = (SInt32)(track->stream->extendedProperties->averageTimePerFrame / 100);
		return kFskErrNone;
	}

	return kFskErrUnimplemented;
}

FskErr asfReaderTrackGetChannelCount(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channelCount;

	return kFskErrNone;
}

FskErr asfReaderTrackGetDimensions(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	asfReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width = track->video.dimensions.width;
	property->value.dimension.height = track->video.dimensions.height;

	return kFskErrNone;
}

/*
	local
*/

static ASFErr asfRead(void *refCon, void *data, ASFFileOffset offset, UInt32 dataSize);
static char *resolveASFCodec(ASFStream stream);
static ASFErr asfAlloc(void *refCon, Boolean clear, UInt32 size, void **data);
static ASFErr asfFree(void *refCon, void *data);

FskErr instantiateASF(asfReader state)
{
	FskErr err = kFskErrNone;
	FskInt64 totalSize;
	UInt8 header[20];

	err = (state->spooler->doGetSize)(state->spooler, &totalSize);
	BAIL_IF_ERR(err);

	err = asfRead(state, header, 0, sizeof(header));
	BAIL_IF_ERR(err);

	state->asfHeaderSize = FskUInt32Read_LtoN(header + 16);

	err = ASFDemuxerNew(&state->demuxer, totalSize, asfRead, state, asfAlloc, (ASFFreeProc)asfFree, state);
	BAIL_IF_ERR(err);

	if (0 == state->demuxer->dataObject_fileOffset)
        BAIL(kFskErrBadData);       // data object is required

	if (0 == state->demuxer->playDuration) {
		UInt8 pBuf[128];
		UInt32 startSend, startDur, endSend, endDur;
		UInt32 packetCount = (UInt32)((totalSize - state->demuxer->dataObject_fileOffset) / state->demuxer->maxPacketSize);

		// read first and last packet to calculate the actual duration, if it fails, treat the file as live.
		if (kFskErrNone == doRead(state, state->demuxer->dataObject_fileOffset, 128, pBuf)) {
			if (kFskErrNone == ASFDemuxerScanPacket(state->demuxer, pBuf, &startSend, &startDur)) {
				if (kFskErrNone == doRead(state, state->demuxer->dataObject_fileOffset + (packetCount - 1) * state->demuxer->maxPacketSize, 128, pBuf)) {
					if (kFskErrNone == ASFDemuxerScanPacket(state->demuxer, pBuf, &endSend, &endDur)) {
						if (endSend <= startSend)
                            BAIL(kFskErrBadData);

						state->demuxer->dataPacketsCount = packetCount;
						state->demuxer->seekable = true;
						state->demuxer->playDuration = ((endSend + endDur) - startSend) * 10000L;		// millisec to nanosec
					}
				}
			}
		}
	}

	err = demuxerInstantiated(state);
	BAIL_IF_ERR(err);

bail:
	state->asfHeaderSize = 0;

	return err;
}


FskErr demuxerInstantiated(asfReader state)
{
	FskErr err = kFskErrNone;
	ASFStream stream;
	UInt32 availableBitRate = state->availableBitRate;
	UInt32 selectedBitRate;
//	ASFContentDescriptor desc;

	if (0 == availableBitRate)
		availableBitRate = 384 * 1024;
	else if (availableBitRate > 384 * 1024)
		availableBitRate = 384 * 1024;			// pin to avoid getting more than we can play (@@ not good for desktop)

	if (state->isMMS && (NULL != state->mms.request->parsedUrl->params)) {
		// cap availableBitRate to WMContentBitrate query parameter
		char *c = state->mms.request->parsedUrl->params;

		while ((NULL != c) && (0 != *c)) {
			if ((0 == FskStrCompareWithLength("WMContentBitrate", c, 16)) && ('=' == c[16])) {
				char *v = FskStrDoCopy(c + 17);
				UInt32 thisBitRate;

				c = FskStrChr(v, '&');
				if (NULL != c)
					*c = 0;
				thisBitRate = FskStrToNum(v);
				if (thisBitRate < availableBitRate)
					availableBitRate = thisBitRate;
				FskMemPtrDispose(v);
				break;
			}

			c = FskStrChr(c, '&');
			if (NULL == c)
				break;

			c += 1;
		}
	}

	resolveAdvancedMutualExclusions(state, availableBitRate);
/*
	// this is a bad idea... this attribute is present for live transcoded Orb streams. Need to find a better way to make this check.
	desc = ASFDemuxerGetExtendedContentDescriptor(state->demuxer, L"WM/MediaIsLive");
	if (NULL != desc)
		state->demuxer->playDuration = 0;			// value of desc is irreelvant. presence of WM/MediaIsLive is enough. See Orb WebCam in Windows Media Player (desktop).
*/
	if (0 != state->demuxer->playDuration)
		state->duration = (UInt32)((state->demuxer->playDuration + 5000.0) / 10000.0);
	else
		state->duration = kASFLiveDuration;		// live
	state->scale = 1000;

	for (stream = state->demuxer->streams, selectedBitRate = 0; NULL != stream; stream = stream->next) {
		asfReaderTrack track = NULL;

		switch (stream->mediaType) {
			case kASFMediaTypeAudio:
				err = FskMemPtrNewClear(sizeof(asfReaderTrackRecord), &track);
				BAIL_IF_ERR(err);

				track->readerTrack.dispatch = &gASFReaderAudioTrack;

				track->format = resolveASFCodec(stream);

				track->audio.channelCount = stream->media.audio.numChannels;
				track->audio.sampleRate = stream->media.audio.samplesPerSecond;
				break;

			case kASFMediaTypeVideo:
				err = FskMemPtrNewClear(sizeof(asfReaderTrackRecord), &track);
				BAIL_IF_ERR(err);

				track->readerTrack.dispatch = &gASFReaderVideoTrack;

				track->format = resolveASFCodec(stream);

				track->video.dimensions.width = stream->media.video.encodedImageWidth;
				track->video.dimensions.height = stream->media.video.encodedImageHeight;
				break;

			default:
				stream->disabled = true;
				break;
		}

		if (stream->encrypted)
			stream->disabled = true;

		if (NULL == track)		// unrecognized stream type
			continue;

		track->state = state;
		track->stream = stream;

		track->readerTrack.state = track;

		FskListAppend((void **)&state->tracks, track);

		if (!stream->disabled && stream->extendedProperties)
			selectedBitRate += stream->extendedProperties->bitRate;
	}

	state->mms.doThinning = selectedBitRate > availableBitRate;

bail:
	return err;
}

char *resolveASFCodec(ASFStream stream)
{
	if (kASFMediaTypeAudio == stream->mediaType) {
		if ((1 == stream->media.audio.formatTag) && (16 == stream->media.audio.bitsPerSample))
			return "x-audio-codec/pcm-16-le";
		else if (353 == stream->media.audio.formatTag)
			return "x-audio-codec/wma";
		else if (354 == stream->media.audio.formatTag)
			return "x-audio-codec/wma-pro";
		else if (355 == stream->media.audio.formatTag)
			return "x-audio-codec/wma-lossless";
		else if (10 == stream->media.audio.formatTag)
			return "x-audio-codec/wma-voice";
		else if (85 == stream->media.audio.formatTag)
			return "x-audio-codec/mp3";
		else if (17 == stream->media.audio.formatTag)
			return "x-audio-codec/adpcm-dvi";

		return "x-audio-codec/unknown";
	}
	else
	if (kASFMediaTypeVideo == stream->mediaType) {
		if (('2S4M' == stream->media.video.compressionID) || ('S4PM' == stream->media.video.compressionID) || ('2s4m' == stream->media.video.compressionID) || ('s4mp' == stream->media.video.compressionID))
			return "x-video-codec/mp4";
		else if ('1VMW' == stream->media.video.compressionID)
			return "x-video-codec/wmv7";
		else if ('2VMW' == stream->media.video.compressionID)
			return "x-video-codec/wmv8";
		else if ('3VMW' == stream->media.video.compressionID)
			return "x-video-codec/wmv9";
		else if ('AVMW' == stream->media.video.compressionID)
			return "x-video-codec/wmva";
		else if ('1CVW' == stream->media.video.compressionID)
			return "x-video-codec/wvc1";

		return "x-video-codec/unknown";
	}

	return NULL;
}

ASFErr asfRead(void *refCon, void *data, ASFFileOffset offset, UInt32 dataSize)
{
	asfReader state = refCon;
	FskErr err;

	if (0 == dataSize)
		return kFskErrNone;			// this happens sometimes when there is an empty atom

	err = doRead(state, offset, dataSize, data);
	if ((kFskErrNeedMoreTime == err) && (0 != state->asfHeaderSize)) {
		if (offset > (state->asfHeaderSize + 24))			// we are trying to read beyond the data block
			err = kASFErrDataUnavailableAfterHeader;
	}

	return err;
}

ASFErr asfAlloc(void *refCon, Boolean clear, UInt32 size, void **data)
{
	if (clear)
		return FskMemPtrNewClear(size, (FskMemPtr *)data);

	return FskMemPtrNew(size, (FskMemPtr *)data);
}

ASFErr asfFree(void *refCon, void *data)
{
	FskMemPtrDispose(data);
	return kFskErrNone;
}

FskErr doRead(asfReader state, FskInt64 offset, UInt32 size, void *bufferIn)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;

	while (0 != size) {
		unsigned char *readBuffer;
		UInt32 bytesRead;

		if ((0 != state->asfHeaderSize) && (offset > (state->asfHeaderSize + 24)))			// beyond the mms header, we don't want to work too hard on a streaming connection
			state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;

		err = FskMediaSpoolerRead(state->spooler, offset, size, &readBuffer, &bytesRead);
		state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		if (err) return err;

		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

	return err;
}

FskErr asfInstantiate(asfReader state)
{
	FskErr err;

	err = instantiateASF(state);
	BAIL_IF_ERR(err);

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}

FskErr asfSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	asfReader state = clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			state->spoolerPosition += (UInt32)param;

			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = asfInstantiate(state);
			break;

		case kFskMediaSpoolerOperationGetURI:
			state->spoolerPosition = ((FskMediaSpoolerGetURI)param)->position;
			break;
	}

	return err;
}

/*
	for mms over http
*/

static ASFErr memoryReadProc(void *refCon, void *data, ASFFileOffset offset, UInt32 dataSize);
static ASFErr asfGetNextPacket(void *readerRefCon, void *data);

FskErr asfHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon)
{
	asfReader state = refCon;
	char *value;
	int responseCode = FskHeaderResponseCode(request->responseHeaders);

	FskMemPtrDisposeAt((void **)&state->mms.asfHeader);
	state->mms.asfHeaderSize = 0;
	state->mms.metaStartOffset = 0;

	state->mms.packetParseState = state->demuxer ? kPacketParseStateHeaderBegin : kPacketParseStateInitialize;
	state->mms.partial = 0;

	// handle redirects
	if ((301 == responseCode) || (302 == responseCode) || (303 == responseCode) || (305 == responseCode) || (307 == responseCode)) {
		char *redirect = FskHeaderFind(kFskStrLocation, responseHeaders);
		char *location = NULL;
		FskErr err;

		if (NULL == redirect)
			return kFskErrOperationCancelled;

		if ('/' != redirect[0]) {
			if (0 != FskStrStr(redirect, "://"))
				location = FskStrDoCopy(redirect);
			else {
				char *t = FskStrDoCat(request->parsedUrl->scheme, "://");
				location = FskStrDoCat(t, redirect);		// no protocol given, so use the current one
				FskMemPtrDispose(t);
			}
		}
		else {
			if (kFskErrNone == FskMemPtrNew(FskStrLen(request->parsedUrl->scheme) + 4 + FskStrLen(request->parsedUrl->host) + FskStrLen(redirect), &location)) {
				FskStrCopy(location, request->parsedUrl->scheme);
				FskStrCat(location, "://");
				FskStrCat(location, request->parsedUrl->host);
				FskStrCat(location, redirect);
			}
		}

		FskMemPtrDispose(state->mms.uri);
		state->mms.uri = location;

		FskHTTPClientRequestSetFinishedCallback(state->mms.request, NULL);
		//@@ don't need to dispose request here?
		state->mms.request = NULL;

		FskHTTPClientDispose(state->mms.client);
		state->mms.client = NULL;

		if (NULL == state->demuxer)
			err = doInstantiateMMS(state);
		else
			err = doStartMMS(state);
        if (err) {
            state->theError = err;
            (state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
        }
		return kFskErrOperationCancelled;
	}

	if (NULL == state->mms.clientID) {
		// extract client-id from the headers
		value = FskHeaderFind("Pragma", responseHeaders);
		if (NULL != value) {
			char *p = value;
			while (*p) {
				char *comma;
				UInt32 length;

				while (*p == ' ')
					p++;

				comma = FskStrChr(p, ',');
				if (0 != FskStrCompareWithLength("client-id=", p, 10)) {
					if (NULL == comma)
						break;
					p = comma + 1;
					continue;
				}
				if (NULL != comma)
					length = comma - p;
				else
					length = FskStrLen(p);
				if (kFskErrNone == FskMemPtrNew(length + 1, (FskMemPtr *)&state->mms.clientID)) {
					FskMemMove(state->mms.clientID, p, length);
					state->mms.clientID[length] = 0;
				}
				break;
			}
		}
	}

	// this check may need to be adjusted
	value = FskHeaderFind("Server", responseHeaders);
	if ((NULL != value) && (0 == FskStrCompareWithLength(value, "Cougar 4.", 9)))
		state->mms.cantReuseConnection = true;

	return kFskErrNone;
}

FskErr asfDataReceived(FskHTTPClientRequest request, char *buffer, int bufferSizeIn, void *refCon)
{
	asfReader state = refCon;
	FskErr err = kFskErrNone;
	unsigned char *b = (unsigned char *)buffer;
	UInt32 bufferSize = bufferSizeIn;
	Boolean ended = false, dataArrived = false;

	FskMediaReaderUsing(state->reader, true);

	while (bufferSize) {
nextState:
		switch (state->mms.packetParseState) {
			case kPacketParseStateInitialize:
				if ((bufferSize >= sizeof(gAsfGUID)) && ASFEqualGUIDS(gAsfGUID, (unsigned char*)buffer)) {
					if (0 == FskStrCompareWithLength("mms://", state->mms.uri, 6))
						state->redirect = FskStrDoCat("http", state->mms.uri + 3);
					else
						state->redirect = FskStrDoCopy(state->mms.uri);

					FskHTTPClientDispose(state->mms.client);
					state->mms.client = NULL;
					state->mms.request = NULL;

					(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

					return kFskErrNone;
				}
				state->mms.packetParseState = kPacketParseStateHeaderBegin;
				break;

			case kPacketParseStateHeaderBegin:
				state->mms.packetParseState = kPacketParseStateCollect;
				state->mms.packetParseStateNext = kPacketParseStateHeaderEnd;
				state->mms.bytesToCollect = 4;
				break;

			case kPacketParseStateHeaderEnd: {
				UInt16 tag = (state->mms.partialBuffer[1] << 8) | (0x07f & state->mms.partialBuffer[0]);	//@@ why is AND with 0x7f needed? (maybe because "when the server sends a burst of packets, it serts a special bit in AF_HEADER_TYPE0 to indicate that it has done so. The client uses this bit as an indicator that it can use the packets to measure the link bit rate." maybe.)
				UInt16 tagSize = (state->mms.partialBuffer[3] << 8) | state->mms.partialBuffer[2];
				// FIXME: endian probelm?
				const UInt16 tag_const[] = {
					(UInt16)('D'<<8 | '$'),
					(UInt16)('H'<<8 | '$'),
					(UInt16)('M'<<8 | '$'),
					(UInt16)('C'<<8 | '$'),
					(UInt16)('E'<<8 | '$'),
				};

				if (tag_const[0] == tag) {
					state->mms.packetParseState = kPacketParseStateCollect;
					state->mms.packetParseStateNext = kPacketParseStateDataPacketBegin;
					state->mms.bytesToCollect = 8;
					state->mms.bytesToSkip = tagSize - 8;		//@@
				}
				else if (tag_const[1] == tag) {
					if (state->reader->mediaState < kFskMediaPlayerStateStopped) {
						state->mms.packetParseState = kPacketParseStateASFHeaderFill;
						state->mms.bytesToSkip = 8;
						state->mms.bytesToCollect = tagSize - 8;
					}
					else {
						state->mms.packetParseState = kPacketParseStateSkip;
						state->mms.bytesToSkip = tagSize;
					}

					if (state->mms.ignorePackets) {
						state->mms.initializeZeroTime = true;
						state->mms.ignorePackets = false;
					}
				}
				else if (tag_const[2] == tag) {
					state->mms.packetParseState = kPacketParseStateASFMetaFill;
					state->mms.asfMetaSize = 0;
					state->mms.bytesToCollect = tagSize;

					FskMemPtrDisposeAt((void **)&state->mms.metaTitle);
					FskMemPtrDisposeAt((void **)&state->mms.playlistSeekID);
				}
				else if (tag_const[3] == tag) {
					state->mms.packetParseState = kPacketParseStateSkip;
					state->mms.bytesToSkip = tagSize;
				}
				else if (tag_const[4] == tag) {
					state->mms.endOfPackets = true;
					state->mms.packetParseState = kPacketParseStateCollect;
					state->mms.packetParseStateNext = kPacketParseStateEndEnd;
					state->mms.bytesToSkip = 0;
					state->mms.bytesToCollect = tagSize;

					ended = true;

					if (NULL != state->mms.client) {
						FskHTTPClientDispose(state->mms.client);
						state->mms.client = NULL;
						state->mms.request = NULL;
						state->mms.stopRequest = NULL;
					}
				}
				else {
					state->mms.packetParseState = kPacketParseStateSkip;
					state->mms.bytesToSkip = tagSize;
				}
				}
				break;

			case kPacketParseStateDataPacketBegin:
				if (kFskErrNone != FskMemPtrNew(sizeof(ASFPacketRecord) + state->mms.bytesToSkip, (FskMemPtr *)&state->mms.workingPacket)) {
					state->mms.packetParseState = kPacketParseStateSkip;
					break;
				}

				state->mms.workingPacket->next = NULL;
				state->mms.workingPacket->number = (UInt32)state->mms.partialBuffer[0] | ((UInt32)state->mms.partialBuffer[1] << 8) | ((UInt32)state->mms.partialBuffer[2] << 16) | ((UInt32)state->mms.partialBuffer[3] << 24);
				state->mms.workingPacket->bytesInPacket = 0;
				state->mms.workingPacket->dataBytes = state->mms.bytesToSkip;

				state->mms.packetParseState = kPacketParseStateDataPacketFill;
				break;

			case kPacketParseStateDataPacketFill: {
				UInt32 bytesToUse = bufferSize;
				UInt32 bytesNeeded = state->mms.workingPacket->dataBytes - state->mms.workingPacket->bytesInPacket;
				if (bufferSize > bytesNeeded)
					bytesToUse = bytesNeeded;
				FskMemMove(&state->mms.workingPacket->data[state->mms.workingPacket->bytesInPacket], b, bytesToUse);
				b += bytesToUse;
				bufferSize -= bytesToUse;
				state->mms.workingPacket->bytesInPacket += bytesToUse;
				if (state->mms.workingPacket->bytesInPacket == state->mms.workingPacket->dataBytes) {
					if (false == state->mms.ignorePackets) {
						ASFPacket walker = state->mms.packets;
						if (NULL == walker)
							state->mms.packets = state->mms.workingPacket;
						else {
							while (walker->next)
								walker = walker->next;

							walker->next = state->mms.workingPacket;
						}
					}
					else
						FskMemPtrDispose(state->mms.workingPacket);

					state->mms.packetCount += 1;
					state->mms.workingPacket = NULL;

					state->mms.packetParseState = kPacketParseStateHeaderBegin;

					dataArrived = true;
				}
				}
				break;

			case kPacketParseStateASFHeaderFill: {
				UInt32 bytesToUse;

				if (0 != state->mms.bytesToSkip) {
					if (state->mms.bytesToSkip >= bufferSize) {
						state->mms.bytesToSkip -= bufferSize;
						b += bufferSize;
						bufferSize = 0;
						break;
					}
					b += state->mms.bytesToSkip;
					bufferSize -= state->mms.bytesToSkip;
					state->mms.bytesToSkip = 0;
				}

				bytesToUse = bufferSize;
				if (bytesToUse > state->mms.bytesToCollect)
					bytesToUse = state->mms.bytesToCollect;
				if (kFskErrNone != FskMemPtrRealloc(state->mms.asfHeaderSize + bytesToUse, (FskMemPtr *)&state->mms.asfHeader)) {
//@@					setReportableErrorIf(asf->media, kFskErrMemFull);
					goto bail;
				}
				FskMemMove(state->mms.asfHeader + state->mms.asfHeaderSize, b, bytesToUse);
				b += bytesToUse;
				state->mms.asfHeaderSize += bytesToUse;
				bufferSize -= bytesToUse;
				state->mms.bytesToCollect -= bytesToUse;
				if (0 == state->mms.bytesToCollect) {
					state->mms.packetParseState = kPacketParseStateHeaderBegin;
					if (state->mms.asfHeaderSize > 64) {
						UInt32 asfHeaderSize = state->mms.asfHeader[16] | (state->mms.asfHeader[17] << 8) | (state->mms.asfHeader[18] << 16) | (state->mms.asfHeader[19] << 24);
						if (asfHeaderSize <= state->mms.asfHeaderSize) {
							state->mms.asfHeaderSize = asfHeaderSize;		// some servers seem to send bonus data, so truncate to the exact header size

							err = ASFDemuxerNew(&state->demuxer, state->mms.asfHeaderSize, memoryReadProc, state, asfAlloc, (ASFFreeProc)asfFree, state);

							FskMemPtrDisposeAt((void **)&state->mms.asfHeader);
							state->mms.asfHeaderSize = 0;

							if (kFskErrNone != err) {
//@@							setReportableErrorIf(asf->media, kKinomaErrUnrecognizedResponse);
								goto bail;
							}

							err = demuxerInstantiated(state);
							if (kFskErrNone == err) {
								state->demuxer->seekable = false;
								ASFDemuxerSetGetNextPacketProc(state->demuxer, asfGetNextPacket);

								(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);
							}
						}
					}
				}
				}
				break;

			case kPacketParseStateASFMetaFill: {
				UInt32 bytesToUse = bufferSize;
				if (bytesToUse > state->mms.bytesToCollect)
					bytesToUse = state->mms.bytesToCollect;
				if (kFskErrNone != FskMemPtrRealloc(state->mms.asfMetaSize + bytesToUse + 1, (FskMemPtr *)&state->mms.asfMeta)) {
//@@				setReportableErrorIf(asf->media, kFskErrMemFull);
					goto bail;
				}
				FskMemMove(state->mms.asfMeta + state->mms.asfMetaSize, b, bytesToUse);
				state->mms.asfMeta[state->mms.asfMetaSize] = 0;
				b += bytesToUse;
				state->mms.asfMetaSize += bytesToUse;
				bufferSize -= bytesToUse;
				state->mms.bytesToCollect -= bytesToUse;
				if (0 == state->mms.bytesToCollect) {
					char *p = (char *)state->mms.asfMeta;
					char *end = p + state->mms.asfMetaSize;
					char *id = p;
					UInt32 value = 0, cdlType;
					Boolean valid = false;

					p += 8;
					while (p < end) {
						if (0 == *p++)
							break;
					}
					if (p >= end)
						goto metaFail;

					if (((p - id) >= 17) && (0 == FskStrCompareCaseInsensitiveWithLength("playlist-gen-id=", id, 16))) {
						char *equal = FskStrChr(id, '=');
						char *comma = FskStrChr(id, ',');
						if (comma && equal && (equal < comma) && (comma < p)) {
							*comma = 0;
							state->mms.playlistSeekID = FskStrDoCat("playlist-gen-id=", equal + 1);
						}
					}


					while (p < end) {
						char *term[5];
						int i;

						for (i=0; i<5 && (p < end); i++) {
							term[i] = p;
							while (p < end) {
								if (',' == *p++) {
									p[-1] = 0;
									break;
								}
							}
						}

						if (p >= end)
							break;

						cdlType = FskStrToNum(term[2]);
						switch (cdlType) {
							case 11:		// boolean
							case 16:		// 1 byte int
							case 2:			// 2 byte int
							case 3:			// 4 byte int
							case 17:		// 1 byte int
							case 18:		// 2 byte int
							case 19:		// 4 byte int
							case 23:		// 4 byte int
								value = FskStrToNum(term[4]);
								valid = true;
								break;
						}

						if (valid) {
							if (0 == FskStrCompare(term[1], "WMS_CONTENT_DESCRIPTION_PLAYLIST_ENTRY_START_OFFSET")) {
								state->mms.metaStartOffset = value;
							}
							else
							if ((31 == cdlType) && (0 == FskStrCompare(term[1], "title"))) {
								state->mms.metaTitle = FskStrDoCopy(term[4]);
							}
						}
					}

metaFail:
					state->mms.packetParseState = kPacketParseStateHeaderBegin;
					FskMemPtrDisposeAt((void *)&state->mms.asfMeta);
				}
				}
				break;

			case kPacketParseStateCollect: {
				UInt32 bytesNeeded = state->mms.bytesToCollect;
				if (bytesNeeded > bufferSize)
					bytesNeeded = bufferSize;
				FskMemMove(&state->mms.partialBuffer[state->mms.partial], b, bytesNeeded);
				state->mms.partial += bytesNeeded;
				b += bytesNeeded;
				bufferSize -= bytesNeeded;
				state->mms.bytesToCollect -= bytesNeeded;
				if (0 == state->mms.bytesToCollect) {
					state->mms.packetParseState = state->mms.packetParseStateNext;
					state->mms.partial = 0;
					goto nextState;
				}
				}
				break;

			case kPacketParseStateSkip:
				if (bufferSize >= state->mms.bytesToSkip) {
					b += state->mms.bytesToSkip;
					bufferSize -= state->mms.bytesToSkip;
					state->mms.bytesToSkip = 0;
					state->mms.packetParseState = kPacketParseStateHeaderBegin;
				}
				else {
					state->mms.bytesToSkip -= bufferSize;
					bufferSize = 0;
				}
				break;

			case kPacketParseStateEndEnd: {
				SInt32 endCode = (UInt32)state->mms.partialBuffer[0] | ((UInt32)state->mms.partialBuffer[1] << 8) | ((UInt32)state->mms.partialBuffer[2] << 16) | ((UInt32)state->mms.partialBuffer[3] << 24);

				if (endCode > 0) {
					ended = false;
					state->mms.endOfPackets = false;
				}
//@@
/*
				if (-1 == asf->media->duration) {
					if (endCode < 0)
						setReportableErrorIf(asf->media, kKinomaErrServerEndedStream);
					else
					if (NULL != state->mms.playlistSeekID) {
						state->mms.xPlayNextEntry = true;

						if (errNone == asfHttpBeginAtTime(asf, TimeNow))
							changeHandlerState(asf, kMediaPlayerStateBuffering);
					}
				}
*/
				state->mms.packetParseState = kPacketParseStateHeaderBegin;
				}
				break;
		}
	}

	if ((ended || dataArrived) && (NULL != state->reader->eventHandler))
		(state->reader->eventHandler)(state->reader, state->reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);

bail:
	FskMediaReaderUsing(state->reader, false);

	return err;
}

FskErr asfFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	asfReader state = refCon;
	int httpResponse = FskHeaderResponseCode(request->responseHeaders);

	if (state->mms.request == request)
		state->mms.request = NULL;

	if ((kFskErrNone != client->status.lastErr) || (200 != httpResponse)) {
		state->theError = (kFskErrNone != client->status.lastErr) ? client->status.lastErr : httpResponse;
		(state->reader->doSetState)(state->reader, kFskMediaPlayerStateFailed);
		// note: doSetState can result in this instance being disposed, so don't access state after the call
	}

	return kFskErrNone;
}

// note: may be called with client & request set to NULL
FskErr asfFinishedStop(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	asfReader state = refCon;

	flushPackets(state);

	state->mms.stopping = false;
	if (state->mms.starting) {

		state->mms.starting = false;
	}

	state->mms.stopRequest = NULL;		//@@ dispose too?

	return kFskErrNone;
}

ASFErr memoryReadProc(void *refCon, void *data, ASFFileOffset offset, UInt32 dataSize)
{
	asfReader state = refCon;

	if ((offset + dataSize) > state->mms.asfHeaderSize)
		return kASFErrEndOfFile;

	FskMemMove(data, state->mms.asfHeader + offset, dataSize);

	return kFskErrNone;
}

ASFErr asfGetNextPacket(void *readerRefCon, void *data)
{
	asfReader state = readerRefCon;
	ASFPacket next;

	if (NULL == state->mms.packets) {
		if ((false == state->mms.endOfPackets) && (state->reader->mediaState >= kFskMediaPlayerStatePlaying))
			state->mms.hadUnderflow = true;
		return state->mms.endOfPackets ? kASFErrEndOfPackets : kFskErrNeedMoreTime;
	}

	if (state->demuxer->maxPacketSize < state->mms.packets->dataBytes)
		return kASFErrEndOfFile;

	if (state->demuxer->maxPacketSize > state->mms.packets->dataBytes)
		FskMemSet(state->mms.packets->dataBytes + (unsigned char *)data, 0, state->demuxer->maxPacketSize - state->mms.packets->dataBytes);

	FskMemMove(data, state->mms.packets->data, state->mms.packets->dataBytes);
	next = state->mms.packets->next;
	FskMemPtrDispose(state->mms.packets);
	state->mms.packets = next;
	state->mms.packetCount -= 1;

//@@
/*
	if (state->mms.suspended) {
		if ((state->mms.packetCount * asf->demuxer->maxPacketSize) < (((float)asf->demuxer->maxBitRate / (float)800) * (float)(2 * kBufferTicks))) {
			FskHTTPClientResume(state->mms.client);
			state->mms.suspended = false;
		}
	}
*/

	return kASFErrNone;
}

void flushPackets(asfReader state)
{
	while (state->mms.packets) {
		ASFPacket packet = FskListRemoveFirst((void **)&state->mms.packets);
		FskMemPtrDispose(packet);
	}

	if (state->mms.workingPacket) {
		if (kPacketParseStateDataPacketFill == state->mms.packetParseState) {
			state->mms.bytesToSkip = state->mms.workingPacket->dataBytes - state->mms.workingPacket->bytesInPacket;
			state->mms.packetParseState = kPacketParseStateSkip;
		}
		FskMemPtrDisposeAt((void **)&state->mms.workingPacket);
	}

	state->mms.packetCount = 0;
}


void ensureClientGUID(asfReader state)
{
	UInt16 i;

	if (*state->mms.clientGUID)
		return;

	FskStrCopy(state->mms.clientGUID, "xClientGUID={3300AD50-2C39-46c0-AE0A-");		// base GUID for privacy (as per Microsoft spec)
	for (i=0; i<12; i++) {
		char c[2];
		FskStrNumToHex(FskRandom(), c, 1);
		FskStrCat(state->mms.clientGUID, c);
	}
	FskStrCat(state->mms.clientGUID, "}");
}


/*
	exclusion groups
*/

static Boolean streamIsPlayable(asfReader state, ASFStream stream);

static void selectFromGroup(asfReader state, ASFAdvancedMutualExclusion ame, ASFStream keeper);

static ASFStream findBestFitStream(asfReader state, ASFAdvancedMutualExclusion ame, UInt32 targetBitRate);


void resolveAdvancedMutualExclusions(asfReader state, UInt32 availableBitRate)
{
	ASFAdvancedMutualExclusion ame;
	ASFAdvancedMutualExclusion videoAME = NULL, audioAME = NULL;
	UInt32 smallestAMEAudioBitRate = 0, ungroupedBitrate = 0;
	ASFStream keeper;
	UInt32 workingBitRate = availableBitRate;
	ASFDemuxer demuxer = state->demuxer;
	ASFStream stream;
//	Boolean insufficientBandwidth = false;

	if (NULL == demuxer->advancedMutualExclusions)
		return;

	for (ame = demuxer->advancedMutualExclusions; NULL != ame; ame = ame->next) {
		ASFStream stream = findBestFitStream(state, ame, 0);			// find lowest bitrate stream to determine the media type of this group
		if (NULL == stream) continue;

		if (kASFMediaTypeAudio == stream->mediaType) {
			audioAME = ame;
			if (NULL != stream->extendedProperties)
				smallestAMEAudioBitRate = stream->extendedProperties->bitRate;
		}
		else
		if (kASFMediaTypeVideo == stream->mediaType)
			videoAME = ame;
	}

	for (stream = demuxer->streams; NULL != stream; stream = stream->next) {
		Boolean inGroup = false;

		for (ame = demuxer->advancedMutualExclusions; (NULL != ame) && (false == inGroup); ame = ame->next) {
			UInt16 i;

			for (i=0; (i < ame->count) && (false == inGroup); i++) {
				if (ame->streams[i] == stream->number)
					inGroup = true;
			}
		}

		if (inGroup || (NULL == stream->extendedProperties))
			continue;

		ungroupedBitrate += stream->extendedProperties->bitRate;
	}

	if (ungroupedBitrate <= workingBitRate)
		workingBitRate -= ungroupedBitrate;
	else {
		workingBitRate = 0;
//		insufficientBandwidth = true;
	}

	if (videoAME) {
		// select video stream while trying to leave room for audio
		if (audioAME)
			keeper = findBestFitStream(state, videoAME, (smallestAMEAudioBitRate < workingBitRate) ? (workingBitRate - smallestAMEAudioBitRate) : 0);
		else
			keeper = findBestFitStream(state, videoAME, workingBitRate);
		selectFromGroup(state, videoAME, keeper);
		if (NULL != keeper->extendedProperties) {
			if (keeper->extendedProperties->bitRate <= workingBitRate)
				workingBitRate -= keeper->extendedProperties->bitRate;
			else {
				workingBitRate = 0;
//				insufficientBandwidth = true;
			}
		}
	}

	if (audioAME) {
		// select audio stream
		keeper = findBestFitStream(state, audioAME, workingBitRate);
		selectFromGroup(state, audioAME, keeper);
//		if (NULL != keeper->extendedProperties) {
//			if (keeper->extendedProperties->bitRate <= workingBitRate)
//				workingBitRate -= keeper->extendedProperties->bitRate;
//			else {
//				workingBitRate = 0;
//				insufficientBandwidth = true;
//			}
//		}
	}

	// resolve any remaining groups (we don't expect any)
	for (ame = demuxer->advancedMutualExclusions; NULL != ame; ame = ame->next) {
		if ((ame == audioAME) || (ame == videoAME))
			continue;

		keeper = findBestFitStream(state, ame, 0);			// lowest bitrate on these mystery media
		selectFromGroup(state, ame, keeper);
	}
}

Boolean streamIsPlayable(asfReader state, ASFStream stream)
{
	char *codec;

	if (NULL == stream)
		return false;

	codec = resolveASFCodec(stream);
	if (NULL == codec)
		return false;

	switch (stream->mediaType) {
		case kASFMediaTypeAudio:
			return (kFskErrNone == FskAudioDecompressNew(NULL, 0, codec, 0, 0, NULL, 0)) ? true : false;

		case kASFMediaTypeVideo:
			// could filter on video bitrate and/or frame rate and/or frame size here too
			return (kFskErrNone == FskImageDecompressNew(NULL, 0, codec, NULL)) ? true : false;

		case kASFMediaTypeImageJFIF:
			return true;
	}

	return false;

}



ASFStream findBestFitStream(asfReader state, ASFAdvancedMutualExclusion ame, UInt32 targetBitRate)
{
	ASFStream result = NULL;
	UInt16 i;

	// find first playable stream, to use as the default (if none are playable, we arbitrarily return the last one)
	for (i = 0; i < ame->count; i++) {
		result = ASFDemuxerStreamGet(state->demuxer, ame->streams[i]);
		if (streamIsPlayable(state, result))
			break;
	}

	// we only resolve bitrate groups
	if ((kASFDemuxerExclusionBitRate != ame->exclusionType) || !result)
		goto bail;

	// check other streams to see if there is a better fit available
	for (; i < ame->count; i++) {
		ASFStream stream = ASFDemuxerStreamGet(state->demuxer, ame->streams[i]);
		UInt32 bestBitRate, thisBitRate;

		if (false == streamIsPlayable(state, stream))
			continue;

		if (NULL == result->extendedProperties) {
			if (NULL != stream->extendedProperties)
				result = stream;
			continue;
		}

		thisBitRate = stream->extendedProperties->bitRate;
		bestBitRate = result->extendedProperties->bitRate;

		if (bestBitRate > targetBitRate) {
			if (thisBitRate < bestBitRate)
				result = stream;
		}
		else {
			if ((thisBitRate < targetBitRate) && (thisBitRate > bestBitRate))
				result = stream;
		}
	}

bail:
	return result;
}

void selectFromGroup(asfReader state, ASFAdvancedMutualExclusion ame, ASFStream keeper)
{
	UInt16 i;

	for (i = 0; i < ame->count; i++) {
		ASFStream stream = ASFDemuxerStreamGet(state->demuxer, ame->streams[i]);
		if (NULL == stream) continue;

		if (state->isMMS)
			stream->disabled = stream != keeper;
		else
		if (stream != keeper)
			ASFDemuxerStreamDispose(stream);
	}
}
