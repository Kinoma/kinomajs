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
#include "FskEnvironment.h"

#include "KplScreen.h"
#include "FskMemory.h"
#include "FskBitmap.h"
#include "FskFiles.h"
#include "FskTextConvert.h"
#include "KplUIEvents.h"
#include "KplFiles.h"

#include "FskFrameBuffer.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "FskEvent.h"
#include "FskWindow.h"
#include "KplUIEvents.h"

#include "FskThread.h"
#include "FskRectBlit.h"
#include "FskRotate90.h"
#include <math.h>
#include <linux/input.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#if SUPPORT_INSTRUMENTATION
FskInstrumentedTypeRecord gKplScreenK4TypeInstrumentation = {NULL, sizeof(FskInstrumentedTypeRecord), "kplscreenk4"};
#endif

// either 0 or 90
// #define FSK_ROTATE (90)

// KIDFLIX_ROTATE is defined by the makefile / linux.make
// so we can control the rotation for gPlugD and Linux 386 build
#define FSK_ROTATE KIDFLIX_ROTATE
	// MDK 140807 - /dev/fb support instead of DirectFB - rotate not tested

static char deviceName[] = "/dev/fb0";
#define FBIO_WAITFORVSYNC           _IOW('F', 0x20, __u32)

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
	int width;
	int height;
#if FSK_ROTATE
	FskBitmap scratchBits;				// what the application draws to
	KplBitmapRecord	scratchBitsKPR;		// KPR repreesentation of scratchBits
	FskBitmap wrapper;					// Fsk representation of screen
#endif
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
#define OFFSCREEN_PAGE(screen)	(screen->displayPage == 1 ? 0 : 1)	
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

    if (skipCopy) {
        ioctl(screen->fbfd, FBIO_WAITFORVSYNC, &dummy);
        return;
    }

    FskMutexAcquire(screen->withCare);
        FskMemMove(screen->baseAddr[OFFSCREEN_PAGE(screen)], screen->bitmaps[1 & screen->bitmapsFlagRead]->bits, screen->width * screen->height * 2);
        screen->bitmapsFlagRead += 1;
        screen->bitmapsReady -= 1;
    FskMutexRelease(screen->withCare);

	if (ioctl(screen->fbfd, FBIO_WAITFORVSYNC, &dummy) < 0) {
		fprintf(stderr, "	devFBFlip - problems with WAITFORVSYNC - %d\n", errno);
	}

	screen->displayPage = OFFSCREEN_PAGE(screen);
	
	var->xoffset = 0;
	var->yoffset = DISPLAY_OFFSET(screen);
	var->activate = FB_ACTIVATE_VBL;

    if (ioctl(screen->fbfd, FBIOPAN_DISPLAY, var) < 0) {
        fprintf(stderr, "	devFBFlip - problems with PAN - %d\n", errno);
    }
}

FskErr KplScreenDisposeBitmap(KplBitmap bitmap)
{
	FskErr err = kFskErrNone;
	KplScreen screen = gKplScreen;

#if FSK_ROTATE
	if (bitmap != &screen->scratchBitsKPR)
#else
	if (bitmap != screen->bitmap)
#endif
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

#if FSK_ROTATE
	FskBitmapDispose(screen->scratchBits);
	FskBitmapDispose(screen->wrapper);
#endif
#if SUPPORT_FLIP_THREAD
	FskSemaphoreDispose(screen->flipSemaphore);
#endif

	FskMemPtrDispose(screen);

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

		if (getenv("FRAMEBUFFER"))
			fbdev = getenv("FRAMEBUFFER");

		FskTimeRecord now;
		
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

			screen->width = screen->vinfo.xres;
			screen->height = screen->vinfo.yres;
			bpp = screen->vinfo.bits_per_pixel;
			screen->rowBytes = screen->vinfo.xres_virtual * bpp / 8;
			screen->screensize = screen->height * screen->rowBytes;

			screen->framebuffer = (char*)mmap((void*)screen->finfo.smem_start, screen->finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, screen->fbfd, 0);

			if (screen->framebuffer == (char*)-1) {
				fprintf(stderr,"Error: can't map framebuffer %d\n", errno);
				exit(4);
			}

			screen->displayPage = 1;
			screen->baseAddr[0] = screen->framebuffer;
			screen->baseAddr[1] = screen->framebuffer + screen->finfo.ypanstep * screen->finfo.line_length;

            FskBitmapNew(screen->width, screen->height, kFskBitmapFormat16RGB565LE, &screen->bitmaps[0]);
            FskBitmapNew(screen->width, screen->height, kFskBitmapFormat16RGB565LE, &screen->bitmaps[1]);
            FskBitmapWriteBegin(screen->bitmaps[0], NULL, NULL, NULL);
            FskBitmapWriteBegin(screen->bitmaps[1], NULL, NULL, NULL);
            
            FskMutexNew(&screen->withCare, "screen flipper");
		}
		else {
			fprintf(stderr, "Error: can't open framebuffer device - use ram\n");
			screen->width = 320;
			screen->height = 240;
			bpp = 16;
			screen->rowBytes = screen->width * bpp / 8;
			screen->screensize = screen->height * screen->rowBytes;
			FskMemPtrNew(screen->screensize, (FskMemPtr*)&screen->framebuffer);
		}
		devFBFlip(screen);
		devFBFlip(screen);
		FskTimeGetNow(&now);
		devFBFlip(screen);
        FskTimeGetNow(&screen->vSyncIntervalTime);
        FskTimeSub(&now, &screen->vSyncIntervalTime);
        screen->vSyncInterval = FskTimeInMS(&screen->vSyncIntervalTime);
        if (!screen->vSyncInterval) screen->vSyncInterval = 1;

		FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "VsyncInterval = %d", screen->vSyncInterval);

		screen->bitmap->baseAddress = screen->baseAddr[screen->displayPage];
		screen->bitmap->rowBytes = screen->rowBytes;
		screen->bitmap->pixelFormat = kFskBitmapFormat16RGB565LE;
		screen->bitmap->depth = 16;
		screen->bitmap->width = screen->width;
		screen->bitmap->height = screen->height;
#if FSK_ROTATE
		FskBitmapNew(screen->height, screen->width, screen->bitmap->pixelFormat, &screen->scratchBits);
		screen->scratchBitsKPR.depth = screen->bitmap->depth;
		screen->scratchBitsKPR.pixelFormat = screen->bitmap->pixelFormat;
		screen->scratchBitsKPR.width = screen->bitmap->height;
		screen->scratchBitsKPR.height = screen->bitmap->width;
		FskBitmapNewWrapper(screen->bitmap->width, screen->bitmap->height, screen->bitmap->pixelFormat, screen->bitmap->depth, NULL, 0, &screen->wrapper);
#endif
#if SUPPORT_FLIP_THREAD
		FskSemaphoreNew(&screen->flipSemaphore, 0);
		FskThreadCreate(&screen->flipThread, flipThread, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit | kFskThreadFlagsHighPriority, screen, "dfb flip");
#endif
		BAIL_IF_ERR(err = FskFrameBufferGetVectors(&vectors));
		vectors->doHasProperty = KplScreenHasProperty;
		vectors->doSetProperty = KplScreenSetProperty;
		vectors->doGetProperty = KplScreenGetProperty;
		gKplScreen = screen;
	}
#if FSK_ROTATE
	*bitmap = &gKplScreen->scratchBitsKPR;
#else
	*bitmap = gKplScreen->bitmap;
#endif
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
			
			FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "START drawing pump **\n");
			
			// Kick off the drawing pump
			FskThreadPostCallback(FskThreadGetMain(), drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);
		}
	}
	else {
		if (gKplScreen->drawingPumpEnabled) {
			gKplScreen->drawingPumpEnabled = false;
			FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "STOP drawing pump **\n");
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
	
	FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "Setting update interval to %ld ms", gKplScreen->updateInterval);
	
	FskTimeClear(&gKplScreen->updateIntervalTime);
	for (i = 0; i < updateRate; ++i)
		FskTimeAdd(&gKplScreen->vSyncIntervalTime, &gKplScreen->updateIntervalTime);
		
	FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "Setting update interval time to %ld.%06lds", gKplScreen->updateIntervalTime.seconds, gKplScreen->updateIntervalTime.useconds);
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

        err = FskMemPtrNewFromData(gKplScreen->width * gKplScreen->height * 2, gKplScreen->baseAddr[ONSCREEN_PAGE(gKplScreen)], &bits);
        if (kFskErrNone == err) {
            FskBitmapNewWrapper(gKplScreen->width, gKplScreen->height, kFskBitmapFormat16RGB565LE, 16, bits, gKplScreen->width * 2, (FskBitmapRecord**)&property->value.bitmap);
            property->type = kFskMediaPropertyTypeBitmap;
        }
    }

    return err;
}


#if FSK_ROTATE

static FskErr fbScreenLockBitmap(KplBitmap bitmap);
static FskErr fbScreenUnlockBitmap(KplBitmap bitmap);

FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	void *bits;
	SInt32 rowBytes;

	FskBitmapWriteBegin(gKplScreen->scratchBits, &bits, &rowBytes, NULL);
	gKplScreen->scratchBitsKPR.baseAddress = bits;
	gKplScreen->scratchBitsKPR.rowBytes = rowBytes;

	return kFskErrNone;
}

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	FskScaleOffset so;
	FskErr err = kFskErrNone;

	err = fbScreenLockBitmap(gKplScreen->bitmap);
	if (err) return err;

	gKplScreen->wrapper->bits = gKplScreen->bitmap->baseAddress;
	gKplScreen->wrapper->rowBytes = gKplScreen->bitmap->rowBytes;

	// so.scaleX = -1 << kFskScaleBits;
	// so.scaleY = +1 << kFskScaleBits;
	// so.offsetX = (gKplScreen->width - 1 )  << kFskOffsetBits;
	// so.offsetY = 0 << kFskOffsetBits;

	so.scaleX = +1 << kFskScaleBits;
	so.scaleY = -1 << kFskScaleBits;
	so.offsetX = 0 << kFskOffsetBits;
	so.offsetY = (gKplScreen->height - 1 )  << kFskOffsetBits;

	FskRotate90(gKplScreen->scratchBits, NULL, gKplScreen->wrapper, NULL, &so, 0);
	//@@ FskRotate90 doesn't lock/unlock bits

	fbScreenUnlockBitmap(gKplScreen->bitmap);
	gKplScreen->unlocked = true;

	FskBitmapWriteEnd(gKplScreen->scratchBits);
	gKplScreen->scratchBitsKPR.baseAddress = BITS_NULL;

	return kFskErrNone;
}

#define KplScreenLockBitmap fbScreenLockBitmap
#define KplScreenUnlockBitmap fbScreenUnlockBitmap

#endif

FskErr KplScreenLockBitmap(KplBitmap bitmap)
{
	FskErr err = kFskErrNone;

	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

	FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "** Locking surface **");

    if (2 == gKplScreen->bitmapsReady) {
        while (2 == gKplScreen->bitmapsReady)
            FskDelay(1);
    }

	gKplScreen->bitmap->baseAddress = gKplScreen->bitmaps[1 & gKplScreen->bitmapsFlagWrite]->bits;
	gKplScreen->bitmap->rowBytes = gKplScreen->rowBytes;
    gKplScreen->bitmapsFlagWrite += 1;
	
	return err;
}

#if !SUPPORT_FLIP_THREAD

FskErr KplScreenUnlockBitmap(KplBitmap bitmap)
{
	FskErr err = kFskErrNone;

	if (bitmap != gKplScreen->bitmap)
		return kFskErrOperationFailed;

	FskInstrumentedTypePrintfVerbose(&gKplScreenK4TypeInstrumentation, "** Unlocking surface **");

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
    FskMutexRelease(gKplScreen->withCare);

    FskSemaphoreRelease(gKplScreen->flipSemaphore);
	gKplScreen->unlocked = true;

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
	
#if SUPPORT_INSTRUMENTATION
	if (0 == gKplScreen->lastUpdateTime.seconds && 0 == gKplScreen->lastUpdateTime.useconds) {
		FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "First draw after enabling drawing pump");
	}
	else {
		FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "It's been %ld.%06lds since the last update", delta.seconds, delta.useconds);
	}
#endif
	
    FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "** update window **");
    FskTimeCopy(&gKplScreen->lastUpdateTime, &now);

    // Estimate the next flip time by adding the vSync interval to the previous flip time.
    // If the previous drawing operation took longer than expected, i.e. longer than our update interval, we skipped one or more flips and need to adjust the next flip time accordingly.
    FskTimeCopy(&nextFlipTime, &gKplScreen->lastFlipTime);
    FskTimeAdd(&gKplScreen->vSyncIntervalTime, &nextFlipTime);
    while (FskTimeCompare(&nextFlipTime, &now) > 0)
        FskTimeAdd(&gKplScreen->vSyncIntervalTime, &nextFlipTime);
    
    FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "Next flip time %ld.%06ld", nextFlipTime.seconds, nextFlipTime.useconds);

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
    FskWindowUpdate(win, &nextFlipTime);
	
	// We know that the client didn't draw if the screen bitmap didn't unlock during the FskWindowUpdate() call.
	// When this happens, force another Flip() to keep the drawing pump alive.
	if (!gKplScreen->unlocked) {
		FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "** client didn't draw, forcing a flip **\n");
        FskSemaphoreRelease(gKplScreen->flipSemaphore);
	}
}

static void drawingPumpCallback(void *arg0, void *arg1, void *arg2, void *arg3)
{	
	UInt32 callbackFiredCount = (UInt32)arg0;
	FskWindow win = FskWindowGetActive();

	gKplScreen->callbackFiredCount = callbackFiredCount;

	FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "In drawing pump callback %ld", callbackFiredCount);
	
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

		FskInstrumentedTypePrintfNormal(&gKplScreenK4TypeInstrumentation, "** after Flip **");

		//Save the last Flip time
		FskTimeGetNow(&(gKplScreen->lastFlipTime));

		gKplScreen->bitmap->baseAddress = BITS_NULL;
		
		// Kick off the next cycle
		if (gKplScreen->drawingPumpEnabled) {
			if  (gKplScreen->callbackPostedCount > gKplScreen->callbackFiredCount) {
				FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "We still have %lu events pending, Skip it", gKplScreen->callbackPostedCount - gKplScreen->callbackFiredCount);
				continue;
			}
			gKplScreen->callbackPostedCount++;
			FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "Before posting callback %ld", gKplScreen->callbackPostedCount);

			FskThreadPostCallback(mainThread, drawingPumpCallback, (void*)gKplScreen->callbackPostedCount, NULL, NULL, NULL);

			FskInstrumentedTypePrintfDebug(&gKplScreenK4TypeInstrumentation, "After posting callback %ld", gKplScreen->callbackPostedCount);
		}
	}
}

#endif


#if LINUX_PLATFORM

/*********************************************************** 
 ********************  For Multi-Touch  ********************
 ***********************************************************/
FskInstrumentedSimpleType(KplScreen, kplscreen);

char *kInputDevicePath = "/dev/input/";

KplDirectoryChangeNotifier devInputChangeNotifier = NULL;

typedef struct inputDeviceRecord {
	struct inputDeviceRecord *next;
	char 					*fullPath;
	char 					*node;
	int						fd;
	FskThreadDataSource 	dataSource;
	FskThreadDataHandler	dataHandler; 
} inputDeviceRecord, *inputDevice;

inputDevice inputDevices = NULL;

static void inputDataHandler(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
void DisposeInputDevice(inputDevice dev);

#define k4PowerButton 68

UInt16 keyState[256];
UInt16 devInputMapToUTF8[] = {
    0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,     // 00
    0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x08, 0x09,     // 08
    0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69,     // 16
    0x6F, 0x70, 0x7B, 0x7D, 0x0D, 0x00, 0x61, 0x73,     // 24
    0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3B,     // 32
    0x27, 0x60, 0x00, 0x5C, 0x7A, 0x78, 0x63, 0x76,     // 40
    0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F, 0x00, 0x2A,     // 48
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 56
    0x00, 0x00, 0x00, 0x00, k4PowerButton, 0x00, 0x00, 0x37,     // 64
    0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,     // 72
    0x32, 0x33, 0x30, 0x2E, 0x00, 0x00, 0x00, 0x00,     // 80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 88
    0x0A, 0x00, 0x2F, 0x00, 0x00, 0x0A, 0x00, 0x1E,     // 96
    0x00, 0x1C, 0x1D, 0x00, 0x1F, 0x00, 0x00, 0x7F,     // 104
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, 0xF1, 0x00,     // 112
    0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 120

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 128
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 136
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 144
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,     // 152
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 160
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 168
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 176
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 184
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 192
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 200
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 208
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 216
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 224
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 232
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 240
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // 248
};

UInt16 devInputMapShiftToUTF8[] = {
    0x00, 0x1B, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E,
    0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x08, 0x09,
    0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49,
    0x4F, 0x50, 0x7B, 0x7D, 0x0D, 0x00, 0x41, 0x53,
    0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3A,
    0x22, 0x7E, 0x00, 0x7C, 0x5A, 0x58, 0x43, 0x56,
    0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F, 0x00, 0x2A,
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37,
    0x38, 0x39, 0x2D, 0x34, 0x35, 0x36, 0x2B, 0x31,
    0x32, 0x33, 0x30, 0x2E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x00, 0x2F, 0x00, 0x00, 0x0A, 0x00, 0x1E,
    0x00, 0x1C, 0x1D, 0x00, 0x1F, 0x00, 0x00, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, 0xF1, 0x00,
    0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

UInt16 mapKeyCode(struct input_event *event, int mod) {
    UInt16 ret;
    if (mod == kFskEventModifierShift)
        ret = devInputMapShiftToUTF8[event->code];
    else
        ret = devInputMapToUTF8[event->code];

    return ret;
}



inputDevice devInputAddDevice(const char *dev) {
	inputDevice newInput = NULL;
	FskErr err;

	MLOG("adding input device : %s\n", dev);
	err = FskMemPtrNewClear(sizeof(inputDeviceRecord), &newInput);
	if (err) {
		fprintf(stderr, "memory err on new input device\n");
		goto newFailed;
	}

	newInput->node = FskStrDoCopy(dev);
	newInput->fullPath = FskStrDoCat(kInputDevicePath, dev);
	newInput->fd = open(newInput->fullPath, O_RDONLY | O_NONBLOCK);
	if (newInput->fd < 0) {
		fprintf(stderr, "opening %s failed %d\n", newInput->fullPath, errno);
		goto newFailed;
	}
	newInput->dataSource = FskThreadCreateDataSource(newInput->fd);
	FskThreadAddDataHandler(&newInput->dataHandler, newInput->dataSource, inputDataHandler, true, false, newInput);
	FskListAppend(&inputDevices, newInput);

	MLOG("added input device : %x - %s\n", newInput, dev);
	return newInput;

newFailed:

	DisposeInputDevice(newInput);
	return NULL;
}

void devInputRemoveDevice(const char *dev) {
	inputDevice scan = inputDevices;

	MLOG("remove InputDevice - %s\n", dev);
	while (scan) {
		if (0 == FskStrCompare(scan->node, dev)) {
			MLOG("found a device to remove: %s\n", dev);
			FskListRemove(&inputDevices, scan);
			DisposeInputDevice(scan);
			return;
		}
		scan = scan->next;
	}
}


FskErr devInputNotifierCallback(UInt32 whatChanged, const char *path, void *refCon) {
	char *str;

	str = FskStrDoCat(kInputDevicePath, path);

	MLOG("devInputNotifierCallback - what: %d, path: %s, ref: %x\n", whatChanged, path, refCon);
	switch (whatChanged) {
		case kKplDirectoryChangeFileChanged:
			MLOG("file %s changed\n", path);
			devInputRemoveDevice(path);
			devInputAddDevice(path);
			break;
		case kKplDirectoryChangeFileCreated:
			{
				MLOG("file %s created\n", path);
				devInputAddDevice(path);
			}
			break;
		case kKplDirectoryChangeFileDeleted:
			MLOG("file %s deleted\n", path);
			devInputRemoveDevice(path);
			break;
	}

	FskMemPtrDispose(str);
	return kFskErrNone;
}

void watchDevInput(char *dir) {
	FskErr err;
	MLOG("Going to watch directory %s\n", dir);
	err = KplDirectoryChangeNotifierNew(dir, kKplDirectoryChangeFileChanged | kKplDirectoryChangeFileCreated | kKplDirectoryChangeFileDeleted, devInputNotifierCallback, (void*)1, &devInputChangeNotifier);
}

void unWatchDevInput() {
	KplDirectoryChangeNotifierDispose(devInputChangeNotifier);
	devInputChangeNotifier = NULL;
}


void DisposeInputDevice(inputDevice dev) {
	if (dev) {
		MLOG("DisposeInputDevice - %x\n", dev);
		if (dev->dataHandler) {
			FskThreadRemoveDataHandler(&dev->dataHandler);
			FskMemPtrDispose(dev->dataSource);
			FskMemPtrDispose(dev->fullPath);
			FskMemPtrDispose(dev->node);
		}
		if (dev->fd > -1)
			close(dev->fd);
		FskMemPtrDispose(dev);
	}
}

void ScanDevInput(char *dir) {
	inputDevice inputIterator = NULL;
	FskDirectoryIterator dirIt = NULL;
	FskErr err = kFskErrNone;
	char *dirEntry, fullPath[PATH_MAX];
	UInt32 entryType;
	Boolean gotOne = false;

	MLOG("scanDevInput %s\n", dir);
	BAIL_IF_ERR(err = FskDirectoryIteratorNew(dir, &dirIt, 0));

	while (kFskErrNone == FskDirectoryIteratorGetNext(dirIt, &dirEntry, &entryType)) {
		if (kFskDirectoryItemIsDirectory == entryType) {
			MLOG("-- got a subdirectory. Do we care? Skip.\n");
		}
		else if (kFskDirectoryItemIsFile == entryType) {
			FskStrCopy(fullPath, dir);
			FskStrCat(fullPath, dirEntry);
			inputIterator = inputDevices;
			gotOne = false;
			while (inputIterator) {
				if (0 == FskStrCompare(inputIterator->fullPath, fullPath))
					gotOne = true;
				inputIterator = inputIterator->next;
			}

			if (gotOne)
				continue;

			devInputAddDevice(dirEntry);
		}
	}

bail:
	FskDirectoryIteratorDispose(dirIt);
}

void CloseDevInputs() {

	MLOG("CloseDevInputs\n");
	while (inputDevices) {
        inputDevice device = inputDevices;
    
        FskListRemove(&inputDevices, device);
		DisposeInputDevice(device);
	}

}


#define kFskMotionMovedGateMS	30
#define TOUCHPOINTS_MAXNUM  5
int glastX[TOUCHPOINTS_MAXNUM];
int glastY[TOUCHPOINTS_MAXNUM];
int glastDown[TOUCHPOINTS_MAXNUM];
FskTimeCallBack touchMovedTimer = NULL;
FskFixed touchScale = 0;

FskPointRecord touchPoints[TOUCHPOINTS_MAXNUM];
static int sentMouseDown[TOUCHPOINTS_MAXNUM] = {-1,-1,-1,-1,-1};
static int currentPointer = -1, lastPointer = -1;

void trackMouseDown(int pointer) {
	glastDown[pointer]++;
    // fprintf(stderr, "trackMouseDown, [%d] = %d\n", pointer, glastDown[pointer]);
}

Boolean anyMouseDown() {
	Boolean ret = false;
	int i;
	for (i=0; i<TOUCHPOINTS_MAXNUM; i++)
		if (glastDown[i] > 0)
			ret = true;

	return ret;
}

int trackMouseUp(int pointer) {
    // glastDown[pointer]--;
    // if (glastDown[pointer] < 0) {
    //     glastDown[pointer] = 0;
    //     fprintf(stderr, "trackMouseUp went negative - let caller know\n");
    //     return 0;
    // }
    glastDown[pointer] = 0;
    // fprintf(stderr, "trackMouseUp, [%d] = %d\n", pointer, glastDown[pointer]);
	return 1;
}

FskErr evtQueue(FskWindow window, FskEvent event) {
	FskErr err = kFskErrNone;

    //fprintf(stderr, "evtQueue %d\n", event->eventCode);
	if (event->eventCode == kFskEventMouseMoved) {
		FskListElement l;
		FskEvent lastMoved = NULL;
		FskListMutexAcquireMutex(window->eventQueue);
		l = FskListGetNext(window->eventQueue->list, NULL);
		while (l) {
			FskEvent ev = (FskEvent)l;
			if (ev->eventCode == kFskEventMouseMoved) {
				lastMoved = ev;
			}
			else if ((ev->eventCode == kFskEventMouseUp)
					|| (ev->eventCode == kFskEventMouseDown)) {
				lastMoved = NULL;	// was going to add mouseMoved, but then got a mouseUp in the list
			}
			l = FskListGetNext(window->eventQueue->list, l);
		}

		if (lastMoved) {
			char *paramData, *p;
			UInt32 oldSize, addSize, newSize;

            FskKplScreenPrintfDebug("the last event type (%d) in the queue is getting this event added\n", event->eventCode);
			FskEventParameterGetSize(lastMoved, kFskEventParameterMouseLocation, &oldSize);
			FskEventParameterGetSize(event, kFskEventParameterMouseLocation, &addSize);
			newSize = oldSize + addSize;
			if (kFskErrNone != (err = FskMemPtrNew(newSize, &paramData)))
				goto bail;
			if (kFskErrNone != (err = FskEventParameterGet(lastMoved, kFskEventParameterMouseLocation, paramData)))
				goto bail;
			FskEventParameterRemove(lastMoved, kFskEventParameterMouseLocation);
			p = paramData + oldSize;
			err = FskEventParameterGet(event, kFskEventParameterMouseLocation, p);
			FskEventParameterAdd(lastMoved, kFskEventParameterMouseLocation, newSize, paramData);
			FskMemPtrDispose(paramData);

			FskEventDispose(event);
			FskListMutexReleaseMutex(window->eventQueue);
			goto done;	// the event was merged with the "lastMoved" event

bail:
			// we weren't able to merge
			if (paramData)
				FskMemPtrDispose(paramData);
            paramData = NULL;
		}
		FskListMutexReleaseMutex(window->eventQueue);
	}
//	if (FskListMutexIsEmpty(window->eventQueue)) {
//fprintf(stderr, "-- sent event to window\n");
//		err = FskWindowEventSend(window, event);
//	}
//	else {
        FskKplScreenPrintfDebug("-- queued event to window\n");
		err = FskWindowEventQueue(window, event);
//		FskThreadYield();
//	}
done:
	return err;
}

void motionMovedCB(struct FskTimeCallBackRecord *callback, const FskTime time, void *param) {
	FskEvent ev = NULL;
    // FskErr err = kFskErrNone;
	FskWindow win = FskWindowGetActive();
	FskTimeRecord mouseTimeR = {0, 0};
	int index = 0;

	if (gNumPts > 0) {
		FskTimeAddMS(&mouseTimeR, gPts[gNumPts - 1].ticks);
        // fprintf(stderr, "touchMovedTimer - blasting %d points @ time[%d,%d]\n",gNumPts, mouseTimeR.seconds, mouseTimeR.useconds);

		FskEventNew(&ev, kFskEventMouseMoved, &mouseTimeR, kFskEventModifierNone);
		FskEventParameterAdd(ev, kFskEventParameterMouseLocation, gNumPts * sizeof(FskPointAndTicksRecord), gPts);

		index = 0;
		FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(index), &index);
        FskWindowEventQueue(win, ev);
		FskMemPtrDisposeAt(&gPts);
		gNumPts = 0;
	}

	if (anyMouseDown()) {
        //fprintf(stderr, "win->updateInterval = %d\n", win->updateInterval);
        FskTimeCallbackScheduleFuture(touchMovedTimer, 0, 100 /* win->updateInterval */, motionMovedCB, win);
    }
	else {
        // fprintf(stderr, "no mousedown - stop motionMovedCB\n");
		FskWindowCancelStillDownEvents(win);
		FskTimeCallbackRemove(touchMovedTimer);
        // fprintf(stderr, "touchMovedTimer removed\n");
		touchMovedTimer = NULL;
	}
}

FskErr addAMoved(FskWindow win, int x, int y, UInt32 ms, int pointer) {
	FskErr err;
    int idx = 0;

    // fprintf(stderr, "addAMoved - x:%d y:%d ms:%d ptr: %d\n", x, y, ms, pointer);
    for(idx = 0; idx < TOUCHPOINTS_MAXNUM; idx++) {
    	if (glastX[idx] == touchPoints[idx].x && glastY[idx] == touchPoints[idx].y) {
    		continue;
    	}
    
    	glastX[pointer] = touchPoints[idx].x;
    	glastY[pointer] = touchPoints[idx].y;
    	if (gPts) {
    		gNumPts += 1;
    		err = FskMemPtrRealloc(gNumPts * sizeof(FskPointAndTicksRecord), &gPts);
    		if (err) {
                gNumPts --;
                goto bail;
            }
    	}
    	else {
    		gNumPts = 1;
    		err = FskMemPtrNew(sizeof(FskPointAndTicksRecord), &gPts);
    		if (err) {
                gNumPts = 0;
                goto bail;
            }
    	}

    	gPts[gNumPts-1].ticks = ms;
    	gPts[gNumPts-1].pt.x = touchPoints[idx].x;
    	gPts[gNumPts-1].pt.y = touchPoints[idx].y;
    	gPts[gNumPts-1].index = idx;
    }

bail:
	if (gNumPts >= 18) {  //small number for better response
		motionMovedCB(NULL, NULL, NULL);
	}

	return err;
}

static void inputHandler(struct input_event *event, inputDevice dev)
{
	FskWindow win = FskWindowGetActive();
	FskErr err = kFskErrNone;
	FskEventCodeEnum cod = kFskEventNone;
	FskEventModifierEnum mod = kFskEventModifierNone;
	UInt32 clicks = 0;
	FskTimeRecord mouseTimeR = {0, 0};

    UInt32      val; // param;
	FskEvent ev = NULL;
	FskPointAndTicksRecord pt;

	if (NULL == win)
		return;

	MLOG("inputEvent from (%x) %s\n", dev, dev->fullPath);

    val = event->value;
    FskKplScreenPrintfDebug("type: %d code: %d val: %d", event->type, event->code, event->value);  
    MLOG("type:  -- %d code: %d val: %d\n", event->type, event->code, event->value);  

    switch (event->type) {
		case EV_KEY : {
				if (event->code > 0xff)		// ignore keypresses out of range
//fprintf(stderr, "Keycode out of range (%x). ignore\n", event->code);
					return;

				if (val == 1) {
					cod = kFskEventKeyDown;
					MLOG(" -- KEY DOWN\n");
				}
				else if (val == 0) {
					MLOG(" -- KEY UP\n");
					cod = kFskEventKeyUp;
				}
				else {
                    cod = kFskEventNone;
					fprintf(stderr, "unknown key value %d (keycode %d)\n", event->value, event->code);
				}
				
			}
			break;
        case EV_ABS :
        switch (event->code) {
                //use ABS_MT_TOUCH_MAJOR replace ABS_MT_SLOT since ABS_MT_SLOT is not supported in kervel 2.6
                case ABS_MT_TOUCH_MAJOR:
                    currentPointer = val;
                    cod = kFskEventNone;
                    break;
				case ABS_MT_TRACKING_ID:
					FskKplScreenPrintfDebug("code: ABS_MT_TRACKING_ID: - touch %d", currentPointer);
					if (val != -1) {
//						FskKplScreenPrintfDebug("   val = %d send MouseDown", val);
                        // cod = kFskEventMouseDown;
                        cod = kFskEventNone;
                        sentMouseDown[currentPointer] = 0;
					}
					else {
                        lastPointer = currentPointer;
						FskKplScreenPrintfDebug("   val = -1 send MouseUp for last pointer %d", lastPointer);
                        
						cod = kFskEventMouseUp;
						motionMovedCB(NULL, NULL, NULL);	// send pending
			 			trackMouseUp(lastPointer);
					}
					break;
                case ABS_MT_POSITION_X:
                case ABS_X:
					FskKplScreenPrintfDebug("code: ABS_X - touch[%d].x = %d", currentPointer, val);
					touchPoints[currentPointer].x = val;
                    break;
                case ABS_MT_POSITION_Y:
                case ABS_Y:
					FskKplScreenPrintfDebug("code: ABS_Y - touch[%d].y = %d", currentPointer, val);
					touchPoints[currentPointer].y = val;
                    break;
            }
            break;
		case 0:
			if (event->code == 0 && event->value == 0) {
				if (!sentMouseDown[currentPointer]) {
					FskKplScreenPrintfDebug("type, code and value all 0 - didn't send Down yet - push MouseDown\n");
					sentMouseDown[currentPointer] = -1;		// send mousedown after we've received a location
					cod = kFskEventMouseDown;		// too early? No x/y yet?
				}
				else if (currentPointer == -1)
					cod = kFskEventNone;
				else {
					FskKplScreenPrintfDebug("type, code and value all 0 - push MouseMoved");
					cod = kFskEventMouseMoved;
				}
			}
        default:
            break;
    }

    if (cod) {
		UInt32 ticks;
		FskTimeGetNow(&mouseTimeR);
        
#if !USE_POSIX_CLOCK
        //prevent msec overflow for SInt32
        if (mouseTimeR.seconds > 1403000000)
            mouseTimeR.seconds -= 1403000000;
#endif

		ticks = FskTimeInMS(&mouseTimeR);
		if (cod == kFskEventMouseMoved) {
			if (!sentMouseDown[currentPointer]) {
                FskKplScreenPrintfDebug(" -- mouseMoved, no eventdown sent yet. Do it. %d: %d %d\n", 
                                currentPointer, touchPoints[currentPointer].x, touchPoints[currentPointer].y);
				cod = kFskEventMouseDown;		// need to set the point
                sentMouseDown[currentPointer] = -1;
			}
			else
            {
				addAMoved(win, touchPoints[currentPointer].x, touchPoints[currentPointer].y, ticks, currentPointer);
			    goto bail;
            }
		}

		if (kFskErrNone != (err = FskEventNew(&ev, cod, &mouseTimeR, mod))) {
			return;
		}

		pt.ticks = ticks;
		
		if (cod == kFskEventMouseUp) {
			pt.pt.x = glastX[lastPointer];
			pt.pt.y = glastY[lastPointer];
			pt.index = lastPointer;

			glastX[lastPointer] = touchPoints[lastPointer].x;
			glastY[lastPointer] = touchPoints[lastPointer].y;
		}
		else {
			pt.pt.x = touchPoints[currentPointer].x;
			pt.pt.y = touchPoints[currentPointer].y;
			pt.index = currentPointer;

			glastX[currentPointer] = touchPoints[currentPointer].x;
			glastY[currentPointer] = touchPoints[currentPointer].y;
		}


		if (cod == kFskEventMouseDown) {
            FskBitmap replace;

			mod = kFskEventModifierMouseButton;
			clicks = 1;
			if (!touchMovedTimer) {
                FskKplScreenPrintfDebug("touchMovedTimer installed - updateInterval %d\n", win->updateInterval);
				FskTimeCallbackNew(&touchMovedTimer);
				FskTimeCallbackScheduleFuture(touchMovedTimer, 0, 100 /* win->updateInterval */, motionMovedCB, win);
			}
			trackMouseDown(currentPointer);
			// touchScale = FskPortScaleGet(win->port);

            /*
                cache current screen for potential screen capture
            */
            if (gKplScreen->touchDown) {
                FskBitmapDispose(gKplScreen->touchDown);
                gKplScreen->touchDown = NULL;
            }

            if (kFskErrNone == FskBitmapNew(gKplScreen->width, gKplScreen->height, kFskBitmapFormat16RGB565LE, &replace)) {
                int page;
                
                FskBitmapWriteBegin(replace, NULL, NULL, NULL);

                FskMutexAcquire(gKplScreen->withCare);
                    page = 1 & (gKplScreen->bitmapsFlagRead - 1);

                    gKplScreen->touchDown = gKplScreen->bitmaps[page];
                    gKplScreen->bitmaps[page] = replace;
                FskMutexRelease(gKplScreen->withCare);

                FskBitmapWriteEnd(gKplScreen->touchDown);
            }
		}

		// scale
		// v = x << 16;
		// pt.pt.x = FskFixDiv(v, touchScale) >> 16;
		// v = y << 16;
		// pt.pt.y = FskFixDiv(v, touchScale) >> 16;

		FskEventParameterAdd(ev, kFskEventParameterMouseLocation, sizeof(pt), &pt);
		if (clicks) {
			FskEventParameterAdd(ev, kFskEventParameterMouseClickNumber, sizeof(clicks), &clicks);
		}
		if (cod == kFskEventMouseUp) {
			FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(lastPointer), &lastPointer);
			FskKplScreenPrintfDebug("doing FskMotionTouch kFskEventMouseUp event %d for pointer %d - x:%d y:%d\n", cod, lastPointer, pt.pt.x, pt.pt.y);
		}
		else {
			FskEventParameterAdd(ev, kFskEventParameterCommand, sizeof(currentPointer), &currentPointer);
			FskKplScreenPrintfDebug("doing FskMotionTouch (not up) event %d for pointer %d - x:%d y:%d\n", cod, currentPointer, pt.pt.x, pt.pt.y);
		}

		if (cod == kFskEventKeyDown || cod ==  kFskEventKeyUp) {
            unsigned char utf8[12];
            UInt32 param = mapKeyCode(event, mod);
            if (k4PowerButton == param) {
                UInt32 functionKey = kFskEventFunctionKeyPower;
                MLOG("posting function key event char param: 0x%02x - mod 0x%02x\n", kFskEventFunctionKeyPower, mod);
                FskEventParameterAdd(ev, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
            }
            else {
                FskTextCharCodeToUTF8(param, utf8);
                MLOG("posting key event char param: 0x%02x - mod 0x%02x\n", param, mod);
                FskEventParameterAdd(ev, kFskEventParameterKeyUTF8, FskStrLen((const char *)utf8) + 1, utf8);
            }
		}

		evtQueue(win, ev);
    }

bail:
	return;
}

static void inputDataHandler(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
    struct  input_event event;
    int     len;
	
	MLOG(" inputDAtAHandler - %x\n", refCon);
	while (0 < (len = read(FskThreadGetDataHandlerDataNode(handler), &event, sizeof(event)))) {
	    while (len > 0) {
			// if (event.type != 0 && event.type != 4 && event.type != 1)
                if (len < sizeof(event)) {
//                    fprintf(stderr, "input: read(%d) less than required (%d)\n", len, sizeof(event));
                    return;
                }
			inputHandler(&event, refCon);
			len -= sizeof(event);
		}
	}
	MLOG("DONE reading events\n\n");
}

void initializeLinuxinput() {
	ScanDevInput(kInputDevicePath);
	watchDevInput(kInputDevicePath);
}

void terminateLinuxinput() {
	CloseDevInputs();
	unWatchDevInput();
}
#endif

