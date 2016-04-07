/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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


#if 1
#define ILOG(...)
#define MLOG(...)
#else
#define ILOG(...) fprintf(stderr, __VA_ARGS__);
//#define MLOG(...) fprintf(stderr, __VA_ARGS__);
#define MLOG(...) do { FskTimeRecord a, b; \
    FskThread t = FskThreadGetCurrent(); \
 FskTimeGetNow(&a); \
 b = a; \
 FskTimeSub(&t->lastLogTime, &b); \
 fprintf(stderr, "%5d.%03d ∆%01d.%04d ", a.seconds, a.useconds / 1000, b.seconds, b.useconds / 100); fprintf(stderr, __VA_ARGS__); \
    t->lastLogTime = a; \
 } while (0)

#endif

#define threadName FskThreadGetCurrent()->name

#define __FSKBITMAP_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKWINDOW_PRIV__

#include "FskPlatform.h"
#include "FskFrameBuffer.h"

#include "KplScreen.h"
#include "FskMemory.h"
#include "FskBitmap.h"
#include "FskThread.h"

#include <fcntl.h>
#include <linux/fb.h>

void initializeLinuxInput(int width, int height);
void terminateLinuxInput();

int gScreenWidth = 1920, gScreenHeight = 1080;

#define kScreenDepth		32
#define kScreenPixelFormat	kFskBitmapFormat32BGRA

FskInstrumentedSimpleType(KplScreen, kplscreen);

typedef struct KplScreenStruct KplScreenRecord, *KplScreen;

struct KplScreenStruct {
	KplBitmap	bitmap;

	Boolean		drawingPumpEnabled;
	Boolean		unlocked;
	SInt32		updateInterval;
	SInt32		vSyncInterval;

	FskThread		flipThread;
	FskSemaphore	flipSemaphore;


	FskTimeRecord	lastUpdateTime;
	FskTimeRecord	lastFlipTime;
	FskTimeRecord	nextFlipTime;
	FskTimeRecord	vSyncIntervalTime;
	FskTimeRecord	updateIntervalTime;

	UInt32		callbackPostedCount;
	UInt32		callbackFiredCount;
};

#define MAX_FRAMERATE 60
#define MAX_FRAMERATE_MS (1000 / MAX_FRAMERATE)

static FskErr KplScreenHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
static FskErr KplScreenSetProperty(UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetProperty(UInt32 propertyID, FskMediaPropertyValue property);

static FskErr KplScreenGetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenSetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenSetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

static void flipThread(void *refcon);
static void drawingPumpCallback(void *arg0, void *arg1, void *arg2, void *arg3);

extern Boolean gQuitting;
static KplScreen gKplScreen = NULL;

static FskMediaPropertyEntryRecord gKplScreenProperties[] = {
	{ kFskFrameBufferPropertyContinuousDrawing, kFskMediaPropertyTypeBoolean, KplScreenGetContinuousDrawing, KplScreenSetContinuousDrawing },
	{ kFskFrameBufferPropertyUpdateInterval, kFskMediaPropertyTypeInteger, KplScreenGetUpdateInterval, KplScreenSetUpdateInterval },
	{ kFskMediaPropertyUndefined, kFskMediaPropertyTypeUndefined, NULL, NULL }
};

static char deviceName[] = "/dev/fb0";

void fbGetScreenDim(int *width, int *height) {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	int fbfd;

	char *fbdev = deviceName;
	if (getenv("FRAMEBUFFER"))
		fbdev = getenv("FRAMEBUFFER");

	fbfd = open(fbdev, O_RDWR);
	if (-1 != fbfd) {
		if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
			fprintf(stderr, "Error: fb - reading variable info failed %d\n", errno);
			exit(2);
		}
		*width = vinfo.xres;
		*height = vinfo.yres;
	}
}

FskErr KplScreenGetBitmap(KplBitmap *bitmap)
{
	FskErr err = kFskErrNone;
	FrameBufferVectorSet fbv;

	if (!gKplScreen) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplScreenRecord), (FskMemPtr *)&gKplScreen));
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplBitmapRecord), (FskMemPtr *)&gKplScreen->bitmap));
		fbGetScreenDim(&gScreenWidth, &gScreenHeight);
fprintf(stderr, "ScreenWidth: %d, ScreenHeight: %d\n", gScreenWidth, gScreenHeight);

		gKplScreen->bitmap->width = gScreenWidth;
		gKplScreen->bitmap->height = gScreenHeight;
		gKplScreen->bitmap->pixelFormat = kScreenPixelFormat;
		gKplScreen->bitmap->depth = kScreenDepth;
		gKplScreen->bitmap->rowBytes = gScreenWidth * (kScreenDepth / 8);
		BAIL_IF_ERR(err = FskMemPtrNewClear(gKplScreen->bitmap->rowBytes * gScreenHeight, (FskMemPtr *)&gKplScreen->bitmap->baseAddress));
		initializeLinuxInput(gScreenWidth, gScreenHeight);
	}

	gKplScreen->vSyncInterval = MAX_FRAMERATE_MS;
	gKplScreen->vSyncIntervalTime.useconds = MAX_FRAMERATE_MS * 1000;

	FskSemaphoreNew(&gKplScreen->flipSemaphore, 0);
	FskThreadCreate(&gKplScreen->flipThread, flipThread, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, gKplScreen, "flip");

	FskFrameBufferGetVectors(&fbv);
	if (fbv) {
		fbv->doHasProperty = KplScreenHasProperty;
		fbv->doSetProperty = KplScreenSetProperty;
		fbv->doGetProperty = KplScreenGetProperty;
	}

	*bitmap = gKplScreen->bitmap;

bail:
	return err;
}

FskErr KplScreenDisposeBitmap(KplBitmap bitmap)
{

	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

	if (gKplScreen) {
		if (gKplScreen->flipThread)
			FskThreadJoin(gKplScreen->flipThread);
		FskMemPtrDispose(gKplScreen->bitmap);
		FskMemPtrDisposeAt(&gKplScreen);

		FskSemaphoreDispose(gKplScreen->flipSemaphore);
	}

	terminateLinuxInput();
	return kFskErrNone;
}

FskErr KplScreenGetAuxInfo(unsigned char **auxInfo, UInt32 *auxInfoSize) {
	if (auxInfo)
		*auxInfo = (unsigned char*)gKplScreen;
	if (auxInfoSize)
		*auxInfoSize = sizeof(gKplScreen);
	return kFskErrNone;
}

FskErr KplScreenHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(gKplScreenProperties, propertyID, get, set, dataType);
}

FskErr KplScreenSetProperty(UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(gKplScreenProperties, NULL, NULL, propertyID, property);
}

FskErr KplScreenGetProperty(UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(gKplScreenProperties, NULL, NULL, propertyID, property);
}

FskErr KplScreenGetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeBoolean;
	property->value.b = gKplScreen->drawingPumpEnabled;
	return kFskErrNone;
}

FskErr KplScreenSetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	Boolean on = property->value.b;

	if (on) {
		if (!gKplScreen->drawingPumpEnabled) {
			gKplScreen->drawingPumpEnabled = true;
			MLOG("[%s] ** Pi START drawing pump **\n", threadName);
			FskThreadPostCallback(FskThreadGetMain(), drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);
		}
	}
	else {
		if (gKplScreen->drawingPumpEnabled) {
			gKplScreen->drawingPumpEnabled = false;
			MLOG("[%s] ** Pi STOP drawing pump **\n", threadName);
		}
	}

	return kFskErrNone;
}

FskErr KplScreenGetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = gKplScreen->updateInterval;
	return kFskErrNone;
}

FskErr KplScreenSetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	UInt32 i, updateRate;

	// We set the actual interval based on the vsync interval
	updateRate = (UInt32)floor(((double)property->value.integer / gKplScreen->vSyncInterval + 0.5));

	if (!updateRate) updateRate = 1;
	gKplScreen->updateInterval = updateRate * gKplScreen->vSyncInterval;

	MLOG("Setting update interval to %ld ms", gKplScreen->updateInterval);

	FskTimeClear(&gKplScreen->updateIntervalTime);
	for (i = 0; i < updateRate; ++i)
		FskTimeAdd(&gKplScreen->vSyncIntervalTime, &gKplScreen->updateIntervalTime);

	MLOG("Setting update interval time to %ld.%06lds", gKplScreen->updateIntervalTime.seconds, gKplScreen->updateIntervalTime.useconds);

	return kFskErrNone;
}

FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	return kFskErrNone;
}

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
		gKplScreen->unlocked = true;

	FskSemaphoreRelease(gKplScreen->flipSemaphore);
	return kFskErrNone;
}

FskErr KplScreenDrawBitmap(KplBitmap src, KplRectangle srcRect, KplBitmap screen, KplRectangle dstRect)
{
	return kFskErrNone;
}

static void drawingPumpUpdate(FskWindow win)
{
	FskTimeRecord delta, now;

	FskTimeGetNow(&now);

	gKplScreen->unlocked = false;

	FskTimeCopy(&delta, &now);
	FskTimeSub(&gKplScreen->lastUpdateTime, &delta);

	FskTimeCopy(&gKplScreen->lastUpdateTime, &now);

	MLOG("[%s] called %d ms after last update\n", threadName, FskTimeInMS(&delta)); 

	// estimate next flip time by adding vSync interval to previous flip time
	FskTimeCopy(&gKplScreen->nextFlipTime, &gKplScreen->lastFlipTime);
	FskTimeAdd(&gKplScreen->vSyncIntervalTime, &gKplScreen->nextFlipTime);
	while (FskTimeCompare(&gKplScreen->nextFlipTime, &now) > 0)
		FskTimeAdd(&gKplScreen->vSyncIntervalTime, &gKplScreen->nextFlipTime);

	MLOG("Next flip time %ld.%06ld \n", gKplScreen->nextFlipTime.seconds, gKplScreen->nextFlipTime.useconds);

	FskWindowCheckEventQueue(win);
	FskWindowUpdate(win, &gKplScreen->nextFlipTime);

	if (!gKplScreen->unlocked)
		FskSemaphoreRelease(gKplScreen->flipSemaphore);

	MLOG("[%s] Finished callback %ld, nextFlipTime %1d.%03d\n", threadName, gKplScreen->callbackFiredCount, gKplScreen->nextFlipTime.seconds, gKplScreen->nextFlipTime.useconds / 1000);

}

static void drawingPumpCallback(void *arg0, void *arg1, void *arg2, void *arg3)
{
	UInt32 callback = (UInt32)arg0;
	UInt32 pending;
	FskWindow win = FskWindowGetActive();

//	MLOG("[%s] In drawing pump callback %ld, pre-fired: %ld\n", threadName, callback, gKplScreen->callbackFiredCount);
	gKplScreen->callbackFiredCount = callback;

	pending = gKplScreen->callbackPostedCount - gKplScreen->callbackFiredCount;
	if (pending > 0) {
		MLOG("[%s] There are %d callbacks pending, skip callback %ld\n", threadName, pending, callback);
		return;
	}

	if (gKplScreen->drawingPumpEnabled && (NULL != win)) {
		drawingPumpUpdate(win);
	}
}

void flipThread(void *refcon)
{
	KplScreen screen = refcon;
	FskThread mainThread = FskThreadGetMain();
	FskTimeRecord later;
	SInt32 ms;

	FskThreadInitializationComplete(FskThreadGetCurrent());
	while (!gQuitting) {
		//Save the last Flip time
		FskTimeGetNow(&gKplScreen->lastFlipTime);
		FskTimeCopy(&later, &gKplScreen->nextFlipTime);
		FskTimeSub(&gKplScreen->lastFlipTime, &later);
		ms = FskTimeInMS(&later);

		if (ms > 0) {
			FskDelay(ms);
			MLOG("[%s] delay %d ms\n", threadName, ms);
		}

		// Kick off the next cycle
		if (gKplScreen->drawingPumpEnabled) {
			FskSemaphoreAcquire(screen->flipSemaphore);
			gKplScreen->callbackPostedCount++;
			FskThreadPostCallback(mainThread, drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);
		}
	}
}

