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

#include "FskShadow.h"
#include "FskBlur.h"
#include "FskPixelOps.h"
#include "FskMemory.h"


/********************************************************************************
 ********************************************************************************
 ** Shadow Pixel procs
 ********************************************************************************
 ********************************************************************************/

typedef void (*ApplyShadowProc)(UInt32 *dst, UInt32 src);


/********************************************************************************
 * 32RGBA 32BGRA shadow straight pixel proc
 ********************************************************************************/

static void
ApplyPremulShadowToStraight32XXXA(UInt32 *dst, UInt32 src) {
	UInt32	dpx			= *dst;
	UInt8	dstAlpha	= (UInt8)(dpx >> 0);
	UInt8	srcAlpha	= (UInt8)(src >> 0);
	UInt8	resAlpha;
	UInt32	res;
	int		t, srcComp;

	if (dstAlpha == 255U || srcAlpha == 0U)	/* If dst is opaque or src is transparent ... */
		return;								/* ... no change */
	if (dstAlpha == 0U) {					/* If dst is transparent ... */
		*dst = src;							/* ... then the src takes over */
		return;
	}

	t = (int)srcAlpha * (int)dstAlpha + 128;
	t += t >> 8;
	t >>= 8;
	resAlpha = srcAlpha + dstAlpha - t;

	srcComp = (UInt8)(src >> 24);
	t = (int)((dpx >> 24) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res = t / resAlpha;

	srcComp = (UInt8)(src >> 16);
	t = (int)((dpx >> 16) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res <<= 8;
	res |= t / resAlpha;

	srcComp = (UInt8)(src >> 8);
	t = (int)((dpx >> 8) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res <<= 8;
	res |= t / resAlpha;

	res <<= 8;
	res |= resAlpha;
	*dst = res;
}
#define ApplyPremulShadowToStraight32RGBA ApplyPremulShadowToStraight32XXXA
#define ApplyPremulShadowToStraight32BGRA ApplyPremulShadowToStraight32XXXA

/********************************************************************************
 * 32ARGB 32ABGR shadow straight pixel proc
 ********************************************************************************/

static void ApplyPremulShadowToStraight32AXXX(UInt32 *dst, UInt32 src) {
	UInt32	dpx			= *dst;
	UInt8	dstAlpha	= dpx >> 24;
	UInt8	srcAlpha	= src >> 24;
	UInt8	resAlpha;
	UInt32	res;
	int		t, srcComp;

	if (dstAlpha == 255U || srcAlpha == 0U)	/* If dst is opaque or src is transparent ... */
		return;								/* ... no change */
	if (dstAlpha == 0U) {					/* If dst is transparent ... */
		*dst = src;							/* ... then the src takes over */
		return;
	}

	t = (int)srcAlpha * (int)dstAlpha + 128;
	t += t >> 8;
	t >>= 8;
	resAlpha = srcAlpha + dstAlpha - t;
	res = resAlpha;

	srcComp = (UInt8)(src >> 16);
	t = (int)((dpx >> 16) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res <<= 8;
	res |= t / resAlpha;

	srcComp = (UInt8)(src >> 8);
	t = (int)((dpx >> 8) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res <<= 8;
	res |= t / resAlpha;

	srcComp = (UInt8)(src >> 0);
	t = (int)((dpx >> 0) & 0xFF) - (int)srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	res <<= 8;
	res |= t / resAlpha;

	*dst = res;
}
#define ApplyPremulShadowToStraight32ARGB ApplyPremulShadowToStraight32AXXX
#define ApplyPremulShadowToStraight32ABGR ApplyPremulShadowToStraight32AXXX


/********************************************************************************
 * 32A16RGB565SE premul shadow to straight pixel proc
 ********************************************************************************/

static void ApplyPremulShadowToStraight32A16RGB565SE(UInt32 *dst, UInt32 src) {
	UInt32	dpx			= *dst;
	UInt8	dstAlpha	= dpx >> 24;
	UInt8	srcAlpha	= src >> 24;
	UInt8	resAlpha;
	UInt32	res;
	int		t, srcComp;

	if (dstAlpha == 255U || srcAlpha == 0U)	/* If dst is opaque or src is transparent ... */
		return;								/* ... no change */
	if (dstAlpha == 0U) {					/* If dst is transparent ... */
		*dst = src;							/* ... then the src takes over */
		return;
	}

	/* Compute composite alpha */
	t = (int)srcAlpha * (int)dstAlpha + 128;
	t += t >> 8;
	t >>= 8;
	resAlpha = srcAlpha + dstAlpha - t;
	res = (UInt32)resAlpha << fsk32A16RGB565SEAlphaPosition;

	/* Compute composite red */
	srcComp = FskMoveField(src, fsk32A16RGB565SERedBits, fsk32A16RGB565SERedPosition, 0);
	t = FskMoveField(dpx, fsk32A16RGB565SERedBits, fsk32A16RGB565SERedPosition, 0) - srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	t /= resAlpha;
	res |= t << fsk32A16RGB565SERedPosition;

	/* Compute composite green */
	srcComp = FskMoveField(src, fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition, 0);
	t = FskMoveField(dpx, fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition, 0) - srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	t /= resAlpha;
	res |= t << fsk32A16RGB565SEGreenPosition;

	/* Compute composite blue */
	srcComp = FskMoveField(src, fsk32A16RGB565SEBlueBits, fsk32A16RGB565SEBluePosition, 0);
	t = FskMoveField(dpx, fsk32A16RGB565SEBlueBits, fsk32A16RGB565SEBluePosition, 0) - srcComp;
	t *= (int)dstAlpha;
	t += 255 * srcComp;
	t += resAlpha >> 1;
	t /= resAlpha;
	res |= t << fsk32A16RGB565SEBluePosition;

	*dst = res;
}


/********************************************************************************
 * FskAlphaBlackDestinationOver32A16RGB565SE
 ********************************************************************************/

static void FskAlphaBlackDestinationOver32A16RGB565SE(UInt32 *dst, UInt32 src) {
	UInt32	d32	= *dst;

	fskConvert32A16RGB565SE32ARGB(d32);
	FskAlphaBlackDestinationOver32ARGB(&d32, src);
	fskConvert32ARGB32A16RGB565SE(d32);
	*dst = d32;
}


#if 0 /* Something thinks that Peter's Mac is big endian. This gets around that. */
/********************************************************************************
 * 32A16RGB565DE premul shadow to straight pixel proc
 ********************************************************************************/

static void ApplyPremulShadowToStraight32A16RGB565DE(UInt32 *dst, UInt32 src) {
	UInt32 d = *dst;
	fskConvert32A16RGB565DE32A16RGB565SE(d);
	fskConvert32A16RGB565DE32A16RGB565SE(src);
	ApplyPremulShadowToStraight32A16RGB565SE(&d, src);
	fskConvert32A16RGB565SE32A16RGB565DE(d);
}


/********************************************************************************
 * FskAlphaBlackDestinationOver32A16RGB565DE
 ********************************************************************************/

static void FskAlphaBlackDestinationOver32A16RGB565DE(UInt32 *dst, UInt32 src) {
	UInt32	d32	= *dst;

	fskConvert32A16RGB565DE32ARGB(d32);
	FskAlphaBlackDestinationOver32ARGB(&d32, src);
	fskConvert32ARGB32A16RGB565DE(d32);
	*dst = d32;
}
#endif


/********************************************************************************
 * ScaleStraightColorToPremulTable
 ********************************************************************************/

static FskErr ScaleStraightColorToPremulTable(FskConstColorRGBA colorRGBA, UInt32 pixelFormat, UInt32 *table) {
	FskErr	err	= kFskErrNone;
	unsigned alpha, beta, t;
	FskColorRGBARecord color;

	if (pixelFormat == kFskBitmapFormat32A16RGB565LE)					/* If 32A16RGB565 ... */
		pixelFormat = FskName2(kFskBitmapFormat, fsk32ARGBKindFormat);	/* ... store premul table as 32ARGB */

	for (alpha = 0; alpha < 256; ++alpha, ++table) {
		t = colorRGBA->a * alpha + 128U; t += t >> 8; t >>= 8;	color.a = beta = t;	/* = colorRGBA->a * a / 255 */
		t = colorRGBA->r *  beta + 128U; t += t >> 8; t >>= 8;	color.r = t;
		t = colorRGBA->g *  beta + 128U; t += t >> 8; t >>= 8;	color.g = t;
		t = colorRGBA->b *  beta + 128U; t += t >> 8; t >>= 8;	color.b = t;
		err = FskConvertColorRGBAToBitmapPixel(&color, pixelFormat, table);
	}
	return err;
}


/********************************************************************************
 * CopyAlpha copies from the [optional] src crop in the src bitmap to the dst bitmap.
 * The source is shifted as specified by the offset parameter.
 * Note that the entire dst bimap memory is cleared to zero outside of the copied alpha.
 ********************************************************************************/

static FskErr
CopyAlpha(FskConstPoint offset, FskBitmap src, FskConstRectangle srcCrop, FskBitmap dst) {
	FskErr	err			= kFskErrNone;
	int		srcPixBytes	= src->depth >> 3;
	int sBump, dBump, w, h;
	const UInt8 *s;
	UInt8 *d;
	FskRectangleRecord srcRect;

	/* Get src rect by assuring that the src crop rect is entirely contained in the src bitmap. */
	if (srcCrop) {	BAIL_IF_FALSE(FskRectangleIntersect(srcCrop, &src->bounds, &srcRect), err, kFskErrNothingRendered);	}
	else {			srcRect = src->bounds;	}

	/* Get pointers to the src and dst bitmap pixels */
	s = (const UInt8*)(src->bits)
	  + (srcRect.y - src->bounds.y) * src->rowBytes
	  + (srcRect.x - src->bounds.x) * srcPixBytes;
	d = (UInt8*)(dst->bits);

	/* Adjust pointers and dimensions for the offset */
	if (offset->x >= 0) {											/* Shifting src to the right */
		if (srcRect.width > (w = dst->bounds.width - offset->x))	/* If there is not enough space in the dst for the shifted src... */
			srcRect.width = w;										/* ...clip the src to fit */
		d += offset->x * (dst->depth >> 3);							/* Update the dst ptr to the right */
	}
	else {															/* Shifting src to the left */
		if ((srcRect.width += offset->x) <= 0)						/* If the whole src is shifted away ... */
			return kFskErrNothingRendered;							/* ... we are done */
		s -= offset->x * srcPixBytes;								/* Skip over the left part of the src */
	}
	if (offset->y >= 0) {											/* Shifting src down */
		if (srcRect.height > (h = dst->bounds.height - offset->y))	/* If there is not enough space in the dst for the shifted src... */
			srcRect.height = h;										/* ...clip the src to fit */
		d += offset->y * dst->rowBytes;								/* Update the dst ptr downward */
	}
	else {															/* Shifting src upward */
		if ((srcRect.height += offset->y) <= 0)						/* If the whole src is shifted away ... */
			return kFskErrNothingRendered;							/* ... we are done */
		s -= offset->y * src->rowBytes;								/* Skip over the top part of the src */
	}

	sBump = src->rowBytes - srcRect.width * srcPixBytes;
	dBump = dst->rowBytes - srcRect.width;

	/* Make sure that dst alpha is clear */
	FskMemSet(dst->bits, 0, dst->rowBytes * dst->bounds.height);

	/* Determine offset to alpha, and adjust src pointer accordingly */
	switch (src->pixelFormat) {
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32ABGR:
#if TARGET_RT_BIG_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:
#endif /* TARGET_RT_BIG_ENDIAN */
			s += 0;
			break;
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:
#if TARGET_RT_LITTLE_ENDIAN
		case kFskBitmapFormat32A16RGB565LE:
#endif /* TARGET_RT_LITTLE_ENDIAN */
			s += 3;
			break;
		default:
			return kFskErrUnsupportedPixelType;
	}

	/* Copy alpha */
	for (h = srcRect.height; h--; s += sBump, d += dBump)
		for (w = srcRect.width; w--; s += srcPixBytes, ++d)
			*d = *s;

bail:
	return err;
}


/********************************************************************************
 ********************************************************************************
 ** Shadow Effect API
 ********************************************************************************
 ********************************************************************************/

FskErr
FskShadow(FskConstPoint offset, float softness, FskConstColorRGBA colorRGBA, FskConstRectangle rect, Boolean premul, FskBitmap bitmap)
{
	FskErr			err			= kFskErrNone;
	FskBitmap		shadowMatte	= NULL;
	UInt32			scolor[256];
	ApplyShadowProc	shadow;
	const UInt8		*s;
	UInt32			*d;
	int				w, h, sBump, dBump;
	FskRectangleRecord crop;

	/* Dispatcher selects the appropriate shadow pixel proc */
	if (!premul)	switch (bitmap->pixelFormat) {
		case kFskBitmapFormat32ARGB:			shadow = FskName2(ApplyPremulShadowToStraight, fsk32ARGBFormatKind);		break;
		case kFskBitmapFormat32ABGR:			shadow = FskName2(ApplyPremulShadowToStraight, fsk32ABGRFormatKind);		break;
		case kFskBitmapFormat32BGRA:			shadow = FskName2(ApplyPremulShadowToStraight, fsk32BGRAFormatKind);		break;
		case kFskBitmapFormat32RGBA:			shadow = FskName2(ApplyPremulShadowToStraight, fsk32RGBAFormatKind);		break;
		case kFskBitmapFormat32A16RGB565LE: 	shadow = FskName2(ApplyPremulShadowToStraight, fsk32A16RGB565LEFormatKind);	break;
		default:								return kFskErrUnsupportedPixelType;
	}
	else switch (bitmap->pixelFormat) {
		case kFskBitmapFormat32ARGB:			shadow = FskName2(FskAlphaBlackDestinationOver, fsk32ARGBFormatKind);		break;
		case kFskBitmapFormat32ABGR:			shadow = FskName2(FskAlphaBlackDestinationOver, fsk32ABGRFormatKind);		break;
		case kFskBitmapFormat32BGRA:			shadow = FskName2(FskAlphaBlackDestinationOver, fsk32BGRAFormatKind);		break;
		case kFskBitmapFormat32RGBA:			shadow = FskName2(FskAlphaBlackDestinationOver, fsk32RGBAFormatKind);		break;
		case kFskBitmapFormat32A16RGB565LE: 	shadow = FskName2(FskAlphaBlackDestinationOver, fsk32A16RGB565LEFormatKind);break;															break;
		default:								return kFskErrUnsupportedPixelType;
	}


	/* Assure that the bitmap rect region is contained in the bitmap bounds */
	if (rect) {	BAIL_IF_FALSE(FskRectangleIntersect(rect, &bitmap->bounds, &crop), err, kFskErrNothingRendered);	}
	else {		crop = bitmap->bounds;	}

	/* Compute the shadow matte by offsetting the alpha pixels and blurring them */
	BAIL_IF_ERR(err = FskBitmapNew(crop.width, crop.height, kFskBitmapFormat8G, &shadowMatte));
	BAIL_IF_ERR(err = CopyAlpha(offset, bitmap, &crop, shadowMatte));
	BAIL_IF_ERR(err = FskGaussianBlur(shadowMatte, NULL, shadowMatte, NULL, softness, softness));
	BAIL_IF_ERR(err = ScaleStraightColorToPremulTable(colorRGBA, bitmap->pixelFormat, scolor));

	/* Initialize blit variables */
	s = (const UInt8*)(shadowMatte->bits);
	d = (UInt32*)((UInt8*)(bitmap->bits)
	  + (crop.y - bitmap->bounds.y) * bitmap->rowBytes
	  + (crop.x - bitmap->bounds.x) * (bitmap->depth >> 3));
	sBump = shadowMatte->rowBytes - crop.width * (shadowMatte->depth >> 3);
	dBump =      bitmap->rowBytes - crop.width *      (bitmap->depth >> 3);

	/* Apply the shadow matte to the entire image */
	for (h = crop.height; h--; s += sBump, d = (UInt32*)((char*)d + dBump))
		for (w = crop.width; w--; ++s, ++d)
			(*shadow)(d, scolor[*s]);

bail:
	if (shadowMatte != NULL)	FskBitmapDispose(shadowMatte);
	return err;
}
