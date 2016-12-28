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

		video overlay support
		X test clipping code (update kp5/ks5 to use)
		X read buffer on file spooler
		streaming
			X rebuffer when running dry
			X live (-1 == duration)
			X tossing used data
			X seeking
			~ redirects
			X http request headers to client
			X http response headers to client
			size buffer and buffer count based on content bit rate
*/

#define __FSKMEDIAPLAYER_PRIV__
#define __FSKMEDIAREADER_PRIV__
#define __FSKPORT_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
//#define SUPPORT_RECORD 1
#include "FskMediaPlayer.h"
#include "FskMediaReader.h"
#include "FskAudio.h"
#include "FskEnvironment.h"
#include "FskHTTPClient.h"
#include "FskHTTPAuth.h"
#include "FskMedia.h"
#include "FskRectBlit.h"
#include "FskVideoSprite.h"
#if SUPPORT_RECORD
	#include "FskMuxer.h"
#endif
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif

#include "FskPlatformImplementation.h"

#if TARGET_OS_WIN32
	FskErr FskMemPtrNewLarge(UInt32 size, FskMemPtr *mem);
	void FskMemPtrDisposeLarge(FskMemPtr mem);
#else
	#define FskMemPtrNewLarge(a, b) FskMemPtrNew(a, b)
	#define FskMemPtrDisposeLarge(a) FskMemPtrDispose(a)
#endif

static void readerMediaPlayerInitializeJumpTable(FskMediaPlayerModule module);
static FskErr readerMediaPlayerCanHandle(UInt32 dataSourceType, const char *mime, UInt32 flags);
static FskErr readerMediaPlayerSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

FskMediaPlayerEntryRecord gMediaPlayerReader =
	{readerMediaPlayerInitializeJumpTable,		readerMediaPlayerInitializeJumpTable,	NULL,	readerMediaPlayerCanHandle, readerMediaPlayerSniff};

/*
	Reader Player
*/

static FskErr readerMediaPlayerModuleNew(FskMediaPlayerModule module, const void *dataSource, UInt32 dataSourceType, const char *mime, FskMediaPlayerPropertyIdAndValue properties);
static void readerMediaPlayerModuleDispose(void *state, FskMediaPlayerModule module);

static FskErr readerMediaPlayerModuleGetTime(void *stateIn, FskMediaPlayerModule module, float scale, FskSampleTime *time);
static FskErr readerMediaPlayerModuleSetTime(void *state, FskMediaPlayerModule module, float scale, FskSampleTime time);
static FskErr readerMediaPlayerModuleStart(void *state, FskMediaPlayerModule module);
static FskErr readerMediaPlayerModuleStop(void *state, FskMediaPlayerModule module);
static Boolean readerMediaPlayerModuleWillDraw(void *state, FskMediaPlayerModule module, FskSampleTime beforeTime);
static FskErr readerMediaPlayerModuleUpdate(void *stateIn, FskMediaPlayerModule module);
static FskErr readerMediaPlayerModuleGetMetadata(void *stateIn, FskMediaPlayerModule module, const char *metaDataType, UInt32 index, FskMediaPropertyValue meta, UInt32 *flags);
static FskErr readerMediaPlayerModuleGetVideoBitmap(void *stateIn, FskMediaPlayerModule module, UInt32 width, UInt32 height, FskBitmap *bitmap);
static FskErr readerMediaPlayerModulePropertyChanged(void *stateIn, FskMediaPlayerModule module, UInt32 property);
static FskErr readerMediaPlayerModuleGetTrack(void *state, FskMediaPlayerModule module, UInt32 index, void **track);
static FskErr readerMediaPlayerModuleTrackHasProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
static FskErr readerMediaPlayerModuleTrackSetProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr readerMediaPlayerModuleTrackGetProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr readerMediaPlayerModuleHasProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
static FskErr readerMediaPlayerModuleSetProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr readerMediaPlayerModuleGetProperty(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);

void readerMediaPlayerInitializeJumpTable(FskMediaPlayerModule module)
{
	module->doNew = readerMediaPlayerModuleNew;
	module->doDispose = readerMediaPlayerModuleDispose;
	module->doGetTime = readerMediaPlayerModuleGetTime;
	module->doSetTime = readerMediaPlayerModuleSetTime;
	module->doStart = readerMediaPlayerModuleStart;
	module->doStop = readerMediaPlayerModuleStop;
	module->doWillDraw = readerMediaPlayerModuleWillDraw;
	module->doUpdate = readerMediaPlayerModuleUpdate;
	module->doGetMetadata = readerMediaPlayerModuleGetMetadata;
	module->doGetVideoBitmap = readerMediaPlayerModuleGetVideoBitmap;
	module->doPropertyChanged = readerMediaPlayerModulePropertyChanged;
	module->doGetTrack = readerMediaPlayerModuleGetTrack;
	module->doTrackHasProperty = readerMediaPlayerModuleTrackHasProperty;
	module->doTrackSetProperty = readerMediaPlayerModuleTrackSetProperty;
	module->doTrackGetProperty = readerMediaPlayerModuleTrackGetProperty;
	module->doHasProperty = readerMediaPlayerModuleHasProperty;
	module->doSetProperty = readerMediaPlayerModuleSetProperty;
	module->doGetProperty = readerMediaPlayerModuleGetProperty;
}

FskErr readerMediaPlayerCanHandle(UInt32 dataSourceType, const char *mime, UInt32 flags)
{
	FskErr err = FskMediaReaderNew(NULL, mime, NULL, NULL, NULL, NULL);
#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY
	if (kFskErrNone != err) {
		if (0 == FskStrCompare(mime, "application/vnd.apple.mpegurl"))
			err = kFskErrNone;
	}
#endif
	return err;
}

FskErr readerMediaPlayerSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	FskErr err = FskMediaReaderSniffForMIME(data, dataSize, headers, uri, mime);
#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY
	if ((kFskErrNone != err) && data && dataSize) {
		if ((NULL != FskStrStr((const char*)data, "#EXT-X-STREAM-INF:PROGRAM-ID")) ||
			(NULL != FskStrStr((const char*)data, "#EXT-X-MEDIA-SEQUENCE:"))) {
			*mime = FskStrDoCopy("application/vnd.apple.mpegurl");
			err = kFskErrNone;
		}
	}
#endif
	return err;
}

enum {
	kFskMediaPlayerReaderUnknown = 0,
	kFskMediaPlayerReaderAudio = 1,
	kFskMediaPlayerReaderVideo = 2
};

#define ADJUST_SPATIAL_TEMPORAL_QUALITY
enum {
	kFskMediaPlayerReaderModeNormal = 0,
	kFskMediaPlayerReaderModeDropHalf = 1,
	kFskMediaPlayerReaderModeDropTwoThirds = 2,
	kFskMediaPlayerReaderModeOnlyKeys = 3
};

typedef struct readerMediaPlayerModuleRecord readerMediaPlayerModuleRecord;
typedef struct readerMediaPlayerModuleRecord *readerMediaPlayerModule;

typedef struct readerMediaPlayerTrackRecord readerMediaPlayerTrackRecord;
typedef struct readerMediaPlayerTrackRecord *readerMediaPlayerTrack;

typedef struct readerMediaPlayerQueueEntryRecord readerMediaPlayerQueueEntryRecord;
typedef struct readerMediaPlayerQueueEntryRecord *readerMediaPlayerQueueEntry;

struct readerMediaPlayerQueueEntryRecord {
	readerMediaPlayerQueueEntry		next;

	UInt32							useCount;
	FskInt64						maxDecodeTime;

	readerMediaPlayerTrack			track;

	unsigned char					*data;
	UInt32							sampleInfoCount;
	FskMediaReaderSampleInfoRecord	*sampleInfo;
};

typedef struct {
	readerMediaPlayerQueueEntry		entries;

	FskInt64						minTime;
	FskInt64						maxTime;
} readerMediaPlayerQueueRecord, *readerMediaPlayerQueue;

typedef struct readerMediaPlayerVideoFrameQueueRecord readerMediaPlayerVideoFrameQueueRecord;
typedef struct readerMediaPlayerVideoFrameQueueRecord *readerMediaPlayerVideoFrameQueue;

struct readerMediaPlayerVideoFrameQueueRecord {
	readerMediaPlayerVideoFrameQueue			next;

	readerMediaPlayerQueueEntry					entry;

	Boolean										decompressing;
	Boolean										decompressed;
	Boolean										releasedQueueEntry;
	Boolean										played;
	Boolean										doDrop;

	FskBitmap									bits;
	FskInt64									decodeTime;
	UInt32										duration;
	UInt32										compositionTimeOffset;
	FskInt64									compositionTime;
	FskInt64									compositionTimeReturned;
	unsigned char								*data;
	UInt32										dataSize;
	UInt32										frameType;
	UInt32										sequenceNumber;

	readerMediaPlayerTrack						track;
};

#define kVideoFrameHistoryCount 96
#define kBackToScreenCount		3;

typedef struct {
	FskInt64									compositionTime;

	Boolean										valid;
	Boolean										played;
} readerMediaPlayerVideoFrameHistoryRecord, *readerMediaPlayerVideoFrameHistory;

struct readerMediaPlayerTrackRecord {
	readerMediaPlayerTrack			next;

	readerMediaPlayerModule			state;
	FskMediaReaderTrack				readerTrack;

	UInt32							mediaFormat;

	FskErr							(*doEnqueuedData)(readerMediaPlayerQueueEntry entry);

	readerMediaPlayerQueueRecord	samples;

	Boolean							hasCrop;
	FskRectangleRecord				crop;

	Boolean							pendingUpdate;
	Boolean							atEnd;

	union {
		struct {
			FskSndChannel			sndChan;

			char					*format;
			UInt32					frequency;
			UInt16					channelCount;
			char					*formatInfo;
			UInt32					formatInfoSize;
			SInt32					samplesToSkip;
			UInt32					timeToBuffer;

			FskTimeCallBack			stop;
		} audio;

		struct {
			FskImageDecompress					deco;
			readerMediaPlayerVideoFrameQueue	frames;
			UInt32								framesAtDecompressor;
			readerMediaPlayerVideoFrameQueue	nextFrameToDraw;

			char								*format;
			FskMediaPropertyValueRecord			formatInfo;

			FskMutex							mutex;
			FskTimeCallBack						drawTimer;

			FskBitmap							bits;
			FskSampleTime						bitsTime;
            FskSampleTime                       lastWillDrawTime;

			Boolean								noDeco;
			Boolean								waitingForSync;
			UInt8								flushing;

			Boolean								hasMode;
			FskGraphicsModeParametersVideoRecord	mode;

			UInt32								sequenceNumber;
			UInt32								playMode;

			Boolean								directToScreenFailed;
			UInt8								canDirect;
			SInt32								windowRotation;

			readerMediaPlayerVideoFrameHistoryRecord
												history[kVideoFrameHistoryCount];
			UInt32								historyIndex;
			SInt32								last_requestedDrawingMethod;
			SInt32								this_requestedDrawingMethod;
			SInt32								back_to_screen_count;

			FskSampleTime						nextQualityAdjustTime;
			SInt32								target_temporal_quality;
			SInt32								integral_temporal_quality;
			SInt32								spatial_quality;
			Boolean								historyWaiting;
			FskSampleTime						historyBegins;
			SInt32								dropBudget;

			SInt32								rotation;
            UInt32                              maxFramesToQueue;

#if TARGET_OS_WIN32
			UInt32								updateSeed;
#endif
		} video;
	};

#if SUPPORT_RECORD
	FskMuxerTrack					muxerTrack;
	unsigned char					*holdData;
	FskMuxerSampleInfoRecord		holdInfo;
	FskInt64						holdDecodeTime;
#endif
};

typedef struct readerMediaPlayerNetBufferRecord readerMediaPlayerNetBufferRecord;
typedef struct readerMediaPlayerNetBufferRecord *readerMediaPlayerNetBuffer;

#define kBufferSize (32 * 1024)

struct readerMediaPlayerNetBufferRecord {
	readerMediaPlayerNetBuffer					next;

	FskInt64									position;
	UInt32										bytes;
	UInt32										lastUsed;

	Boolean										dirty;
	Boolean										large;

	UInt32										pad;	// ensure that data is long aligned

	unsigned char								data[kBufferSize];
};

typedef struct readerMediaPlayerFragmentRecord readerMediaPlayerFragmentRecord;
typedef struct readerMediaPlayerFragmentRecord *readerMediaPlayerFragment;

struct readerMediaPlayerFragmentRecord {
	readerMediaPlayerFragment					next;

	UInt32										sequenceNumber;
	SInt32										bandwidth;
	SInt32										programID;
	Boolean										reloadM3U;

	char										uri[1];
};

struct readerMediaPlayerModuleRecord {
	FskMediaPlayer				player;
	FskMediaPlayerModule		module;

	FskMediaReader				reader;

	char						*uri;

	FskMediaSpoolerRecord		spooler;

	FskTimeCallBack				refillTimer;
	FskSampleTime				maxTimeExtracted;
	FskSampleTime				bufferToTime;
	UInt32						refillInterval;
	FskTimeRecord				firstExtractClock;	// system time stamp of first extract after start
	FskSampleTime				firstExtractTime;	// stream time stamp of first extract after start
	FskTimeCallBack				bufferingCallback;
	FskThread					thread;
	FskTimeCallBack				emptyBuffersCallback;
	FskTimeCallBack				afterStopCallback;
	FskTimeCallBack				endCallback;

	UInt32						timeScale;
	double						playRate;
	Boolean						playing;
	UInt8						prerolling;
	Boolean						readerRunning;
	Boolean						readerIsForUpdate;
	Boolean						isNetwork;
	Boolean						spoolerOpen;
	Boolean						doRestart;
	Boolean						atEnd;
	Boolean						scrub;
	Boolean						lowBandwidth;
	Boolean						stalled;
	Boolean						usingGL;

	FskErr						error;
	FskErr						extractErr;

	FskTimeRecord				zeroMovieSystemTime;
	FskSampleTime				zeroMovieTime;

    UInt32                      minTimeToBuffer;		// units = ms
	UInt32						timeToBuffer;   		// units = ms
    
    UInt32                      bitRate;

	readerMediaPlayerTrack		tracks;

	FskRectangleRecord			clip;
	FskPort						port;
	SInt32						windowRotation;

	FskEvent					updateEvent;

	SInt32						pan;
	char						*eq;

	FskTimeRecord				atSystemTime;			// units = microseconds
	FskSampleTime				atMediaTime;			// units = readerMediaPlayerModuleRecord.timeScale
	FskSampleTime				lastMediaTime;			// atMediaTime from the last cycle through extractMoreCallback

	struct {
		FskFile						fref;
		FskInt64					position;

		UInt32						bufferCount;
		UInt32						bufferSize;
		UInt32						seed;
		readerMediaPlayerNetBuffer	buffers;

        Boolean                     keepLocal;
	} spool;

	struct {
		FskHTTPClient				client;
		FskHTTPClientRequest		request;

		Boolean						headersReceived;
		Boolean						suspended;
		Boolean						done;
		Boolean						isDownload;
		Boolean						isDownloadFailed;
		Boolean						requestHadPosition;
		Boolean						noSeeking;
		FskInt64					contentLength;
		FskInt64					position;
		FskInt64					lastRequestedPosition;


		char						*uri;			// may not match readerMediaPlayerModuleRecord.uri because of redirects

		char						*user;
		char						*password;
		char						*realm;

		char						*additionalHeaders;

		readerMediaPlayerNetBuffer	downloadBuffer;
		char						*downloadPath;
		SInt32						downloadPreference;
		FskInt64					downloadPosition;
		FskInt64					downloadDataSize;

        Boolean                     canDLNATimeSeek;
        double                      lastUseSeekTime;
	} http;

	struct {
		FskHTTPClient				m3uClient;
		FskHTTPClientRequest		m3uRequest;

		char						*m3u;
		UInt32						m3uBytes;
		Boolean						m3uDone;

		SInt32						bitRate;
		UInt32						sequenceNumber;

		FskHTTPClient				streamClient;
		FskHTTPClientRequest		streamRequest;

		readerMediaPlayerFragment	fragment;
	} frag;

#if SUPPORT_RECORD
	struct {
		Boolean						failed;
		char						*path;
		char						*mime;
		FskMuxer					muxer;
		FskFile						fref;
	} record;
#endif
};

static FskErr rmpFileSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr rmpFileSpoolerClose(FskMediaSpooler spooler);
static FskErr rmpFileSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr rmpFileSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size);

static FskErr rmpHTTPSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr rmpHTTPSpoolerClose(FskMediaSpooler spooler);
static FskErr rmpHTTPSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr rmpHTTPSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size);
static FskErr rmpHTTPSpoolerRestart(readerMediaPlayerModule state, FskInt64 position);

#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY
static FskErr rmpMPEGFragmentsSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr rmpMPEGFragmentsSpoolerClose(FskMediaSpooler spooler);
static FskErr rmpMPEGFragmentsSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr rmpMPEGFragmentsSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size);
#endif

static FskErr rmpNULLSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr rmpNULLSpoolerClose(FskMediaSpooler spooler);
static FskErr rmpNULLSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr rmpNULLSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size);

static FskErr rmpSpoolerFlushBuffer(FskMediaSpooler spooler, readerMediaPlayerNetBuffer buffer);
static FskErr getFreeBuffer(readerMediaPlayerModule state, readerMediaPlayerNetBuffer *buffer);
static FskErr allocateBuffers(readerMediaPlayerModule state, UInt32 count);
static void disposeBuffers(readerMediaPlayerModule state);
static void checkBufferAllocations(readerMediaPlayerModule state);
static void updateBuffersForBitrate(readerMediaPlayerModule state);

static FskErr readerMediaPlayerModuleMoreCallBack(FskSndChannel sndChan, void *refCon, SInt32 requestedSamples);
static void readerMediaPlayerModuleDoneCallBack(FskSndChannel sndChan, void *refCon, void *dataRefCon, Boolean played);
static void readerMediaPlayerModuleAbort(void *a, void *b, void *c, void *d);
static void readerMediaPlayerModuleAbortCallBack(FskSndChannel sndChan, void *refCon, FskErr err);

#define kRefillInterval (250)
#define kMinTimeToBuffer (2500)
const UInt32 kMinFramesToStart = 4;
const UInt32 kMaxFramesToQueue = 7;	//android used to be 35 to accomodate hw codec before GetMaxFramesToQueue() is implemented

static FskErr ensureReader(readerMediaPlayerModule state, double time);
static FskErr doUsingMediaPlayer(void *refCon, Boolean inUse);
static void extractMoreCallback(FskTimeCallBack callback, const FskTime time, void *param);
static void bufferingCallback(FskTimeCallBack callback, const FskTime time, void *param);
static Boolean flushQueues(readerMediaPlayerModule state);
static void flushQueue(readerMediaPlayerQueue queue);
static void usingQueueEntry(readerMediaPlayerQueueEntry entry);
static void doneWithQueueEntry(readerMediaPlayerQueueEntry entry);
static void disposeQueueEntry(readerMediaPlayerQueueEntry entry);

static void updateNaturalBounds(readerMediaPlayerModule state);
static void movieEnd(readerMediaPlayerModule state);
static void movieStop(FskTimeCallBack callback, const FskTime timeIn, void *param);
static void trackEnd(readerMediaPlayerTrack track);
static void audioStop(FskTimeCallBack callback, const FskTime timeIn, void *param);

static FskErr audioTrackEnqueuedData(readerMediaPlayerQueueEntry entry);

static FskErr queueAudioFrames(readerMediaPlayerTrack track, readerMediaPlayerQueueEntry entry);

static FskErr videoTrackEnqueuedData(readerMediaPlayerQueueEntry entry);
static FskErr decompressVideoFrames(readerMediaPlayerTrack track);
static void videoTrackDecompressComplete(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits);
static void flushVideoFrames(readerMediaPlayerTrack track);
static void scheduleVideoDrawTimer(readerMediaPlayerTrack track, FskSampleTime time);
static void videoDrawCallback(FskTimeCallBack callback, const FskTime time, void *param);
static Boolean syncVideoBitmap(readerMediaPlayerTrack track, FskSampleTime time, Boolean wait);
static void flushVideoBefore(readerMediaPlayerTrack track, const FskSampleTime *time);
static void sendUpdate(readerMediaPlayerModule state);
static void sendWarning(readerMediaPlayerModule state, const char *msg, const char *detail);
static void playerFailed(readerMediaPlayerModule state, FskErr err);
static void checkStart(readerMediaPlayerModule state);
static FskErr rotateBitmap(FskBitmap input, FskBitmap *output, SInt32 degrees);
static void afterStopCallback(FskTimeCallBack callback, const FskTime time, void *param);
static void emptyBuffersCallback(FskTimeCallBack callback, const FskTime time, void *param);

static FskErr readerMediaPlayerEventCallback(FskMediaReader reader, void *refCon, FskEventCodeEnum eventCode, FskEvent event);

#if SUPPORT_RECORD
	static FskErr recordExtracted(readerMediaPlayerModule state, FskMediaReaderTrack track, UInt32 infoCount, FskMediaReaderSampleInfo info, unsigned char *data);
#endif

static void clear_history( readerMediaPlayerTrack track );
static void update_history( readerMediaPlayerTrack track, readerMediaPlayerVideoFrameQueue frame );

FskErr readerMediaPlayerModuleNew(FskMediaPlayerModule module, const void *dataSource, UInt32 dataSourceType, const char *mime, FskMediaPlayerPropertyIdAndValue properties)
{
	FskErr err = kFskErrNone;
	readerMediaPlayerModule state;

	// allocate state
	err = FskMemPtrNewClear(sizeof(readerMediaPlayerModuleRecord), &state);
	BAIL_IF_ERR(err);

	module->state = state;
	state->player = module->player;
	state->module = module;
	state->thread = FskThreadGetCurrent();

	if (kFskMediaPlayerDataSourceFile == dataSourceType)
		state->uri = FskStrDoCat("file://", dataSource);
	else
	if (kFskMediaPlayerDataSourceHTTP == dataSourceType) {
		state->uri = FskStrDoCopy(dataSource);
		state->http.uri = state->uri;
	}
	else {
		err = kFskErrInvalidParameter;
		goto bail;
	}

	state->spooler.refcon = state;
#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY
	if (0 == FskStrCompare(mime, "application/vnd.apple.mpegurl")) {
		state->spooler.flags = kFskMediaSpoolerValid | kFskMediaSpoolerIsNetwork | kFskMediaSpoolerCantSeek;
		state->spooler.doOpen = rmpMPEGFragmentsSpoolerOpen;
		state->spooler.doClose = rmpMPEGFragmentsSpoolerClose;
		state->spooler.doRead = rmpMPEGFragmentsSpoolerRead;
		state->spooler.doGetSize = rmpMPEGFragmentsSpoolerGetSize;

		state->isNetwork = true;		//@@

		mime = "video/mpeg";
	}
	else
#endif
	if (0 == FskStrCompareWithLength("file://", state->uri, 7)) {
		state->spooler.flags = kFskMediaSpoolerValid;
		state->spooler.doOpen = rmpFileSpoolerOpen;
		state->spooler.doClose = rmpFileSpoolerClose;
		state->spooler.doRead = rmpFileSpoolerRead;
		state->spooler.doGetSize = rmpFileSpoolerGetSize;
	}
	else
	if ((0 == FskStrCompareWithLength("http://", state->uri, 7)) || (0 == FskStrCompareWithLength("https://", state->uri, 8))) {
		state->spooler.flags = kFskMediaSpoolerValid | kFskMediaSpoolerIsNetwork;
		state->spooler.doOpen = rmpHTTPSpoolerOpen;
		state->spooler.doClose = rmpHTTPSpoolerClose;
		state->spooler.doRead = rmpHTTPSpoolerRead;
		state->spooler.doGetSize = rmpHTTPSpoolerGetSize;

		state->isNetwork = true;
	}
	else {
		state->spooler.flags = kFskMediaSpoolerValid;
		state->spooler.doOpen = rmpNULLSpoolerOpen;
		state->spooler.doClose = rmpNULLSpoolerClose;
		state->spooler.doRead = rmpNULLSpoolerRead;
		state->spooler.doGetSize = rmpNULLSpoolerGetSize;

		state->isNetwork = true;			// assumption: unrecognized protocols are network based (e.g. mms, rtsp)
	}

	state->playRate = 1.0;
	FskRectangleSetFull(&state->clip);

	// set-up download parameters before instantiating so we can instantiate from downloaded data
	while (properties && properties->id) {
		switch (properties->id) {
			case kFskMediaPropertyDownloadPath:
				FskMemPtrDisposeAt(&state->http.downloadPath);
				state->http.downloadPath = FskStrDoCopy(properties->value.value.str);
				break;

			case kFskMediaPropertyDownloadPosition:
				state->http.downloadPosition = (FskInt64)properties->value.value.number;
				break;

			case kFskMediaPropertyDataSize:
				state->http.downloadDataSize = (FskInt64)properties->value.value.number;
				break;

			case kFskMediaPropertyDownload:
				state->http.downloadPreference = properties->value.value.integer;
				if (1 == (3 & state->http.downloadPreference))
					state->http.isDownload = true;
				break;

			case kFskMediaPropertyRequestHeaders:
				err = FskStrListDoCopy(properties->value.value.str, &state->http.additionalHeaders);
				BAIL_IF_ERR(err);
				break;

		}

		properties += 1;
	}

	err = FskMediaReaderNew(&state->reader, mime, state->uri, &state->spooler, readerMediaPlayerEventCallback, state);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSetOwner(state->reader, state->player);

	state->reader->doUsing = doUsingMediaPlayer;
	state->reader->doUsingRefCon = state;

#if SUPPORT_RECORD
	state->record.path = FskStrDoCopy("c:/record.mov");
	if (0 != FskStrCompare("application/sdp", mime))
		state->record.mime = FskStrDoCopy(mime);
	else
		state->record.mime = FskStrDoCopy("video/quicktime");
#endif

bail:
	if (kFskErrNone != err) {
		readerMediaPlayerModuleDispose(state, module);
		module->state = NULL;
	}

	return err;
}

void readerMediaPlayerModuleDispose(void *stateIn, FskMediaPlayerModule module)
{
	readerMediaPlayerModule state = stateIn;

	if (state) {
		FskTimeCallbackDispose(state->refillTimer);
		FskTimeCallbackDispose(state->bufferingCallback);
		FskTimeCallbackDispose(state->afterStopCallback);
		FskTimeCallbackDispose(state->emptyBuffersCallback);
		FskTimeCallbackDispose(state->endCallback);

		while (state->tracks) {
			readerMediaPlayerTrack track = FskListRemoveFirst((FskList*)(void*)(&state->tracks));
			if (kFskMediaPlayerReaderAudio == track->mediaFormat) {
				FskSndChannelDispose(track->audio.sndChan);
				FskMemPtrDispose(track->audio.format);
				FskMemPtrDispose(track->audio.formatInfo);
			}
			else
			if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
				flushVideoFrames(track);
				track->video.flushing += 1;
				FskImageDecompressDispose(track->video.deco);
				FskMutexDispose(track->video.mutex);
				FskTimeCallbackDispose(track->video.drawTimer);
				FskMediaPropertyEmpty(&track->video.formatInfo);
				FskMemPtrDispose(track->video.format);
				FskBitmapDispose(track->video.bits);
			}
			flushQueue(&track->samples);
			FskMemPtrDispose(track);
		}

		FskMediaReaderDispose(state->reader);
		FskHTTPClientDispose(state->http.client);
		disposeBuffers(state);
		FskMemPtrDispose(state->uri);
		if (state->uri != state->http.uri)
			FskMemPtrDispose(state->http.uri);
		FskMemPtrDispose(state->http.user);
		FskMemPtrDispose(state->http.password);
		FskMemPtrDispose(state->http.realm);
		FskMemPtrDispose(state->http.additionalHeaders);
		FskEventDispose(state->updateEvent);
		FskMemPtrDispose(state->eq);
		FskFileClose(state->spool.fref);

		if ((NULL != state->http.downloadPath) && (0 == (8 & state->http.downloadPreference)))
			FskFileDelete(state->http.downloadPath);
		FskMemPtrDispose(state->http.downloadPath);

#if SUPPORT_RECORD
		if (state->record.muxer)
			FskMuxerStop(state->record.muxer);
		FskMuxerDispose(state->record.muxer);
		FskFileClose(state->record.fref);
		FskMemPtrDispose(state->record.path);
		FskMemPtrDispose(state->record.mime);
#endif

		FskMemPtrDispose(state);
	}
}

FskErr readerMediaPlayerModuleSetTime(void *stateIn, FskMediaPlayerModule module, float scale, FskSampleTime time)
{
	readerMediaPlayerModule state = stateIn;
	Boolean needsUpdate;

	if (state->readerRunning) {
		FskMediaReaderStop(state->reader);
		state->readerRunning = false;
	}

	needsUpdate = flushQueues(state);
	if (needsUpdate) {
		if (state->scrub && state->isNetwork && !state->http.isDownload)
			;
		else
			sendUpdate(state);
	}

	if ((0 == time) && state->http.isDownloadFailed)
		rmpHTTPSpoolerClose(&state->spooler);

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleGetTime(void *stateIn, FskMediaPlayerModule module, float scale, FskSampleTime *time)
{
	readerMediaPlayerModule state = stateIn;

	if (module->playState < kFskMediaPlayerStatePlaying) {
		*time = (FskSampleTime)(0.5 + ((double)state->zeroMovieTime) / scale);
	}
	else {
		FskTimeRecord delta;
		double dt;
		FskTimeGetNow(&delta);
		FskTimeSub(&state->zeroMovieSystemTime, &delta);
		dt = (double)delta.seconds + (double)delta.useconds / 1000000.0;
		dt += (double)state->zeroMovieTime / (double)state->timeScale;
		*time = (FskSampleTime)(0.5 + (dt * scale));
	}

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleStart(void *stateIn, FskMediaPlayerModule module)
{
	readerMediaPlayerModule state = stateIn;
	FskErr err = kFskErrNone;
	readerMediaPlayerTrack track;
	FskMediaPropertyValueRecord property;
	UInt32 playerState = kFskMediaPlayerStateStopped;

	if (kFskErrNone != state->error)
		return kFskErrBadState;

	state->doRestart = false;
	state->atEnd = false;

	if (NULL != state->afterStopCallback)
		FskTimeCallbackRemove(state->afterStopCallback);

	if (NULL != state->emptyBuffersCallback)
		FskTimeCallbackRemove(state->emptyBuffersCallback);
	else
		FskTimeCallbackNew(&state->emptyBuffersCallback);

	if (state->readerRunning && state->readerIsForUpdate) {
		FskMediaReaderStop(state->reader);
		state->readerRunning = false;
		state->readerIsForUpdate = false;

		for (track = state->tracks; NULL != track; track = track->next) {
			if (kFskMediaPlayerReaderVideo == track->mediaFormat)
				flushVideoFrames(track);

			flushQueue(&track->samples);
		}
	}

	state->prerolling += 1;

	FskMediaPlayerGetTime(module->player, (float)state->timeScale, &state->zeroMovieTime);

	for (track = state->tracks; NULL != track; track = track->next) {
		track->atEnd = false;

		if (kFskMediaPlayerReaderAudio == track->mediaFormat) {
			if (0 == track->audio.format) {
				err = FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyFormat, &property);
				BAIL_IF_ERR(err);

				track->audio.format = property.value.str;

				err = FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertySampleRate, &property);
				BAIL_IF_ERR(err);

				track->audio.frequency = property.value.integer;

				err = FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyChannelCount, &property);
				BAIL_IF_ERR(err);

				track->audio.channelCount = (UInt16)property.value.integer;

				if (kFskErrNone == FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyFormatInfo, &property)) {
					track->audio.formatInfo = property.value.data.data;
					track->audio.formatInfoSize = property.value.data.dataSize;
				}
			}

			if (NULL == track->audio.sndChan) {
				if (kFskErrNone == FskSndChannelNew(&track->audio.sndChan, kFskAudioOutDefaultOutputID, kFskAudioFormatUndefined, 0)) {
					FskMediaPropertyValueRecord value, sr;

					FskInstrumentedItemSetOwner(track->audio.sndChan, state->player);

					FskSndChannelSetFormat(track->audio.sndChan, kFskAudioFormatUndefined, track->audio.format, track->audio.channelCount, track->audio.frequency, (unsigned char *)track->audio.formatInfo, track->audio.formatInfoSize);

					FskSndChannelSetMoreCallback(track->audio.sndChan, readerMediaPlayerModuleMoreCallBack, track);
					FskSndChannelSetDoneCallback(track->audio.sndChan, readerMediaPlayerModuleDoneCallBack, track);
					FskSndChannelSetAbortCallback(track->audio.sndChan, readerMediaPlayerModuleAbortCallBack, track);

					FskSndChannelSetVolume(track->audio.sndChan, module->volume);
					FskSndChannelSetPan(track->audio.sndChan, state->pan);
					value.type = kFskMediaPropertyTypeString;
					value.value.str = state->eq;
					FskSndChannelSetProperty(track->audio.sndChan, kFskMediaPropertyEQ, &value);

					track->audio.timeToBuffer = state->minTimeToBuffer;
					if (kFskErrNone == FskSndChannelGetProperty(track->audio.sndChan, kFskMediaPropertyBufferDuration, &value)) {
						if (kFskErrNone == FskSndChannelGetProperty(track->audio.sndChan, kFskMediaPropertySampleRate, &sr)) {
							UInt32 ms = (UInt32)(1000.0 * (double)value.value.integer / sr.value.number);
							if (ms > track->audio.timeToBuffer)
								track->audio.timeToBuffer = ms;
						}
					}
				}
			}

			if (NULL != track->audio.sndChan) {
				property.type = kFskMediaPropertyTypeFloat;
				property.value.number = state->playRate;
				FskSndChannelSetProperty(track->audio.sndChan, kFskMediaPropertyPlayRate, &property);
			}
		}
		else if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
			readerMediaPlayerVideoFrameQueue frame;

			if (NULL == track->video.drawTimer) {
				FskTimeCallbackNew(&track->video.drawTimer);
				if (NULL == track->video.drawTimer) {
					err = kFskErrUnknown;
					goto bail;
				}
				FskInstrumentedItemSetOwner(track->video.drawTimer, state->player);
			}

			clear_history(track);

			for (frame = track->video.frames; NULL != frame; frame = frame->next) {
				if (frame->decompressed)
					continue;

				frame->doDrop = false;
			}
		}
	}

	state->timeToBuffer = state->minTimeToBuffer;
	if ((kFskErrNone == FskMediaReaderGetProperty(state->reader, kFskMediaPropertyBufferDuration, &property)) && (kFskMediaPropertyTypeInteger == property.type))
		state->timeToBuffer = (UInt32)(1000.0 * ((float)property.value.integer) / ((float)state->timeScale));

	if ((state->lowBandwidth && (0 == state->firstExtractClock.seconds)) || state->stalled)
		state->timeToBuffer *= 2;			// if low bandwidth state on last stop, use bigger buffers this time around

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderAudio != track->mediaFormat)
			continue;

		if (track->audio.timeToBuffer > state->timeToBuffer)
			state->timeToBuffer = track->audio.timeToBuffer;
	}

	state->bufferToTime = state->zeroMovieTime + (UInt32)(((double)state->timeToBuffer / 1000.0) * (double)state->timeScale);
	state->refillInterval = kRefillInterval;

	if ((-1 != state->module->duration) && (state->bufferToTime > state->module->duration))
		state->bufferToTime = state->module->duration;

	if (NULL == state->refillTimer) {
		FskTimeCallbackNew(&state->refillTimer);
		if (NULL == state->refillTimer) {
			err = kFskErrUnknown;
			goto bail;
		}
		FskInstrumentedItemSetOwner(state->refillTimer, state->player);
	}

	err = ensureReader(state, (double)state->zeroMovieTime);
	BAIL_IF_ERR(err);

	if (state->maxTimeExtracted < state->bufferToTime) {
		playerState = kFskMediaPlayerStateBuffering;
		goto bail;
	}

	// wait for initial queues of video frames to be decompressed
	while (true) {
		readerMediaPlayerVideoFrameQueue frame;
		Boolean ready = true;

		for (track = state->tracks; NULL != track; track = track->next) {
			UInt32 processed_total = 0;

			if (kFskMediaPlayerReaderVideo != track->mediaFormat)
				continue;

			for (frame = track->video.frames; NULL != frame; frame = frame->next) {
				if (frame->decompressing) {
					ready = false;
					break;
				}
				else
				if (frame->decompressed) {
					if ((frame->next && (frame->next->compositionTime < state->zeroMovieTime)) ||
						(!frame->next && frame->duration && ((frame->compositionTime + frame->duration) < state->zeroMovieTime))) {
						flushVideoBefore(track, &state->zeroMovieTime);
						decompressVideoFrames(track);
						ready = false;
						break;
					}

					processed_total++;
					if (processed_total >= kMinFramesToStart) {
						ready = true;
						break;
					}
				}
			}
		}

		if (ready)
			break;

		FskThreadYield();
	}

	if (state->error) {	// error in preroll. surrender.
		playerFailed(state, state->error);
		goto bail;
	}

	FskTimeCallbackScheduleFuture(state->refillTimer, 0, state->refillInterval, extractMoreCallback, state);

	FskTimeGetNow(&state->zeroMovieSystemTime);

	state->playing = true;
	state->lastMediaTime = -1;

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderAudio == track->mediaFormat) {
			if (NULL != track->audio.sndChan) {
				FskSampleTime atTime;

				while (track->samples.entries && track->samples.entries->next && track->samples.entries->next->sampleInfo && (track->samples.entries->next->sampleInfo->decodeTime < state->zeroMovieTime)) {
					readerMediaPlayerQueueEntry entry = track->samples.entries;
					FskListRemoveFirst((FskList*)(void*)(&track->samples.entries));
					disposeQueueEntry(entry);
				}

				track->audio.samplesToSkip = 0;
				err = kFskErrNone;
				if (NULL != track->samples.entries) {
					if (track->samples.entries->sampleInfo && (track->samples.entries->sampleInfo[0].decodeTime < state->zeroMovieTime)) {
						track->audio.samplesToSkip = (UInt32)(state->zeroMovieTime - (track->samples.entries->sampleInfo[0].decodeTime));
						track->audio.samplesToSkip = (UInt32)((((double)track->audio.samplesToSkip) / (double)state->timeScale) * (double)track->audio.frequency);
					}

					err = queueAudioFrames(track, track->samples.entries);
					if ((kFskErrNone == err) && track->atEnd)
						FskSndChannelEnqueue(track->audio.sndChan, (void *)-1, 0, 0, 0, NULL, NULL);			// flush
				}

				FskMediaPlayerGetTime(module->player, (float)track->audio.frequency, &atTime);
				if (kFskErrNone == err)
					err = FskSndChannelStart(track->audio.sndChan, atTime);
				if (err) {
					if (module->sndChan == track->audio.sndChan) {
						FskMediaPlayerDetachSoundClock(state->player);
                        module->sndChan = NULL;
                    }
					FskSndChannelDispose(track->audio.sndChan);
					track->audio.sndChan = NULL;
					if (kFskErrCodecNotFound == err)
						sendWarning(state, "unsupported.codec.audio", track->audio.format);
					else
						sendWarning(state, "unsupported.audio", NULL);

					err = kFskErrNone;
				}
				else {
					if (NULL != track->audio.sndChan)
						FskSndChannelSetVolume(track->audio.sndChan, module->volume);

					if (!track->atEnd && !state->atEnd) {
						//@@ not correct when there are multiple audio tracks
						module->sndChan = track->audio.sndChan;
 						module->useSoundChannelForTime = true;
					}
				}
			}
		}
		else if (kFskMediaPlayerReaderVideo == track->mediaFormat)
			scheduleVideoDrawTimer(track, state->zeroMovieTime);
	}

	playerState = kFskMediaPlayerStatePlaying;

bail:
	FskMediaPlayerSetPlayState(module->player, playerState);

	state->prerolling -= 1;

	return err;
}

FskErr readerMediaPlayerModuleStop(void *stateIn, FskMediaPlayerModule module)
{
	readerMediaPlayerModule state = stateIn;
	readerMediaPlayerTrack track;

	state->playing = false;
	state->doRestart = false;

	state->firstExtractClock.seconds = 0;
	state->firstExtractTime = 0;

    FskMediaPlayerSetPlayState(module->player, kFskMediaPlayerStateStopped);

	if (NULL != state->refillTimer)
		FskTimeCallbackRemove(state->refillTimer);

	if (NULL != state->bufferingCallback)
		FskTimeCallbackRemove(state->bufferingCallback);

	if (NULL != state->endCallback) {
		FskTimeCallbackDispose(state->endCallback);
		state->endCallback = NULL;
	}

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderAudio == track->mediaFormat) {
			if (NULL != track->audio.sndChan)
				FskSndChannelStop(track->audio.sndChan);
			if (track->audio.stop) {
				FskTimeCallbackDispose(track->audio.stop);
				track->audio.stop = NULL;
			}
		}
		else
		if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
			if (NULL != track->video.drawTimer)
				FskTimeCallbackRemove(track->video.drawTimer);
		}
	}

	// ideally this should be done in consultation with the media reader
	if (state->isNetwork && state->spoolerOpen && (state->spooler.flags & kFskMediaSpoolerCantSeek))
		;
	else
	if (state->readerRunning && !state->http.isDownload && !state->http.isDownloadFailed && ((false == state->spoolerOpen) || (-1 == state->module->duration)))		{	// our spoolers can pause, but we can't assume media readers than manage their own i/o can do the same. also we have to do a full stop (rather than implicit suspend) on a live stream, since data will continue to flow.
		FskMediaReaderStop(state->reader);
		state->readerRunning = false;

		flushQueues(state);

		if (state->isNetwork && !state->http.isDownload && !state->http.isDownloadFailed)
			(state->spooler.doClose)(&state->spooler);
	}

	if (NULL == state->afterStopCallback)
		FskTimeCallbackNew(&state->afterStopCallback);
	FskTimeCallbackScheduleFuture(state->afterStopCallback, 1, 000, afterStopCallback, state);

	return kFskErrNone;
}

Boolean readerMediaPlayerModuleWillDraw(void *stateIn, FskMediaPlayerModule module, FskSampleTime beforeTime)
{
	readerMediaPlayerModule state = stateIn;
    readerMediaPlayerTrack track;
    Boolean result = false;

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderVideo != track->mediaFormat)
			continue;

        syncVideoBitmap(track, beforeTime, false);

        if (track->video.lastWillDrawTime != track->video.bitsTime) {
            track->video.lastWillDrawTime = track->video.bitsTime;
            result = true;
        }
    }
 
    return result;
}

FskErr readerMediaPlayerModuleUpdate(void *stateIn, FskMediaPlayerModule module)
{
	readerMediaPlayerModule state = stateIn;
	readerMediaPlayerTrack track;
	FskErr err = kFskErrNone;
	SInt32 originX, originY;
	FskSampleTime time;
	FskWindow window = (kFskMediaPlayerDrawingMethodDirect == module->requestedDrawingMethod) ? module->window : NULL;
	FskPort port = state->port;
	Boolean didDraw = false;
	Boolean doCheckStart = false;

	if (NULL == port)
		return kFskErrNone;

	FskPortGetOrigin(port, &originX, &originY);
	FskPortSetOrigin(port, 0, 0);

	FskMediaPlayerGetTime(module->player, (float)state->timeScale, &time);

	for (track = state->tracks; NULL != track; track = track->next) {

		if (kFskMediaPlayerReaderVideo != track->mediaFormat)
			continue;

		if (!(state->scrub && state->isNetwork && !state->http.isDownload) && (track->pendingUpdate || (NULL == track->video.bits))) {
			if (false == state->readerRunning) {
				err = ensureReader(state, (double)time);
				BAIL_IF_ERR(err);

				state->readerIsForUpdate = true;
			}

			doCheckStart = true;

			if (syncVideoBitmap(track, time, !state->playing))
				track->pendingUpdate = false;
		}
		if (NULL == track->video.bits)
			continue;

		if ((NULL == window) || track->video.directToScreenFailed || !track->video.canDirect) {
			FskRectangleRecord saveClip;
			UInt32 saveMode;
			FskGraphicsModeParameters saveModeParams;

			rotateBitmap(track->video.bits, &track->video.bits, 360 - track->video.bits->rotation);

            FskPortBeginDrawing(port, NULL);

			FskPortGetClipRectangle(port, &saveClip);
			FskPortSetClipRectangle(port, &state->clip);

			FskPortGetGraphicsMode(port, &saveMode, &saveModeParams);
			FskPortSetGraphicsMode(port, kFskGraphicsModeCopy | (state->usingGL ? kFskGraphicsModeBilinear : 0), track->video.hasMode ? &track->video.mode.header : NULL);
			if (180 != track->video.rotation)
				FskPortBitmapDraw(port, track->video.bits, track->hasCrop ? &track->crop : NULL, &module->windowBounds);
			else {
				FskScaleOffset scaleOffset;
				FskRectangle srcRect, dstRect;

				srcRect = track->hasCrop ? &track->crop : &track->video.bits->bounds;
				dstRect = &module->windowBounds;
				scaleOffset.scaleX = FskFixedNDiv(dstRect->width  - 1, srcRect->width  - 1, kFskScaleBits);
				scaleOffset.scaleY = FskFixedNDiv(dstRect->height - 1, srcRect->height - 1, kFskScaleBits);
				scaleOffset.offsetX = dstRect->x << kFskOffsetBits;
				scaleOffset.offsetY = dstRect->y << kFskOffsetBits;

				//@@ should only be flipping Y here. this is a hack... we need another property to describe flip... rotation by itself isn't enough.
				scaleOffset.scaleX = -scaleOffset.scaleX;
				scaleOffset.offsetX += (dstRect->width - 1) << kFskOffsetBits;

				scaleOffset.scaleY = -scaleOffset.scaleY;
				scaleOffset.offsetY += (dstRect->height - 0) << kFskOffsetBits;
				FskPortBitmapScaleOffset(port, track->video.bits, track->hasCrop ? &track->crop : NULL, &scaleOffset);
			}

			FskPortSetGraphicsMode(port, saveMode, saveModeParams);
			FskMemPtrDispose(saveModeParams);

			FskPortSetClipRectangle(port, &saveClip);

            FskPortEndDrawing(port);

			if (0 != track->video.windowRotation) {
				FskMediaPropertyValueRecord rotation;
				rotation.type = kFskMediaPropertyTypeFloat;
				rotation.value.number = 0;
				FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyRotation, &rotation);

				track->video.windowRotation = 0;
			}

			didDraw = true;
		}
		else {
			FskRectangleRecord src, dst;

#if TARGET_OS_ANDROID
			// android correctly rotates framebuffer for us
#else
			SInt32 rotation;

			FskWindowGetRotation(state->module->window, NULL, &rotation);
			if (track->video.bits->rotation != rotation)
				rotateBitmap(track->video.bits, &track->video.bits, ((360 - track->video.bits->rotation) + rotation) % 360);
#endif

			if (track->hasCrop) {
				src = track->crop;

				if ((90 == track->video.bits->rotation) || (270 == track->video.bits->rotation)) {
					SInt32 t = src.width;
					src.width = src.height;
					src.height = t;
				}
			}
			else
				FskBitmapGetBounds(track->video.bits, &src);

			dst = module->windowBounds;
			FskPortRectScale(module->port, &dst);

			didDraw = (kFskErrNone == FskWindowCopyBitsToWindow(window, track->video.bits, &src, &dst, kFskGraphicsModeCopy, track->video.hasMode ? &track->video.mode.header : NULL));

			if (didDraw && (state->windowRotation != track->video.windowRotation)) {
				FskMediaPropertyValueRecord rotation;

				track->video.windowRotation = state->windowRotation;

				rotation.type = kFskMediaPropertyTypeFloat;
				rotation.value.number = (double)state->windowRotation;
				FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyRotation, &rotation);
			}
		}
	}

	if (false == didDraw)
		err = kFskErrNothingRendered;

	if ((kFskMediaPlayerStateStopped == state->module->playState) && state->readerRunning && state->readerIsForUpdate) {
		Boolean pendingUpdate = false;

		for (track = state->tracks; NULL != track; track = track->next) {
			if ((kFskMediaPlayerReaderVideo == track->mediaFormat) && track->pendingUpdate)
				pendingUpdate = track->pendingUpdate;
		}

		if (!pendingUpdate) {
			FskMediaReaderStop(state->reader);
			state->readerRunning = false;

			for (track = state->tracks; NULL != track; track = track->next) {
				if (kFskMediaPlayerReaderVideo == track->mediaFormat)
					flushVideoFrames(track);

				flushQueue(&track->samples);
			}
		}
	}

	// we might have buffered up more data in the call to ensureReader (extract) - enough that we can start now
	if (doCheckStart && (kFskMediaPlayerStateBuffering == state->module->playState))
		checkStart(state);

bail:
	FskPortSetOrigin(module->port, originX, originY);

	return err;
}

FskErr readerMediaPlayerModuleGetMetadata(void *stateIn, FskMediaPlayerModule module, const char *metaDataType, UInt32 index, FskMediaPropertyValue meta, UInt32 *flags)
{
	readerMediaPlayerModule state = stateIn;
	return FskMediaReaderGetMetadata(state->reader, metaDataType, index, meta, flags);
}

FskErr readerMediaPlayerModuleGetVideoBitmap(void *stateIn, FskMediaPlayerModule module, UInt32 width, UInt32 height, FskBitmap *bitsOut)
{
	readerMediaPlayerModule state = stateIn;
	FskErr err = kFskErrNone;
	FskSampleTime time;
	FskBitmap bits = NULL;
	FskRectangleRecord r, natural;
	readerMediaPlayerTrack track;

	FskMediaPlayerGetTime(module->player, (float)state->timeScale, &time);

	if (-1 != state->module->duration) {
		err = ensureReader(state, (double)time);
		BAIL_IF_ERR(err);
	}

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
			syncVideoBitmap(track, time, !state->playing);
			if (NULL != track->video.bits) {
				rotateBitmap(track->video.bits, &track->video.bits, 360 - track->video.bits->rotation);
				bits = track->video.bits;
				break;
			}
		}
	}

	if (NULL == bits) {
		err = kFskErrBadState;
		goto bail;
	}

	FskBitmapGetBounds(bits, &natural);
	if (width && height && ((SInt32)width < natural.width) && ((SInt32)height < natural.height)) {		// only scale down
		FskRectangleSet(&r, 0, 0, width, height);
		FskRectangleScaleToFit(&r, &natural, &r);
		r.x = r.y = 0;
	}
	else
		r = natural;

	err = FskBitmapNew(r.width, r.height, kFskBitmapFormatDefault, bitsOut);
	BAIL_IF_ERR(err);

	if (180 != track->video.rotation)
		err = FskBitmapDraw(bits, NULL, *bitsOut, &r, NULL, NULL, kFskGraphicsModeCopy, NULL);
	else {
		FskScaleOffset scaleOffset;
		FskRectangle srcRect, dstRect;

		srcRect = &natural;
		dstRect = &r;
		scaleOffset.scaleX = FskFixedNDiv(dstRect->width  - 0, srcRect->width  - 0, kFskScaleBits);
		scaleOffset.scaleY = FskFixedNDiv(dstRect->height - 0, srcRect->height - 0, kFskScaleBits);
		scaleOffset.offsetX = scaleOffset.offsetY = 0;

		//@@ should only be flipping Y here. this is a hack... we need another property to describe flip... rotation by itself isn't enough.
		scaleOffset.scaleX = -scaleOffset.scaleX;
		scaleOffset.offsetX = FskIntToFixed(dstRect->width);

		scaleOffset.scaleY = -scaleOffset.scaleY;
		scaleOffset.offsetY = FskIntToFixed(dstRect->height);
		FskScaleOffsetBitmap(bits, srcRect, *bitsOut, dstRect,
							 &scaleOffset, NULL, kFskGraphicsModeCopy, NULL);
	}
	if (err) {
		FskBitmapDispose(*bitsOut);
		bitsOut = NULL;
		goto bail;
	}

bail:
	return err;
}

FskErr readerMediaPlayerModulePropertyChanged(void *stateIn, FskMediaPlayerModule module, UInt32 property)
{
	readerMediaPlayerModule state = stateIn;
	readerMediaPlayerTrack track;

	if (kFskMediaPlayerPropertyBounds == property) {
		FskEventDispose(state->updateEvent);
		state->updateEvent = NULL;

		for (track = state->tracks; NULL != track; track = track->next) {
			if (kFskMediaPlayerReaderVideo == track->mediaFormat)
				track->video.directToScreenFailed = false;
		}
	}
	else
	if (kFskMediaPlayerPropertyWindow == property) {
		state->usingGL = module->port ? FskBitmapIsOpenGLDestinationAccelerated(module->port->bits) : false;
		if (state->port != module->port) {
			Boolean reschedule = (NULL == state->port) && (NULL != module->port) && (state->module->playState >= kFskMediaPlayerStatePlaying);

			for (track = state->tracks; NULL != track; track = track->next) {
				if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
					clear_history(track);
					track->video.directToScreenFailed = false;
				}
			}

			state->port = module->port;
			if (reschedule) {
				FskSampleTime time;

				FskMediaPlayerGetTime(module->player, (float)state->timeScale, &time);
				for (track = state->tracks; NULL != track; track = track->next) {
					if (kFskMediaPlayerReaderVideo == track->mediaFormat)
						scheduleVideoDrawTimer(track, time);
				}
			}

			if (NULL == state->port) {
				for (track = state->tracks; NULL != track; track = track->next) {
					if ((kFskMediaPlayerReaderVideo == track->mediaFormat) && (NULL != track->video.deco)) {
						// stop the decompressor
						track->video.flushing += 1;
						FskImageDecompressFlush(track->video.deco);
						track->video.flushing -= 1;
					}
				}
			}
		}
	}

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleGetTrack(void *stateIn, FskMediaPlayerModule module, UInt32 index, void **trackOut)
{
	readerMediaPlayerModule state = stateIn;
	readerMediaPlayerTrack track;

	for (track = state->tracks; NULL != track; track = track->next) {
		if (0 == index--)
			break;
	}

	*trackOut = track;

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleTrackHasProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	readerMediaPlayerTrack track = trackIn;

	switch (propertyID) {
		case kFskMediaPropertyCrop:
		case kFskMediaPropertyContrast:
		case kFskMediaPropertyBrightness:
		case kFskMediaPropertySprites:
			*get = *set = true;
			if (kFskMediaPropertySprites == propertyID)
				*dataType = kFskMediaPropertyTypeSprites;
			else
				*dataType = (kFskMediaPropertyCrop == propertyID) ? kFskMediaPropertyTypeRectangle : kFskMediaPropertyTypeFloat;
			return (kFskMediaPlayerReaderVideo == track->mediaFormat) ? kFskErrNone : kFskErrUnimplemented;

		default:
			return FskMediaReaderTrackHasProperty(track->readerTrack, propertyID, get, set, dataType);
	}

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleTrackSetProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, FskMediaPropertyValue property)
{
	readerMediaPlayerModule state = stateIn;
	readerMediaPlayerTrack track = trackIn;
	FskErr err = kFskErrNone;

	switch (propertyID) {
		case kFskMediaPropertyCrop:
			if (kFskMediaPropertyTypeRectangle != property->type)
				return kFskErrInvalidParameter;
			track->hasCrop = true;
			track->crop = property->value.rect;
			break;

		case kFskMediaPropertyContrast:
		case kFskMediaPropertyBrightness: {
			FskFixed f;

			if ((kFskMediaPropertyTypeFloat != property->type) || (kFskMediaPlayerReaderVideo != track->mediaFormat))
				return kFskErrInvalidParameter;
			f = FskRoundFloatToFixed(property->value.number);
			if (kFskMediaPropertyContrast == propertyID)
				track->video.mode.contrast = f;
			else
				track->video.mode.brightness = f;

			track->video.hasMode = (0 != track->video.mode.contrast) || (0 != track->video.mode.brightness) || (0 != track->video.mode.sprites);
			sendUpdate(state);
			}
			break;

		case kFskMediaPropertySprites:
			track->video.mode.sprites = property->value.sprites;
			track->video.hasMode = (0 != track->video.mode.contrast) || (0 != track->video.mode.brightness) || (0 != track->video.mode.sprites);
			sendUpdate(state);
			break;

		default:
			err = FskMediaReaderTrackSetProperty(track->readerTrack, propertyID, property);
			break;

	}

	return err;
}

FskErr readerMediaPlayerModuleTrackGetProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, FskMediaPropertyValue property)
{
	readerMediaPlayerTrack track = trackIn;

	switch (propertyID) {
		case kFskMediaPropertyCrop:
			if (!track->hasCrop)
				return kFskErrBadState;

			property->type = kFskMediaPropertyTypeRectangle;
			property->value.rect = track->crop;
			break;

		default:
			return FskMediaReaderTrackGetProperty(track->readerTrack, propertyID, property);
	}

	return kFskErrNone;
}

FskErr readerMediaPlayerModuleHasProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	readerMediaPlayerModule state = stateIn;
	FskErr err = kFskErrNone;

	switch (propertyID) {
		case kFskMediaPropertyPlayRate:
			if (get) *get = false;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeFloat;
			break;

		case kFskMediaPropertyClip:
			if (get) *get = true;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeRectangle;
			break;

		case kFskMediaPropertyDuration:
			if (get) *get = false;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeFloat;
			break;

		case kFskMediaPropertyBalance:
			if (get) *get = true;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeFloat;
			break;

		case kFskMediaPropertyBuffer:
			if (get) *get = true;
			if (set) *set = false;
			*dataType = kFskMediaPropertyTypeFloat;
			break;

		case kFskMediaPropertyEQ:
			if (get) *get = false;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeString;
			break;

		case kFskMediaPropertyPixelFormat:
			if (get) *get = true;
			if (set) *set = false;
			*dataType = kFskMediaPropertyTypeUInt32List;
			break;

		case kFskMediaPropertyAuthentication:
			if (state->isNetwork) {
				if (get) *get = true;
				if (set) *set = true;
				*dataType = kFskMediaPropertyTypeStringList;
			}
			else
				err = FskMediaReaderHasProperty(state->reader, propertyID, get, set, dataType);
			break;

		case kFskMediaPropertyError:
			if (get) *get = true;
			if (set) *set = false;
			*dataType = kFskMediaPropertyTypeInteger;
			break;

		case kFskMediaPropertyScrub:
			if (get) *get = false;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeBoolean;
			break;

		case kFskMediaPropertyBitRate:
			FskMediaReaderHasProperty(state->reader, propertyID, get, set, dataType);
			err = kFskErrNone;
			*set = true;
			*dataType = kFskMediaPropertyTypeInteger;
			break;

		case kFskMediaPropertyRequestHeaders:
			if (get) *get = true;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeStringList;
			break;

		case kFskMediaPropertyLocation:
			if (get) *get = true;
			if (set) *set = false;
			*dataType = kFskMediaPropertyTypeString;
			break;

        case kFskMediaPropertyBufferDuration:
			if (get) *get = true;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeInteger;
			break;

        case kFskMediaPropertyLocal:
			if (get) *get = false;
			if (set) *set = true;
			*dataType = kFskMediaPropertyTypeBoolean;
			break;

		default:
			err = FskMediaReaderHasProperty(state->reader, propertyID, get, set, dataType);
			break;
	}

	return err;
}

FskErr readerMediaPlayerModuleSetProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, FskMediaPropertyValue property)
{
	readerMediaPlayerModule state = stateIn;
	FskErr err = kFskErrNone;
	readerMediaPlayerTrack track;
	FskMediaPropertyValueRecord p;

	switch (propertyID) {
		case kFskMediaPropertyPlayRate:
			if (kFskMediaPropertyTypeFloat != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
			state->playRate = property->value.number;
			break;


		case kFskMediaPropertyClip:
			if (kFskMediaPropertyTypeRectangle != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
			state->clip = property->value.rect;
			break;

		case kFskMediaPropertyDuration:
			if (kFskMediaPropertyTypeFloat != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
			// value coming in is in seconds, but media reader expects units of time scale (durationScale)
			state->module->duration = (FskSampleTime)(property->value.number * state->module->durationScale);
			p.type = kFskMediaPropertyTypeFloat;
			p.value.number = (double)state->module->duration;
			FskMediaReaderSetProperty(state->reader, propertyID, &p);
			break;

		case kFskMediaPropertyBalance:
			if (kFskMediaPropertyTypeFloat != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
			if (property->value.number < -1.0)
				state->pan = -256;
			else if (property->value.number > 1.0)
				state->pan = 256;
			else
				state->pan = (SInt32)(property->value.number * 256);

			for (track = state->tracks; NULL != track; track = track->next) {
				if ((kFskMediaPlayerReaderAudio == track->mediaFormat) && (track->audio.sndChan != NULL))
					FskSndChannelSetPan(track->audio.sndChan, state->pan);
			}
			break;

		case kFskMediaPropertyEQ:
			FskMemPtrDisposeAt((void**)(void*)(&state->eq));
			if ((NULL != property->value.str) && (0 != *property->value.str))
				state->eq = FskStrDoCopy(property->value.str);
			for (track = state->tracks; NULL != track; track = track->next) {
				if ((kFskMediaPlayerReaderAudio == track->mediaFormat) && (track->audio.sndChan != NULL))
					FskSndChannelSetProperty(track->audio.sndChan, kFskMediaPropertyEQ, property);
			}
			break;

		case kFskMediaPropertyAuthentication:
			if (state->isNetwork) {
				if (FskStrListLen(property->value.str) < 2) {
					err = kFskErrInvalidParameter;
					goto bail;
				}
				FskMemPtrDispose(state->http.user);
				FskMemPtrDispose(state->http.password);
				state->http.user = FskStrDoCopy(property->value.str);
				state->http.password = FskStrDoCopy(property->value.str + FskStrLen(property->value.str) + 1);

				if (state->http.client && (state->http.user || state->http.password))
					FskHTTPClientSetCredentials(state->http.client, state->http.user, state->http.password, 0, kFskHTTPAuthCredentialsTypeString);
			}
			else
				err = FskMediaReaderSetProperty(state->reader, propertyID, property);
			break;

		case kFskMediaPropertyScrub:
			err = FskMediaReaderSetProperty(state->reader, propertyID, property);
			if (kFskErrUnimplemented == err)
				err = kFskErrNone;

			if (kFskMediaPropertyTypeBoolean == property->type) {
				Boolean scrub = property->value.b;

				if (scrub != state->scrub) {
					state->scrub = scrub;

					if (!scrub) {
						for (track = state->tracks; NULL != track; track = track->next) {
							if (kFskMediaPlayerReaderVideo == track->mediaFormat)
								track->pendingUpdate = true;
						}
						sendUpdate(state);
					}
				}
			}
			break;

		case kFskMediaPropertyBitRate:
			state->frag.bitRate = property->value.integer;
			err = FskMediaReaderSetProperty(state->reader, propertyID, property);
			if (kFskErrUnimplemented == err)
				err = kFskErrNone;
			break;

		case kFskMediaPropertyRequestHeaders:
			err = FskStrListDoCopy(property->value.str, &state->http.additionalHeaders);
			BAIL_IF_ERR(err);
			break;

		case kFskMediaPropertyHibernate:
			if (property->value.b) {
				for (track = state->tracks; NULL != track; track = track->next) {
					if ((kFskMediaPlayerReaderAudio == track->mediaFormat) && (NULL != track->audio.sndChan)) {
						FskMediaPropertyValueRecord value;

						value.type = kFskMediaPropertyTypeBoolean;
						value.value.b = true;
						FskSndChannelSetProperty(track->audio.sndChan, kFskMediaPropertyHibernate, &value);
					}
				}
			}
			break;

		case kFskMediaPropertyBufferDuration:
			if (kFskMediaPropertyTypeInteger != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
			state->minTimeToBuffer = property->value.integer;
			break;

        case kFskMediaPropertyLocal:
			if (kFskMediaPropertyTypeBoolean != property->type) {
				err = kFskErrInvalidParameter;
				goto bail;
			}
            state->spool.keepLocal = property->value.b;
            checkBufferAllocations(state);
            break;

		default:
			err = FskMediaReaderSetProperty(state->reader, propertyID, property);
			break;
	}

bail:
	return err;
}

FskErr readerMediaPlayerModuleGetProperty(void *stateIn, FskMediaPlayerModule module, void *trackIn, UInt32 propertyID, FskMediaPropertyValue property)
{
	readerMediaPlayerModule state = stateIn;
	FskErr err = kFskErrNone;

	switch (propertyID) {
		case kFskMediaPropertyDuration:
			err = FskMediaReaderGetProperty(state->reader, propertyID, property);
			// media reader returns time in units of durationScale. media player units are seconds.
			if ((kFskErrNone == err) && (kFskMediaPropertyTypeFloat == property->type) && (-1.0 != property->value.number))
				property->value.number /= state->module->durationScale;
			break;

		case kFskMediaPropertyPlayRate:
			property->type = kFskMediaPropertyTypeFloat;
			property->value.number = state->playRate;
			break;

		case kFskMediaPropertyClip:
			property->type = kFskMediaPropertyTypeRectangle;
			property->value.rect = state->clip;
			break;

		case kFskMediaPropertyBalance:
			property->type = kFskMediaPropertyTypeFloat;
			property->value.number = state->pan;
			break;

		case kFskMediaPropertyBuffer:
			property->type = kFskMediaPropertyTypeFloat;
			if (state->module->playState < kFskMediaPlayerStateBuffering)
				property->value.number = 0.0;
			else
			if (state->module->playState > kFskMediaPlayerStateBuffering)
				property->value.number = 1.0;
			else {
				if (0 == state->firstExtractClock.seconds)
					property->value.number = 0.0;
				else if (state->maxTimeExtracted <= state->firstExtractTime)
					property->value.number = 0.0;
				else if (state->maxTimeExtracted >= state->bufferToTime)
					property->value.number = 1.0;
				else
					property->value.number = ((double)(state->maxTimeExtracted - state->firstExtractTime)) / ((double)(state->bufferToTime - state->firstExtractTime));
			}
			break;

		case kFskMediaPropertyAuthentication:
			if (state->isNetwork) {
				char *realm = state->http.realm;
				property->type = kFskMediaPropertyTypeStringList;
				if ((0 == FskStrCompare(realm, "idisk.mac.com")) || (0 == FskStrCompare(realm, "me.com"))) {
					char *slash;
					realm = FskStrDoCopy(state->http.uri + 7);
					slash = FskStrChr(realm, '/');
					if (slash) {
						slash = FskStrChr(slash + 1, '/');
						if (slash)
							*slash = 0;
					}
				}
				err = FskMemPtrNewClear(FskStrLen(realm) + 2, (FskMemPtr*)(void*)(&property->value.str));
				if (kFskErrNone == err)
					FskStrCopy(property->value.str, realm);
				if (realm != state->http.realm)
					FskMemPtrDispose(realm);
			}
			else
				err = FskMediaReaderGetProperty(state->reader, propertyID, property);
			break;

		case kFskMediaPropertyError:
			if (kFskErrNone != state->error) {
				property->type = kFskMediaPropertyTypeInteger;
				property->value.integer = state->error;
			}
			else
				err = FskMediaReaderGetProperty(state->reader, propertyID, property);
			break;

		case kFskMediaPropertySeekableSegment:
			err = FskMediaReaderGetProperty(state->reader, propertyID, property);
			if (((kFskErrUnimplemented == err) && (true == state->http.isDownloadFailed)) ||
				((kFskErrNone == err) && (kFskMediaPropertyTypeFloat == property->type))) {
				double start, stop;

				if (kFskErrUnimplemented == err) {
					FskSampleTime atTime;

					FskMediaPlayerGetTime(state->player, (float)1000, &atTime);
					start = stop = (double)atTime / 1000.0;
				}
				else {
					start = 0;
					stop = property->value.number;
				}

				err = FskMemPtrNew(sizeof(double) * 2, &property->value.numbers.number);
				if (kFskErrNone == err) {
					property->value.numbers.number[0] = start;
					property->value.numbers.number[1] = stop;
					property->value.numbers.count = 2;

					property->type = kFskMediaPropertyTypeFloatList;
				}
			}
			break;

		case kFskMediaPropertyDownloadPath:
			if (state->http.downloadPath) {
				property->type = kFskMediaPropertyTypeString;
				property->value.str = FskStrDoCopy(state->http.downloadPath);
			}
			break;

		case kFskMediaPropertyDownload:
			property->type = kFskMediaPropertyTypeInteger;
			property->value.integer = state->http.downloadPreference;
			break;

		case kFskMediaPropertyDownloadPosition:
			if (state->http.isDownload) {
				readerMediaPlayerNetBuffer walker;

				// flush all buffers so that downloadPosition is up-to-date
				for (walker = state->spool.buffers; NULL != walker; walker = walker->next) {
					if (walker->dirty)
						rmpSpoolerFlushBuffer(&state->spooler, walker);
				}

				property->type = kFskMediaPropertyTypeFloat;
				property->value.number = (double)(state->http.position ? state->http.position : state->http.downloadPosition);
			}
			break;

		case kFskMediaPropertyRequestHeaders:
			FskMemPtrDisposeAt(&state->http.additionalHeaders);
			err = FskStrListDoCopy(property->value.str, &state->http.additionalHeaders);
			break;

		case kFskMediaPropertyLocation:
			property->type = kFskMediaPropertyTypeString;
			property->value.str = FskStrDoCopy(state->http.uri);
			break;

		case kFskMediaPropertyBufferDuration:
			property->type = kFskMediaPropertyTypeInteger;
			property->value.integer = state->minTimeToBuffer;
			break;

		default:
			err = FskMediaReaderGetProperty(state->reader, propertyID, property);
			break;
	}

	return err;
}

/*
	locals
*/

FskErr rmpFileSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err;

	err = FskFileOpen(state->uri + 7, permissions, &state->spool.fref);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSetOwner(state->spool.fref, state->player);

	state->spool.position = 0;

	state->spoolerOpen = true;

	err = allocateBuffers(state, 4);
	BAIL_IF_ERR(err);

bail:
	return err;
}

FskErr rmpFileSpoolerClose(FskMediaSpooler spooler)
{
	readerMediaPlayerModule state = spooler->refcon;

	FskFileClose(state->spool.fref);
	state->spool.fref = NULL;

	disposeBuffers(state);

	state->spoolerOpen = false;

	return kFskErrNone;
}

FskErr rmpFileSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **bufferOut, UInt32 *bytesReadOut)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;
	readerMediaPlayerNetBuffer buffer;

	for (buffer = state->spool.buffers; NULL != buffer; buffer = buffer->next) {
		if ((buffer->position <= position) && (position < (buffer->position + buffer->bytes)))
			break;
	}

	if (NULL == buffer) {
		FskInt64 nextPosition = (position / state->spool.bufferSize) * state->spool.bufferSize;		// align read to bufferSize
		UInt32 bufferBytesToRead;

		err = getFreeBuffer(state, &buffer);
		BAIL_IF_ERR(err);

		bufferBytesToRead = state->spool.bufferSize;
		if (state->http.isDownload && ((position + bufferBytesToRead) > state->http.position)) {	// because the file is pre-extended to total duration, pin read to portion that has already been downloaded
			if (state->http.position <= position) {
				err = kFskErrNeedMoreTime;
				goto bail;
			}

			bufferBytesToRead = (UInt32)(state->http.position - position);
			if (0 == bufferBytesToRead) {
				err = kFskErrNeedMoreTime;
				goto bail;
			}
		}

		if (nextPosition != state->spool.position) {
			err = FskFileSetPosition(state->spool.fref, &nextPosition);
			BAIL_IF_ERR(err);

			state->spool.position = nextPosition;
		}

		err = FskFileRead(state->spool.fref, bufferBytesToRead, buffer->data, &buffer->bytes);
		BAIL_IF_ERR(err);

		buffer->position = state->spool.position;
		state->spool.position += buffer->bytes;

		if ((buffer->position + buffer->bytes) <= position) {
			// didn't read enough to reach the start of this request
			if (!state->http.isDownload)
				err = kFskErrEndOfFile;
			else {
				buffer->lastUsed = 0;		// make it very eligible to be re-used when we need another block
				err = kFskErrNeedMoreTime;
			}
			goto bail;
		}
	}

	buffer->lastUsed = ++state->spool.seed;
	*bufferOut = buffer->data + (position - buffer->position);

	if ((position + bytesToRead) <= (buffer->position + buffer->bytes)) {
		if (bytesReadOut)
			*bytesReadOut = bytesToRead;
	}
	else {
		if (!bytesReadOut) {
			err = kFskErrInvalidParameter;
			goto bail;
		}
		*bytesReadOut = (UInt32)(buffer->position + buffer->bytes - position);
	}

	if (bytesReadOut && (0 == *bytesReadOut) && (0 != bytesToRead))
		err = kFskErrEndOfFile;

bail:
	return err;
}

FskErr rmpFileSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size)
{
	readerMediaPlayerModule state = spooler->refcon;
	return FskFileGetSize(state->spool.fref, size);
}

FskErr readerMediaPlayerEventCallback(FskMediaReader reader, void *refCon, FskEventCodeEnum eventCode, FskEvent event)
{
	readerMediaPlayerModule state = refCon;
	FskErr err = kFskErrNone;

	doUsingMediaPlayer(state, true);

	if (kFskEventMediaPlayerInitialize == eventCode) {
		FskMediaPropertyValueRecord property;
		UInt32 readerTrackIndex = 0;

		err = FskMediaReaderGetProperty(state->reader, kFskMediaPropertyTimeScale, &property);
		BAIL_IF_ERR(err);

		state->timeScale = property.value.integer;
		if (!state->minTimeToBuffer)
        state->minTimeToBuffer = kMinTimeToBuffer;

		err = FskMediaReaderGetProperty(state->reader, kFskMediaPropertyDuration, &property);
		BAIL_IF_ERR(err);

		state->module->duration = (FskSampleTime)property.value.number;
		state->module->durationScale = (float)state->timeScale;

		while (true) {
			FskMediaReaderTrack readerTrack;
			readerMediaPlayerTrack track;
			UInt32 mediaFormat = kFskMediaPlayerReaderUnknown;

			if (kFskErrNone != FskMediaReaderGetTrack(state->reader, readerTrackIndex++, &readerTrack))
				break;

			if (kFskErrNone != FskMediaReaderTrackGetProperty(readerTrack, kFskMediaPropertyMediaType, &property))
				continue;

			if (0 == FskStrCompare(property.value.str, "audio"))
				mediaFormat = kFskMediaPlayerReaderAudio;
			else if (0 == FskStrCompare(property.value.str, "video"))
				mediaFormat = kFskMediaPlayerReaderVideo;
			FskMemPtrDispose(property.value.str);

			if (kFskMediaPlayerReaderUnknown == mediaFormat)
				continue;

			err = FskMemPtrNewClear(sizeof(readerMediaPlayerTrackRecord), &track);
			BAIL_IF_ERR(err);

			track->mediaFormat = mediaFormat;
			if (kFskMediaPlayerReaderAudio == mediaFormat)
				track->doEnqueuedData = audioTrackEnqueuedData;
			else
			if (kFskMediaPlayerReaderVideo == mediaFormat) {
				track->doEnqueuedData = videoTrackEnqueuedData;

				err = FskMutexNew(&track->video.mutex, "videoMutex");
				BAIL_IF_ERR(err);

				track->video.spatial_quality		 = 75;		//@@
				track->video.target_temporal_quality = 85;
				track->video.integral_temporal_quality = 0;
				track->video.integral_temporal_quality = 0;
				//track->video.prebuffer_fullness		 = 50;
				track->video.last_requestedDrawingMethod = -1;
				track->video.this_requestedDrawingMethod = -1;
				track->video.back_to_screen_count		 = 0;

				track->video.playMode = kFskMediaPlayerReaderModeNormal;

				track->video.mode.header.dataSize = sizeof(track->video.mode);
				track->video.mode.header.blendLevel = 256;
				track->video.mode.kind = 'cbcb';

				if (kFskErrNone == FskMediaReaderTrackGetProperty(readerTrack, kFskMediaPropertyRotation, &property)) {
					if (property.type == kFskMediaPropertyTypeFloat) {
						if (180 == property.value.number)
							track->video.rotation = 180;
					}
				}
			}

            if ((kFskErrNone == FskMediaReaderTrackGetProperty(readerTrack, kFskMediaPropertyBitRate, &property)) && (kFskMediaPropertyTypeInteger == property.type))
                state->bitRate += property.value.integer;

			track->readerTrack = readerTrack;
			track->state = state;

			FskListAppend((FskList*)(void*)(&state->tracks), track);

			state->module->trackCount += 1;
		}

        updateBuffersForBitrate(state);
		updateNaturalBounds(state);
	}
	else
	if (kFskEventMediaReaderDataArrived == eventCode) {
		if (kFskMediaPlayerStateBuffering == state->module->playState) {
			state->prerolling += 1;

			if (kFskErrNone == ensureReader(state, (double)state->zeroMovieTime))
				checkStart(state);

			state->prerolling -= 1;
		}
		else
		if ((kFskMediaPlayerStateStopped == state->module->playState) && state->readerIsForUpdate)
			extractMoreCallback(NULL, NULL, state);		// when readerIsForUpdate
		goto bail;
	}
	else
	if (kFskEventMediaPlayerMetaDataChanged == eventCode) {
		FskMediaPropertyValueRecord property;
		err = FskMediaReaderGetProperty(state->reader, kFskMediaPropertyDuration, &property);
		BAIL_IF_ERR(err);

		state->module->duration = (FskSampleTime)property.value.number;
	}
	if (kFskEventMediaPlayerStateChange == eventCode)
		FskMediaPlayerSetPlayState(state->player, state->reader->mediaState);
	else
		FskMediaPlayerSendEvent(state->player, eventCode);

bail:
	doUsingMediaPlayer(state, false);

	return err;
}

FskErr readerMediaPlayerModuleMoreCallBack(FskSndChannel sndChan, void *refCon, SInt32 requestedSamples)
{
	return kFskErrNoData;
}

void readerMediaPlayerModuleDoneCallBack(FskSndChannel sndChan, void *refCon, void *dataRefCon, Boolean played)
{
	readerMediaPlayerQueueEntry entry = dataRefCon;
	doneWithQueueEntry(entry);
}

void readerMediaPlayerModuleAbort(void *a, void *b, void *c, void *d)
{
	readerMediaPlayerTrack track = a;
	readerMediaPlayerModule state = track->state;
	FskErr err = (FskErr)b;
	Boolean playing = state->playing;

	if (kFskErrAudioOutReset == err)
		FskMediaPlayerDetachSoundClock(state->player);
	else
		FskMediaPlayerSendEvent(state->player, kFskEventMediaPlayerResourceLost);

	FskMediaPlayerStop(state->player);

	if (kFskErrAudioOutReset == err) {
		FskMediaPropertyValueRecord flags;

		if (state->module->sndChan == track->audio.sndChan) {
			FskMediaPlayerDetachSoundClock(state->player);
            state->module->sndChan = NULL;
        }

		flags.value.integer = 0;
		if (playing)
			FskSndChannelGetProperty(track->audio.sndChan, kFskMediaPropertyFlags, &flags);

		FskSndChannelDispose(track->audio.sndChan);
		track->audio.sndChan = NULL;

		if (playing && !(kFskAudioOutDontRestartOnReset & flags.value.integer))
			FskMediaPlayerStart(state->player);
	}

	doUsingMediaPlayer(state, false);
}

void readerMediaPlayerModuleAbortCallBack(FskSndChannel sndChan, void *refCon, FskErr err)
{
	readerMediaPlayerTrack track = refCon;
	readerMediaPlayerModule state = track->state;

	if ((kFskErrAudioOutReset != err) && (state->module->sndChan == sndChan)) {
		FskMediaPlayerDetachSoundClock(state->player);
        state->module->sndChan = track->audio.sndChan = NULL;
	}

	doUsingMediaPlayer(state, true);
	FskThreadPostCallback(state->thread, readerMediaPlayerModuleAbort, track, (void *)err, NULL, NULL);
}

FskErr ensureReader(readerMediaPlayerModule state, double atTime)
{
	FskErr err = kFskErrNone;
	readerMediaPlayerTrack walker;

	if (false == state->readerRunning) {
		if (kFskErrNone != state->error)
			return kFskErrBadState;

        if (state->reader->mediaState < kFskMediaPlayerStateStopped)
            BAIL(kFskErrNeedMoreTime);

		err = FskMediaReaderStart(state->reader, &atTime, NULL);
		BAIL_IF_ERR(err);

		state->readerRunning = true;
		state->readerIsForUpdate = false;
		state->maxTimeExtracted = 0;
		state->atEnd = false;

        if (state->spooler.flags & kFskMediaSpoolerUseTimeSeek) {
            if (state->http.lastUseSeekTime != atTime) {
                readerMediaPlayerNetBuffer buffer;
                for (buffer = state->spool.buffers; NULL != buffer; buffer = buffer->next)
                    buffer->bytes = buffer->lastUsed = buffer->position = 0;

                state->http.lastUseSeekTime = atTime;

                FskHTTPClientDispose(state->http.client);
                state->http.request = NULL;
                state->http.client = NULL;
            }
        }
        else
            state->http.lastUseSeekTime = -1;

		for (walker = state->tracks; NULL != walker; walker = walker->next) {
			if (kFskMediaPlayerReaderVideo == walker->mediaFormat)
				walker->video.waitingForSync = true;
		}
	}

	extractMoreCallback(NULL, NULL, state);

bail:
	return err;
}

FskErr doUsingMediaPlayer(void *refCon, Boolean inUse)
{
	readerMediaPlayerModule state = refCon;

	if (inUse)
		state->player->useCount += 1;
	else
		FskMediaPlayerDispose(state->player);

	return kFskErrNone;
}

void extractMoreCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	readerMediaPlayerModule state = param;
	FskErr err;
	FskSampleTime atTime, targetTime;
	readerMediaPlayerTrack track, walker;
	readerMediaPlayerQueueEntry entry;

	if (state->error)
		return;

	if (state->doRestart) {
		state->doRestart = false;
		FskMediaPlayerStop(state->player);
		FskMediaPlayerStart(state->player);
		return;
	}

	FskMediaPlayerGetTime(state->player, (float)state->timeScale, &state->atMediaTime);
	FskTimeGetNow(&state->atSystemTime);
	atTime = state->atMediaTime;

	if (state->playing && (atTime <= state->lastMediaTime)) {
		state->stalled = true;
		FskMediaPlayerStop(state->player);
		FskMediaPlayerStart(state->player);
		return;
	}

	doUsingMediaPlayer(state, true);

	state->lastMediaTime = atTime;

	if (state->prerolling)
		targetTime = state->bufferToTime;
	else {
		if (state->playing)
			targetTime = atTime + (FskSampleTime)((state->timeScale * (state->timeToBuffer / 1000.0)));
		else
			targetTime = atTime + 1;
		if ((-1 != state->module->duration) && (targetTime > state->module->duration))
			targetTime = state->module->duration;
	}

	// clear out anything we're done with
	for (track = state->tracks; NULL != track; track = track->next) {
		while (track->samples.entries) {
			entry = track->samples.entries;
			if ((0 != entry->useCount) || (entry->maxDecodeTime >= atTime))
				break;

			FskListRemoveFirst((FskList*)(void*)(&track->samples.entries));

			disposeQueueEntry(entry);
		}
	}

	while ((state->maxTimeExtracted < targetTime) && !state->atEnd) {
		FskMediaReaderTrack extractedTrack;
		UInt32 infoCount;
		FskMediaReaderSampleInfo info, last;
		unsigned char *data;

		err = FskMediaReaderExtract(state->reader, &extractedTrack, &infoCount, &info, &data);
		state->extractErr = err;
		if (err) {
			if (kFskErrEndOfFile == err) {
				state->bufferToTime = state->maxTimeExtracted;
				movieEnd(state);
				break;
			}
			else
			if (kFskErrNeedMoreTime == err)
				;
			else
				playerFailed(state, err);
			goto bail;
		}

		if ((0 == state->firstExtractClock.seconds) && (0 != info->samples)) {
			FskTimeGetNow(&state->firstExtractClock);
			state->firstExtractTime = info->decodeTime;
		}

#if SUPPORT_RECORD
		recordExtracted(state, extractedTrack, infoCount, info, data);
#endif

		for (track = state->tracks; NULL != track; track = track->next) {
			if (extractedTrack != track->readerTrack)
				continue;

			if ((0 == info->samples) && (kFskImageFrameEndOfMedia & info->flags)) {
				trackEnd(track);
				break;
			}

			err = FskMemPtrNew(sizeof(readerMediaPlayerQueueEntryRecord), &entry);
			if (err) {
				FskMemPtrDispose(data);
				FskMemPtrDispose(info);
				goto bail;
			}

			if (NULL == track->samples.entries)
				track->samples.minTime = info->decodeTime;

			last = &info[infoCount - 1];
			track->samples.maxTime = last->decodeTime + (last->samples * last->sampleDuration);

			// calculate maxTimeExtracted as the minimum of all the extracted maximums
			for (walker = state->tracks, state->maxTimeExtracted = targetTime; NULL != walker; walker = walker->next) {
				if (walker->atEnd) continue;
				if (((state->scrub || state->readerIsForUpdate) && (kFskMediaPlayerReaderVideo == walker->mediaFormat)) ||
					(!state->scrub && !state->readerIsForUpdate)) {
					if (state->maxTimeExtracted > walker->samples.maxTime)
						state->maxTimeExtracted = walker->samples.maxTime;
				}
			}

			// some media readers (FLV) don't report EOF, so we check that for them.
			if ((-1 != state->module->duration) && (state->maxTimeExtracted >= state->module->duration)) {
				state->bufferToTime = state->module->duration;
				movieEnd(state);
			}

			entry->sampleInfo = info;
			entry->sampleInfoCount = infoCount;
			entry->data = data;
			entry->next = NULL;
			entry->useCount = 0;
			entry->maxDecodeTime = track->samples.maxTime;
			entry->track = track;

			FskListAppend((FskList*)(void*)(&track->samples.entries), entry);

			if (track->doEnqueuedData)
				(track->doEnqueuedData)(entry);

			data = NULL;
			info = NULL;
			break;
		}

		if (NULL != info) {
			FskMemPtrDispose(data);
			FskMemPtrDispose(info);
		}
	}

bail:
	if (state->playing) {
		if (state->maxTimeExtracted > state->atMediaTime) {
			SInt32 delta = (SInt32)(state->maxTimeExtracted - state->atMediaTime);
			delta = (delta * 1000) / state->timeScale;
			FskTimeCallbackScheduleFuture(state->emptyBuffersCallback, delta / 1000, delta % 1000, emptyBuffersCallback, state);
		}
		else
			FskTimeCallbackRemove(state->emptyBuffersCallback);
	}

	if ((kFskMediaPlayerStateBuffering == state->module->playState) && state->firstExtractClock.seconds && state->isNetwork) {
		if (NULL == state->bufferingCallback)
			FskTimeCallbackNew(&state->bufferingCallback);

		if (NULL != state->bufferingCallback)
			FskTimeCallbackScheduleFuture(state->bufferingCallback, 0, 1000, bufferingCallback, state);

		state->module->idleFlags |= kFskMediaPlayerIdleChangePropertyBuffer;
	}

	if (NULL != callback)
		FskTimeCallbackScheduleFuture(callback, 0, state->refillInterval, extractMoreCallback, state);

	doUsingMediaPlayer(state, false);
}

void bufferingCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	readerMediaPlayerModule state = param;
	FskTimeRecord timeNow;
	SInt32 bufferingMS;
	SInt32 bufferedMS;
	Boolean lowBandwidth;

	FskTimeGetNow(&timeNow);
	FskTimeSub(&state->firstExtractClock, &timeNow);
	bufferingMS = FskTimeInMS(&timeNow);

	bufferedMS = (SInt32)(((state->maxTimeExtracted - state->firstExtractTime) / (double)state->timeScale) * 1000.0);
	lowBandwidth = bufferingMS > (bufferedMS * 1.2);
	if ((lowBandwidth != state->lowBandwidth) && (bufferingMS > 1000)) {
		state->lowBandwidth = lowBandwidth;
		sendWarning(state, lowBandwidth ? "bandwidth.insufficient" : "bandwidth.ok", NULL);
	}

	if ((kFskMediaPlayerStatePlaying == state->module->playState) && (false == state->lowBandwidth))
		;	// we're done. let the timer lapse.
	else
		FskTimeCallbackScheduleFuture(state->bufferingCallback, 0, 500, bufferingCallback, state);
}

Boolean flushQueues(readerMediaPlayerModule state)
{
	readerMediaPlayerTrack track;
	Boolean needsUpdate = false;

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderVideo == track->mediaFormat) {
			flushVideoFrames(track);
			if (-1 != state->module->duration) {
				track->pendingUpdate = true;
				needsUpdate = true;
			}
		}

		flushQueue(&track->samples);
	}

	return needsUpdate;
}

void flushQueue(readerMediaPlayerQueue queue)
{
	while (queue->entries) {
		readerMediaPlayerQueueEntry entry = FskListRemoveFirst((FskList*)(void*)(&queue->entries));
		disposeQueueEntry(entry);
	}

	queue->minTime = 0;
	queue->maxTime = 0;
}

void usingQueueEntry(readerMediaPlayerQueueEntry entry)
{
	entry->useCount += 1;
}

void doneWithQueueEntry(readerMediaPlayerQueueEntry entry)
{
	if (NULL != entry)
		entry->useCount -= 1;
}

void disposeQueueEntry(readerMediaPlayerQueueEntry entry)
{
	FskMemPtrDispose(entry->data);
	FskMemPtrDispose(entry->sampleInfo);
	FskMemPtrDispose(entry);
}

//@@ we should take the track x/y offset and scaling (matrix) into account here at some point
void updateNaturalBounds(readerMediaPlayerModule state)
{
	readerMediaPlayerTrack track;
	FskDimensionRecord naturalBounds = {0, 0};

	for (track = state->tracks; NULL != track; track = track->next) {
		FskMediaPropertyValueRecord property;

		if (kFskErrNone == FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyDimensions, &property)) {
			if (property.value.dimension.width > naturalBounds.width)
				naturalBounds.width = property.value.dimension.width;
			if (property.value.dimension.height > naturalBounds.height)
				naturalBounds.height = property.value.dimension.height;
		}
	}

	state->module->naturalWidth = naturalBounds.width;
	state->module->naturalHeight = naturalBounds.height;
}

void movieEnd(readerMediaPlayerModule state)
{
	readerMediaPlayerTrack walker;
	FskSampleTime now;
	UInt32 delta;

	state->atEnd = true;

	for (walker = state->tracks; NULL != walker; walker = walker->next)
		trackEnd(walker);

	FskMediaPlayerGetTime(state->player, (float)state->module->durationScale, &now);
	delta = (UInt32)(((state->module->duration - now) / (float)state->module->durationScale) * 1000.0);
	if (!state->endCallback)
		FskTimeCallbackNew(&state->endCallback);
	FskTimeCallbackScheduleFuture(state->endCallback, delta / 1000, delta % 1000, movieStop, state);
}

void movieStop(FskTimeCallBack callback, const FskTime timeIn, void *param)
{
	readerMediaPlayerModule state = param;
	FskMediaPlayerSendEvent(state->player, kFskEventMediaPlayerEnd);
}

void trackEnd(readerMediaPlayerTrack track)
{
	readerMediaPlayerModule state = track->state;

	if (track->atEnd)
		return;

	track->atEnd = true;

	if ((kFskMediaPlayerReaderAudio == track->mediaFormat) && track->audio.sndChan) {
		if (state->playing)
			FskSndChannelEnqueue(track->audio.sndChan, (void *)-1, 0, 0, 0, NULL, NULL);			// flush

		if (state->module->sndChan == track->audio.sndChan) {
			FskSampleTime now;

			FskMediaPlayerDetachSoundClock(state->player);

			FskMediaPlayerGetTime(state->player, (float)track->state->timeScale, &now);
			if (now >= track->samples.maxTime)
				FskSndChannelStop(track->audio.sndChan);
			else {
				UInt32 delta = (UInt32)(((track->samples.maxTime - now) / (float)track->state->timeScale) * 1000.0);
				if (!track->audio.stop)
					FskTimeCallbackNew(&track->audio.stop);
				FskTimeCallbackScheduleFuture(track->audio.stop, delta / 1000, delta % 1000, audioStop, track);
			}
		}
	}
	else
	if ((kFskMediaPlayerReaderVideo == track->mediaFormat) && track->video.deco) {
		readerMediaPlayerVideoFrameQueue frame, last = NULL;

		for (frame = track->video.frames; NULL != frame; frame = frame->next)
			last = frame;

		// if last frame already queued and we're waiting on that, then issue a flush (EOS)
		if (last && last->decompressing)
			FskImageDecompressFrame(track->video.deco, NULL, 0, NULL, true,
				videoTrackDecompressComplete, NULL, NULL, NULL,
				NULL, NULL, 0);
	}
}

void audioStop(FskTimeCallBack callback, const FskTime timeIn, void *param)
{
	readerMediaPlayerTrack track = param;
	FskSndChannelStop(track->audio.sndChan);
}

FskErr audioTrackEnqueuedData(readerMediaPlayerQueueEntry entry)
{
	readerMediaPlayerTrack track = entry->track;

	if (track->state->playing)
		queueAudioFrames(track, entry);

	return kFskErrNone;
}

FskErr queueAudioFrames(readerMediaPlayerTrack track, readerMediaPlayerQueueEntry entry)
{
	FskErr err = kFskErrNone;
	SInt32 samplesToSkip = track->audio.samplesToSkip;
	readerMediaPlayerModule state = track->state;

	if (NULL == track->audio.sndChan)
		return kFskErrNone;

	track->audio.samplesToSkip = 0;

	for ( ; NULL != entry; entry = entry->next) {
		usingQueueEntry(entry);

		if ((NULL == entry->data) && (NULL == entry->sampleInfo)) {
			SInt32 t = entry->sampleInfoCount;
			for (t += track->audio.frequency; t > 0; t -= track->audio.frequency)
				FskSndChannelEnqueue(track->audio.sndChan, NULL, 0, track->audio.frequency, 1, NULL, NULL);
		}
		else
		if (1 == entry->sampleInfoCount) {

			if (!(kFskImageFrameGap & entry->sampleInfo[0].flags)) {
				err = FskSndChannelEnqueueWithSkip(track->audio.sndChan, entry->data, entry->sampleInfo[0].samples * entry->sampleInfo[0].sampleSize,
												entry->sampleInfo[0].samples, entry->sampleInfo[0].sampleDuration, entry, NULL, samplesToSkip);
			}
			else {
				UInt32 sc = entry->sampleInfo[0].samples * entry->sampleInfo[0].sampleDuration;
				sc = (UInt32)(((double)sc * (double)track->audio.frequency) / (double)state->timeScale);		// convert to audio time scale
				err = FskSndChannelEnqueueWithSkip(track->audio.sndChan, NULL, 0, sc, 1, entry, NULL, 0);
			}
			if (err) {
				disposeQueueEntry(entry);
				goto bail;
			}
		}
		else {
			UInt32 dataSize = 0, samples = 0;
			UInt32 i, j, k;
			UInt32 *frameSizes;

			// note: this code assumes sampleDuration is constant across all info records. if that is not true, multiple calls to enqueue would be needed

			for (i = 0; i < entry->sampleInfoCount; i++) {
				dataSize += entry->sampleInfo[i].samples * entry->sampleInfo[i].sampleSize;
				samples += entry->sampleInfo[i].samples;
			}

			err = FskMemPtrNew(sizeof(UInt32) * samples, &frameSizes);
			BAIL_IF_ERR(err);

			for (i = 0, k = 0; i < entry->sampleInfoCount; i++) {
				for (j = 0; j < entry->sampleInfo[i].samples; j++)
					frameSizes[k++] = entry->sampleInfo[i].sampleSize;
			}

			err = FskSndChannelEnqueueWithSkip(track->audio.sndChan, entry->data, dataSize, samples, entry->sampleInfo[0].sampleDuration, entry, frameSizes, samplesToSkip);
			FskMemPtrDispose(frameSizes);
			BAIL_IF_ERR(err);
		}

		samplesToSkip = 0;

		if (entry->sampleInfo && (kFskImageFrameEndOfMedia & entry->sampleInfo[entry->sampleInfoCount - 1].flags) && (state->module->duration > track->samples.maxTime)) {
			trackEnd(track);
			break;
		}
	}

bail:
	return err;
}

FskErr videoTrackEnqueuedData(readerMediaPlayerQueueEntry entry)
{
	readerMediaPlayerTrack track = entry->track;
	readerMediaPlayerModule state = track->state;
	FskErr err = kFskErrNone;
	UInt32 i, j;

	FskMutexAcquire(track->video.mutex);

	for (i = 0; i < entry->sampleInfoCount; i++) {
		for (j = 0; j < entry->sampleInfo[i].samples; j++) {
			readerMediaPlayerVideoFrameQueue frame;
			UInt32 flags = entry->sampleInfo[i].flags;

			if (track->video.waitingForSync && (flags != kFskImageFrameTypeSync) && (flags != kFskImageFrameTypePartialSync))
				continue;
			else
				track->video.waitingForSync = false;

			err = FskMemPtrNewClear(sizeof(readerMediaPlayerVideoFrameQueueRecord), &frame);
			BAIL_IF_ERR(err);

			frame->track = track;
			frame->entry = entry;
			frame->duration = entry->sampleInfo[i].sampleDuration;
			frame->decodeTime = (j * frame->duration) + entry->sampleInfo[i].decodeTime;
			frame->compositionTime = (-1 != entry->sampleInfo[i].compositionTime) ? entry->sampleInfo[i].compositionTime : frame->decodeTime;
			frame->compositionTimeOffset = (UInt32)(frame->compositionTime - frame->decodeTime);
			frame->compositionTimeReturned = frame->compositionTime;
			frame->data = (j * entry->sampleInfo[i].sampleSize) + (unsigned char *)entry->data;
			frame->dataSize = entry->sampleInfo[i].sampleSize;
			frame->frameType = flags;
			frame->sequenceNumber = track->video.sequenceNumber++;
			usingQueueEntry(entry);

			FskListAppend((FskList*)(void*)(&track->video.frames), frame);
		}
	}

	if (state->port)
		decompressVideoFrames(track);
	else {
		readerMediaPlayerVideoFrameQueue frame, key = NULL;
		FskSampleTime atTime;

		FskMediaPlayerGetTime(track->state->module->player, (float)track->state->timeScale, &atTime);

		for (frame = track->video.frames; NULL != frame; frame = frame->next) {
			if (frame->decompressing || (frame->decodeTime >= atTime) || (frame->compositionTime >= atTime))
				break;

			if (kFskImageFrameTypeSync == frame->frameType)
				key = frame;
		}

		if (key) {
			for (frame = track->video.frames; key != frame; frame = frame->next)
				frame->decompressed = true;			// so flushVideoBefore will really dispose it

			flushVideoBefore(track, &key->decodeTime);
		}
	}

bail:
	FskMutexRelease(track->video.mutex);

	if ((kFskMediaPlayerStateBuffering == state->module->playState) || track->pendingUpdate)
		sendUpdate(state);		// this is a little aggressive, but being accurate and efficient is really tough

	return kFskErrNone;
}


void update_history( readerMediaPlayerTrack track, readerMediaPlayerVideoFrameQueue frame )
{
	// update history
	readerMediaPlayerVideoFrameHistory history;

	if (track->video.historyWaiting) {
		if (!frame->played || (frame->compositionTime < track->video.historyBegins)) {
//			fprintf( stderr, "###SKIP_history: compositionTime=%d, played=%d, decompressed=%d\n", (int)frame->compositionTime, (int)frame->played, (int)frame->decompressed);
			return;
		}

//fprintf(stderr, "historyWaiting = false in update_history\n");
		track->video.historyWaiting = false;
		FskMediaPlayerGetTime(track->state->module->player, (float)track->state->timeScale, &track->video.nextQualityAdjustTime);
		track->video.nextQualityAdjustTime += track->state->timeScale;			// want at least one second of data before making any decisions
	}

//	fprintf( stderr, "###update_history: compositionTime=%d, played=%d, decompressed=%d\n", (int)frame->compositionTime, (int)frame->played, (int)frame->decompressed);

	history = &track->video.history[track->video.historyIndex];
	track->video.historyIndex += 1;
	if (track->video.historyIndex >= kVideoFrameHistoryCount)
		track->video.historyIndex = 0;

	history->compositionTime = frame->compositionTime;
	history->played = frame->played;
	history->valid = true;

}

void clear_history( readerMediaPlayerTrack track )
{
	int i;
	readerMediaPlayerVideoFrameHistory history;
	readerMediaPlayerModule state = track->state;

	for (i = 0, history = track->video.history; i < kVideoFrameHistoryCount; i++, history++)
	{
		history->compositionTime = 0;
		history->valid = false;
		history->played = false;
	}

	track->video.historyIndex = 0;
	track->video.last_requestedDrawingMethod = -1;
	track->video.this_requestedDrawingMethod = -1;
	track->video.back_to_screen_count = 0;

	track->video.historyWaiting = true;
	FskMediaPlayerGetTime(state->module->player, (float)state->timeScale, &track->video.historyBegins);

//	fprintf( stderr, "clear_history\n");		//@@
	track->video.dropBudget = 0;
}


SInt32 fuse_quality( int play_mode, int percet_played, int candidate_quality )
{
	int spatial_quality  = candidate_quality;

	//fprintf( stderr, "candidate_quality=%d, ", candidate_quality );

	if( play_mode == kFskMediaPlayerReaderModeNormal )
	{
		if( spatial_quality < 75 )
			spatial_quality = 80;
	}
	else if(  play_mode == kFskMediaPlayerReaderModeDropHalf )
	{
		spatial_quality = (int)(spatial_quality * 1.4);
	}
	else if(  play_mode == kFskMediaPlayerReaderModeDropTwoThirds )
	{
		spatial_quality = (int)(spatial_quality * 1.7);
	}
	else if(  play_mode == kFskMediaPlayerReaderModeOnlyKeys )
	{
		spatial_quality =  100;
	}

	if( spatial_quality > 100 )
		spatial_quality = 100;

//	fprintf( stderr, "quality=%d\n", spatial_quality );

	return spatial_quality;
}

SInt32 adjust_quality( readerMediaPlayerTrack track, FskSampleTime atTime)
{
	readerMediaPlayerModule state = track->state;
	SInt32 spatial_quality			= track->video.spatial_quality;
	int	   real_temporal_quality    = 100;
	//int	   decompressing_total = 0, decompressed_total = 0;
	//int	   prebuffer_fullness = 50;

	if ((atTime < track->video.nextQualityAdjustTime) || track->video.historyWaiting)
		goto bail;

/*
	//adjust quality by looking forward, based on the number of bitmaps queued for decompress versus those still pending
	for (frame = track->video.frames; NULL != frame; frame = frame->next)
	{
		if (frame->decompressing) decompressing_total++;
		if (frame->decompressed ) decompressed_total++;
	}

	total = decompressing_total + decompressed_total;
	if( total >= 1 )
	{
		int history_prebuffer_fullness = track->video.prebuffer_fullness;
		int	history_factor = 80;

		prebuffer_fullness = decompressed_total * 100 / total;
		prebuffer_fullness = (history_prebuffer_fullness*history_factor + prebuffer_fullness*(100-history_factor))/100;
		track->video.prebuffer_fullness = prebuffer_fullness;
	}
*/

	// adjust quality looking backwards, based on the number of frames dropped over the last 3 seconds
	{
		int historyFrames = 0, playedFrames = 0, i;
		readerMediaPlayerVideoFrameHistory history;
		FskInt64 earliestTime;
		FskInt64 first_frame_time = 0;
		FskInt64 last_frame_time  = 0;
		float load_ratio = 0;
		float  play_fps  = -1;
		float  movie_fps = -1;

		if (atTime < (state->timeScale * 3))
			earliestTime = 0;
		else
			earliestTime = atTime - (state->timeScale * 3);

		for (i = 0, history = track->video.history; i < kVideoFrameHistoryCount; i++, history++)
		{
			if (!history->valid)
				continue;

			if (history->compositionTime < earliestTime || history->compositionTime > atTime)
				continue;

			historyFrames++;
			if (history->played)
				playedFrames++;

			if( first_frame_time > history->compositionTime || first_frame_time == 0 )
				first_frame_time = history->compositionTime;

			if( last_frame_time < history->compositionTime || last_frame_time == 0 )
				last_frame_time = history->compositionTime;
		}

		if (0 != historyFrames)
			real_temporal_quality = (playedFrames * 100) / historyFrames;
		else
			real_temporal_quality = 100;

		{
			//determine play mode based on the codec real timne decode fps and the movie's default fps
			FskMediaPropertyValueRecord p;

			if (kFskErrNone == FskImageDecompressGetProperty(track->video.deco, kFskMediaPropertyPerformanceInfo, &p))
				play_fps = (float)p.value.number;

			if (kFskErrNone == FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyFrameRate, &p) )
				movie_fps = (float)p.value.ratio.numer / (float)p.value.ratio.denom;
			else if( last_frame_time - first_frame_time > 0 && historyFrames - 1 > 0 )
				movie_fps = (float)(historyFrames - 1 )/((float)(last_frame_time - first_frame_time)/ (float)track->state->timeScale );

			if( movie_fps > 0 && play_fps > 0 )
			{
#define kHalfThreshold		1.1
#define kOneThirdThreshold	0.8
#define kKeyOnlyThreshold	0.45
#define kKeyOnlyThreshold_in_KeyOnlyMode	0.4

				load_ratio = play_fps / movie_fps;
				//fprintf( stderr,  "ratio=%5.2f, temporalQ=%d, old_mode=%d, ", load_ratio, real_temporal_quality,  track->video.playMode );
				//fprintf( stderr,  "play_fps=%5.2f, movie_fps=%5.2f\n", play_fps, movie_fps);
				//if( track->video.back_to_screen_count )
				//	fprintf( stderr,  "just back to to screen\n");

				if( !track->video.back_to_screen_count && load_ratio < kKeyOnlyThreshold_in_KeyOnlyMode && track->video.playMode == kFskMediaPlayerReaderModeOnlyKeys )
					track->video.playMode = kFskMediaPlayerReaderModeOnlyKeys;
				else if(  !track->video.back_to_screen_count && load_ratio < kKeyOnlyThreshold )
					track->video.playMode = kFskMediaPlayerReaderModeOnlyKeys;
				else if( load_ratio < kOneThirdThreshold )
					track->video.playMode = kFskMediaPlayerReaderModeDropTwoThirds;
				else if( load_ratio < kHalfThreshold )
					track->video.playMode = kFskMediaPlayerReaderModeDropHalf;
				else
					track->video.playMode = kFskMediaPlayerReaderModeNormal;

				if( real_temporal_quality < 40 && track->video.playMode < kFskMediaPlayerReaderModeDropHalf )
					track->video.playMode = kFskMediaPlayerReaderModeDropHalf;

				if( real_temporal_quality < 20 && track->video.playMode < kFskMediaPlayerReaderModeDropTwoThirds )
					track->video.playMode = kFskMediaPlayerReaderModeDropTwoThirds;

				if( track->video.back_to_screen_count >= 1 )
					track->video.back_to_screen_count--;

				//track->video.playMode = kFskMediaPlayerReaderModeDropHalf;

				//if( real_temporal_quality < 8 && track->video.playMode != kFskMediaPlayerReaderModeOnlyKeys )
				//	track->video.playMode = kFskMediaPlayerReaderModeOnlyKeys;
				//fprintf( stderr, "new_mode=%d\n", track->video.playMode );

				{
					FskMediaPropertyValueRecord value;

					value.type = kFskMediaPropertyTypeInteger;
					value.value.integer = track->video.playMode;
					FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyPlayMode | 0x80000000, &value);
				}
			}
		}

		{
			int	target_temporal_quality  = track->video.target_temporal_quality;
			int i_tq, p_tq;
			int kp = 100;
			int ki = 50;
			int	sq;

			i_tq = track->video.integral_temporal_quality;
			p_tq = real_temporal_quality - target_temporal_quality ;

			i_tq = i_tq *60/100 + p_tq * 40/100;
			if( i_tq > 30 ) i_tq  =  50;
			if( i_tq < -60 ) i_tq = -50;
			sq = 90 + (p_tq * kp + i_tq * ki ) / 100;

			if (sq < 0)   sq = 0;
			if (sq > 100) sq = 100;

			spatial_quality = fuse_quality( track->video.playMode, real_temporal_quality, sq );

			if( spatial_quality >= 90 &&
				track->video.playMode == kFskMediaPlayerReaderModeNormal &&
				load_ratio >= 1.8 )
			{
				spatial_quality = 150; //singla of allowing postprocessing
			}

			if( play_fps > 22 && load_ratio >= 1.5 && spatial_quality < 100 )
				spatial_quality = 100;	//when fps is enough high, crank up spatial quality

			//if (real_temporal_quality >= 80) spatial_quality += 5;
			//else spatial_quality -= 5;//20;
			track->video.integral_temporal_quality = i_tq;
			track->video.nextQualityAdjustTime += state->timeScale / 3;
//			printf("history = %d, tq = %d, sq=%d, output_sq = %d, prebuf=%d, p_tq=%d, i_tq=%d, played=%d, atTime=%d, timeScale=%d\n",
//				(int)historyFrames,  (int)(real_temporal_quality),
//				sq, (int)spatial_quality,(int)prebuffer_fullness,
//				p_tq, i_tq, playedFrames, atTime, (int)state->timeScale );

		}
	}

bail:

	return spatial_quality;
}

/*
	Warning: this function is sometimes called from the video decompress callback, which is
		not the owning thread of this media reader instance
*/
FskErr decompressVideoFrames(readerMediaPlayerTrack track)
{
	FskErr err = kFskErrNone;
	readerMediaPlayerModule state = track->state;
	readerMediaPlayerVideoFrameQueue frame;
	FskTimeRecord systemDelta;
	FskSampleTime atTime;
	SInt32 quality;
	FskBitmapFormatEnum *preferredYUVFormats;

	if (track->video.flushing)
		return kFskErrNone;

	FskMutexAcquire(track->video.mutex);

	if (NULL == track->video.deco) {
		FskMediaPropertyValueRecord pixelFormat, framesToQueue = {0};

		if (track->video.noDeco) {
			err = kFskErrExtensionNotFound;
			goto bail;
		}

		if (NULL == track->video.format) {
			FskMediaPropertyValueRecord property;

			err = FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyFormat, &property);
			BAIL_IF_ERR(err);

			track->video.format = property.value.str;

			err = FskMediaReaderTrackGetProperty(track->readerTrack, kFskMediaPropertyFormatInfo, &track->video.formatInfo);
			if ((kFskErrNone != err) && (kFskErrUnimplemented != err)) {
				FskMemPtrDisposeAt(&track->video.format);
				goto bail;
			}

		}

		err = FskImageDecompressNew(&track->video.deco, 0, track->video.format, NULL);
		if (err) {
			if (kFskErrExtensionNotFound == err) {
				track->video.noDeco = true;
				sendWarning(state, "unsupported.codec.video", track->video.format);
			}
			goto bail;
		}

		FskInstrumentedItemSetOwner(track->video.deco, state->player);

		FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyFormatInfo, &track->video.formatInfo);

		BAIL_IF_ERR(err = FskPortPreferredYUVFormatsGet(state->port, &preferredYUVFormats));	/* Get NULL-terminated list of preferred YUV formats */
		pixelFormat.value.integers.integer = (UInt32 *)preferredYUVFormats;						/* Choose the favorite format */
        for (pixelFormat.value.integers.count = 0; 0 != preferredYUVFormats[pixelFormat.value.integers.count]; pixelFormat.value.integers.count++)
            ;
		pixelFormat.type = kFskMediaPropertyTypeUInt32List;
		FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyPixelFormat, &pixelFormat);
		FskMemPtrDispose(preferredYUVFormats);

        track->video.maxFramesToQueue = kMaxFramesToQueue;
		if ((kFskErrNone == FskImageDecompressGetProperty(track->video.deco, kFskMediaPropertyMaxFramesToQueue, &framesToQueue)) && (framesToQueue.type == kFskMediaPropertyTypeInteger))
            track->video.maxFramesToQueue = (UInt32)framesToQueue.value.integer;
	}

	if (track->video.framesAtDecompressor >= track->video.maxFramesToQueue)
		goto bail;

	quality = track->video.spatial_quality;

	if (state->playing) {
		FskTimeGetNow(&systemDelta);
		FskTimeSub(&state->atSystemTime, &systemDelta);
		if (kFskTimeMsecPerSec == state->timeScale)
			atTime = state->atMediaTime + FskTimeInMS(&systemDelta);
		else
			atTime = state->atMediaTime + (FskTimeInMS(&systemDelta) * state->timeScale) / kFskTimeMsecPerSec;

#ifdef ADJUST_SPATIAL_TEMPORAL_QUALITY
		quality = adjust_quality( track, atTime );
		//fprintf( stderr, "adjust qualit to: %d, mode to:%d\n", quality, track->video.playMode );
#endif
	}
	else
		FskMediaPlayerGetTime(state->module->player, (float)state->timeScale, &atTime);

	for (frame = track->video.frames; (NULL != frame) && (track->video.framesAtDecompressor <= track->video.maxFramesToQueue); frame = frame->next) {
		UInt32 frameTypeAddition = 0;
		Boolean drop = false;

		if (frame->decompressing || frame->decompressed)
			continue;

		if ((kFskMediaPlayerReaderModeOnlyKeys == track->video.playMode) && (kFskImageFrameTypeSync != frame->frameType)) {
			frame->decompressed = true;
			frame->releasedQueueEntry = true;
			doneWithQueueEntry(frame->entry);
			continue;
		}

		if (((kFskMediaPlayerReaderModeDropHalf == track->video.playMode) && (1 & frame->sequenceNumber)) ||
			((kFskMediaPlayerReaderModeDropTwoThirds == track->video.playMode) && (frame->sequenceNumber % 3)))
			track->video.dropBudget += 1;

		if (0 != track->video.dropBudget) {
			if ((track->video.dropBudget > 2) || (kFskImageFrameTypeDroppable == frame->frameType)) {
				drop = true;
				goto doQueue;
			}
		}

		if (state->playing && ((frame->decodeTime + (state->timeScale / 4)) <= atTime)) {
			// we're behind by at least a quarter of a second, so see what we can do to get caught up.
			readerMediaPlayerVideoFrameQueue walker;

			// see if there's a key frame on the horizon (e.g. within the next second)
			for (walker = frame->next; NULL != walker; walker = walker->next) {
				if (walker->decodeTime > (atTime + state->timeScale))
					break;

				if ((kFskImageFrameTypeSync == walker->frameType) || (kFskImageFrameTypePartialSync == walker->frameType)) {
					// key frame ahead of us in the queue. let's skip ahead to that.
					for (; walker != frame; frame = frame->next) {
						frame->decompressed = true;
						frame->releasedQueueEntry = true;
						doneWithQueueEntry(frame->entry);
					}

					goto doQueue;
				}
			}

			//fprintf(stderr, "drop to catch up next key frame\n");
			drop = true;
		}
		else if (((state->module->playState <= kFskMediaPlayerStateBuffering) || state->prerolling) && !state->scrub) {
			FskSampleTime targetTime = (kFskMediaPlayerStateBuffering == state->module->playState) ? state->zeroMovieTime : atTime;

			// when buffering or prerolling, don't need frames before the buffering start time
			if (0 != frame->duration) {
				if ((frame->decodeTime + frame->duration) < targetTime)
					drop = true;
			}
			else {
				if (frame->next && (frame->next->decodeTime < targetTime))
					drop = true;
			}
		}

doQueue:
		if (drop) {
			track->video.dropBudget -= 1;
			if (track->video.dropBudget < 0) track->video.dropBudget = 0;

			frame->doDrop = true;

			if (kFskImageFrameTypeDroppable == frame->frameType) {
				frame->decompressed = true;
				frame->releasedQueueEntry = true;
				doneWithQueueEntry(frame->entry);
				continue;
			}

			if ((kFskImageFrameTypeSync == frame->frameType) && (NULL != frame->next) && (kFskImageFrameTypeSync == frame->next->frameType)) {
				// a key frame can be dropped if the next frame is a key frame (all key frame case, e.g. motion jpeg qt files)
				frame->decompressed = true;
				frame->releasedQueueEntry = true;
				doneWithQueueEntry(frame->entry);
				continue;
			}

			// we can't skip this frame, but at least let the decompressor know we don't really need it
			frameTypeAddition |= kFskImageFrameDrop;
		}

		if (state->scrub)
			frameTypeAddition |= kFskImageFrameImmediate;

		frame->decompressing = true;
		track->video.framesAtDecompressor += 1;
//printf("framesAtDecompressor = %d (enqueue)\n", track->video.framesAtDecompressor);

#ifdef ADJUST_SPATIAL_TEMPORAL_QUALITY
		if (quality != track->video.spatial_quality)
		{
			FskMediaPropertyValueRecord value;

			value.type = kFskMediaPropertyTypeFloat;
			value.value.number = (double)quality / 100.0;
			FskImageDecompressSetProperty(track->video.deco, kFskMediaPropertyQuality | 0x80000000, &value);
//printf("change to quality = %d\n", quality);
			track->video.spatial_quality = quality;
		}
#endif

		err = FskImageDecompressFrame(track->video.deco, frame->data, frame->dataSize, &frame->bits, true,
			videoTrackDecompressComplete, frame, &frame->decodeTime, &frame->compositionTimeOffset,
			&frame->compositionTimeReturned, NULL, frame->frameType | frameTypeAddition);
		if (err) {
			frame->decompressing = false;
			track->video.framesAtDecompressor -= 1;
//printf("framesAtDecompressor = %d - ERROR\n", track->video.framesAtDecompressor);
			goto bail;
		}

		if (track->atEnd && (NULL == frame->next)) {
			FskImageDecompressFrame(track->video.deco, NULL, 0, NULL, true,
				videoTrackDecompressComplete, NULL, NULL, NULL,
				NULL, NULL, 0);
		}


		if (state->playing)
			break;			// when playing, we queue the next frame from the completion callback of the current frame
	}

bail:
	FskMutexRelease(track->video.mutex);

	return err;
}

void videoTrackDecompressComplete(FskImageDecompress deco, void *refcon, FskErr result, FskBitmap bits)
{
	readerMediaPlayerVideoFrameQueue frame = refcon;
	readerMediaPlayerTrack track;

	if (NULL == frame) {
		FskBitmapDispose(bits);
		return;
	}

	track = frame->track;

	FskMutexAcquire(track->video.mutex);

	frame->decompressing = false;
	frame->decompressed = true;
	frame->releasedQueueEntry = true;
	doneWithQueueEntry(frame->entry);

	if ((kFskErrNone == result) && (NULL != bits)) {
		frame->bits = bits;
		frame->compositionTime = frame->compositionTimeReturned;

		FskInstrumentedItemSetOwner(bits, track->state->player);
	}
	else {
		track->video.framesAtDecompressor -= 1;

//printf("framesAtDecompressor = %d (no bitmap)\n", track->video.framesAtDecompressor);
		if (kFskErrMemFull == result)
			track->state->error = result;
	}

	decompressVideoFrames(track);

	FskMutexRelease(track->video.mutex);
}

void flushVideoFrames(readerMediaPlayerTrack track)
{
	if (NULL != track->video.deco) {
		track->video.flushing += 1;
		FskImageDecompressFlush(track->video.deco);
		track->video.flushing -= 1;
	}

	flushVideoBefore(track, NULL);
}

void scheduleVideoDrawTimer(readerMediaPlayerTrack track, FskSampleTime time)
{
	readerMediaPlayerVideoFrameQueue frame, target = NULL;
	UInt32 delta;

	if ((NULL == track->state->module->port) ||
        (kFskMediaPlayerDrawingMethodComposite == track->state->module->requestedDrawingMethod))
		return;

	FskMutexAcquire(track->video.mutex);

	for (frame = track->video.frames; NULL != frame; frame = frame->next) {
		if ((frame->compositionTime < time) || ((NULL == frame->bits) && frame->decompressed) || frame->doDrop)
			continue;

		if (NULL == target) {
			target = frame;
			continue;
		}

		if (frame->compositionTime < target->compositionTime)
			target = frame;
	}

	if (NULL != target)
		delta = (UInt32)(((((double)(target->compositionTime - time)) / (double)track->state->timeScale) * 1000.0) + 0.5);
	else
		delta = 100;		// nothing ready, so give it some time

	track->video.nextFrameToDraw = target;

	FskMutexRelease(track->video.mutex);
//if (target)
//	printf("schedule %d \n", target->sequenceNumber);
	FskTimeCallbackScheduleFuture(track->video.drawTimer, 0, delta, videoDrawCallback, track);
}

void videoDrawCallback(FskTimeCallBack callback, const FskTime timeIn, void *param)
{
	readerMediaPlayerTrack track = param;
	readerMediaPlayerModule state = track->state;
	FskMediaPlayerModule module = state->module;
	FskSampleTime time;

	doUsingMediaPlayer(state, true);

	if (NULL != track->video.nextFrameToDraw)
		time = track->video.nextFrameToDraw->compositionTime;
	else
		FskMediaPlayerGetTime(module->player, (float)state->timeScale, &time);

	if (true == syncVideoBitmap(track, time, false)) {
#if TARGET_OS_WIN32
		if (track->video.directToScreenFailed && state->module->window && (track->video.updateSeed != state->module->window->updateSeed)) {
			track->video.directToScreenFailed = false;
			track->video.updateSeed = state->module->window->updateSeed;
		}
#endif

		if ((kFskMediaPlayerDrawingMethodDirect == module->requestedDrawingMethod) && !track->video.directToScreenFailed && (0 == track->video.rotation)) {
			FskErr err;

			track->video.canDirect++;
				err = readerMediaPlayerModuleUpdate(state, module);
			track->video.canDirect--;

			if (kFskErrNothingRendered == err) {
				track->video.directToScreenFailed = true;
				sendUpdate(state);
			}
		}
		else
			sendUpdate(state);
	}

	//fprintf( stderr, "videoDrawCallback: %d\n", module->requestedDrawingMethod);
	track->video.last_requestedDrawingMethod = track->video.this_requestedDrawingMethod ;
	track->video.this_requestedDrawingMethod = module->requestedDrawingMethod;

	if(
		track->video.last_requestedDrawingMethod == kFskMediaPlayerDrawingMethodComposite &&
		track->video.this_requestedDrawingMethod == kFskMediaPlayerDrawingMethodDirect
	   )
	   track->video.back_to_screen_count = kBackToScreenCount;

	decompressVideoFrames(track);

	FskMediaPlayerGetTime(module->player, (float)state->timeScale, &time);		// refresh this here, so it is as accurate as possible
	scheduleVideoDrawTimer(track, time);

	doUsingMediaPlayer(state, false);
}

Boolean syncVideoBitmap(readerMediaPlayerTrack track, FskSampleTime time, Boolean wait)
{
	Boolean result = false;
	readerMediaPlayerVideoFrameQueue frame, target = NULL;
	Boolean didEOS = false;

	FskMutexAcquire(track->video.mutex);

	for (frame = track->video.frames; NULL != frame; frame = frame->next) {
		if (frame->compositionTime > time)
			continue;

		if ((false == wait) && !frame->decompressed)
			continue;			// no waiting, so cannot select frame that that is not ready

		if (NULL == target) {
			target = frame;
			continue;
		}

		if (frame->compositionTime > target->compositionTime)
			target = frame;
	}

	if (NULL == target)
		goto bail;

	while (false == target->decompressed) {
		FskErr err;

		FskMutexRelease(track->video.mutex);

		err = decompressVideoFrames(track);
		FskThreadYield();
		FskMutexAcquire(track->video.mutex);

		flushVideoBefore(track, &target->decodeTime);

		if (kFskErrNone != err)
			goto bail;

		if (!didEOS && target->decompressing) {
			didEOS = true;
			FskImageDecompressFrame(track->video.deco, NULL, 0, NULL, true,
				videoTrackDecompressComplete, NULL, NULL, NULL,
				NULL, NULL, 0);
		}
	}

	if (NULL == target->bits)
		goto flush;

	if (track->video.bits) {
#if FSKBITMAP_OPENGL
		if (track->state->usingGL)
			FskGLPortBitmapSwap(track->video.bits, target->bits, false);	// This transfers the GL texture from track->video.bits to target->bits
#endif
		FskBitmapDispose(track->video.bits);
	}
	track->video.bits = target->bits;
#if FSKBITMAP_OPENGL
	if (track->state->usingGL)
		FskBitmapSetOpenGLSourceAccelerated(track->video.bits, true);		// This only sets a bit, but does not upload the bitmap to the texture
#endif
	track->video.bitsTime = target->compositionTime;
	target->bits = NULL;
	target->played = true;
	result = true;
	track->video.framesAtDecompressor -= 1;
//printf("framesAtDecompressor = %d (sync)\n", track->video.framesAtDecompressor);
	decompressVideoFrames(track);
	
flush:
	flushVideoBefore(track, &target->decodeTime);

bail:
	FskMutexRelease(track->video.mutex);

	return result;
}

void flushVideoBefore(readerMediaPlayerTrack track, const FskSampleTime *timeIn)
{
	readerMediaPlayerVideoFrameQueue frame, next;

	for (frame = track->video.frames; NULL != frame; frame = next) {
		next = frame->next;

		if (true == frame->decompressing)
			break;

		if (NULL != timeIn) {
			if (false == frame->decompressed)
				break;

			if (frame->compositionTime >= *timeIn)
				continue;
		}

		FskListRemove((FskList*)(void*)(&track->video.frames), frame);

		if (NULL != frame->bits) {
			FskBitmapDispose(frame->bits);
			track->video.framesAtDecompressor -= 1;
//printf("framesAtDecompressor = %d (flush before)\n", track->video.framesAtDecompressor);
		}

		if (frame == track->video.nextFrameToDraw)
			track->video.nextFrameToDraw = NULL;

		if (false == frame->releasedQueueEntry)
			doneWithQueueEntry(frame->entry);

		update_history( track, frame );

		FskMemPtrDispose(frame);
	}
}

void sendUpdate(readerMediaPlayerModule state)
{
	if (state->module->eventHandler) {
		FskEvent e = state->updateEvent;

		if (NULL == e) {
			if (kFskErrNone != FskEventNew(&state->updateEvent, kFskEventWindowUpdate, NULL, 0))
				return;

			FskEventParameterAdd(state->updateEvent, kFskEventParameterUpdateRectangle, sizeof(state->module->windowBounds), &state->module->windowBounds);

			e = state->updateEvent;
		}

		(state->module->eventHandler)(state->player, state->module->eventHandlerRefCon, kFskEventWindowUpdate, e);
	}
}

void sendWarning(readerMediaPlayerModule state, const char *msg, const char *detail)
{
	FskEvent e;
	UInt32 listCount;
	char *list;

	if (!state->module->eventHandler)
		return;

	if (kFskErrNone != FskEventNew(&e, kFskEventMediaPlayerWarning, NULL, 0))
		return;

	listCount = FskStrLen(msg) + 3 + FskStrLen(detail);
	if (kFskErrNone != FskMemPtrNewClear(listCount, (FskMemPtr*)(void*)(&list))) {
		FskEventDispose(e);
		return;
	}
	FskStrCopy(list, msg);
	if (detail)
		FskStrCopy(list + FskStrLen(msg) + 1, detail);

	FskEventParameterAdd(e, kFskEventParameterFileList, listCount, list);

	(state->module->eventHandler)(state->player, state->module->eventHandlerRefCon, kFskEventMediaPlayerWarning, e);

	FskMemPtrDispose(list);
	FskEventDispose(e);
}

void playerFailed(readerMediaPlayerModule state, FskErr err)
{
	state->error = err;

	FskHTTPClientDispose(state->http.client);
	state->http.request = NULL;
	state->http.client = NULL;

	doUsingMediaPlayer(state, true);

	FskMediaPlayerStop(state->player);
	FskMediaPlayerSetPlayState(state->player, kFskMediaPlayerStateFailed);

	doUsingMediaPlayer(state, false);
}

void checkStart(readerMediaPlayerModule state)
{
	if (state->maxTimeExtracted >= state->bufferToTime) {
		doUsingMediaPlayer(state, true);

		if (kFskErrNone == FskMediaPlayerSendEvent(state->player, kFskEventMediaPlayerReady))
			FskMediaPlayerStart(state->player);

		doUsingMediaPlayer(state, false);
	}
}

FskErr rotateBitmap(FskBitmap input, FskBitmap *output, SInt32 degrees)
{
	return kFskErrUnimplemented;
}

void afterStopCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	readerMediaPlayerModule state = param;
	readerMediaPlayerTrack track;

	for (track = state->tracks; NULL != track; track = track->next) {
		if (kFskMediaPlayerReaderAudio == track->mediaFormat) {
			if (NULL != track->audio.sndChan) {
				doUsingMediaPlayer(state, true);
				readerMediaPlayerModuleAbort((void*)track, (void*)kFskErrAudioOutReset, NULL, NULL);
			}
		}
	}
}

void emptyBuffersCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	readerMediaPlayerModule state = param;

	if (!state->playing || state->atEnd)
		return;

	state->doRestart = true;
}

static FskErr httpHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon);
static FskErr httpClientFinished(FskHTTPClient client, void *refCon);
static FskErr httpAuthenticate(FskHTTPClient client, FskHTTPClientRequest request, char *url, char *realm, FskHTTPAuth auth, void *refCon);
static FskErr httpDataReceived(FskHTTPClientRequest request, char *buffer, int bufferSize, void *refCon);
static FskErr httpFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);

FskErr rmpHTTPSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;
	const char *userAgent;
	Boolean doSetPosition = true;
	char *uri = state->http.uri;

	err = allocateBuffers(state, 16);       // no fewer than 16 buffers
	BAIL_IF_ERR(err);
    
    updateBuffersForBitrate(state);         // but maybe more

	state->http.headersReceived = false;
	state->http.contentLength = 0;
	state->http.suspended = false;
	state->http.requestHadPosition = false;

	err = FskHTTPClientNew(&state->http.client, "media reader");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(state->http.client, state);

	FskHTTPClientSetIdleTimeout(state->http.client, 60);			// after 60 seconds, drop the connection
	FskHTTPClientSetFinishedCallback(state->http.client, httpClientFinished);

	FskHTTPClientSetAuthCallback(state->http.client, httpAuthenticate);
	if (state->http.user && state->http.password)
		FskHTTPClientSetCredentials(state->http.client, state->http.user, state->http.password, 0, kFskHTTPAuthCredentialsTypeString);

	if (state->http.isDownload && state->http.downloadPath) {
		spooler->flags |= kFskMediaSpoolerDownloading;
		if (state->spool.fref || (kFskErrNone == FskFileOpen(state->http.downloadPath, kFskFilePermissionReadWrite, &state->spool.fref)))
			state->http.position = state->http.downloadPosition;
		else
			state->http.downloadPosition = 0;
	}

	if (state->spooler.onSpoolerCallback) {
		FskMediaSpoolerGetURIRecord getURI;
		getURI.uriIn = uri;
		getURI.position = state->http.position;
		getURI.uriOut = NULL;
		getURI.doSetPosition = true;
		if (kFskErrNone == (state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationGetURI, &getURI)) {
			if (getURI.uriOut)
				uri = getURI.uriOut;
			doSetPosition = getURI.doSetPosition;
			state->http.position = getURI.position;
		}
	}

	err = FskHTTPClientRequestNew(&state->http.request, uri);
	if (uri != state->http.uri)
		FskMemPtrDispose(uri);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(state->http.request, state);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(state->http.request, httpHeadersReceived, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(state->http.request, httpDataReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(state->http.request, httpFinished);

	userAgent = FskEnvironmentGet("http-user-agent");
	if (userAgent)
		FskHTTPClientRequestAddHeader(state->http.request, "User-Agent", (char *)userAgent);

    if ((state->spooler.flags & kFskMediaSpoolerTimeSeekSupported) && (state->spooler.flags & kFskMediaSpoolerUseTimeSeek)) {
        char num[64];

        FskStrDoubleToStr((double)state->zeroMovieTime / (double)state->timeScale, num, sizeof(num), "npt=%.3f-");
        FskHTTPClientRequestAddHeader(state->http.request, "TimeSeekRange.dlna.org", num);
        state->http.position = 0;
    }
	else if ((0 != state->http.position) && doSetPosition && (state->spooler.flags & kFskMediaSpoolerByteSeekSupported)) {
		char value[50];
		FskStrCopy(value, "bytes=");
		FskStrNumToStr((UInt32)state->http.position, value + FskStrLen(value), sizeof(value) - FskStrLen(value));		//@@ wrong when position > 4GB
		FskStrCat(value, "-");
		FskHTTPClientRequestAddHeader(state->http.request, "Range", value);

		state->http.requestHadPosition = true;
	}

	if (state->http.additionalHeaders) {
		char *p = state->http.additionalHeaders;
		while (*p) {
			char *name, *value;

			name = p;
			p += FskStrLen(p) + 1;
			if (!*p) break;
			value = p;
			p += FskStrLen(p) + 1;

			FskHTTPClientRequestAddHeader(state->http.request, name, value);
		}
	}

	if (state->spooler.onSpoolerCallback)
		(state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationSetHeaders, state->http.request->requestHeaders);

	err = FskHTTPClientBeginRequest(state->http.client, state->http.request);
	if (err) {
		FskHTTPClientRequestDispose(state->http.request);
		state->http.request = NULL;
		goto bail;
	}

	state->spoolerOpen = true;

bail:
	return err;
}

FskErr rmpHTTPSpoolerClose(FskMediaSpooler spooler)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;

	FskHTTPClientDispose(state->http.client);

	state->http.client = NULL;
	state->http.request = NULL;

	state->http.position = 0;
	state->http.done = false;

	state->spoolerOpen = false;

	disposeBuffers(state);

	return err;
}

FskErr rmpHTTPSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **dataOut, UInt32 *bytesRead)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;
	readerMediaPlayerNetBuffer buffer;
	UInt32 bytesAvailable = 0;
	SInt32 gap;

	if (state->http.isDownload) {
		err = rmpFileSpoolerRead(spooler, position, bytesToRead, dataOut, bytesRead);
		if (kFskErrEndOfFile == err)
			err = kFskErrNeedMoreTime;
		return err;
	}

	for (buffer = state->spool.buffers; NULL != buffer; buffer = buffer->next) {
		if ((buffer->position <= position) && (position < (buffer->position + buffer->bytes)))
			break;
	}

//printf("spooler request from %d to %d\n", (int)position, (int)(position + bytesToRead));
	if (NULL == buffer) {
		if (NULL != state->http.client) {
			if (state->http.done && (position >= state->http.position))
				return kFskErrEndOfFile;

			if ((state->http.position <= position) && (position <= (state->http.position + (state->spool.bufferSize * (state->spool.bufferCount >> 1))))) {
				err = kFskErrNeedMoreTime;			// we're close, wait for it.
				goto checkResume;
			}

			if (spooler->flags & kFskMediaSpoolerDontSeekIfExpensive)
				return kFskErrNeedMoreTime;

			if (state->http.isDownloadFailed)
				return kFskErrNeedMoreTime;
		}

		err = rmpHTTPSpoolerRestart(state, position);
		if (err) return err;

		return kFskErrNeedMoreTime;
	}

	bytesAvailable = (UInt32)((buffer->position + buffer->bytes) - position);
	if (bytesAvailable >= bytesToRead) {
		*dataOut = buffer->data + (position - buffer->position);
		if (NULL != bytesRead)
			*bytesRead = bytesToRead;
//printf("spooler returned from %d to %d\n", (int)position, (int)(position + bytesToRead));
		return kFskErrNone;
	}

	if (NULL == bytesRead) {
//printf("spooler straddle at %d\n", (int)(position));
		return kFskErrNeedMoreTime;
	}

	*dataOut = buffer->data + (position - buffer->position);
	*bytesRead = bytesAvailable;
//printf("spooler returned from %d to %d\n", (int)position, (int)(position + bytesAvailable));

checkResume:
	state->http.lastRequestedPosition = position + bytesAvailable;
	gap = (SInt32)(state->http.position - state->http.lastRequestedPosition);
	if (((state->spooler.flags & kFskMediaSpoolerForwardOnly) && (gap <= (SInt32)((state->spool.bufferCount - 1) * kBufferSize))) ||
		(!(state->spooler.flags & kFskMediaSpoolerForwardOnly) && (gap <= kBufferSize))) {
		if (true == state->http.suspended) {
			state->http.suspended = false;
			FskHTTPClientResume(state->http.client);
		}
	}

	return err;
}

FskErr rmpHTTPSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;

	if (state->http.downloadDataSize && !state->http.contentLength) {
		*size = state->http.downloadDataSize;
		goto bail;
	}

	if (false == state->http.headersReceived) {
		err = kFskErrNeedMoreTime;
		goto bail;
	}

	*size = state->http.contentLength;

bail:
	return err;
}

FskErr rmpHTTPSpoolerRestart(readerMediaPlayerModule state, FskInt64 position)
{
	rmpHTTPSpoolerClose(&state->spooler);

	if (state->http.noSeeking)
		position = 0;

	state->http.position = position;
	state->http.lastRequestedPosition = position;

	return rmpHTTPSpoolerOpen(&state->spooler, kFskFilePermissionReadOnly);
}

#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY

static FskErr mpegFragmentsRefreshM3U(readerMediaPlayerModule state);

FskErr rmpMPEGFragmentsSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err;

	err = mpegFragmentsRefreshM3U(state);
	BAIL_IF_ERR(err);

	state->spoolerOpen = true;

bail:
	return err;
}

FskErr rmpMPEGFragmentsSpoolerClose(FskMediaSpooler spooler)
{
	readerMediaPlayerModule state = spooler->refcon;
//@@
	FskHTTPClientRequestDispose(state->frag.streamRequest);
	FskHTTPClientDispose(state->frag.streamClient);
	state->frag.streamRequest = NULL;
	state->frag.streamClient = NULL;

	FskHTTPClientRequestDispose(state->frag.m3uRequest);
	FskHTTPClientDispose(state->frag.m3uClient);
	state->frag.m3uRequest = NULL;
	state->frag.m3uClient = NULL;

	disposeBuffers(state);

	state->frag.m3uBytes = 0;
	FskMemPtrDisposeAt(&state->frag.m3u);

	while (state->frag.fragment) {
		readerMediaPlayerFragment next = state->frag.fragment->next;
		FskMemPtrDispose(state->frag.fragment);
		state->frag.fragment = next;
	}

	state->spoolerOpen = false;

	return kFskErrNone;
}

FskErr rmpMPEGFragmentsSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **dataOut, UInt32 *bytesRead)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;
	readerMediaPlayerNetBuffer buffer;
	UInt32 bytesAvailable = 0;
	SInt32 gap;

	for (buffer = state->spool.buffers; NULL != buffer; buffer = buffer->next) {
		if ((buffer->position <= position) && (position < (buffer->position + buffer->bytes)))
			break;
	}

	if (NULL == buffer) {
		if (state->atEnd)
			return kFskErrEndOfFile;
		return kFskErrNeedMoreTime;
	}

	bytesAvailable = (UInt32)((buffer->position + buffer->bytes) - position);
	if (bytesAvailable >= bytesToRead) {
		*dataOut = buffer->data + (position - buffer->position);
		if (NULL != bytesRead)
			*bytesRead = bytesToRead;
		return kFskErrNone;
	}

	if (NULL == bytesRead)
		return kFskErrNeedMoreTime;

	*dataOut = buffer->data + (position - buffer->position);
	*bytesRead = bytesAvailable;

	state->http.lastRequestedPosition = position + bytesAvailable;
	gap = (SInt32)(state->http.position - state->http.lastRequestedPosition);
	if ((state->spooler.flags & kFskMediaSpoolerForwardOnly) && (gap <= (SInt32)((state->spool.bufferCount - 1) * kBufferSize))) {
		if (true == state->http.suspended) {
			state->http.suspended = false;
			FskHTTPClientResume(state->frag.streamClient);
		}
	}

	return err;
}

FskErr rmpMPEGFragmentsSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size)
 {
	*size = 0;

	return kFskErrNone;
}
#endif

FskErr rmpNULLSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions)
{
	return kFskErrUnimplemented;
}

FskErr rmpNULLSpoolerClose(FskMediaSpooler spooler)
{
	return kFskErrUnimplemented;
}

FskErr rmpNULLSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead)
{
	return kFskErrUnimplemented;
}

FskErr rmpNULLSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size)
{
	return kFskErrUnimplemented;
}

FskErr rmpSpoolerFlushBuffer(FskMediaSpooler spooler, readerMediaPlayerNetBuffer buffer)
{
	readerMediaPlayerModule state = spooler->refcon;
	FskErr err = kFskErrNone;

	if (buffer->position != state->spool.position) {
		err = FskFileSetPosition(state->spool.fref, &buffer->position);
		BAIL_IF_ERR(err);

		state->spool.position = buffer->position;
	}

	err = FskFileWrite(state->spool.fref, buffer->bytes, buffer->data, NULL);
	BAIL_IF_ERR(err);

	state->spool.position += buffer->bytes;

	buffer->dirty = false;

bail:
	return err;
}

FskErr allocateBuffers(readerMediaPlayerModule state, UInt32 count)
{
	FskErr err = kFskErrNone;

	state->spool.bufferSize = kBufferSize;

	while (count--) {
		readerMediaPlayerNetBuffer buffer;
		Boolean large = true;

		err = FskMemPtrNewLarge(sizeof(readerMediaPlayerNetBufferRecord), (FskMemPtr*)(void*)(&buffer));
		if (err) {
			err = FskMemPtrNew(sizeof(readerMediaPlayerNetBufferRecord), &buffer);
			BAIL_IF_ERR(err);

			large = false;
		}

		FskMemSet(buffer, 0, sizeof(readerMediaPlayerNetBufferRecord) - kBufferSize);
		buffer->large = large;

		FskListPrepend((FskList*)(void*)(&state->spool.buffers), buffer);
        state->spool.bufferCount += 1;
	}

bail:
	return err;
}

void disposeBuffers(readerMediaPlayerModule state)
{
	while (state->spool.buffers) {
		readerMediaPlayerNetBuffer buffer = FskListRemoveFirst((FskList*)(void*)(&state->spool.buffers));
		if (buffer->large)
			FskMemPtrDisposeLarge((FskMemPtr)buffer);
		else
			FskMemPtrDispose(buffer);
	}

	state->http.downloadBuffer = NULL;
    state->spool.bufferCount = 0;
}

void checkBufferAllocations(readerMediaPlayerModule state)
{
    UInt32 buffersNeeded;

    if ((0 == state->http.contentLength) || !state->spool.keepLocal)
        return;

    buffersNeeded = (state->http.contentLength + state->spool.bufferSize - 1) / state->spool.bufferSize;
    if (buffersNeeded > state->spool.bufferCount)
        allocateBuffers(state, buffersNeeded - state->spool.bufferCount);
}

void updateBuffersForBitrate(readerMediaPlayerModule state)
{
    if (state->bitRate) {
        UInt32 bytesPerSecond = state->bitRate / 8;
        UInt32 pessimisticBytesPerSecond = bytesPerSecond + (bytesPerSecond >> 2);
        UInt32 totalBufferSize = pessimisticBytesPerSecond * 4;
        UInt32 buffersNeeded = 1 + (totalBufferSize / kBufferSize);
        if (buffersNeeded > state->spool.bufferCount)
            allocateBuffers(state, buffersNeeded - state->spool.bufferCount);
    }
}

FskErr getFreeBuffer(readerMediaPlayerModule state, readerMediaPlayerNetBuffer *bufferOut)
{
	readerMediaPlayerNetBuffer buffer, walker;
	FskErr err = kFskErrNone;

	for (buffer = state->spool.buffers, walker = buffer->next; NULL != walker; walker = walker->next) {
		if (walker->lastUsed < buffer->lastUsed)
			buffer = walker;
	}

	if (buffer == state->http.downloadBuffer)
		state->http.downloadBuffer = NULL;

	if (buffer->dirty) {
		err = rmpSpoolerFlushBuffer(&state->spooler, buffer);
		BAIL_IF_ERR(err);
	}

	buffer->bytes = 0;

bail:
	*bufferOut = buffer;

	return err;
}

FskErr httpHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err = kFskErrNone;
	char *value, *contentLength, *features;
	SInt32 responseCode = FskHeaderResponseCode(responseHeaders);

	// handle continue
	if (100 == responseCode)
		return kFskErrOperationCancelled;
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
				char *t = FskStrDoCat(state->http.request->parsedUrl->scheme, "://");
				location = FskStrDoCat(t, redirect);		// no protocol given, so use the current one
				FskMemPtrDispose(t);
			}
		}
		else {
			if (kFskErrNone == FskMemPtrNew(FskStrLen(state->http.request->parsedUrl->scheme) + 4 + FskStrLen(state->http.request->parsedUrl->host) + FskStrLen(redirect), &location)) {
				FskStrCopy(location, state->http.request->parsedUrl->scheme);
				FskStrCat(location, "://");
				FskStrCat(location, state->http.request->parsedUrl->host);
				FskStrCat(location, redirect);
			}
		}

		if (state->uri != state->http.uri)
			FskMemPtrDispose(state->http.uri);
		state->http.uri = location;

		FskMemPtrDisposeAt(&state->http.additionalHeaders);		// conservative: cannot send headers across domain. ultimately need an event to allow client to provide correct headers.
		FskMediaPlayerSendEvent(state->player, kFskEventMediaPlayerHeaders);

		err = rmpHTTPSpoolerRestart(state, 0);		// in theory, we don't have to toss the client here when the new URL is on the same host/port/protocol as the original request
		if (err) return err;

		return kFskErrOperationCancelled;
	}

#if 1
	// the rush limbaugh subscriber podcast times out the redirected URI quickly (one minute), but not the original URI. so, treat the failure as a redirect to the original URI.
	if ((403 == responseCode) && (0 == FskStrCompareWithLength("download.premiereradio.net", request->parsedUrl->host, 26))) {
		if (state->uri != state->http.uri)
			FskMemPtrDispose(state->http.uri);
		state->http.uri = state->uri;

		err = rmpHTTPSpoolerRestart(state, 0);
		if (err) return err;

		return kFskErrOperationCancelled;
	}
#endif

	doUsingMediaPlayer(state, true);

	state->http.headersReceived = true;

	contentLength = FskHeaderFind(kFskStrContentLength, responseHeaders);
	if (NULL != contentLength) {
		state->http.contentLength = FskStrToNum(contentLength);
        checkBufferAllocations(state);
    }

	// if we requested a seek, make sure that the server delivered that
	if ((200 == responseCode) && !state->http.requestHadPosition)
		;			// GET with no byte range
	else
	if (206 == responseCode)
		state->http.contentLength += state->http.position;			// GET with byte range
	else
	if ((200 == responseCode) && state->http.requestHadPosition && (0 != FskStrStr(state->http.uri, "audible.com")))
		state->http.contentLength += state->http.position;			// GET with byte range from Audible - they return 200 by mistake...
	else {
		state->http.noSeeking = true;			// we've been surprised with a response that indicates seeking is impossible, even though we thought it was...

		if (state->http.isDownload) {
			playerFailed(state, (responseCode && ((responseCode / 100) == 2)) ? kFskErrUnsupportedSeek : responseCode);
			doUsingMediaPlayer(state, false);

			return kFskErrNone;
		}

		if (416 == responseCode)
			responseCode = 200;
	}

	if (!responseCode || (2 != (responseCode / 100))) {
		playerFailed(state, responseCode ? responseCode : kFskErrBadData);
		doUsingMediaPlayer(state, false);

		return kFskErrNone;
	}

	state->spooler.flags |= kFskMediaSpoolerByteSeekSupported;
    // see if DLNA time seeking is supported
    features = FskHeaderFind("ContentFeatures.DLNA.ORG", responseHeaders);
    if (features) {
		char *flags = FskStrStr(features, "DLNA.ORG_FLAGS=");
		char *op = FskStrStr(features, "DLNA.ORG_OP=");
		state->http.canDLNATimeSeek = false;
		state->spooler.flags &= ~(kFskMediaSpoolerTimeSeekSupported | kFskMediaSpoolerUseTimeSeek);
		state->http.canDLNATimeSeek = flags ? (4 & flags[15]) : (op ? ('1' == op[12]) : false);
		if (state->http.canDLNATimeSeek)
			state->spooler.flags |= kFskMediaSpoolerTimeSeekSupported;
		if (!(flags ? (2 & flags[15]) : (op ? ('1' == op[13]) : false)))
			state->spooler.flags &= ~kFskMediaSpoolerByteSeekSupported;
	}

	// is this a download request?
	if (!state->http.isDownload && !state->http.isDownloadFailed) {
		value = FskHeaderFind("Accept-Ranges", responseHeaders);
		if (NULL != value) {
			if ((FskStrLen(request->parsedUrl->host) > 8) &&
				(0 == FskStrCompareWithLength(".npr.org", request->parsedUrl->host + FskStrLen(request->parsedUrl->host) - 8, 8)))
				;		// they support seeking - they just don't admit it...
			else
			if ((FskStrLen(request->parsedUrl->host) > 12) &&
				(0 == FskStrCompareWithLength(".reuters.com", request->parsedUrl->host + FskStrLen(request->parsedUrl->host) - 12, 12)))
				;		// they support seeking - they just don't admit it...
			else {
				if (NULL != FskStrStr(value, "none"))
					state->http.noSeeking = true;
				if (NULL == FskStrStr(value, "bytes"))
					state->http.noSeeking = true;
			}
		}
		else {
#if 0
			// if there's no Accept-ranges header and there is download implying content-disposition header, we revert to download
			value = FskHeaderFind("Content-Disposition", responseHeaders);
			if (NULL != value) {
				if (NULL != FskStrStr(value, "attachment"))
					state->http.noSeeking = true;
				if (NULL != FskStrStr(value, "inline"))
					state->http.noSeeking = true;
			}
#endif
			if (contentLength) {
				if (0 == FskStrCompare(responseHeaders->protocol, "HTTP/1.0"))
					state->http.noSeeking = true;								// no range requests in 1.0, unless we see an accept-ranges
			}
		}
	}

	if (state->spooler.onSpoolerCallback)
		(state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationGetHeaders, responseHeaders);

	if (kFskMediaSpoolerCantSeek & state->spooler.flags)
		state->http.noSeeking = true;

	if ((kFskMediaSpoolerDownloadPreferred & state->spooler.flags) && !state->http.isDownloadFailed)
		state->http.isDownload = true;

	if ((kFskMediaSpoolerDontDownload & state->spooler.flags) && (1 != (3 & state->http.downloadPreference)))	// client wins tie - mosty because of audible server bug workaround
		state->http.isDownload = false;

	if (state->http.noSeeking &&										// can't byte seek
        !state->http.canDLNATimeSeek &&                                 // can't time seek  @@ this check is early, we don't know if reader can use this feature
		!state->http.isDownload &&										// haven't turned on download yet
		!state->http.isDownloadFailed &&								// haven't started download failed yet either
		(0 == ((kFskMediaSpoolerDontDownload | kFskMediaSpoolerDownloadPreferred) & state->spooler.flags)) &&		// spooler has no preference
		(0 == (3 & state->http.downloadPreference))) {					// client doesn't want download
		FskSampleTime now;

		// no download - live-ish
		state->http.isDownload = false;
		state->http.isDownloadFailed = true;

		state->http.position = 0;

		FskMediaPlayerGetTime(state->player, 1000, &now);
		if (0 != now)
			FskMediaPlayerSetTime(state->player, 1, 0);
	}

	// allocate the download scratch file
	if (state->http.isDownload && (NULL == state->spool.fref)) {
		char *tempDir = NULL, *tempPath = NULL, *contentLength;

		if (0 == FskStrCompare(FskEnvironmentGet("mediaplayerreader-downloadandplay"), "never")) {
			state->http.isDownload = false;
			state->http.isDownloadFailed = true;
			goto fail;
		}

		if (NULL == state->http.downloadPath) {
			err = FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeTemporary, true, NULL, &tempDir);
			if (err) goto fail;

			tempPath = FskStrDoCat(tempDir, "download_and_play_XXXX.dat");
			if (NULL == tempPath) {
				err = kFskErrMemFull;
				goto fail;
			}

			while (true) {
				char *x = FskStrRChr(tempPath, '_') + 1;
				FskFileInfo finfo;
				int i;

				for (i = 0; i < 4; i++)
					*x++ = (char)((FskRandom() % 10) + '0');

				if (kFskErrFileNotFound == FskFileGetFileInfo(tempPath, &finfo))
					break;
			}

			FskFileDelete(tempPath);
		}

		FskFileCreate(tempPath ? tempPath : state->http.downloadPath);
		err = FskFileOpen(tempPath ? tempPath : state->http.downloadPath, kFskFilePermissionReadWrite, &state->spool.fref);
		if (err) goto fail;

		state->spooler.flags |= kFskMediaSpoolerDownloading;

		contentLength = FskHeaderFind(kFskStrContentLength, responseHeaders);
		if (NULL != contentLength) {
			FskInt64 size = FskStrToNum(contentLength);
			if (size) {
				err = FskFileSetSize(state->spool.fref, &size);
				if (err) {
					FskFileClose(state->spool.fref);
					state->spool.fref = NULL;

					// revert to live behavior
					state->spooler.flags &= ~kFskMediaSpoolerDownloading;
					state->http.isDownload = false;
					state->http.isDownloadFailed = true;
					FskMemPtrDisposeAt(&tempPath);
					err = kFskErrNone;
				}
			}
		}

		if (state->http.downloadPosition != state->http.position)
			err = rmpHTTPSpoolerRestart(state, state->http.downloadPosition);

fail:
		if (err)
			playerFailed(state, err);
		else {
			if (tempPath)
				state->http.downloadPath = tempPath;
			tempPath = NULL;
		}

		FskMemPtrDispose(tempPath);
		FskMemPtrDispose(tempDir);
	}

	state->module->needsIdle = state->http.isDownload || state->reader->needsIdle;

	doUsingMediaPlayer(state, false);

	return err;
}

FskErr httpClientFinished(FskHTTPClient client, void *refCon)
{
	readerMediaPlayerModule state = refCon;

	if ((kFskErrTimedOut == client->status.lastErr) || (kFskErrNetworkInterfaceRemoved == client->status.lastErr) ||
		(kFskErrConnectionClosed == client->status.lastErr) || (kFskErrConnectionDropped == client->status.lastErr)) {
		FskHTTPClientDispose(state->http.client);
		state->http.request = NULL;
		state->http.client = NULL;
	}

	return kFskErrNone;
}

FskErr httpAuthenticate(FskHTTPClient client, FskHTTPClientRequest request, char *url, char *realm, FskHTTPAuth auth, void *refCon)
{
	readerMediaPlayerModule state = refCon;

	FskMemPtrDispose(state->http.realm);
	state->http.realm = FskStrDoCopy(realm);

	if (!state->http.user || !state->http.password)
		FskMediaPlayerSendEvent(state->player, kFskEventMediaPlayerAuthenticate);

	return kFskErrNone;
}

FskErr httpDataReceived(FskHTTPClientRequest request, char *data, int dataSize, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err = kFskErrNone;
	UInt32 bytesReceived = dataSize;

	doUsingMediaPlayer(state, true);

	if ((state->spooler.flags & kFskMediaSpoolerTransformReceivedData) && state->spooler.onSpoolerCallback) {
		FskMediaSpoolerTransformDataRecord xform;
		xform.data = data;
		xform.dataSize = dataSize;
		if (kFskErrNone == (state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationTransformData, (void *)&xform)) {
			data = xform.data;
			dataSize = xform.dataSize;
		}
	}

	while (0 != dataSize) {
		UInt32 bytesToUse;
		readerMediaPlayerNetBuffer buffer;
		SInt32 gap;

		if ((NULL == state->http.downloadBuffer) || (kBufferSize == state->http.downloadBuffer->bytes)) {
			err = getFreeBuffer(state, &state->http.downloadBuffer);
			BAIL_IF_ERR(err);		//@@ if this fails, probably disk full - we should go to failed state

			state->http.downloadBuffer->position = state->http.position;
		}
		buffer = state->http.downloadBuffer;

		bytesToUse = dataSize;
		if (bytesToUse > (kBufferSize - buffer->bytes))
			bytesToUse = kBufferSize - buffer->bytes;

		FskMemMove(&buffer->data[buffer->bytes], data, bytesToUse);

		data += bytesToUse;
		dataSize -= bytesToUse;
		buffer->bytes += bytesToUse;
		buffer->dirty = state->http.isDownload;
		buffer->lastUsed = ++state->spool.seed;

		state->http.position += bytesToUse;

		gap = (SInt32)(state->http.position - state->http.lastRequestedPosition);
		if (((state->spooler.flags & kFskMediaSpoolerForwardOnly) && (gap > (SInt32)(kBufferSize * (state->spool.bufferCount - 2)))) ||
			(!(state->spooler.flags & kFskMediaSpoolerForwardOnly) && (gap > (SInt32)((kBufferSize * (state->spool.bufferCount >> 1)))))) {
			if ((false == state->http.suspended) && !state->http.isDownload) {
				state->http.suspended = true;
				FskHTTPClientSuspend(state->http.client);
			}
		}
	}

	if (state->spooler.onSpoolerCallback)
		(state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationDataReady, (void *)bytesReceived);

	if (state->http.isDownload)
		state->module->idleFlags |= kFskMediaPlayerIdleChangePropertySeekableSegment | kFskMediaPlayerIdleChangePropertyDownloadPosition;
	else if (state->http.isDownloadFailed)
		state->module->idleFlags |= kFskMediaPlayerIdleChangePropertySeekableSegment;

	if (kFskMediaPlayerStateBuffering == state->module->playState) {
		state->prerolling += 1;
		if (kFskErrNone == ensureReader(state, (double)state->zeroMovieTime))
			checkStart(state);
		state->prerolling -= 1;
	}
	else
	if ((kFskMediaPlayerStateStopped == state->module->playState) && state->readerIsForUpdate)
		extractMoreCallback(NULL, NULL, state);		// when readerIsForUpdate

bail:
	doUsingMediaPlayer(state, false);

	return err;
}

FskErr httpFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err;

	state->http.done = true;
	state->http.request = NULL;

	if (kFskErrNone == client->status.lastErr) {
		httpDataReceived(request, NULL, 0, refCon);

		if (state->http.isDownload && state->http.downloadPath && state->spool.fref) {
			FskMediaPropertyValueRecord value;

			err = readerMediaPlayerModuleGetProperty(state, state->module, NULL, kFskMediaPropertyDownloadPosition, &value);
			if (err) {
				playerFailed(state, err);
				goto bail;
			}

			FskFileClose(state->spool.fref);
			state->spool.fref = NULL;

			if (16 & state->http.downloadPreference) {
				FskFileInfo finfo;

				err = FskFileGetFileInfo(state->http.downloadPath, &finfo);
				if (kFskErrNone == err) {
					finfo.flags &= ~kFileFileHidden;
					err = FskFileSetFileInfo(state->http.downloadPath, &finfo);
				}

				if (err) {
					playerFailed(state, err);
					goto bail;
				}
			}


			err = FskFileOpen(state->http.downloadPath, kFskFilePermissionReadOnly, &state->spool.fref);
			if (err)
				playerFailed(state, err);
		}
	}
	else
	if ((kFskErrTimedOut == client->status.lastErr) || (kFskErrNetworkInterfaceRemoved == client->status.lastErr) || (kFskErrConnectionClosed == client->status.lastErr)) {
		FskInt64 position = state->http.position;
		FskErr lastErr = client->status.lastErr;

		FskHTTPClientDispose(state->http.client);
		state->http.request = NULL;
		state->http.client = NULL;

		if ((kFskErrNetworkInterfaceRemoved == lastErr) && (state->module->playState > kFskMediaPlayerStateStopped))
			rmpHTTPSpoolerRestart(state, position);
	}
	else
		playerFailed(state, client->status.lastErr);

bail:
	state->module->needsIdle = state->reader->needsIdle;

	return kFskErrNone;
}

#if FSK_APPLICATION_PLAYDEV || FSK_APPLICATION_PLAY

static FskErr mpegFragmentsM3UReceived(FskHTTPClientRequest request, char *data, int dataSize, void *refCon);
static FskErr mpegFragmentsM3UFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);
static FskErr mpegFragmentsNextFragment(readerMediaPlayerModule state);
static FskErr mpegFragmentsStreamReceived(FskHTTPClientRequest request, char *data, int dataSize, void *refCon);
static FskErr mpegFragmentsStreamFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);

FskErr mpegFragmentsRefreshM3U(readerMediaPlayerModule state)
{
	FskErr err;
	char *userAgent;

	state->frag.m3uBytes = 0;
	FskMemPtrDisposeAt(&state->frag.m3u);

	err = FskHTTPClientNew(&state->frag.m3uClient, "media reader m3u");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(state->frag.m3uClient, state);

	err = FskHTTPClientRequestNew(&state->frag.m3uRequest, state->http.uri);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(state->frag.m3uRequest, state);
	FskHTTPClientRequestSetReceivedDataCallback(state->frag.m3uRequest, mpegFragmentsM3UReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(state->frag.m3uRequest, mpegFragmentsM3UFinished);

	userAgent = FskEnvironmentGet("http-user-agent");
	if (userAgent)
		FskHTTPClientRequestAddHeader(state->frag.m3uRequest, "User-Agent", (char *)userAgent);

	err = FskHTTPClientBeginRequest(state->frag.m3uClient, state->frag.m3uRequest);
	BAIL_IF_ERR(err);

bail:
	return err;
}

FskErr mpegFragmentsM3UReceived(FskHTTPClientRequest request, char *data, int dataSize, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err;

	err = FskMemPtrRealloc(dataSize + state->frag.m3uBytes + 1, &state->frag.m3u);
	BAIL_IF_ERR(err);

	FskMemMove(state->frag.m3u + state->frag.m3uBytes, data, dataSize);

	state->frag.m3uBytes += dataSize;
	state->frag.m3u[state->frag.m3uBytes] = 0;

bail:
	return err;
}

FskErr mpegFragmentsM3UFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err = kFskErrNone;
	char *c = state->frag.m3u;
	Boolean hasSequenceNumber = false;
	SInt32 sequenceNumber = 0;
	SInt32 targetDuration = 0;
	SInt32 bandwidth = 0, programID = 0;
	char *base = FskStrDoCopy(state->http.uri ? state->http.uri : state->uri), *slash;
	readerMediaPlayerFragment fragments = NULL;

	state->frag.m3uRequest = NULL;

	slash = FskStrRChr(base, '/');		//@@ won't always be correct
	if (slash) slash[1] = 0;

	while (c && *c) {
		char *start = c, *end = NULL;

		while (*c) {
			if ((10 == *c) || (13 == *c)) {
				end = c;
				*c++ = 0;
				if ((10 == *c) || (13 == *c))
					c++;
				break;
			}
			c++;
		}
		if (!end) break;

		if ('#' == *start) {
			if (0 == FskStrCompareWithLength(start, "#EXT-X-MEDIA-SEQUENCE:", 22)) {
				start += 22;
				while (' ' == *start)
					start++;
				sequenceNumber = FskStrToNum(start);
				hasSequenceNumber = true;
			}
			else
			if (0 == FskStrCompare(start, "#EXT-X-ENDLIST"))
				state->frag.m3uDone = true;
			else
			if (0 == FskStrCompareWithLength(start, "#EXT-X-TARGETDURATION:", 22)) {
				start += 22;
				while (' ' == *start)
					start++;
				targetDuration = FskStrToNum(start);
			}
			if (0 == FskStrCompareWithLength(start, "#EXT-X-STREAM-INF:", 18)) {
				start += 18;
				while (*start) {
					Boolean done = false;
					char *equal = FskStrChr(start, '=');
					char *comma = FskStrChr(start, ',');
					if (!comma) {
						comma = end;
						done = true;
					}
					while (' ' == *start)
						start++;
					if (!*start || !equal) break;

					*equal++ = 0;
					*comma = 0;
					if (0 == FskStrCompare(start, "BANDWIDTH"))
						bandwidth = FskStrToNum(equal);
					else if (0 == FskStrCompare(start, "PROGRAM-ID"))
						programID = FskStrToNum(equal);
					else if (0 == FskStrCompare(start, "CODECS"))
						;		// ISO codec format

					start = comma + 1;
					if (done) break;
				}
			}
		}
		else {
			char *uri = FskStrDoCopy(start);
			char *colon = FskStrChr(uri, ':'), *slash = FskStrChr(uri, '/');
			readerMediaPlayerFragment fragment;

			if (!colon || (slash && (slash < colon))) {
				char *t = FskStrDoCat(base, uri);
				FskMemPtrDispose(uri);
				uri = t;
			}

			err = FskMemPtrNewClear(sizeof(readerMediaPlayerFragmentRecord) + FskStrLen(uri) + 1, &fragment);
			if (err) {
				FskMemPtrDispose(uri);
				goto bail;
			}
			fragment->sequenceNumber = hasSequenceNumber ? sequenceNumber++ : 0;
			fragment->programID = programID;
			fragment->bandwidth = bandwidth;
			FskStrCopy(fragment->uri, uri);
			FskListAppend((FskList*)(void*)(&fragments), fragment);

			FskMemPtrDispose(uri);
		}
	}

	FskHTTPClientDispose(state->frag.m3uClient);
	state->frag.m3uClient = NULL;

	if (!fragments)
		;
	else
	if (!hasSequenceNumber && !targetDuration) {
		readerMediaPlayerFragment walker = fragments, best = NULL, smallest = NULL;

		for ( ; NULL != walker; walker = walker->next) {
			if (walker->bandwidth <= state->frag.bitRate) {
				if (!best || (best->bandwidth < walker->bandwidth))
					best = walker;
			}
			if (!smallest || (smallest->bandwidth > walker->bandwidth))
				smallest = walker;
		}

		if (state->uri != state->http.uri)
			FskMemPtrDispose(state->http.uri);
		state->http.uri = FskStrDoCopy(best ? best->uri : smallest->uri);

		mpegFragmentsRefreshM3U(state);
	}
	else {
		readerMediaPlayerFragment walker;
		UInt32 fragmentCount;

		while (state->frag.sequenceNumber && fragments && (fragments->sequenceNumber <= state->frag.sequenceNumber)) {
			readerMediaPlayerFragment next = fragments->next;
			FskMemPtrDispose(fragments);
			fragments = next;
		}

		if ((NULL != fragments) && (false == state->frag.m3uDone)) {
			for (fragmentCount = FskListCount(fragments) >> 1, walker = fragments; fragmentCount > 0; fragmentCount--)
				walker = walker->next;
			walker->reloadM3U = true;
		}

		state->frag.fragment = fragments;
		fragments = NULL;

		if (NULL == state->frag.streamRequest) {
			err = mpegFragmentsNextFragment(state);
			BAIL_IF_ERR(err);
		}
	}

bail:
	if (kFskErrNone != err) {
		FskHTTPClientDispose(state->frag.m3uClient);
		state->frag.m3uClient = NULL;
	}

	while (fragments) {
		readerMediaPlayerFragment next = fragments->next;
		FskMemPtrDispose(fragments);
		fragments = next;
	}

	FskMemPtrDispose(base);

	return err;
}

FskErr mpegFragmentsNextFragment(readerMediaPlayerModule state)
{
	FskErr err = kFskErrNone;
	readerMediaPlayerFragment fragment = NULL;
	char *userAgent;

	if (NULL == state->spool.buffers) {
		err = allocateBuffers(state, (512 * 1024) / kBufferSize);			//@@ could base this on bitrate, when known
		BAIL_IF_ERR(err);
	}

	fragment = state->frag.fragment;
	if (NULL == fragment) {
		if (false == state->frag.m3uDone)
			err = mpegFragmentsRefreshM3U(state);		// get next m3u
		goto bail;
	}

	state->frag.fragment = fragment->next;

	if (state->frag.streamClient) {
		FskHTTPClientRequestDispose(state->frag.streamRequest);
		FskHTTPClientDispose(state->frag.streamClient);
		state->frag.streamRequest = NULL;
		state->frag.streamClient = NULL;
	}

	state->frag.sequenceNumber = fragment->sequenceNumber;

	err = FskHTTPClientNew(&state->frag.streamClient, "media reader stream");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(state->frag.streamClient, state);

	err = FskHTTPClientRequestNew(&state->frag.streamRequest, fragment->uri);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(state->frag.streamRequest, state);
	FskHTTPClientRequestSetReceivedDataCallback(state->frag.streamRequest, mpegFragmentsStreamReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(state->frag.streamRequest, mpegFragmentsStreamFinished);

	userAgent = FskEnvironmentGet("http-user-agent");
	if (userAgent)
		FskHTTPClientRequestAddHeader(state->frag.streamRequest, "User-Agent", (char *)userAgent);

	err = FskHTTPClientBeginRequest(state->frag.streamClient, state->frag.streamRequest);
	BAIL_IF_ERR(err);

	if (fragment->reloadM3U)
		err = mpegFragmentsRefreshM3U(state);		// get next m3u

bail:
	FskMemPtrDispose(fragment);

	return err;
}

FskErr mpegFragmentsStreamReceived(FskHTTPClientRequest request, char *data, int dataSize, void *refCon)
{
	readerMediaPlayerModule state = refCon;
	FskErr err = kFskErrNone;
	UInt32 bytesReceived = dataSize;

	doUsingMediaPlayer(state, true);

	while (0 != dataSize) {
		UInt32 bytesToUse;
		readerMediaPlayerNetBuffer buffer;
		SInt32 gap;

		if ((NULL == state->http.downloadBuffer) || (kBufferSize == state->http.downloadBuffer->bytes)) {
			err = getFreeBuffer(state, &state->http.downloadBuffer);
			BAIL_IF_ERR(err);		//@@ if this fails, probably disk full - we should go to failed state

			state->http.downloadBuffer->position = state->http.position;
		}
		buffer = state->http.downloadBuffer;

		bytesToUse = dataSize;
		if (bytesToUse > (kBufferSize - buffer->bytes))
			bytesToUse = kBufferSize - buffer->bytes;

		FskMemMove(&buffer->data[buffer->bytes], data, bytesToUse);

		data += bytesToUse;
		dataSize -= bytesToUse;
		buffer->bytes += bytesToUse;
		buffer->dirty = false;
		buffer->lastUsed = ++state->spool.seed;

		state->http.position += bytesToUse;

		gap = (SInt32)(state->http.position - state->http.lastRequestedPosition);
		if (gap > (SInt32)(kBufferSize * (state->spool.bufferCount - 2))) {
			if ((false == state->http.suspended) && !state->http.isDownload) {
				state->http.suspended = true;
				FskHTTPClientSuspend(state->frag.streamClient);
			}
		}
	}

	if (state->spooler.onSpoolerCallback)
		(state->spooler.onSpoolerCallback)(state->spooler.clientRefCon, kFskMediaSpoolerOperationDataReady, (void *)bytesReceived);

	if (kFskMediaPlayerStateBuffering == state->module->playState) {
		state->prerolling += 1;
		if (kFskErrNone == ensureReader(state, (double)state->zeroMovieTime))
			checkStart(state);
		state->prerolling -= 1;
	}
	else
	if ((kFskMediaPlayerStateStopped == state->module->playState) && state->readerIsForUpdate)
		extractMoreCallback(NULL, NULL, state);		// when readerIsForUpdate

bail:
	doUsingMediaPlayer(state, false);

	return err;
}

FskErr mpegFragmentsStreamFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	readerMediaPlayerModule state = refCon;

	state->http.suspended = false;
	state->frag.streamRequest = NULL;	//@@ check with mike

	mpegFragmentsNextFragment(state);

	return kFskErrNone;
}

#endif

#if SUPPORT_RECORD

static FskErr recordExtractedWrite(FskMuxer muxer, void *refCon, const void *data, FskFileOffset offset, UInt32 dataSize);

FskErr recordExtracted(readerMediaPlayerModule state, FskMediaReaderTrack track, UInt32 infoCount, FskMediaReaderSampleInfo readerInfo, unsigned char *data)
{
	FskErr err = kFskErrNone;
	readerMediaPlayerTrack walker;
	UInt32 i;
	FskMuxerSampleInfo writerInfo = NULL;

	if (state->record.failed)
		return kFskErrNone;

	if (NULL == state->record.muxer) {
		err = FskMuxerNew(&state->record.muxer, state->record.mime, state->timeScale, recordExtractedWrite, state);
		BAIL_IF_ERR(err);

		for (walker = state->tracks; NULL != walker; walker = walker->next) {
			FskMediaPropertyValueRecord scale = {0}, format = {0}, formatInfo = {0}, dimension = {0}, sampleRate = {0}, channelCount = {0};
			Boolean haveScale;
			const char *media;

			if (kFskMediaPlayerReaderAudio == walker->mediaFormat)
				media = "sound";
			else if (kFskMediaPlayerReaderVideo == walker->mediaFormat)
				media = "video";
			else
				continue;

			haveScale = kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertyTimeScale, &scale);

			err = FskMuxerTrackNew(state->record.muxer, &walker->muxerTrack, media, haveScale ? scale.value.integer : state->timeScale);
			BAIL_IF_ERR(err);

			if (kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertyFormat, &format)) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyFormat, &format);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);
			}

			if (kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertyFormatInfo, &formatInfo)) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyFormatInfo, &formatInfo);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);
			}

			if (haveScale) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyTimeScale, &scale);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);
			}

			if (kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertyDimensions, &dimension)) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyDimensions, &dimension);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);
			}

			if (kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertySampleRate, &sampleRate)) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertySampleRate, &sampleRate);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);

				if (!haveScale) {
					err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyTimeScale, &sampleRate);
					if (kFskErrUnimplemented == err) err = kFskErrNone;
					BAIL_IF_ERR(err);
				}
			}

			if (kFskErrNone == FskMediaReaderTrackGetProperty(walker->readerTrack, kFskMediaPropertyChannelCount, &channelCount)) {
				err = FskMuxerTrackSetProperty(walker->muxerTrack, kFskMediaPropertyChannelCount, &channelCount);
				if (kFskErrUnimplemented == err) err = kFskErrNone;
				BAIL_IF_ERR(err);
			}

			FskMediaPropertyEmpty(&format);
			FskMediaPropertyEmpty(&formatInfo);
		}

		err = FskMuxerStart(state->record.muxer);
		BAIL_IF_ERR(err);
	}

	for (walker = state->tracks; NULL != walker; walker = walker->next) {
		if (walker->readerTrack == track)
			break;
	}

	if ((NULL == walker) || (NULL == walker->muxerTrack))
		goto bail;

	if (NULL != walker->holdData) {
		walker->holdInfo.sampleDuration = (UInt32)((readerInfo[0].decodeTime - walker->holdDecodeTime) / walker->holdInfo.sampleCount);
		err = FskMuxerTrackAdd(walker->muxerTrack, walker->holdData, 1, &walker->holdInfo);
		BAIL_IF_ERR(err);

		FskMemPtrDisposeAt((void **)&walker->holdData);
	}

	err = FskMemPtrNewClear(sizeof(FskMuxerSampleInfoRecord) * infoCount, (FskMemPtr *)&writerInfo);
	BAIL_IF_ERR(err);

	for (i = 0; i < infoCount; i++) {
		FskMediaReaderSampleInfo r = &readerInfo[i];
		FskMuxerSampleInfo w = &writerInfo[i];

		w->sampleCount = r->samples;
		w->sampleSize = r->sampleSize;
		w->sampleDuration = r->sampleDuration;
		w->compositionOffset = 0;		//@@
		w->flags = (kFskImageFrameTypeSync == r->flags) ? 0 : 1;
	}

	if (0 == writerInfo[infoCount - 1].sampleDuration) {
		unsigned char *d = data;

		for (i = 0; i < infoCount - 1; i++)
			d += writerInfo[i].sampleCount * writerInfo[i].sampleSize;

		infoCount -= 1;

		walker->holdInfo = writerInfo[infoCount];
		walker->holdDecodeTime = readerInfo[infoCount].decodeTime;

		err = FskMemPtrNewFromData(walker->holdInfo.sampleCount * walker->holdInfo.sampleSize, d, (FskMemPtr *)&walker->holdData);
		BAIL_IF_ERR(err);

		if (0 == infoCount)
			goto bail;
	}

	err = FskMuxerTrackAdd(walker->muxerTrack, data, infoCount, writerInfo);
	BAIL_IF_ERR(err);

bail:
	if (kFskErrNone != err)
		state->record.failed = true;

	FskMemPtrDispose(writerInfo);

	return err;
}

FskErr recordExtractedWrite(FskMuxer muxer, void *refCon, const void *data, FskFileOffset offset, UInt32 dataSize)
{
	readerMediaPlayerModule state = (readerMediaPlayerModule)refCon;
	FskErr err = kFskErrNone;

	if (NULL == state->record.fref) {
		FskFileDelete(state->record.path);

		err = FskFileCreate(state->record.path);
		BAIL_IF_ERR(err);

		err = FskFileOpen(state->record.path, kFskFilePermissionReadWrite, &state->record.fref);
		BAIL_IF_ERR(err);
	}

	err = FskFileSetPosition(state->record.fref, &offset);
	BAIL_IF_ERR(err);

	err = FskFileWrite(state->record.fref, dataSize, data, NULL);
	BAIL_IF_ERR(err);

bail:
	return err;
}

#endif

#if TARGET_OS_WIN32

typedef struct {
	HANDLE	hFile;
	UInt32	a0;
	UInt32	a1;
	UInt32	a2;
} FskMemPtrLargeHeaderRecord, *FskMemPtrLargeHeader;

FskErr FskMemPtrNewLarge(UInt32 size, FskMemPtr *mem)
{
	FskMemPtrLargeHeader header;
	HANDLE hFile;

	hFile = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size + sizeof(FskMemPtrLargeHeaderRecord), NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return kFskErrMemFull;

	header = MapViewOfFile(hFile, FILE_MAP_WRITE, 0, 0, size + sizeof(FskMemPtrLargeHeaderRecord));
	if (NULL == header) {
		CloseHandle(hFile);
		return kFskErrMemFull;
	}

	header->hFile = hFile;
	*mem = (FskMemPtr)(header + 1);

	return kFskErrNone;
}

void FskMemPtrDisposeLarge(FskMemPtr mem)
{
	if (mem) {
		FskMemPtrLargeHeader header = ((FskMemPtrLargeHeader)mem) - 1;
		HANDLE hFile = header->hFile;

		UnmapViewOfFile(header);
		CloseHandle(hFile);
	}
}

#endif
