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

#include <android/native_window.h>
#include <android/native_window_jni.h>

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

fprintf(stderr, "AKVM - androidStartFrameBufferReadable\n");

	FskAndroidFrameBufferPrintfDebug("XXXXX -- androidStartFramebufferReadable - MidSizeChange %d backBufferLocked %d -- surfaceLocked %d", gFBGlobals.midSizeChange, gFBGlobals.backBufferLocked, gFBGlobals.surfaceLocked);

	FskAndroidFrameBufferPrintfDebug("androidStartFramebufferReadable - cycle native buffers");
	// hack - cycle through buffers by locking/unlocking
	lockFskSurfaceArea(gFBGlobals.frameBuffer, &r, NULL, NULL);
	FskAndroidFrameBufferPrintfDebug(" - first Lock - bits %x", gFBGlobals.frameBuffer->bits);
	unlockFskSurface(gFBGlobals.frameBuffer);
	lockFskSurfaceArea(gFBGlobals.frameBuffer, &r, NULL, NULL);
	FskAndroidFrameBufferPrintfDebug(" - second Lock - bits %x", gFBGlobals.frameBuffer->bits);
	*bitmapOut = gFBGlobals.frameBuffer;

	return kFskErrNone;
}

FskErr androidEndFramebufferReadable(FskBitmap bitmap) {
	FskAndroidFrameBufferPrintfDebug("androidEndFramebufferReadable - unlock");
	unlockFskSurface(bitmap);
	return kFskErrNone;
}

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

fprintf(stderr, "NOT GL\n");
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

/***********/
FskErr lockFskSurfaceArea(FskBitmap bitmap, FskRectangleRecord *r, void **baseaddr, int *rowBytes) {
	FskRectangleRecord area = *r;
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

	return kFskErrNone;
}

/***********/
FskErr unlockFskSurface(FskBitmap bitmap) {
	int32_t	aerr = 0;

	if (bitmap->glPort) {
		gAndroidCallbacks->checkSizeChangeCompleteCB();
		return kFskErrNone;
	}

	if (bitmap != gFBGlobals.frameBuffer) {
		bitmap->surfaceLocked--;
		return kFskErrNone;
	}

	return kFskErrNone;
}


FskErr androidGrabScreenForDrawing() {
	FskMutexAcquire(gFBGlobals.screenMutex);
fprintf(stderr, "LOCK SCREEN MUTEX\n");
	return kFskErrNone;
}

FskErr androidReleaseScreenForDrawing() {
fprintf(stderr, "UNLOCK SCREEN MUTEX\n");
	FskMutexRelease(gFBGlobals.screenMutex);
	return kFskErrNone;
}


FskErr getCursorLocation(FskPoint location) {
	gAndroidCallbacks->getLastXYCB(&location->x, &location->y);
	return kFskErrNone;
}


FskErr getFskScreenBitmap(FskBitmap *bitmap) {
    FskAndroidFrameBufferPrintfDebug(" getFskScreenBitmap %x (bits:%x)  into  %x", gFBGlobals.frameBuffer, gFBGlobals.frameBuffer->bits, bitmap);
    FskAndroidFrameBufferPrintfDebug("  -- frameBuffer bounds: %d, %d, %d, %d\n", gFBGlobals.frameBuffer->bounds.x, gFBGlobals.frameBuffer->bounds.y, gFBGlobals.frameBuffer->bounds.width, gFBGlobals.frameBuffer->bounds.height);
    *bitmap = gFBGlobals.frameBuffer;
    return kFskErrNone;
}

FskErr getFskSurfaceBounds(FskRectangleRecord *bounds) {
    FskAndroidFrameBufferPrintfDebug(" getFskSurfaceBounds %d %d %d %d", gFBGlobals.frameBuffer->bounds.x, gFBGlobals.frameBuffer->bounds.y, gFBGlobals.frameBuffer->bounds.width, gFBGlobals.frameBuffer->bounds.height);
    return FskBitmapGetBounds(gFBGlobals.frameBuffer, bounds);
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

	vectors.doHasProperty = androidHasProperty;
	vectors.doSetProperty = androidSetProperty;
	vectors.doGetProperty = androidGetProperty;

    FskFrameBufferSetVectors(&vectors);

	// need to check info - native format
	// and convert that to kFskBitmapFormat16RGB565LE

	// set up bitmap for entire screen (for backing store if no status bar)
	FskBitmapNewWrapper(mw, mh, kFskBitmapFormat16RGB565LE, 16, (void*)NULL, rowBytes, &gFBGlobals.frameBuffer);

	FskAndroidFrameBufferPrintfMinimal("Screen width: %d, height: %d - current surface: %d %d", (int)mw, (int)mh, (int)w, (int)h);
	// resize the bounds to match current screen state (no status bar)
	FskRectangleSet(&gFBGlobals.frameBuffer->bounds, 0, 0, w, h);

	return 0;
}

int terminateAndroidFramebuffer() {
    FskBitmapDispose(gFBGlobals.frameBuffer);
	FskMutexDispose(gFBGlobals.screenMutex);
	gFBGlobals.screenMutex = NULL;
	return 0;
}

