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
#ifndef __FSK_FRAMEBUFFER_H__
#define __FSK_FRAMEBUFFER_H__

#include "FskBitmap.h"
#include "FskWindow.h"
#include "FskGraphics.h"
#include "FskMedia.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef FskErr (*FrameBufferBitmapNewFunction)(UInt32 pixelFormat, UInt32 pixelsSize, UInt32 w, UInt32 h, FskBitmap *out);
typedef FskErr (*FrameBufferBitmapDisposeFunction)(FskBitmap bm);
typedef FskErr (*FrameBufferFillRectFunction)(FskBitmap dst, FskConstColorRGBA color, FskConstGraphicsModeParameters modeParams, FskConstRectangle dstR);
typedef FskErr (*FrameBufferShowCursorFunction)(FskConstRectangle obscure);
typedef FskErr (*FrameBufferHideCursorFunction)(FskConstRectangle obscure);
typedef FskErr (*FrameBufferGetCursorLocationFunction)(FskPoint location);
typedef FskErr (*FrameBufferSetCursorLocationFunction)(FskConstPoint location);
typedef FskErr (*FrameBufferDisplayWindowFunction)(FskWindow window, FskBitmap bits, FskConstRectangle src, FskConstRectangle dst, UInt32 mode, FskConstGraphicsModeParameters modeParams);
typedef FskErr (*FrameBufferGetScreenBitmapFunction)(FskBitmap *bitmap);
typedef FskErr (*FrameBufferGetScreenBoundsFunction)(FskRectangleRecord *bounds);

typedef FskErr (*FrameBufferLockSurfaceFunction)(FskBitmap bitmap, void **baseaddr, int *rowBytes);
typedef FskErr (*FrameBufferLockSurfaceAreaFunction)(FskBitmap bitmap, FskRectangleRecord *r, void **baseaddr, int *rowBytes);
typedef FskErr (*FrameBufferUnlockSurfaceFunction)(FskBitmap bitmap);

typedef FskErr (*FrameBufferLockSurfaceForReadingFunction)(FskBitmap *bitmap);
typedef FskErr (*FrameBufferUnlockSurfaceForReadingFunction)(FskBitmap bitmap);


typedef FskErr (*FrameBufferRefreshFromBackbufferFunction)(FskRectangleRecord *area);

typedef FskErr (*FrameBufferGrabScreenForDrawingFunction)();
typedef FskErr (*FrameBufferReleaseScreenForDrawingFunction)();

typedef FskErr (*FrameBufferSetTransitionStateFunction)(int state);

typedef FskErr (*FrameBufferGetEGLContextFunction)(void **display, void **surface, void **context, void **nativeWindow);

typedef FskErr (*FrameBufferHasPropertyFunction)(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
typedef FskErr (*FrameBufferSetPropertyFunction)(UInt32 propertyID, FskMediaPropertyValue property);
typedef FskErr (*FrameBufferGetPropertyFunction)(UInt32 propertyID, FskMediaPropertyValue property);

typedef struct FrameBufferVectors {

	FrameBufferBitmapNewFunction		doNewBitmap;
	FrameBufferBitmapDisposeFunction	doDisposeBitmap;
	FrameBufferFillRectFunction			doFillRect;
	FrameBufferShowCursorFunction		doShowCursor;
	FrameBufferHideCursorFunction		doHideCursor;
	FrameBufferGetCursorLocationFunction		doGetCursorLocation;
	FrameBufferSetCursorLocationFunction		doSetCursorLocation;
	FrameBufferDisplayWindowFunction	doDisplayWindow;

	FrameBufferGetScreenBitmapFunction	doGetScreenBitmap;
	FrameBufferGetScreenBoundsFunction	doGetScreenBounds;

	FrameBufferLockSurfaceFunction		doLockSurface;
	FrameBufferLockSurfaceAreaFunction	doLockSurfaceArea;
	FrameBufferUnlockSurfaceFunction	doUnlockSurface;

	FrameBufferLockSurfaceForReadingFunction	doLockSurfaceReadable;
	FrameBufferUnlockSurfaceForReadingFunction	doUnlockSurfaceReadable;

	FrameBufferRefreshFromBackbufferFunction	doBackbufferRefresh;

	FrameBufferGrabScreenForDrawingFunction		doGrabScreenForDrawing;
	FrameBufferReleaseScreenForDrawingFunction	doReleaseScreenForDrawing;

	FrameBufferSetTransitionStateFunction		doSetTransitionState;

	FrameBufferGetEGLContextFunction			doGetEGLContext;
	
	FrameBufferHasPropertyFunction				doHasProperty;
	FrameBufferSetPropertyFunction				doSetProperty;
	FrameBufferGetPropertyFunction				doGetProperty;
} FrameBufferVectors, *FrameBufferVectorSet;

#define kFskFrameBufferTransitionInitialize	0
#define kFskFrameBufferTransitionBeforeSourceFrame	1
#define kFskFrameBufferTransitionBeforeFinalFrame	2

enum {
	kFskFrameBufferPropertyContinuousDrawing = 1,	// Boolean
	kFskFrameBufferPropertyUpdateInterval,			// integer in ms
	kFskFrameBufferPropertyRetainsPixelsBetweenUpdates,			// Boolean - whether previous image is retained in next update
    kFskFrameBufferPropertyDisplayCopy               // FskBitmap
};

FskAPI(FskErr) FskFrameBufferSetVectors(FrameBufferVectorSet vectors);
FskAPI(FskErr) FskFrameBufferGetVectors(FrameBufferVectorSet *vectors);

extern FrameBufferVectorSet fbVectors;

typedef struct FskFBGlobalsRecord {
	FskBitmap		frameBuffer;
	int				surfaceLocked;
	FskBitmap		backingBuffer;
	int				backBufferLocked;
	FskBitmap		tempBuffer;
	FskMutex		screenMutex;
	void			*surface;
	int				midSizeChange;
} FskFBGlobalsRecord, *FskFBGlobals;

#if USE_FRAMEBUFFER_VECTORS
FskAPI(FskErr) FskFrameBufferBitmapNew(UInt32 pixelFormat, UInt32 pixelsSize, UInt32 w, UInt32 h, FskBitmap *out);
FskAPI(FskErr) FskFrameBufferBitmapDispose(FskBitmap bm);
FskAPI(FskErr) FskFrameBufferFillRect(FskBitmap dst, FskConstColorRGBA color, FskConstGraphicsModeParameters modeParams, FskConstRectangle dstR);
FskAPI(FskErr) FskFrameBufferShowCursor(FskConstRectangle obscure);
FskAPI(FskErr) FskFrameBufferHideCursor(FskConstRectangle obscure);
FskAPI(FskErr) FskFrameBufferGetCursorLocation(FskPoint location);
FskAPI(FskErr) FskFrameBufferSetCursorLocation(FskConstPoint location);
FskAPI(FskErr) FskFrameBufferDisplayWindow(FskWindow window, FskBitmap bits, FskConstRectangle src, FskConstRectangle dst, UInt32 mode, FskConstGraphicsModeParameters modeParams);

FskAPI(FskErr) FskFrameBufferGetScreenBitmap(FskBitmap *bitmap);
FskAPI(FskErr) FskFrameBufferGetScreenBounds(FskRectangleRecord *bounds);

FskAPI(FskErr) FskFrameBufferSetTransitionState(int state);

FskAPI(FskErr) FskFrameBufferGetEGLContext(void **display, void **surface, void **context, void **nativeWindow);


FskAPI(FskErr) FskFrameBufferLockSurface(FskBitmap bitmap, void **baseaddr, int *rowBytes);
FskAPI(FskErr) FskFrameBufferLockSurfaceArea(FskBitmap bitmap, FskRectangleRecord *r, void **baseaddr, int *rowBytes);
FskAPI(FskErr) FskFrameBufferUnlockSurface(FskBitmap bitmap);

FskAPI(FskErr) FskFrameBufferLockSurfaceForReading(FskBitmap *bitmap);
FskAPI(FskErr) FskFrameBufferUnlockSurfaceForReading(FskBitmap bitmap);

FskAPI(FskErr) FskFrameBufferRefreshFromBackbuffer(FskRectangleRecord *area);

FskAPI(FskErr) FskFrameBufferGrabScreenForDrawing();
FskAPI(FskErr) FskFrameBufferReleaseScreenForDrawing();

FskAPI(FskErr) FskFrameBufferHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskFrameBufferSetProperty(UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskFrameBufferGetProperty(UInt32 propertyID, FskMediaPropertyValue property);

#else

#define FskFrameBufferBitmapNew(a,b,c,d,e)      (kFskErrUnimplemented)
#define FskFrameBufferBitmapDispose(a)          (kFskErrUnimplemented)
#define FskFrameBufferFillRect(a,b,c,d)			(kFskErrUnimplemented)
#define FskFrameBufferShowCursor(a)				(kFskErrUnimplemented)
#define FskFrameBufferHideCursor(a)				(kFskErrUnimplemented)
#define FskFrameBufferGetCursorLocation(a)		(kFskErrUnimplemented)
#define FskFrameBufferSetCursorLocation(a)		(kFskErrUnimplemented)

#define FskFrameBufferGetScreenBitmap(a)		(kFskErrUnimplemented)
#define FskFrameBufferGetScreenBounds(a)		(kFskErrUnimplemented)

#define FskFrameBufferLockSurface(a,b,c)		(kFskErrUnimplemented)
#define FskFrameBufferLockSurfaceArea(a,b,c,d)	(kFskErrUnimplemented)
#define FskFrameBufferUnlockSurface(a)			(kFskErrUnimplemented)

#define FskFrameBufferLockSurfaceForReading(a)	(kFskErrUnimplemented)
#define FskFrameBufferUnlockSurfaceForReading(a)	(kFskErrUnimplemented)

#define FskFrameBufferRefreshFromBackbuffer(a)	(kFskErrUnimplemented)

#define FskFrameBufferGrabScreenForDrawing()	(kFskErrUnimplemented)
#define FskFrameBufferReleaseScreenForDrawing()	(kFskErrUnimplemented)
#define FskFrameBufferSetTransitionState(a)		(kFskErrUnimplemented)

#define FskFrameBufferHasProperty(a,b,c,d)		(*b = false, *c = false, *d = 0, kFskErrUnimplemented)
#define FskFrameBufferSetProperty(a,b)			(kFskErrUnimplemented)
#define FskFrameBufferGetProperty(a,b)			(kFskErrUnimplemented)

#endif

#ifdef __cplusplus
}
#endif

#endif

