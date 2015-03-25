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
#ifndef __FSKMEDIAPLAYER__
#define __FSKMEDIAPLAYER__

#include "FskAudio.h"
#include "FskEvent.h"
#include "FskExtensions.h"
#include "FskHeaders.h"
#include "FskMedia.h"
#include "FskTime.h"
#include "FskWindow.h"
#include "xs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FskMediaPlayerInstall(a) (FskExtensionInstall(kFskExtensionMediaPlayer, a))
#define FskMediaPlayerUninstall(a) (FskExtensionUninstall(kFskExtensionMediaPlayer, a))

typedef struct FskMediaPlayerRecord FskMediaPlayerRecord;
typedef FskMediaPlayerRecord *FskMediaPlayer;

typedef struct FskMediaPlayerModuleRecord FskMediaPlayerModuleRecord;
typedef FskMediaPlayerModuleRecord *FskMediaPlayerModule;

typedef void (*FskMediaPlayerModuleInitialize)(FskMediaPlayerModule module);

typedef struct {
	FskMediaPlayerModuleInitialize			doInitializeFile;
	FskMediaPlayerModuleInitialize			doInitializeHTTP;
	const char								*mime;
	FskErr									(*doCanHandle)(UInt32 dataSourceType, const char *mime, UInt32 flags);
	FskErr									(*doSniff)(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);
} FskMediaPlayerEntryRecord, *FskMediaPlayerEntry;

typedef struct FskMediaPlayerPropertyIdAndValueRecord FskMediaPlayerPropertyIdAndValueRecord;
typedef FskMediaPlayerPropertyIdAndValueRecord *FskMediaPlayerPropertyIdAndValue;

struct FskMediaPlayerPropertyIdAndValueRecord {
	UInt32							id;
	FskMediaPropertyValueRecord		value;
};

typedef FskErr (*FskMediaPlayerEvent)(FskMediaPlayer player, void *refCon, UInt32 eventCode, FskEvent event);

typedef FskErr (*FskMediaPlayerModuleNew)(FskMediaPlayerModule module,
			const void *dataSource, UInt32 dataSourceType, const char *mime, FskMediaPlayerPropertyIdAndValue properties);
typedef void (*FskMediaPlayerModuleDispose)(void *state, FskMediaPlayerModule module);

typedef FskErr (*FskMediaPlayerModuleGetTime)(void *state, FskMediaPlayerModule module, float scale, FskSampleTime *time);
typedef FskErr (*FskMediaPlayerModuleSetTime)(void *state, FskMediaPlayerModule module, float scale, FskSampleTime time);
typedef FskErr (*FskMediaPlayerModuleStart)(void *state, FskMediaPlayerModule module);
typedef FskErr (*FskMediaPlayerModuleStop)(void *state, FskMediaPlayerModule module);
typedef Boolean (*FskMediaPlayerModuleWillDraw)(void *state, FskMediaPlayerModule module, FskSampleTime beforeTime);
typedef FskErr (*FskMediaPlayerModuleDoUpdate)(void *state, FskMediaPlayerModule module);
typedef FskErr (*FskMediaPlayerModulePropertyChanged)(void *state, FskMediaPlayerModule module, UInt32 property);
typedef FskErr (*FskMediaPlayerModuleGetMetadata)(void *state, FskMediaPlayerModule module, const char *metaDataType, UInt32 index, FskMediaPropertyValue property, UInt32 *flags);
typedef FskErr (*FskMediaPlayerModuleGetVideoBitmap)(void *state, FskMediaPlayerModule module, UInt32 width, UInt32 height, FskBitmap *bitMap);
typedef FskErr (*FskMediaPlayerModuleGetTrack)(void *state, FskMediaPlayerModule module, UInt32 index, void **track);
typedef FskErr (*FskMediaPlayerModuleTrackHasProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
typedef FskErr (*FskMediaPlayerModuleTrackSetProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*FskMediaPlayerModuleTrackGetProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*FskMediaPlayerModuleHasProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
typedef FskErr (*FskMediaPlayerModuleSetProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*FskMediaPlayerModuleGetProperty)(void *state, FskMediaPlayerModule module, void *track, UInt32 propertyID, FskMediaPropertyValue property);

enum {
	kFskMediaPlayerPropertyBounds = 1,
	kFskMediaPlayerPropertyWindow,
	kFskMediaPlayerPropertyVolume
};

enum {
	kFskMediaPlayerStateClosed = -1024,
	kFskMediaPlayerStateFailed = -512,
	kFskMediaPlayerStateInitializing = -256,
	kFskMediaPlayerStateInstantiating = -128,
	kFskMediaPlayerStateInstantiatingProgress = -112,	// continues to -12 (-112 = 0% up to -12 = 100%)
	kFskMediaPlayerStateStopped = 0,
	kFskMediaPlayerStateBuffering = 128,
	kFskMediaPlayerStatePlaying = 256
};

enum {
	kFskMediaPlayerFlagHasVideo = 1L << 0,
	kFskMediaPlayerFlagHasAudio = 1L << 1,
	kFskMediaPlayerFlagHasQTPanorama = 1L << 16,
	kFskMediaPlayerFlagHasQTObject = 1L << 17,
	kFskMediaPlayerFlagHasAudioBook = 1L << 18
};

enum {
	kFskMediaPlayerIdleChangeTime = 1L << 0,
	kFskMediaPlayerIdleChangePropertyBuffer = 1L << 1,
	kFskMediaPlayerIdleChangePropertyDownloadPosition = 1L << 2,
	kFskMediaPlayerIdleChangePropertySeekableSegment = 1L << 3
};

#if defined(__FSKMEDIAPLAYER_PRIV__) || SUPPORT_INSTRUMENTATION

	struct FskMediaPlayerModuleRecord {
		void								*state;					// module sets

		FskMediaPlayer						player;
		SInt32								playState;				// module sets

		// events
		FskMediaPlayerEvent					eventHandler;
		void								*eventHandlerRefCon;

		// visual
		FskWindow							window;
		FskPort								port;
		UInt32								requestedDrawingMethod;
		FskRectangleRecord					windowBounds;
		UInt32								naturalWidth;			// module sets
		UInt32								naturalHeight;			// module sets

		// audio
		FskSndChannel						sndChan;
		float								volume;
		Boolean								useSoundChannelForTime;	// module sets
	
		// time
		FskSampleTime						duration;				// module sets
		float								durationScale;			// module sets

		// tracks
		UInt32								trackCount;				// module sets

		UInt32								idleFlags;				// module sets
		Boolean								needsIdle;				// module sets

		// call me often
		FskMediaPlayerModuleNew					doNew;					// module sets
		FskMediaPlayerModuleDispose				doDispose;				// module sets
		FskMediaPlayerModuleGetTime				doGetTime;				// module sets
		FskMediaPlayerModuleSetTime				doSetTime;				// module sets
		FskMediaPlayerModuleStart				doStart;				// module sets
		FskMediaPlayerModuleStop				doStop;					// module sets
        FskMediaPlayerModuleWillDraw            doWillDraw;             // module sets
		FskMediaPlayerModuleDoUpdate			doUpdate;				// module sets
		FskMediaPlayerModulePropertyChanged		doPropertyChanged;		// module sets
		FskMediaPlayerModuleGetMetadata			doGetMetadata;			// module sets
		FskMediaPlayerModuleGetVideoBitmap		doGetVideoBitmap;		// module sets
		FskMediaPlayerModuleGetTrack			doGetTrack;				// module sets

		FskMediaPropertyEntry					properties;				// module sets - point to array of FskMediaPropertyEntry - terminated with kFskMediaPropertyUndefined
		FskMediaPropertyEntry					trackProperties;		// module sets - point to array of FskMediaPropertyEntry - terminated with kFskMediaPropertyUndefined

		FskMediaPlayerModuleTrackHasProperty	doTrackHasProperty;
		FskMediaPlayerModuleTrackGetProperty	doTrackGetProperty;
		FskMediaPlayerModuleTrackSetProperty	doTrackSetProperty;

		FskMediaPlayerModuleHasProperty			doHasProperty;
		FskMediaPlayerModuleGetProperty			doGetProperty;
		FskMediaPlayerModuleSetProperty			doSetProperty;
	};
#endif

#if defined(__FSKMEDIAPLAYER_PRIV__) || SUPPORT_INSTRUMENTATION
	struct FskMediaPlayerRecord {
		UInt32							callBackPeriodInMS;	
		FskMediaPlayerModuleRecord		module;

		FskTimeCallBack					callback;
		UInt32							callbackPeriod;

		FskSampleTime					zeroTime;
		float							zeroTimeScale;

		SInt16							useCount;
        Boolean                         initialized;

		FskEvent						idleEvent;

		FskInstrumentedItemDeclaration
	};
#endif

#ifdef __FSKMEDIAPLAYER_PRIV__
	#include "FskFiles.h"
	#include "FskImage.h"
#endif

enum {
	kFskMediaPlayerDataSourceFile = 1,
	kFskMediaPlayerDataSourceHTTP = 2
};

// potential
FskAPI(Boolean) FskMediaPlayerCanHandle(UInt32 dataSourceType, const char *mime);
FskAPI(FskErr) FskMediaPlayerSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

// management
FskAPI(FskErr) FskMediaPlayerNew(FskMediaPlayer *player, const void *dataSource, UInt32 dataSourceType, const char *mime, FskMediaPlayerPropertyIdAndValue properties);
FskAPI(void) FskMediaPlayerDispose(FskMediaPlayer player);

FskAPI(FskErr) FskMediaPlayerSetEventHandler(FskMediaPlayer player, FskMediaPlayerEvent eventHandler, void *refCon);
FskAPI(FskErr) FskMediaPlayerSetCallbackPeriod(FskMediaPlayer player, UInt32 ms);

FskAPI(FskErr) FskMediaPlayerGetMetadata(FskMediaPlayer player, const char *metadataTag, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);

// time
FskAPI(FskErr) FskMediaPlayerGetDuration(FskMediaPlayer player, float scale, FskSampleTime *duration);
FskAPI(FskErr) FskMediaPlayerGetTime(FskMediaPlayer player, float scale, FskSampleTime *time);
FskAPI(FskErr) FskMediaPlayerSetTime(FskMediaPlayer player, float scale, FskSampleTime time);

// sound
FskAPI(FskErr) FskMediaPlayerSetVolume(FskMediaPlayer player, float volume);
FskAPI(FskErr) FskMediaPlayerGetVolume(FskMediaPlayer player, float *volume);

// visual
FskAPI(FskErr) FskMediaPlayerGetNaturalDimensions(FskMediaPlayer player, UInt32 *width, UInt32 *height);

FskAPI(FskErr) FskMediaPlayerGetVideoBitmap(FskMediaPlayer player, UInt32 width, UInt32 height, FskBitmap *bitMap);

enum {
	kFskMediaPlayerDrawingMethodComposite,
	kFskMediaPlayerDrawingMethodDirect
};

FskAPI(FskErr) FskMediaPlayerSetWindow(FskMediaPlayer player, FskWindow window, UInt32 drawingMethod);
FskAPI(FskErr) FskMediaPlayerSetPort(FskMediaPlayer player, FskPort port);
FskAPI(FskErr) FskMediaPlayerSetBounds(FskMediaPlayer player, const FskRectangle bounds);
FskAPI(Boolean) FskMediaPlayerWillDraw(FskMediaPlayer player, FskSampleTime beforeTime);
FskAPI(FskErr) FskMediaPlayerUpdate(FskMediaPlayer player);

// play
FskAPI(FskErr) FskMediaPlayerStart(FskMediaPlayer player);
FskAPI(FskErr) FskMediaPlayerStop(FskMediaPlayer player);
FskAPI(SInt32) FskMediaPlayerGetPlayState(FskMediaPlayer player);

#ifdef __FSKMEDIAPLAYER_PRIV__
	FskAPI(FskErr) FskMediaPlayerSetPlayState(FskMediaPlayer player, SInt32 state);		// only for use by modules
	FskAPI(void) FskMediaPlayerDetachSoundClock(FskMediaPlayer player);
	FskAPI(FskErr) FskMediaPlayerSendEvent(FskMediaPlayer player, UInt32 eventCode);
#endif

FskAPI(FskErr) FskMediaPlayerHasProperty(FskMediaPlayer player, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskMediaPlayerSetProperty(FskMediaPlayer player, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskMediaPlayerGetProperty(FskMediaPlayer player, UInt32 propertyID, FskMediaPropertyValue property);

// tracks
FskAPI(FskErr) FskMediaPlayerGetTrackCount(FskMediaPlayer player, UInt32 *count);
FskAPI(FskErr) FskMediaPlayerGetTrack(FskMediaPlayer player, UInt32 index, void **track);
FskAPI(FskErr) FskMediaPlayerTrackHasProperty(FskMediaPlayer player, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskMediaPlayerTrackSetProperty(FskMediaPlayer player, void *track, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskMediaPlayerTrackGetProperty(FskMediaPlayer player, void *track, UInt32 propertyID, FskMediaPropertyValue property);

#if SUPPORT_INSTRUMENTATION

typedef struct {
	FskSampleTime	time;
	double			scale;
} FskMediaPlayerInstrMsgSetTimeRecord;

enum {
	kFskMediaPlayerInstrMsgStart = kFskInstrumentedItemFirstCustomMessage,
	kFskMediaPlayerInstrMsgStop,
	kFskMediaPlayerInstrMsgSetTime,
	kFskMediaPlayerInstrMsgStateChange,
	kFskMediaPlayerInstrMsgGetMetadata,
	kFskMediaPlayerInstrMsgGetVideoBitmap,
	kFskMediaPlayerInstrMsgGetTime
};

#endif

#ifdef __cplusplus
}
#endif

#endif
