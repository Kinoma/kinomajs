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
#include "Fsk.h"
#include "FskBitmap.h"
#include "FskThread.h"
#include "FskExtensions.h"
#include "FskFrameBuffer.h"
#include "FskCursor.h"

#if TARGET_OS_ANDROID
#define USE_CURSOR	0
#else
#define USE_CURSOR	1
#endif

#if USE_CURSOR
static FrameBufferVectorSet vs;
static void setupCursors();
static FskBitmap	saveBits;
static FskRectangleRecord saveBitsRect;
static int			cursorHideDepth = 0;
#endif

static FskCursor	builtInCursors = NULL;

#define Q	0xff000000
#define B	0xffffffff
#define R	0xffff0000
typedef struct sCursor {
	UInt32	cursorID;
	UInt32	pixelFormat;
	int		depth;
	int		width;
	int		height;
	int		hotspotX;
	int		hotspotY;
	UInt32	data[16 * 16]; 	// +
} sCursor, *cursor;

#if USE_CURSOR
static sCursor staticCursors[] = {
{
	kFskCursorArrow,
	kFskBitmapFormat32BGRA, 32,
	16, 16,
	1, 1,
	{
	  B,B,0,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,B,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,B,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,B,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,B,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,B,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,Q,B,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,Q,Q,   B,0,0,0,0,0,0,0,

	  B,Q,Q,Q,Q,Q,Q,Q,   Q,B,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,B,B,   B,B,0,0,0,0,0,0,
	  B,Q,Q,B,Q,Q,B,0,   0,0,0,0,0,0,0,0,
	  B,Q,B,0,B,Q,Q,B,   0,0,0,0,0,0,0,0,
	  B,B,0,0,B,Q,Q,B,   0,0,0,0,0,0,0,0,
	  0,0,0,0,0,B,Q,Q,   B,0,0,0,0,0,0,0,
	  0,0,0,0,0,B,Q,Q,   B,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,B,B,   0,0,0,0,0,0,0,0
	}
},
{
	kFskCursorCopyArrow,
	kFskBitmapFormat32BGRA, 32,
	16, 16, 1, 1,
	{
	  B,B,0,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,B,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,B,0,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,B,0,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,B,0,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,B,0,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,Q,B,   0,0,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,Q,Q,Q,   B,0,0,0,0,0,0,0,

	  B,Q,Q,Q,Q,Q,Q,Q,   Q,B,B,B,B,B,0,0,
	  B,Q,Q,Q,Q,Q,B,B,   B,B,B,Q,Q,B,0,0,
	  B,Q,Q,B,Q,Q,B,0,   B,B,B,Q,Q,B,B,B,
	  B,Q,B,0,B,Q,Q,B,   B,Q,Q,Q,Q,Q,Q,B,
	  B,B,0,0,B,Q,Q,B,   B,Q,Q,Q,Q,Q,Q,B,
	  0,0,0,0,0,B,Q,Q,   B,B,B,Q,Q,B,B,B,
	  0,0,0,0,0,B,Q,Q,   B,B,B,Q,Q,B,0,0,
	  0,0,0,0,0,0,B,B,   0,0,B,B,B,B,0,0
	}
},
{
	kFskCursorWait,
	kFskBitmapFormat32BGRA, 32,
	16, 16, 5, 7,
	{
	  0,0,0,0,B,B,B,0,   0,0,0,0,0,0,0,0,
	  0,0,0,0,B,B,B,0,   0,0,0,0,0,0,0,0,
	  0,0,0,B,B,B,B,B,   0,0,0,0,0,0,0,0,
	  0,0,B,Q,Q,Q,Q,Q,   B,0,0,0,0,0,0,0,
	  0,B,Q,Q,Q,B,Q,Q,   Q,B,0,0,0,0,0,0,
	  B,Q,Q,Q,Q,B,Q,Q,   Q,Q,B,0,0,0,0,0,
	  B,Q,Q,Q,Q,B,Q,Q,   Q,Q,B,B,0,0,0,0,
	  B,Q,Q,Q,Q,B,B,B,   Q,Q,Q,B,0,0,0,0,

	  B,Q,Q,Q,Q,Q,Q,Q,   Q,Q,B,B,0,0,0,0,
	  B,Q,Q,Q,Q,Q,Q,Q,   Q,Q,B,0,0,0,0,0,
	  0,B,Q,Q,Q,Q,Q,Q,   Q,B,0,0,0,0,0,0,
	  0,0,B,Q,Q,Q,Q,Q,   B,0,0,0,0,0,0,0,
	  0,0,0,B,B,B,B,B,   0,0,0,0,0,0,0,0,
	  0,0,0,0,B,B,B,0,   0,0,0,0,0,0,0,0,
	  0,0,0,0,B,B,B,0,   0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	}
},
{
	kFskCursorIBeam,
	kFskBitmapFormat32BGRA, 32,
	16, 16, 4, 9,
	{
	  0,Q,B,B,0,0,Q,B, B,0,0,0,0,0,0,0,
	  0,0,Q,Q,B,Q,B,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,

	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,Q,B,Q,B,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,0,0,Q,B,0,0, 0,0,0,0,0,0,0,0,
	  0,0,Q,Q,B,Q,B,0, 0,0,0,0,0,0,0,0,
	  0,Q,B,B,0,0,Q,B, B,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0,
	}
},
{
	kFskCursorNotAllowed,
	kFskBitmapFormat32BGRA, 32,
	16, 16, 8, 8,
	{
	  0,0,0,0,0,0,Q,Q,   Q,Q,Q,0,0,0,0,0,
	  0,0,0,0,Q,Q,R,R,   R,R,R,Q,Q,0,0,0,
	  0,0,0,Q,R,R,0,0,   0,0,0,R,R,Q,0,0,
	  0,0,Q,R,0,0,0,0,   0,0,0,Q,R,R,Q,0,
	  0,Q,R,0,0,0,0,0,   0,0,Q,R,R,0,R,Q,
	  0,Q,R,0,0,0,0,0,   0,Q,R,R,0,0,R,Q,
	  Q,R,0,0,0,0,0,0,   Q,R,R,0,0,0,0,R,
	  Q,R,0,0,0,0,0,Q,   R,R,0,0,0,0,0,R,

	  Q,R,0,0,0,0,Q,R,   R,0,0,0,0,0,0,R,
	  Q,R,0,0,0,Q,R,R,   0,0,0,0,0,0,0,R,
	  Q,R,0,0,Q,R,R,0,   0,0,0,0,0,0,0,R,
	  0,0,R,Q,R,R,0,0,   0,0,0,0,0,0,R,0,
	  0,0,R,R,R,0,0,0,   0,0,0,0,0,0,R,0,
	  0,0,0,R,0,0,0,0,   0,0,0,0,0,R,0,0,
	  0,0,0,0,R,R,0,0,   0,0,0,R,R,0,0,0,
	  0,0,0,0,0,0,R,R,   R,R,R,0,0,0,0,0,
	}
}
};	// staticCursors
#endif

static FskCursorRecord	*currentCursor = NULL;

FskErr FskCursorSetShape(UInt32 shape) {
#if !USE_CURSOR
	return kFskErrNone;
#else
	FskCursor nu;
	FskErr	err = kFskErrNone;

	if (shape == currentCursor->cursorID)
		return kFskErrNone;

	FskCursorHide(NULL);

	nu = FskCursorFind(shape);
	if (!nu) {
		err = kFskErrUnknownElement;
		nu = FskCursorFind(kFskCursorArrow);
	}
	currentCursor = nu;

	FskCursorShow(NULL);
	return err;
#endif
}

FskCursor FskCursorFind(UInt32 id) {
#if USE_CURSOR
	FskCursor cur = builtInCursors;

	while (cur) {
		if (id == cur->cursorID)
			return cur;
		cur = cur->next;
	}
#endif
	return NULL;
}

void setupCursors() {
#if USE_CURSOR
	unsigned long *bits;
    unsigned int i;
	int x, y, j;
	FskCursor	curs, last = NULL;
	sCursor		*raw = staticCursors;

	// Future: - need to manage save bits with the setting of a cursor
	// so we can keep the size right
	if (kFskErrNone != FskBitmapNew(16, 16, kFskBitmapFormatDefault, &saveBits))
		return;

	for (i=0; i< sizeof(staticCursors) / sizeof(sCursor); i++) {
		FskMemPtrNew(sizeof(FskCursorRecord), &curs);
		curs->next = last;
		last = curs;
		curs->cursorID = raw->cursorID;
		curs->pixelFormat = raw->pixelFormat;
		curs->hotspotX = raw->hotspotX;
		curs->hotspotY = raw->hotspotY;
		curs->width = raw->width;
		curs->height = raw->height;
		if (kFskErrNone != FskBitmapNew(curs->width, curs->height, curs->pixelFormat, &curs->bits)) {
			FskBitmapSetHasAlpha(curs->bits, true);
			FskBitmapWriteBegin(curs->bits, (void**)(void*)&bits, NULL, NULL);
			j = 0;
			for (y=0; y<curs->height; y++) {
				for (x=0; x<curs->width; x++) {
					bits[j] = raw->data[j];
					j++;
				}
			}
			FskBitmapWriteEnd(curs->bits);
			FskListAppend(&builtInCursors, curs);
		}
		raw++;
	}
	currentCursor = FskCursorFind(kFskCursorArrow);
#endif
}

void FskCursorDispose(FskCursor curs) {

	if (curs) {
		FskListRemove(&builtInCursors, curs);
		if (currentCursor == curs)
			currentCursor = builtInCursors;
		FskBitmapDispose(curs->bits);
		FskMemPtrDispose(curs);
	}
}

void teardownCursors() {
#if USE_CURSOR
	while (builtInCursors)
		FskCursorDispose(builtInCursors);
	currentCursor = NULL;
#endif
}

void FskCursorShow(FskRectangle obscure) {
#if USE_CURSOR
	FskRectangleRecord	dstRect;
	FskCursor 			curs = currentCursor;
	FskPointRecord		loc;
	FskBitmap			fb = NULL;

	(void)FskFrameBufferGetScreenBitmap(&fb);

	if (!fb)
		goto bail;

	FskBitmapWriteBegin(fb, NULL, NULL, NULL);

	if (obscure) {
		FskRectangleIntersect(obscure, &saveBitsRect, &dstRect);
		if (FskRectangleIsEmpty(&dstRect))
			goto bail;
	}
	if (++cursorHideDepth == 1) {
		int	xloc, yloc;

		(void)FskFrameBufferGetCursorLocation(&loc);
		xloc = loc.x - curs->hotspotX;
		yloc = loc.y - curs->hotspotY;

		// save bits under cursor
		FskRectangleSet(&saveBitsRect, xloc, yloc, curs->width, curs->height);
		dstRect = saveBitsRect;
		FskRectangleOffset(&dstRect, -dstRect.x, -dstRect.y);
		FskBitmapDraw(fb, &saveBitsRect, saveBits, &dstRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
		// draw cursor
		FskBitmapDraw(curs->bits, &dstRect, fb, &saveBitsRect, NULL, NULL, kFskGraphicsModeAlpha, NULL);
	}

	FskBitmapWriteEnd(fb);
bail:
	;
#endif
}

void FskCursorHide(FskRectangle obscure) {
#if USE_CURSOR
	FskRectangleRecord	dstRect;
	FskBitmap fb = NULL;

	(void)FskFrameBufferGetScreenBitmap(&fb);
	if (!fb)
		goto bail;

	FskBitmapWriteBegin(fb, NULL, NULL, NULL);

	if (obscure) {
		FskRectangleIntersect(obscure, &saveBitsRect, &dstRect);
		if (FskRectangleIsEmpty(&dstRect))
			goto bail;
	}
	if (--cursorHideDepth == 0) {
		// replace bits under cursor
		dstRect = saveBitsRect;
		FskRectangleOffset(&dstRect, -dstRect.x, -dstRect.y);
		FskBitmapDraw(saveBits, &dstRect, fb, &saveBitsRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
	}

bail:
	if (fb)
		FskBitmapWriteEnd(fb);
#endif
}

FskErr FskCursorInitialize() {
#if ! USE_CURSOR
	return kFskErrNone;
#else
	FskFrameBufferGetVectors(&vs);
	vs->doShowCursor = (FrameBufferShowCursorFunction)FskCursorShow;
	vs->doHideCursor = (FrameBufferHideCursorFunction)FskCursorHide;
	setupCursors();
	return kFskErrNone;
#endif
}

FskErr FskCursorTerminate() {
#if USE_CURSOR
	FskCursorHide(NULL);
	teardownCursors();
	FskBitmapDispose(saveBits);
#endif
	return kFskErrNone;
}


