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
#include "FskPick.h"
#include "FskPixelOps.h"

/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****							Macros and constants						****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/

#define LOCAL_BUF_DIM	5

#define FIXED_ONE				(1 << 16)



/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****							Utility Functions							****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/



/*******************************************************************************
 * PickImageRect8
 *******************************************************************************/

static FskPickResult
PickImageRect8(
	register const UInt8	*p,
	register SInt32			pixelBytes,
	SInt32					rowBytes,
	UInt32					width,
	UInt32					height,
	UInt8					transparentThresh
)
{
	register UInt32	i;
	register int	hit;
	register int	thresh	= transparentThresh;

	for (rowBytes -= width * pixelBytes, hit = 0; height--; p += rowBytes) {
		for (i = width; i--; p += pixelBytes)
			hit |= thresh - (int)(*p);	/* Accumulate threshold excession */
		if (hit < 0)					/* And check on it once a line */
			return kFskHitTrue;
	}

	return kFskHitFalse;
}


/*******************************************************************************
 * PickImageRect4in16
 *******************************************************************************/

static FskPickResult
PickImageRect4in16(
	register const UInt16	*p,
	SInt32					rowBytes,
	UInt32					width,
	UInt32					height,
	UInt8					transparentThresh,
	UInt16					alphaMask
)
{
	register UInt32	i;
	register int	hit;
	register int	mask	= alphaMask;
	register int	thresh = ((transparentThresh >> 4) * 0x1111) & mask;

	for (rowBytes -= width * sizeof(UInt16), hit = 0; height--; p = (const UInt16*)((const char*)p + rowBytes)) {
		for (i = width; i--; p++)
			hit |= thresh - (*p & mask);	/* Accumulate threshold excession */
		if (hit < 0)						/* And check on it once a line */
			return kFskHitTrue;
	}

	return kFskHitFalse;
}


/*******************************************************************************
 * PickImageRect1in16
 *******************************************************************************/

static FskPickResult
PickImageRect1in16(
	register const UInt16	*p,
	SInt32					rowBytes,
	UInt32					width,
	UInt32					height,
	register UInt16			alphaMask
)
{
	register UInt32	i;
	register int	hit;

	for (rowBytes -= width * sizeof(UInt16), hit = 0; height--; p = (const UInt16*)((const char*)p + rowBytes)) {
		for (i = width; i--; p++)
			hit |= *p & alphaMask;			/* Accumulate threshold excession */
		if (hit != 0)						/* And check on it once a line */
			return kFskHitTrue;
	}

	return kFskHitFalse;
}



/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****									API									****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/



/*******************************************************************************
 *******************************************************************************
 **		Path Picking
 *******************************************************************************
 *******************************************************************************/


/*******************************************************************************
 * FskPickPath
 *******************************************************************************/

FskPickResult
FskPickPath(
	FskConstPath			path,				/* The path to be hit-tested */
	FskFixed				strokeWidth,		/* frame thickness, or negative for no frame */
	FskFixed				jointSharpness,		/* 0 => round, 1 => bevel, >1 => miter */
	int						endCaps,			/* kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt */
	int						fillRule,			/* kFskFillRuleNonZero, kFskFillRuleEvenOdd, or negative for no fill */
	const FskColorSource	*frameColor,		/* Frame color - can be NULL if the pixel color is not needed */
	const FskColorSource	*fillColor,			/* Fill  color - can be NULL if the pixel color is not needed */
	const FskFixedMatrix3x2	*M,					/* Transformation matrix; NULL implies identity */
	FskConstRectangle		pickRect,			/* Test to see if the path hits this rect */
	FskColorRGBA			pixelColor			/* The color of the pixel under the pick (can be NULL if not needed) */
)
{
	FskPickResult			result;
	UInt32					locBuf[LOCAL_BUF_DIM*LOCAL_BUF_DIM];
	int						*heapBuf			= NULL;
	UInt32					pickBufSize;
	FskBitmapRecord			bmr;
	FskColorSourceConstant	color;
	FskColorRGBARecord		swatch;

	BAIL_IF_NULL(pickRect, result, kFskErrInvalidParameter);

	/* Select the appropriate size pick buffer and clear it */
	FskMemSet(&bmr, 0, sizeof(FskBitmapRecord));													/* Hack together a temporary Bitmap ... */
	if (kFskBitmapFormat32RGBA == kFskBitmapFormatDefaultRGBA	||
		kFskBitmapFormat32ARGB == kFskBitmapFormatDefaultRGBA	||
		kFskBitmapFormat32ABGR == kFskBitmapFormatDefaultRGBA	||
		kFskBitmapFormat32BGRA == kFskBitmapFormatDefaultRGBA
	) {																								/* We prefer 32 bit kFskBitmapFormatDefaultRGBA */
		bmr.depth       = 32;
		bmr.pixelFormat = kFskBitmapFormatDefaultRGBA;
		bmr.hasAlpha	= 1;
	}
	else {																							/* Otherwise, we prefer a 32 bit destination */
		#if   defined(DST_32RGBA)        && DST_32RGBA
			bmr.pixelFormat =   kFskBitmapFormat32RGBA;			bmr.depth = 32;	bmr.hasAlpha = 1;
		#elif defined(DST_32BGRA)        && DST_32BGRA
			bmr.pixelFormat =   kFskBitmapFormat32BGRA;			bmr.depth = 32;	bmr.hasAlpha = 1;
		#elif defined(DST_32ARGB)        && DST_32ARGB
			bmr.pixelFormat =   kFskBitmapFormat32ARGB;			bmr.depth = 32;	bmr.hasAlpha = 1;
		#elif defined(DST_32ABGR)        && DST_32ABGR
			bmr.pixelFormat =   kFskBitmapFormat32ABGR;			bmr.depth = 32;	bmr.hasAlpha = 1;
		#elif defined(DST_16AG)          && DST_16AG												/* Otherwise, we prefer a 16 bit format with alpha */
			bmr.pixelFormat =   kFskBitmapFormat16AG;			bmr.depth = 16;	bmr.hasAlpha = 1;
		#elif defined(DST_16RGBA4444LE)  && DST_16RGBA4444LE
			bmr.pixelFormat =   kFskBitmapFormat16RGBA4444LE;	bmr.depth = 16;	bmr.hasAlpha = 1;
		#elif defined(DST_16RGB565LE)    && DST_16RGB565LE											/* Otherwise, we prefer a 16 bit format */
			bmr.pixelFormat =   kFskBitmapFormat16RGB565LE;		bmr.depth = 16;	bmr.hasAlpha = 0;
		#elif defined(DST_16RGB565BE)    && DST_16RGB565BE
			bmr.pixelFormat =   kFskBitmapFormat16RGB565BE;		bmr.depth = 16;	bmr.hasAlpha = 0;
		#elif defined(DST_8G)            && DST_8G													/* Otherwise, we prefer a 8 bit grayscale */
			bmr.pixelFormat =   kFskBitmapFormat8G;				bmr.depth = 8;	bmr.hasAlpha = 0;
		#else
			bmr.pixelFormat =   kFskBitmapFormatUnknown;		bmr.depth = 0;	bmr.hasAlpha = 0;	/* It should never get here */
		#endif
	}
	BAIL_IF_FALSE(kFskBitmapFormatUnknown != bmr.pixelFormat, result, kFskErrUnimplemented);
	bmr.bounds      = *pickRect;																	/* Temporary bitmap bounds matches the pickRect */
	bmr.rowBytes    = pickRect->width * (bmr.depth >> 3);											/* No need for interline slop */
	pickBufSize     = pickRect->height * bmr.rowBytes;												/* Total buffer size to allocate */
	if (pickBufSize > sizeof(locBuf)) {																/* If the pick rect is unusually large ... */
		BAIL_IF_ERR(result = FskMemPtrNew(pickBufSize, (FskMemPtr*)(void*)(&heapBuf)));				/* ... we need to allocate a pick buffer ... */
		bmr.bits = heapBuf;																			/* ... from the heap */
	}
	else {																							/* Pick rect is of the expected area ... */
		bmr.bits = locBuf;																			/* ... so we use a local buffer */
	}
	FskMemSet(bmr.bits, 0, pickBufSize);															/* Clear the pick buffer to zero */

	/* Initialize a default color source */
	color.colorSource.type			= kFskColorSourceTypeConstant;
	color.colorSource.dashCycles	= 0;															/* No dashing */
	color.colorSource.dash			= NULL;
	color.colorSource.dashPhase		= 0;
	color.color.r = 0xFF;																			/* We'll render all 1's into it */
	color.color.g = 0xFF;
	color.color.b = 0xFF;
	color.color.a = 0xFF;
	if (frameColor == NULL)	frameColor = &color.colorSource;										/* Supply a default frame color if none was supplied */
	if (fillColor  == NULL)	fillColor  = &color.colorSource;										/* Supply a default fill  color if none was supplied */

	if (strokeWidth < 0) {																												/* Fill only */
		BAIL_IF_NEGATIVE(fillRule, result, kFskErrInvalidParameter);
		result = FskPathFill(     path,                                                   fillColor, fillRule, M, 0, pickRect, &bmr);
	}
	else if (fillRule < 0) {																											/* Frame only */
		result = FskPathFrame(    path, strokeWidth, jointSharpness, endCaps, frameColor,                      M, 0, pickRect, &bmr);
	}
	else {																																/* Frame and fill */
		result = FskPathFrameFill(path, strokeWidth, jointSharpness, endCaps, frameColor, fillColor, fillRule, M, 0, pickRect, &bmr);
	}

	if (result >= 0) {																				/* If no errors, */
		if (result == kFskErrNothingRendered) {														/* If everything was clipped out ... */
			result = kFskHitFalse;																	/* ... then there is no hit */
		}
		else if (bmr.depth == 32) {																	/* Platform supports 32 bits */
			UInt32	*p, a;
			for (a = 0, p = (UInt32*)(bmr.bits), pickBufSize /= sizeof(UInt32); pickBufSize--; )
				a |= *p++;																			/* We look to see if anything was written */
			FskConvertBitmapPixelToColorRGBA(&a, bmr.pixelFormat, &swatch);							/* Check out the composite alpha */
			result = (swatch.a != 0) ? kFskHitTrue : kFskHitFalse;									/* If alpha is nonzero, we definitely have a hit! */
		}
		else if (bmr.depth == 16) {
			if (bmr.hasAlpha) {
				UInt16	*p, a;
				for (a = 0, p = (UInt16*)(bmr.bits), pickBufSize /= sizeof(UInt16); pickBufSize--; )
					a |= *p++;																		/* We look to see if anything was written */
				FskConvertBitmapPixelToColorRGBA(&a, bmr.pixelFormat, &swatch);						/* Check out the composite alpha */
				result = (swatch.a != 0) ? kFskHitTrue : kFskHitFalse;								/* If alpha is nonzero, we definitely have a hit! */
			}
			else {
				result = kFskHitTrue;																/* Since we have no alpha, assume the renderer tells the truth about kFskErrNothingRendered */
			}
		}
		else if (bmr.depth == 8) {
			result = kFskHitTrue;																	/* Since we have no alpha, assume the renderer tells the truth about kFskErrNothingRendered */
		}

		/* Retrieve the pick color */
		if ((result == kFskHitTrue) && (pixelColor != NULL)) {
			bmr.bits =	(UInt8*)(bmr.bits)
					+	(pickRect->height >> 1) * bmr.rowBytes
					+	(pickRect->width  >> 1) * (bmr.depth >> 3);									/* Offset pixel pointer to the center of the rect */
			FskConvertBitmapPixelToColorRGBA(bmr.bits, bmr.pixelFormat, pixelColor);				/* Get the color of the pixel under the pick point */
		}
	}

bail:
	if (heapBuf != NULL)	FskMemPtrDispose(heapBuf);
	return result;
}


/*******************************************************************************
 * FskPickPathRadius
 *******************************************************************************/

FskPickResult
FskPickPathRadius(
	FskConstPath			path,				/* The path to be hit-tested */
	FskFixed				strokeWidth,		/* frame thickness, or negative for no frame */
	FskFixed				jointSharpness,		/* 0 => round, 1 => bevel, >1 => miter */
	int						endCaps,			/* kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt */
	int						fillRule,			/* kFskFillRuleNonZero, kFskFillRuleEvenOdd, or negative for no fill */
	const FskColorSource	*frameColor,		/* Frame color - can be NULL if the pixel color is not needed */
	const FskColorSource	*fillColor,			/* Fill  color - can be NULL if the pixel color is not needed */
	const FskFixedMatrix3x2	*M,					/* Transformation matrix; NULL implies identity */
	int						pickRadius,			/* Within this radius (roughly) of the given point */
	FskConstPoint			pickPoint,			/* Does something hit this point? */
	FskColorRGBA			pixelColor			/* The color of the pixel under the pick (can be NULL if not needed) */
)
{
	FskRectangleRecord	pickRect;

	if (pickRadius <= 0)	pickRadius = 0;
	else					pickRadius--;

	pickRect.x = pickPoint->x - pickRadius;
	pickRect.y = pickPoint->y - pickRadius;
	pickRect.width = pickRect.height = 1 + (pickRadius << 1);	/* radius={1,2,3,...} ==> width={1,3,5,...} */

	return FskPickPath(path, strokeWidth, jointSharpness, endCaps, fillRule, frameColor, fillColor, M, &pickRect, pixelColor);
}



/*******************************************************************************
 *******************************************************************************
 **		Bitmap Picking
 *******************************************************************************
 *******************************************************************************/


/*******************************************************************************
 * FskPickBitmap
 *******************************************************************************/

FskPickResult
FskPickBitmap(
	FskBitmap				bm,					/* The bitmap to be hit-tested */
	UInt8					transparentThresh,	/* An alpha greater than this level is a hit */
	const FskFixedMatrix3x2	*M,					/* Transformation matrix; NULL implies identity */
	FskConstRectangle		pickRect,			/* Test to see if the bm hits this rect */
	FskColorRGBA			pixelColor			/* The color of the pixel under the pick (can be NULL if not needed) */
)
{
	FskRectangleRecord	pr;
	unsigned char		*p0;
	FskPickResult		result;

	/*
	 * Transform the pick rect if a matrix is specified.
	 */
	if (M != NULL) {
		FskFixedPoint2D	scrPt, bmPt;
		FskFixed		radius, scale;

		/* We approximate a rect with a circle, transform the circle, then approximate the circle with a rect */
		scrPt.x = (pickRect->x << 16) + ((pickRect->width  - 1) << 15);	/* Center X */
		scrPt.y = (pickRect->y << 16) + ((pickRect->height - 1) << 15);	/* Center Y */
		radius  = FskFixedNSqrt(pickRect->width * pickRect->height, 30);/* Geometric average radius */
		scale = FskSolveFixedMatrix3x2Equation(&scrPt, &bmPt, M);		/* Find determinant and bitmap point */
		if (scale < 0)	scale = -scale;									/* Absolute value of determinant */
		scale  = FskFixSqrt(scale);										/* Geometric scale factor */
		radius = FskFixDiv(radius, scale);								/* Scale pick radius by geometric scale factor */
		if (radius < (FIXED_ONE >> 1))	radius = (FIXED_ONE >> 1);		/* Assure radius >= 1/2 */
		pr.x      = FskCeilFixedToInt( bmPt.x - radius);				/* Compute a pick rect */
		pr.y      = FskCeilFixedToInt( bmPt.y - radius);
		pr.width  = FskFloorFixedToInt(bmPt.x + radius) + 1 - pr.x;
		pr.height = FskFloorFixedToInt(bmPt.y + radius) + 1 - pr.y;
		pickRect  = &pr;
	}

	/*
	 * Perform a geometric hit test with the pick rect, then see if the pick rect contains any opaque pixels.
	 */
	if (FskRectangleIntersect(pickRect, &bm->bounds, &pr) == 0) {		/* See if the pick rect intersects with the bitmap */
		result	= kFskHitFalse;											/* No intersection -- no hit -- done */
	}
	else {																/* The pick rect intersects with the bitmap */
		if (bm->hasAlpha == 0) {										/* Pixels are all opaque -- we have a hit! */
			result = kFskHitTrue;										/* We have a hit if the intersection is not null and the pixels are opaque */
		}
		else {															/* Some pixels are transparent -- check to see if any opaque pixels are in the pick rect */
			p0	= (unsigned char*)(bm->bits)
				+ pr.y * bm->rowBytes
				+ pr.x * (bm->depth >> 3);
			switch (bm->pixelFormat) {
				case kFskBitmapFormat32ARGB:		/* fall through */
				case kFskBitmapFormat32ABGR:		result = PickImageRect8(p0+0, 4, bm->rowBytes, pr.width, pr.height, transparentThresh);			break;
				case kFskBitmapFormat32BGRA:		/* fall through */
				case kFskBitmapFormat32RGBA:		result = PickImageRect8(p0+3, 4, bm->rowBytes, pr.width, pr.height, transparentThresh);			break;
				case kFskBitmapFormat16AG:			result = PickImageRect8(p0+0, 2, bm->rowBytes, pr.width, pr.height, transparentThresh);			break;
				case kFskBitmapFormat8A:			result = PickImageRect8(p0+0, 1, bm->rowBytes, pr.width, pr.height, transparentThresh);			break;
				case kFskBitmapFormat16RGBA4444LE:	result = PickImageRect4in16((UInt16*)p0,bm->rowBytes, pr.width, pr.height, transparentThresh,
																			0xF << FskName3(fsk,fsk16RGBA4444LEFormatKind,AlphaPosition));			break;
				case kFskBitmapFormat16RGB5515LE:	result = PickImageRect1in16((UInt16*)p0,bm->rowBytes, pr.width, pr.height,
																			0x1 << FskName3(fsk,fsk16RGB5515LEFormatKind,AlphaPosition));			break;
				default:							result = kFskErrUnsupportedPixelType;															break;
			}

			if ((result == kFskHitTrue) && (pixelColor != NULL)) {
				p0	+= (pr.height >> 1) * bm->rowBytes
					+  (pr.width  >> 1) * (bm->depth >> 3);							/* Center of rect */
				FskConvertBitmapPixelToColorRGBA(p0, bm->pixelFormat, pixelColor);	/* Color in center of pick rect, converted to 32RGBA */
			}
		}
	}

	return result;
}


/*******************************************************************************
 * FskPickBitmapRadius
 *******************************************************************************/

FskPickResult
FskPickBitmapRadius(
	FskBitmap				bm,					/* The bitmap to be hit-tested */
	UInt8					transparentThresh,	/* An alpha greater than this level is a hit */
	const FskFixedMatrix3x2	*M,					/* Transformation matrix; NULL implies identity */
	int						pickRadius,			/* Within this radius (roughly) of the given point */
	FskConstPoint			pickPoint,			/* Does something hit this point? */
	FskColorRGBA			pixelColor			/* The color of the pixel under the pick (can be NULL if not needed) */
)
{
	FskRectangleRecord	pr;

	if (M != NULL) {
		FskFixedPoint2D	scrPt, bmPt;
		FskFixed		radius, scale;

		scrPt.x = pickPoint->x << 16;								/* Convert screen point from integer ... */
		scrPt.y = pickPoint->y << 16;								/* ...  to Fixed 16.16 */
		scale = FskSolveFixedMatrix3x2Equation(&scrPt, &bmPt, M);	/* Find determinant and bm point */
		if (scale < 0)	scale = -scale;								/* Absolute value of determinant */
		scale = FskFixDiv(FIXED_ONE, FskFixSqrt(scale));			/* Geometric scale factor */
		radius = scale * pickRadius;								/* Scale pick radius */
		if (radius < (FIXED_ONE >> 1))	radius = (FIXED_ONE >> 1);	/* Assure radius >= 1/2 */
		pr.x      = FskCeilFixedToInt( bmPt.x - radius);			/* Compute a pick rect */
		pr.y      = FskCeilFixedToInt( bmPt.y - radius);
		pr.width  = FskFloorFixedToInt(bmPt.x + radius) + 1 - pr.x;
		pr.height = FskFloorFixedToInt(bmPt.y + radius) + 1 - pr.y;
	}
	else {
		if (pickRadius <= 0)	pickRadius = 0;
		else					pickRadius--;
		pr.x = pickPoint->x - pickRadius;
		pr.y = pickPoint->y - pickRadius;
		pr.width = pr.height = 1 + (pickRadius << 1);	/* radius={1,2,3,...} ==> width={1,3,5,...} */
	}
	return FskPickBitmap(bm, transparentThresh, NULL, &pr, pixelColor);
}

