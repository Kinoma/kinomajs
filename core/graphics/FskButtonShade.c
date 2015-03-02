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

#include "Fsk.h"
#include "FskButtonShade.h"
#include "FskErrors.h"
#include "FskBitmap.h"
#include "FskPixelOps.h"



/***************************************************************************************************
 ***************************************************************************************************
 **        Pixel Procs                                                                            **
 ***************************************************************************************************
 **************************************************************************************************/


static UInt8
PixelCopy(UInt8 d, UInt8 s)
{
	return s;
}


typedef UInt8 (*BlendOp)(UInt8 d, UInt8 s);
typedef void (*ShadeProc)(BlendOp blendOp, const UInt16 *shd, const void *src, void *dst);

/***************************************************************************************************
 * These have alpha as the first byte of the src and dst.
 * src and dst can be the same memory location.
 **************************************************************************************************/

static void
ButtonShade32AXYZ(BlendOp blendOp, const UInt16 *shd, const void *src, void *dst)
{
	const UInt8 *m = (const UInt8*)shd;
	const UInt8 *s = (const UInt8*)src;
	UInt8 *d = (UInt8*)dst;
	UInt8 k;
	
	k = m[0];						/* Get alpha from the shade image */
	*d++ = FskPixelMul8(*s++, k);	/* Alpha */
	k = m[1];						/* Get intensity from the shade image */
	*d++ = (*blendOp)(*s++, k);		/* X */
	*d++ = (*blendOp)(*s++, k);		/* Y */
	*d   = (*blendOp)(*s,   k);		/* Z */
}


/***************************************************************************************************
 * These have alpha in the last byte of the UInt32 word of the src and dst.
 * src and dst can be the same memory location.
 **************************************************************************************************/

static void
ButtonShade32XYZA(BlendOp blendOp, const UInt16 *shd, const void *src, void *dst)
{
	const UInt8 *m = (const UInt8*)shd;
	const UInt8 *s = (const UInt8*)src;
	UInt8 *d = (UInt8*)dst;
	UInt8 k;
	
	k = m[1];						/* Get intensity from the shade image */
	*d++ = (*blendOp)(*s++, k);		/* X */
	*d++ = (*blendOp)(*s++, k);		/* Y */
	*d++ = (*blendOp)(*s++, k);		/* Z */
	k = m[0];						/* Get alpha from the shade image */
	*d   = FskPixelMul8(*s, k);		/* Alpha */
}

#define ButtonShade32ARGB ButtonShade32AXYZ
#define ButtonShade32ABGR ButtonShade32AXYZ
#define ButtonShade32RGBA ButtonShade32XYZA
#define ButtonShade32BGRA ButtonShade32XYZA

//#define convert16RGB565LE32BGRAKind		FskName3(fskConvert, fsk16RGB565LEFormatKind, 32BGRA)
//#define convert32BGRAKind32A16RGB565LE	FskName3(fskConvert, 32BGRA, fsk32A16RGB565LEFormatKind)
//#define convert16RGB565LE32ARGBKind		FskName3(fskConvert, fsk16RGB565LEFormatKind, 32ARGB)
//#define convert32ARGBKind32A16RGB565LE	FskName3(fskConvert, 32ARGB, fsk32A16RGB565LEFormatKind)

#define convert16RGB565LE32ARGB		FskName3(fskConvert, fsk16RGB565LEFormatKind, fsk32ARGBFormatKind)
#define convert32ARGB32A16RGB565LE	FskName3(fskConvert, fsk32ARGBFormatKind, fsk32A16RGB565LEFormatKind)


/***************************************************************************************************
 * src and dst can be the same memory location.
 **************************************************************************************************/

static void
ButtonShade16RGB565LE32A16RGB565LE(BlendOp blendOp, const UInt16 *shd, const void *src, void *dst) {
	UInt32 dst32;
	UInt32 src32 = *((const UInt16*)src);
	convert16RGB565LE32ARGB(src32);
	ButtonShade32ARGB(blendOp, shd, &src32, &dst32);
	convert32ARGB32A16RGB565LE(dst32);
	*((UInt32*)dst) = dst32;
}


/***************************************************************************************************
 ***************************************************************************************************
 ****  FskButtonShade API
 ***************************************************************************************************
 **************************************************************************************************/

FskErr
FskButtonShade(
	int					compositionMode,
	FskBitmap			shadeBM,	/* Apply this image ... */
	FskBitmap			srcBM,		/* ... to this image ... */
	FskConstPoint		srcLoc,		/* ... at this location ... */
	FskBitmap			dstBM,		/* ... and write it to this image ... */
	FskConstPoint		dstLoc		/* ... at this location. */
) {
	FskRectangleRecord dstRect;
	FskPointRecord srcOrg;
	int w, h, srcPixBytes, dstPixBytes, mBump, sBump, dBump;
	ShadeProc shadeProc = NULL;
	const UInt16 *m;
	const char *s;
	char *d;
	BlendOp blendOp;
	static const BlendOp compProcTable[] = {
		PixelCopy,				/* These are defined above */
		FskPixelMul8,			/* These are defined in FskPixelOps.h */
		FskPixelScreen8,
		FskPixelOverlay8,
		FskPixelDarken8,
		FskPixelLighten8,
		FskPixelColorDodge8,
		FskPixelColorBurn8,
		FskPixelHardLight8,
		FskPixelSoftLight8,
		FskPixelDifference8,
		FskPixelExclusion8
	};

	/* Choose the blend proc */
	if (compositionMode < 0 || compositionMode >= (SInt32)(sizeof(compProcTable) / sizeof(compProcTable[0])))
		return kFskErrInvalidParameter;
	blendOp = compProcTable[compositionMode];

	/* We currently only support alpha+gray shading bitmaps */
	if (shadeBM->pixelFormat != kFskBitmapFormat16AG)
		return kFskErrUnsupportedPixelType;

	/* Choose the appropriate button shade proc */
	if (srcBM->pixelFormat == kFskBitmapFormat16RGB565LE) {
		if (dstBM->pixelFormat == kFskBitmapFormat32A16RGB565LE)
			shadeProc = ButtonShade16RGB565LE32A16RGB565LE;
	}
	else if (srcBM->pixelFormat == dstBM->pixelFormat) {
		switch (srcBM->pixelFormat) {
			case kFskBitmapFormat32ARGB:	shadeProc = ButtonShade32ARGB;	break;
			case kFskBitmapFormat32ABGR:	shadeProc = ButtonShade32ABGR;	break;
			case kFskBitmapFormat32BGRA:	shadeProc = ButtonShade32BGRA;	break;
			case kFskBitmapFormat32RGBA:	shadeProc = ButtonShade32RGBA;	break;
            default: break;
		}
	}
	if (shadeProc == NULL)
		return kFskErrUnsupportedPixelType;

	/* Determine src and dst locations, specified either explicitly or implicitly */
	if (dstLoc != NULL) {
		dstRect.x = dstLoc->x;
		dstRect.y = dstLoc->y;
	}
	else {
		dstRect.x = dstBM->bounds.x;
		dstRect.y = dstBM->bounds.y;
	}
	if (srcLoc != NULL) {
		srcOrg = *srcLoc;
	}
	else {
		srcOrg.x = srcBM->bounds.x;
		srcOrg.y = srcBM->bounds.y;
	}
	
	/* Compute dst rect as the intersection of the source, destination, and shading bitmap. */
	dstRect.width  = shadeBM->bounds.width;
	dstRect.height = shadeBM->bounds.height;
	if ((w = dstBM->bounds.x + dstBM->bounds.width  - dstRect.x) < dstRect.width)	dstRect.width  = w;
	if ((h = dstBM->bounds.y + dstBM->bounds.height - dstRect.y) < dstRect.height)	dstRect.height = h;
	if ((w = srcBM->bounds.x + srcBM->bounds.width  - srcOrg.x)  < dstRect.width)	dstRect.width  = w;
	if ((h = srcBM->bounds.y + srcBM->bounds.height - srcOrg.y)  < dstRect.height)	dstRect.height = h;
	if (w <= 0 || h <= 0)
		return kFskErrNothingRendered;

	/* Get parameters for the blit */
	srcPixBytes = srcBM->depth >> 3;
	dstPixBytes = dstBM->depth >> 3;
	m = (const UInt16*)(shadeBM->bits);
	s = (const char*)(srcBM->bits) +  srcOrg.y * srcBM->rowBytes +  srcOrg.x * srcPixBytes;
	d =       (char*)(dstBM->bits) + dstRect.y * dstBM->rowBytes + dstRect.x * dstPixBytes;
	mBump = shadeBM->rowBytes - dstRect.width * sizeof(UInt16);
	sBump =   srcBM->rowBytes - dstRect.width * srcPixBytes;
	dBump =   dstBM->rowBytes - dstRect.width * dstPixBytes;

	for (h = dstRect.height; h--; m = (const UInt16*)((const char*)m + mBump), s += sBump, d += dBump)
		for (w = dstRect.width; w--; ++m, s += srcPixBytes, d += dstPixBytes)
			(*shadeProc)(blendOp, m, s, d);

	return kFskErrNone;
}
