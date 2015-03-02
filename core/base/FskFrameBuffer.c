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
#define __FSKBITMAP_PRIV__
#include "FskBitmap.h"
#include "FskFrameBuffer.h"

FrameBufferVectorSet fbVectors = NULL;


#if USE_FRAMEBUFFER_VECTORS
FskErr FskFrameBufferBitmapNew(UInt32 pixelFormat, UInt32 pixelsSize, UInt32 w, UInt32 h, FskBitmap *out) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doNewBitmap))
		return kFskErrUnimplemented;
	return (*fbVectors->doNewBitmap)(pixelFormat, pixelsSize, w, h, out);
}
FskErr FskFrameBufferBitmapDispose(FskBitmap bm) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doDisposeBitmap))
		return kFskErrUnimplemented;
	return (*fbVectors->doDisposeBitmap)(bm);
}
FskErr FskFrameBufferFillRect(FskBitmap dst, FskConstColorRGBA color, FskConstGraphicsModeParameters modeParams, FskConstRectangle dstR) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doFillRect))
		return kFskErrUnimplemented;
	return (*fbVectors->doFillRect)(dst, color, modeParams, dstR);
}
FskErr FskFrameBufferShowCursor(FskConstRectangle obscure) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doShowCursor))
		return kFskErrUnimplemented;
	return (*fbVectors->doShowCursor)(obscure);
}
FskErr FskFrameBufferHideCursor(FskConstRectangle obscure) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doHideCursor))
		return kFskErrUnimplemented;
	return (*fbVectors->doHideCursor)(obscure);
}
FskErr FskFrameBufferGetCursorLocation(FskPoint location) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGetCursorLocation))
		return kFskErrUnimplemented;
	return (*fbVectors->doGetCursorLocation)(location);
}
FskErr FskFrameBufferSetCursorLocation(FskConstPoint location) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doSetCursorLocation))
		return kFskErrUnimplemented;
	return (*fbVectors->doSetCursorLocation)(location);
}
FskErr FskFrameBufferDisplayWindow(FskWindow window, FskBitmap bits, FskConstRectangle src, FskConstRectangle dst, UInt32 mode, FskConstGraphicsModeParameters modeParams) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doDisplayWindow))
		return kFskErrUnimplemented;
	return (*fbVectors->doDisplayWindow)(window, bits, src, dst, mode, modeParams);
}

FskErr FskFrameBufferGetScreenBitmap(FskBitmap *bitmap) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGetScreenBitmap))
		return kFskErrUnimplemented;
	return (*fbVectors->doGetScreenBitmap)(bitmap);
}
FskErr FskFrameBufferGetScreenBounds(FskRectangle bounds) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGetScreenBounds))
		return kFskErrUnimplemented;
	return (*fbVectors->doGetScreenBounds)(bounds);
}

FskErr FskFrameBufferSetTransitionState(int state) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doSetTransitionState))
		return kFskErrUnimplemented;
	return (*fbVectors->doSetTransitionState)(state);
}

FskErr FskFrameBufferGetEGLContext(void **display, void **surface, void **context, void **nativeWindow) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGetEGLContext))
		return kFskErrUnimplemented;
	return (*fbVectors->doGetEGLContext)(display, surface, context, nativeWindow);
}

FskErr FskFrameBufferLockSurface(FskBitmap bm, void **baseaddr, int *rowBytes) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doLockSurface))
		return kFskErrUnimplemented;
	return (*fbVectors->doLockSurface)(bm, baseaddr, rowBytes);
}

FskErr FskFrameBufferLockSurfaceArea(FskBitmap bm, FskRectangleRecord *r, void **baseaddr, int *rowBytes) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doLockSurfaceArea))
		return kFskErrUnimplemented;
	return (*fbVectors->doLockSurfaceArea)(bm, r, baseaddr, rowBytes);
}
FskErr FskFrameBufferUnlockSurface(FskBitmap bm) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doUnlockSurface))
		return kFskErrUnimplemented;
	return (*fbVectors->doUnlockSurface)(bm);
}
FskErr FskFrameBufferRefreshFromBackbuffer(FskRectangleRecord *r) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doBackbufferRefresh))
		return kFskErrUnimplemented;
	return (*fbVectors->doBackbufferRefresh)(r);
}
FskErr FskFrameBufferLockSurfaceForReading(FskBitmap *bitmap) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doLockSurfaceReadable))
		return kFskErrUnimplemented;
	return (*fbVectors->doLockSurfaceReadable)(bitmap);
}
FskErr FskFrameBufferUnlockSurfaceForReading(FskBitmap bitmap) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doUnlockSurfaceReadable))
		return kFskErrUnimplemented;
	return (*fbVectors->doUnlockSurfaceReadable)(bitmap);
}

FskErr FskFrameBufferGrabScreenForDrawing() {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGrabScreenForDrawing))
		return kFskErrUnimplemented;
	return (*fbVectors->doGrabScreenForDrawing)();
}
FskErr FskFrameBufferReleaseScreenForDrawing() {
	if ((NULL == fbVectors) || (NULL == fbVectors->doReleaseScreenForDrawing))
		return kFskErrUnimplemented;
	return (*fbVectors->doReleaseScreenForDrawing)();
}

FskErr FskFrameBufferHasProperty(UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doHasProperty))
		return kFskErrUnimplemented;
	return (*fbVectors->doHasProperty)(propertyID, get, set, dataType);
}

FskErr FskFrameBufferSetProperty(UInt32 propertyID, FskMediaPropertyValue property) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doSetProperty))
		return kFskErrUnimplemented;
	return (*fbVectors->doSetProperty)(propertyID, property);
}

FskErr FskFrameBufferGetProperty(UInt32 propertyID, FskMediaPropertyValue property) {
	if ((NULL == fbVectors) || (NULL == fbVectors->doGetProperty))
		return kFskErrUnimplemented;
	return (*fbVectors->doGetProperty)(propertyID, property);
}

#endif


FskErr FskFrameBufferSetVectors(FrameBufferVectorSet vectors) {
	fbVectors = vectors;
	return kFskErrNone;
}

FskErr FskFrameBufferGetVectors(FrameBufferVectorSet *vectors) {
	*vectors = fbVectors;
	return kFskErrNone;
}

