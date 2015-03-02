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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskRotate90.h"
#include "FskPlatform.h"
#include "FskPixelOps.h"


#define FIXED_BITS		16						/**< The number of fractional bits in a standard 16.16 fixed point number. */
#define FWD_BITS		18						/**< The number of fractional bits used for forward differencing. */
#define LOG_LERP_BITS	2						/**< The base-2 logarithm of the number of bits used for linear interpolation. */
#define FIXED_ONE		(1 <<  FIXED_BITS)		/**< One,      as represented in standard 16.16 fixed-point notation. */
#define FIXED_HALF		(1 << (FIXED_BITS - 1))	/**< One half, as represented in standard 16.16 fixed-point notation. */
#define FWD_ONE			(1 << FWD_BITS)			/**< One,      as used in the forward differencing calculations. */
#define FWD_HALF		(1 << (FWD_BITS - 1))	/**< One half, as used in the forward differencing calculations. */
#define LERP_BITS		(1 << LOG_LERP_BITS)	/**< The number of bits (4) used for linear interpolation. */

#if FWD_BITS < kFskOffsetBits
	#error FWD_BITS < kFskOffsetBits
#endif /* FWD_BITS < kFskOffsetBits */


/** Blit proc for rotation. */
typedef void (*FskRotate90Proc)(
	FskFixed	u0,
	FskFixed	v0,
	FskFixed	du,
	FskFixed	dv,
	const void	*srcBaseAddr,
	SInt32		srcRowBytes,
	void		*dstBaseAddr,
	SInt32		dstRowBytes,
	UInt32		dstWidth,
	UInt32		dstHeight
);

#if defined(__MWERKS__)
	#define COMPILER_CAN_GENERATE_CONST_PROC_POINTERS	1
#else
	#define COMPILER_CAN_GENERATE_CONST_PROC_POINTERS	0
#endif

#define GENERAL_BLIT_PROTO_FILE "FskRotate90.proto.c"
#include "FskBlitSrcDst.h"

#if !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS
	#include "FskRotate90Null.h"
#endif /* !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */


/********************************************************************************
 * Dispatch table: dst, src, op, qual
 ********************************************************************************/

#define NUM_QUALS	2
#define R_DS(d,s)	{ FskName4(FskRotate90P,s,d,Q0),FskName4(FskRotate90P,s,d,Q1) }

#if	  NUM_SRC_FORMATS == 1
	#define R_D(d)		{ R_DS(d,SRC_KIND_0) }
#elif NUM_SRC_FORMATS == 2
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1) }
#elif NUM_SRC_FORMATS == 3
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2) }
#elif NUM_SRC_FORMATS == 4
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3) }
#elif NUM_SRC_FORMATS == 5
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4) }
#elif NUM_SRC_FORMATS == 6
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5) }
#elif NUM_SRC_FORMATS == 7
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6) }
#elif NUM_SRC_FORMATS == 8
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7) }
#elif NUM_SRC_FORMATS == 9
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8) }
#elif NUM_SRC_FORMATS == 10
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9) }
#elif NUM_SRC_FORMATS == 11
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10) }
#elif NUM_SRC_FORMATS == 12
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11) }
#elif NUM_SRC_FORMATS == 13
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12) }
#elif NUM_SRC_FORMATS == 14
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13) }
#elif NUM_SRC_FORMATS == 15
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14) }
#elif NUM_SRC_FORMATS == 16
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14),R_DS(d,SRC_KIND_15) }
#elif NUM_SRC_FORMATS == 17
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14),R_DS(d,SRC_KIND_15),R_DS(d,SRC_KIND_16) }
#elif NUM_SRC_FORMATS == 18
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14),R_DS(d,SRC_KIND_15),R_DS(d,SRC_KIND_16),R_DS(d,SRC_KIND_17) }
#elif NUM_SRC_FORMATS == 19
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14),R_DS(d,SRC_KIND_15),R_DS(d,SRC_KIND_16),R_DS(d,SRC_KIND_17),R_DS(d,SRC_KIND_18) }
#elif NUM_SRC_FORMATS == 20
	#define R_D(d)		{ R_DS(d,SRC_KIND_0),R_DS(d,SRC_KIND_1),R_DS(d,SRC_KIND_2),R_DS(d,SRC_KIND_3),R_DS(d,SRC_KIND_4),R_DS(d,SRC_KIND_5),R_DS(d,SRC_KIND_6),R_DS(d,SRC_KIND_7),R_DS(d,SRC_KIND_8),R_DS(d,SRC_KIND_9),R_DS(d,SRC_KIND_10),R_DS(d,SRC_KIND_11),R_DS(d,SRC_KIND_12),R_DS(d,SRC_KIND_13),R_DS(d,SRC_KIND_14),R_DS(d,SRC_KIND_15),R_DS(d,SRC_KIND_16),R_DS(d,SRC_KIND_17),R_DS(d,SRC_KIND_18),R_DS(d,SRC_KIND_19) }
#else
	#error Unexpected large number of source pixel formats
#endif /* NUM_SRC_FORMATS */

static FskRotate90Proc	rotDispatch[NUM_DST_FORMATS][NUM_SRC_FORMATS][NUM_QUALS] = {
		R_D(DST_KIND_0)
	#if NUM_DST_FORMATS > 1
		,R_D(DST_KIND_1)
	#endif
	#if NUM_DST_FORMATS > 2
		,R_D(DST_KIND_2)
	#endif
	#if NUM_DST_FORMATS > 3
		,R_D(DST_KIND_3)
	#endif
	#if NUM_DST_FORMATS > 4
		,R_D(DST_KIND_4)
	#endif
	#if NUM_DST_FORMATS > 5
		,R_D(DST_KIND_5)
	#endif
	#if NUM_DST_FORMATS > 6
		,R_D(DST_KIND_6)
	#endif
	#if NUM_DST_FORMATS > 7
		,R_D(DST_KIND_7)
	#endif
	#if NUM_DST_FORMATS > 8
		,R_D(DST_KIND_8)
	#endif
	#if NUM_DST_FORMATS > 9
		,R_D(DST_KIND_9)
	#endif
	#if NUM_DST_FORMATS > 10
		,R_D(DST_KIND_10)
	#endif
	#if NUM_DST_FORMATS > 11
		,R_D(DST_KIND_11)
	#endif
	#if NUM_DST_FORMATS > 12
		,R_D(DST_KIND_12)
	#endif
	#if NUM_DST_FORMATS > 13
		,R_D(DST_KIND_13)
	#endif
	#if NUM_DST_FORMATS > 14
		,R_D(DST_KIND_14)
	#endif
	#if NUM_DST_FORMATS > 15
		,R_D(DST_KIND_15)
	#endif
	#if NUM_DST_FORMATS > 16
		,R_D(DST_KIND_16)
	#endif
	#if NUM_DST_FORMATS > 17
		#error Unexpected large number of destination pixel formats
	#endif
 };


/********************************************************************************
 * PixelFormatToProcTableSrcIndex
 ********************************************************************************/

static SInt32
PixelFormatToProcTableSrcIndex(UInt32 pixelFormat)
{
	return (pixelFormat < sizeof(srcPixelFormatToPixelKindIndex) / sizeof(srcPixelFormatToPixelKindIndex[0])) ?
			srcPixelFormatToPixelKindIndex[pixelFormat] :
			-1;
}


/********************************************************************************
 * PixelFormatToProcTableDstIndex
 ********************************************************************************/

static SInt32
PixelFormatToProcTableDstIndex(UInt32 pixelFormat)
{
	return (pixelFormat < sizeof(dstPixelFormatToPixelKindIndex) / sizeof(dstPixelFormatToPixelKindIndex[0])) ?
			dstPixelFormatToPixelKindIndex[pixelFormat] :
			-1;
}


/********************************************************************************
 * FskRotate90_wmmx
 * TODO: Does anybody know what the implementation restrictions are for this function?
 * It doesn't work for 32-bit pixels.
 ********************************************************************************/
#ifdef SUPPORT_WMMX
#ifdef MARVELL_SOC_PXA168
#define LOOPINNERCOUNT 16
extern void RotatePixel256_arm_wmmx_s(UInt16 *dd,const UInt16 *s,UInt32 ddinc,UInt32 ssinc);
FskErr	FskRotate90_wmmx(FskBitmap 	srcBM,
			FskBitmap	dstBM,
			UInt32		dstWidth,//1280/320
			UInt32		dstHeight)//720//240
{
	FskErr err = kFskErrNone;
	UInt16 *pr = (UInt16 *)srcBM->bits;
	UInt16 *d = (UInt16 *)dstBM->bits;
	SInt32 u0 = dstHeight - 1;
	SInt32 v0 = 0;
	SInt32 dv = 1;
	SInt32 dstBump = LOOPINNERCOUNT - dstWidth*dstHeight;
	SInt32 u,v,w,h;
	for (w = dstWidth, v = v0; w > 0; v += (dv << 4), d = d + dstBump, w -= LOOPINNERCOUNT)
	{
		for (h = dstHeight, u = u0; h>0; h-=16)
		{
			UInt16 *dd = d;
			const UInt16 *sc = (const UInt16 *)(pr) + u;
			const UInt16 *s = (const UInt16 *)(sc + v * dstHeight);
			RotatePixel256_arm_wmmx_s(dd,s,(dstWidth<<1),(dstHeight<<1));
			u-=16;
			d = d + (dstWidth<<4);
		}
	}
	return err;

}
#endif /* MARVELL_SOC_PXA168 */
#endif /* SUPPORT_WMMX */


/********************************************************************************
 * FskRotate90
 * srcClipRect is transformed by scaleOffset to dstBM->bounds and then clipped by dstClipRect
 * The transformation applied is this:
 *	dstX = scaleOffset->scaleX * srcY + scaleOffset->offsetX;
 *	dstY = scaleOffset->scaleY * srcX + scaleOffset->offsetY;
 * where (srcX, srcY) are specified relative to srcClipRect,
 * and (dstX, dstY) are specified relative to dstBM->bounds.
 * dstClipRect is intended to be used for window updates
 * when only part of the window needs refreshing.
 * Either scaleOffset->scaleX or scaleOffset->scaleY should be negative
 * in order to produce an image rotation; otherwise a reflection will occur.
 ********************************************************************************/

FskErr
FskRotate90(
	FskBitmap				srcBM,			/* The source image */
	FskConstRectangle		srcClipRect,	/* This can be NULL, this effectively being srcBM->bounds */
	FskBitmap				dstBM,			/* The destination image */
	FskConstRectangle		dstClipRect,	/* This can be NULL, thus effectively being dstBM->bounds */
	const FskScaleOffset	*scaleOffset,	/* The scale and offset are specified relative to dstBM->bounds */
	UInt32					quality			/* 0 -> point-sampling, 1 -> bilinear interpolation */
) {
	FskFixed	u0, v0, du, dv;
	const void	*srcBaseAddr;
	void		*dstBaseAddr;
	SInt32		dstWidth, dstHeight;
	int			srcIndex, dstIndex;
	FskErr		err = kFskErrNone;
	SInt32		i;
	SInt32		dstX0, dstX1, dstY0, dstY1;
	FskRotate90Proc blitProc;
	FskRectangleRecord srcClip, dstClip;

#ifdef SUPPORT_WMMX
#ifdef MARVELL_SOC_PXA168
	if (srcBM->depth				== 16					&&
		quality						== 0					&&
		srcBM->pixelFormat			== dstBM->pixelFormat	&&
		(srcBM->bounds.width << 1)	== srcBM->rowBytes		&&
		(dstBM->bounds.width << 1)	== dstBM->rowBytes		&&
		(dstBM->bounds.width  & 15)	== 0					&&
		(dstBM->bounds.height & 15)	== 0
	)
		return FskRotate90_wmmx( srcBM, dstBM, dstBM->bounds.width, dstBM->bounds.height );
#endif /* MARVELL_SOC_PXA168 */
#endif /* SUPPORT_WMMX */

	/* Set defaults if srcClipRect or dstClipRect are NULL */
	if (srcClipRect == NULL) { srcClip = srcBM->bounds; srcClipRect = &srcBM->bounds;        }
	else					 { FskRectangleIntersect(srcClipRect, &srcBM->bounds, &srcClip); }
	if (dstClipRect == NULL) { dstClip = dstBM->bounds;                                      }
	else					 { FskRectangleIntersect(dstClipRect, &dstBM->bounds, &dstClip); }

	BAIL_IF_NULL(scaleOffset, err, kFskErrInvalidParameter);

	/* Determine the transformed src */
	dstX0 = scaleOffset->offsetX << (FWD_BITS - kFskOffsetBits);									/* X0 is the first coordinate */
	#if kFskScaleBits >= FWD_BITS
		dstX1 = FskFixedNMul(srcClip.height-1, scaleOffset->scaleX, kFskScaleBits-FWD_BITS) + dstX0;/* X1 is here the scaled, offset height-1 */
	#else /* kFskScaleBits < FWD_BITS */
		dstX1 = ((srcClip.height-1) << (FWD_BITS - kFskScaleBits)) * scaleOffset->scaleX + dstX0;	/* X1 is here the scaled, offset height-1 */
	#endif /* kFskScaleBits < FWD_BITS */
	if (scaleOffset->scaleX >= 0)																	/* If the scale is positive, ... */
		dstX1 += FWD_ONE;																			/* ... add one to get the rightmost coordinate */
	else																							/* If the scale is negative, ... */
		dstX1 -= FWD_ONE;																			/* ... subtract one to get the leftmost coordinate */
	if (dstX0 > dstX1) {																			/* If X0 is to the right of X1, ... */
		i = dstX0;
		dstX0 = dstX1;																				/* ... swap them, so X0 <= X1 */
		dstX1 = i;
	}

	dstY0 = scaleOffset->offsetY << (FWD_BITS - kFskOffsetBits);									/* Y0 is the first coordinate */
	#if kFskScaleBits >= FWD_BITS
		dstY1 = FskFixedNMul(srcClip.width-1, scaleOffset->scaleY, kFskScaleBits-FWD_BITS) + dstY0;	/* Y1 is here the scaled, offset width-1 */
	#else /* kFskScaleBits < FWD_BITS */
		dstY1 = ((srcClip.width-1) << (FWD_BITS - kFskScaleBits)) * scaleOffset->scaleY + dstX0;	/* Y1 is here the scaled, offset width-1 */
	#endif /* kFskScaleBits < FWD_BITS */
	if (scaleOffset->scaleY >= 0)																	/* If the scale is positive, ... */
		dstY1 += FWD_ONE;																			/* ... add one to get the rightmost coordinate */
	else																							/* If the scale is negative, ... */
		dstY1 -= FWD_ONE;																			/* ... subtract one to get the leftmost coordinate */
	if (dstY0 > dstY1) {																			/* If Y0 is below Y1, ... */
		i = dstY0;
		dstY0 = dstY1;																				/* ... swap them, so Y0 <= Y1 */
		dstY1 = i;
	}

	/* Compute delta of the src w.r.t. unit step in dst */
	du = FskFixedNDiv(FWD_ONE, scaleOffset->scaleY, kFskScaleBits);									/* FWD_BITS fractional bits */
	dv = FskFixedNDiv(FWD_ONE, scaleOffset->scaleX, kFskScaleBits);									/* FWD_BITS fractional bits */

	/* Adjust coordinates to the closest integer inside the intervals [X0, X1] and [Y0, Y1] */
	dstX0 = (dstX0 + FWD_ONE-1) >> FWD_BITS;														/* Ceil */
	dstX1 = (dstX1 + 0        ) >> FWD_BITS;														/* Floor */
	dstY0 = (dstY0 + FWD_ONE-1) >> FWD_BITS;														/* Ceil */
	dstY1 = (dstY1 + 0        ) >> FWD_BITS;														/* Floor */

	/* Clip the transformed source to the dstClip */
	if (dstX0 < dstClip.x)
		dstX0 = dstClip.x;
	if (dstY0 < dstClip.y)
		dstY0 = dstClip.y;
	if (dstX1 > (i = dstClip.x + dstClip.width	- 1))
		dstX1 = i;
	if (dstY1 > (i = dstClip.y + dstClip.height - 1))
		dstY1 = i;

	/* Compute phase of the src at the upper left of the dst */
	u0 = FskFixedNMul((dstY0 << kFskOffsetBits) - scaleOffset->offsetY, du, kFskOffsetBits);		/* FWD_BITS fractional bits */
	v0 = FskFixedNMul((dstX0 << kFskOffsetBits) - scaleOffset->offsetX, dv, kFskOffsetBits);		/* FWD_BITS fractional bits */

	/* Round coordinates */
	i = FWD_HALF >> (quality << LOG_LERP_BITS);														/* pre-rounding: 1/2 for point-sampling, 1/32 for bilinear */
	u0 += i;
	v0 += i;

	/* Check that it stays within source bounds */
	i = (dstY1 - dstY0) * du + u0;	/* Compute u1 */
	#if 1	/* One pixel adjustment should be enough */
		if (du >= 0) {
			if (u0 < ( srcClip.x                      << FWD_BITS))		{ ++dstY0;	u0 += du; }
			if (i  > ((srcClip.x + srcClip.width - 1) << FWD_BITS))		{ --dstY1;	          }
		}
		else {	/* du < 0 */
			if (i  < ( srcClip.x                      << FWD_BITS))		{ --dstY1;	          }
			if (u0 > ((srcClip.x + srcClip.width - 1) << FWD_BITS))		{ ++dstY0;	u0 += du; }
		}
		i = (dstX1 - dstX0) * dv + v0;	/* Compute v1 */
		if (dv >= 0) {
			if (v0 < ( srcClip.y                       << FWD_BITS))	{ ++dstX0;	v0 += dv; }
			if (i  > ((srcClip.y + srcClip.height - 1) << FWD_BITS))	{ --dstX1;	          }
		}
		else {	/* dv < 0 */
			if (i  < ( srcClip.y                       << FWD_BITS))	{ --dstX1;	          }
			if (v0 > ((srcClip.y + srcClip.height - 1) << FWD_BITS))	{ ++dstX0;	v0 += dv; }
		}
	#else /* Just in case one adjustment is not enough */
		if (du >= 0) {
			while (u0 < ( srcClip.x                       << FWD_BITS))	{ ++dstY0;	u0 += du; }
			while (i  > ((srcClip.x + srcClip.width - 1)  << FWD_BITS))	{ --dstY1;	i  -= du; }
		}
		else {	/* du < 0 */
			while (i  < ( srcClip.x                       << FWD_BITS))	{ --dstY1;	i  -= du; }
			while (u0 > ((srcClip.x + srcClip.width - 1)  << FWD_BITS))	{ ++dstY0;	u0 += du; }
		}
		i = (dstX1 - dstX0) * dv + v0;	/* Compute v1 */
		if (dv >= 0) {
			while (v0 < ( srcClip.y                       << FWD_BITS))	{ ++dstX0;	v0 += dv; }
			while (i  > ((srcClip.y + srcClip.height - 1) << FWD_BITS))	{ --dstX1;	i  -= dv; }
		}
		else {	/* dv < 0 */
			while (i  < ( srcClip.y                       << FWD_BITS))	{ --dstX1;	i  -= dv; }
			while (v0 > ((srcClip.y + srcClip.height - 1) << FWD_BITS))	{ ++dstX0;	v0 += dv; }
		}
	#endif /* 0 */

	/* Compute width and height and determine whether there is any work left to do */
	dstWidth  = dstX1 - dstX0 + 1;
	dstHeight = dstY1 - dstY0 + 1;
	BAIL_IF_FALSE(((dstWidth > 0) && (dstHeight > 0)), err, kFskErrNothingRendered);

	/* Src image description */
	srcBaseAddr = (const void*)((const char*)srcBM->bits
				+ (srcClipRect->y - srcBM->bounds.y) *	srcBM->rowBytes
				+ (srcClipRect->x - srcBM->bounds.x) * (srcBM->depth >> 3));

	/* Dst image description */
	dstBaseAddr = (void*)((char*)dstBM->bits
				+ (dstY0 - dstBM->bounds.y) *  dstBM->rowBytes
				+ (dstX0 - dstBM->bounds.x) * (dstBM->depth >> 3));

	srcIndex = PixelFormatToProcTableSrcIndex(srcBM->pixelFormat);
	dstIndex = PixelFormatToProcTableDstIndex(dstBM->pixelFormat);
	BAIL_IF_TRUE(((srcIndex < 0) || (dstIndex < 0)), err, kFskErrUnsupportedPixelType);
	if (quality != 0)
		quality = 1;
	BAIL_IF_NULL((blitProc = rotDispatch[dstIndex][srcIndex][quality]), err, kFskErrInvalidParameter);	/* Get proc from dispatch table */
	(*blitProc)(u0, v0, du, dv, srcBaseAddr, srcBM->rowBytes, dstBaseAddr, dstBM->rowBytes, dstWidth, dstHeight);

bail:
	return err;
}
