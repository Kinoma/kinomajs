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
#define __FSKMEDIAPLAYER_PRIV__
#define __FSKWINDOW_PRIV__
#include "FskMediaPlayer.h"

#include "FskEndian.h"
#include "FskImage.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"

static void mediaPlayerCallBack(FskTimeCallBack callback, const FskTime time, void *param);

Boolean FskMediaPlayerCanHandle(UInt32 dataSourceType, const char *mime)
{
	UInt32 i = 0;

	while (true) {
		FskMediaPlayerEntry playerEntry = (FskMediaPlayerEntry)FskExtensionGetByIndex(kFskExtensionMediaPlayer, i++);
		if (NULL == playerEntry)
			break;

		if (playerEntry->doCanHandle) {
			if (kFskErrNone == (playerEntry->doCanHandle)(dataSourceType, mime, 0))
				return true;
		}
		else {
			if (0 != FskStrCompareCaseInsensitive(playerEntry->mime, mime))
				continue;

			if ((kFskMediaPlayerDataSourceFile == dataSourceType) && playerEntry->doInitializeFile)
				return true;
			if ((kFskMediaPlayerDataSourceHTTP == dataSourceType) && playerEntry->doInitializeHTTP)
				return true;
		}
	}

	return false;
}

FskErr FskMediaPlayerSniffForMIME(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	UInt32 i = 0;

	while (true) {
		FskMediaPlayerEntry aDispatch = (FskMediaPlayerEntry)FskExtensionGetByIndex(kFskExtensionMediaPlayer, i++);
		if (NULL == aDispatch)
			break;

		if (NULL == aDispatch->doSniff)
			continue;

		if ((kFskErrNone == (aDispatch->doSniff)(data, dataSize, headers, uri, mime)) && (NULL != *mime))
			return kFskErrNone;
	}

	return kFskErrExtensionNotFound;
}

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageMediaPlayer(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gMediaPlayerTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"mediaplayer",
		FskInstrumentationOffset(FskMediaPlayerRecord),
		NULL,
		0,
		NULL,
		doFormatMessageMediaPlayer
	};
#endif

FskErr FskMediaPlayerNew(FskMediaPlayer *playerOut, const void *dataSource, UInt32 dataSourceType, const char *mime, FskMediaPlayerPropertyIdAndValue properties)
{
	FskErr err = kFskErrNone;
	FskMediaPlayer player;
	FskMediaPlayerModuleInitialize handler = NULL;
	UInt32 i = 0;

	err = FskMemPtrNewClear(sizeof(FskMediaPlayerRecord), &player);
	BAIL_IF_ERR(err);

	player->module.player = player;

	FskInstrumentedItemNew(player, FskStrDoCopy_Untracked((const char *)dataSource), &gMediaPlayerTypeInstrumentation);

	player->useCount = 1;

	// initialize player instance
	player->module.volume = 1.0;
	player->zeroTimeScale = 1.0;
	player->module.duration = -1;		// duration not known
	player->module.durationScale = 1.0;
	player->module.playState = kFskMediaPlayerStateInitializing;

	player->module.requestedDrawingMethod = kFskMediaPlayerDrawingMethodComposite;

	while (true) {
		FskMediaPlayerEntry playerEntry = (FskMediaPlayerEntry)FskExtensionGetByIndex(kFskExtensionMediaPlayer, i++);
		if (NULL == playerEntry)
			break;

		if (NULL != playerEntry->doCanHandle) {
			if (kFskErrNone == (playerEntry->doCanHandle)(dataSourceType, mime, 0)) {
				if (kFskMediaPlayerDataSourceFile == dataSourceType)
					handler = playerEntry->doInitializeFile;
				else
					handler = playerEntry->doInitializeHTTP;
				break;
			}
		}
		else {
			if (0 != FskStrCompareCaseInsensitive(playerEntry->mime, mime))
				continue;

			if ((kFskMediaPlayerDataSourceFile == dataSourceType) && playerEntry->doInitializeFile) {
				handler = playerEntry->doInitializeFile;
				break;
			}
			if ((kFskMediaPlayerDataSourceHTTP == dataSourceType) && playerEntry->doInitializeHTTP) {
				handler = playerEntry->doInitializeHTTP;
				break;
			}
		}
	}

	if (!handler)
        BAIL(kFskErrUnimplemented);

	(handler)(&player->module);

	// initialize the module
	err = (player->module.doNew)(&player->module, dataSource, dataSourceType, mime, properties);
	BAIL_IF_ERR(err);

	// if player left state as initializing, then progress it to stopped. otherwise we assume player will advance it when necessary.
	if (kFskMediaPlayerStateInitializing == player->module.playState)
		FskMediaPlayerSetPlayState(player, kFskMediaPlayerStateStopped);

bail:
	if (err) {
		FskMediaPlayerDispose(player);
		player = NULL;
	}

	*playerOut = player;

	return err;
}

void FskMediaPlayerDispose(FskMediaPlayer player)
{
	if (player) {
		if (player->useCount > 1) {
			player->useCount -= 1;
			return;
		}

		FskMediaPlayerStop(player);

		FskMediaPlayerSendEvent(player, kFskEventMediaPlayerTerminate);

		if (player->module.doDispose)
			(player->module.doDispose)(player->module.state, &player->module);

		FskTimeCallbackDispose(player->callback);
		FskEventDispose(player->idleEvent);

#if SUPPORT_INSTRUMENTATION
		FskInstrumentedItemDispose(player);
		FskMemPtrDispose_Untracked((void **)player->_instrumented.name);
		player->_instrumented.name = NULL;
#endif

		FskMemPtrDispose(player);
	}
}

FskErr FskMediaPlayerSetEventHandler(FskMediaPlayer player, FskMediaPlayerEvent eventHandler, void *refCon)
{
	player->module.eventHandler = eventHandler;
	player->module.eventHandlerRefCon = refCon;
	return kFskErrNone;
}

FskErr FskMediaPlayerSetCallbackPeriod(FskMediaPlayer player, UInt32 ms)
{
	if (NULL == player->callback) {
		FskTimeCallbackUINew(&player->callback);
		FskInstrumentedItemSetOwner(player->callback, player);
	}
	else {
		if (player->module.playState >= kFskMediaPlayerStatePlaying)
			FskTimeCallbackRemove(player->callback);
	}

	player->callbackPeriod = ms;

	if (((player->module.playState > kFskMediaPlayerStateStopped) && ms) || player->module.needsIdle)
		FskTimeCallbackScheduleFuture(player->callback, 0, ms, mediaPlayerCallBack, player);

	return kFskErrNone;
}

FskErr FskMediaPlayerGetDuration(FskMediaPlayer player, float scale, FskSampleTime *durationOut)
{
	double duration;

	if (-1 == player->module.duration) {
		// duration unknown - for example, MP3 audio stream
		*durationOut = 0;
		return kFskErrUnknownElement;
	}

	duration = (double)player->module.duration;
	duration = (duration / player->module.durationScale) * scale;

	*durationOut = (FskSampleTime)duration;

	return kFskErrNone;
}

FskErr FskMediaPlayerGetTime(FskMediaPlayer player, float scale, FskSampleTime *timeOut)
{
	FskErr err;

	if (player->module.playState < kFskMediaPlayerStatePlaying) {
		double temp;

		temp = (((double)player->zeroTime) / player->zeroTimeScale) * scale;
		*timeOut = (FskSampleTime)temp;

		FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgGetTime, timeOut);

		return kFskErrNone;
	}

	if (player->module.useSoundChannelForTime) {
		FskSampleTime time;
		double sampleRate, temp;

		err = FskSndChannelGetSamplePosition(player->module.sndChan, &time);
		if (kFskErrNone == err) {
			FskSndChannelGetFormat(player->module.sndChan, NULL, NULL, NULL, &sampleRate, NULL, NULL);

			temp = (double)time / sampleRate;
			if (-1 != player->module.duration) {
				double end = (double)player->module.duration / player->module.durationScale;
				if (temp > end)
					temp = end;
			}
			*timeOut = (FskSampleTime)(temp * scale);
			FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgGetTime, timeOut);

			goto bail;
		}
	}

	if (player->module.doGetTime)
		err = (player->module.doGetTime)(player->module.state, &player->module, scale, timeOut);
	else
		err = kFskErrUnimplemented;

	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgGetTime, timeOut);

bail:
	return err;
}

FskErr FskMediaPlayerSetTime(FskMediaPlayer player, float scale, FskSampleTime time)
{
	FskErr err = kFskErrNone;
	Boolean wasPlaying = player->module.playState >= kFskMediaPlayerStateBuffering;

	if (-1 == player->module.duration)
		return kFskErrBadState;			// can't set the time of a live stream

	if (wasPlaying)
		FskMediaPlayerStop(player);

#if SUPPORT_INSTRUMENTATION
	{
	FskMediaPlayerInstrMsgSetTimeRecord msg;
	msg.time = time;
	msg.scale = scale;
	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgSetTime, &msg);
	}
#endif

	player->zeroTime = time;
	player->zeroTimeScale = scale;

	if (player->module.doSetTime)
		err = (player->module.doSetTime)(player->module.state, &player->module, scale, time);

	if (wasPlaying)
		err = FskMediaPlayerStart(player);

	return err;
}


FskErr FskMediaPlayerSetVolume(FskMediaPlayer player, float volume)
{
	player->module.volume = volume;
	if (player->module.sndChan)
		FskSndChannelSetVolume(player->module.sndChan, volume);
	else {
		if (player->module.doPropertyChanged)
			(player->module.doPropertyChanged)(player->module.state, &player->module, kFskMediaPlayerPropertyVolume);
	}

	return kFskErrNone;
}

FskErr FskMediaPlayerGetVolume(FskMediaPlayer player, float *volume)
{
	*volume = player->module.volume;

	return kFskErrNone;

}

// visual
FskErr FskMediaPlayerSetWindow(FskMediaPlayer player, FskWindow window, UInt32 drawingMethod)
{
	player->module.window = window;
	if (window)
		player->module.port = FskWindowGetPort(window);
	else
		player->module.port = NULL;

	player->module.requestedDrawingMethod = drawingMethod;

	if (player->module.doPropertyChanged)
		(player->module.doPropertyChanged)(player->module.state, &player->module, kFskMediaPlayerPropertyWindow);

	return kFskErrNone;
}

FskErr FskMediaPlayerSetPort(FskMediaPlayer player, FskPort port)
{
	player->module.window = NULL;
	player->module.port = port;
//@@	player->module.requestedDrawingMethod = kFskMediaPlayerDrawingMethodComposite;
	player->module.requestedDrawingMethod = kFskMediaPlayerDrawingMethodDirect;

	if (player->module.doPropertyChanged)
		(player->module.doPropertyChanged)(player->module.state, &player->module, kFskMediaPlayerPropertyWindow);

	return kFskErrNone;
}

FskErr FskMediaPlayerSetBounds(FskMediaPlayer player, const FskRectangle bounds)
{
	player->module.windowBounds = *bounds;
	if (player->module.doPropertyChanged)
		(player->module.doPropertyChanged)(player->module.state, &player->module, kFskMediaPlayerPropertyBounds);
	return kFskErrNone;
}

Boolean FskMediaPlayerWillDraw(FskMediaPlayer player, FskSampleTime beforeTime)
{
	if (player->module.doWillDraw)
		return (player->module.doWillDraw)(player->module.state, &player->module, beforeTime);

	return false;
}

FskErr FskMediaPlayerUpdate(FskMediaPlayer player)
{
	FskErr err = kFskErrNone;

	if (player->module.doUpdate)
		err = (player->module.doUpdate)(player->module.state, &player->module);

	return err;
}

FskErr FskMediaPlayerGetNaturalDimensions(FskMediaPlayer player, UInt32 *width, UInt32 *height)
{
	if (width) *width = player->module.naturalWidth;
	if (height) *height = player->module.naturalHeight;
	return kFskErrNone;
}

FskErr FskMediaPlayerStart(FskMediaPlayer player)
{
	FskErr err = kFskErrUnimplemented;

	if (player->module.playState > kFskMediaPlayerStateBuffering)
		return kFskErrNone;

	if (player->module.playState < kFskMediaPlayerStateStopped)
		return kFskErrBadState;			// cannot start playback before instantiated

	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgStart, NULL);

	player->useCount += 1;

	if (player->module.doStart) {
		err = (player->module.doStart)(player->module.state, &player->module);
		BAIL_IF_ERR(err);

		if (player->callbackPeriod)
			FskTimeCallbackScheduleFuture(player->callback, 0, player->callbackPeriod, mediaPlayerCallBack, player);
	}

bail:
	FskMediaPlayerDispose(player);		// down count

	return err;
}

FskErr FskMediaPlayerStop(FskMediaPlayer player)
{
	FskErr err = kFskErrUnimplemented;

	if (kFskMediaPlayerStateStopped == player->module.playState)
		return kFskErrNone;

	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgStop, NULL);

	if (player->callback && !player->module.needsIdle)
		FskTimeCallbackRemove(player->callback);

	// remember the time we stopped at
	player->zeroTimeScale = 1000.0;
	FskMediaPlayerGetTime(player, player->zeroTimeScale, &player->zeroTime);

    if (!player->initialized)
        err = kFskErrNone;
	else if (player->module.doStop && player->module.state)
		err = (player->module.doStop)(player->module.state, &player->module);

	return err;
}

SInt32 FskMediaPlayerGetPlayState(FskMediaPlayer player)
{
	return player->module.playState;
}

FskErr FskMediaPlayerSetPlayState(FskMediaPlayer player, SInt32 playState)
{
	if (playState != player->module.playState) {
		SInt32 previousState = player->module.playState;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(player, kFskInstrumentationLevelNormal)) {
		void *msgData[2];
		msgData[0] = (void *)playState;
		msgData[1] = (void *)player->module.playState;
		FskInstrumentedItemSendMessageForLevel(player, kFskMediaPlayerInstrMsgStateChange, msgData, kFskInstrumentationLevelNormal);
	}
#endif

		player->useCount += 1;

		player->module.playState = playState;
		if ((previousState < kFskMediaPlayerStateStopped) && (playState >= kFskMediaPlayerStateStopped))
			FskMediaPlayerSendEvent(player, kFskEventMediaPlayerInitialize);

		FskMediaPlayerSendEvent(player, kFskEventMediaPlayerStateChange);

		FskMediaPlayerDispose(player);		// down useCount
	}

	return kFskErrNone;
}

FskErr FskMediaPlayerGetMetadata(FskMediaPlayer player, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgGetMetadata, (void *)metaDataType);
	if (player->module.doGetMetadata)
		return (player->module.doGetMetadata)(player->module.state, &player->module, metaDataType, index, value, flags);
	return kFskErrUnimplemented;
}

FskErr FskMediaPlayerGetVideoBitmap(FskMediaPlayer player, UInt32 width, UInt32 height, FskBitmap *bitmap)
{
	FskInstrumentedItemSendMessage(player, kFskMediaPlayerInstrMsgGetVideoBitmap, NULL);
	*bitmap = NULL;
	if (player->module.doGetVideoBitmap)
		return (player->module.doGetVideoBitmap)(player->module.state, &player->module, width, height, bitmap);
	return kFskErrUnimplemented;
}

void FskMediaPlayerDetachSoundClock(FskMediaPlayer player)
{
	if (player->module.useSoundChannelForTime && (NULL != player->module.sndChan)) {
		player->zeroTimeScale = 1000.0;
		FskMediaPlayerGetTime(player, player->zeroTimeScale, &player->zeroTime);

		player->module.useSoundChannelForTime = false;
	}
}

void mediaPlayerCallBack(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskMediaPlayer player = (FskMediaPlayer)param;

	player->useCount += 1;

	player->module.idleFlags = 0;

	FskMediaPlayerSendEvent(player, kFskEventMediaPlayerIdle);

	// reschedule
	if (((player->module.playState > kFskMediaPlayerStateStopped) && player->callbackPeriod) || player->module.needsIdle)
		FskTimeCallbackScheduleFuture(player->callback, 0, player->callbackPeriod, mediaPlayerCallBack, player);

	FskMediaPlayerDispose(player);		// down count
}

FskErr FskMediaPlayerSendEvent(FskMediaPlayer player, UInt32 eventCode)
{
	FskErr err = kFskErrNone;
	FskEvent event;

    if (kFskEventMediaPlayerInitialize == eventCode)
        player->initialized = true;

	if (NULL == player->module.eventHandler)
		goto bail;

	if (kFskEventMediaPlayerIdle == eventCode) {
		event = player->idleEvent;
		if (NULL == event) {
			if (kFskErrNone != FskEventNew(&event, eventCode, NULL, 0))
				goto bail;

			player->idleEvent = event;
		}
	}
	else {
		if (kFskErrNone != FskEventNew(&event, eventCode, NULL, 0))
			goto bail;
	}

	// do some work to make it safe for event handler to dispose of media player
	player->useCount += 1;

	err = (player->module.eventHandler)(player, player->module.eventHandlerRefCon, eventCode, event);

	if (event != player->idleEvent)
		FskEventDispose(event);

	FskMediaPlayerDispose(player);		// down count

bail:
	return err;
}

FskErr FskMediaPlayerHasProperty(FskMediaPlayer player, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	if (NULL != player->module.doHasProperty)
		return (*player->module.doHasProperty)(player->module.state, &player->module, NULL, propertyID, get, set, dataType);
	return FskMediaHasProperty(player->module.properties, propertyID, get, set, dataType);
}

FskErr FskMediaPlayerSetProperty(FskMediaPlayer player, UInt32 propertyID, FskMediaPropertyValue property)
{
	if (NULL != player->module.doSetProperty)
		return (*player->module.doSetProperty)(player->module.state, &player->module, NULL, propertyID, property);
	return FskMediaSetProperty(player->module.properties, player->module.state, &player->module, propertyID, property);
}

FskErr FskMediaPlayerGetProperty(FskMediaPlayer player, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskErr err;

	if (NULL != player->module.doGetProperty)
		err = (*player->module.doGetProperty)(player->module.state, &player->module, NULL, propertyID, property);
	else
		err = FskMediaGetProperty(player->module.properties, player->module.state, &player->module, propertyID, property);

	if ((kFskErrUnimplemented == err) && (kFskMediaPropertyFlags == propertyID)) {
		UInt32 flags = 0, index = 0;

		while (true) {
			void *track;
			FskMediaPropertyValueRecord prop;

			if (kFskErrNone != FskMediaPlayerGetTrack(player, index++, &track))
				break;

			if (kFskErrNone != FskMediaPlayerTrackGetProperty(player, track, kFskMediaPropertyMediaType, &prop))
				continue;

			err = kFskErrNone;

			if (0 == FskStrCompare(prop.value.str, "video"))
				flags |= kFskMediaPlayerFlagHasVideo;
			else if (0 == FskStrCompare(prop.value.str, "audio"))
				flags |= kFskMediaPlayerFlagHasAudio;

			FskMemPtrDispose(prop.value.str);
		}

		if (kFskErrNone == err) {
			property->type = kFskMediaPropertyTypeInteger;
			property->value.integer = flags;
		}
	}

	return err;
}

FskErr FskMediaPlayerGetTrackCount(FskMediaPlayer player, UInt32 *count)
{
	*count = player->module.trackCount;
	return kFskErrNone;
}

FskErr FskMediaPlayerGetTrack(FskMediaPlayer player, UInt32 index, void **track)
{
	if (index >= player->module.trackCount)
		return kFskErrInvalidParameter;

	if (player->module.doGetTrack)
		return (*player->module.doGetTrack)(player->module.state, &player->module, index, track);
	return kFskErrUnimplemented;
}

FskErr FskMediaPlayerTrackHasProperty(FskMediaPlayer player, void *track, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	if (NULL != player->module.doTrackHasProperty)
		return (*player->module.doTrackHasProperty)(player->module.state, &player->module, track, propertyID, get, set, dataType);
	return FskMediaHasProperty(player->module.trackProperties, propertyID, get, set, dataType);
}

FskErr FskMediaPlayerTrackSetProperty(FskMediaPlayer player, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	if (NULL != player->module.doTrackSetProperty)
		return (*player->module.doTrackSetProperty)(player->module.state, &player->module, track, propertyID, property);
	return FskMediaSetProperty(player->module.trackProperties, player->module.state, track, propertyID, property);
}

FskErr FskMediaPlayerTrackGetProperty(FskMediaPlayer player, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	if (NULL != player->module.doTrackGetProperty)
		return (*player->module.doTrackGetProperty)(player->module.state, &player->module, track, propertyID, property);
	return FskMediaGetProperty(player->module.trackProperties, player->module.state, track, propertyID, property);
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageMediaPlayer(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskMediaPlayerInstrMsgStart:
			snprintf(buffer, bufferSize, "start");
			return true;

		case kFskMediaPlayerInstrMsgStop:
			snprintf(buffer, bufferSize, "stop");
			return true;

		case kFskMediaPlayerInstrMsgSetTime: {
			FskMediaPlayerInstrMsgSetTimeRecord *d = (FskMediaPlayerInstrMsgSetTimeRecord *)msgData;
			snprintf(buffer, bufferSize, "set time %u, scale=%lf", (unsigned int)d->time, d->scale);
			}
			return true;

		case kFskMediaPlayerInstrMsgStateChange: {
			void **md = (void **)msgData;
			snprintf(buffer, bufferSize, "state change from %u to %u", (unsigned int)md[1], (unsigned int)md[0]);
			}
			return true;

		case kFskMediaPlayerInstrMsgGetMetadata:
			snprintf(buffer, bufferSize, "get metadata %s", (char *)msgData);
			return true;

		case kFskMediaPlayerInstrMsgGetVideoBitmap:
			snprintf(buffer, bufferSize, "get video bitmap");
			return true;

		case kFskMediaPlayerInstrMsgGetTime:
			snprintf(buffer, bufferSize, "got time %u", (unsigned int)(*(FskSampleTime *)msgData));
			return true;
	}

	return false;
}

#endif
