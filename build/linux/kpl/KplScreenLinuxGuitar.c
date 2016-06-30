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
#undef DEBUG 
#define DEBUG 0
#if DEBUG
#define MLOG(...) do { FskTimeRecord a; FskTimeGetNow(&a); fprintf(stderr, "%5d.%03d ", a.seconds, a.useconds / 1000); fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define MLOG(...)
#endif

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

#include "FskRectBlit.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <math.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/ioctl.h>

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gKplScreenTypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kplscreen"};
#endif


static char deviceName[] = "/dev/fb0";
#define FBIO_WAITFORVSYNC           _IOW('F', 0x20, __u32)

int gScreenWidth = 1920, gScreenHeight = 1080;

#define SUPPORT_FLIP_THREAD 1


typedef struct KplScreenStruct KplScreenRecord, *KplScreen;
struct KplScreenStruct {
	char* framebuffer;
	KplBitmap bitmap;
	FskThread thread;
	int fbfd;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	int rowBytes;
	int screensize;

#if SUPPORT_FLIP_THREAD
	FskThread flipThread;
	FskSemaphore flipSemaphore;
#endif
	Boolean drawingPumpEnabled;
	Boolean unlocked;
	SInt32 updateInterval;				// 0 = let the frame buffer update on every v-sync
	SInt32 vSyncInterval;

	FskTimeRecord lastUpdateTime;
	FskTimeRecord lastFlipTime;
	FskTimeRecord vSyncIntervalTime;
	FskTimeRecord updateIntervalTime;
	
	UInt32 callbackPostedCount;
	UInt32 callbackFiredCount;

	int displayPage;
	char *baseAddr[2];

	FskBitmap bitmaps[2];
    int bitmapsFlagRead;
    int bitmapsFlagWrite;
    int bitmapsReady;

    FskBitmap touchDown;
    
    FskMutex withCare;
};

#define ONSCREEN_PAGE(screen)	(screen->displayPage)
//#define OFFSCREEN_PAGE(screen)	(screen->displayPage == 1 ? 0 : 1)	
#define OFFSCREEN_PAGE(screen)	(screen->displayPage)
#define DISPLAY_OFFSET(screen)	(screen->displayPage == 1 ? screen->finfo.ypanstep : 0)


#define BITS_NULL	((void *)1)	// to make sure the framework crashes if the screen bitmap bits are accessed without locking

static FskErr KplScreenHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
static FskErr KplScreenSetProperty(UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetProperty(UInt32 propertyID, FskMediaPropertyValue property);

static FskErr KplScreenGetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenSetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenSetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetRetainsPixelsBetweenUpdates(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr KplScreenGetDisplayCopy(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property);

#if SUPPORT_FLIP_THREAD
static void flipThread(void *refcon);
#endif


static void drawingPumpCallback(void *arg0, void *arg1, void *arg2, void *arg3);
static void motionMovedCB(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);

int gNumPts = 0;
FskPointAndTicksRecord *gPts = NULL;

extern Boolean gQuitting;
static KplScreen gKplScreen = NULL;

static FskMediaPropertyEntryRecord gKplScreenProperties[] = {
	{ kFskFrameBufferPropertyContinuousDrawing, kFskMediaPropertyTypeBoolean, KplScreenGetContinuousDrawing, KplScreenSetContinuousDrawing },
	{ kFskFrameBufferPropertyUpdateInterval, kFskMediaPropertyTypeInteger, KplScreenGetUpdateInterval, KplScreenSetUpdateInterval },
	{ kFskFrameBufferPropertyRetainsPixelsBetweenUpdates, kFskMediaPropertyTypeBoolean, KplScreenGetRetainsPixelsBetweenUpdates, NULL },
	{ kFskFrameBufferPropertyDisplayCopy, kFskMediaPropertyTypeBitmap, KplScreenGetDisplayCopy, NULL },
	{ kFskMediaPropertyUndefined, kFskMediaPropertyTypeUndefined, NULL, NULL }
};

void initializeLinuxInput(int width, int height);
void terminateLinuxInput();

#if 0
#pragma mark -
#pragma mark KplScreen
#endif
// --------------------------------------------------
// KplScreen
// --------------------------------------------------

void devFBFlip(KplScreen screen) {
	UInt32 dummy;
	struct fb_var_screeninfo *var = &screen->vinfo;
    Boolean skipCopy = (0 == screen->bitmapsReady);
//    Boolean skipCopy = 1;

    if (skipCopy) {
//        ioctl(screen->fbfd, FBIO_WAITFORVSYNC, &dummy);
        return;
    }

    FskMutexAcquire(screen->withCare);
//        FskMemMove(screen->baseAddr[OFFSCREEN_PAGE(screen)], screen->bitmaps[1 & screen->bitmapsFlagRead]->bits, gScreenWidth * gScreenHeight * 4);
        FskMemMove(screen->baseAddr[OFFSCREEN_PAGE(screen)], screen->bitmaps[0]->bits, gScreenWidth * gScreenHeight * 4);
        screen->bitmapsFlagRead += 1;
        screen->bitmapsReady -= 1;
    FskMutexRelease(screen->withCare);

//	if (ioctl(screen->fbfd, FBIO_WAITFORVSYNC, &dummy) < 0) {
//		fprintf(stderr, "	devFBFlip - problems with WAITFORVSYNC - %d\n", errno);
//	}

	screen->displayPage = OFFSCREEN_PAGE(screen);
	
#if 0
	var->xoffset = 0;
	var->yoffset = DISPLAY_OFFSET(screen);
	var->activate = FB_ACTIVATE_VBL;

    if (ioctl(screen->fbfd, FBIOPAN_DISPLAY, var) < 0) {
        fprintf(stderr, "	devFBFlip - problems with PAN - %d\n", errno);
    }
#endif
}

FskErr KplScreenDisposeBitmap(KplBitmap bitmap)
{
	FskErr err = kFskErrNone;
	KplScreen screen = gKplScreen;

	if (bitmap != screen->bitmap)
		return kFskErrOperationFailed;

	if (screen->thread) {
//		KplScreenEventWakeUp();
		FskThreadJoin(screen->thread);
		screen->thread = NULL;
	}
#if SUPPORT_FLIP_THREAD
	if (screen->flipThread) {
		KplScreenUnlockBitmap(bitmap);
		FskThreadJoin(screen->flipThread);
		screen->flipThread = NULL;
	}
#endif
	if (screen->fbfd == -1) {
		FskMemPtrDispose(screen->framebuffer);
	}
	else {
		munmap(screen->framebuffer, screen->finfo.smem_len);
		close(screen->fbfd);
	}

    FskBitmapDispose(screen->touchDown);

#if SUPPORT_FLIP_THREAD
	FskSemaphoreDispose(screen->flipSemaphore);
#endif

	FskMemPtrDispose(screen);

	terminateLinuxInput();

	gKplScreen = NULL;
	return err;
}


FskErr KplScreenGetAuxInfo(unsigned char **auxInfo, UInt32 *auxInfoSize)
{
	if (auxInfo)
		*auxInfo = (unsigned char*)gKplScreen;
	if (auxInfoSize)
		*auxInfoSize = sizeof(gKplScreen);

	return kFskErrNone;
}

FskErr KplScreenGetBitmap(KplBitmap *bitmap)
{
	FskErr err = kFskErrNone;
	KplScreen screen = NULL;
	int bpp;

	// Perform one-time intialization on the first request
	if (!gKplScreen) {
		FrameBufferVectorSet vectors;
		char *fbdev = deviceName;
		FskTimeRecord now;

		if (getenv("FRAMEBUFFER"))
			fbdev = getenv("FRAMEBUFFER");
		
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplScreenRecord), (FskMemPtr *)&screen));
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(KplBitmapRecord), (FskMemPtr *)&screen->bitmap));

		screen->fbfd = open(fbdev, O_RDWR);
		if (-1 != screen->fbfd) {
			if (ioctl(screen->fbfd, FBIOGET_FSCREENINFO, &screen->finfo)) {
				fprintf(stderr, "Error: fb - reading fixed info\n");
				exit(2);
			}

			if (ioctl(screen->fbfd, FBIOGET_VSCREENINFO, &screen->vinfo)) {
				fprintf(stderr, "Error: fb - reading variable info\n");
				exit(3);
			}

fprintf(stderr, "Red: offset: %d, length: %d, msb: %d\n",
	screen->vinfo.red.offset, screen->vinfo.red.length, screen->vinfo.red.msb_right);
fprintf(stderr, "Green: offset: %d, length: %d, msb: %d\n",
	screen->vinfo.green.offset, screen->vinfo.green.length, screen->vinfo.green.msb_right);
fprintf(stderr, "Blue: offset: %d, length: %d, msb: %d\n",
	screen->vinfo.blue.offset, screen->vinfo.blue.length, screen->vinfo.blue.msb_right);
fprintf(stderr, "Transp: offset: %d, length: %d, msb: %d\n",
	screen->vinfo.transp.offset, screen->vinfo.transp.length, screen->vinfo.transp.msb_right);

			gScreenWidth = screen->vinfo.xres;
			gScreenHeight = screen->vinfo.yres;
			bpp = screen->vinfo.bits_per_pixel;
fprintf(stderr, "width: %d, height: %d, bpp: %d\n", gScreenWidth, gScreenHeight, bpp);
			screen->rowBytes = screen->vinfo.xres_virtual * bpp / 8;
			screen->screensize = gScreenHeight * screen->rowBytes;

			screen->framebuffer = (char*)mmap((void*)screen->finfo.smem_start, screen->finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, screen->fbfd, 0);

fprintf(stderr, "framebuffer mem_start: %d, length %d\n", screen->finfo.smem_start, screen->finfo.smem_len);
fprintf(stderr, " framebuffer at %x\n", screen->framebuffer);
			if (screen->framebuffer == (char*)-1) {
				fprintf(stderr,"Error: can't map framebuffer %d\n", errno);
				exit(4);
			}

			screen->displayPage = 0;
			screen->baseAddr[0] = screen->framebuffer;
//			screen->baseAddr[1] = screen->framebuffer + screen->finfo.ypanstep * screen->finfo.line_length;

            FskBitmapNew(gScreenWidth, gScreenHeight, kFskBitmapFormat32BGRA, &screen->bitmaps[0]);
//            FskBitmapNew(gScreenWidth, gScreenHeight, kFskBitmapFormat32BGRA, &screen->bitmaps[1]);
            FskBitmapWriteBegin(screen->bitmaps[0], NULL, NULL, NULL);
//            FskBitmapWriteBegin(screen->bitmaps[1], NULL, NULL, NULL);
            
            FskMutexNew(&screen->withCare, "screen flipper");
		}
		else {
			fprintf(stderr, "Error: can't open framebuffer device - use ram\n");
			gScreenWidth = 320;
			gScreenHeight = 240;
			bpp = 32;
			screen->rowBytes = gScreenWidth * bpp / 8;
			screen->screensize = gScreenHeight * screen->rowBytes;
			FskMemPtrNew(screen->screensize, (FskMemPtr*)&screen->framebuffer);
		}
#if 0
		devFBFlip(screen);
		devFBFlip(screen);
#endif
		FskTimeGetNow(&now);
		devFBFlip(screen);
        FskTimeGetNow(&screen->vSyncIntervalTime);
        FskTimeSub(&now, &screen->vSyncIntervalTime);
        screen->vSyncInterval = FskTimeInMS(&screen->vSyncIntervalTime);
//        if (!screen->vSyncInterval) screen->vSyncInterval = 1;
        if (!screen->vSyncInterval) screen->vSyncInterval = 32;

		FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "VsyncInterval = %d", screen->vSyncInterval);

		screen->bitmap->baseAddress = screen->baseAddr[screen->displayPage];
		screen->bitmap->rowBytes = screen->rowBytes;
		screen->bitmap->pixelFormat = kFskBitmapFormat32BGRA;
		screen->bitmap->depth = 32;
		screen->bitmap->width = gScreenWidth;
		screen->bitmap->height = gScreenHeight;
#if SUPPORT_FLIP_THREAD
		FskSemaphoreNew(&screen->flipSemaphore, 0);
		FskThreadCreate(&screen->flipThread, flipThread, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit | kFskThreadFlagsHighPriority, screen, "dfb flip");
#endif
		BAIL_IF_ERR(err = FskFrameBufferGetVectors(&vectors));
		vectors->doHasProperty = KplScreenHasProperty;
		vectors->doSetProperty = KplScreenSetProperty;
		vectors->doGetProperty = KplScreenGetProperty;
		gKplScreen = screen;

		initializeLinuxInput(gScreenWidth, gScreenHeight);

	}
	*bitmap = gKplScreen->bitmap;
	if (NULL == *bitmap)
		err = kFskErrNotFound;

bail:
	if (err) {
		if (screen) {
//dispose of data structures here
			FskMemPtrDispose(screen);
		}
		*bitmap = NULL;
	}
	return err;
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

#if SUPPORT_FLIP_THREAD
	if (on) {
		if (!gKplScreen->drawingPumpEnabled) {
			gKplScreen->drawingPumpEnabled = true;
			FskTimeClear(&gKplScreen->lastUpdateTime);
			
			FskTimeGetNow(&gKplScreen->lastFlipTime);	// Just trying to get close, since we just turned on the pump and can't rely on the last flip time
			FskTimeAddMS(&gKplScreen->lastFlipTime, FskTimeInMS(&gKplScreen->updateIntervalTime) >> 1);
			
			FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "START drawing pump **\n");
			
			// Kick off the drawing pump
			FskThreadPostCallback(FskThreadGetMain(), drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);
		}
	}
	else {
		if (gKplScreen->drawingPumpEnabled) {
			gKplScreen->drawingPumpEnabled = false;
			FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "STOP drawing pump **\n");
		}
	}
#endif

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
#if !SUPPORT_FLIP_THREAD
	UInt32 i, updateRate;
	
	// We set the actual interval based on the vsync interval
	updateRate = (UInt32)floor(((double)property->value.integer / gKplScreen->vSyncInterval + 0.5));

	if (!updateRate) updateRate = 1;
	gKplScreen->updateInterval = updateRate * gKplScreen->vSyncInterval;
	
	FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "Setting update interval to %ld ms", gKplScreen->updateInterval);
	
	FskTimeClear(&gKplScreen->updateIntervalTime);
	for (i = 0; i < updateRate; ++i)
		FskTimeAdd(&gKplScreen->vSyncIntervalTime, &gKplScreen->updateIntervalTime);
		
	FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "Setting update interval time to %ld.%06lds", gKplScreen->updateIntervalTime.seconds, gKplScreen->updateIntervalTime.useconds);
#endif
	return kFskErrNone;
}

FskErr KplScreenGetRetainsPixelsBetweenUpdates(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeBoolean;
	property->value.integer = false;
    return kFskErrNone;
}

FskErr KplScreenGetDisplayCopy(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    FskErr err = kFskErrNone;

    if (gKplScreen->touchDown) {
        property->value.bitmap = gKplScreen->touchDown;
        property->type = kFskMediaPropertyTypeBitmap;
        gKplScreen->touchDown = NULL;
    }
    else {
        void *bits;

        err = FskMemPtrNewFromData(gScreenWidth * gScreenHeight * 4, gKplScreen->baseAddr[ONSCREEN_PAGE(gKplScreen)], &bits);
        if (kFskErrNone == err) {
            FskBitmapNewWrapper(gScreenWidth, gScreenHeight, kFskBitmapFormat32BGRA, 32, bits, gScreenWidth * 4, (FskBitmap *)&property->value.bitmap);
            property->type = kFskMediaPropertyTypeBitmap;
        }
    }

    return err;
}


FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

	FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "** Locking surface **");

	while (2 == gKplScreen->bitmapsReady)
		FskDelay(1);

    FskMutexAcquire(gKplScreen->withCare);
//		gKplScreen->bitmap->baseAddress = gKplScreen->bitmaps[1 & gKplScreen->bitmapsFlagWrite]->bits;
		gKplScreen->bitmap->baseAddress = gKplScreen->bitmaps[0]->bits;
		gKplScreen->bitmap->rowBytes = gKplScreen->rowBytes;
		gKplScreen->bitmapsFlagWrite += 1;
    FskMutexRelease(gKplScreen->withCare);
	
	return kFskErrNone;
}

#if !SUPPORT_FLIP_THREAD

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	FskErr err = kFskErrNone;

	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

	FskInstrumentedTypePrintfVerbose(&gKplScreenTypeInstrumentation, "** Unlocking surface **");

	devFBFlip(gKplScreen);
	gKplScreen->bitmap->baseAddress = BITS_NULL;
bail:
	return err;
}

#else

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

    FskMutexAcquire(gKplScreen->withCare);
        gKplScreen->bitmapsReady += 1;
		gKplScreen->unlocked = true;
    FskMutexRelease(gKplScreen->withCare);

    FskSemaphoreRelease(gKplScreen->flipSemaphore);

	return kFskErrNone;
}

static void drawingPumpUpdate(FskWindow win)
{
	FskTimeRecord delta, now, nextFlipTime;
	
	FskTimeGetNow(&now);

	// Initialize unlock variable
	gKplScreen->unlocked = false;

	// Calculate how long it's been since we last updated the screen
	FskTimeCopy(&delta, &now);
	FskTimeSub(&gKplScreen->lastUpdateTime, &delta);

MLOG("drawingPumpUpdate\n");	
#if SUPPORT_INSTRUMENTATION
	if (0 == gKplScreen->lastUpdateTime.seconds && 0 == gKplScreen->lastUpdateTime.useconds) {
		FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "First draw after enabling drawing pump");
	}
	else {
		FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "It's been %ld.%06lds since the last update", delta.seconds, delta.useconds);
	}
#endif
	
    FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "** update window **");
    FskTimeCopy(&gKplScreen->lastUpdateTime, &now);

    // Estimate the next flip time by adding the vSync interval to the previous flip time.
    // If the previous drawing operation took longer than expected, i.e. longer than our update interval, we skipped one or more flips and need to adjust the next flip time accordingly.
    FskTimeCopy(&nextFlipTime, &gKplScreen->lastFlipTime);
    FskTimeAdd(&gKplScreen->vSyncIntervalTime, &nextFlipTime);
    while (FskTimeCompare(&nextFlipTime, &now) > 0)
        FskTimeAdd(&gKplScreen->vSyncIntervalTime, &nextFlipTime);
    
    FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "Next flip time %ld.%06ld", nextFlipTime.seconds, nextFlipTime.useconds);

#if 0
    if (gNumPts) {
        if (1 == gNumPts)
            motionMovedCB(NULL, NULL, NULL);
        else {
            FskPointAndTicksRecord defer = gPts[gNumPts - 1];
            gNumPts -= 1;
            motionMovedCB(NULL, NULL, NULL);
    		if (kFskErrNone == FskMemPtrNewFromData(sizeof(defer), &defer, &gPts))
                gNumPts = 1;
        }
    }

    FskWindowCheckEventQueue(win);      // flush pending mouse events
#endif
    FskWindowUpdate(win, &nextFlipTime);
	
	// We know that the client didn't draw if the screen bitmap didn't unlock during the FskWindowUpdate() call.
	// When this happens, force another Flip() to keep the drawing pump alive.
	if (!gKplScreen->unlocked) {
		FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "** client didn't draw, forcing a flip **\n");
        FskSemaphoreRelease(gKplScreen->flipSemaphore);
	}
}

static void drawingPumpCallback(void *arg0, void *arg1, void *arg2, void *arg3)
{	
	UInt32 callbackFiredCount = (UInt32)arg0;
	FskWindow win = FskWindowGetActive();

	gKplScreen->callbackFiredCount = callbackFiredCount;

	FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "In drawing pump callback %ld", callbackFiredCount);
	
	if (gKplScreen->drawingPumpEnabled && (NULL != win)) {
		drawingPumpUpdate(win);
	}
}

void flipThread(void *refcon)
{
	KplScreen screen = refcon;
	FskThread mainThread = FskThreadGetMain();

	FskThreadInitializationComplete(FskThreadGetCurrent());
	while (!gQuitting) {
        FskSemaphoreAcquire(screen->flipSemaphore);

		devFBFlip(screen);

		FskInstrumentedTypePrintfNormal(&gKplScreenTypeInstrumentation, "** after Flip **");

		//Save the last Flip time
		FskTimeGetNow(&(gKplScreen->lastFlipTime));

		gKplScreen->bitmap->baseAddress = BITS_NULL;
		
		// Kick off the next cycle
		if (gKplScreen->drawingPumpEnabled) {
			if  (gKplScreen->callbackPostedCount > gKplScreen->callbackFiredCount) {
				FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "We still have %lu events pending, Skip it", gKplScreen->callbackPostedCount - gKplScreen->callbackFiredCount);
				continue;
			}
			gKplScreen->callbackPostedCount++;
			FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "Before posting callback %ld", gKplScreen->callbackPostedCount);

MLOG("posting drawingPumpCallback\n");
			FskThreadPostCallback(mainThread, drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);

			FskInstrumentedTypePrintfDebug(&gKplScreenTypeInstrumentation, "After posting callback %ld", gKplScreen->callbackPostedCount);
		}
	}
}

#endif


