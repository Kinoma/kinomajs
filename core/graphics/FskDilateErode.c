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
#define __FSKBITMAP_PRIV__

#include "FskDilateErode.h"
#include "FskMemory.h"

typedef struct Dilly Dilly;
typedef void (*HFilterProc)(Dilly *dilly, const UInt8 *src, UInt8 *dst);
typedef void (*VFilterProc)(Dilly *dilly, UInt8 *dst);

struct Dilly {
	int dstWidth;
	int midWidth;
	int srcPixBytes;
	int radius;
	int diam;
	int bufRowBytes;
	int srcRowBytes;
	int dstRowBytes;
	int shiftCount;
	UInt8 *buf;

};


/*******************************************************************************
 * SmearBuffer - replicate the uppermost Prime-initialized line upward,
 * e.g. for vKernelRadius=2,
 *  -----         -----
 *  -----         ABCDE
 *  -----   -->   ABCDE
 *  ABCDE         ABCDE
 *  FGHIJ         FGHIJ
 ******************************************************************************/

static void
SmearBuffer(Dilly *dilly)
{
	int i = dilly->radius * dilly->dstWidth;
	UInt8 *d = dilly->buf + i + dilly->dstWidth;
	const UInt8 *s = d + dilly->dstWidth;
	while (i--)
		*--d = *--s;
}


/*******************************************************************************
 * ShiftBuffer
 ******************************************************************************/

#if 0
static void
ShiftBuffer(Dilly *dilly) {
	int i = dilly->shiftCount;
	UInt8 *to = dilly->buf;
	const UInt8 *fr = to + dilly->dstWidth;

	while (i--)
		*to++ = *fr++;
}
#else
#define ShiftBuffer(dilly) FskMemMove((dilly)->buf, (dilly)->buf + (dilly)->bufRowBytes, \
									   (dilly)->shiftCount * sizeof(*(dilly)->buf))
#endif


/*******************************************************************************
 * Filter
 ******************************************************************************/

static FskErr
Filter(HFilterProc hFilter, VFilterProc vFilter, int radius, FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc) {
	FskErr		err	= kFskErrNone;
	const UInt8	*src;
	UInt8		*dst, *b, *bufLast;
	int			h;
	Dilly		dilly;
	FskRectangleRecord sRect, dRect;

	dilly.buf = NULL;

	/* Clip src rects and acquire src pointer */
	if (srcRect) {
		BAIL_IF_FALSE(FskRectangleIntersect(srcRect, &srcBM->bounds, &sRect), err, kFskErrNone);
	}
	else {
		sRect = srcBM->bounds;
	}
	src = (const UInt8*)(srcBM->bits)
		+ (sRect.y - srcBM->bounds.y) * srcBM->rowBytes
		+ (sRect.x - srcBM->bounds.x) * (srcBM->depth >> 3);

	/* Clip src rects and acquire src pointer */
	if (dstLoc) {
		dRect.x = (dstLoc->x >= dstBM->bounds.x) ? dstLoc->x : dstBM->bounds.x;
		dRect.y = (dstLoc->y >= dstBM->bounds.y) ? dstLoc->y : dstBM->bounds.y;
		dRect.width  = dstBM->bounds.width  - (dRect.x - dstBM->bounds.x);
		dRect.height = dstBM->bounds.height - (dRect.y - dstBM->bounds.y);
	}
	else {
		dRect = dstBM->bounds;
	}
	dst = (UInt8*)(dstBM->bits)
		+ (dRect.y - dstBM->bounds.y) * dstBM->rowBytes
		+ (dRect.x - dstBM->bounds.x) * (dstBM->depth >> 3);

	/* Clip to smaller of src and dst rect */
	if (dRect.width  > sRect.width)
		dRect.width  = sRect.width;
	if (dRect.height > sRect.height)
		dRect.height = sRect.height;

	/* Adjust src pointer to point to alpha */
	switch (srcBM->pixelFormat) {
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32ABGR:
		case kFskBitmapFormat8A:
		case kFskBitmapFormat8G:
		case kFskBitmapFormat16AG:
#if TARGET_RT_BIG_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:
#endif /* TARGET_RT_BIG_ENDIAN */
			src += 0;
			break;
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:
#endif /* TARGET_RT_LITTLE_ENDIAN */
			src += 3;
			break;
		default:
			return kFskErrUnsupportedPixelType;
	}

	if (dstBM->pixelFormat != kFskBitmapFormat8G && dstBM->pixelFormat != kFskBitmapFormat8A)
		return kFskErrUnsupportedPixelType;

	if (dRect.width < (radius << 1) || dRect.height < (radius << 1))
		return kFskErrInvalidParameter;

	dilly.dstWidth    = dRect.width;
	dilly.midWidth    = dilly.dstWidth - (radius << 1);
	dilly.srcPixBytes = (srcBM->depth >> 3);
	dilly.radius      = radius;
	dilly.diam        = (radius << 1) + 1;
	dilly.bufRowBytes = dilly.dstWidth;
	dilly.srcRowBytes = srcBM->rowBytes;
	dilly.dstRowBytes = dstBM->rowBytes;
	dilly.shiftCount  = (dilly.diam - 1) * dilly.bufRowBytes;
	BAIL_IF_ERR(err = FskMemPtrNew(dilly.diam * dilly.bufRowBytes, &dilly.buf));
	bufLast = dilly.buf + dilly.shiftCount;

	/* Prime */
	for (h = radius, b = dilly.buf + (radius + 1) * dilly.bufRowBytes; h--; src += dilly.srcRowBytes, b += dilly.bufRowBytes)
		(*hFilter)(&dilly, src, b);
	SmearBuffer(&dilly);

	/* Pump */
	for (h = dRect.height - radius; h-- > 0; src += dilly.srcRowBytes, dst += dilly.dstRowBytes) {
		ShiftBuffer(&dilly);
		(*hFilter)(&dilly, src, bufLast);
		(*vFilter)(&dilly, dst);
	}

	/* Flush */
	for (h = radius; h--; dst += dilly.dstRowBytes) {
		ShiftBuffer(&dilly);
		(*vFilter)(&dilly, dst);
	}

bail:
	FskMemPtrDispose(dilly.buf);
	return err;
}


/********************************************************************************
 * HDilate - Dilate horizontally
 ********************************************************************************/

static void
HDilate(Dilly *dilly, const UInt8 *src, UInt8 *dst)
{
	UInt8 pix;
	int w, i;
	const UInt8 *s;

	/* Prime */
	for (w = dilly->radius; w > 0; --w) {
		pix = 0;
		for (i = dilly->diam - w, s = src; i--; s += dilly->srcPixBytes)
			if (pix < *s)
				pix = *s;
		*dst++ = pix;
	}

	/* Pump */
	for (w = dilly->midWidth; w-- > 0; src += dilly->srcPixBytes) {
		pix = 0;
		for (i = dilly->diam, s = src; i--; s += dilly->srcPixBytes)
			if (pix < *s)
				pix = *s;
		*dst++ = pix;
	}

	/* Flush */
	for (w = dilly->radius; w > 0; --w, src += dilly->srcPixBytes) {
		pix = 0;
		for (i = dilly->radius + w, s = src; i--; s += dilly->srcPixBytes)
			if (pix < *s)
				pix = *s;
		*dst++ = pix;
	}
}


/********************************************************************************
 * VDilate - Dilate vertically
 ********************************************************************************/

static void
VDilate(Dilly *dilly, UInt8 *dst) {
	int w, i;
	UInt8 *src, *s;
	for (w = dilly->dstWidth, src = dilly->buf; w--; ++src) {
		UInt8 pix = 0;
		for (i = dilly->diam, s = src; i--; s += dilly->bufRowBytes)
			if (pix < *s)
				pix = *s;
		*dst++ = pix;
	}
}


/********************************************************************************
 * HErode - Erode horizontally
 ********************************************************************************/

static void
HErode(Dilly *dilly, const UInt8 *src, UInt8 *dst)
{
	UInt8 pix;
	int w, i;
	const UInt8 *s;

	/* Prime */
	for (w = dilly->radius; w > 0; --w) {
		pix = 255;
		for (i = dilly->diam - w, s = src; i--; s += dilly->srcPixBytes)
			if (pix > *s)
				pix = *s;
		*dst++ = pix;
	}

	/* Pump */
	for (w = dilly->midWidth; w-- > 0; src += dilly->srcPixBytes) {
		pix = 255;
		for (i = dilly->diam, s = src; i--; s += dilly->srcPixBytes)
			if (pix > *s)
				pix = *s;
		*dst++ = pix;
	}

	/* Flush */
	for (w = dilly->radius; w > 0; --w, src += dilly->srcPixBytes) {
		pix = 255;
		for (i = dilly->radius + w, s = src; i--; s += dilly->srcPixBytes)
			if (pix > *s)
				pix = *s;
		*dst++ = pix;
	}
}


/********************************************************************************
 * VErode - Erode vertically
 ********************************************************************************/

static void
VErode(Dilly *dilly, UInt8 *dst) {
	int w, i;
	UInt8 *src, *s;
	for (w = dilly->dstWidth, src = dilly->buf; w--; ++src) {
		UInt8 pix = 255;
		for (i = dilly->diam, s = src; i--; s += dilly->bufRowBytes)
			if (pix > *s)
				pix = *s;
		*dst++ = pix;
	}
}


/*******************************************************************************
 * FskDilate
 ******************************************************************************/

FskErr
FskDilate(int radius, FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc) {
	return Filter(HDilate, VDilate, radius, srcBM, srcRect, dstBM, dstLoc);
}


/*******************************************************************************
 * FskErode
 ******************************************************************************/

FskErr
FskErode(int radius, FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc) {
	return Filter(HErode, VErode, radius, srcBM, srcRect, dstBM, dstLoc);
}
