/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#define __FSKTHREAD_PRIV__

#define __FSKWINDOW_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <math.h>
#include <stdio.h>

#include <jni.h>

#include "Fsk.h"
#include "FskFrameBuffer.h"
#include "FskExtensions.h"
#include "FskFrameBuffer.h"
#include "FskHardware.h"

#if (ANDROID_VERSION == 3)
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#elif (ANDROID_VERSION == 4)
#include <android/native_window.h>
#include <android/native_window_jni.h>

#else
#include <ui/Surface.h>
#include <ui/ISurfaceComposer.h>
#endif

FrameBufferVectors	vectors;

FskFBGlobalsRecord	gFBGlobals;

Boolean gDrawingPumpEnabled = false;
UInt32 gUpdateRate = 0;


extern "C" {
	FskErr lockFskSurface(FskBitmap bitmap, void **baseaddr, int *rowBytes);
	FskErr lockFskSurfaceArea(FskBitmap bitmap, FskRectangleRecord *r, void **baseaddr, int *rowBytes);
	FskErr unlockFskSurface(FskBitmap bitmap);
	FskErr getFskScreenBitmap(FskBitmap *bitmap);
	FskErr getFskSurfaceBounds(FskRectangleRecord *bounds);
	int setupAndroidFramebuffer();
	int terminateAndroidFramebuffer();

	FskErr androidStartFramebufferReadable(FskBitmap *bitmapOut);
	FskErr androidEndFramebufferReadable(FskBitmap bitmap);

	FskErr doGrabScreenForDrawing(void);
	FskErr doReleaseScreenForDrawing(void);

	FskInstrumentedSimpleType(AndroidFrameBuffer, androidframebuffer);
	FskInstrumentedSimpleType(AndroidSurface, androidsurface);
}


FskErr androidStartFramebufferReadable(FskBitmap *bitmapOut) {
	FskRectangleRecord r = {0, 0, 1, 1};

	FskAndroidFrameBufferPrintfDebug("XXXXX -- androidStartFramebufferReadable - MidSizeChange %d backBufferLocked %d -- surfaceLocked %d", gFBGlobals.midSizeChange, gFBGlobals.backBufferLocked, gFBGlobals.surfaceLocked);

	if (gFBGlobals.tempBuffer) {
		FskAndroidFrameBufferPrintfDebug("androidStartFramebufferReadable - return gFBGlobals.tempBuffer %x", gFBGlobals.tempBuffer);
		lockFskSurfaceArea(gFBGlobals.tempBuffer, &r, NULL, NULL);
		*bitmapOut = gFBGlobals.tempBuffer;
	}
	else {
		FskAndroidFrameBufferPrintfDebug("androidStartFramebufferReadable - cycle native buffers");
		// hack - cycle through buffers by locking/unlocking
		lockFskSurfaceArea(gFBGlobals.frameBuffer, &r, NULL, NULL);
		FskAndroidFrameBufferPrintfDebug(" - first Lock - bits %x", gFBGlobals.frameBuffer->bits);
		unlockFskSurface(gFBGlobals.frameBuffer);
		lockFskSurfaceArea(gFBGlobals.frameBuffer, &r, NULL, NULL);
		FskAndroidFrameBufferPrintfDebug(" - second Lock - bits %x", gFBGlobals.frameBuffer->bits);
		*bitmapOut = gFBGlobals.frameBuffer;
	}

	return kFskErrNone;
}

FskErr androidEndFramebufferReadable(FskBitmap bitmap) {
	FskAndroidFrameBufferPrintfDebug("androidEndFramebufferReadable - unlock");
	unlockFskSurface(bitmap);
	return kFskErrNone;
}

#if (ANDROID_VERSION == 4)
/***********/
FskErr lockFskSurface(FskBitmap bitmap, void **baseaddr, int *rowBytes) {
	FskErr err;
	int rb;
	ANativeWindow_Buffer info;
	ARect dirty;
	int aerr = 0;


	if ((bitmap != gFBGlobals.frameBuffer) || (bitmap->glPort)) {
		if (baseaddr)
			*baseaddr = bitmap->bits;
		if (rowBytes)
			*rowBytes = bitmap->rowBytes;
		bitmap->surfaceLocked++;
		return kFskErrNone;
	}

	FskFrameBufferGrabScreenForDrawing();


	FskAndroidFrameBufferPrintfDebug("lockFskSurface - frameBuffer - bitmap->surfaceLocked(%d)", bitmap->surfaceLocked);

	if (0 == bitmap->surfaceLocked++) {
		int screenSurface = 1;

		dirty.left = dirty.top = 0;
		dirty.right = dirty.bottom = 1;

		if (gFBGlobals.surface == NULL) {
			FskAndroidFrameBufferPrintfMinimal("gFBGlobals.surface is NULL (yikes!)");
			aerr = -1;
		}
		else {
			gFBGlobals.surfaceLocked++;
			aerr = ANativeWindow_lock((ANativeWindow*)gFBGlobals.surface, &info, &dirty);
			FskAndroidFrameBufferPrintfDebug("--- native Screen Locked here (bits:%p) [%d x %d] here fullscreen", info.bits, info.width, info.height);
		}


		if (aerr == -EAGAIN) {
			FskAndroidFrameBufferPrintfMinimal("trying to lock the surface - but it's held");
			rb = 0;
		}
		else if (aerr) {
			info.width = gFBGlobals.backingBuffer->bounds.width;
			info.height = gFBGlobals.backingBuffer->bounds.height;
			rb = gFBGlobals.backingBuffer->rowBytes;
			info.bits = gFBGlobals.backingBuffer->bits;
		}
		else {
			rb = info.stride * 2;
		}

		gFBGlobals.frameBuffer->bits = info.bits;
		gFBGlobals.frameBuffer->rowBytes = rb;
		gFBGlobals.frameBuffer->bounds.width = info.width;
		gFBGlobals.frameBuffer->bounds.height = info.height;
	}
	else {
		FskAndroidFrameBufferPrintfDebug("lockFskSurface - frameBuffer %x ALREADY LOCKED  - lockCount(%d) bits=%x - fbBits=%x", bitmap, bitmap->surfaceLocked, bitmap->bits, gFBGlobals.frameBuffer->bits);
	}


	if (baseaddr) {
		*baseaddr = gFBGlobals.frameBuffer->bits;
		if ((int)*baseaddr == -1) {
			FskAndroidFrameBufferPrintfMinimal("returning -1 for bits in lockSurface.");
		}
	}

	if (rowBytes)
		*rowBytes = gFBGlobals.frameBuffer->rowBytes;

	FskFrameBufferReleaseScreenForDrawing();

	return kFskErrNone;
}

#else

/***********/
FskErr lockFskSurface(FskBitmap bitmap, void **baseaddr, int *rowBytes) {
	int rb;
	android::Surface::SurfaceInfo info;
	android::status_t aerr = 0;

	if ((bitmap != gFBGlobals.frameBuffer) || (bitmap->glPort)) {
		if (baseaddr)
			*baseaddr = bitmap->bits;
		if (rowBytes)
			*rowBytes = bitmap->rowBytes;
		bitmap->surfaceLocked++;
		return kFskErrNone;
	}

	FskFrameBufferGrabScreenForDrawing();

	FskAndroidFrameBufferPrintfDebug("lockFskSurface - \"frameBuffer\" - lockCount(%d)", bitmap->surfaceLocked);

	if (0 == bitmap->surfaceLocked++) {
		android::Surface *screenSurface = (android::Surface *)gFBGlobals.surface;

		if (screenSurface) {
			gFBGlobals.surfaceLocked++;
			aerr = screenSurface->lock(&info);

			FskAndroidFrameBufferPrintfDebug("--- native screen locked here (bits:%p) [%d x %d] here fullscreen", info.bits, info.w, info.h);
			if (aerr == -EAGAIN) {
				FskAndroidFrameBufferPrintfMinimal("trying to lock the surface - but it's held");
				rb = 0;
			}
			else if (aerr) {
				FskAndroidFrameBufferPrintfMinimal("Error from screenSurface->lock (%d) - there's no info. use back-buffer", aerr);
				info.w = gFBGlobals.backingBuffer->bounds.width;
				info.h = gFBGlobals.backingBuffer->bounds.height;
				rb = gFBGlobals.backingBuffer->rowBytes;
				info.bits = gFBGlobals.backingBuffer->bits;
			}
			else {
				rb = info.s * 2;
			}
		}
		else {
			gFBGlobals.backBufferLocked++;
			if (gFBGlobals.midSizeChange) {
				FskAndroidFrameBufferPrintfDebug("-- not locking frambuffer --  lock backingStore->bits: %p - gMidSizeChange(%d) screenSurface(%x)", gFBGlobals.backingBuffer->bits, gFBGlobals.midSizeChange, screenSurface);
			}
			else {
				FskAndroidFrameBufferPrintfDebug("-- screenSurface is NULL (%x) gMidSizeChange(%d), \"lock\" bits in backingStore: %x", screenSurface, gFBGlobals.midSizeChange, gFBGlobals.backingBuffer->bits);
			}
			info.w = gFBGlobals.backingBuffer->bounds.width;
			info.h = gFBGlobals.backingBuffer->bounds.height;
			rb = gFBGlobals.backingBuffer->rowBytes;
			info.bits = gFBGlobals.backingBuffer->bits;
		}

		gFBGlobals.frameBuffer->bits = info.bits;
		gFBGlobals.frameBuffer->rowBytes = rb;
		gFBGlobals.frameBuffer->bounds.width = info.w;
		gFBGlobals.frameBuffer->bounds.height = info.h;
	}
	else {
		FskAndroidFrameBufferPrintfDebug("lockFskSurface - frameBuffer %x ALREADY LOCKED  - lockCount(%d) bits=%x - fbBits=%x", bitmap, bitmap->surfaceLocked, bitmap->bits, gFBGlobals.frameBuffer->bits);
	}


	if (baseaddr) {
		*baseaddr = gFBGlobals.frameBuffer->bits;
		if ((int)*baseaddr == -1) {
			FskAndroidFrameBufferPrintfMinimal("returning -1 for bits in lockSurface.");
		}
	}

	if (rowBytes)
		*rowBytes = gFBGlobals.frameBuffer->rowBytes;

	FskFrameBufferReleaseScreenForDrawing();

	return kFskErrNone;
}
#endif

/***********/
FskErr lockFskSurfaceArea(FskBitmap bitmap, FskRectangleRecord *r, void **baseaddr, int *rowBytes) {
	FskRectangleRecord area = *r;
	int rb;
#if (ANDROID_VERSION == 4)
	ANativeWindow_Buffer info;
	ARect dirty;
	int aerr = 0;
#else
	android::Surface::SurfaceInfo info;
	android::status_t aerr = 0;
#endif

	if ((bitmap != gFBGlobals.frameBuffer) || (bitmap->glPort)) {
		if (baseaddr)
			*baseaddr = bitmap->bits;
		if (rowBytes)
			*rowBytes = bitmap->rowBytes;
		bitmap->surfaceLocked++;
		return kFskErrNone;
	}

	FskFrameBufferGrabScreenForDrawing();

	FskAndroidFrameBufferPrintfDebug("lockFskSurfaceArea[%d,%d %dx%d] - frameBuffer - prior lockCount(%d)", area.x, area.y, area.width, area.height, bitmap->surfaceLocked);

	FskRectangleIntersect(&area, &gFBGlobals.frameBuffer->bounds, &area);

	if (0 == bitmap->surfaceLocked++) {
#if (ANDROID_VERSION == 4)
		int screenSurface = (int)gFBGlobals.surface;
#else
		android::Surface *screenSurface = (android::Surface *)gFBGlobals.surface;
#endif
		if (screenSurface) {
			gFBGlobals.surfaceLocked++;

#if (ANDROID_VERSION == 4)
			dirty.left = area.x;
			dirty.right = area.x + area.width;
			dirty.top = area.y;
			dirty.bottom = area.y + area.height;
			aerr = ANativeWindow_lock((ANativeWindow*)gFBGlobals.surface, &info, &dirty);
			FskAndroidFrameBufferPrintfDebug("--- surface (area) [%d , %d %d x %d] locked here (bits:%x) fullscreen:[%d x %d]", dirty.left, dirty.top, dirty.right, dirty.bottom, info.bits, info.width, info.height);
#else
			android::Region *rgn = new android::Region(android::Rect(area.x, area.y, area.x+area.width, area.y+area.height));
			aerr = screenSurface->lock(&info, rgn);

			FskAndroidFrameBufferPrintfDebug("--- surface (area) locked here (bits:%x) [%d x %d] here fullscreen", info.bits, info.w, info.h);
#endif
			if (aerr == -EAGAIN) {
				FskAndroidFrameBufferPrintfMinimal("trying to lock the surface (area) - but it's held");
				rb = 0;
			}
			else {
#if (ANDROID_VERSION == 2 || ANDROID_VERSION == 3)
				rb = info.s * 2;
#elif (ANDROID_VERSION == 4)
				rb = info.stride * 2;
#else
				rb = info.bpr;
#endif
			}
		}
		else {
			gFBGlobals.backBufferLocked++;
#if (ANDROID_VERSION == 4)
			FskAndroidFrameBufferPrintfDebug("-- screenSurface is NULL (%x) or gMidSizeChange (%d) , \"lock\" bits in backingStore: %x [%d, %d]", screenSurface, gFBGlobals.midSizeChange, gFBGlobals.backingBuffer->bits, gFBGlobals.backingBuffer->bounds.width, gFBGlobals.backingBuffer->bounds.height);
			info.width = gFBGlobals.backingBuffer->bounds.width;
			info.height = gFBGlobals.backingBuffer->bounds.height;
			rb = gFBGlobals.backingBuffer->rowBytes;
			info.bits = gFBGlobals.backingBuffer->bits;
#else
			FskAndroidFrameBufferPrintfDebug("-- screenSurface is NULL (%x) or gMidSizeChange (%d) , \"lock\" bits in backingStore: %x [%d, %d]", screenSurface, gFBGlobals.midSizeChange, gFBGlobals.backingBuffer->bits, gFBGlobals.backingBuffer->bounds.width, gFBGlobals.backingBuffer->bounds.height);
			info.w = gFBGlobals.backingBuffer->bounds.width;
			info.h = gFBGlobals.backingBuffer->bounds.height;
			rb = gFBGlobals.backingBuffer->rowBytes;
			info.bits = gFBGlobals.backingBuffer->bits;
#endif
		}

		gFBGlobals.frameBuffer->bits = info.bits;
		gFBGlobals.frameBuffer->rowBytes = rb;
#if (ANDROID_VERSION == 4)
		gFBGlobals.frameBuffer->bounds.width = info.width;
		gFBGlobals.frameBuffer->bounds.height = info.height;
#else
		gFBGlobals.frameBuffer->bounds.width = info.w;
		gFBGlobals.frameBuffer->bounds.height = info.h;
#endif
	}
	else {
		FskAndroidFrameBufferPrintfDebug("lockFskSurfaceArea - frameBuffer ALREADY LOCKED  - lockCount now(%d)", bitmap->surfaceLocked);
	}


	if (baseaddr) {
		*baseaddr = gFBGlobals.frameBuffer->bits;
		if ((int)*baseaddr == -1) {
			FskAndroidFrameBufferPrintfMinimal("returning -1 for bits in lockSurfaceArea.");
		}
	}

	if (rowBytes)
		*rowBytes = gFBGlobals.frameBuffer->rowBytes;

	FskFrameBufferReleaseScreenForDrawing();

	return kFskErrNone;
}

/***********/
FskErr unlockFskSurface(FskBitmap bitmap) {
#if (ANDROID_VERSION == 4)
	int32_t	aerr = 0;
#else
	android::status_t aerr = 0;
#endif

	if (bitmap->glPort) {
		gAndroidCallbacks->checkSizeChangeCompleteCB();
		return kFskErrNone;
	}

	if (bitmap != gFBGlobals.frameBuffer) {
		bitmap->surfaceLocked--;
		return kFskErrNone;
	}

	FskFrameBufferGrabScreenForDrawing();

	FskAndroidFrameBufferPrintfDebug("unlockFskSurface - frameBuffer lockCount prior (%d)", bitmap->surfaceLocked);

	if (0 == --bitmap->surfaceLocked) {
		if (gFBGlobals.backBufferLocked) {
			FskAndroidFrameBufferPrintfDebug("--- backbuffer was locked (%d) - decrease it.", gFBGlobals.backBufferLocked);
			gFBGlobals.backBufferLocked--;
			bitmap->bits = (void*)-1;
		}
		else {
#if (ANDROID_VERSION == 4)
			{
				if (NULL != gFBGlobals.surface) {
					FskAndroidFrameBufferPrintfDebug("--- screen was locked doing the UNLOCK of (%x) here", gFBGlobals.frameBuffer->bits);
					aerr = ANativeWindow_unlockAndPost((ANativeWindow*)gFBGlobals.surface);
				}
				else {
					FskAndroidFrameBufferPrintfMinimal("trying to unlockAndPost surface, but it's NULL");
				}
#else		// not ANDROID_VERSION 4
			android::Surface *screenSurface = (android::Surface *)gFBGlobals.surface;
			if (!screenSurface) {
				FskAndroidFrameBufferPrintfMinimal("where did screenSurface go? it was locked!");
			}
			else {
				FskAndroidFrameBufferPrintfDebug("--- screen was locked doing the UNLOCK of (%x) here", gFBGlobals.frameBuffer->bits);
				aerr = screenSurface->unlockAndPost();
#endif
			}

			gFBGlobals.surfaceLocked--;
if (gFBGlobals.surfaceLocked < 0) {
	FskAndroidFrameBufferPrintfMinimal("Win The Future? surfaceLocked went negative: %d", gFBGlobals.surfaceLocked);
	gFBGlobals.surfaceLocked = 0;
}
			gFBGlobals.frameBuffer->bits = (void*)-1;
			if (aerr < 0) {
				FskAndroidFrameBufferPrintfMinimal("native error unlocking surface: %d", aerr);
			}
		}
		gAndroidCallbacks->checkSizeChangeCompleteCB();
	}
	else {
		if (gFBGlobals.backBufferLocked) {
			FskAndroidFrameBufferPrintfDebug("--- backbuffer remains locked (%d)", gFBGlobals.backBufferLocked);
		}
		if (gFBGlobals.surfaceLocked) {
			FskAndroidFrameBufferPrintfDebug("--- native surface remains locked");
		}
	}
	FskFrameBufferReleaseScreenForDrawing();

	return kFskErrNone;
}


FskErr androidGrabScreenForDrawing() {
	FskMutexAcquire(gFBGlobals.screenMutex);
	return kFskErrNone;
}

FskErr androidReleaseScreenForDrawing() {
	FskMutexRelease(gFBGlobals.screenMutex);
	return kFskErrNone;
}


FskErr getCursorLocation(FskPoint location) {
	gAndroidCallbacks->getLastXYCB(&location->x, &location->y);
	return kFskErrNone;
}


FskErr getFskScreenBitmap(FskBitmap *bitmap) {
#if 0
	// This caused grief with O-Auth. When in the background, an invalidate
	// was attempted an hung up here. http server was not allowed to run
	// to complete the O-Auth. Bugz: 104354
	// Originally, this code helped Bugz: 103880, but other changes seem to have
	// improved things.
	while (gAndroidCallbacks->noWindowDontDrawCB()) {
		// - it's possible this can spin forever waiting for the screeen to appear.
		FskDebugStr(" - getFskScreenBitmap - about to Yield");
		FskThreadYield();
		usleep(100000);
	}
#endif
    FskAndroidFrameBufferPrintfDebug(" getFskScreenBitmap %x (bits:%x)  into  %x", gFBGlobals.frameBuffer, gFBGlobals.frameBuffer->bits, bitmap);

    *bitmap = gFBGlobals.frameBuffer;
    return kFskErrNone;
}

FskErr getFskSurfaceBounds(FskRectangleRecord *bounds) {
//    FskAndroidFrameBufferPrintfDebug(" getFskSurfaceBounds %d %d %d %d", gFBGlobals.frameBuffer->bounds.x, gFBGlobals.frameBuffer->bounds.y, gFBGlobals.frameBuffer->bounds.width, gFBGlobals.frameBuffer->bounds.height);
    return FskBitmapGetBounds(gFBGlobals.frameBuffer, bounds);
}


FskErr androidSetTransitionState(int state) {
	gAndroidCallbacks->setTransitionStateCB(state);
	return kFskErrNone;
}

static FskErr androidGetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeBoolean;
	property->value.b = gDrawingPumpEnabled;
	return kFskErrNone;
}

static FskErr androidSetContinuousDrawing(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	Boolean on = property->value.b;

	if (on) {
		if (!gDrawingPumpEnabled) {
			gDrawingPumpEnabled = true;
			gAndroidCallbacks->setContinuousDrawingCB(true);
			FskAndroidFrameBufferPrintfDebug("** Android START drawing pump **");
		}
	}
	else {
		if (gDrawingPumpEnabled) {
			gDrawingPumpEnabled = false;
			gAndroidCallbacks->setContinuousDrawingCB(false);
			FskAndroidFrameBufferPrintfDebug("** Android STOP drawing pump **");
		}
	}

	return kFskErrNone;
}

static FskErr androidGetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	UInt32 interval;
	gAndroidCallbacks->getContinuousDrawingUpdateIntervalCB(&interval);

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = interval;
	return kFskErrNone;
}


static FskErr androidSetUpdateInterval(void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	//We set the fixed value to 16ms, need to get this value from android
	//gUpdateInterval = property->value.integer;

	return kFskErrNone;

}


static FskMediaPropertyEntryRecord gAndroidProperties[] = {
	{ kFskFrameBufferPropertyContinuousDrawing, kFskMediaPropertyTypeBoolean, androidGetContinuousDrawing, androidSetContinuousDrawing },
	{ kFskFrameBufferPropertyUpdateInterval, kFskMediaPropertyTypeInteger, androidGetUpdateInterval, androidSetUpdateInterval },
	{ kFskMediaPropertyUndefined, kFskMediaPropertyTypeUndefined, NULL, NULL }
};

FskErr androidHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	//check the android version, Choreographer only works above 4.1
	if (propertyID == kFskFrameBufferPropertyContinuousDrawing) {
		char	*modelName  = NULL;
		char	*osVersion  = NULL;

		gAndroidCallbacks->getModelInfoCB(&modelName, &osVersion, NULL, NULL, NULL);		  int ver_hi = osVersion[8]  - '0';
		int ver_lo = osVersion[10] - '0';

		if ((ver_hi<4) || ((ver_hi==4) && (ver_lo<1))) {
			*set = false;
			*get = false;
			*dataType = kFskMediaPropertyTypeUndefined;
			return kFskErrUnimplemented;
		}
	}

	return FskMediaHasProperty(gAndroidProperties, propertyID, get, set, dataType);
}

FskErr androidSetProperty(UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(gAndroidProperties, NULL, NULL, propertyID, property);
}

FskErr androidGetProperty(UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(gAndroidProperties, NULL, NULL, propertyID, property);
}

int setupAndroidFramebuffer() {
	SInt32 w, h, mw, mh;
	int rowBytes;

	FskMemSet(&gFBGlobals, 0, sizeof(FskFBGlobalsRecord));
	gAndroidCallbacks->setFBGlobalsCB(&gFBGlobals);
	gAndroidCallbacks->getScreenSizeCB(&w, &h, &mw, &mh);

	FskMutexNew(&gFBGlobals.screenMutex, (char*)"screenMutex");
	FskAndroidFrameBufferPrintfMinimal("setupAndroidFramebuffer");
	rowBytes = mw * 2;

	FskAndroidFrameBufferPrintfDebug(" - w=%d h=%d rowBytes %d", mw, mh, rowBytes);

    vectors.doGetScreenBounds = getFskSurfaceBounds;
	vectors.doGetScreenBitmap = getFskScreenBitmap;
 	vectors.doGetCursorLocation = getCursorLocation;
 	vectors.doLockSurface = lockFskSurface;
 	vectors.doLockSurfaceArea = lockFskSurfaceArea;
 	vectors.doUnlockSurface = unlockFskSurface;

	vectors.doLockSurfaceReadable = androidStartFramebufferReadable;
	vectors.doUnlockSurfaceReadable = androidEndFramebufferReadable;
	vectors.doGrabScreenForDrawing = androidGrabScreenForDrawing;
	vectors.doReleaseScreenForDrawing = androidReleaseScreenForDrawing;
	vectors.doSetTransitionState = androidSetTransitionState;

	vectors.doHasProperty = androidHasProperty;
	vectors.doSetProperty = androidSetProperty;
	vectors.doGetProperty = androidGetProperty;

    FskFrameBufferSetVectors(&vectors);

	// need to check info - native format
	// and convert that to kFskBitmapFormat16RGB565LE

	// set up bitmap for entire screen (for backing store if no status bar)
    FskBitmapNew(mw, mh, kFskBitmapFormat16RGB565LE, &gFBGlobals.frameBuffer);
	FskBitmapNewWrapper(mw, mh, kFskBitmapFormat16RGB565LE, 16, (void*)gFBGlobals.frameBuffer->bits, rowBytes, &gFBGlobals.backingBuffer);

	FskAndroidFrameBufferPrintfMinimal("Screen width: %d, height: %d - current surface: %d %d", (int)mw, (int)mh, (int)w, (int)h);
	// resize the bounds to match current screen state (no status bar)
	FskRectangleSet(&gFBGlobals.frameBuffer->bounds, 0, 0, w, h);
	FskRectangleSet(&gFBGlobals.backingBuffer->bounds, 0, 0, w, h);

	FskAndroidFrameBufferPrintfDebug(" - created gFBGlobals.backingBuffer %x bits at %x", gFBGlobals.backingBuffer, gFBGlobals.backingBuffer->bits);
	FskAndroidFrameBufferPrintfDebug(" - setting framebuffer %x to use gFBGlobals.backingBuffer bits", gFBGlobals.frameBuffer);

	return 0;
}

int terminateAndroidFramebuffer() {
    FskBitmapDispose(gFBGlobals.frameBuffer);
	FskMutexDispose(gFBGlobals.screenMutex);
	FskBitmapDispose(gFBGlobals.backingBuffer);
	gFBGlobals.screenMutex = NULL;
	return 0;
}

