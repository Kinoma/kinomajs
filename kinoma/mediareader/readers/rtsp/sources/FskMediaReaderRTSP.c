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
#include "FskFiles.h"
#include "FskMediaReader.h"
#include "FskTextConvert.h"
#include "FskEnvironment.h"
#include "RTSPSession.h"
#include "FskDIDLGenMedia.h"
#include "FskAudio.h"
#include "FskTime.h"
#include "QTReader.h"
#include "QTMoviesFormat.h"

#include "stddef.h"

FskInstrumentedSimpleType(RTSPReader, rtspreader);

#define SUPPORT_B_FRAMES 1
#define RTSP_OPTIONS_INTERVAL_SECS 60
#define RTSP_PACKET_RECEIVE_TIMEOUT_SECS 10
#define kMSToBuffer 7000

#define WriteLongN( v, p )  { p[0] = (UInt8)(((v)&0x000000ff)); p[1] = (UInt8)(((v)&0x0000ff00)>>8) ; p[2] = (UInt8)(((v)&0x00ff0000)>>16); p[3] = (UInt8)(((v)&0xff000000)>>24); }
#define FskEndianU32_NtoN(v) (v)
#define FskEndianU16_NtoN(v) (v)

/*
	root reader
*/

static Boolean rtspReaderCanHandle(const char *mimeType);
static FskErr rtspReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr rtspReaderDispose(FskMediaReader reader, void *readerState);
static FskErr rtspReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr rtspReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr rtspReaderStop(FskMediaReader reader, void *readerState);
static FskErr rtspReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr rtspReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr rtspReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);
static FskErr rtspReaderGetRedirect(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderGetError(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskErr rtspReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskErr rtspReaderTrackGetMediaType(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderTrackGetFormat(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderGetBufferDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord rtspReaderProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			rtspReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			rtspReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		rtspReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		rtspReaderGetState,			NULL},
	{kFskMediaPropertyBufferDuration,		kFskMediaPropertyTypeInteger,		rtspReaderGetBufferDuration,NULL},
	{kFskMediaPropertyRedirect,				kFskMediaPropertyTypeString,		rtspReaderGetRedirect,		NULL},
	{kFskMediaPropertyError,				kFskMediaPropertyTypeInteger,		rtspReaderGetError,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						rtspReaderSetScrub},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderRTSP = {rtspReaderCanHandle, rtspReaderNew, rtspReaderDispose, rtspReaderGetTrack, rtspReaderStart, rtspReaderStop, rtspReaderExtract, rtspReaderGetMetadata, rtspReaderProperties, rtspReaderSniff};

/*
	audio track
*/

static FskErr rtspReaderAudioTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderAudioTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderAudioTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord rtspReaderAudioTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		rtspReaderTrackGetMediaType,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		rtspReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		rtspReaderAudioTrackGetSampleRate,		NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		rtspReaderAudioTrackGetChannelCount,	NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		rtspReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			rtspReaderAudioTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,									NULL}
};

FskMediaReaderTrackDispatchRecord gMediaReaderRTSPAudioTrack = {rtspReaderAudioTrackProperties};

/*
	video track
*/

static FskErr rtspReaderVideoTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderVideoTrackGetTimeScale(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr rtspReaderVideoTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord rtspReaderVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		rtspReaderTrackGetMediaType,		NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		rtspReaderVideoTrackGetTimeScale,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		rtspReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			rtspReaderVideoTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		rtspReaderVideoTrackGetDimensions,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		rtspReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,									NULL}
};

FskMediaReaderTrackDispatchRecord gMediaReaderRTSPVideoTrack = {rtspReaderVideoTrackProperties};

Boolean rtspReaderCanHandle(const char *mimeType)
{
	if (0 == FskStrCompareCaseInsensitive("application/sdp", mimeType))
		return true;

	if (0 == FskStrCompareCaseInsensitive("application/x-rtsp", mimeType))
		return true;

	return false;
}

typedef struct rtspReaderRecord rtspReaderRecord;
typedef struct rtspReaderRecord *rtspReader;

typedef struct rtspReaderTrackRecord rtspReaderTrackRecord;
typedef struct rtspReaderTrackRecord *rtspReaderTrack;

typedef struct RTPPacketQueueRecord RTPPacketQueueRecord;
typedef struct RTPPacketQueueRecord *RTPPacketQueue;

typedef struct RTPPacketListRecord RTPPacketListRecord;
typedef struct RTPPacketListRecord RTPPacketList;

struct RTPPacketQueueRecord {
	RTPPacket			head;
	RTPPacket			tail;

	RTPPacketParser		packetParser;
};

struct RTPPacketListRecord {
	FskList head;
	
	UInt32	duration;
};

struct rtspReaderTrackRecord {
	rtspReaderTrackRecord				*next;

	rtspReader							state;
	FskMediaReaderTrackRecord			readerTrack;
	char								*format;
	Boolean								disabled;

	RTSPMediaStream						stream;
	RTPPacketParser						packetParserRTP;
	RTCPPacketParser					packetParserRTCP;
	RTPPacketQueueRecord				packetQueue;

	UInt32								duration;
	UInt32								scale;
	UInt32								bitrate;
	UInt32								zeroTime;

	UInt32								nextSeq;
	Boolean								needsSeqCheck;
	Boolean								allMediaReceived;
	FskTimeCallBack						packetReceiveTimer;

	union {
		struct {
			UInt32						sampleRate;
			UInt32						channelCount;
			UInt32						bytesPerSample;
			UInt32						samplesPerDecode;
			Boolean						allAudioQueued;
			Boolean						needsQueueSilenceCheck;
			Boolean						needsTimestampCheck;
			Boolean						badTimeStamps;
			Boolean						disableSilence;
			Boolean						queueSilence;
		} audio;

		struct {
			FskDimensionRecord			dimensions;
			Boolean						needsTimestampCheck;
			UInt32						decodeIncrementalTimeStep;
			UInt32						decodeIncrementalTimeStepBump;
			UInt32						nextDecodeTime;
		} video;
	};
};

struct rtspReaderRecord {
	FskMediaReader				reader;
	rtspReaderTrack				tracks;

	FskMediaSpooler				spooler;
	Boolean						spoolerOpen;
	FskInt64					spoolerPosition;

	Boolean						hasAudio;
	rtspReaderTrack				audioTrack;
	UInt32						audioTime;			// in audio time scale
	Boolean						audioConfigured;
	Boolean						audioDone;

	Boolean						hasVideo;
	rtspReaderTrack				videoTrack;
	UInt32						videoTime;			// in video time scale
	Boolean						videoConfigured;
	Boolean						videoDone;

	UInt32						duration;
	UInt32						scale;
	UInt32						startTime;
	UInt32						endTime;
	Boolean						hasEndTime;
	Boolean						scrub;

	DIDLMusicItemRecord			mi;

	RTSPSession					session;
	char						*uri;
	UInt32						proxyAddr;
	UInt32						proxyPort;
	char						*redirect;

	Boolean						sdpPreDelivered;
	char						*sdp;
	UInt32						sdpSize;

	Boolean						disablePause;
	Boolean						useTCP;
	Boolean						needSessionStart;
	Boolean						needToPlay;
	Boolean						stopping;
	Boolean						starting;
	Boolean						disposing;
	Boolean						dontPause;

	FskTimeCallBack				packetWatchdogTimer;
	FskTimeCallBack				optionsTimer;

	UInt32						msToBuffer;
	FskErr						theError;
};

static FskErr initSession(rtspReader reader);
static FskErr disposeSession(rtspReader state);
static FskErr createAudioTrack(rtspReader state, RTSPMediaStream stream);
static FskErr createVideoTrack(rtspReader state, RTSPMediaStream stream);
static void enqueuePacket(rtspReader state, RTPPacketQueue queue, RTPPacket packet);
static void emptyQueuePacket(RTPPacketQueue queue, RTPPacket packet);
static FskErr responseHeaderCB(char *method, FskHeaders *headers, void *refCon);
static FskErr stateChangeCB(SInt16 newState, void *refCon);
static double getPresentationRange(RTSPSession session);
// static Boolean allConfigured(rtspReader state);
static void setMediaTitle(rtspReader state);
static void flushAll(rtspReader state);
static FskErr doPlay(rtspReader state);
static void doReaderSetState(rtspReader state, SInt32 newState);
static FskErr rtspInstantiate(rtspReader state);
static void reportIfError(rtspReader state, FskErr err);
static FskErr rtspInitializeFromSDP(rtspReader state);

static FskErr aacInitialize(rtspReaderTrack track);
static FskErr amrInitialize(rtspReaderTrack track);
static FskErr qcelpInitialize(rtspReaderTrack track);

static FskErr configureAudio(rtspReader state, rtspReaderTrack track);
static FskErr audioPacketCB(RTPPacket packet, void *refCon);
static FskErr audioRTCPPacketCB(RTCPPacket packet, void *refCon);
static FskErr enqueueAudioPacket(rtspReaderTrack track, RTPPacket packet);
static Boolean checkForAACPlusTimeStamps(rtspReaderTrack track);
static void fixAACPlusTimeStamp(rtspReaderTrack track, RTPPacket packet);
static void audioFlush(rtspReaderTrack track);
// static FskErr queueSilence(rtspReaderTrack track, UInt32 samplesInBuffer, UInt32 samplesToQueue, UInt32 *bufferSize, char **buffer);

static FskErr configureVideo(rtspReader state, rtspReaderTrack track);
static FskErr videoPacketCB(RTPPacket packet, void *refCon);
static FskErr videoRTCPPacketCB(RTCPPacket packet, void *refCon);
static FskErr enqueueVideoPacket(rtspReaderTrack track, RTPPacket packet);
static Boolean calculateVideoDecodeTimeStep(rtspReaderTrack track);

static FskErr rtspReaderAudioExtract(rtspReader state, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr rtspReaderVideoExtract(rtspReader state, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);

static void optionsTimerCB(FskTimeCallBack callback, const FskTime time, void *param);
static void packetWatchdogTimerCB(FskTimeCallBack callback, const FskTime time, void *param);
static void packetReceiveTimerCB(FskTimeCallBack callback, const FskTime time, void *param);

static FskErr rtspSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);

FskErr rtspReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	rtspReader state = NULL;

	err = FskMemPtrNewClear(sizeof(rtspReaderRecord), &state);
	BAIL_IF_ERR(err);

	state->reader = reader;

	state->uri = FskStrDoCopy(uri);

	// Use the spooler to read the SDP delivered over http or via local file
	if (0 == FskStrCompare(mimeType, "application/x-rtsp") || 0 == FskStrCompareWithLength(uri, "file://", 7)) {
        BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
		state->sdpPreDelivered = true;
		if (spooler->doOpen) {
			err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
			BAIL_IF_ERR(err);
			state->spoolerOpen = true;
		}
		state->spooler = spooler;
		state->spooler->onSpoolerCallback = rtspSpoolerCallback;
		state->spooler->clientRefCon = state;
	}

	err = initSession(state);
	BAIL_IF_ERR(err);

	FskMediaMetaDataNew(&state->mi.meta);

	doReaderSetState(state, kFskMediaPlayerStateInstantiating);

	// Kick off the session
	if (false == state->sdpPreDelivered) {
		err = RTSPSessionSetURL(state->session, state->uri);
		BAIL_IF_ERR(err);

		err = RTSPSessionConnectToHost(state->session);
		BAIL_IF_ERR(err);
	}
	else {
		err = rtspSpoolerCallback(state, kFskMediaSpoolerOperationDataReady, NULL);		// kicks off session for local files
		BAIL_IF_ERR(err);
	}

bail:
	if (kFskErrNone != err) {
		FskRTSPReaderPrintfNormal("Disposing reader in new!");
		rtspReaderDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr rtspReaderDispose(FskMediaReader reader, void *readerState)
{
	rtspReader state = readerState;
	rtspReaderTrack track;

	if (NULL == state)
		return kFskErrNone;

	state->disposing = true;

	FskRTSPReaderPrintfNormal("Disposing reader");

	if (state->spoolerOpen && state->spooler->doClose)
		(state->spooler->doClose)(state->spooler);
	FskMemPtrDispose(state->sdp);

	// Dispose the tracks before the session, so that we can clear the callbacks as early as possible
	track = state->tracks;
	while (NULL != track) {
		rtspReaderTrack next = track->next;

		RTCPPacketParserSetAppReceivePacketCallback(track->packetParserRTCP, NULL);
		RTPPacketParserSetAppReceivePacketCallback(track->packetParserRTP, NULL);
		FskMemPtrDispose(track->format);
		FskTimeCallbackDispose(track->packetReceiveTimer);
		FskMemPtrDispose(track);

		track = next;
	}

	disposeSession(state);
	FskMemPtrDispose(state->uri);
	FskMemPtrDispose(state->redirect);
	FskTimeCallbackDispose(state->optionsTimer);
	FskTimeCallbackDispose(state->packetWatchdogTimer);

	FskMediaMetaDataDispose(state->mi.meta);

	FskRTSPReaderPrintfNormal("reader disposed");

	FskMemPtrDispose(state);

	return kFskErrNone;
}

FskErr rtspReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *trackOut)
{
	rtspReader state = readerState;
	rtspReaderTrack track = state->tracks;

	while ((NULL != track) && (track->disabled || 0 != index)) {
		index -= 1;
		track = track->next;
	}

	if (NULL == track)
		return kFskErrNotFound;

	*trackOut = &track->readerTrack;

	return kFskErrNone;
}

FskErr rtspReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	rtspReader state = readerState;
	FskErr err = kFskErrNone;
	double duration = state->duration;
	double atTime;

	if ((false == state->hasAudio) && (false == state->hasVideo))
		return kFskErrOperationFailed;

	atTime = startTime ? *startTime : 0;
	
	FskRTSPReaderPrintfNormal("In start, start requested at %f", atTime);

	if (atTime >= duration)
		return kFskErrInvalidParameter;

	if (state->hasAudio)
		state->audioTime = (UInt32)(atTime * state->audioTrack->scale / state->scale);
	if (state->hasVideo)
		state->videoTime = (UInt32)(atTime * state->videoTrack->scale / state->scale);

	state->startTime = (UInt32)atTime;
	state->endTime = (UInt32)duration;
	if (endTime && *endTime < duration)
		state->endTime = (UInt32)*endTime;

	state->audioDone = !state->hasAudio;
	state->videoDone = !state->hasVideo;

	// Reset our audio and video times to zero for live streams
	if (kFskUInt32Max == state->duration) {
		state->audioTime = state->videoTime = 0;
	}
	
	// Force a recalculation of the starting decode time stamp
	if (NULL != state->videoTrack && 0 != state->videoTrack->video.decodeIncrementalTimeStep)
		state->videoTrack->video.needsTimestampCheck = true;

	if (state->session && state->session->interfaceLost) {
		rtspReaderTrack track;

		disposeSession(state);
		state->session = NULL;

		track = state->tracks;
		while (NULL != track) {
			rtspReaderTrack next = track->next;
			track->packetParserRTP = NULL;
			track->packetParserRTCP = NULL;
			track->stream = NULL;
			track = next;
		}
		state->needSessionStart = true;
	}

	// The session may have already been started in the reader new() call.
	// Check the session state and set a flag if we need to issue a PLAY command
	if (state->needSessionStart) {
		state->needSessionStart = false;

		err = initSession(state);
		BAIL_IF_ERR(err);

		if (NULL != state->audioTrack)
			state->audioTrack->packetQueue.packetParser = NULL;
		if (NULL != state->videoTrack)
			state->videoTrack->packetQueue.packetParser = NULL;

		if (state->sdpPreDelivered) {
			err = rtspInitializeFromSDP(state);
            BAIL_IF_ERR(err);
		}
		else {
			err = RTSPSessionSetURL(state->session, state->uri);
			BAIL_IF_ERR(err);

			err = RTSPSessionConnectToHost(state->session);
			BAIL_IF_ERR(err);

			state->needToPlay = true;
		}
	}
	else
	if (!state->starting && (NULL != state->session) && (state->session->sessionState < kRTSPClientStatePlaying)) {
		FskRTSPReaderPrintfNormal("PLAY");
		err = doPlay(state);
		BAIL_IF_ERR(err);
	}

bail:
	FskRTSPReaderPrintfNormal("Out start");
	return err;
}

FskErr rtspReaderStop(FskMediaReader reader, void *readerState)
{
	FskErr err = kFskErrNone;
	rtspReader state = readerState;

	FskRTSPReaderPrintfNormal("In stop");

	FskTimeCallbackDispose(state->packetWatchdogTimer);
	state->packetWatchdogTimer = NULL;
	FskTimeCallbackDispose(state->optionsTimer);
	state->optionsTimer = NULL;

	if (state->stopping || state->starting || state->dontPause) {
#if SUPPORT_INSTRUMENTATION
		if (state->stopping)
			FskRTSPReaderPrintfNormal("STOP request denied - currently stopping");
		if (state->starting)
			FskRTSPReaderPrintfNormal("STOP request denied - currently starting");
		if (state->dontPause)
			FskRTSPReaderPrintfNormal("STOP request denied - dontPause is set true");
#endif
		goto bail;
	}

	flushAll(state);

	if (!state->disablePause) {
		FskRTSPReaderPrintfNormal("PAUSE");

		err = RTSPSessionPause(state->session);
		BAIL_IF_ERR(err);

		state->stopping = true;
	}
	else {
		rtspReaderTrack track;

		disposeSession(state);
		state->session = NULL;

		track = state->tracks;
		while (NULL != track) {
			rtspReaderTrack next = track->next;
			track->packetParserRTP = NULL;
			track->packetParserRTCP = NULL;
			track->stream = NULL;
			track = next;
		}
		state->needSessionStart = true;
	}

bail:
	FskRTSPReaderPrintfNormal("Out stop");
	return err;
}

FskErr rtspReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data)
{
	rtspReader state = readerState;

	if (state->audioDone && state->videoDone)
		return kFskErrEndOfFile;
		
	if (state->hasAudio && state->hasVideo) {
		UInt32 audioTime, videoTime;

		audioTime = (UInt32)((double)state->audioTime * state->scale / state->audioTrack->scale);
		videoTime = (UInt32)((double)state->videoTime * state->scale / state->videoTrack->scale);

		if ((audioTime <= videoTime) && !state->audioDone) {
			return rtspReaderAudioExtract(state, track, infoCount, info, data);
		}

		if (!state->videoDone)
			return rtspReaderVideoExtract(state, track, infoCount, info, data);

		return kFskErrEndOfFile;
	}

	if (state->hasAudio) {
		return rtspReaderAudioExtract(state, track, infoCount, info, data);
	}

	return rtspReaderVideoExtract(state, track, infoCount, info, data);
}

static FskErr rtspReaderAudioExtract(rtspReader state, FskMediaReaderTrack *trackOut, UInt32 *infoCount, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
#define kDataChunkSize 8192
#define kInfoChunkCount 25
	FskErr err = 0;
	rtspReaderTrack track = state->audioTrack;
	RTPPacketQueue queue = &track->packetQueue;
	FskMediaReaderSampleInfo info = NULL;
	UInt32 nInfo, nInfoAllocated, dataSizeAllocated, dataOffset;
	unsigned char *data = NULL;
	UInt32 targetTime;
	RTPPacket packet;
	FskInt64 decodeTime;

	if (kFskUInt32Max != state->duration && (state->audioTime >= track->duration || (track->allMediaReceived && (NULL == queue->head)))) {
		state->audioDone = true;
        BAIL(kFskErrEndOfFile);
	}
	
    BAIL_IF_NULL(queue->head, err, kFskErrNeedMoreTime);

    BAIL_IF_TRUE(track->audio.needsTimestampCheck, err, kFskErrNeedMoreTime);

	// Check for gaps once per extract
	if (0 != state->audioTime) {
		packet = queue->head;
		decodeTime = (FskInt64)((double)packet->presentationTime * state->scale / track->scale);
		if (state->audioTime != decodeTime && packet->presentationTime > state->audioTime) {
			err = FskMemPtrNew(sizeof(FskMediaReaderSampleInfoRecord), &info);
			BAIL_IF_ERR(err);
			FskMemSet(info, 0, sizeof(FskMediaReaderSampleInfoRecord));
			nInfo = 1;
			info->samples = 1;
			info->flags = kFskImageFrameGap;
			info->decodeTime = state->audioTime;
			info->sampleDuration = (UInt32)(decodeTime - state->audioTime);
			state->audioTime = (UInt32)decodeTime;

			FskRTSPReaderPrintfNormal("Inserted %ld silent samples", info->sampleDuration);
			goto done;
		}
	}

	// Target one second in the future
	targetTime = state->audioTime + track->audio.sampleRate;

	dataOffset = 0;
	dataSizeAllocated = 0;
	nInfoAllocated = 0;
	nInfo = 0;

	while (queue->head && (state->audioTime < targetTime)) {
		packet = queue->head;

		if (packet->presentationTime < state->audioTime) {
			// Something is amiss... just throw away this packet.
			// Sometimes a server will send multiple packets with
			// the same timestamp, which throws our assumptions off.
			// This should catch the "extra" packets and throw the
			// data contained within away.  At least this way we can
			// get back on track.
			FskRTSPReaderPrintfNormal("Tossing early audio packet %ld presentation time %ld audio time %ld", packet->sequenceNumber, packet->presentationTime, state->audioTime);
			emptyQueuePacket(queue, packet);
			continue;
		}

#if SUPPORT_INSTRUMENTATION
		if (0 != track->nextSeq && track->nextSeq != packet->sequenceNumber)
			FskRTSPReaderPrintfNormal("Packet sequence error - expect %ld got %ld", track->nextSeq, packet->sequenceNumber);
#endif

		if (-1 != packet->totalSamples && NULL != packet->frames) {
			RTPCompressedMediaFrame frame = packet->frames;
			decodeTime = (FskInt64)((double)packet->presentationTime * state->scale / track->scale);

			// Stop here if there's a gap.  We'll insert silence in the next call
			if (0 != state->audioTime && state->audioTime != decodeTime) {
				break;
			}

			while (NULL != frame) {
				UInt8 *src = (UInt8*)(frame + 1);
				UInt32 *sampleSizeP = &frame->sampleSizes[0];
				FskMediaReaderSampleInfo infoPtr;

				while (*sampleSizeP) {
					if (*sampleSizeP + dataOffset >= dataSizeAllocated) {
						dataSizeAllocated += kDataChunkSize;
						err = FskMemPtrRealloc(dataSizeAllocated, (FskMemPtr*)&data);
						BAIL_IF_ERR(err);
					}
					FskMemMove(&data[dataOffset], src, *sampleSizeP);
					dataOffset += *sampleSizeP;
					src += *sampleSizeP;

					if (nInfo >= nInfoAllocated) {
						nInfoAllocated += kInfoChunkCount;
						err = FskMemPtrRealloc(nInfoAllocated * sizeof(FskMediaReaderSampleInfoRecord), &info);
						BAIL_IF_ERR(err);
					}
					++nInfo;
					infoPtr = &info[nInfo - 1];
					FskMemSet(infoPtr, 0, sizeof(FskMediaReaderSampleInfoRecord));
					infoPtr->samples = 1;
					infoPtr->sampleSize = *sampleSizeP;
					infoPtr->flags = kFskImageFrameTypeSync;
					infoPtr->decodeTime = decodeTime;
					infoPtr->sampleDuration = track->audio.samplesPerDecode;
					infoPtr->compositionTime = -1;

					FskRTSPReaderPrintfNormal("Extracted audio at time %ld", infoPtr->decodeTime);
					
					decodeTime += infoPtr->sampleDuration;

					++sampleSizeP;
				}
				
				frame = frame->next;
			}
			state->audioTime = (UInt32)decodeTime;
		}
					
		FskRTSPReaderPrintfNormal("Extracted audio packet number %ld seq %ld time %ld", packet->packetNumber, packet->sequenceNumber, packet->presentationTime);

		track->nextSeq = packet->sequenceNumber + 1;
		emptyQueuePacket(queue, packet);
	}

    BAIL_IF_ZERO(nInfo, err, kFskErrNeedMoreTime);

	if ((kFskUInt32Max != track->duration) && !queue->tail && !track->packetReceiveTimer) {
		FskRTSPReaderPrintfNormal("Audio extract packet delta is %ld", track->duration - state->audioTime);
		if (track->duration - state->audioTime < track->scale) {
			//fprintf(stderr, "Enable audio packet receive timer\n");
			FskTimeCallbackNew(&track->packetReceiveTimer);
			FskTimeCallbackScheduleFuture(track->packetReceiveTimer, 0, 750, packetReceiveTimerCB, track);
		}
	}

	err = FskMemPtrRealloc(dataOffset + kFskDecompressorSlop, &data);	// trim down the data to the actual size
	BAIL_IF_ERR(err);
	
done:
	*trackOut = &state->audioTrack->readerTrack;
	*infoCount = nInfo;
	*infoOut = info;
	*dataOut = data;

bail:
	if (0 != err) {
		FskMemPtrDispose(info);
		FskMemPtrDispose(data);
		*infoOut = NULL;
		*dataOut = NULL;
		*infoCount = 0;
	}

	return err;
}

static FskErr rtspReaderVideoExtract(rtspReader state, FskMediaReaderTrack *trackOut, UInt32 *infoCount, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	FskErr err = 0;
	FskMediaReaderSampleInfo infoPtr, info = NULL;
	rtspReaderTrack track = state->videoTrack;
	RTPPacketQueue queue = &track->packetQueue;
	RTPCompressedMediaFrame frame;
	UInt32 nInfo = 0;
	UInt32 dataSize = 0;
	UInt8 *data = NULL;
	RTPPacket packet;

	if (kFskUInt32Max != state->duration && (state->videoTime >= track->duration || (track->allMediaReceived && (NULL == queue->head)))) {
		state->videoDone = true;
        BAIL(kFskErrEndOfFile);
	}

	packet = queue->head;

	// Make sure the first packet has compressed frames
	while (NULL != packet && NULL == packet->frames) {
		RTPPacket next = packet->next;
		emptyQueuePacket(queue, packet);
		packet = next;
	}

	if (NULL == packet)
		return kFskErrNeedMoreTime;

    BAIL_IF_TRUE(track->video.needsTimestampCheck, err, kFskErrNeedMoreTime);
	
	// Allocate enough sample records to contain the linked frames
	frame = packet->frames;
	while (NULL != frame) {
		++nInfo;
		frame = frame->next;
	}
	err = FskMemPtrNewClear(nInfo * sizeof(FskMediaReaderSampleInfoRecord), &info);
	BAIL_IF_ERR(err);

	frame = packet->frames;
	infoPtr = info;
	nInfo = 0;
	while (frame != NULL) {
		err = FskMemPtrRealloc(dataSize + frame->length + kFskDecompressorSlop, &data);
		BAIL_IF_ERR(err);

		++nInfo;
		FskMemMove(data + dataSize, (UInt8*)(frame + 1), frame->length);
		dataSize += frame->length;

		infoPtr->samples = 1;
		infoPtr->flags = frame->key ? kFskImageFrameTypeSync : (frame->droppable ? kFskImageFrameTypeDroppable : kFskImageFrameTypeDifference);

		infoPtr->sampleSize = frame->length;
		infoPtr->sampleDuration = 0;
		
		if (0 == track->video.decodeIncrementalTimeStep) {
			infoPtr->decodeTime = (FskInt64)((double)packet->presentationTime * state->scale / track->scale);
			infoPtr->compositionTime = -1;
			state->videoTime = packet->presentationTime;
		}
		else {
			infoPtr->compositionTime = (FskInt64)((double)frame->compositionTimeStamp * state->scale / track->scale);
			infoPtr->decodeTime = (FskInt64)((double)frame->decodeTimeStamp * state->scale / track->scale);
			state->videoTime = frame->decodeTimeStamp;
		}
		
#if SUPPORT_INSTRUMENTATION
		if (kFskImageFrameTypeSync == infoPtr->flags) {
			FskRTSPReaderPrintfNormal("Extracted KEY FRAME video at time %ld", infoPtr->decodeTime);
		}
		else {
			FskRTSPReaderPrintfNormal("Extracted video at time %ld", infoPtr->decodeTime);
		}
#endif

		++infoPtr;
		frame = frame->next;
	}
		
	FskRTSPReaderPrintfNormal("Extracted video packet number %ld seq %ld time %ld", packet->packetNumber, packet->sequenceNumber, packet->presentationTime);

	track->nextSeq = packet->sequenceNumber + 1;
	emptyQueuePacket(queue, packet);

	if ((kFskUInt32Max != track->duration) && !queue->tail && !track->packetReceiveTimer) {
		FskRTSPReaderPrintfNormal("Video extract packet delta is %ld", track->duration - state->videoTime);
		if (track->duration - state->videoTime < track->scale) {
			//fprintf(stderr, "Enable video packet receive timer\n");
			FskTimeCallbackNew(&track->packetReceiveTimer);
			FskTimeCallbackScheduleFuture(track->packetReceiveTimer, 0, 750, packetReceiveTimerCB, track);
		}
	}

	*trackOut = &state->videoTrack->readerTrack;
	*infoCount = nInfo;
	*infoOut = info;
	*dataOut = data;

bail:
	if (0 != err) {
		FskMemPtrDispose(data);
		FskMemPtrDispose(info);
		*infoOut = NULL;
		*dataOut = NULL;
		*infoCount = 0;
	}

	return err;
}

FskErr rtspReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	rtspReader state = readerState;

	if (NULL == state->session)
		return kFskErrNeedMoreTime;

	return FskMediaMetaDataGetForMediaPlayer(state->mi.meta, metaDataType, index, value, flags);
}

FskErr rtspReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	*mime = NULL;

	if (0 == FskStrCompareWithLength(uri, "rtsp://", 7)) {
		*mime = FskStrDoCopy("application/sdp");
		return kFskErrNone;
	}

#define kSniffSize 16
	if (dataSize >= kSniffSize) {
		unsigned char buffer[kSniffSize + 1];

		FskMemMove(buffer, data, kSniffSize);
		buffer[kSniffSize] = 0;
		if ((NULL != FskStrStr((char *)buffer, "v=0")) && (NULL != FskStrStr((char *)buffer, "o="))) {
			*mime = FskStrDoCopy("application/x-rtsp");
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr rtspReaderGetError(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->theError;

	return kFskErrNone;
}

FskErr rtspReaderGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (kFskUInt32Max != state->duration) ? state->duration : -1.0;

	return kFskErrNone;
}

FskErr rtspReaderGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;
	
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = 0;
	if (state->audioTrack)
		property->value.number = (double)state->audioTime * state->scale / state->audioTrack->scale;
	if (state->videoTrack) {
		double t =  (double)state->videoTime * state->scale / state->videoTrack->scale;
		if ((state->audioTrack && (t < property->value.number)) || !state->hasAudio)
			property->value.number = t;
	}

	return kFskErrNone;
}

FskErr rtspReaderGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

static FskErr rtspReaderGetBufferDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = (UInt32)((double)state->msToBuffer / 1000 * state->scale);

	return kFskErrNone;
}

FskErr rtspReaderSetScrub(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	state->scrub = property->value.b;
	
#if SUPPORT_INSTRUMENTATION
	if (state->scrub) {
		FskRTSPReaderPrintfNormal("Scrub ON");
	}
	else {
		FskRTSPReaderPrintfNormal("Scrub OFF");
	}
#endif

	return kFskErrNone;
}

FskErr rtspReaderGetRedirect(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	if (!state->redirect)
		return kFskErrUnimplemented;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(state->redirect);

	return kFskErrNone;
}

FskErr rtspReaderGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReader state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr rtspReaderTrackGetMediaType(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;
	rtspReader state = track->state;

	property->type = kFskMediaPropertyTypeString;
	if (track == state->audioTrack) {
		property->value.str = FskStrDoCopy("audio");
	}
	else
	if (track == state->videoTrack) {
		property->value.str = FskStrDoCopy("video");
	}
	else {
		property->value.str = FskStrDoCopy("unknown");
	}

	return kFskErrNone;
}

FskErr rtspReaderTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err = kFskErrUnimplemented;
	rtspReaderTrack track = readerState;

	if (0 != track->bitrate) {
		property->type = kFskMediaPropertyTypeInteger;
		property->value.integer = track->bitrate;
		err = kFskErrNone;
	}

	return err;
}

static FskErr rtspReaderTrackGetFormat(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);

	return kFskErrNone;
}

/*
	audio track
*/


FskErr rtspReaderAudioTrackGetSampleRate(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.sampleRate;

	return kFskErrNone;
}

FskErr rtspReaderAudioTrackGetChannelCount(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channelCount;

	return kFskErrNone;
}

static FskErr rtspReaderAudioTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;
	FskErr err = kFskErrUnimplemented;
	QTSoundDescription desc = NULL;
	UInt8 *esds, *esdsPtr;
	UInt32 esdsSize, descSize = 0;

	// No format info for AMR
	if (kRTPAudioFormatAMR == track->packetParserRTP->mediaFormat)
		goto bail;

	err = RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorESDS, &esds, &esdsSize);
	BAIL_IF_ERR(err);

	// version 0 sound description - no samplesPerPacket, bytesPerPacket, bytesPerFrame, bytesPerSample
	descSize = sizeof(QTSoundDescriptionRecord) - (sizeof(UInt32) * 4) + 8 + esdsSize;
	err = FskMemPtrNewClear(descSize, &desc);
	BAIL_IF_ERR(err);

	desc->descSize = descSize;
	if (kRTPAudioFormatAAC == track->packetParserRTP->mediaFormat)
		desc->dataFormat = 'mp4a';
	else
	if (kRTPAudioFormatQCELP == track->packetParserRTP->mediaFormat)
		desc->dataFormat = 'Qclp';
	desc->sampleSize = 16;
	desc->dataRefIndex = 1;
	desc->numChannels = (UInt16)track->audio.channelCount;
	desc->sampleRate = track->audio.sampleRate << 16;
	desc->compressionID = -1;		//@@ check

	esdsPtr = (UInt8*)desc + sizeof(QTSoundDescriptionRecord) - (sizeof(UInt32) * 4);
	WriteLongN( esdsSize + 8, esdsPtr );	// write the four byte size
	esdsPtr += 4;
	WriteLongN( 'esds', esdsPtr );			// write the 4cc
	esdsPtr += 4;
	FskMemMove(esdsPtr, esds, esdsSize);	// write the decoder specific info

bail:
	if (0 != err) {
		FskMemPtrDisposeAt(&desc);
		descSize = 0;
	}

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = descSize;
	property->value.data.data = desc;

	return err;
}

/*
	video track
*/

FskErr rtspReaderVideoTrackGetTimeScale(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->state->scale;

	return kFskErrNone;
}

FskErr rtspReaderVideoTrackGetDimensions(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width = track->video.dimensions.width;
	property->value.dimension.height = track->video.dimensions.height;

	return kFskErrNone;
}

FskErr rtspReaderVideoTrackGetFormatInfo(void *trackState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	rtspReaderTrack track = trackState;
	FskErr err = kFskErrUnimplemented;

	// Return esds for MPEG-4 and AVC video
	if (kRTPVideoFormatMPEG4 == track->packetParserRTP->mediaFormat || kRTPVideoFormatAVC == track->packetParserRTP->mediaFormat) {
		char *esds, *esdsPtr;
		UInt32 esdsSize, descSize;
		QTImageDescription idp, desc;

		err = RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorESDS, &esds, &esdsSize);
		BAIL_IF_ERR(err);

		descSize = sizeof(QTImageDescriptionRecord) + 8 + esdsSize;
		err = FskMemPtrNewClear( descSize, &desc);
		BAIL_IF_ERR(err);
	
		idp = desc;
		esdsPtr = (char *)( idp + 1 );

		idp->idSize			= FskEndianU32_NtoN(descSize);
		idp->cType			= kRTPVideoFormatMPEG4 == track->packetParserRTP->mediaFormat ? FskEndianU32_NtoN('mp4v') : FskEndianU32_NtoN('avc1');
		idp->version		= FskEndianU16_NtoN(0);							// version of codec data format
		idp->revisionLevel	= FskEndianU16_NtoN(0);						// revision of codec data format
		idp->vendor			= FskEndianU32_NtoN('appl');			// Apple
		idp->spatialQuality = FskEndianU32_NtoN(512);	// we could be clever, but nobody would care
		idp->temporalQuality = FskEndianU32_NtoN(512);	// we could be clever, but nobody would care
		if (kRTPVideoFormatMPEG4 == track->packetParserRTP->mediaFormat) {
			//idp->width			= FskEndianU16_NtoN(320);
			//idp->height			= FskEndianU16_NtoN(240);
			idp->width			= FskEndianU16_NtoN((UInt16)track->video.dimensions.width);
			idp->height			= FskEndianU16_NtoN((UInt16)track->video.dimensions.height);
		}
		else {
			idp->width			= FskEndianU16_NtoN((UInt16)track->video.dimensions.width);
			idp->height			= FskEndianU16_NtoN((UInt16)track->video.dimensions.height);
		}
		idp->hRes			= FskEndianU32_NtoN(72<<16);							// dots-per-inch 
		idp->vRes			= FskEndianU32_NtoN(72<<16);							// dots-per-inch 
		idp->frameCount		= FskEndianU16_NtoN(1);						// one frame at a time
		if (kRTPVideoFormatMPEG4 == track->packetParserRTP->mediaFormat) {
			idp->name[0]		= 4;
			idp->name[1]		= 'M';
			idp->name[2]		= 'P';
			idp->name[3]		= '4';
			idp->name[4]		= 'V';
		}
		else {
			idp->name[0]		= 5;
			idp->name[1]		= 'H';
			idp->name[2]		= '.';
			idp->name[3]		= '2';
			idp->name[4]		= '6';
			idp->name[5]		= '4';
		}
		idp->depth			= FskEndianU16_NtoN(24);							// color.
		idp->clutID			= FskEndianU16_NtoN(-1);							// not using a clut
	
		WriteLongN( esdsSize + 8, esdsPtr );	// write the four byte size
		esdsPtr += 4;
	
		if (kRTPVideoFormatMPEG4 == track->packetParserRTP->mediaFormat) {	// write the 4cc
			WriteLongN( 'esds', esdsPtr );
		}
		else {
			WriteLongN( 'avcC', esdsPtr );
		}
		esdsPtr += 4;

		FskMemMove(esdsPtr, esds, esdsSize);	// write the decoder specific info

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = descSize;
		property->value.data.data = desc;
	}

bail:
	return err;
}

/*
	audio
*/

FskErr createAudioTrack(rtspReader state, RTSPMediaStream stream)
{
	FskErr err = 0;
	rtspReaderTrack track;

	if (NULL == state->audioTrack) {
		err = FskMemPtrNewClear(sizeof(rtspReaderTrackRecord), &track);
		BAIL_IF_ERR(err);
		state->audioTrack = track;
		FskListAppend((void **)&state->tracks, track);
	}
	track = state->audioTrack;

	stream->refCon = (UInt32)track;
	track->stream = stream;

	track->readerTrack.dispatch = &gMediaReaderRTSPAudioTrack;
	track->readerTrack.state = track;

	state->hasAudio = true;
	track->state = state;
	track->zeroTime = kFskUInt32Max;
	track->needsSeqCheck = true;
	
	// Install our RTP/RTCP packet callbacks
	track->packetParserRTCP = stream->packetParserRTCP;
	RTCPPacketParserSetAppReceivePacketCallback(track->packetParserRTCP, audioRTCPPacketCB);
	RTCPPacketParserSetAppRefCon(track->packetParserRTCP, track);
	
	track->packetParserRTP = stream->packetParser;
	track->packetQueue.packetParser = track->packetParserRTP;
	RTPPacketParserSetAppReceivePacketCallback(track->packetParserRTP, audioPacketCB);
	RTPPacketParserSetAppRefCon(track->packetParserRTP, track);

bail:
	return err;
}

FskErr audioRTCPPacketCB(RTCPPacket packet, void *refCon)
{
	rtspReaderTrack track = (rtspReaderTrack)refCon;
	rtspReader state = track->state;
	RTSPSession session = state->session;
	FskErr err = 0;
	UInt16 i;
	
	FskMediaReaderUsing(state->reader, true);

	if (kRTCPPacketTypeSenderReport == packet->header.PT) {
		for (i = 0; i < session->nMediaStreams; ++i) {
			if (session->mediaStream[i].ssrc == packet->flavor.sr.SSRC) {
				session->mediaStream[i].sendReceiverReport = true;
				break;
			}
		}
	}

	else if (kRTCPPacketTypeBYE == packet->header.PT) {
		for (i = 0; i < session->nMediaStreams; ++i) {
			if (session->mediaStream[i].ssrc == packet->flavor.sr.SSRC) {
				track->allMediaReceived = true;
				FskRTSPReaderPrintfNormal("Got audio BYE");
				break;
			}
		}
	}

	else if (kRTCPPacketTypeSDES == packet->header.PT) {
		for (i = 0; i < session->nMediaStreams; ++i) {
			if (session->mediaStream[i].ssrc == packet->flavor.sd.SSRC) {
			
				// Handle Opticodec in-stream metadata
				if (0 != *packet->flavor.sd.NOTE) {
					char *p, *q;
					UInt32 len;
					Boolean gotMeta = false;
					FskMediaPropertyValueRecord prop;

					p = FskStrStr(packet->flavor.sd.NOTE, "Artist=");
					if (NULL != p) {
						p += 7;
						q = FskStrStr(p, ";");
						if (q != NULL) {
							len = (UInt32)(q - p);

							while (true) {
								if (kFskErrNone != FskMediaMetaDataRemove(state->mi.meta, "Artist", 0))
									break;
							}
							gotMeta = true;
							prop.type = kFskMediaPropertyTypeString;
							prop.value.str = (char*)FskMemPtrCalloc(len + 1);
							FskStrNCopy(prop.value.str, p, len);
							FskMediaMetaDataAdd(state->mi.meta, "Artist", NULL, &prop, kFskMediaMetaDataFlagOwnIt);
						}
					}
					p = FskStrStr(packet->flavor.sd.NOTE, "Title=");
					if (NULL != p) {
						p += 6;
						q = FskStrStr(p, ";");
						if (q != NULL) {
							len = (UInt32)(q - p);

							while (true) {
								if (kFskErrNone != FskMediaMetaDataRemove(state->mi.meta, "FullName", 0))
									break;
							}
							gotMeta = true;
							prop.type = kFskMediaPropertyTypeString;
							prop.value.str = (char*)FskMemPtrCalloc(len + 1);
							FskStrNCopy(prop.value.str, p, len);
							FskMediaMetaDataAdd(state->mi.meta, "FullName", NULL, &prop, kFskMediaMetaDataFlagOwnIt);
						}
					}
					if (gotMeta)
						FskMediaReaderSendEvent(state->reader, kFskEventMediaPlayerMetaDataChanged);
				}
				break;
			}
		}
	}

	RTCPPacketDispose(track->packetParserRTCP, packet);

	FskMediaReaderUsing(state->reader, false);

	return err;
}

FskErr configureAudio(rtspReader state, rtspReaderTrack track)
{
	UInt32 audioChannels = 0, audioSampleRate = 0, mediaFormat = kRTPMediaFormatUnknown;
	SDPAttribute attr;
	FskErr err;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorMediaFormat, &mediaFormat, NULL);
	if (kRTPMediaFormatUnknown == mediaFormat) return kFskErrNeedMoreTime;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorAudioChannels, &audioChannels, NULL);
	if (0 == audioChannels) return kFskErrNeedMoreTime;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorAudioSampleRate, &audioSampleRate, NULL);
	if (0 == audioSampleRate) return kFskErrNeedMoreTime;

	RTPPacketParserSetAppReceivePacketCallback(track->packetParserRTP, audioPacketCB);
	
	err = kFskErrNone;
	
	track->audio.sampleRate = audioSampleRate;
	track->audio.channelCount = audioChannels;
	track->packetParserRTP->mediaFormat = mediaFormat;

	FskRTSPReaderPrintfNormal("Audio time scale %ld", audioSampleRate);

	// Initialize the audio decoder
	switch (mediaFormat) {
		case kRTPAudioFormatAAC:
			err = aacInitialize(track);
			break;

		case kRTPAudioFormatAMR:
			err = amrInitialize(track);
			break;

		case kRTPAudioFormatQCELP:
			err = qcelpInitialize(track);
			break;

		case kRTPAudioFormatPCM16BitBigEndian:
			track->format = FskStrDoCopy("x-audio-codec/pcm-16-be");
			track->audio.bytesPerSample = 2;
			break;

		case kRTPAudioFormatPCM16BitLittleEndian:
			track->format = FskStrDoCopy("x-audio-codec/pcm-16-le");
			track->audio.bytesPerSample = 2;
			break;
		
		case kRTPAudioFormatPCM8BitTwosComplement:
			track->format = FskStrDoCopy("x-audio-codec/pcm-8-twos");
			track->audio.bytesPerSample = 1;
			break;

		case kRTPAudioFormatPCM8BitOffsetBinary:
			track->format = FskStrDoCopy("x-audio-codec/pcm-8-offset");
			track->audio.bytesPerSample = 1;
			break;
			
		default:
			err = 1;
			break;
	}
	
	// If we can't configure the audio decoder then claim that the stream has no audio
	if (0 != err) {
		state->hasAudio = false;
		err = kFskErrNone;
		goto bail;
	}
	
	state->hasAudio = true;

	attr = SDPFindMediaAttribute(track->stream->mediaDescription, "AS");
	if (NULL != attr)
		track->bitrate = FskStrToNum(attr->value) * 1000;

	track->scale = audioSampleRate;

	// Calculate the track duration
	track->duration = (UInt32)(getPresentationRange(state->session) * track->scale);
	if (0 == track->duration)
		track->duration = kFskUInt32Max;	// live

	// We check with the packet parser to see if they can handle droppped packets.
	// If the packet parser can't handle dropped packets, the rtsp player will
	// insert silence for packet gaps.
	track->audio.needsQueueSilenceCheck = true;

bail:
	return err;
}

FskErr aacInitialize(rtspReaderTrack track)
{
	FskErr err = 0;

	track->audio.bytesPerSample = 2;
	track->audio.samplesPerDecode = 1024;

	track->audio.needsTimestampCheck = true;

	track->format = FskStrDoCopy("x-audio-codec/aac");

	return err;
}

FskErr amrInitialize(rtspReaderTrack track)
{
	FskErr err = 0;

	track->audio.samplesPerDecode = 160;
	track->audio.bytesPerSample = 2;

	track->format = FskStrDoCopy("x-audio-codec/amr-nb");

	return err;
}

FskErr qcelpInitialize(rtspReaderTrack track)
{
	FskErr err = 0;

	track->audio.samplesPerDecode = 160;
	track->audio.bytesPerSample = 2;

	track->format = FskStrDoCopy("x-audio-codec/qcelp");

	return err;
}

static void audioFlush(rtspReaderTrack track)
{
}

#if 0
static FskErr queueSilence(rtspReaderTrack track, UInt32 samplesInBuffer, UInt32 samplesToQueue, UInt32 *bufferSize, char **buffer)
{
	UInt32 bytesPerSample = track->audio.bytesPerSample * track->audio.channelCount;
	UInt32 bytesInBuffer = samplesInBuffer * bytesPerSample;
	UInt32 bytesToBuffer = samplesToQueue * bytesPerSample;
	UInt8 *bufferPtr;
	FskErr err = 0;

	// Grow buffer if necessary
	if (bytesToBuffer + bytesInBuffer > *bufferSize) {
		err = FskMemPtrRealloc(bytesToBuffer + bytesInBuffer, (FskMemPtr*)buffer);
		BAIL_IF_ERR(err);

		*bufferSize = bytesToBuffer + bytesInBuffer;
	}

	bufferPtr = (UInt8 *)(*buffer + bytesInBuffer);
	FskMemSet(bufferPtr, 0, bytesToBuffer);

	track->state->audioTime += samplesToQueue;

bail:
	return err;
}
#endif

FskErr audioPacketCB(RTPPacket packet, void *refCon)
{
	FskErr err;
	rtspReaderTrack track = refCon;
	rtspReader state = track->state;
	
	FskMediaReaderUsing(state->reader, true);

	FskTimeCallbackDispose(state->packetWatchdogTimer);
	state->packetWatchdogTimer = NULL;

	if (track->packetReceiveTimer) {
		//fprintf(stderr, "dispose audio packet receive timer\n");
		FskTimeCallbackDispose(track->packetReceiveTimer);
		track->packetReceiveTimer = NULL;
	}
	
	if (!state->hasAudio || state->audioDone) {
		err = RTPPacketDispose(track->packetParserRTP, packet, true);
	}
	else {
		err = enqueueAudioPacket(track, packet);
	}
		
	FskMediaReaderUsing(state->reader, false);

	return err;
}

FskErr enqueueAudioPacket(rtspReaderTrack track, RTPPacket packet)
{
	FskErr err = 0;
	RTSPMediaStream stream = track->stream;
	RTPPacketQueue queue = &track->packetQueue;

	if (kFskUInt32Max == track->zeroTime) {
		rtspReader state = track->state;

		if (kFskUInt32Max == track->duration)
			track->zeroTime = (UInt32)((FskInt64)state->startTime * track->scale / state->scale);
		else
			track->zeroTime = (UInt32)((FskInt64)state->session->startTime * track->scale);

		FskRTSPReaderPrintfNormal("Reset audio zero time to %ld", track->zeroTime);
	}
	
	// Reject packets older than what we have already decoded.
	// Note that this will still allow out-of-order packets to be
	// reordered in the packet queue below, but it will properly
	// reject packets that have no hope of being reordered.
	if ((NULL == packet->frames && -1 == packet->totalSamples) || packet->sequenceNumber < track->nextSeq) {
		FskRTSPReaderPrintfNormal("Audio: rejecting packet %ld sequence %d < next seq %d", packet->packetNumber, packet->sequenceNumber, track->nextSeq);
		RTPPacketDispose(queue->packetParser, packet, true);
		goto bail;
	}

	if (kFskUInt32Max == stream->rtpTime) {
		stream->rtpTime = packet->timestamp;
	}
	
	// On live streams the RTSP session manager sets the stream starting sequence number to zero.
	// In practice, some streaming servers deliver live packet sequences where the packet sequence
	// numbers start higher.  So we set the starting sequence number here based on the first packet received.
	if ((0 == stream->seq) && track->needsSeqCheck) {
		track->needsSeqCheck = false;
		stream->seq = packet->sequenceNumber;
	}
	
	// This *should* never occur, but servers do occasionally send us bad
	// timestamps like these.  If not disposed here, the resulting negative
	// presentation time wreaks havoc during playback.  See BUGZ case 3474.
	if (packet->timestamp < stream->rtpTime) {
		RTPPacketDispose(queue->packetParser, packet, true);
		goto bail;
	}

	// Calculate the packet presentation time
	packet->presentationTime = (packet->timestamp - stream->rtpTime) + track->zeroTime;
	
	// Have we queued all the audio?
	if (kFskUInt32Max != track->duration) {
		if (packet->presentationTime + track->audio.samplesPerDecode >= track->duration)
			track->audio.allAudioQueued = true;
	}
	
	if (track->audio.badTimeStamps) {
		fixAACPlusTimeStamp(track, packet);
	}
					
	// Enqueue the packet in the correct position
	enqueuePacket(track->state, queue, packet);
	
	if (track->audio.needsTimestampCheck) {
		if (checkForAACPlusTimeStamps(track)) {
			track->audio.needsTimestampCheck = false;
			
			// Patch up all the audio packet timestamps
			if (track->audio.badTimeStamps && (kFskUInt32Max != stream->rtpTime)) {
				RTPPacket walker = queue->head;
				while (NULL != walker) {
					fixAACPlusTimeStamp(track, walker);
					walker = walker->next;
				}
			}
		}
	}
	
	if (track->audio.needsQueueSilenceCheck) {
		UInt8 parserCanHandleDroppedPackets = false;
		track->audio.needsQueueSilenceCheck = false;
		
		if (0 == RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorHandleDroppedPackets, &parserCanHandleDroppedPackets, NULL)) {
			if (!track->audio.disableSilence)
				track->audio.disableSilence = parserCanHandleDroppedPackets;
		}
	}

bail:
	return err;
}

Boolean checkForAACPlusTimeStamps(rtspReaderTrack track)
{
	Boolean handled = false;
	RTPPacketQueue queue = &track->packetQueue;
	RTPPacket p0, packet = queue->head;
	
	if (NULL == packet)
		goto bail;
		
	p0 = packet;
	
	while (true) {
		RTPPacket p1, p2, p3 = 0;
		
		p1 = p0;
		p2 = p1->next;
		if (p2)
			p3 = p2->next;
		if (!p1 || !p2 || !p3) break;
		
		// The marker bit should be set on each packet indicating one frame per packet
		if (!p1->marker || !p2->marker || !p3->marker) break;
		
		// Need actual frame data
		if (!p1->frames) break;

		// If these are sequential packets, or "sequential" packets containing interleaved frames
		if (((p1->sequenceNumber + 1 == p2->sequenceNumber) && (p2->sequenceNumber + 1 == p3->sequenceNumber)) ||
			((p2->sequenceNumber - p1->sequenceNumber) == (p3->sequenceNumber - p2->sequenceNumber))) {
			RTPCompressedMediaFrame frame = p1->frames;
			UInt32 samplesPerFrame, *sampleSizeP = &frame->sampleSizes[0];
			UInt16 nFrames = 0;
			
			// Calculate the number of frames in the first packet
			do {
				++nFrames;
			} while (*sampleSizeP++);

			// Calculate the number of samples in the first frame
            
			samplesPerFrame = (p2->timestamp - p1->timestamp) / nFrames;

			track->audio.badTimeStamps = (samplesPerFrame != 1024);
			
			handled = true;
			break;
		}
		
		p0 = p2;
	}
	
bail:
	return handled;
}

void fixAACPlusTimeStamp(rtspReaderTrack track, RTPPacket packet)
{
	RTPCompressedMediaFrame frame;
	UInt32 *sampleSizeP;
	UInt32 nFrames = 0;
	
	if (NULL == packet->frames)
		return;
		
	frame = packet->frames;
	sampleSizeP = &frame->sampleSizes[0];

	while (*sampleSizeP++)
		++nFrames;
		
	packet->timestamp = track->stream->rtpTime + 
		(packet->sequenceNumber - track->stream->seq) * nFrames * 1024;
	packet->presentationTime = (packet->timestamp - track->stream->rtpTime) + track->zeroTime;
}

/*
	video
*/

FskErr createVideoTrack(rtspReader state, RTSPMediaStream stream)
{
	FskErr err = 0;
	rtspReaderTrack track;

	if (NULL == state->videoTrack) {
		err = FskMemPtrNewClear(sizeof(rtspReaderTrackRecord), &track);
		BAIL_IF_ERR(err);
		state->videoTrack = track;
		FskListAppend((void **)&state->tracks, track);
	}
	track = state->videoTrack;

	track->stream = stream;
	stream->refCon = (UInt32)track;

	track->readerTrack.dispatch = &gMediaReaderRTSPVideoTrack;
	track->readerTrack.state = track;

	state->scale = 1000;
	state->hasVideo = true;
	track->state = state;
	track->zeroTime = -1;
	track->needsSeqCheck = true;
	
	// Install our RTP/RTCP packet callbacks
	track->packetParserRTCP = stream->packetParserRTCP;
	RTCPPacketParserSetAppReceivePacketCallback(track->packetParserRTCP, videoRTCPPacketCB);
	RTCPPacketParserSetAppRefCon(track->packetParserRTCP, track);
	
	track->packetParserRTP = stream->packetParser;
	track->packetQueue.packetParser = track->packetParserRTP;
	RTPPacketParserSetAppReceivePacketCallback(track->packetParserRTP, videoPacketCB);
	RTPPacketParserSetAppRefCon(track->packetParserRTP, track);

bail:
	return err;
}

FskErr configureVideo(rtspReader state, rtspReaderTrack track)
{
	FskErr err = 1;
	UInt32 width, height, timeScale, mediaFormat;
	SDPAttribute attr;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorMediaFormat, &mediaFormat, NULL);
	if (kRTPMediaFormatUnknown == mediaFormat) return kFskErrNeedMoreTime;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorVideoWidth, &width, NULL);
	if (0 == width) return kFskErrNeedMoreTime;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorVideoHeight, &height, NULL);
	if (0 == height) return kFskErrNeedMoreTime;
	
	RTPPacketParserGetInfo(track->packetParserRTP, kRTPPacketParserSelectorVideoTimeScale, &timeScale, NULL);
	if (0 == timeScale) return kFskErrNeedMoreTime;

	RTPPacketParserSetAppReceivePacketCallback(track->packetParserRTP, videoPacketCB);
	
	err = kFskErrNone;
	
	// We're good!
	track->video.dimensions.width = width;
	track->video.dimensions.height = height;
	track->scale = timeScale;
	track->packetParserRTP->mediaFormat = mediaFormat;
	
	FskRTSPReaderPrintfNormal("Video time scale %ld", timeScale);

	switch (track->packetParserRTP->mediaFormat) {
		case kRTPVideoFormatH263:
			track->format = FskStrDoCopy("x-video-codec/263");
			break;
			
		case kRTPVideoFormatMPEG4:
			track->format = FskStrDoCopy("x-video-codec/mp4");
#if SUPPORT_B_FRAMES
			track->video.needsTimestampCheck = true;
#endif
			break;
			
		case kRTPVideoFormatAVC:
			track->format = FskStrDoCopy("x-video-codec/avc");
#if SUPPORT_B_FRAMES
			track->video.needsTimestampCheck = true;
#endif
			break;
		
		default:
			err = 1;
			break;
	}
	
	if (0 != err) {
		state->hasVideo = false;
		err = kFskErrNone;
		goto bail;
	}

	state->hasVideo = true;

	attr = SDPFindMediaAttribute(track->stream->mediaDescription, "AS");
	if (NULL != attr)
		track->bitrate = FskStrToNum(attr->value) * 1000;

	// Calculate the track duration
	track->duration = (UInt32)(getPresentationRange(state->session) * track->scale);
	if (0 == track->duration)
		track->duration = -1;	// live
	
bail:
	return err;
}

FskErr videoPacketCB(RTPPacket packet, void *refCon)
{
	FskErr err;
	rtspReaderTrack track = refCon;
	rtspReader state = track->state;
	
	FskMediaReaderUsing(state->reader, true);

	FskTimeCallbackDispose(state->packetWatchdogTimer);
	state->packetWatchdogTimer = NULL;

	if (track->packetReceiveTimer) {
		//fprintf(stderr, "dispose video packet receive timer\n");
		FskTimeCallbackDispose(track->packetReceiveTimer);
		track->packetReceiveTimer = NULL;
	}
		
	if (!state->hasVideo || state->videoDone) {
		err = RTPPacketDispose(track->packetParserRTP, packet, true);
	}
	else {
		err = enqueueVideoPacket(track, packet);
	}
		
	FskMediaReaderUsing(state->reader, false);

	return err;
}

FskErr videoRTCPPacketCB(RTCPPacket packet, void *refCon)
{
	rtspReaderTrack track = (rtspReaderTrack)refCon;
	rtspReader state = track->state;
	RTSPSession session = track->state->session;
	UInt16 i;
	FskErr err = 0;
	
	FskMediaReaderUsing(state->reader, true);

	if (kRTCPPacketTypeSenderReport == packet->header.PT) {
		for (i = 0; i < session->nMediaStreams; ++i) {
			if (session->mediaStream[i].ssrc == packet->flavor.sr.SSRC) {
				session->mediaStream[i].sendReceiverReport = true;
				break;
			}
		}
	}
	else if (kRTCPPacketTypeBYE == packet->header.PT) {
		for (i = 0; i < session->nMediaStreams; ++i) {
			if (session->mediaStream[i].ssrc == packet->flavor.sr.SSRC) {
				FskRTSPReaderPrintfNormal("Got video BYE");
				track->allMediaReceived = true;
				break;
			}
		}
	}

	RTCPPacketDispose(track->packetParserRTCP, packet);

	FskMediaReaderUsing(state->reader, false);
	return err;
}

FskErr enqueueVideoPacket(rtspReaderTrack track, RTPPacket packet)
{
	FskErr err = 0;
	RTSPMediaStream stream = track->stream;
	RTPPacketQueue queue = &track->packetQueue;

	if (kFskUInt32Max == track->zeroTime) {
		rtspReader state = track->state;

		if (kFskUInt32Max == track->duration)
			track->zeroTime = (UInt32)((FskInt64)state->startTime * track->scale / state->scale);
		else
			track->zeroTime = (UInt32)((FskInt64)state->session->startTime * track->scale);

		FskRTSPReaderPrintfNormal("Reset video zero time to %ld", track->zeroTime);
	}
	
	// Reject packets older than what we have already decoded.
	// Note that this will still allow out-of-order packets to be
	// reordered in the packet queue below, but it will properly
	// reject packets that have no hope of being reordered.
	if ((NULL == packet->frames && -1 == packet->totalSamples) || packet->sequenceNumber < track->nextSeq) {
		RTPPacketDispose(queue->packetParser, packet, true);
		goto bail;
	}

	if (kFskUInt32Max == stream->rtpTime) {
		stream->rtpTime = packet->timestamp;
	}
	
	// On live streams the RTSP session manager sets the stream starting sequence number to zero.
	// In practice, some streaming servers deliver live packet sequences where the packet sequence
	// numbers start higher.  So we set the starting sequence number here based on the first packet received.
	if ((0 == stream->seq) && track->needsSeqCheck) {
		track->needsSeqCheck = false;
		stream->seq = packet->sequenceNumber;
	}
	
	// This *should* never occur, but servers do occasionally send us bad
	// timestamps like these.  If not disposed here, the resulting negative
	// presentation time wreaks havoc during playback.  See BUGZ case 3474.
	if ((packet->timestamp < stream->rtpTime) && !track->video.needsTimestampCheck) {
		RTPPacketDispose(queue->packetParser, packet, true);
		goto bail;
	}

	// Calculate the packet presentation time
	packet->presentationTime = (packet->timestamp - stream->rtpTime) + track->zeroTime;
	
	// Calculate the packet decode time
	if (!track->video.needsTimestampCheck && (0 != track->video.decodeIncrementalTimeStep)) {
		RTPCompressedMediaFrame frame;
		frame = packet->frames;
		while (NULL != frame) {
			SInt32 decodeTimeStamp;
			frame->compositionTimeStamp = (frame->compositionTimeStamp - stream->rtpTime);
			decodeTimeStamp = (SInt32)frame->compositionTimeStamp - track->video.decodeIncrementalTimeStep;
			if (decodeTimeStamp < 0)
				decodeTimeStamp = 0;
			if (decodeTimeStamp <= (SInt32)track->video.nextDecodeTime)
				decodeTimeStamp = track->video.nextDecodeTime + track->video.decodeIncrementalTimeStepBump;
			track->video.nextDecodeTime = decodeTimeStamp;
			frame->decodeTimeStamp = decodeTimeStamp + track->zeroTime;
			frame->compositionTimeStamp = frame->compositionTimeStamp + track->zeroTime;
			frame = frame->next;
		}
	}

	// Enqueue the packet in the correct position
	enqueuePacket(track->state, queue, packet);
	
	// If the packets have composition time stamps, we need to calculate the decode time step
	if (track->video.needsTimestampCheck) {
		if (FskListCount(queue->head) >= 8) {
			track->video.needsTimestampCheck = false;
			calculateVideoDecodeTimeStep(track);
		}
	}
					
bail:
	return err;
}

static int compareTimeStamps(const void *a, const void *b)
{
	UInt32 t1 = *(UInt32*)a;
	UInt32 t2 = *(UInt32*)b;
	if (t1 == t2)
		return 0;
	else
	if (t1 < t2)
		return -1;
	return +1;
}

Boolean calculateVideoDecodeTimeStep(rtspReaderTrack track)
{
	RTPPacketQueue queue = &track->packetQueue;
	RTPPacket walker = queue->head;
	UInt32 i, j, count, *compositionTimeStamps = NULL, *sortedCompositionTimeStamps = NULL;
	UInt32 delta, maxDelta;
	Boolean haveCompositionTimeStamps = false;
	RTPCompressedMediaFrame frame;
	FskErr err;
	
	// First make sure we have composition time stamps
	while (NULL != walker) {
		frame = walker->frames;
		if (0 != frame->compositionTimeStamp) {
			haveCompositionTimeStamps = true;
			break;
		}
		walker = walker->next;
	}
	if (!haveCompositionTimeStamps) goto bail;
		
	// Next sort our packet composition time stamps
	walker = queue->head;
	count = 0;
	while (NULL != walker) {
		frame = walker->frames;
		while (NULL != frame) {
			++count;
			err = FskMemPtrRealloc(count * sizeof(UInt32), (FskMemPtr*)&compositionTimeStamps);
			BAIL_IF_ERR(err);
			compositionTimeStamps[count-1] = frame->compositionTimeStamp;
			frame = frame->next;
		}
		walker = walker->next;
	}
	
	err = FskMemPtrNew(count * sizeof(UInt32), (FskMemPtr*)&sortedCompositionTimeStamps);
    BAIL_IF_ERR(err);
	
	FskMemMove(sortedCompositionTimeStamps, compositionTimeStamps, count * sizeof(UInt32));
	FskQSort(sortedCompositionTimeStamps, count, sizeof(UInt32), compareTimeStamps);

	// If the sorted composition times are the same as the packet time stamps, then the decode time is equal to the composition time
	// and we don't need to do anything special
	for (i = 0, haveCompositionTimeStamps = false; i < count; ++i) {
		if (compositionTimeStamps[i] != sortedCompositionTimeStamps[i]) {
			haveCompositionTimeStamps = true;
			break;
		}
	}
	if (!haveCompositionTimeStamps) goto bail;
	
	// OK, it looks like we've got b-frames, since the composition time stamps are out of order compared to the decode order.
	for (i = 0, maxDelta = 0; i < count - 1; ++i) {
		UInt32 timeStamp = compositionTimeStamps[i];
		for (j = i + 1; j < count; ++j) {
			UInt32 timeStamp2 = compositionTimeStamps[j];
			if (timeStamp2 < timeStamp) {
				delta = timeStamp - timeStamp2;
				if (delta > maxDelta)
					maxDelta = delta;
			}
		}
	}

	track->video.decodeIncrementalTimeStep = maxDelta;
	track->video.decodeIncrementalTimeStepBump = (track->video.decodeIncrementalTimeStep/10);
	
	//track->stream->rtpTime = queue->head->timestamp;
	//track->stream->rtpTime = queue->head->frames->compositionTimeStamp;
	track->stream->rtpTime = sortedCompositionTimeStamps[0];
	track->video.nextDecodeTime = 0;
	
	// Go back through the queue and fix up the decode time stamps
	walker = queue->head;
	count = 0;
	while (NULL != walker) {
		frame = walker->frames;
		while (NULL != frame) {
			SInt32 decodeTimeStamp;
			frame->compositionTimeStamp = (frame->compositionTimeStamp - track->stream->rtpTime);
			decodeTimeStamp = (SInt32)frame->compositionTimeStamp - track->video.decodeIncrementalTimeStep;
			if (decodeTimeStamp < 0)
				decodeTimeStamp = 0;
			if (decodeTimeStamp <= (SInt32)track->video.nextDecodeTime && 0 != count)
				decodeTimeStamp = track->video.nextDecodeTime + track->video.decodeIncrementalTimeStepBump;
			track->video.nextDecodeTime = decodeTimeStamp;
			frame->decodeTimeStamp = decodeTimeStamp + track->zeroTime;
			frame->compositionTimeStamp = frame->compositionTimeStamp + track->zeroTime;
			frame = frame->next;
			++count;
		}
		walker = walker->next;
	}


bail:
	FskMemPtrDispose(compositionTimeStamps);
	FskMemPtrDispose(sortedCompositionTimeStamps);
	
	return haveCompositionTimeStamps;
}

/*
	session
*/

FskErr initSession(rtspReader rtsp)
{
	char *userAgent;
	RTSPSession session = 0;
	FskErr err = 0;
	char *rtpMode = FskEnvironmentGet("RTPMode");

	FskRTSPReaderPrintfNormal("In InitSession");

	if (NULL != rtpMode)
		rtsp->useTCP = (0 == FskStrCompare(rtpMode, "tcp"));

	err = RTSPSessionNew(&session, rtsp->useTCP);
	BAIL_IF_ERR(err);
	
	rtsp->session = session;

	RTSPSessionSetRefCon(session, (void*)rtsp);

#if RTSP_DEBUG_DROP_PACKETS
	RTSPSessionSetDroppedPackets(session, 10, 2, NULL);
#endif

	userAgent = FskEnvironmentGet("http-user-agent");
	RTSPSessionSetUserAgent(session, userAgent);
	RTSPSessionSetNetworkType(session, NULL);
	RTSPSessionSetReferer(session, NULL);
	RTSPSessionSetResponseHeaderCallback(session, responseHeaderCB);
	RTSPSessionSetStateChangeCallback(session, stateChangeCB);
	
	if (0 != rtsp->proxyAddr) {
		RTSPSessionSetProxy(session, NULL, rtsp->proxyAddr, rtsp->proxyPort);
	}

	// Disable Reliable UDP for archive.org streams, since their media delivery isn't so great
	if (NULL != FskStrStr(rtsp->uri, "us.archive.org")) {
		RTSPSessionSetFeature(session, kRTSPFeatureDisableReliableUDP, NULL);
	}

	// Drop the connection after 60 seconds of inactivity.
	// Note this isn't necessary when using TCP for RTP packets, since we dispose the session on stop.
	if (!rtsp->useTCP)
		RTSPSessionSetIdleTimeout(session, 60);			// after 60 seconds, drop the connection
	
	rtsp->needSessionStart = false;
	rtsp->msToBuffer = kMSToBuffer;
	rtsp->disablePause = rtsp->useTCP;	// Servers delivering RTP over TCP don't like the PAUSE command

bail:
	FskRTSPReaderPrintfNormal("Out initSession");
	return err;
}

FskErr disposeSession(rtspReader state)
{
	FskErr err = 0;
	RTSPSession session = state->session;

	FskRTSPReaderPrintfNormal("In disposeSession");

	if (NULL == session || NULL == session->skt)
		goto bail;
	
	RTSPSessionTeardown(session);

#if 0
	// @@ Peter suggests we don't wait since otherwise it could slow down media browsing
	// Give the TEARDOWN up to a second to complete
	{
	FskTimeRecord now, when;
	FskTimeGetNow(&when);
	FskTimeAddSecs(&when, 1);
	do {
		FskThreadYield();
		FskTimeGetNow(&now);
	} while (!session->tornDown && (FskTimeCompare(&when, &now) < 0));
	}
#endif

bail:
	err = RTSPSessionDispose(session);

	FskRTSPReaderPrintfNormal("Out disposeSession");

	return err;
}

FskErr stateChangeCB(SInt16 newState, void *refCon)
{
	FskErr err = 0;
	rtspReader state = (rtspReader)refCon;
	RTSPSession session = state->session;

	// Avoid an infinite loop where FskMediaReaderUsing() tries to dispose us when we're already disposing
	if (state->disposing) {
		FskRTSPReaderPrintfNormal("In stateChangeCB when disposing");
		return 0;
	}

	FskMediaReaderUsing(state->reader, true);

	FskRTSPReaderPrintfNormal("In stateChangeCB");

	// Handle redirect and timeout here, otherwise get out on any other error
	if (kRTSPClientStateError == newState) {
		err = session->status.lastErr;
		if (kFskErrRTSPSessionRedirect == err) {
			if (NULL != session->redirect) {
				state->redirect = FskStrDoCopy(state->session->redirect);
				state->dontPause = true;
				doReaderSetState(state, kFskMediaPlayerStateStopped);
				state->dontPause = false;
				err = 0;
			}
		}
		else
		if (kFskErrTimedOut == err) {
			rtspReaderTrack track;

			disposeSession(state);
			state->session = NULL;

			track = state->tracks;
			while (NULL != track) {
				rtspReaderTrack next = track->next;
				track->packetParserRTP = NULL;
				track->packetParserRTCP = NULL;
				track->stream = NULL;
				track = next;
			}
			state->needSessionStart = true;
			err = 0;
		}
		goto bail;
	}

	// If a connection has been established with the host, so go ahead and start the session
	if (kRTSPClientStateConnected == newState && state->session->sessionState == kRTSPClientStateIdle) {
		err = RTSPSessionStart(session);
	}

bail:
	reportIfError(state, err);

	FskRTSPReaderPrintfNormal("Out stateChangeCB");

	FskMediaReaderUsing(state->reader, false);

	return 0;
}

FskErr responseHeaderCB(char *method, FskHeaders *headers, void *refCon)
{
	rtspReader state = (rtspReader)refCon;
	RTSPSession session = state->session;
	FskErr err = 0;

	FskMediaReaderUsing(state->reader, true);

	FskRTSPReaderPrintfNormal("In responseHeaderCB, method = %s", (char*)method);

	// When response to PAUSE is received, we're stopped
	if (0 == FskStrCompareCaseInsensitive(method, "PAUSE")) {
		state->stopping = false;

		FskRTSPReaderPrintfNormal("Got PAUSE response");
	}

	// When response to PLAY is received, we're no longer starting
	if (0 == FskStrCompareCaseInsensitive(method, "PLAY")) {
		FskRTSPReaderPrintfNormal("Got PLAY response");

		state->starting = false;

		if (state->audioTrack) {
			state->audioTrack->nextSeq = state->audioTrack->stream->seq;
			FskRTSPReaderPrintfNormal("Reset audio next seq to %d", state->audioTrack->nextSeq);
		}
		if (state->videoTrack) {
			state->videoTrack->nextSeq = state->videoTrack->stream->seq;
			FskRTSPReaderPrintfNormal("Reset video next seq to %d", state->videoTrack->nextSeq);
		}

		// Reschedule our periodic C->S OPTIONS request and packet watchdog timers
		if (!state->useTCP) {
			if (NULL == state->optionsTimer)
				FskTimeCallbackNew(&state->optionsTimer);
			FskTimeCallbackScheduleFuture(state->optionsTimer, RTSP_OPTIONS_INTERVAL_SECS, 0, optionsTimerCB, state);

			if (NULL == state->packetWatchdogTimer)
				FskTimeCallbackNew(&state->packetWatchdogTimer);
			FskTimeCallbackScheduleFuture(state->packetWatchdogTimer, RTSP_PACKET_RECEIVE_TIMEOUT_SECS, 0, packetWatchdogTimerCB, state);
		}
	}
	
	if (0 == FskStrCompareCaseInsensitive(method, "SETUP")) {
		if (state->session->useDynamicBufferingRate)
			state->msToBuffer = kMSToBuffer / 2;

		// Handle first-time reader instantiation if all the streams are ready
		if ((state->reader->mediaState <= kFskMediaPlayerStateInstantiating) || state->needToPlay) {
			UInt16 i;
			for (i = 0; i < session->nMediaStreams; ++i) {
				if (!session->mediaStream[i].ready)
					goto bail;
			}

			err = rtspInstantiate(state);
			BAIL_IF_ERR(err);

			if (state->audioTrack && !state->hasAudio)
				state->audioTrack->disabled = true;
			if (state->videoTrack && !state->hasVideo)
				state->videoTrack->disabled = true;

			if (state->audioTrack && !state->audioTrack->disabled)
				state->scale = state->audioTrack->scale;
			else
			if (state->videoTrack && !state->videoTrack->disabled)
				state->scale = state->videoTrack->scale;

			// Calculate the duration
			state->duration = (UInt32)(getPresentationRange(session) * state->scale);
			if (0 == state->duration)
				state->duration = kFskUInt32Max;	// live

			setMediaTitle(state);

			if (state->needToPlay) {
				state->needToPlay = false;
				err = doPlay(state);
				BAIL_IF_ERR(err);
			}
			else {
				FskRTSPReaderPrintfNormal("RTSP reader: about to change state to stopped");
				state->dontPause = true;
				doReaderSetState(state, kFskMediaPlayerStateStopped);
				state->dontPause = false;
				FskRTSPReaderPrintfNormal("RTSP reader: changed state to stopped");
			}
		}
	}

bail:
	FskRTSPReaderPrintfNormal("Out responseHeaderCB");

	FskMediaReaderUsing(state->reader, false);

	return err;	
}


double getPresentationRange(RTSPSession session)
{
	double range = 0;
	SDPSessionDescription sdp = session->sdp;
	SDPAttribute attribute;
	char *value = 0;
	
	// First look for the session level "range" attribute
	attribute = SDPFindSessionAttribute(sdp, "range");
	if (0 != attribute) {
		char *parts[4];
		UInt16 nParts = 2;
		value = FskStrDoCopy(attribute->value);
		splitToken(value, &nParts, '=', &parts[0]);
		
		// npt time format
		if (0 == FskStrCompareCaseInsensitive("npt", parts[0])) {
			double start, end;
			nParts = 2;
			splitToken(parts[1], &nParts, '-', &parts[2]);
			if (2 == nParts) {
				start = FskStrToD(FskStrStripHeadSpace(parts[2]), 0);
				end = FskStrToD(FskStrStripHeadSpace(parts[3]), 0);
				range = end - start;
			}
		}

		// clock time format
		else if (0 == FskStrCompareCaseInsensitive("clock", parts[0])) {
			// @@
		}
	}
	FskMemPtrDispose(value);
	
	// bugzid: 36380
#if 0
	if (0 == FskStrCompare(session->sdp->origin.username, "Orbiter")) {
		if (range > 2)
			range -= 2;
	}
#endif
	
	return range;
}

void setMediaTitle(rtspReader state)
{
	RTSPSession session = state->session;
	char *title = NULL;

	if (0 != session->sdp->name) {
		SDPAttribute attr;

		if (0 == FskStrCompare(session->sdp->name, "live.3gp"))
			return;				// orb hack to ignore their unneeded name

		attr = SDPFindSessionAttribute(session->sdp, "x-qt-text-nam");
		if (0 != attr)
			title = FskStrDoCopy(attr->value);
		else					
			title = FskStrDoCopy(session->sdp->name);
			
		// Eliminate leading slash
		if (title[0] == '/' || title[0] == '\\') {
			FskMemMove(title, &title[1], FskStrLen(title));
		}
	}
#if 0
	else if (0 != session->path) {
		title = FskStrDoCopy(session->path);
	}
#endif

	if (NULL != title) {
		FskMediaPropertyValueRecord prop;
		while (true) {
			if (kFskErrNone != FskMediaMetaDataRemove(state->mi.meta, "FullName", 0))
				break;
		}
		prop.type = kFskMediaPropertyTypeString;
		prop.value.str = FskStrDoCopy(title);
		FskMediaMetaDataAdd(state->mi.meta, "FullName", NULL, &prop, kFskMediaMetaDataFlagOwnIt);
	}

	FskMemPtrDispose(title);
}

void enqueuePacket(rtspReader state, RTPPacketQueue queue, RTPPacket packet)
{
	RTPPacket walker, prior;

	if (0 == queue->head) {
		queue->head = queue->tail = packet;
		packet->next = 0;
	}
	else if (0 != queue->tail && packet->sequenceNumber > queue->tail->sequenceNumber) {
		queue->tail->next = packet;
		queue->tail = packet;
		packet->next = 0;
	}
	else {
		walker = queue->head;
		prior = 0;
		while (0 != walker && packet->sequenceNumber > walker->sequenceNumber) {
			prior = walker;
			walker = walker->next;
		}
		if (0 == walker) {
			if (0 != queue->tail)
				queue->tail->next = packet;
			queue->tail = packet;
			packet->next = 0;
		}
		else {
			if (0 != prior)
				prior->next = packet;
			packet->next = walker;
		}
	}
	if (NULL != state->reader->eventHandler)
		(state->reader->eventHandler)(state->reader, state->reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);
}

// empties the packet and everything before it
void emptyQueuePacket(RTPPacketQueue queue, RTPPacket packet)
{
	if (NULL == packet)
		return;

	while (queue->head) {
		RTPPacket walker = queue->head;
		RTPPacket next = walker->next;

		RTPPacketDispose(queue->packetParser, walker, true);

		queue->head = next;
		if (queue->tail == walker) {
			queue->tail = NULL;
			break;
		}

		if (walker == packet)
			break;
	}
}

void flushAll(rtspReader state)
{
	rtspReaderTrack track;

	audioFlush(state->audioTrack);
	
	track = state->tracks;
	while (NULL != track) {
		emptyQueuePacket(&track->packetQueue, track->packetQueue.tail);
		track->allMediaReceived = false;
		//fprintf(stderr, "dispose packet receive timer\n");
		FskTimeCallbackDispose(track->packetReceiveTimer);
		track->packetReceiveTimer = NULL;
		track = track->next;
	}
}

FskErr rtspInstantiate(rtspReader state)
{
	FskErr err = 0;
	RTSPSession session = state->session;
	UInt16 i;

	// Create the tracks
	for (i = 0; i < session->nMediaStreams; ++i) {
		if (session->mediaStream[i].streamType == kRTSPMediaStreamAudio) {
			err = createAudioTrack(state, &session->mediaStream[i]);
			BAIL_IF_ERR(err);

			if (!state->audioConfigured)
				if (kFskErrNone == configureAudio(state, state->audioTrack))
					state->audioConfigured = true;
		}
		if (session->mediaStream[i].streamType == kRTSPMediaStreamVideo) {
			err = createVideoTrack(state, &session->mediaStream[i]);
			BAIL_IF_ERR(err);

			if (!state->videoConfigured)
				if (kFskErrNone == configureVideo(state, state->videoTrack))
					state->videoConfigured = true;
		}
	}

bail:
	return err;
}

FskErr rtspInitializeFromSDP(rtspReader state)
{
	FskErr err = 0;

	err = RTSPSessionSetSDP(state->session, state->sdp, state->sdpSize);

	return err;
}

FskErr doPlay(rtspReader state)
{
	SInt32 timeToPlay;
	FskMediaPropertyValueRecord timeNow;
	UInt32 atTime;
	rtspReaderTrack track;
	FskErr err = 0;

	// Reset our track states for the new time
	track = state->tracks;
	while (NULL != track) {
		track->zeroTime = kFskUInt32Max;
		track = track->next;
	}

	if (!state->hasVideo && state->hasAudio && state->audioTrack) {
		state->audioTrack->audio.disableSilence = true;	// don't insert silence for audio-only
	}

	rtspReaderGetTime(state, NULL, 0, &timeNow);
	atTime = (UInt32)timeNow.value.number;
	timeToPlay = (kFskUInt32Max == state->duration) ? kFskUInt32Max : (SInt32)((double)atTime * 1000 / state->scale);

#if SUPPORT_INSTRUCTION
	if (kFskUInt32Max != state->duration) {
		FskRTSPReaderPrintfNormal("PLAY at %f secs, duration = %f secs", (double)atTime / state->scale, (double)state->duration / state->scale);
	}
#endif

	err = RTSPSessionPlay(state->session, timeToPlay);
	BAIL_IF_ERR(err);

	state->starting = true;

bail:
	return err;
}

void doReaderSetState(rtspReader state, SInt32 newState)
{
	(state->reader->doSetState)(state->reader, newState);
}

void reportIfError(rtspReader state, FskErr err)
{
	if (0 != err) {
		switch(err) {
			case kFskErrRTSPBadPacket:
			case kFskErrNoData:
				if (err == state->session->status.lastErr)
					state->session->status.lastErr = 0;
				break;

			default:
				state->theError = err;
				doReaderSetState(state, kFskMediaPlayerStateFailed);
				break;
		}
	}
}

void optionsTimerCB(FskTimeCallBack callback, const FskTime time, void *param)
{
	rtspReader state = (rtspReader)param;
	RTSPSession session = state->session;

	FskMediaReaderUsing(state->reader, true);

	// Send periodic OPTIONS requests to keep the socket alive
	RTSPSessionOptions(session);

	FskTimeCallbackScheduleFuture(state->optionsTimer, RTSP_OPTIONS_INTERVAL_SECS, 0, optionsTimerCB, state);

	FskMediaReaderUsing(state->reader, false);
}

void packetWatchdogTimerCB(FskTimeCallBack callback, const FskTime time, void *param)
{
	rtspReader state = (rtspReader)param;

	FskMediaReaderUsing(state->reader, true);

	reportIfError(state, kFskErrRTSPNoUDPPackets);

	FskMediaReaderUsing(state->reader, false);
}

void packetReceiveTimerCB(FskTimeCallBack callback, const FskTime time, void *param)
{
	rtspReaderTrack track = (rtspReaderTrack)param;
	rtspReader state = track->state;

	FskMediaReaderUsing(state->reader, true);

	if (track == state->audioTrack) {
		state->audioDone = true;
	}
	else {
		state->videoDone = true;
	}
	if (NULL != state->reader->eventHandler)
		(state->reader->eventHandler)(state->reader, state->reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);
	
	FskMediaReaderUsing(state->reader, false);
}

FskErr rtspSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	rtspReader state = (rtspReader)clientRefCon;
	FskErr err = kFskErrNone;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped) {
				void *buffer;
				UInt32 bytesRead;
				FskInt64 sdpSize = 0;

				if (NULL != state->spooler->doGetSize) {
					err = (state->spooler->doGetSize)(state->spooler, &sdpSize);
					BAIL_IF_ERR(err);
				}
				if (0 == sdpSize)
					sdpSize = 2048;

				err = FskMediaSpoolerRead(state->spooler, state->spoolerPosition, (UInt32)sdpSize, &buffer, &bytesRead);
				if (kFskErrNone == err) {
					err = FskMemPtrRealloc((UInt32)state->spoolerPosition + bytesRead, (FskMemPtr*)&state->sdp);
					BAIL_IF_ERR(err);

					FskMemMove(state->sdp + state->spoolerPosition, buffer, bytesRead);
					state->spoolerPosition += bytesRead;
				}
				if (kFskErrNeedMoreTime == err) {
					err = kFskErrNone;
				}
				else
				if ((kFskErrEndOfFile == err) || (sdpSize && (state->spoolerPosition == sdpSize))) {
					state->sdpSize = (UInt32)state->spoolerPosition;
					err = rtspInitializeFromSDP(state);
				}
			}
			break;
	}

bail:
	return err;
}

