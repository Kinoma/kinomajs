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
/**
	\file	FskEffects.c
	\brief	Software implementations of visual effects.
*/
#define __FSKBITMAP_PRIV__
#define __FSKEFFECTS_PRIV__

#include "FskEffects.h"	/* Make sure that the header is self-sufficient by putting it first */

#include <math.h>
#include "FskBlur.h"
#include "FskDilateErode.h"
#include "FskMemory.h"
#include "FskPixelOps.h"
#include "FskPremultipliedAlpha.h"
#include "FskString.h"

#if SUPPORT_INSTRUMENTATION

	#define LOG_PARAMETERS

	FskInstrumentedSimpleType(Effects, effects);						/**< This declares the types needed for instrumentation. */
	#define EFFECTS_DEBUG	1
#endif /* SUPPORT_INSTRUMENTATION */

#ifndef EFFECTS_DEBUG
	#define EFFECTS_DEBUG	0
#endif /* EFFECTS_DEBUG */
#if EFFECTS_DEBUG
	#define	LOGD(...)	FskEffectsPrintfDebug(__VA_ARGS__)
	#define	LOGI(...)	FskEffectsPrintfVerbose(__VA_ARGS__)
#endif /* EFFECTS_DEBUG */
#define		LOGE(...)	FskEffectsPrintfMinimal(__VA_ARGS__)
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Support										*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#define S_INC(p, d) p = (const void*)((const char*)(p) + (d))	/**< Increment a read  pointer. \param[in,out] p the pointer. \param[in] d the byte increment. */
#define D_INC(p, d) p = (      void*)((      char*)(p) + (d))	/**< Increment a write pointer. \param[in,out] p the pointer. \param[in] d the byte increment. */


#if EFFECTS_DEBUG && defined(LOG_PARAMETERS)

static void LogSrcBitmap(FskConstBitmap srcBM, const char *name) {
	if (!srcBM)
		return;
	if (!name)
		name = "SRCBM";
	LOGD("\t%s: bounds(%ld, %ld, %ld, %ld), depth=%lu, format=%s, rowBytes=%ld, bits=%p, alpha=%d, premul=%d",
		name, srcBM->bounds.x, srcBM->bounds.y, srcBM->bounds.width, srcBM->bounds.height, srcBM->depth,
		FskBitmapFormatName(srcBM->pixelFormat), srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied);
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%ld, %ld, %ld, %ld)", name, r->x, r->y, r->width, r->height);
}

static void LogPoint(FskConstPoint p, const char *name) {
	if (!p)
		return;
	if (!name)
		name = "POINT";
	LOGD("\t%s(%ld, %ld)", name, p->x, p->y);
}

static void LogDstBitmap(FskBitmap dstBM, const char *name) {
	if (!dstBM)
		return;
	if (!name)
		name = "DSTBM";
	LOGD("\t%s: bounds(%ld, %ld, %ld, %ld),depth=%lu, format=%s, rowBytes=%ld, bits=%p, alpha=%d, premul=%d",
		name, dstBM->bounds.x, dstBM->bounds.y, dstBM->bounds.width, dstBM->bounds.height, dstBM->depth,
		FskBitmapFormatName(dstBM->pixelFormat), dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied);
}

static void LogEffectsParameters(const char *func, const void *params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	LOGD("%s(params=%p, src=%p, srcRect=%p, dst=%p, dstPoint=%p)",
		func, params, src, srcRect, dst, dstPoint);
	LogSrcBitmap(src, "src");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dst, "dst");
	LogPoint(dstPoint, "dstPoint");
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	LOGD("\t%s(%3u, %3u, %3u, %3u)", name, color->r, color->g, color->b, color->a);
}


#endif /* EFFECTS_DEBUG && LOG_PARAMETERS */


/********************************************************************************
 * ALPHAMUL
 ********************************************************************************/

void ALPHAMUL(UInt32 dst, UInt8 a0, UInt8 a1);
#define ALPHAMUL(dst, a0, a1)	\
	((dst) = (unsigned)(a0) * (a1) + 128U, (dst) = ((dst) + ((dst) >> 8)) >> 8)
	//do { (dst) = (a0); (dst) *= (a1); (dst) += 128; (dst) += (dst) >> 8; (dst) >>= 8; } while(0)


/***************************************************************************//**
 * BeginOneSrcBlit
 *	\param[in]	src				the source bitmap.
 *	\param[in]	srcRect			the subrect of the src; NULL implies the whole source.
 *	\param[in]	dst				the destination bitmap.
 *	\param[in]	dstPt			the location in the dst; NULL implies the upper-left of the dst bitmap.
 *	\param[out]	ps				pointer to a place to store the source pointer, appropriately offset.
 *	\param[out]	pSrcPixBytes	pointer to a place to store the number of bytes per pixel in the src.
 *	\param[out]	pSrcBump		pointer to a place to store the byte increment to advance from the last pixel of one line to the first of the next in the src.
 *	\param[out]	pd				pointer to a place to store the destination pointer, appropriately offset.
 *	\param[out]	pDstPixBytes	pointer to a place to store the number of bytes per pixel in the dst.
 *	\param[out]	pDstBump		pointer to a place to store the byte increment to advance from the last pixel of one line to the first of the next in the dst.
 *	\param[out]	dim				pointer to a place to store the dimensions of the rectangle to be scanned.
 *	\return		kFskErrNone		if successful.
 *******************************************************************************/

static FskErr BeginOneSrcBlit(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPt,
					const void **ps, SInt32 *pSrcPixBytes, SInt32 *pSrcBump, void **pd, SInt32 *pDstPixBytes, SInt32 *pDstBump, FskDimension dim
) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	r;
	FskPointRecord		ds;

	*ps = NULL;
	*pd = NULL;

	/* Compute clip rects */
	r = srcRect ? *srcRect : src->bounds;															/* Get source rect */
	if (dstPt)		{ ds.x = dstPt->x;		ds.y = dstPt->y;		}								/* Get destination location */
	else			{ ds.x = dst->bounds.x;	ds.y = dst->bounds.y;	}
	ds.x -= r.x;																					/* Compute delta from src to dst */
	ds.y -= r.y;
	BAIL_IF_FALSE(FskRectangleIntersect(&src->bounds, &r, &r), err, kFskErrNothingRendered);		/* Clip rect against src */
	r.x += ds.x;																					/* Offset rect to dst */
	r.y += ds.y;
	BAIL_IF_FALSE(FskRectangleIntersect(&dst->bounds, &r, &r), err, kFskErrNothingRendered);		/* Clip rect against dst */
	dim->width  = r.width;
	dim->height = r.height;

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)src, ps, pSrcBump, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(          dst, pd, pDstBump, NULL));
	*pDstPixBytes = dst->depth >> 3;
	*pd	= (void*)((char*)(*pd)																		/* Compute dst pointer */
	 	+ (r.y - dst->bounds.y) * *pDstBump
		+ (r.x - dst->bounds.x) * *pDstPixBytes);
	*pDstBump -= r.width * *pDstPixBytes;
	r.x -= ds.x;																					/* Shift rect back to src */
	r.y -= ds.y;
	*pSrcPixBytes = src->depth >> 3;
	*ps	= (const void*)((const char*)(*ps)															/* Compute src pointer */
		+ (r.y - src->bounds.y) * *pSrcBump
		+ (r.x - src->bounds.x) * *pSrcPixBytes);
	*pSrcBump -= r.width * *pSrcPixBytes;


bail:
	if (err) {
		if (*ps) {	FskBitmapReadEnd((FskBitmap)src);	*ps = NULL;	}
		if (*pd) {	FskBitmapWriteEnd(          dst);	*pd = NULL;	}
	}
	return err;
}


/***************************************************************************//**
 * BeginTwoSrcBlit
 *	\param[in]	src1			the source bitmap.
 *	\param[in]	src1Rect		the subrect of the src; NULL implies the whole source.
 *	\param[in]	src2			the source bitmap.
 *	\param[in]	src2Pt			the location of src2.
 *	\param[in]	dst				the destination bitmap.
 *	\param[in]	dstPt			the location in the dst; NULL implies the upper-left of the dst bitmap.
 *	\param[out]	ps1				pointer to a place to store the source pointer, appropriately offset.
 *	\param[out]	pSrc1PixBytes	pointer to a place to store the number of bytes per pixel in the src.
 *	\param[out]	ps2				pointer to a place to store the source pointer, appropriately offset.
 *	\param[out]	pSrc2PixBytes	pointer to a place to store the number of bytes per pixel in the src.
 *	\param[out]	pSrc2Bump		pointer to a place to store the byte increment to advance from the last pixel of one line to the first of the next in the src.
 *	\param[out]	pd				pointer to a place to store the destination pointer, appropriately offset.
 *	\param[out]	pDstPixBytes	pointer to a place to store the number of bytes per pixel in the dst.
 *	\param[out]	pDstBump		pointer to a place to store the byte increment to advance from the last pixel of one line to the first of the next in the dst.
 *	\param[out]	dim				pointer to a place to store the dimensions of the rectangle to be scanned.
 *	\return		kFskErrNone		if successful.
 *	\note		This assumes that there will be no scaling of src2, i.e. that there is a 1:1 pixel correspondence between
 *				src1Rect in the source and (0, 0, src1Rect->width, src1Rect->height) in the destination.
 *******************************************************************************/

static FskErr BeginTwoSrcBlit(FskConstBitmap src1, FskConstRectangle src1Rect, FskConstBitmap src2, FskConstPoint src2Pt, FskBitmap dst, FskConstPoint dstPt,
					const void **ps1, SInt32 *pSrc1PixBytes, SInt32 *pSrc1Bump, const void **ps2, SInt32 *pSrc2PixBytes, SInt32 *pSrc2Bump,
					void **pd, SInt32 *pDstPixBytes, SInt32 *pDstBump, FskDimension dim
) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	r;
	FskPointRecord		ds1, ss2;

	*ps1 = NULL;
	*ps2 = NULL;
	*pd  = NULL;

	/* Compute clip rects */
	r = src1Rect ? *src1Rect : src1->bounds;														/* Get source 1 rect */
	if (dstPt)		{ ds1.x = dstPt->x;			ds1.y = dstPt->y;		}							/* Get destination location */
	else			{ ds1.x = dst->bounds.x;	ds1.y = dst->bounds.y;	}
	if (src2Pt)		{ ss2.x = src2Pt->x;		ss2.y = src2Pt->y;		}							/* Get source 2 location */
	else			{ ss2.x = src2->bounds.x;	ss2.y = src2->bounds.y;	}
	ss2.x -= r.x;																					/* Compute delta from src1 to src2 */
	ss2.y -= r.y;
	ds1.x -= r.x;																					/* Compute delta from src1 to dst */
	ds1.y -= r.y;
	BAIL_IF_FALSE(FskRectangleIntersect(&src1->bounds, &r, &r), err, kFskErrNothingRendered);		/* Clip rect against src1 */
	r.x += ss2.x;																					/* Offset rect to src2 */
	r.y += ss2.y;
	BAIL_IF_FALSE(FskRectangleIntersect(&src2->bounds, &r, &r), err, kFskErrNothingRendered);		/* Clip rect against src2 */
	r.x += ds1.x - ss2.x;																			/* Offset rect to dst */
	r.y += ds1.y - ss2.y;
	BAIL_IF_FALSE(FskRectangleIntersect(&dst->bounds, &r, &r), err, kFskErrNothingRendered);		/* Clip rect against dst */
	dim->width  = r.width;
	dim->height = r.height;

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)src1, ps1, pSrc1Bump, NULL));
	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)src2, ps2, pSrc2Bump, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(          dst,  pd,  pDstBump, NULL));
	*pDstPixBytes = dst->depth >> 3;
	*pd	= (void*)((char*)(*pd)																		/* Compute dst pointer */
	 	+ (r.y - dst->bounds.y) * *pDstBump
		+ (r.x - dst->bounds.x) * *pDstPixBytes);
	*pDstBump -= r.width * *pDstPixBytes;
	r.x -= ds1.x;																					/* Shift rect back to src1 */
	r.y -= ds1.y;
	*pSrc1PixBytes = src1->depth >> 3;
	*ps1	= (const void*)((const char*)(*ps1)														/* Compute src1 pointer */
			+ (r.y - src1->bounds.y) * *pSrc1Bump
			+ (r.x - src1->bounds.x) * *pSrc1PixBytes);
	*pSrc1Bump -= r.width * *pSrc1PixBytes;
	r.x += ss2.x;																					/* Shift rect to src2 */
	r.y += ss2.y;
	*pSrc2PixBytes = src2->depth >> 3;
	*ps2	= (const void*)((const char*)(*ps2)														/* Compute src2 pointer */
			+ (r.y - src2->bounds.y) * *pSrc2Bump
			+ (r.x - src2->bounds.x) * *pSrc2PixBytes);
	*pSrc2Bump -= r.width * *pSrc2PixBytes;


bail:
	if (err) {
		if (*ps1)	{ FskBitmapReadEnd((FskBitmap)src1);	*ps1 = NULL; }
		if (*ps2)	{ FskBitmapReadEnd((FskBitmap)src2);	*ps2 = NULL; }
		if (*pd)	{ FskBitmapWriteEnd(           dst);	*pd  = NULL; }
	}
	return err;
}


/********************************************************************************
 * Copy the alpha channel.
 *	\param[in]	src		the source bitmap.
 *	\param[in]	srcRect	a subrect of the source bitmap to copy. NULL implies the whole bitmap.
 *	\param[out]	dst		the destination bitmap.
 *	\param[in]	dstPt	the location to which the src alpha channel is to be copied in the destination.
 *						NULL implies the upper left.
 *	\return		kFskErrNone	if the operation was completed successfully.
 *	\note		The alpha channel outside of the affected destination is set transparent.
 ********************************************************************************/

static FskErr CopyAlpha(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPt) {
	FskErr				err		= kFskErrNone;
	const UInt8			*s		= NULL;
	UInt8				*d		= NULL;
	SInt32				srcPixBytes, dstPixBytes, sBump, dBump, w, h;
	FskDimensionRecord	dim;

	BAIL_IF_FALSE(
		(8 == src->depth || 8 == FskBitmapFormatAlphaBits(src->pixelFormat))	&&					/* Only support 8 bit alpha or grayscale */
		(8 == dst->depth || 8 == FskBitmapFormatAlphaBits(dst->pixelFormat)),
		err, kFskErrUnsupportedPixelType);

	BAIL_IF_ERR(err = BeginOneSrcBlit(src, srcRect, dst, dstPt, (const void**)(const void*)&s, &srcPixBytes, &sBump, (void**)(void*)&d, &dstPixBytes, &dBump, &dim));
	FskMemSet(dst->bits, 0, dst->rowBytes * dst->bounds.height);									/* Make sure that dst alpha is clear */
	d += FskBitmapFormatAlphaOffset(dst->pixelFormat);
	s += FskBitmapFormatAlphaOffset(src->pixelFormat);
	for (h = dim.height; h--; s += sBump, d += dBump)
		for (w = dim.width; w--; s += srcPixBytes, d += dstPixBytes)
			*d = *s;																				/* Copy alpha */
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);

bail:
	return err;
}


/****************************************************************************//**
 * Convert a ColorRGBA from straight alpha to premultiplied alpha.
 *	\param[in]	src	the input color.
 *	\param[out]	dst	the output color. This can be the same as the src.
 *	\return	dst	teh destination color pointer, convenient for chaining.
 ********************************************************************************/

static FskColorRGBA PremultiplyColorRGBA(FskConstColorRGBA src, FskColorRGBA dst) {
	dst->a = src->a;
	dst->r = FskAlphaMul(src->a, src->r);
	dst->g = FskAlphaMul(src->a, src->g);
	dst->b = FskAlphaMul(src->a, src->b);
	return dst;
}


/****************************************************************************//**
 * ConvertColorRGBAToBitmapPixel
 *	\param[in]	src				The RGBA color to be converted. This is assumed to be given with straight alpha.
 *	\param[in]	pixelFormat		The destination pixel format.
 *	\param[in]	premultiplied	Indicates whether the destination has the colors premultiplied by alpha.
 *	\param[out]	dst				The location to store the resultant color.
 *	\return		kFskErrNone		if the operation was completed successfully.
 ********************************************************************************/

static FskErr ConvertColorRGBAToBitmapPixel(FskConstColorRGBA src, FskBitmapFormatEnum pixelFormat, Boolean premultiplied, void *dst) {
	FskColorRGBARecord color;
	if (premultiplied) {
		(void)PremultiplyColorRGBA(src, &color);
		src = &color;
	}
	return FskConvertColorRGBAToBitmapPixel(src, pixelFormat, dst);
}


union ColorInt { FskColorRGBARecord c; UInt32 i; };


/****************************************************************************//**
 * Generate a 256-entry table that linearly interpolates between two colors: c0 and c1.
 *	\param[in]	c0		the color corresponding to luminance=0.
 *	\param[in]	c1		the color corresponding to luminance=255.
 *	\param[in]	dst		the bitmap for which the table is being initialized.
 *	\param[out]	mono	the resultant monochrome table.
 *						Note that this table is dense, i.e. the stride is dst->depth/8.
 ********************************************************************************/

static void MakeMonoColorTable(FskConstColorRGBA c0, FskConstColorRGBA c1, Boolean premultiplied, FskBitmapFormatEnum pixelFormat, UInt32 mono[256]) {
	const SInt32	pixBytes	= FskBitmapFormatPixelBytes(pixelFormat);
	unsigned		i;

	if (4 == pixBytes && kFskBitmapFormat32A16RGB565LE != pixelFormat) {							/* Specialize for common case */
		#if 1	/* Faster */
			(void)ConvertColorRGBAToBitmapPixel(c0, pixelFormat, premultiplied, &mono[0]);
			(void)ConvertColorRGBAToBitmapPixel(c1, pixelFormat, premultiplied, &mono[255]);
			for (i = 1U; i < 255U; ++i) {															/* Make table, skipping 0 and 255 */
				mono[i] = mono[0];
				FskBlend32(&mono[i], mono[255], i);
			}
		#else/* More accurate */
			FskConvertColorRGBAToBitmapPixel(c0, pixelFormat, &mono[0]);
			FskConvertColorRGBAToBitmapPixel(c1, pixelFormat, &mono[255]);
			for (i = 1U; i < 255U; ++i) {															/* Make table, skipping 0 and 255 */
				mono[i] = mono[0];
				FskBlend32(&mono[i], mono[255], i);
			}
			if (premultiplied && (c0->a & c1->a) != 255) {											/* If premultiplied and either alpha < 255 */
				UInt32 aPos = FskBitmapFormatAlphaPosition(pixelFormat);
				UInt32 aMsk = 255U << aPos;
				for (i = 0; i < 256; ++i)
					mono[i] = FskAlphaScale32(mono[i] >> aPos, mono[i] | aMsk);						/* Premultiply if requested */
			}
		#endif
	}
	else {
		UInt8			*m			= (UInt8*)mono;
		union ColorInt	ci, ci0, ci1;

		ci0.c = *c0,
		ci1.c = *c1;
		FskConvertColorRGBAToBitmapPixel(&ci0.c, pixelFormat, m);									/* 0 */
		for (i = 1U, m += pixBytes; i < 255U; ++i, m += pixBytes) {									/* Make table, skipping 0 and 255 */
			ci.i = ci0.i;
			FskBlend32(&ci.i, ci1.i, i);															/* Interpolate as R8G8B8A8 */
			//ConvertColorRGBAToBitmapPixel(&ci.c, pixelFormat, premultiplied, m);
			FskConvertColorRGBAToBitmapPixel(&ci.c, pixelFormat, m);
		}
		FskConvertColorRGBAToBitmapPixel(&ci1.c, pixelFormat, m);									/* 255 */
	}
}


#define CAN_SCALE_8G (defined(SRC_8G) && SRC_8G && defined(DST_8G) && DST_8G)

#if !CAN_SCALE_8G
#define FWD_SUBBITS 18
static FskErr StretchAlpha(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect) {
	FskErr		err;
	FskFixed	x, y, x0, xd, yd;
	SInt32		srb, dBump;
	UInt32		widthBytes, height;
	const UInt8	*s0 = NULL, *sr;
	UInt8		*d  = NULL, *dEnd;

	BAIL_IF_FALSE(src->depth == 8 && dst->depth == 8, err, kFskErrUnsupportedPixelType);
	BAIL_IF_ERR(err = FskBitmapReadBegin ((FskBitmap)src, (const void**)&s0, &srb,   NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin((FskBitmap)dst,       (void**)&d,  &dBump, NULL));
	if (!srcRect)	srcRect = &src->bounds;
	if (!dstRect)	dstRect = &dst->bounds;
	s0 += srcRect->y * srb   + srcRect->x;
	d  += dstRect->y * dBump + dstRect->x;
	widthBytes = dstRect->width;
	dBump -= widthBytes;
	height = dstRect->height;
	x0 = 0;
	y  = 0;
	xd = FskFixedNDiv(srcRect->width  - 1, dstRect->width  - 1, FWD_SUBBITS);
	yd = FskFixedNDiv(srcRect->height - 1, dstRect->height - 1, FWD_SUBBITS);
	for ( ; height--; y += yd, d += dBump)
		for (x = x0, sr = s0 + (y >> FWD_SUBBITS) * srb, dEnd = widthBytes + (UInt8 *)d; (UInt8 *)d != dEnd; x += xd, ++d)
			*d = FskBilerp8((x >> (FWD_SUBBITS-4)) & 0xF, (y >> (FWD_SUBBITS-4)) & 0xF, sr + (x >> FWD_SUBBITS), srb);

bail:
	if (d)	FskBitmapWriteEnd(dst);
	if (s0)	FskBitmapReadEnd((FskBitmap)src);
	return err;
}
#endif /* !CAN_SCALE_8G */


#if 0
/****************************************************************************//**
 * Copy from source to destination outside the matte rectangle.
 *	\param[in]	src		the source bitmap.
 *	\param[in]	srcRect	the rectangle of the source to copy. NULL implies the whole src.
 *	\param[in]	mttRect	the rectangle outside of which is to be copied.
 *	\param[out]	dst		the destination bitmap.
 *	\param[in]	dstPt	the location to which the src is to be copied. NULL implies dst upper left.
 *	\return		kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/

static FskErr CopyOutsideRect(FskConstBitmap src, FskConstRectangle srcRect, FskConstRectangle mttRect, FskBitmap dst, FskConstPoint dstPt) {
	FskErr	err		= kFskErrNone;
	FskRectangleRecord sRect, dRect;

	if (!srcRect)	srcRect = &src->bounds;															/* If no source rect was supplied, use the whole src */
	if (!dstPt)		dstPt = (FskConstPoint)(&dst->bounds);											/* If no dst rect was supplied, use dst upper left */
	if (!mttRect) {																					/* If no matte rect was supplied, ... */
		FskRectangleSet(&dRect, dstPt->x, dstPt->y, srcRect->width, srcRect->height);
		return FskBitmapDraw(src, srcRect, dst, &dRect, NULL, NULL, kFskGraphicsModeCopy, NULL);	/* ... copy the whole thing. */
	}

	/* Top */
	if ((sRect.height = mttRect->y - (sRect.y = srcRect->y)) > 0) {
		sRect.width = srcRect->width;
		sRect.x = srcRect->x;
		FskRectangleSet(&dRect, dstPt->x, dstPt->y, sRect.width, sRect.height);
		err = FskBitmapDraw(src, &sRect, dst, &dRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
		if (err < 0) goto bail;
	}

	/* Left */
	if ((sRect.width = mttRect->x - (sRect.x = srcRect->x)) > 0) {
		sRect.height = mttRect->height;
		sRect.y = mttRect->y;
		FskRectangleSet(&dRect, dstPt->x , dstPt->y + sRect.y - srcRect->y, sRect.width, sRect.height);
		err = FskBitmapDraw(src, &sRect, dst, &dRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
		if (err < 0) goto bail;
	}

	/* Right */
	if ((sRect.width = (srcRect->x + srcRect->width) - (sRect.x = mttRect->x + mttRect->width)) > 0) {
		sRect.height = mttRect->height;
		sRect.y = mttRect->y;
		FskRectangleSet(&dRect, dstPt->x + sRect.x - srcRect->x, dstPt->y + sRect.y - srcRect->y, sRect.width, sRect.height);
		err = FskBitmapDraw(src, &sRect, dst, &dRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
		if (err < 0) goto bail;
	}

	/* Bottom */
	if ((sRect.height = (srcRect->y + srcRect->height) - (sRect.y = mttRect->y + mttRect->height)) > 0) {
		sRect.width = srcRect->width;
		sRect.x = srcRect->x;
		FskRectangleSet(&dRect, dstPt->x , dstPt->y + sRect.y - srcRect->y, sRect.width, sRect.height);
		err = FskBitmapDraw(src, &sRect, dst, &dRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
		if (err < 0) goto bail;
	}

bail:
	return err;
}
#endif /* 0 */


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Effects									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskEffectBoxBlurApply
 ********************************************************************************/

static FskErr FskEffectBoxBlurApply(FskConstEffectBoxBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: radius=(%.3g, %.3g)", params->radiusX, params->radiusY);
	#endif /* LOG_PARAMETERS */

	return FskBoxBlur(src, srcRect, dst, dstPoint, params->radiusX, params->radiusY);
}


/********************************************************************************
 * FskEffectColorizeApply
 * This only works when src & dst pixel formats are identical.
 ********************************************************************************/

static FskErr FskEffectColorizeApply(FskConstEffectColorize params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	const UInt32		*s;
	UInt32				*d;
	SInt32				srcPixBytes, dstPixBytes, sBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				color, aPos, aMask;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(dst->pixelFormat == src->pixelFormat, err, kFskErrMismatch);									/* Only support same pixel type in src & dst */
	BAIL_IF_FALSE(	32 == src->depth								&&											/* Only support 32 bit pixels */
					8 == FskBitmapFormatRedBits(src->pixelFormat),												/* Only support 8 bit red, green blue */
					err, kFskErrUnsupportedPixelType);

	BAIL_IF_ERR(err = BeginOneSrcBlit(src, srcRect, dst, dstPoint, (const void**)(const void*)&s, &srcPixBytes, &sBump, (void**)(void*)&d, &dstPixBytes, &dBump, &dim));
	FskConvertColorRGBAToBitmapPixel(&params->color, dst->pixelFormat, &color);
	aPos = FskBitmapFormatAlphaPosition(src->pixelFormat);
	aMask = ~(((1 << 8) - 1) << aPos);
	for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump)) {
		for (w = dim.width; w--; ++s, ++d) {
			UInt32	pix		= *s;																				/* Get the source pixel */
			UInt8	alpha	= (UInt8)(pix >> aPos);																/* Save alpha of the src */
			if (!src->alphaIsPremultiplied)	FskBlend32(&pix, color, 						params->color.a);	/* Blend the    straight   colors */
			else							FskBlend32(&pix, FskAlphaScale32(alpha, color), params->color.a);	/* Blend the premultiplied colors */
			*d = (pix & aMask) | (alpha << aPos);																/* Restore alpha into the src. */
		}
	}
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);
bail:
	return err;
}


/********************************************************************************
 * FskEffectColorizeAlphaApply
 ********************************************************************************/

static FskErr FskEffectColorizeAlphaApply(FskConstEffectColorizeAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	const UInt8			*s;
	UInt32				*d;
	SInt32				srcPixBytes, dstPixBytes, sBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				mono[256];

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LogColor(&params->color0, "color0");
		LogColor(&params->color1, "color1");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(	(8 == src->depth || 8 == FskBitmapFormatAlphaBits(src->pixelFormat)) &&				/* Only support sources with 8 bit alpha */
					32 == (dst->depth),																	/* Only support 32 bit pixel destinations */
					err, kFskErrUnsupportedPixelType);

	MakeMonoColorTable(&params->color0, &params->color1, dst->alphaIsPremultiplied, dst->pixelFormat, mono);
	BAIL_IF_ERR(err = BeginOneSrcBlit(src, srcRect, dst, dstPoint, (const void**)(const void*)&s, &srcPixBytes, &sBump, (void**)(void*)&d, &dstPixBytes, &dBump, &dim));
	s += FskBitmapFormatAlphaOffset(src->pixelFormat);
	for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump))
		for (w = dim.width; w--; s += srcPixBytes, ++d)
			*d = mono[*s];
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);
bail:
	return err;
}


/********************************************************************************
 * FskEffectColorizeInnerApply
 ********************************************************************************/

static FskErr FskEffectColorizeInnerApply(FskConstEffectColorizeInner params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	const UInt8			*m;
	const UInt32		*s;
	UInt32				*d;
	SInt32				srcPixBytes, mttPixBytes, dstPixBytes, sBump, mBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				dPix, color, dPosA, sAlf, dAlf, dMskA, dClrA;
	UInt8				opaq;
	void				(*blend)(UInt32 *d, UInt32 p, UInt8 opacity) = (dst->pixelFormat == kFskBitmapFormat32A16RGB565LE) ? FskBlend32A16RGB565SE : FskBlend32;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LogSrcBitmap(params->matte, "matte");
		LogColor(&params->color,    "color");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(	(8 == params->matte->depth || 8 == FskBitmapFormatAlphaBits(params->matte->pixelFormat))	&&	/* Only support mattes with 8 bit alpha */
					dst->pixelFormat == src->pixelFormat														&&	/* Src & dst pixel formats must be identical */
					32 == (dst->depth)																			&&	/* Only support 32 bit pixel destinations */
					(kFskBitmapFormat32A16RGB565LE != dst->pixelFormat || !dst->alphaIsPremultiplied),				/* If 32A16RGB565LE, only with straight alpha */
					err, kFskErrUnsupportedPixelType);

	BAIL_IF_ERR(err = BeginTwoSrcBlit(src, srcRect, params->matte, NULL, dst, dstPoint,
				(const void**)(const void*)&s, &srcPixBytes, &sBump,
				(const void**)(const void*)&m, &mttPixBytes, &mBump,
				(      void**)(      void*)&d, &dstPixBytes, &dBump, &dim));
	FskConvertColorRGBAToBitmapPixel(&params->color, dst->pixelFormat, &color);
	dPosA = FskBitmapFormatAlphaPosition(dst->pixelFormat);
	dMskA = 255 << dPosA;
	dClrA = ~dMskA;
	opaq  = (UInt8)(color >> dPosA);
	m += FskBitmapFormatAlphaOffset(params->matte->pixelFormat);
	for (    h = dim.height; h--; S_INC(s, sBump),       S_INC(m, mBump),       D_INC(d, dBump)) {
		for (w = dim.width;  w--; S_INC(s, srcPixBytes), S_INC(m, mttPixBytes), D_INC(d, dstPixBytes)) {
			if (0 != (dAlf = (dPix = *s) & dMskA)				&&											/* If the dst pixel is not transparent, ... */
				0 != (sAlf = 255U - *m)							&&											/* ... and the complemented matte alpha is not transparent, ... */
				0 != (sAlf = FskAlphaMul((UInt8)sAlf, opaq))												/* ... and the scaled matte alpha is not transparent */
			) {																								/* We need to do some blending */
				if (!src->alphaIsPremultiplied) {
					(*blend)(&dPix, color, (UInt8)sAlf);													/* Blend color */
					dPix &= dClrA; dPix |= dAlf;															/* Restore dst alpha */
				}
				else {
					(*blend)(&dPix, FskAlphaScale32((UInt8)(dAlf >> dPosA), color | dMskA), (UInt8)sAlf);	/* Only for 8888; premultiplied 32A16RGB565 has unacceptable quality */
				}
			}
			*d = dPix;
		}
	}
	FskBitmapReadEnd((FskBitmap)params->matte);
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);
bail:
	return err;
}


/********************************************************************************
 * FskEffectColorizeOuterApply
 ********************************************************************************/

static FskErr FskEffectColorizeOuterApply(FskConstEffectColorizeOuter params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	const UInt8			*m;
	const UInt32		*s;
	UInt32				*d;
	SInt32				srcPixBytes, mttPixBytes, dstPixBytes, sBump, mBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				pix, color, dPosA, sAlf, dMskA, dClrA;
	UInt8				cAlf;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LogSrcBitmap(params->matte, "matte");
		LogColor(&params->color,    "color");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(	32 == (dst->depth)	&&																		/* Only support 32 bit pixel destinations */
					(8 == params->matte->depth || 8 == FskBitmapFormatAlphaBits(params->matte->pixelFormat)),	/* Only support mattes with 8 bit alpha */
					err, kFskErrUnsupportedPixelType);

	BAIL_IF_ERR(err = BeginTwoSrcBlit(src, srcRect, params->matte, NULL, dst, dstPoint,
				(const void**)(const void*)&s, &srcPixBytes, &sBump,
				(const void**)(const void*)&m, &mttPixBytes, &mBump,
				(      void**)(      void*)&d, &dstPixBytes, &dBump, &dim));
	(void)ConvertColorRGBAToBitmapPixel(&params->color, dst->pixelFormat, src->alphaIsPremultiplied, &color);
	dPosA = FskBitmapFormatAlphaPosition(dst->pixelFormat);
	dMskA = 255 << dPosA;
	dClrA = ~dMskA;
	cAlf  = (UInt8)(color >> dPosA);												/* = params->color.alpha */
	m += FskBitmapFormatAlphaOffset(params->matte->pixelFormat);
	if (!src->alphaIsPremultiplied) {
		UInt32 (*over)(UInt32 dst, UInt32 src);
		switch (dst->pixelFormat) {
			case FskName2(kFskBitmapFormat, fsk32RGBAKindFormat):	case FskName2(kFskBitmapFormat, fsk32BGRAKindFormat):	over = FskAlphaStraightOver32XXXA;	break;
			case FskName2(kFskBitmapFormat, fsk32ABGRKindFormat):	case FskName2(kFskBitmapFormat, fsk32ARGBKindFormat):	over = FskAlphaStraightOver32AXXX;	break;
			#if TARGET_RT_LITTLE_ENDIAN
				case kFskBitmapFormat32A16RGB565LE:																	over = FskAlphaStraightOver32A16RGB565SE;	break;
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			default: BAIL(kFskErrUnsupportedPixelType);
		}
		color &= dClrA;
		for (    h = dim.height; h--; S_INC(s, sBump),       S_INC(m, mBump),       D_INC(d, dBump)) {
			for (w = dim.width;  w--; S_INC(s, srcPixBytes), S_INC(m, mttPixBytes), D_INC(d, dstPixBytes)) {
				pix = *s;
				if (0 != (sAlf = *m)						&&						/* If the matte is not transparent, ... */
					0 != (sAlf = FskAlphaMul((UInt8)sAlf, cAlf))					/* ... and the scaled source is transparent ... */
				)
					pix = (*over)(color | (sAlf << dPosA), pix);					/* Replace alpha with scaled alpha and blend behind */
				*d = pix;
			}
		}
	}
	else {
		void (*over)(UInt32 *dst, UInt32 src);
		switch (dst->pixelFormat) {
			case FskName2(kFskBitmapFormat, fsk32RGBAKindFormat):	case FskName2(kFskBitmapFormat, fsk32BGRAKindFormat):	over = FskAlphaBlack32A;	break;
			case FskName2(kFskBitmapFormat, fsk32ABGRKindFormat):	case FskName2(kFskBitmapFormat, fsk32ARGBKindFormat):	over = FskAlphaBlackA32;	break;
			#if TARGET_RT_LITTLE_ENDIAN
				case kFskBitmapFormat32A16RGB565LE:																	over = FskAlphaBlack32A16RGB565SE;	break;
			#endif /* TARGET_RT_LITTLE_ENDIAN */
			default: BAIL(kFskErrUnsupportedPixelType);
		}
		for (    h = dim.height; h--; S_INC(s, sBump),       S_INC(m, mBump),       D_INC(d, dBump)) {
			for (w = dim.width;  w--; S_INC(s, srcPixBytes), S_INC(m, mttPixBytes), D_INC(d, dstPixBytes)) {
				if (0 != (sAlf = *m)		&&										/* If the source is not transparent, ... */
					0 != ((pix = FskAlphaScale32((UInt8)sAlf, color)) & dMskA)		/* ... and the scaled source is not totally transparent, ... */
				) {
					(*over)(&pix, *s);
					*d = pix;
				}
				else
					*d = *s;
			}
		}
	}
	FskBitmapReadEnd((FskBitmap)params->matte);
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);
bail:
	return err;
}


/********************************************************************************
 * FskEffectCompoundApply
 ********************************************************************************/

static FskErr FskEffectCompoundApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err		= kFskErrNone;
	FskBitmap	mid1	= NULL,
				mid2	= NULL;
	FskBitmap	dbm;
	int			numStages;

	#if defined(LOG_PARAMETERS)
		LOGD("%s(effect=%p, src=%p, srcRect=%p, dst=%p, dstPoint=%p)", __FUNCTION__, effect, src, srcRect, dst, dstPoint);
		LogSrcBitmap(src, "src");
		LogRect(srcRect, "srcRect");
		LogDstBitmap(dst, "dst");
		LogPoint(dstPoint, "dstPoint");
		LOGD("\teffectID=%d (%s) topology=%d numStages=%d", effect->effectID, ((effect->effectID == kFskEffectCompound) ? "compound" : "???"),
			effect->params.compound.topology, effect->params.compound.numStages);
		LOGD("{");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NONPOSITIVE(numStages = effect->params.compound.numStages, err, kFskErrEmpty);

	if (1 == numStages)	return FskEffectApply(effect + 1, src, srcRect, dst, dstPoint);		/* No intermediate buffers needed for a single stage */

	if (srcRect == NULL)	srcRect = &src->bounds;											/* Make sure that we know the bounds */
	BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, src->pixelFormat, &mid1));
	mid1->hasAlpha = src->hasAlpha;
	mid1->alphaIsPremultiplied = src->alphaIsPremultiplied;
	if (numStages > 2) {
		BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, src->pixelFormat, &mid2));
		mid2->hasAlpha = src->hasAlpha;
		mid2->alphaIsPremultiplied = src->alphaIsPremultiplied;
	}
	dbm = mid1;																				/* mid1 is the first destination bitmap */

	switch (effect->params.compound.topology) {

		case kFskEffectCompoundTopologyPipeline:
			for (++effect; numStages-- > 1; ++effect, src = dbm, dbm = ((dbm == mid1) ? mid2 : mid1), srcRect = NULL)
				BAIL_IF_ERR(err = FskEffectApply(effect, src, srcRect, dbm, NULL));
			BAIL_IF_ERR(err = FskEffectApply(effect, src, NULL, dst, dstPoint));
			break;

		default:
			BAIL(kFskErrNetworkInterfaceError);
	}
	#if defined(LOG_PARAMETERS)
		LOGD("}");
	#endif /* LOG_PARAMETERS */

bail:
	FskBitmapDispose(mid2);
	FskBitmapDispose(mid1);
	return err;
}


/********************************************************************************
 * FskEffectDilateApply
 ********************************************************************************/

static FskErr FskEffectDilateApply(FskConstEffectDilate params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: radius=%d", params->radius);
	#endif /* LOG_PARAMETERS */

	err = FskDilate(params->radius, src, srcRect, dst, dstPoint);

	return err;
}


/********************************************************************************
 * FskEffectErodeApply
 ********************************************************************************/

static FskErr FskEffectErodeApply(FskConstEffectErode params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: radius=%d", params->radius);
	#endif /* LOG_PARAMETERS */

	err = FskErode(params->radius, src, srcRect, dst, dstPoint);

	return err;
}


/********************************************************************************
 * FskEffectMonochromeApply
 ********************************************************************************/

static FskErr FskEffectMonochromeApply(FskConstEffectMonochrome params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	const UInt32		*s;
	UInt32				*d;
	SInt32				srcPixBytes, dstPixBytes, sBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				rPos, gPos, bPos, aSrc, aDst, aClr, pix;
	UInt32				mono[256];
	UInt8				alpha;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LogColor(&params->color0, "color0");
		LogColor(&params->color1, "color1");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(src->depth == dst->depth, err, kFskErrMismatch);

	MakeMonoColorTable(&params->color0, &params->color1, src->alphaIsPremultiplied, dst->pixelFormat, mono);
	BAIL_IF_ERR(err = BeginOneSrcBlit(src, srcRect, dst, dstPoint,
				(const void**)(const void*)&s, &srcPixBytes, &sBump,
				(      void**)(      void*)&d, &dstPixBytes, &dBump, &dim));

	rPos = FskBitmapFormatRedPosition  (src->pixelFormat);						/* Determine component positions */
	gPos = FskBitmapFormatGreenPosition(src->pixelFormat);
	bPos = FskBitmapFormatBluePosition (src->pixelFormat);
	aSrc = FskBitmapFormatAlphaPosition(src->pixelFormat);
	aDst = FskBitmapFormatAlphaPosition(dst->pixelFormat);
	aClr = ~(0xFF << aDst);

	/* 32RGBA, 32BGRA, 32ABGR, 32ARGB. src and dst can be different pixel types */
	if (32 == src->depth && 8 == FskBitmapFormatRedBits(src->pixelFormat)) {			/* Straight 32RGBA, 32BGRA, 32ABGR, 32ARGB */
		if (!src->alphaIsPremultiplied) {
			for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump)) for (w = dim.width; w--; ++s, ++d) {
				pix = *s;																/* Get the source pixel */
				alpha = (UInt8)(pix >> aSrc);											/* Save src alpha */
				pix = fskRtoYCoeff * ((pix >> rPos) & 0xFF)								/* Convert to gray with fskToYCoeffShift extra bits */
					+ fskGtoYCoeff * ((pix >> gPos) & 0xFF)
					+ fskBtoYCoeff * ((pix >> bPos) & 0xFF);
				pix += 1 << (fskToYCoeffShift - 1); pix >>= fskToYCoeffShift;			/* Round and reduce down to 8 bits */
				pix = mono[pix];														/* Interpolate between color0 and color1 */
				alpha = FskAlphaMul(alpha, (UInt8)(pix >> aDst));						/* Compute alpha */
				pix = (pix & aClr) | (alpha << aDst);									/* Replace alpha */
				*d = pix;
			}
			goto done;
		}
		else if (8 == FskBitmapFormatRedBits(dst->pixelFormat)) {						/* Premultiplied 32RGBA, 32BGRA, 32ABGR, 32ARGB */
			for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump)) for (w = dim.width; w--; ++s, ++d) {
				pix = *s;																/* Get the source pixel */
				alpha = (UInt8)(pix >> aSrc);											/* Save src alpha */
				if (alpha != 0) {
					pix = fskRtoYCoeff * ((pix >> rPos) & 0xFF)							/* Convert to gray with fskToYCoeffShift extra bits */
						+ fskGtoYCoeff * ((pix >> gPos) & 0xFF)
						+ fskBtoYCoeff * ((pix >> bPos) & 0xFF);
					if (alpha == 255U) {												/* If source alpha is 255, then we just grab values from the table */
						pix += 1 << (fskToYCoeffShift - 1);	pix >>= fskToYCoeffShift;	/* Round and reduce down to 8 bits */
						pix = mono[pix];												/* Interpolate between color0 and color1. */
					}
					else {	/* 0 < alpha < 255 */										/* We need to convert to straight alpha, lookup, then premultiply */
						pix = (pix * 255 + (alpha >> 1)) / alpha;						/* Convert to straight gray if the src was premultiplied, taking advantage of extra precision */
						if (pix > (255 << fskToYCoeffShift))							/* Saturate */
							pix = (255 << fskToYCoeffShift);
						pix += 1 << (fskToYCoeffShift - 1);	pix >>= fskToYCoeffShift;	/* Round and reduce down to 8 bit grayscale */
						pix = FskAlphaScale32(alpha, mono[pix]);						/* Interpolate between color0 and color1 and scale everything by src alpha */
					}
				}
				/* if alpha == 0, then pix == 0 since it is premultiplied */
				*d = pix;
			}
			goto done;
		}
	}

	/* 32A16RGB565SE */
	if (FskName2(kFskBitmapFormat,fsk32A16RGB565SEKindFormat) == src->pixelFormat && src->pixelFormat == dst->pixelFormat) {
		for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump)) for (w = dim.width; w--; ++s, ++d) {
			pix = *s;																/* Get the source pixel */
			alpha = (UInt8)(pix >> aSrc);											/* Save src alpha */
			fskConvert32A16RGB565SE8G(pix);											/* Convert to gray */
			if (src->alphaIsPremultiplied) {										/* If the source is premultiplied by alpha, ... */
				if (alpha) {
					pix = (pix * 255 + (alpha >> 1)) / alpha;						/* ... convert to straight alpha -- really crufty */
					if (pix > 255)
						pix = 255;
				}
			}
			pix = mono[pix];														/* Convert to bichromatic */
			alpha = FskAlphaMul(alpha, (UInt8)(pix >> aDst));						/* Compute alpha */
			pix = (pix & aClr) | (alpha << aDst);									/* Replace alpha */
			if (!dst->alphaIsPremultiplied) {
				*d = pix;
			} else {
				*d = 0;
				FskAlpha32A16RGB565SE(d, pix);										/* Convert to premultiplied */
			}
		}
		goto done;
	}

	/* General case */
	for (h = dim.height; h--; S_INC(s, sBump), D_INC(d, dBump)) for (w = dim.width; w--; S_INC(s, srcPixBytes), D_INC(d, dstPixBytes)) {
		union ColorInt color;
		FskConvertBitmapPixelToColorRGBA(s, src->pixelFormat, &color.c);
		pix = color.i;
		FskName3(fskConvert,fsk32RGBAFormatKind,8G)(pix);
		FskMemCopy(d, (char*)mono + pix * dstPixBytes, dstPixBytes);
	}


done:
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);

bail:
	return err;
}


/********************************************************************************
 * FskEffectMaskApply
 ********************************************************************************/

static FskErr FskEffectMaskApply(FskConstEffectMask params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskBitmap			tmp		= NULL;
	FskConstBitmap		msk;
	FskRectangleRecord	mskRect;
	const UInt32		*s;
	const UInt8			*m;
	UInt32				*d, aLoc, aClr;
	SInt32				srcPixBytes, mskPixBytes, dstPixBytes, sBump, mBump, dBump, w, h;
	FskDimensionRecord	dim;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: mask=%p", params->mask);
		LogSrcBitmap(params->mask, "mask");
		LogRect(&params->maskRect, "maskRect");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(	(8 == params->mask->depth || 8 == FskBitmapFormatAlphaBits(params->mask->pixelFormat))	&&	/* Only support 8 bit alpha or grayscale mask */
					src->pixelFormat == dst->pixelFormat													&&	/* The src and dst must have the same pixel format */
					32 == dst->depth, err, kFskErrUnsupportedPixelType);										/* The dst must be 32 bits */

	if (srcRect == NULL)
		srcRect = &src->bounds;
	msk     = params->mask;
	mskRect = params->maskRect;
	if (srcRect->width != mskRect.width || srcRect->height != mskRect.height) {									/* If src & msk are not the same size, ... */
		BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, msk->pixelFormat, &tmp));				/* ... allocate a tmp buffer, ... */
		#if CAN_SCALE_8G
			BAIL_IF_ERR(err = FskBitmapDraw(msk, &mskRect, tmp, NULL, NULL, NULL, kFskGraphicsModeCopy|kFskGraphicsModeBilinear, NULL));	/* ... and scale the mask anisotropically */
		#else /* !CAN_SCALE_8G */
			BAIL_IF_ERR(err = StretchAlpha(msk, &mskRect, tmp, NULL));
		#endif /* !CAN_SCALE_8G */
		msk = tmp;																								/* use the tmp buffer as the mask */
		mskRect = tmp->bounds;
	}

	aLoc = FskBitmapFormatAlphaPosition(dst->pixelFormat);
	aClr = ~(255 << aLoc);

	BAIL_IF_ERR(err = BeginTwoSrcBlit(src, srcRect, msk, /*(FskConstPoint)*/(const void*)(&mskRect), dst, dstPoint,
				(const void**)(const void*)&s, &srcPixBytes, &sBump,
				(const void**)(const void*)&m, &mskPixBytes, &mBump,
				(      void**)(      void*)&d, &dstPixBytes, &dBump, &dim));
	m += FskBitmapFormatAlphaOffset(msk->pixelFormat);

	dst->hasAlpha = 1;
	if (!dst->alphaIsPremultiplied) {
		for (    h = dim.height; h--; S_INC(s, sBump),       S_INC(m, mBump),       D_INC(d, dBump))
			for (w = dim.width;  w--; S_INC(s, srcPixBytes), S_INC(m, mskPixBytes), D_INC(d, dstPixBytes))
				*d = (*s & aClr) | ((UInt32)(*m) << aLoc);														/* Replace src alpha with mask alpha */
	}
	else {
		for (    h = dim.height; h--; S_INC(s, sBump),       S_INC(m, mBump),       D_INC(d, dBump))
			for (w = dim.width;  w--; S_INC(s, srcPixBytes), S_INC(m, mskPixBytes), D_INC(d, dstPixBytes))
				*d = FskAlphaScale32(*m, *s);																	/* Scale src with mask alpha */
	}
	FskBitmapReadEnd((FskBitmap)msk);
	FskBitmapReadEnd((FskBitmap)src);
	FskBitmapWriteEnd(          dst);
	FskBitmapDispose(tmp);

bail:
	return err;
}


/********************************************************************************
 * FskEffectShadeApply
 ********************************************************************************/

static FskErr FskEffectShadeApply(FskConstEffectShade params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskBitmap			tmp		= NULL;
	FskConstBitmap		shd;
	FskRectangleRecord	shdRect;
	const UInt32		*s, *z;
	UInt32				*d;
	SInt32				srcPixBytes, zhdPixBytes, dstPixBytes, sBump, zBump, dBump, w, h;
	FskDimensionRecord	dim;
	UInt32				alpha, aPos, aMsk, aClr, zPix, dPix, dAlf;
	void				(*blend)(UInt32 *d, UInt32 p, UInt8 opacity) = (dst->pixelFormat == kFskBitmapFormat32A16RGB565LE) ? FskBlend32A16RGB565SE : FskBlend32;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: shadow=%p", params->shadow);
		LogSrcBitmap(params->shadow, "shadow");
		LogRect(&params->shadowRect, "shadowRect");
	#endif /* LOG_PARAMETERS */

	if (srcRect == NULL)
		srcRect = &src->bounds;
	shd     = params->shadow;
	shdRect = params->shadowRect;
	if (srcRect->width != shdRect.width || srcRect->height != shdRect.height ||						/* If src & shd are not the same size, ... */
		(shd->hasAlpha && src->alphaIsPremultiplied != shd->alphaIsPremultiplied)					/* ... or their type of alpha is not compatible, ... */
	) {
		BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, shd->pixelFormat, &tmp));	/* ... allocate a tmp buffer, ... */
		err = FskBitmapDraw(shd, &shdRect, tmp, NULL, NULL, NULL, kFskGraphicsModeCopy|kFskGraphicsModeBilinear, NULL);/* ... and scale the mask anisotropically */
        tmp->hasAlpha = shd->hasAlpha;																/* Make sure that the copy has the same alpha ... */
        tmp->alphaIsPremultiplied = shd->alphaIsPremultiplied;										/* ... properties as the shd ... */
        if (src->alphaIsPremultiplied && !tmp->alphaIsPremultiplied)								/* ... and make consistent ... */
            FskStraightAlphaToPremultipliedAlpha(tmp, NULL);										/* ... with the src */
		shd = tmp;																					/* use the tmp buffer as the shade */
		shdRect = tmp->bounds;
	}

	BAIL_IF_FALSE(	32 == params->shadow->depth						&&
					dst->pixelFormat ==            src->pixelFormat &&
					dst->pixelFormat == params->shadow->pixelFormat,
					err, kFskErrUnsupportedPixelType);

	if (params->opacity) {
		BAIL_IF_ERR(err = BeginTwoSrcBlit(src, srcRect, shd, /*(FskConstPoint)*/(const void*)(&shdRect), dst, dstPoint,
					(const void**)(const void*)&s, &srcPixBytes, &sBump,
					(const void**)(const void*)&z, &zhdPixBytes, &zBump,
					(      void**)(      void*)&d, &dstPixBytes, &dBump, &dim));
		aPos = FskBitmapFormatAlphaPosition(dst->pixelFormat);
		aMsk = 255U << aPos;
		aClr = ~aMsk;
		for (h = dim.height; h--; S_INC(s, sBump), S_INC(z, zBump), D_INC(d, dBump)) for (w = dim.width; w--; ++s, ++z, ++d) {
			zPix = *z;																					/* Get shade  pixel */
			dPix = *s;																					/* Get source pixel */
			ALPHAMUL(alpha, (zPix >> aPos) & 0xFF, params->opacity);									/* Scale shadow alpha by the overall transparency */
			if (0 != alpha) {
				dAlf = dPix & aMsk;																		/* Save destination alpha */
				if (!dst->alphaIsPremultiplied)															/* Cd = Cd + (Ao * As) * (Cs - Cd); Ad = Ad; */
					(*blend)(&dPix, zPix, (UInt8)alpha);												/* Scale shadow with mask alpha */
				else																					/* Cd = (1 - Ao * As) * Cd + Ad * Ao * Cs; Ad = Ad; */
					dPix = FskAXPY8888((UInt8)(255 - alpha), dPix, FskAlphaScale32(FskAlphaMul((UInt8)(dPix >> aPos), params->opacity), zPix));
				dPix &= aClr;	dPix |= dAlf;															/* Restore destination alpha */
			}
			*d = dPix;
		}

		FskBitmapReadEnd((FskBitmap)shd);
		FskBitmapReadEnd((FskBitmap)src);
		FskBitmapWriteEnd(          dst);
	}
	else {
		FskRectangleRecord dstRect;
		if (!dstPoint)	{ dstRect.x = dst->bounds.x; dstRect.y = dst->bounds.y; }
		else			{ dstRect.x = dstPoint->x;   dstRect.y = dstPoint->y;   }
		dstRect.width  = srcRect->width;
		dstRect.height = srcRect->height;
		err = FskBitmapDraw(src, srcRect, dst, &dstRect, NULL, NULL, kFskGraphicsModeCopy, NULL);
	}

bail:
	FskBitmapDispose(tmp);
	return err;
}


/********************************************************************************
 * FskEffectGaussianBlurApply
 ********************************************************************************/

static FskErr FskEffectGaussianBlurApply(FskConstEffectGaussianBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: sigma=(%.3g, %.3g)", params->sigmaX, params->sigmaY);
	#endif /* LOG_PARAMETERS */

	return FskBlur(src, srcRect, dst, dstPoint, params->sigmaX, params->sigmaY);
}


/********************************************************************************
 * FskEffectInnerGlowApply
 ********************************************************************************/

static FskErr FskEffectInnerGlowApply(FskConstEffectInnerGlow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err		= kFskErrNone;
	FskBitmap	matte	= NULL;
	FskEffectColorizeInnerRecord	innerParams;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: radius=%d, blurSigma=%.3g", params->radius, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect = &src->bounds;
	BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, kFskBitmapFormat8G, &matte));		/* Does this need to be bigger? */
	BAIL_IF_ERR(err = FskErode(params->radius, src, srcRect, matte, NULL));
	BAIL_IF_ERR(err = FskGaussianBlur(matte, NULL, matte, NULL, params->blurSigma, params->blurSigma));
	innerParams.matte = matte; innerParams.color = params->color;										/* Set parameters for Colorize Inner Effect */
	BAIL_IF_ERR(err = FskEffectColorizeInnerApply(&innerParams, src, srcRect, dst, dstPoint));

bail:
	FskBitmapDispose(matte);
	return err;
}


/********************************************************************************
 * FskEffectInnerShadowApply
 ********************************************************************************/

static FskErr FskEffectInnerShadowApply(FskConstEffectInnerShadow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err			= kFskErrUnimplemented;
	FskBitmap	matte	= NULL;
	FskEffectColorizeInnerRecord	innerParams;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: offset=(%d, %d), blurSigma=%.3g", params->offset.x, params->offset.y, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect = &src->bounds;
	BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, kFskBitmapFormat8G, &matte));		/* Does this need to be bigger? */
	BAIL_IF_ERR(err = CopyAlpha(src, srcRect, matte, &params->offset));
	BAIL_IF_ERR(err = FskGaussianBlur(matte, NULL, matte, NULL, params->blurSigma, params->blurSigma));
	innerParams.matte = matte; innerParams.color = params->color;										/* Set parameters for Colorize Inner Effect */
	BAIL_IF_ERR(err = FskEffectColorizeInnerApply(&innerParams, src, srcRect, dst, dstPoint));

bail:
	FskBitmapDispose(matte);
	return err;
}


/********************************************************************************
 * FskEffectOuterGlowApply
 ********************************************************************************/

static FskErr FskEffectOuterGlowApply(FskConstEffectOuterGlow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err		= kFskErrNone;
	FskBitmap	matte	= NULL;
	FskEffectColorizeOuterRecord outerParams;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: radius=%d, blurSigma=%.3g", params->radius, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect = &src->bounds;
	BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, kFskBitmapFormat8G, &matte));		/* Does this need to be bigger? */
	BAIL_IF_ERR(err = FskDilate(params->radius, src, srcRect, matte, NULL));
	BAIL_IF_ERR(err = FskGaussianBlur(matte, NULL, matte, NULL, params->blurSigma, params->blurSigma));
	outerParams.matte = matte; outerParams.color = params->color;										/* Set parameters for Colorize Outer Effect */
	BAIL_IF_ERR(err = FskEffectColorizeOuterApply(&outerParams, src, srcRect, dst, dstPoint));

bail:
	FskBitmapDispose(matte);
	return err;
}


/********************************************************************************
 * FskEffectOuterShadowApply
 ********************************************************************************/

static FskErr FskEffectOuterShadowApply(FskConstEffectOuterShadow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr		err		= kFskErrNone;
	FskBitmap	matte	= NULL;
	FskEffectColorizeOuterRecord outerParams;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tparams: offset=(%d, %d), blurSigma=%.3g", params->offset.x, params->offset.y, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect = &src->bounds;
	BAIL_IF_ERR(err = FskBitmapNew(srcRect->width, srcRect->height, kFskBitmapFormat8G, &matte));		/* Does this need to be bigger? */
	BAIL_IF_ERR(err = CopyAlpha(src, srcRect, matte, &params->offset));
	BAIL_IF_ERR(err = FskGaussianBlur(matte, NULL, matte, NULL, params->blurSigma, params->blurSigma));
	outerParams.matte = matte; outerParams.color = params->color;										/* Set parameters for Colorize Outer Effect */
	BAIL_IF_ERR(err = FskEffectColorizeOuterApply(&outerParams, src, srcRect, dst, dstPoint));

bail:
	FskBitmapDispose(matte);
	return err;
}


/********************************************************************************
 * FskEffectPremultiplyAlphaApply
 ********************************************************************************/

static FskErr FskEffectPremultiplyAlphaApply(FskConstEffectPremultiplyAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	dstRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect  = &src->bounds;
	if (!dstPoint)	dstPoint = (const void*)(&dst->bounds);
	dstRect.x		= dstPoint->x;
	dstRect.y		= dstPoint->y;
	dstRect.width	= srcRect->width;
	dstRect.height	= srcRect->height;

	dst->hasAlpha = src->hasAlpha;
	dst->alphaIsPremultiplied = src->alphaIsPremultiplied;
	BAIL_IF_ERR(err = FskBitmapDraw(src, srcRect, dst, &dstRect, NULL, NULL, kFskGraphicsModeCopy, NULL));
	BAIL_IF_ERR(err = FskStraightAlphaToPremultipliedAlpha(dst, &dstRect));

bail:
	return err;
}


/********************************************************************************
 * FskEffectStraightAlphaApply
 ********************************************************************************/

static FskErr FskEffectStraightAlphaApply(FskConstEffectStraightAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	dstRect;
	FskColorRGBRecord	bgColor;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint);
		LOGD("\tcolor(%3u, %3u, %3u)", params->color.r, params->color.g, params->color.b);
	#endif /* LOG_PARAMETERS */

	if (!srcRect)	srcRect  = &src->bounds;
	if (!dstPoint)	dstPoint = (const void*)(&dst->bounds);
	dstRect.x		= dstPoint->x;
	dstRect.y		= dstPoint->y;
	dstRect.width	= srcRect->width;
	dstRect.height	= srcRect->height;
	FskColorRGBSet(&bgColor, params->color.r, params->color.g, params->color.b);

	dst->hasAlpha = src->hasAlpha;
	dst->alphaIsPremultiplied = src->alphaIsPremultiplied;
	BAIL_IF_ERR(err = FskBitmapDraw(src, srcRect, dst, &dstRect, NULL, NULL, kFskGraphicsModeCopy, NULL));
	BAIL_IF_ERR(err = FskPremultipliedAlphaToStraightAlpha(dst, &bgColor, &dstRect));

bail:
	return err;
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/********************************************************************************
 * FskEffectApply
 ********************************************************************************/

FskErr FskEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr err;
	switch (effect->effectID) {
		case kFskEffectBoxBlur:				err = FskEffectBoxBlurApply(			&effect->params.boxBlur,			src, srcRect, dst, dstPoint);	break;
		case kFskEffectColorize:			err = FskEffectColorizeApply(			&effect->params.colorize,			src, srcRect, dst, dstPoint);	break;
		case kFskEffectColorizeAlpha:		err = FskEffectColorizeAlphaApply(		&effect->params.colorizeAlpha,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectColorizeInner:		err = FskEffectColorizeInnerApply(		&effect->params.colorizeInner,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectColorizeOuter:		err = FskEffectColorizeOuterApply(		&effect->params.colorizeOuter,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectCompound:			err = FskEffectCompoundApply(			effect,								src, srcRect, dst, dstPoint);	break;
		case kFskEffectDilate:				err = FskEffectDilateApply(				&effect->params.dilate,				src, srcRect, dst, dstPoint);	break;
		case kFskEffectErode:				err = FskEffectErodeApply(				&effect->params.erode,				src, srcRect, dst, dstPoint);	break;
		case kFskEffectGaussianBlur:		err = FskEffectGaussianBlurApply(		&effect->params.gaussianBlur,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectInnerGlow:			err = FskEffectInnerGlowApply(			&effect->params.innerGlow,			src, srcRect, dst, dstPoint);	break;
		case kFskEffectInnerShadow:			err = FskEffectInnerShadowApply(		&effect->params.innerShadow,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectMask:				err = FskEffectMaskApply(				&effect->params.mask,				src, srcRect, dst, dstPoint);	break;
		case kFskEffectMonochrome:			err = FskEffectMonochromeApply(			&effect->params.monochrome,			src, srcRect, dst, dstPoint);	break;
		case kFskEffectOuterGlow:			err = FskEffectOuterGlowApply(			&effect->params.outerGlow,			src, srcRect, dst, dstPoint);	break;
		case kFskEffectOuterShadow:			err = FskEffectOuterShadowApply(		&effect->params.outerShadow,		src, srcRect, dst, dstPoint);	break;
		case kFskEffectPremultiplyAlpha:	err = FskEffectPremultiplyAlphaApply(	&effect->params.premultiplyAlpha,	src, srcRect, dst, dstPoint);	break;
		case kFskEffectShade:				err = FskEffectShadeApply(				&effect->params.shade,				src, srcRect, dst, dstPoint);	break;
		case kFskEffectStraightAlpha:		err = FskEffectStraightAlphaApply(		&effect->params.straightAlpha,		src, srcRect, dst, dstPoint);	break;

		#if 0
			case kFskEffectCopy:
			case kFskEffectCopyMirrorBorders:
			case kFskEffectDirectionalBoxBlur:
			case kFskEffectDirectionalDilate:
			case kFskEffectDirectionalErode:
			case kFskEffectDirectionalGaussianBlur:
		#endif /* 0 */
		default:
			err = kFskErrUnimplemented;	break;
	}
	return err;
}
