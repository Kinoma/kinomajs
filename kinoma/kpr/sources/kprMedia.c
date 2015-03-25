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
#define __FSKMEDIAPLAYER_PRIV__ 1 

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPClient.h"
#include "kprHTTPKeychain.h"
#include "kprImage.h"
#include "kprLayer.h"
#include "kprMedia.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprShell.h"
#if TARGET_OS_IPHONE
#include "kpr_iOS.h"
#endif

#define kprMediaGetState(SELF) (SELF->state)
#define kprMediaSetState(SELF,STATE) (SELF->state = STATE)

static void KprMediaDispose(void* it);
static void KprMediaDraw(void* it, FskPort port, FskRectangle area);
static void KprMediaIdle(void* it, double ticks);
static void KprMediaLayered(void* it, KprLayer layer, Boolean layerIt);
static void KprMediaMeasureHorizontally(void* it);
static void KprMediaMeasureVertically(void* it);
static void KprMediaPlaced(void* it);
static void KprMediaSetWindow(void* it, KprShell shell, KprStyle style);

static void KprMediaAdjust(KprMedia self);
static FskErr KprMediaEventHandler(FskMediaPlayer mp, void *it, UInt32 eventCode, FskEvent event);
static void KprMediaFeedback(KprMedia self, Boolean metadataChanged,  Boolean stateChanged, Boolean timeChanged, Boolean finished, FskErr err);
static FskErr KprMediaLoad(KprMedia self, UInt32 type, char* path, char* mime, char* user, char* password);
static void KprMediaOnCallback(void* it);
static void KprMediaOnMetaDataChange(void* it);
static void KprMediaOnWarning(void* it, FskEvent event);
static void KprMediaReportWarning(KprMedia self, char* warning);
static void KprMediaUnload(KprMedia self);

static FskErr KprMediaSnifferLoad(KprMedia self, char* url, char* mime);
static FskErr KprMediaSnifferLoadFile(KprMedia self, char* url, char* mime);
static FskErr KprMediaSnifferLoadMessage(KprMedia self, char* url, char* mime);
static void KprMediaSnifferLoadMessageComplete(KprMessage xkprMessage, void* it);
static void KprMediaSnifferLoadMessageDispose(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprMediaInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprMedia", FskInstrumentationOffset(KprMediaRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprMediaDispatchRecord = {
	"media",
	KprContentActivated,
	KprContentAdded,
	KprContentCascade,
	KprMediaDispose,
	KprMediaDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprMediaIdle,
	KprContentInvalidated,
	KprMediaLayered,
	KprContentMark,
	KprMediaMeasureHorizontally,
	KprMediaMeasureVertically,
	KprContentPlace,
	KprMediaPlaced,
	KprContentPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprMediaSetWindow,
	KprContentShowing,
	KprContentShown,
	KprContentUpdate
};

FskErr KprMediaNew(KprMedia* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprMedia self;

	bailIfError(FskMemPtrNewClear(sizeof(KprMediaRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprMediaInstrumentation);
	self->dispatch = &KprMediaDispatchRecord;
	self->flags = kprVisible | kprImageFit;
	self->setWindowHook = FskMediaPlayerSetWindow;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	kprMediaSetState(self, kprMediaStateWaiting);
bail:
	return err;
}

Boolean KprMediaGetSeeking(KprMedia self)
{
	if (self->mp) {
		FskMediaPropertyValueRecord property;
		FskMediaPlayerGetProperty(self->mp, kFskMediaPropertyScrub, &property);
		return property.value.b;
	}
	return false;
}

void KprMediaGetVolume(KprMedia self, float *volume) 
{
	if (self->mp)
		FskMediaPlayerGetVolume(self->mp, volume);
}

void KprMediaSetSeeking(KprMedia self, Boolean seekIt)
{
	if (self->mp) {
		FskMediaPropertyValueRecord property;
		property.type = kFskMediaPropertyTypeBoolean;
		property.value.b = seekIt;
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyScrub, &property);
	}
}

void KprMediaSetTime(KprMedia self, double time) 
{
	if (self->mp) {
		FskMediaPlayerSetTime(self->mp, 1000.0, (FskSampleTime)time);
		KprMediaOnCallback(self);
	}
}

void KprMediaSetURL(KprMedia self, char* url, char* mime) 
{
	FskErr err = kFskErrNone;
	KprMediaUnload(self);	
	if (url) {
		self->url = FskStrDoCopy(url);	
		bailIfNULL(url);
		bailIfError(KprMediaSnifferLoad(self, self->url, mime));
	}
bail:
	if (!url || !mime || (err != kFskErrNone))
		KprMediaFeedback(self, true, true, true, false, err);
}

void KprMediaSetVolume(KprMedia self, float volume) 
{
	if (self->mp)
#if TARGET_OS_ANDROID
		KprShellSetVolume(self->shell, volume);
#else
		FskMediaPlayerSetVolume(self->mp, volume);
#endif
}

void KprMediaSetBitRate(KprMedia self, UInt32 bitRate)
{
	self->bitRate = bitRate;
	if (self->mp)
	{
		FskMediaPropertyValueRecord property;
		property.type = kFskMediaPropertyTypeInteger;
		property.value.integer = self->bitRate;
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyBitRate, &property);
	}
}

void KprMediaStart(KprMedia self)
{
	if (self->mp)
		FskMediaPlayerStart(self->mp);
}

void KprMediaStop(KprMedia self)
{
	if (self->mp)
		FskMediaPlayerStop(self->mp);
}

void KprMediaHibernate(KprMedia self)
{
	if (self->mp) {
		FskMediaPropertyValueRecord value;
		
		value.type = kFskMediaPropertyTypeBoolean;
		value.value.b = true;
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyHibernate, &value);
	}
}

/* DISPATCH */

void KprMediaDispose(void* it) 
{
	KprMedia self = it;
	if (self->mp) {
		FskMediaPlayerSetEventHandler(self->mp, NULL, NULL);
		FskMediaPlayerDispose(self->mp);
	}
	FskMemPtrDispose(self->url);
	FskMemPtrDispose(self->title);
	FskMemPtrDispose(self->artist);
	FskMemPtrDispose(self->album);
	FskBitmapDispose(self->cover);
	if (self->message)
		KprMessageCancel(self->message);
	KprContentDispose(it);
}

void KprMediaDraw(void* it, FskPort port, FskRectangle area UNUSED) 
{
	FskErr err = kFskErrNone;
	KprMedia self = it;
	if (self->mp) {
		if (self->flags & kprMediaVideo) {
			if ((err = FskMediaPlayerUpdate(self->mp)))
				KprContentDraw(it, port, area);
		}
		else if (self->cover) {
			FskBitmap bitmap = self->cover;
			FskRectangleRecord srcRect, dstRect;
			FskRectangleSet(&dstRect, 0, 0, self->bounds.width, self->bounds.height);
			FskBitmapGetBounds(bitmap, &srcRect);
			KprAspectApply(self->flags, &srcRect, &dstRect);
			FskPortBitmapDraw(port, bitmap, &srcRect, &dstRect);
		}
	}
}

void KprMediaIdle(void* it, double ticks)
{
	KprMedia self = it;
	if (self->mp) {
        FskSampleTime beforeTime;

        FskMediaPlayerGetTime(self->mp, self->scale, &beforeTime);
        if (FskMediaPlayerWillDraw(self->mp, beforeTime + (self->scale / 120))) //@@ assumes drawing pump runs at 60 fps
            KprContentInvalidate(it);

        self->accumulatedInterval += ticks;
        if (self->accumulatedInterval >= 125) {
            KprMediaOnCallback(self);
            self->accumulatedInterval = 0;
        }
    }
}

void KprMediaLayered(void* it, KprLayer layer UNUSED, Boolean layerIt UNUSED)
{
	KprMedia self = it;
    if (self->shell)
		KprMediaAdjust(self);
}

void KprMediaMeasureHorizontally(void* it) 
{
	KprMedia self = it;
	UInt16 horizontal = self->coordinates.horizontal;
	if (!(horizontal & kprWidth)) {
		if (self->cover) {
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->cover, &bounds);
			self->coordinates.width = bounds.width;
		}
		else if (self->mp) {
			UInt32 width, height;
			FskMediaPlayerGetNaturalDimensions(self->mp, &width, &height);
			self->coordinates.width = width;
		}
		else
			self->coordinates.width = 0;
	}
}

void KprMediaMeasureVertically(void* it) 
{
	KprMedia self = it;
	UInt16 vertical = self->coordinates.vertical;
	if (!(vertical & kprHeight)) {
		if (self->cover) {
			FskRectangleRecord bounds;
			FskBitmapGetBounds(self->cover, &bounds);
			self->coordinates.height = bounds.height;
		}
		else if (self->mp) {
			UInt32 width, height;
			FskMediaPlayerGetNaturalDimensions(self->mp, &width, &height);
			self->coordinates.height = height;
		}
		else
			self->coordinates.height = 0;
	}
}

void KprMediaPlaced(void* it) 
{
	KprMedia self = it;
	KprMediaAdjust(self);
	KprContentPlaced(it);
}

void KprMediaSetWindow(void* it, KprShell shell, KprStyle style) 
{
	KprMedia self = it;
	if (self->shell && !shell) {
		KprShellStopPlacing(self->shell, it);
		if (self->mp && (self->flags & kprMediaReady) && (self->flags & kprMediaVideo))
			FskMediaPlayerSetPort(self->mp, NULL);
	}
	if (!self->shell && shell)
		KprShellStartPlacing(shell, it);
	KprContentSetWindow(it, shell, style);
}

/* IMPLEMENTATION */

void KprMediaAdjust(KprMedia self)
{
	if (self->mp && (self->flags & kprMediaReady) && (self->flags & kprMediaVideo)) {
		FskMediaPropertyValueRecord property;
		FskRectangleRecord natural;
		FskRectangleRecord bounds = *KprBounds(self);
		KprContainer container = self->container;
		natural.x = natural.y = 0; 
		FskMediaPlayerGetNaturalDimensions(self->mp, (UInt32*)&natural.width, (UInt32*)&natural.height);
		property.type = kFskMediaPropertyTypeRectangle;
		property.value.rect = bounds;
		if (self->flags & kprImageFill) {
			if (self->flags & kprImageFit) {
			}
			else {
				FskRectangleScaleToFill(&bounds, &natural, &bounds);
			}
		}
		else {
			if (self->flags & kprImageFit) {
				FskRectangleScaleToFit(&bounds, &natural, &bounds);
			}
			else {
				bounds.x += (bounds.width - natural.width) >> 1;
				bounds.y += (bounds.height - natural.height) >> 1;
				bounds.width = natural.width;
				bounds.height = natural.height;
			}
		}
		while (container && (!(container->flags & kprLayer))) {
			bounds.x += container->bounds.x;
			bounds.y += container->bounds.y;
			property.value.rect.x += container->bounds.x;
			property.value.rect.y += container->bounds.y;
			if (container->flags & kprClip)
				FskRectangleIntersect(&property.value.rect, &container->bounds, &property.value.rect);
			container = container->container;
		}
		FskMediaPlayerSetBounds(self->mp, &bounds);
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyClip, &property);

		if (container)
			FskMediaPlayerSetPort(self->mp, ((KprLayer)container)->port);
		else
			(*self->setWindowHook)(self->mp, self->shell->window, kFskMediaPlayerDrawingMethodComposite);
	}
}

FskErr KprMediaEventHandler(FskMediaPlayer mp, void *it, UInt32 eventCode, FskEvent event)
{
	KprMedia self = it;
//	char* handlerName = NULL;
	switch (eventCode) {
	case kFskEventMediaPlayerIdle:
//		handlerName = "kFskEventMediaPlayerIdle";
		break;
	case kFskEventWindowUpdate:
//		handlerName = "kFskEventWindowUpdate";
//@@		KprContentInvalidate(it);
		break;
	case kFskEventMediaPlayerStateChange:
//		handlerName = "kFskEventMediaPlayerStateChange";
		KprMediaOnCallback(it);
		break;
	case kFskEventMediaPlayerBoundsChange:
//		handlerName = "kFskEventMediaPlayerBoundsChange";
		if (self->shell) {
			KprContentInvalidate((KprContent)self);
			KprContentReflow((KprContent)self, kprSizeChanged);
		}
		break;
	case kFskEventMediaPlayerEditChange:
//		handlerName = "kFskEventMediaPlayerEditChange";
		break;
	case kFskEventMediaPlayerMetaDataChanged:
//		handlerName = "kFskEventMediaPlayerMetaDataChanged";
		KprMediaOnMetaDataChange(it);
		break;
	case kFskEventMediaPlayerEnd:
//		handlerName = "kFskEventMediaPlayerEnd";
		KprMediaFeedback(self, false, false, false, true, kFskErrNone);
		break;
	case kFskEventMediaPlayerInitialize:
//		handlerName = "kFskEventMediaPlayerInitialize";
		if (FskMediaPlayerGetPlayState(mp) >= kFskMediaPlayerStateStopped) {		
            FskMediaPropertyValueRecord scale;

            FskMediaPlayerGetProperty(self->mp, kFskMediaPropertyTimeScale, &scale);
            self->scale = scale.value.integer;

			KprMediaOnMetaDataChange(it);
			KprMediaOnCallback(self);
			self->flags |= kprMediaReady;
			if (self->shell) {
				KprContentInvalidate((KprContent)self);
				KprContentReflow((KprContent)self, kprSizeChanged);
			}
			kprDelegateLoaded(self);
		}
		break;
	case kFskEventMediaPlayerTerminate:
//		handlerName = "kFskEventMediaPlayerTerminate";
		break;
	case kFskEventMediaPlayerAuthenticate:
//		handlerName = "kFskEventMediaPlayerAuthenticate";
		{
			FskErr err;
			FskMediaPropertyValueRecord property;
			KprURLPartsRecord parts;
			char* host = NULL;
			char* realm = NULL;
			char* user = NULL;
			char* password = NULL;
			KprHTTPKeychain keychain = KprHTTPClientKeychain();
			property.type = kFskMediaPropertyTypeUndefined;
			bailIfError(FskMediaPlayerGetProperty(mp, kFskMediaPropertyLocation, &property));
			KprURLSplit(property.value.str, &parts);
			bailIfError(FskMemPtrNewClear(parts.authorityLength + 1, &host));
			FskStrNCopy(host, parts.authority, parts.authorityLength);
			FskMediaPropertyEmpty(&property);
			bailIfError(FskMediaPlayerGetProperty(mp, kFskMediaPropertyAuthentication, &property));
			realm = FskStrDoCopy(property.value.str);
			bailIfNULL(realm);
			FskMediaPropertyEmpty(&property);
			KprHTTPKeychainGet(keychain, host, realm, &user, &password);
			if (user && password) {
				bailIfError(FskMemPtrNewClear(FskStrLen(user) + 1 + FskStrLen(password) + 1 + 1, &property.value.str));
				property.type = kFskMediaPropertyTypeStringList;
				FskStrCopy(property.value.str, user);
				FskStrCopy(property.value.str + FskStrLen(user) + 1, password);
				FskMediaPlayerSetProperty(mp, kFskMediaPropertyAuthentication, &property);
			}
		bail:
			FskMediaPropertyEmpty(&property);
			FskMemPtrDispose(password);
			FskMemPtrDispose(user);
			FskMemPtrDispose(realm);
			FskMemPtrDispose(host);
		}
		break;
	case kFskEventMediaPlayerWarning:
//		handlerName = "kFskEventMediaPlayerWarning";
		KprMediaOnWarning(it, event);
		break;
	case kFskEventMediaPlayerReady:
//		handlerName = "kFskEventMediaPlayerReady";
		break;
	case kFskEventMediaPlayerHeaders:
//		handlerName = "kFskEventMediaPlayerHeaders";
		break;
	}
	//fprintf(stderr, "%s\n", handlerName);
	return kFskErrNone;
}

void KprMediaFeedback(KprMedia self, Boolean metadataChanged, Boolean stateChanged, Boolean timeChanged, Boolean finished, FskErr err UNUSED)
{
	if (metadataChanged)
		kprDelegateMetadataChanged(self);
	if (stateChanged)
		kprDelegateStateChanged(self);
	if (timeChanged) {
		kprDelegateTimeChanged(self);
#if TARGET_OS_IPHONE
		KprSystemNowPlayingInfoSetTime(self->duration / 1000.0, self->time / 1000.0);
#endif
	}
	if (finished) {
		kprDelegateFinished(self);
#if TARGET_OS_IPHONE
		KprSystemNowPlayingInfoSetMetadata(NULL, NULL);
#endif
	}
}

FskErr KprMediaLoad(KprMedia self, UInt32 type, char* path, char* mime, char* user, char* password)
{
	FskErr err = kFskErrNone;
	FskMediaPlayerPropertyIdAndValue properties = NULL;
	char* value = NULL;
	if (type == 2) {
		KprHTTPClientCookiesGet(path, &value);
		if (value) {
			char* name = "Cookie";
			bailIfError(FskMemPtrNewClear(sizeof(FskMediaPlayerPropertyIdAndValueRecord) * (1), &properties));
			properties[0].id = kFskMediaPropertyRequestHeaders;
			properties[0].value.type = kFskMediaPropertyTypeStringList;
			FskMemPtrNewClear(FskStrLen(name) + 1 + FskStrLen(value) + 1 + 1, &properties[0].value.value.str);
			FskStrCopy(properties[0].value.value.str, name);
			FskStrCopy(properties[0].value.value.str + FskStrLen(name) + 1, value);
		}
	}
	bailIfError(FskMediaPlayerNew(&self->mp, path, type, mime, properties));
	if (self->bitRate) {
		FskMediaPropertyValueRecord property;
		property.type = kFskMediaPropertyTypeInteger;
		property.value.integer = self->bitRate;
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyBitRate, &property);
	}	
	if ((type == 2) && user && password) {
		FskMediaPropertyValueRecord property;
		property.type = kFskMediaPropertyTypeStringList;
		FskMemPtrNewClear(FskStrLen(user) + 1 + FskStrLen(password) + 1 + 1, &property.value.str);
		FskStrCopy(property.value.str, user);
		FskStrCopy(property.value.str + FskStrLen(user) + 1, password);
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyAuthentication, &property);
		FskMediaPropertyEmpty(&property);
 	}
	FskMediaPlayerSetEventHandler(self->mp, KprMediaEventHandler, self);
	if (FskMediaPlayerGetPlayState(self->mp) >= kFskMediaPlayerStateStopped)
		KprMediaEventHandler(self->mp, self, kFskEventMediaPlayerInitialize, NULL);
bail:
    if (err)
        kprTraceDiagnostic(self->the, "Failed to load media %s. Error %s.", FskInstrumentationCleanPath(path), FskInstrumentationGetErrorString(err));
    FskMemPtrDispose(value);
	if (properties) {
		FskMediaPropertyEmpty(&properties[0].value);
		FskMemPtrDispose(properties);
	}
	return err;
}

void KprMediaOnCallback(void* it)
{
	KprMedia self = it;
	FskMediaPlayer mp = self->mp;
	SInt32 mpState = FskMediaPlayerGetPlayState(mp);
	FskMediaPropertyValueRecord property;
	FskErr err = kFskErrNone;
	Boolean stateChanged = false;
	Boolean timeChanged = false;
	UInt32 state = kprMediaStateFailed;
	double progress = 0;
	double time = 0;
	double duration = 0;
	double seekableFrom = 0;
	double seekableTo = 0;
	
	if (mpState <= kFskMediaPlayerStateFailed) {
        kprTraceDiagnostic(self->the, "Failed to instantiate media %s", self->message->url);
		err = kFskErrUnknown;
		if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertyError, &property)) {
			if (property.type == kFskMediaPropertyTypeInteger)
				err = property.value.integer;
			FskMediaPropertyEmpty(&property);
		}
	}
	else if (mpState >= kFskMediaPlayerStateStopped) {
		FskSampleTime sampleTime;
		FskMediaPlayerGetTime(mp, 1000, &sampleTime);
		time = (double)sampleTime;
		if (kFskErrNone == FskMediaPlayerGetDuration(mp, 1000, &sampleTime)) {
			duration = (double)sampleTime;
			seekableFrom = 0;
			seekableTo = (double)sampleTime;
			if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertySeekableSegment, &property)) {
				if ((property.type == kFskMediaPropertyTypeFloatList) && (property.value.numbers.count == 2)) {
					seekableFrom = property.value.numbers.number[0];
					seekableTo = property.value.numbers.number[1];
				}
				FskMediaPropertyEmpty(&property);
			}
		}
		else {
			duration = 0;
			seekableFrom = 0;
			seekableTo = 0;
		}
		if (mpState == kFskMediaPlayerStateStopped) {
			state = kprMediaStatePaused;
		}
		else if (mpState < kFskMediaPlayerStatePlaying) {
			state = kprMediaStateWaiting;
			if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertyBuffer, &property)) {
				if (property.type == kFskMediaPropertyTypeFloat)
					progress = property.value.number;
				FskMediaPropertyEmpty(&property);
			}
		}
		else {
			state = kprMediaStatePlaying;
		}
	}
	else {
		state = kprMediaStateWaiting;
	}
	if (kprMediaGetState(self) != state) { 
		if (state == kprMediaStatePlaying)
			KprContentStart(it);
		else if (state == kprMediaStatePaused)
			KprContentStop(it);
		else
			KprContentInvalidate(it);
		kprMediaSetState(self, state); 
		stateChanged = true; 
	}
	if (self->progress != progress) { self->progress = progress; stateChanged = true; }
	if (self->time != time) { self->time = time; timeChanged = true; }
	if (self->duration != duration) { self->duration = duration; timeChanged = true; }
	if (self->seekableFrom != seekableFrom) { self->seekableFrom = seekableFrom; timeChanged = true; }
	if (self->seekableTo != seekableTo) { self->seekableTo = seekableTo; timeChanged = true; }
	KprMediaFeedback(self, false, stateChanged, timeChanged, false, err);
}

void KprMediaOnMetaDataChange(void* it)
{
	KprMedia self = it;
	FskMediaPlayer mp = self->mp;
	FskMediaPropertyValueRecord meta;
#if TARGET_OS_IPHONE
	FskMediaPropertyValueRecord artwork = {0};
#endif
	Boolean flag = false;
	
    FskMemPtrDisposeAt(&self->title);
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "FullName", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			self->title = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
    FskMemPtrDisposeAt(&self->artist);
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Artist", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			self->artist = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
    FskMemPtrDisposeAt(&self->album);
	if (kFskErrNone == FskMediaPlayerGetMetadata(mp, "Album", 1, &meta, NULL)) {
		if ((meta.type == kFskMediaPropertyTypeString) && (meta.value.str))
			self->album = FskStrDoCopy(meta.value.str);
		FskMediaPropertyEmpty(&meta);
	}
	if (self->cover) {
		FskBitmapDispose(self->cover);
		self->cover = NULL;
	}
	if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertyFlags, &meta)) {
		if (meta.type == kFskMediaPropertyTypeInteger)
			flag = (meta.value.integer & 1) ? false : true;
		FskMediaPropertyEmpty(&meta);
	}
	if (flag) {
		FskErr err = FskMediaPlayerGetMetadata(mp, "AlbumArt", 1, &meta, NULL);
		if (err == kFskErrUnknownElement)
			err = FskMediaPlayerGetMetadata(mp, "WM/WMCollectionID", 1, &meta, NULL);
		if (err == kFskErrNone) {
			if (meta.type == kFskMediaPropertyTypeImage) {
				char* mime = (char*)meta.value.data.data;
				UInt32 length = FskStrLen(mime) + 1;
				FskBitmap bitmap = NULL;
				if (kFskErrNone == FskImageDecompressData(mime + length, meta.value.data.dataSize - length, mime, NULL, 0, 0, NULL, NULL, &bitmap)) {
					self->cover = bitmap;
#if TARGET_OS_IPHONE
					artwork = meta;
#endif
				}
			}
#if !TARGET_OS_IPHONE
			FskMediaPropertyEmpty(&meta);
#endif
		}
	}
	else
		self->flags |= kprMediaVideo;

#if TARGET_OS_IPHONE
	KprSystemNowPlayingInfoSetMetadata(self, &artwork);
	FskMediaPropertyEmpty(&artwork);
#endif

	KprMediaFeedback(self, true, false, false, false, kFskErrNone);
}

void KprMediaOnWarning(void* it, FskEvent event)
{
	KprMedia self = it;
	UInt32 paramSize;
	if (kFskErrNone == FskEventParameterGetSize(event, kFskEventParameterFileList, &paramSize)) {
		FskMemPtr param;
		if (kFskErrNone == FskMemPtrNew(paramSize, &param)) {
			if (kFskErrNone == FskEventParameterGet(event, kFskEventParameterFileList, param)) {
				char *string = (char*)param;
				if (!FskStrCompare(string, "bandwidth.insufficient")) {
					if (!(self->flags & kprInsufficientBandwidth)) {
						self->flags |= kprInsufficientBandwidth;
						KprMediaFeedback(self, false, true, false, false, kFskErrNone);
					}
				}
				else if (!FskStrCompare(string, "bandwidth.ok")) {
					if (self->flags & kprInsufficientBandwidth) {
						self->flags &= ~kprInsufficientBandwidth;
						KprMediaFeedback(self, false, true, false, false, kFskErrNone);
					}
				}
				else if (!FskStrCompare(string, "unsupported.codec.audio")) {
					string += FskStrLen(string) + 1;
					KprMediaReportWarning(self, string);
				}
				else if (!FskStrCompare(string, "unsupported.codec.video")) {
					string += FskStrLen(string) + 1;
					KprMediaReportWarning(self, string);
				}
			}
		}
		FskMemPtrDispose(param);
	}
}

void KprMediaReportWarning(KprMedia self UNUSED, char* warning)
{
    kprTraceDiagnostic(self->the, "Media %s reports warning %s", self->url, warning);
}

void KprMediaUnload(KprMedia self)
{
	self->flags &= ~(kprInsufficientBandwidth | kprMediaReady | kprMediaVideo);
	kprMediaSetState(self, kprMediaStateWaiting);
	if (self->mp) {
		FskMediaPlayerSetEventHandler(self->mp, NULL, NULL);
		FskMediaPlayerDispose(self->mp);
		self->mp = NULL;
	}
    FskMemPtrDisposeAt(&self->url);
	self->progress = 0;
	self->time = 0;
	self->duration = 0;
	self->seekableFrom = 0;
	self->seekableTo = 0;
    FskMemPtrDisposeAt(&self->title);
    FskMemPtrDisposeAt(&self->artist);
    FskMemPtrDisposeAt(&self->album);
	if (self->cover) {
		FskBitmapDispose(self->cover);
		self->cover = NULL;
	}
	if (self->message)
        KprMessageCancel(self->message);
	KprContentInvalidate((KprContent)self);
	KprContentReflow((KprContent)self, kprSizeChanged);
}

FskErr KprMediaSnifferLoad(KprMedia self, char* url, char* mime)
{
	char* sniff = NULL;
	if (url) {
		if (!FskStrCompareWithLength(url, "file://", 7))
			return KprMediaSnifferLoadFile(self, url, mime);
		if (mime)
			return KprMediaLoad(self, 2, url, mime, NULL, NULL);
		if (!FskMediaPlayerSniffForMIME(NULL, 0, NULL, url, &sniff)) {
			FskErr err = KprMediaLoad(self, 2, url, sniff, NULL, NULL);
			FskMemPtrDispose(sniff);
			return err;
		}
		if ((!FskStrCompareWithLength(url, "http://", 7)) || (!FskStrCompareWithLength(url, "https://", 8)) || (!FskStrCompareWithLength(url, "xkpr://", 7)))
			return KprMediaSnifferLoadMessage(self, url, mime);
#if TARGET_OS_IPHONE
		if (!FskStrCompareWithLength(url, "assets-library://", 17))
			return KprMediaSnifferLoadMessage(self, url, mime);
#endif
	}
	return kFskErrUnimplemented;
}

FskErr KprMediaSnifferLoadFile(KprMedia self, char* url, char* mime)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile fref = NULL;
	char* sniff = NULL;
	
	bailIfError(KprURLToPath(url, &path));
	if (!mime) {
		unsigned char buffer[4096];
		UInt32 size;
		bailIfErrorWithDiagnostic(FskFileOpen(path, kFskFilePermissionReadOnly, &fref), self->the, "Failed to open media file %s. Error %s.", FskInstrumentationCleanPath(path), FskInstrumentationGetErrorString(err));
		bailIfError(FskFileRead(fref, sizeof(buffer), buffer, &size));
		FskFileClose(fref);
		fref = NULL;
		bailIfErrorWithDiagnostic(FskMediaPlayerSniffForMIME(buffer, size, NULL, self->url, &sniff), self->the, "Unrecognized media file type %s. Error %s.", FskInstrumentationCleanPath(path), FskInstrumentationGetErrorString(err));
		mime = sniff;
	}
	bailIfError(KprMediaLoad(self, 1, path, mime, NULL, NULL));
bail:
	FskMemPtrDispose(sniff);
	FskFileClose(fref);
	FskMemPtrDispose(path);
	return err;
}

FskErr KprMediaSnifferLoadMessage(KprMedia self, char* url, char* mime UNUSED)
{
	FskErr err = kFskErrNone;
	KprMessage message = NULL;
	kprMediaSetState(self, kprMediaStateWaiting);
	KprMediaFeedback(self, false, true, false, false, err);
	bailIfError(KprMessageNew(&message, url));
	bailIfError(KprMessageSetRequestHeader(message, "Range", "bytes=0-4095"));
	message->sniffing = true;
	self->message = message;
	KprMessageInvoke(message, KprMediaSnifferLoadMessageComplete, KprMediaSnifferLoadMessageDispose, self);
bail:
	return err;
}

void KprMediaSnifferLoadMessageComplete(KprMessage message, void* it)
{
	KprMedia self = it;
	FskErr err = kFskErrNone;
	char* url = KprMessageGetResponseHeader(message, "location");
	char* mime = KprMessageGetResponseHeader(message, "content-type");
	char* sniff = NULL;
	if (!url)
		url = message->url;
	if (message->response.body) {
		bailIfErrorWithDiagnostic(FskMediaPlayerSniffForMIME(message->response.body, message->response.size, NULL, message->url, &sniff), self->the, "Unrecognized media  type %s. Error %s.", url, FskInstrumentationGetErrorString(err));
		mime = sniff;
	}
    if (!FskStrCompareWithLength(url, "xkpr://", 7))
		err = kFskErrUnsupportedSchema;
    else if (!mime)
        err = kFskErrUnsupportedMIME;
    else 
		err = KprMediaLoad(self, 2, url, mime, message->user, message->password);
bail:
	FskMemPtrDispose(sniff);
	if (err != kFskErrNone) {
		kprMediaSetState(self, kprMediaStateFailed);
		KprMediaFeedback(self, false, true, false, false, err);
	}
}

void KprMediaSnifferLoadMessageDispose(void* it)
{
	KprMedia self = it;
	self->message = NULL;
}

/* ECMASCRIPT */

void KPR_Media(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprMedia self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprMediaNew(&self, &coordinates));
	kprContentConstructor(KPR_Media);
	if (c > 2)
		xsCall2_noResult(xsThis, xsID_load, xsArg(1), xsArg(2));
	else if (c > 1)
		xsCall1_noResult(xsThis, xsID_load, xsArg(1));
}

void KPR_media_get_album(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	if (self->album)
		xsResult = xsString(self->album);
}

void KPR_media_get_artist(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	if (self->artist)
		xsResult = xsString(self->artist);
}

void KPR_media_get_bitRate(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	FskMediaPlayer mp = self->mp;
	FskMediaPropertyValueRecord property;
	UInt32 bitRate = 0;
	if (kFskErrNone == FskMediaPlayerGetProperty(mp, kFskMediaPropertyBitRate, &property)) {
		if (property.type == kFskMediaPropertyTypeInteger) {
			bitRate = property.value.integer;
		}
	}
	if (!bitRate) {
		UInt32 index = 0;
		while (true) {
			void *track;

			if (kFskErrNone != FskMediaPlayerGetTrack(mp, index++, &track))
				break;

			if (kFskErrNone == FskMediaPlayerTrackGetProperty(mp, track, kFskMediaPropertyBitRate, &property)) {
				if (property.type == kFskMediaPropertyTypeInteger) {
					bitRate += property.value.integer;
				}
			}
		}
	}
	if (bitRate)
		xsResult = xsInteger(bitRate);
}

void KPR_media_get_duration(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->duration);
}

void KPR_media_get_insufficientBandwidth(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprInsufficientBandwidth) ? xsTrue : xsFalse;
}

void KPR_media_get_progress(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->progress);
}

void KPR_media_get_ready(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = (self->flags & kprMediaReady) ? xsTrue : xsFalse;
}

void KPR_media_get_seekableFrom(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->seekableFrom);
}

void KPR_media_get_seekableTo(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->seekableTo);
}

void KPR_media_get_seeking(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsBoolean(KprMediaGetSeeking(self));
}

void KPR_media_get_state(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->state);
}

void KPR_media_get_time(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsResult = xsNumber(self->time);
}

void KPR_media_get_title(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	if (self->title)
		xsResult = xsString(self->title);
}

void KPR_media_get_url(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	char* url = self->url;
	if (url)
		xsResult = xsString(url);
}

void KPR_media_get_volume(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	float volume = 0;
	KprMediaGetVolume(self, &volume);
	xsResult = xsNumber(volume);
}

void KPR_media_hasCover(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	if ((self->flags & kprMediaVideo) == 0)
		xsResult = (self->cover) ? xsTrue : xsFalse;
}

void KPR_media_set_local(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	if (self->mp) {
		FskMediaPropertyValueRecord property;
        property.value.b = xsTest(xsArg(0));
        property.type = kFskMediaPropertyTypeBoolean;
		FskMediaPlayerSetProperty(self->mp, kFskMediaPropertyLocal, &property);
	}
}

void KPR_media_set_seeking(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	KprMediaSetSeeking(self, (xsTest(xsArg(0))) ? true : false);
}

void KPR_media_set_state(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	xsIntegerValue state = xsToInteger(xsArg(0));
	if (state == kprMediaStatePaused)
		KprMediaStop(self);
	else if (state == kprMediaStatePlaying)
		KprMediaStart(self);
	else
		xsError(kFskErrParameterError);
}

void KPR_media_set_time(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	KprMediaSetTime(self, xsToNumber(xsArg(0)));
}

void KPR_media_set_url(xsMachine *the)
{
	xsCall1_noResult(xsThis, xsID_load, xsArg(0));
}

void KPR_media_set_volume(xsMachine *the)
{
	KprMedia self = xsGetHostData(xsThis);
	float volume = (float)xsToNumber(xsArg(0));
	KprMediaSetVolume(self, volume);
}

void KPR_media_set_bitRate(xsMachine *the)
{

	KprMedia self = xsGetHostData(xsThis);
	xsIntegerValue bitRate = xsToInteger(xsArg(0));
	KprMediaSetBitRate(self, bitRate);
}

void KPR_media_load(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprMedia self = kprGetHostData(xsThis, this, media);
	char* url = NULL;
	char* mime = NULL;
	xsTry {
		if ((c > 0) && xsTest(xsArg(0))) {
			xsStringValue reference = xsToString(xsArg(0));
			xsStringValue base = xsToString(xsModuleURL);
			xsThrowIfFskErr(KprURLMerge(base, reference, &url));	
		}
		if ((c > 1) && xsTest(xsArg(1)))
			mime = xsToString(xsArg(1));
		KprMediaSetURL(self, url, mime);
		FskMemPtrDispose(url);
	}
	xsCatch {
		FskMemPtrDispose(url);
	}
}

void KPR_media_start(xsMachine *the)
{
	KprMedia self = kprGetHostData(xsThis, this, media);
	KprMediaStart(self);
}

void KPR_media_stop(xsMachine *the)
{
	KprMedia self = kprGetHostData(xsThis, this, media);
	KprMediaStop(self);
}

void KPR_media_hibernate(xsMachine *the)
{
	KprMedia self = kprGetHostData(xsThis, this, media);
	KprMediaHibernate(self);
}

void KPR_Media_canPlayAudio(xsMachine *the)
{
	char *mime = xsToString(xsArg(0));
#if USE_AUDIO_QUEUE
	if (FskStrCompare(mime, "x-audio-codec/aac") == 0 || FskStrCompare(mime, "x-audio-codec/mp3") == 0) {
		xsResult = xsTrue;
		return;
	}
#endif
	if (kFskErrNone == FskAudioDecompressNew(NULL, 0, mime, 0, 0, NULL, 0))
		xsResult = xsTrue;
	else
		xsResult = xsFalse;
}

void KPR_Media_canPlayVideo(xsMachine *the)
{
	char *mime = xsToString(xsArg(0));
	if (kFskErrNone == FskImageDecompressNew(NULL, 0, mime, NULL))
		xsResult = xsTrue;
	else
		xsResult = xsFalse;
}

void KPR_Media_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Media"));
	xsNewHostProperty(xsResult, xsID("FAILED"), xsInteger(kprMediaStateFailed), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("PAUSED"), xsInteger(kprMediaStatePaused), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("PLAYING"), xsInteger(kprMediaStatePlaying), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("WAITING"), xsInteger(kprMediaStateWaiting), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("canPlayAudio"), xsNewHostFunction(KPR_Media_canPlayAudio, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("canPlayVideo"), xsNewHostFunction(KPR_Media_canPlayVideo, 1), xsDefault, xsDontScript);
}
