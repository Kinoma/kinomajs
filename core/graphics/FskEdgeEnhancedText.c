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


#include "FskEdgeEnhancedText.h"
#include "FskPixelOps.h"
#include "FskUtilities.h"
#include <string.h>

#define USE_MEMSTRING	/* To use the system's memmove() and memset() */

#define FLAG_NO_LEFT_EDGE				(1 << 0)				/* Don't enhance the edge on the left  pixel */
#define FLAG_NO_RIGHT_EDGE				(1 << 1)				/* Don't enhance the edge on the right pixel */
#define FLAG_NO_TOP_EDGE				(1 << 2)				/* Don't enhance the edge on the top    scanline */
#define FLAG_NO_BOTTOM_EDGE				(1 << 3)				/* Don't enhance the edge on the bottom scanline */
#define FLAG_PRELOAD_LEFT_TEXT			(1 << 4)				/* We have clipped more than just the left   edge */
#define FLAG_PRELOAD_RIGHT_TEXT			(1 << 5)				/* We have clipped more than just the right  edge */
#define FLAG_PRELOAD_TOP_TEXT			(1 << 6)				/* We have clipped more than just the top    edge */
#define FLAG_PRELOAD_BOTTOM_TEXT		(1 << 7)				/* We have clipped more than just the bottom edge */

#define FLAG_CLIP_LEFT_EDGE				(FLAG_NO_LEFT_EDGE)
#define FLAG_CLIP_LEFT_TEXT				(FLAG_NO_LEFT_EDGE   | FLAG_PRELOAD_LEFT_TEXT)
#define FLAG_CLIP_RIGHT_EDGE			(FLAG_NO_RIGHT_EDGE)
#define FLAG_CLIP_RIGHT_TEXT			(FLAG_NO_RIGHT_EDGE  | FLAG_PRELOAD_RIGHT_TEXT)
#define FLAG_CLIP_TOP_EDGE				(FLAG_NO_TOP_EDGE)
#define FLAG_CLIP_TOP_TEXT				(FLAG_NO_TOP_EDGE    | FLAG_PRELOAD_TOP_TEXT)
#define FLAG_CLIP_BOTTOM_EDGE			(FLAG_NO_BOTTOM_EDGE)
#define FLAG_CLIP_BOTTOM_TEXT			(FLAG_NO_BOTTOM_EDGE | FLAG_PRELOAD_BOTTOM_TEXT)

#define TARGET_RT_BIG_ENDIAN_BITS		1						/* The leftmost pixel is in the  most significant bit */
#define TARGET_RT_LITTLE_ENDIAN_BITS	0						/* The leftmost pixel is in the least significant bit */

#if TARGET_RT_BIG_ENDIAN_BITS
	#define FIRST_BIT_IN_BYTE			7						/* The first bit is the MSB */
	#define BIT_MASK(b)					(1 << (7 - (b)))
#else /* TARGET_RT_LITTLE_ENDIAN_BITS */
	#define FIRST_BIT_IN_BYTE			0						/* The first bit is the LSB */
	#define BIT_MASK(b)					(1 << (b))
#endif /* TARGET_RT_LITTLE_ENDIAN_BITS */
#define LAST_BIT_IN_BYTE				(7 - FIRST_BIT_IN_BYTE)



#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /*  !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /*  !PRAGMA_MARK_SUPPORTED */
#endif /*  PRAGMA_MARK_SUPPORTED */


typedef union ColorBuddy {
	UInt8	p1;
	UInt16	p2;
	UInt8	p3[3];
	UInt32	p4;
} ColorBuddy;


typedef struct Span {
	UInt32		width;
	UInt32		xOffset;
	SInt32		topBorder;
	SInt32		bottomBorder;
	SInt32		leftBorder;
	SInt32		rightBorder;
	ColorBuddy	edgeColor;
	ColorBuddy	textColor;
	SInt32		blendLevel;
} Span;

typedef struct MyBounds {	SInt32	x0, y0, x1, y1;		} MyBounds;

typedef void (*SetSpanProc)(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst);

#ifdef DEBUG
#include <stdio.h>
static void PrintBitMap(UInt8 *baseAddr, SInt32 rowBytes, SInt32 x, SInt32 y, UInt32 width, UInt32 height, FILE *fd);
static void PrintTwitMap(UInt8 *txt, SInt32 txtRowBytes, UInt8 *edg, SInt32 edgRowBytes, SInt32 x, SInt32 y, UInt32 width, UInt32 height, FILE *fd);
#endif /* DEBUG */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Common Utilities							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * CopyBytes
 ********************************************************************************/

#ifdef USE_MEMSTRING
	#define CopyBytes(src, numBytes, dst)	memmove(dst, src, numBytes)
#else /* !USE_MEMSTRING */
static void
CopyBytes(register const UInt8 *src, register UInt32 numBytes, register UInt8 *dst)
{
	while (numBytes--)
		*dst++ = *src++;
}
#endif /* !USE_MEMSTRING */


/********************************************************************************
 * ZeroBytes
 ********************************************************************************/

#ifdef USE_MEMSTRING
	#define ZeroBytes(dst, numBytes)	memset(dst, 0, numBytes)
#else /* !USE_MEMSTRING */
static void
ZeroBytes(register UInt8 *dst, register UInt32 numBytes)
{
	while (numBytes--)
		*dst++ = 0;
}
#endif /* !USE_MEMSTRING */


/********************************************************************************
 * Clip
 ********************************************************************************/

static int								/* 0 if all clipped, 1 if something to render */
Clip(
	FskConstRectangle	txtRect,		/* Text rect */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskPoint			dstPoint,		/* Location of text */
	SInt32				borderWidth,	/* The thickness of the border */
	Span				*span,			/* We set the flags */
	FskRectangle		srcRect			/* The clipped result */
)
{
	MyBounds	clipBounds, dstBounds, srcBounds;

	/* Do clipping */
	clipBounds.x1 = (clipBounds.x0 =  dstClip->x) + dstClip->width;				/* Bounds of destination clipping rect */
	clipBounds.y1 = (clipBounds.y0 =  dstClip->y) + dstClip->height;
	dstBounds.x1  = ( dstBounds.x0 = dstPoint->x) + txtRect->width;				/* Bounds of text in destination without edge */
	dstBounds.y1  = ( dstBounds.y0 = dstPoint->y) + txtRect->height;
	dstBounds.x0 -= borderWidth;	dstBounds.x1 += borderWidth;				/* Bounds of text in destination  with   edge */
	dstBounds.y0 -= borderWidth;	dstBounds.y1 += borderWidth;
	if (dstBounds.x0 < clipBounds.x0)	dstBounds.x0 = clipBounds.x0;			/* Clip transformed src againat dst clip */
	if (dstBounds.y0 < clipBounds.y0)	dstBounds.y0 = clipBounds.y0;
	if (dstBounds.x1 > clipBounds.x1)	dstBounds.x1 = clipBounds.x1;
	if (dstBounds.y1 > clipBounds.y1)	dstBounds.y1 = clipBounds.y1;
	srcBounds.x0 = dstBounds.x0 - dstPoint->x + txtRect->x;						/* Transform clipped bounds from dst to back to src */
	srcBounds.x1 = dstBounds.x1 - dstPoint->x + txtRect->x;
	srcBounds.y0 = dstBounds.y0 - dstPoint->y + txtRect->y;
	srcBounds.y1 = dstBounds.y1 - dstPoint->y + txtRect->y;

	/* See whether any of the border has been clipped out, and if not, adjust srcBounds to eliminate synthetic edge */
	if ((span->leftBorder   = txtRect->x   - srcBounds.x0) > 0) {
		if (span->leftBorder > borderWidth)
			span->leftBorder = borderWidth;
		srcBounds.x0 += span->leftBorder;
	}
	else if (span->leftBorder < -borderWidth)
		span->leftBorder = -borderWidth;

	if ((span->topBorder    = txtRect->y   - srcBounds.y0) > 0) {
		if (span->topBorder > borderWidth)
			span->topBorder = borderWidth;
		srcBounds.y0 += span->topBorder;
	}
	else if (span->topBorder < -borderWidth)
		span->topBorder = -borderWidth;

	if ((span->rightBorder  = srcBounds.x1 - (txtRect->x + txtRect->width))	 > 0) {
		if (span->rightBorder > borderWidth)
			span->rightBorder = borderWidth;
		srcBounds.x1 -= span->rightBorder;
	}
	else if (span->rightBorder < -borderWidth)
		span->rightBorder = -borderWidth;

	if ((span->bottomBorder = srcBounds.y1 - (txtRect->y + txtRect->height)) > 0) {
		if (span->bottomBorder > borderWidth)
			span->bottomBorder = borderWidth;
		srcBounds.y1 -= span->bottomBorder;
	}
	else if (span->bottomBorder < -borderWidth)
		span->bottomBorder = -borderWidth;

	/* Set the resultant clip result */
	if (	((srcRect->width  = srcBounds.x1 - (srcRect->x = srcBounds.x0)) < 0)
		||	((srcRect->height = srcBounds.y1 - (srcRect->y = srcBounds.y0)) < 0)
	)	return 0;																/* All clipped out */
	dstPoint->x = dstBounds.x0;
	dstPoint->y = dstBounds.y0;

	return 1;																	/* Something to render */
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									Bilevel								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * BilevelSetSpan8
 ********************************************************************************/

static void
BilevelSetSpan8(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst)
{
	UInt32	width;
	UInt32	xOffset		= span->xOffset;

	UInt8	*dst		= (UInt8*)vDst;
	UInt8	textColor	= span->textColor.p1;
	UInt8	edgeColor	= span->edgeColor.p1;

	if (span->leftBorder > 0) {											/* If the left edge isn't clipped ... */
		if ((xOffset == 0)	? (smr[-1] & BIT_MASK(7))					/* ... and the previous smeared bit is set (previous byte ... */
							: (smr[ 0] & BIT_MASK(xOffset-1))			/* ... or current byte) ... */
		)
			*dst = edgeColor;											/* ... then set the edge color */
		dst++;															/* In any case, advance to the next pixel */
	}

	for (width = span->width ; width--; dst++) {
		if (*smr & BIT_MASK(xOffset))
			*dst = (*src & BIT_MASK(xOffset)) ? textColor : edgeColor;	/* Set middle pixels */
		if (++xOffset == 8) {	xOffset = 0;	smr++;		src++; }	/* Advance bit counter and advance smear and text pointers if it overflows */
	}

	if (	(span->rightBorder > 0)										/* If the right edge isn't clipped ... */
		&&	(*smr & BIT_MASK(xOffset))									/* ... and the smeared bit is set ... */
	) *dst = edgeColor;													/* ... then set the edge color */
}


/********************************************************************************
 * BilevelSetSpan16
 ********************************************************************************/

static void
BilevelSetSpan16(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst)
{
	UInt32	width;
	UInt32	xOffset		= span->xOffset;

	UInt16	*dst		= (UInt16*)vDst;
	UInt16	textColor	= span->textColor.p2;
	UInt16	edgeColor	= span->edgeColor.p2;

	if (span->leftBorder > 0) {											/* If the left edge isn't clipped ... */
		if ((xOffset == 0)	? (smr[-1] & BIT_MASK(7))					/* ... and the previous smeared bit is set (previous byte ... */
							: (smr[ 0] & BIT_MASK(xOffset-1))			/* ... or current byte) ... */
		)
			*dst = edgeColor;											/* ... then set the edge color */
		dst++;															/* In any case, advance to the next pixel */
	}

	for (width = span->width ; width--; dst++) {
		if (*smr & BIT_MASK(xOffset))
			*dst = (*src & BIT_MASK(xOffset)) ? textColor : edgeColor;	/* Set middle pixels */
		if (++xOffset == 8) {	xOffset = 0;	smr++;		src++; }	/* Advance bit counter and advance smear and text pointers if it overflows */
	}

	if (	(span->rightBorder > 0)										/* If the right edge isn't clipped ... */
		&&	(*smr & BIT_MASK(xOffset))									/* ... and the smeared bit is set ... */
	) *dst = edgeColor;													/* ... then set the edge color */

}


/********************************************************************************
 * BilevelSetSpan24
 ********************************************************************************/

static void
BilevelSetSpan24(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst)
{
	UInt32	width;
	UInt32	xOffset		= span->xOffset;

	UInt8	*dst		= (UInt8*)vDst;

	if (span->leftBorder > 0) {											/* If the left edge isn't clipped ... */
		if ((xOffset == 0)	? (smr[-1] & BIT_MASK(7))					/* ... and the previous smeared bit is set (previous byte ... */
							: (smr[ 0] & BIT_MASK(xOffset-1))			/* ... or current byte) ... */
		) {
			dst[0] = span->edgeColor.p3[0];	dst[1] = span->edgeColor.p3[1];	dst[2] = span->edgeColor.p3[2];
		}
		dst += 3;														/* In any case, advance to the next pixel */
	}

	for (width = span->width ; width--; dst += 3) {
		if (*smr & BIT_MASK(xOffset)) {		/* Set middle pixels to the text color if the src bit is set, or the edge color if it is not */
			if (*src & BIT_MASK(xOffset)) {	dst[0] = span->textColor.p3[0];	dst[1] = span->textColor.p3[1];	dst[2] = span->textColor.p3[2];	}
			else {							dst[0] = span->edgeColor.p3[0];	dst[1] = span->edgeColor.p3[1];	dst[2] = span->edgeColor.p3[2];	}
		}
		if (++xOffset == 8) {	xOffset = 0;	smr++;		src++; }	/* Advance bit counter and advance smear and text pointers if it overflows */
	}

	if (	(span->rightBorder > 0)										/* If the right edge isn't clipped ... */
		&&	(*smr & BIT_MASK(xOffset))									/* ... and the smeared bit is set ... */
	) {	dst[0] = span->edgeColor.p3[0];	dst[1] = span->edgeColor.p3[1];	dst[2] = span->edgeColor.p3[2];	}

}


/********************************************************************************
 * BilevelSetSpan32
 ********************************************************************************/

static void
BilevelSetSpan32(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst)
{
	UInt32	width;
	UInt32	xOffset		= span->xOffset;

	UInt32	*dst		= (UInt32*)vDst;
	UInt32	textColor	= span->textColor.p4;
	UInt32	edgeColor	= span->edgeColor.p4;

	if (span->leftBorder > 0) {											/* If the left edge isn't clipped ... */
		if ((xOffset == 0)	? (smr[-1] & BIT_MASK(7))					/* ... and the previous smeared bit is set (previous byte ... */
							: (smr[ 0] & BIT_MASK(xOffset-1))			/* ... or current byte) ... */
		)
			*dst = edgeColor;											/* ... then set the edge color */
		dst++;															/* In any case, advance to the next pixel */
	}

	for (width = span->width ; width--; dst++) {
		if (*smr & BIT_MASK(xOffset))
			*dst = (*src & BIT_MASK(xOffset)) ? textColor : edgeColor;	/* Set middle pixels */
		if (++xOffset == 8) {	xOffset = 0;	smr++;		src++; }	/* Advance bit counter and advance smear and text pointers if it overflows */
	}

	if (	(span->rightBorder > 0)										/* If the right edge isn't clipped ... */
		&&	(*smr & BIT_MASK(xOffset))									/* ... and the smeared bit is set ... */
	) *dst = edgeColor;													/* ... then set the edge color */

}


/********************************************************************************
 * HSmearBilevel
 ********************************************************************************/

static void
HSmearBilevel(const UInt8 *src, UInt32 numBytes, register UInt8 *dst)
{
	register UInt32 n;

	/* Copy with left and right zero pads */
	*dst++ = 0;
	for (n = numBytes; n--; )
		*dst++ = *src++;
	*dst = 0;

	/* Smear horizontally */
	for (n = numBytes, dst -= numBytes; n--; dst++) {
		#if TARGET_RT_BIG_ENDIAN_BITS
			dst[-1] |= dst[0] >> 7;					/* Interbyte smear to the left */
			dst[ 1] |= dst[0] << 7;					/* Interbyte smear to the right */
			dst[ 0] |= dst[0] << 1;					/* Intrabyte smear to the left */
			dst[ 0] |= dst[0] >> 1;					/* Intrabyte smear to the right */
		#else /* TARGET_RT_LITTLE_ENDIAN_BITS */
			dst[-1] |= dst[0] << 7;					/* Interbyte smear to the left */
			dst[ 1] |= dst[0] >> 7;					/* Interbyte smear to the right */
			dst[ 0] |= dst[0] >> 1;					/* Intrabyte smear to the left */
			dst[ 0] |= dst[0] << 1;					/* Intrabyte smear to the right */
		#endif /* TARGET_RT_LITTLE_ENDIAN_BITS */
	}
}


/********************************************************************************
 * VSmearBilevel
 ********************************************************************************/

static void
VSmearBilevel(register const UInt8 *src, register SInt32 srcRB, register UInt32 numBytes, register UInt8 *dst)
{
	for (src += srcRB; numBytes--; src++)
		*dst++ = src[-srcRB] | src[0] | src[srcRB];
}


/********************************************************************************
 * FskEdgeEnhancedBilevelText
 ********************************************************************************/

FskErr
FskEdgeEnhancedBilevelText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelBytes,	/* The number of bytes per pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	void				*tmpMem			/* At least (txtRect->width / 2 + 8) bytes (cannot be NULL) */
)
{
	FskErr				err			= kFskErrNone;
	const UInt8			*src;
	UInt8				*dst, *sm, *sm0, *sm1, *sm2;
	SInt32				h, smRB, txtWB;
	SetSpanProc			setSpan;
	Span				span;
	FskRectangleRecord	srcRect;
	FskPointRecord		dstLoc;

	BAIL_IF_NULL(tmpMem, err, kFskErrInvalidParameter);

	dstLoc = *dstPoint;																		/* This is set by the clipper */
	BAIL_IF_FALSE(Clip(txtRect, dstClip, &dstLoc, 1, &span, &srcRect), err, kFskErrNone);	/* Do clipping, or return if nothing left */

	/* Set span proc and edge and text colors */
	switch (dstPixelBytes) {
		case 1:	setSpan = BilevelSetSpan8;								/*  8 bit color */
				span.edgeColor.p1 = *((const UInt8*)edgeColor);
				span.textColor.p1 = *((const UInt8*)textColor);
				break;
		case 2:	setSpan = BilevelSetSpan16;								/* 16 bit color */
				span.edgeColor.p2 = *((const UInt16*)edgeColor);
				span.textColor.p2 = *((const UInt16*)textColor);
				break;
		case 4:	setSpan = BilevelSetSpan32;								/* 32 bit color */
				span.edgeColor.p4 = *((const UInt32*)edgeColor);
				span.textColor.p4 = *((const UInt32*)textColor);
				break;
		case 3:	setSpan = BilevelSetSpan24;								/* 24 bit color */
				span.edgeColor.p3[0] = ((const UInt8*)edgeColor)[0];
				span.edgeColor.p3[1] = ((const UInt8*)edgeColor)[1];
				span.edgeColor.p3[2] = ((const UInt8*)edgeColor)[2];
				span.textColor.p3[0] = ((const UInt8*)textColor)[0];
				span.textColor.p3[1] = ((const UInt8*)textColor)[1];
				span.textColor.p3[2] = ((const UInt8*)textColor)[2];
				break;
		default:
			BAIL(kFskErrUnsupportedPixelType);
	}

	span.xOffset	= srcRect.x & 7;									/* Bit offset into the first byte to the first pixel */
	span.width		= srcRect.width;									/* Width of the non-outlined character */

	src		= (const UInt8*)txtBaseAddr
			+ (srcRect.y * txtRowBytes)
			+ (srcRect.x >> 3);											/* Point to the first byte of the source */
	dst		= (UInt8*)dstBaseAddr
			+ (dstLoc.y * dstRowBytes)
			+ (dstLoc.x * dstPixelBytes);								/* Point to the first byte of the destination */
	txtWB	= (span.xOffset + span.width + 7) >> 3;						/* The number of bytes needed for each text scanline */
	smRB	= txtWB + 2;												/* The number of bytes needed for each scanline in the smear buffer */
	sm		= (UInt8 *)tmpMem;											/* The current horizontally and vertically smeared scanline */
	sm0		= sm  + smRB;												/* The previous horizontally-smeared scanline */
	sm1		= sm0 + smRB;												/* The current  horizontally-smeared scanline */
	sm2		= sm1 + smRB;												/* The next     horizontally-smeared scanline */


	/* First line */
	if (span.topBorder <= 0) {											/* Clipping of at least the edge: preload only, no rendering */
		if (span.topBorder == 0)										/* Clipping of only the edge */
			ZeroBytes(sm1, smRB);										/* Clear the previous line */
		else															/* Major clipping */
			HSmearBilevel(src - txtRowBytes, txtWB, sm1);				/* Get the previous scanline */
		HSmearBilevel(    src,               txtWB, sm2);				/* Get the current scanline */
		/* Rendering is done in the main loop */
	}
	else {																/* No clipping at all: render edge */
		ZeroBytes(sm0, smRB << 1);										/* Clear the previous and current lines */
		HSmearBilevel(    src,               txtWB, sm2);				/* Get the next scanline */
		VSmearBilevel(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+1, sm0, dst);								/* Set the span for the appropriate pixel size */
		dst += dstRowBytes;												/* Advance to the text portion */
	}


	/* Middle lines */
	h = srcRect.height;													/* Preload everything in the middle loop ...  */
	if (span.bottomBorder >= 0)	h--;									/* ... unless we're not supposed to */
	for ( ; h-- > 0; src += txtRowBytes, dst += dstRowBytes) {
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		HSmearBilevel(src + txtRowBytes, txtWB, sm2);					/* Get the next scanline */
		VSmearBilevel(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+1, src, dst);								/* Set the span for the appropriate pixel size */
	}


	/* Penultimate line */
	if (span.bottomBorder >= 0) {										/* The next line is zero */
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		ZeroBytes(sm2, smRB);											/* Clear the next line */
		VSmearBilevel(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+1, src, dst);								/* Set the span for the appropriate pixel size */
		dst += dstRowBytes;
	}


	/* Bottom line */
	if (span.bottomBorder > 0) {										/* No clipping: synthesize the bottom edge */
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		ZeroBytes(sm2, smRB);											/* Clear the next line (current line as cleared in the penultimate code) */
		VSmearBilevel(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+1, sm2, dst);								/* Set the span for the appropriate pixel size */
	}

bail:
	return err;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Grayscale								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * GrayscaleSetSpan prototype
 ********************************************************************************/

#define MakeGrayScaleSetSpan(PixelKind)		static void																	\
GrayscaleSetSpan##PixelKind(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst) {							\
	UInt32	width;	UInt8	alfa, beta;																					\
	FskName3(Fsk,PixelKind,Type)	*dst		= (FskName3(Fsk,PixelKind,Type)*)vDst,									\
									textColor	= *((FskName3(Fsk,PixelKind,Type)*)(void*)(&span->textColor)),			\
									edgeColor	= *((FskName3(Fsk,PixelKind,Type)*)(void*)(&span->edgeColor));			\
	if (span->leftBorder > 0) {												/* If the left edge isn't clipped ... */	\
		beta = (UInt8)((smr[-1] * span->blendLevel) >> 8);																\
		if (beta != 0)														/* ... and there is some edge ... */		\
			FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... blend in the edge color */			\
		dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes));		/* Advance dst */		\
	}																													\
	for (width = span->width ; width--;																					\
			smr++, src++, dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes))) {			\
		beta = (UInt8)((*smr * span->blendLevel) >> 8);																	\
		if (beta != 0) {													/* We need to do something */				\
			alfa = *src;																								\
			if (alfa == 0)	{												/* No text color */							\
				if (beta == 255)	{	*dst = edgeColor;					/* Pure edge color */						\
				} else				{	FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);	/* Blended edge color */	\
				}																										\
			} else if (alfa == 255)	{																					\
				if (span->blendLevel >= 255)																			\
					*dst = textColor;	/* Pure text color */															\
				else																									\
					FskName2(FskBlend,PixelKind)(dst, textColor, (UInt8)span->blendLevel);	/* Pure text color */		\
			} else					{	FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);	/* blend edge ... */		\
										FskName2(FskBlend,PixelKind)(dst, textColor, (UInt8)((alfa * span->blendLevel) >> 8));	/* ... and text colors */	\
	}	}	}																											\
	if (span->rightBorder > 0)	{											/* If the right edge isn't clipped ... */	\
		beta = (UInt8)((*smr * span->blendLevel) >> 8);																	\
		if (beta != 0)														/* ... and there is some edge ... */		\
			FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... then set the edge color */			\
	}																													\
}


/********************************************************************************
 * GrayscaleSetSpan instantiations
 ********************************************************************************/

MakeGrayScaleSetSpan(32ARGB)
MakeGrayScaleSetSpan(32BGRA)
MakeGrayScaleSetSpan(16RGB565SE)
MakeGrayScaleSetSpan(16RGB565DE)
MakeGrayScaleSetSpan(16RGB5515SE)
#if TARGET_RT_BIG_ENDIAN
	MakeGrayScaleSetSpan(16RGB5515DE)
#endif
MakeGrayScaleSetSpan(16RGBA4444SE)
MakeGrayScaleSetSpan(24RGB)
MakeGrayScaleSetSpan(8G)




/********************************************************************************
 * ClearHSmear
 ********************************************************************************/

#ifdef USE_MEMSTRING
	#define ClearHSmear(dst, numBytes)	memset(dst, 255, numBytes)
#else /* !USE_MEMSTRING */
static void
ClearHSmear(register UInt8 *dst, register UInt32 numBytes)
{
	while (numBytes--)
		*dst++ = 255;
}
#endif /* !USE_MEMSTRING */


/********************************************************************************
 * HSmearGrayscale
 ********************************************************************************/

static void
HSmearGrayscale(const UInt8 *src, UInt32 numBytes, Span *span, register UInt8 *dst)
{
	register UInt32	n, t;
	register UInt32	p0, p1, p2;

	/* Copy and invert with left and right [inverted] zero pads */
	*dst++ = (span->leftBorder <  0) ? ~src[-1] : 255;				/* If clipped, grab a sample before the beginning */
	for (n = numBytes; n--; )
		*dst++ = ~*src++;											/* The interval proper */
	*dst = (span->rightBorder <  0) ? ~src[0] : 255;				/* If clipped, grab a sample past the end */
	dst -= numBytes + 1;											/* Reset dst pointer */

	/* Smear */
	p0 = dst[0];													/* Prime the pump */
	p1 = dst[1];													/* We read and write each sample only once */
	t = p0; t *= p1;
	t += t >> 8; t += 1 << (8-1); t >>= 8;
	*dst++ = (UInt8)t;

	for (n = numBytes; n--; p0 = p1, p1 = p2) {
		p2 = dst[1];
		t  = p0;	t *= p1;	t *= p2;							/* Multiply */
		t += t >> 7;	t += 1 << (16-1);	t >>= 16;				/* Normalize */
		*dst++ = (UInt8)t;											/* Write inverted */
	}

	t = p0; t *= p1;
	t += t >> 8; t += 1 << (8-1); t >>= 8;
	*dst = (UInt8)t;
}


/********************************************************************************
 * VSmearGrayscale
 ********************************************************************************/

static void
VSmearGrayscale(register const UInt8 *src, register SInt32 srcRB, register UInt32 numBytes, register UInt8 *dst)
{
	register UInt32 t;

	for (src += srcRB; numBytes--; src++) {
		t  = src[-srcRB];	t *= src[0];	t *= src[srcRB];		/* Multiply */
		t += (t >> 7);	t += (1 << 15);	t >>= 16;					/* Normalize */
		t = ~t;														/* Complement */
		*dst++ = (UInt8)t;											/* Write */
	}
}


/********************************************************************************
 * FskEdgeEnhancedGrayscaleText
 ********************************************************************************/

FskErr
FskEdgeEnhancedGrayscaleText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelFormat,	/* The format of the destination pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	SInt32				blendLevel,		/* The blend level from 0 to 255 */
	void				*tmpMem			/* At least (txtRect->width * 4 + 8) bytes (cannot be NULL) */
)
{
	FskErr				err			= kFskErrNone;
	const UInt8			*src;
	UInt8				*dst, *sm, *sm0, *sm1, *sm2;
	SInt32				h, smRB, txtWB;
	SetSpanProc			setSpan;
	Span				span;
	FskRectangleRecord	srcRect;
	FskPointRecord		dstLoc;
	UInt32				dstPixelBytes;

	BAIL_IF_NULL(tmpMem, err, kFskErrInvalidParameter);

	dstLoc = *dstPoint;																		/* This is set by the clipper */
	BAIL_IF_FALSE(Clip(txtRect, dstClip, &dstLoc, 1, &span, &srcRect), err, kFskErrNone);	/* Do clipping, or return if nothing left */

	/* Set span proc and edge and text colors */
	span.blendLevel = blendLevel;
	switch (dstPixelFormat) {
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
			setSpan = GrayscaleSetSpan24RGB;							/* 24 bit color */
			dstPixelBytes = 3;
			span.edgeColor.p3[0] = ((const UInt8*)edgeColor)[0];
			span.edgeColor.p3[1] = ((const UInt8*)edgeColor)[1];
			span.edgeColor.p3[2] = ((const UInt8*)edgeColor)[2];
			span.textColor.p3[0] = ((const UInt8*)textColor)[0];
			span.textColor.p3[1] = ((const UInt8*)textColor)[1];
			span.textColor.p3[2] = ((const UInt8*)textColor)[2];
			break;

		case kFskBitmapFormat32ARGB:									/* 32 bit AXXX color */
		case kFskBitmapFormat32ABGR:
			setSpan = FskName2(GrayscaleSetSpan,fsk32ARGBFormatKind);
			dstPixelBytes = 4;
			span.edgeColor.p4 = *((const UInt32*)edgeColor);
			span.textColor.p4 = *((const UInt32*)textColor);
			break;

		case kFskBitmapFormat32BGRA:									/* 32 bit XXXA color */
		case kFskBitmapFormat32RGBA:
			setSpan = FskName2(GrayscaleSetSpan,fsk32BGRAFormatKind);
			dstPixelBytes = 4;
			span.edgeColor.p4 = *((const UInt32*)edgeColor);
			span.textColor.p4 = *((const UInt32*)textColor);
			break;

		case kFskBitmapFormat16RGB565BE:
			setSpan = FskName2(GrayscaleSetSpan,fsk16RGB565BEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGB565LE:
			setSpan = FskName2(GrayscaleSetSpan,fsk16RGB565LEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGB5515LE:
			setSpan = FskName2(GrayscaleSetSpan,fsk16RGB5515LEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGBA4444LE:
			setSpan = GrayscaleSetSpan16RGBA4444SE;						/* The same code is generated for Same and Different Endian */
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat8G:
			setSpan = GrayscaleSetSpan8G;
			dstPixelBytes = 1;
			span.edgeColor.p1 = *((const UInt8*)edgeColor);
			span.textColor.p1 = *((const UInt8*)textColor);
			break;

		default:
			BAIL(kFskErrUnsupportedPixelType);
	}

	span.width		= srcRect.width;									/* Width of the non-outlined character */

	src		= (const UInt8*)txtBaseAddr
			+ (srcRect.y * txtRowBytes)
			+ (srcRect.x);												/* Point to the first byte of the source */
	dst		= (UInt8*)dstBaseAddr
			+ (dstLoc.y * dstRowBytes)
			+ (dstLoc.x * dstPixelBytes);								/* Point to the first byte of the destination */
	txtWB	= span.width;												/* The number of bytes needed for each text scanline */
	smRB	= txtWB + 2;												/* The number of bytes needed for each scanline in the smear buffer */
	sm		= (UInt8 *)tmpMem;											/* The current horizontally and vertically smeared scanline */
	sm0		= sm  + smRB;												/* The previous horizontally-smeared scanline */
	sm1		= sm0 + smRB;												/* The current  horizontally-smeared scanline */
	sm2		= sm1 + smRB;												/* The next     horizontally-smeared scanline */


	/* First line */
	if (span.topBorder <= 0) {											/* Clipping of at least the edge: preload only, no rendering */
		if (0 != srcRect.height) {
			if (span.topBorder == 0)										/* Clipping of only the edge */
				ClearHSmear(sm1, smRB);										/* Clear the previous line */
			else															/* Major clipping */
				HSmearGrayscale(src - txtRowBytes, txtWB, &span, sm1);		/* Get the previous scanline */
			HSmearGrayscale(    src,               txtWB, &span, sm2);		/* Get the current scanline */
		}
		else {																/* entirely glyph clipped, only bottom border */
			HSmearGrayscale(src - txtRowBytes, txtWB, &span, sm1);			/* load bottom line */
			ClearHSmear(sm2, smRB);											/* clear a fake source buffer */
			goto bottom;													/* render only bottom border */
		}
		/* Rendering is done in the main loop */
	}
	else {																/* No clipping at all: render edge */
		ClearHSmear(sm0, smRB << 1);									/* Clear the previous and current lines */
		HSmearGrayscale(    src,               txtWB, &span, sm2);		/* Get the next scanline */
		VSmearGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
		ZeroBytes(sm0, smRB);											/* Clear a fake src buffer */
		(*setSpan)(&span, sm+1, sm0, dst);								/* Set the span for the appropriate pixel size */
		dst += dstRowBytes;												/* Advance to the text portion */
	}


	/* Middle lines */
	h = srcRect.height;													/* Preload everything in the middle loop ...  */
	if (span.bottomBorder >= 0)	h--;									/* ... unless we're not supposed to */
	for ( ; h-- > 0; src += txtRowBytes, dst += dstRowBytes) {
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		HSmearGrayscale(src + txtRowBytes, txtWB, &span, sm2);			/* Get the next scanline */
		VSmearGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
		(*setSpan)(&span, sm+1, src, dst);								/* Set the span for the appropriate pixel size */
	}


	/* Penultimate line */
	if (span.bottomBorder >= 0) {										/* The next line is zero */
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		ClearHSmear(sm2, smRB);											/* Clear the next line */
		VSmearGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
		(*setSpan)(&span, sm+1, src, dst);								/* Set the span for the appropriate pixel size */
		dst += dstRowBytes;
	}

bottom:
	/* Bottom line */
	if (span.bottomBorder > 0) {										/* No clipping: synthesize the bottom edge */
		CopyBytes(sm1, smRB << 1, sm0);									/* Shift smear lines up to clear the way for next the line */
		ClearHSmear(sm2, smRB);											/* Clear the next line (current line as cleared in the penultimate code) */
		VSmearGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
		ZeroBytes(sm2, smRB);											/* Clear a fake src buffer */
		(*setSpan)(&span, sm+1, sm2, dst);								/* Set the span for the appropriate pixel size */
	}

bail:
	return err;
}





#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							DoubleGrayscale								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * DoubleGrayscaleSetSpan prototype
 ********************************************************************************/

#define MakeDoubleGrayscaleSetSpan(PixelKind)		static void															\
DoubleGrayscaleSetSpan##PixelKind(const Span *span, const UInt8 *smr, const UInt8 *src, void *vDst) {					\
	UInt32	width;	UInt8	alfa, beta;																					\
	FskName3(Fsk,PixelKind,Type)	*dst		= (FskName3(Fsk,PixelKind,Type)*)vDst,									\
									textColor	= *((FskName3(Fsk,PixelKind,Type)*)(void*)(&span->textColor)),			\
									edgeColor	= *((FskName3(Fsk,PixelKind,Type)*)(void*)(&span->edgeColor));			\
	if (span->leftBorder == 2) {																						\
		beta = (UInt8)((smr[-2] * span->blendLevel) >> 8);																\
		if (beta != 0)														/* ... and there is some edge ... */		\
			FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... blend in the edge color */			\
		dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes));		/* Advance dst */		\
	}																													\
	if (span->leftBorder >= 1) {																						\
		beta = (UInt8)((smr[-1] * span->blendLevel) >> 8);																\
		if (beta != 0)														/* ... and there is some edge ... */		\
			FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... blend in the edge color */			\
		dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes));		/* Advance dst */		\
	}																													\
	for (width = span->width ; width--;																					\
			smr++, src++, dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes))) {			\
		beta = (UInt8)((*smr * span->blendLevel) >> 8);																	\
		if (beta != 0) {													/* We need to do something */				\
			alfa = *src;																								\
			if (alfa == 0)	{												/* No text color */							\
				if (beta == 255)	{	*dst = edgeColor;					/* Pure edge color */						\
				} else				{	FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);	/* Blended edge color */	\
				}																										\
			} else if (alfa == 255)	{																					\
				if (span->blendLevel >= 255)																			\
					*dst = textColor;	/* Pure text color */															\
				else																									\
					FskName2(FskBlend,PixelKind)(dst, textColor, (UInt8)span->blendLevel);	/* Pure text color */		\
			} else					{	FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);	/* blend edge ... */		\
										FskName2(FskBlend,PixelKind)(dst, textColor, (UInt8)((alfa * span->blendLevel) >> 8));	/* ... and text colors */	\
	}	}	}																											\
	if (span->rightBorder > 0) {																						\
		beta = (UInt8)((*smr++ * span->blendLevel) >> 8);																\
		if (beta != 0)														/* ... and there is some edge ... */		\
			FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... then set the edge color */			\
		dst = (FskName3(Fsk,PixelKind,Type)*)((char*)dst + FskName3(fsk,PixelKind,Bytes));		/* Advance dst */		\
		beta = (UInt8)((*smr * span->blendLevel) >> 8);																	\
		if (	(span->rightBorder > 1)										/* If the right edge isn't clipped ... */	\
			&&	(beta != 0)													/* ... and there is some edge ... */		\
		) FskName2(FskBlend,PixelKind)(dst, edgeColor, beta);				/* ... then set the edge color */			\
	}																													\
}


/********************************************************************************
 * DoubleGrayscaleSetSpan instantiations
 ********************************************************************************/

MakeDoubleGrayscaleSetSpan(32ARGB)
MakeDoubleGrayscaleSetSpan(32BGRA)
MakeDoubleGrayscaleSetSpan(16RGB565SE)
MakeDoubleGrayscaleSetSpan(16RGB565DE)
MakeDoubleGrayscaleSetSpan(16RGB5515SE)
#if TARGET_RT_BIG_ENDIAN
	MakeDoubleGrayscaleSetSpan(16RGB5515DE)
#endif
MakeDoubleGrayscaleSetSpan(16RGBA4444SE)
MakeDoubleGrayscaleSetSpan(24RGB)
MakeDoubleGrayscaleSetSpan(8G)


#define DOUBLE_MULT			0
#define DOUBLE_MAX			0
#define DOUBLE_NONZEROMAX	1	/* This is the clear winner */


/********************************************************************************
 * HSmearDoubleGrayscale
 * We store the complement of the smeared alpha.
 ********************************************************************************/

static void
HSmearDoubleGrayscale(const UInt8 *src, UInt32 numBytes, Span *span, register UInt8 *dst)
{

#if DOUBLE_MULT

	register UInt32	n, t;
	register UInt32	p0, p1, p2, p3, p4;

	/* Copy and invert with left and right [inverted] zero pads */
	*dst++ = (span->leftBorder < -1) ? ~src[-2] : 255;			/* If clipped, grab some samples before the beginning */
	*dst++ = (span->leftBorder <  0) ? ~src[-1] : 255;
	for (n = numBytes; n--; )
		*dst++ = ~*src++;										/* The interval proper */
	*dst++ = (span->rightBorder <  0) ? ~src[0] : 255;			/* If clipped, grab some samples past the end */
	*dst   = (span->rightBorder < -1) ? ~src[1] : 255;
	dst -= numBytes + 3;										/* Reset dst pointer */

	/* Load the first few pixels */
	p0 = 255;
	p1 = dst[0];												/* Prime the pump */
	p2 = dst[1];
	p3 = dst[2];
	p4 = dst[3];												/* We read and write each sample only once */

	/* First pixel */
	*dst++ = p3;												/* Write inverted [-2] */

	/* Second pixel */
	t = p3; t *= p4;											/* Multiply */
	t += t >> 8;	t += 1 << (8-1);	t >>= 8;				/* Normalize to 255 */
	*dst++ = (UInt8)t;											/* Write inverted [-1] */

	/* Pump the middle pixels through the smearing machine */
	for (n = numBytes; n--; p0 = p1, p1 = p2, p2 = p3, p3 = p4) {
		p4 = dst[2];
		t = p0;	t *= p1; t *= p3; t *= p4;						/* Multiply 4 of 5 factors */
		t += t >> 6; t >>= 23; t++; t >>= 1;					/* Normalize to 256 */
		t *= p2;												/* Multiply the 5th factor */
		t += 1 << (8-1); t >>= 8;								/* Normalize to 255 */
		*dst++ = (UInt8)t;										/* Write inverted */
	}

	/* Penultimate pixel */
	t = p0; t *= p1;											/* Multiply */
	t += t >> 8;	t += 1 << (8-1);	t >>= 8;				/* Normalize to 255 */
	*dst++ = (UInt8)t;											/* Write inverted [n] */

	/* Smear the last pixel */
	*dst = p1;													/* Write inverted [n+1] */

#elif DOUBLE_MAX

	register UInt32	n, t;
	register UInt32	p0, p1, p2, p3, p4;

	/* Copy with left and right zero pads */
	*dst++ = (span->leftBorder < -1) ? src[-2] : 0;				/* If clipped, grab some samples before the beginning */
	*dst++ = (span->leftBorder <  0) ? src[-1] : 0;
	for (n = numBytes; n--; )
		*dst++ = *src++;										/* The interval proper */
	*dst++ = (span->rightBorder <  0) ? src[0] : 0;				/* If clipped, grab some samples past the end */
	*dst   = (span->rightBorder < -1) ? src[1] : 0;
	dst -= numBytes + 3;										/* Reset dst pointer */

	/* Load the first few pixels */
	p0 = p1 = p2 = 0;											/* We already set dst[0] = dst[1] = ~0; */
	p3 = dst[2];												/* Prime the pump */
	p4 = dst[3];												/* We read and write each sample only once */

	/* First pixel */
	*dst++ = (UInt8)p3;											/* Write [-2] */

	/* Second pixel */
	t = (p3 > p4) ? p3 : p4;
	*dst++ = (UInt8)t;											/* Write [-1] */

	/* Pump the middle pixels through the smearing machine */
	for (n = numBytes; n--; p0 = p1, p1 = p2, p2 = p3, p3 = p4) {
		p4 = dst[2];
		t = p0;
		if (t < p1)	t = p1;
		if (t < p2)	t = p2;
		if (t < p3)	t = p3;
		if (t < p4)	t = p4;
		*dst++ = (UInt8)t;										/* Write */
	}

	/* Penultimate pixel */
	t = (p0 > p1) ? p0 : p1;
	*dst++ = (UInt8)t;											/* Write [n] */

	/* Smear the last pixel */
	*dst = (UInt8)p1;											/* Write [n+1] */

#elif DOUBLE_NONZEROMAX

	/* Copy with left and right zero pads */
	*dst++ = (span->leftBorder < -1) ? src[-2] : 0;				/* If clipped, grab some samples before the beginning */
	*dst++ = (span->leftBorder <  0) ? src[-1] : 0;
	for ( ; numBytes--; )
		*dst++ = *src++;										/* The interval proper */
	*dst++ = (span->rightBorder <  0) ? src[0] : 0;				/* If clipped, grab some samples past the end */
	*dst   = (span->rightBorder < -1) ? src[1] : 0;

#else
	#error Must specify an algorithm for double outlines
#endif /* DOUBLE_MAX */
}


/********************************************************************************
 * VSmearDoubleGrayscale
 ********************************************************************************/

static void
VSmearDoubleGrayscale(register const UInt8 *src, register SInt32 srcRB, register UInt32 numBytes, register UInt8 *dst)
{

#if DOUBLE_MULT

	register UInt32 t;
	SInt32	srcRB2, srcRB3, srcRB4;

	srcRB2 = srcRB  + srcRB;
	srcRB3 = srcRB2 + srcRB;
	srcRB4 = srcRB3 + srcRB;

	for ( ; numBytes--; src++) {
		t = src[0]; t *= src[srcRB]; t *= src[srcRB3]; t *= src[srcRB4];	/* Multiply 4 of 5 factors */
		t += t >> 6; t >>= 23; t++; t >>= 1;								/* Normalize to 256 */
		t *= src[srcRB2];													/* Multiply the 5th factor */
		t += 1 << (8-1); t >>= 8;											/* Normalize to 255 */
		t = ~t;																/* Complement */
		*dst++ = (UInt8)t;													/* Write */
	}

#elif DOUBLE_MAX

	register UInt32 t;
	SInt32	srcRB2, srcRB3, srcRB4;

	srcRB2 = srcRB  + srcRB;
	srcRB3 = srcRB2 + srcRB;
	srcRB4 = srcRB3 + srcRB;

	for ( ; numBytes--; src++) {
		t = src[0];
		if (t < src[srcRB])		t = src[srcRB];
		if (t < src[srcRB2])	t = src[srcRB2];
		if (t < src[srcRB3])	t = src[srcRB3];
		if (t < src[srcRB4])	t = src[srcRB4];
		*dst++ = (UInt8)t;													/* Write */
	}

#elif DOUBLE_NONZEROMAX

	register UInt32 t;
	SInt32	srcRB2, srcRB3, srcRB4;

	srcRB2 = srcRB  + srcRB;
	srcRB3 = srcRB2 + srcRB;
	srcRB4 = srcRB3 + srcRB;

	numBytes -=4;

	/* First [border] pixel */
	if (	( src[srcRB +0] | src[srcRB +1]
			| src[srcRB2+0] | src[srcRB2+1]
			| src[srcRB3+0] | src[srcRB3+1]
			) != 0														/* Something in the inner ring of 9 is not totally transparent */
	) {
		t = 255;														/* Make the adjacent boundary totally opaque */
	} else {
		t = src[0+0];
		if (t < src[0     +1])		t = src[0     +1];
		if (t < src[0     +2])		t = src[0     +2];
		if (t < src[srcRB +2])		t = src[srcRB +2];
		if (t < src[srcRB2+2])		t = src[srcRB2+2];
		if (t < src[srcRB3+2])		t = src[srcRB3+2];
		if (t < src[srcRB4+0])		t = src[srcRB4+0];
		if (t < src[srcRB4+1])		t = src[srcRB4+1];
		if (t < src[srcRB4+2])		t = src[srcRB4+2];
	}
	*dst++ = (UInt8)t;													/* Write first pixel */

	/* Second [border] pixel */
	if (	( src[srcRB +0] | src[srcRB +1] | src[srcRB +2]
			| src[srcRB2+0] | src[srcRB2+1] | src[srcRB2+2]
			| src[srcRB3+0] | src[srcRB3+1] | src[srcRB3+2]
			) != 0														/* Something in the inner ring of 9 is not totally transparent */
	) {
		t = 255;														/* Make the adjacent boundary totally opaque */
	} else {
		t = src[0+0];
		if (t < src[0     +1])		t = src[0     +1];
		if (t < src[0     +2])		t = src[0     +2];
		if (t < src[0     +3])		t = src[0     +3];
		if (t < src[srcRB +3])		t = src[srcRB +3];
		if (t < src[srcRB2+3])		t = src[srcRB2+3];
		if (t < src[srcRB3+3])		t = src[srcRB3+3];
		if (t < src[srcRB4+0])		t = src[srcRB4+0];
		if (t < src[srcRB4+1])		t = src[srcRB4+1];
		if (t < src[srcRB4+2])		t = src[srcRB4+2];
		if (t < src[srcRB4+3])		t = src[srcRB4+3];
	}
	*dst++ = (UInt8)t;													/* Write second pixel */


	for ( ; numBytes--; src++) {

		if (	( src[srcRB +1] | src[srcRB +2] | src[srcRB +3]
				| src[srcRB2+1] | src[srcRB2+2] | src[srcRB2+3]
				| src[srcRB3+1] | src[srcRB3+2] | src[srcRB3+3]
				) != 0														/* Something in the inner ring of 9 is not totally transparent */
		) {
			t = 255;														/* Make the adjacent boundary totally opaque */
		} else {
			t = src[0+0];
			if (t < src[0     +1])		t = src[0     +1];
			if (t < src[0     +2])		t = src[0     +2];
			if (t < src[0     +3])		t = src[0     +3];
			if (t < src[0     +4])		t = src[0     +4];
			if (t < src[srcRB +0])		t = src[srcRB +0];
			if (t < src[srcRB +4])		t = src[srcRB +4];
			if (t < src[srcRB2+0])		t = src[srcRB2+0];
			if (t < src[srcRB2+4])		t = src[srcRB2+4];
			if (t < src[srcRB3+0])		t = src[srcRB3+0];
			if (t < src[srcRB3+4])		t = src[srcRB3+4];
			if (t < src[srcRB4+0])		t = src[srcRB4+0];
			if (t < src[srcRB4+1])		t = src[srcRB4+1];
			if (t < src[srcRB4+2])		t = src[srcRB4+2];
			if (t < src[srcRB4+3])		t = src[srcRB4+3];
			if (t < src[srcRB4+4])		t = src[srcRB4+4];
		}

		*dst++ = (UInt8)t;													/* Write */
	}

	/* Penultimate [border] pixel */
	if (	( src[srcRB +1] | src[srcRB +2] | src[srcRB +3]
			| src[srcRB2+1] | src[srcRB2+2] | src[srcRB2+3]
			| src[srcRB3+1] | src[srcRB3+2] | src[srcRB3+3]
			) != 0															/* Something in the inner ring of 9 is not totally transparent */
	) {
		t = 255;															/* Make the adjacent boundary totally opaque */
	} else {
		t = src[0+0];
		if (t < src[0     +1])		t = src[0     +1];
		if (t < src[0     +2])		t = src[0     +2];
		if (t < src[0     +3])		t = src[0     +3];
		if (t < src[srcRB +0])		t = src[srcRB +0];
		if (t < src[srcRB2+0])		t = src[srcRB2+0];
		if (t < src[srcRB3+0])		t = src[srcRB3+0];
		if (t < src[srcRB4+0])		t = src[srcRB4+0];
		if (t < src[srcRB4+1])		t = src[srcRB4+1];
		if (t < src[srcRB4+2])		t = src[srcRB4+2];
		if (t < src[srcRB4+3])		t = src[srcRB4+3];
	}
	*dst++ = (UInt8)t;														/* Write penultimate pixel */

	/* Last [border] pixel */
	src++;
	if (	( src[srcRB +1] | src[srcRB +2]
			| src[srcRB2+1] | src[srcRB2+2]
			| src[srcRB3+1] | src[srcRB3+2]
			) != 0															/* Something in the inner ring of 9 is not totally transparent */
	) {
		t = 255;															/* Make the adjacent boundary totally opaque */
	} else {
		t = src[0+0];
		if (t < src[0     +1])		t = src[0     +1];
		if (t < src[0     +2])		t = src[0     +2];
		if (t < src[srcRB +0])		t = src[srcRB +0];
		if (t < src[srcRB2+0])		t = src[srcRB2+0];
		if (t < src[srcRB3+0])		t = src[srcRB3+0];
		if (t < src[srcRB4+0])		t = src[srcRB4+0];
		if (t < src[srcRB4+1])		t = src[srcRB4+1];
		if (t < src[srcRB4+2])		t = src[srcRB4+2];
	}
	*dst++ = (UInt8)t;														/* Write last pixel */

#else
	#error Must specify an algorithm for double outlines
#endif /* DOUBLE_MAX */

}


#if DOUBLE_MULT
	#define ClearH2Smear	ClearHSmear
#else /* DOUBLE_MAX */
	#define ClearH2Smear	ZeroBytes
#endif /* DOUBLE_MAX */


/********************************************************************************
 * FskEdgeEnhancedDoubleGrayscaleText
 ********************************************************************************/

FskErr
FskEdgeEnhancedDoubleGrayscaleText(
	const void			*txtBaseAddr,	/* The base address of the text */
	SInt32				txtRowBytes,	/* Y-stride for the text */
	FskConstRectangle	txtRect,		/* Text rect */
	void				*dstBaseAddr,	/* The base address of the destination */
	UInt32				dstPixelFormat,	/* The format of the destination pixel */
	SInt32				dstRowBytes,	/* Y-stride */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	const void			*textColor,		/* The color of the internal glyph, preformatted for the dst */
	const void			*edgeColor,		/* The color of the edge of the glyph, preformatted for the dst */
	SInt32				blendLevel,		/* The blend level from 0 to 255 */
	void				*tmpMem			/* At least ((txtRect->width + 4) * 6) bytes (cannot be NULL) */
)
{
	FskErr				err		= kFskErrNone;
	const UInt8			*src;
	UInt8				*dst, *sm, *sm0, *sm1, *sm2, *sm3, *sm4;
	SInt32				h, smRB, txtWB;
	SetSpanProc			setSpan;
	Span				span;
	FskRectangleRecord	srcRect;
	FskPointRecord		dstLoc;
	UInt32				dstPixelBytes;

	BAIL_IF_NULL(tmpMem, err, kFskErrInvalidParameter);

	dstLoc = *dstPoint;																		/* This is set by the clipper */
	BAIL_IF_FALSE(Clip(txtRect, dstClip, &dstLoc, 2, &span, &srcRect), err, kFskErrNone);	/* Do clipping, or return if nothing left */

	/* Set span proc and edge and text colors */
	span.blendLevel = blendLevel;
	switch (dstPixelFormat) {
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
			setSpan = DoubleGrayscaleSetSpan24RGB;					/* 24 bit color */
			dstPixelBytes = 3;
			span.edgeColor.p3[0] = ((const UInt8*)edgeColor)[0];
			span.edgeColor.p3[1] = ((const UInt8*)edgeColor)[1];
			span.edgeColor.p3[2] = ((const UInt8*)edgeColor)[2];
			span.textColor.p3[0] = ((const UInt8*)textColor)[0];
			span.textColor.p3[1] = ((const UInt8*)textColor)[1];
			span.textColor.p3[2] = ((const UInt8*)textColor)[2];
			break;

		case kFskBitmapFormat32ARGB:								/* 32 bit AXXX color */
		case kFskBitmapFormat32ABGR:
			setSpan = FskName2(DoubleGrayscaleSetSpan,fsk32ARGBFormatKind);
			dstPixelBytes = 4;
			span.edgeColor.p4 = *((const UInt32*)edgeColor);
			span.textColor.p4 = *((const UInt32*)textColor);
			break;

		case kFskBitmapFormat32BGRA:								/* 32 bit XXXA color */
		case kFskBitmapFormat32RGBA:
			setSpan = FskName2(DoubleGrayscaleSetSpan,fsk32BGRAFormatKind);
			dstPixelBytes = 4;
			span.edgeColor.p4 = *((const UInt32*)edgeColor);
			span.textColor.p4 = *((const UInt32*)textColor);
			break;

		case kFskBitmapFormat16RGB565BE:
			setSpan = FskName2(DoubleGrayscaleSetSpan,fsk16RGB565BEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGB565LE:
			setSpan = FskName2(DoubleGrayscaleSetSpan,fsk16RGB565LEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGB5515LE:
			setSpan = FskName2(DoubleGrayscaleSetSpan,fsk16RGB5515LEFormatKind);
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat16RGBA4444LE:
			setSpan = DoubleGrayscaleSetSpan16RGBA4444SE;					/* The same code is generated for Same and Different Endian */
			dstPixelBytes = 2;
			span.edgeColor.p2 = *((const UInt16*)edgeColor);
			span.textColor.p2 = *((const UInt16*)textColor);
			break;

		case kFskBitmapFormat8G:
			setSpan = DoubleGrayscaleSetSpan8G;
			dstPixelBytes = 1;
			span.edgeColor.p1 = *((const UInt8*)edgeColor);
			span.textColor.p1 = *((const UInt8*)textColor);
			break;

		default:
			BAIL(kFskErrUnsupportedPixelType);
	}

	span.width		= srcRect.width;											/* Width of the non-outlined character */

	src		= (const UInt8*)txtBaseAddr
			+ (srcRect.y * txtRowBytes)
			+ (srcRect.x);														/* Point to the first byte of the source */
	dst		= (UInt8*)dstBaseAddr
			+ (dstLoc.y * dstRowBytes)
			+ (dstLoc.x * dstPixelBytes);										/* Point to the first byte of the destination */
	txtWB	= span.width;														/* The number of bytes needed for each text scanline */
	smRB	= txtWB + 4;														/* The number of bytes needed for each scanline in the smear buffer */
	sm		= (UInt8 *)tmpMem;													/* The current horizontally and vertically smeared scanline */
	sm0		= sm  + smRB;														/* The previous horizontally-smeared scanline */
	sm1		= sm0 + smRB;														/* The current  horizontally-smeared scanline */
	sm2		= sm1 + smRB;														/* The next     horizontally-smeared scanline */
	sm3		= sm2 + smRB;														/* The next     horizontally-smeared scanline */
	sm4		= sm3 + smRB;														/* The next     horizontally-smeared scanline */


	ClearH2Smear(sm0, smRB * 5);												/* Clear all lines */

	/* First few lines */
	switch (span.topBorder) {
		case -2:	/* Two lines of the glyph are clipped */
			HSmearDoubleGrayscale(src - 2*txtRowBytes, txtWB, &span, sm1);		/* Smear into line 1 */
			/* Fall through */
		case -1:	/* One line  of the glyph is clipped */
			HSmearDoubleGrayscale(src - 1*txtRowBytes, txtWB, &span, sm2);		/* Smear into line 2 */
			/* Fall through */
		case 0:		/* Border is entirely clipped out */
		case 1:		/* Clip one line of the border */
			HSmearDoubleGrayscale(src + 0*txtRowBytes, txtWB, &span, sm3);		/* Smear into line 3 */
		case_2_end:
			if (txtRect->height > 1)
				HSmearDoubleGrayscale(src + 1*txtRowBytes, txtWB, &span, sm4);	/* Smear into line 4 */
			else
				ClearH2Smear(sm4, smRB);										/* Or clear line 4 if there is no more source */
			if (span.topBorder < 1)
				break;	/* Case 0 ends without writing anything */
		/* case 1 and 2 continue on to write a line: */
			VSmearDoubleGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
			ZeroBytes(sm0, smRB);												/* Clear a fake src buffer */
			(*setSpan)(&span, sm+2, sm0, dst);									/* Set the span for the appropriate pixel size */
			dst += dstRowBytes;													/* Advance to the glyph portion */
			break;
		case 2:		/* Full border */
			HSmearDoubleGrayscale(src + 0*txtRowBytes, txtWB, &span, sm4);		/* Smear into line 4 */
			VSmearDoubleGrayscale(sm0, smRB, smRB, sm);							/* Smear vertically */
			ZeroBytes(sm0, smRB);												/* Clear a fake src buffer */
			(*setSpan)(&span, sm+2, sm0, dst);									/* Set the span for the appropriate pixel size */
			dst += dstRowBytes;													/* Advance to the next line */
			CopyBytes(sm1, smRB << 2, sm0);										/* Shift 4 smear lines up to clear the way for next the line */
			goto case_2_end;
	}


	/* Middle lines */
	h = srcRect.height;															/* Preload everything in the middle loop ...  */
	if      (span.bottomBorder >=  0)	h -= 2;									/* ... unless we're not supposed to */
	else if (span.bottomBorder == -1)	h--;
	for ( ; h-- > 0; src += txtRowBytes, dst += dstRowBytes) {
		CopyBytes(sm1, smRB << 2, sm0);											/* Shift 4 smear lines up to clear the way for next the line */
		HSmearDoubleGrayscale(src + (txtRowBytes<<1), txtWB, &span, sm4);		/* Get the next scanline */
		VSmearDoubleGrayscale(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+2, src, dst);										/* Set the span for the appropriate pixel size */
	}

	/* Trailing lines */
	if (span.bottomBorder < -1)	goto endTrail;									/* -2 border ends here */
		CopyBytes(sm1, smRB << 2, sm0);											/* Shift 4 smear lines up to clear the way for next the line */
		ClearH2Smear(sm4, smRB);												/* Clear the next line */
		VSmearDoubleGrayscale(sm0, smRB, smRB, sm);								/* Smear vertically */
		(*setSpan)(&span, sm+2, src + 0*txtRowBytes, dst);						/* Set the span for the appropriate pixel size */
	if (span.bottomBorder < 0)	goto endTrail;									/* -1 border ends here */
		dst += dstRowBytes;
		CopyBytes(sm1, smRB << 2, sm0);											/* Shift smear lines up and duplicate the cleared line */
		VSmearDoubleGrayscale(sm0, smRB, smRB, sm);								/* Smear vertically */
		if (txtRect->height > 1)
			(*setSpan)(&span, sm+2, src + 1*txtRowBytes, dst);					/* Set the span for the appropriate pixel size */
		else {
			ZeroBytes(sm0, smRB);												/* Clear a fake src buffer */
			(*setSpan)(&span, sm+2, sm0, dst);									/* Set the span for the appropriate pixel size */
		}
	if (span.bottomBorder < 1)	goto endTrail;									/* 0 border ends here */
		dst += dstRowBytes;
		CopyBytes(sm1, smRB << 2, sm0);											/* Shift smear lines up and duplicate the cleared line */
		VSmearDoubleGrayscale(sm0, smRB, smRB, sm);								/* Smear vertically */
		ZeroBytes(sm0, smRB);													/* Clear a fake src buffer */
		(*setSpan)(&span, sm+2, sm0, dst);										/* Set the span for the appropriate pixel size */
	if (span.bottomBorder < 2)	goto endTrail;									/* 1 border ends here */
		dst += dstRowBytes;
		CopyBytes(sm1, smRB << 2, sm0);											/* Shift smear lines up and duplicate the cleared line */
		VSmearDoubleGrayscale(sm0, smRB, smRB, sm);								/* Smear vertically */
		ZeroBytes(sm0, smRB);													/* Clear a fake src buffer */
		(*setSpan)(&span, sm+2, sm0, dst);										/* Set the span for the appropriate pixel size */
	endTrail:																	/* 2 border ends here */
bail:
	return err;
}




#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskTransferEdgeEnhancedBilevelText
 ********************************************************************************/

FskErr
FskTransferEdgeEnhancedBilevelText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGB	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGB	edgeColor
)
{
	FskErr		err;
	FskMemPtr	scratchMem;
	UInt32		txco, ejco;

	BAIL_IF_ERR(err = FskMemPtrNew(txtBM->bounds.width * 4 + 8, &scratchMem));
	FskConvertColorRGBToBitmapPixel(textColor, dstBM->pixelFormat, &txco);
	FskConvertColorRGBToBitmapPixel(edgeColor, dstBM->pixelFormat, &ejco);
	(void)FskBitmapReadBegin((FskBitmap)txtBM, NULL, NULL, NULL);
	(void)FskBitmapWriteBegin(          dstBM, NULL, NULL, NULL);
	err = FskEdgeEnhancedBilevelText(txtBM->bits, txtBM->rowBytes, txtRect,
			dstBM->bits, dstBM->depth>>3, dstBM->rowBytes, dstPoint, dstClip,
			&txco, &ejco, scratchMem);
	(void)FskBitmapWriteEnd(          dstBM);
	(void)FskBitmapReadEnd((FskBitmap)txtBM);
	FskMemPtrDispose(scratchMem);

bail:
	return err;
}


/********************************************************************************
 * FskTransferEdgeEnhancedGrayscaleText
 ********************************************************************************/

FskErr
FskTransferEdgeEnhancedGrayscaleText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGBA	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGBA	edgeColor,
	SInt32				blendLevel
)
{
	FskErr		err;
	FskMemPtr	scratchMem;
	UInt32		txco, ejco;

	BAIL_IF_ERR(err = FskMemPtrNew(txtBM->bounds.width * 4 + 8, &scratchMem));
	FskConvertColorRGBAToBitmapPixel(textColor, dstBM->pixelFormat, &txco);
	FskConvertColorRGBAToBitmapPixel(edgeColor, dstBM->pixelFormat, &ejco);
	(void)FskBitmapReadBegin((FskBitmap)txtBM, NULL, NULL, NULL);
	(void)FskBitmapWriteBegin(          dstBM, NULL, NULL, NULL);
	err = FskEdgeEnhancedGrayscaleText(txtBM->bits, txtBM->rowBytes, txtRect,
			dstBM->bits, dstBM->pixelFormat, dstBM->rowBytes, dstPoint, dstClip,
			&txco, &ejco, blendLevel, scratchMem);
	(void)FskBitmapWriteEnd(          dstBM);
	(void)FskBitmapReadEnd((FskBitmap)txtBM);
	FskMemPtrDispose(scratchMem);

bail:
	return err;
}


/********************************************************************************
 * FskTransferEdgeEnhancedDoubleGrayscaleText
 ********************************************************************************/

FskErr
FskTransferEdgeEnhancedDoubleGrayscaleText(
	FskConstBitmap		txtBM,			/* The source bitmap containing the bilevel text */
	FskConstRectangle	txtRect,		/* Text rect */
	FskBitmap			dstBM,			/* The destination bitmap */
	FskConstPoint		dstPoint,		/* Location of text */
	FskConstRectangle	dstClip,		/* Clip to this rect */
	FskConstColorRGBA	textColor,		/* The color of the internal glyph, preformatted for the dst */
	FskConstColorRGBA	edgeColor,
	SInt32				blendLevel
)
{
	FskErr		err;
	FskMemPtr	scratchMem;
	UInt32		txco, ejco;

	BAIL_IF_ERR(err = FskMemPtrNew((txtBM->bounds.width + 4) * 6, &scratchMem));
	FskConvertColorRGBAToBitmapPixel(textColor, dstBM->pixelFormat, &txco);
	FskConvertColorRGBAToBitmapPixel(edgeColor, dstBM->pixelFormat, &ejco);
	(void)FskBitmapReadBegin((FskBitmap)txtBM, NULL, NULL, NULL);
	(void)FskBitmapWriteBegin(          dstBM, NULL, NULL, NULL);
	err = FskEdgeEnhancedDoubleGrayscaleText(txtBM->bits, txtBM->rowBytes, txtRect,
			dstBM->bits, dstBM->pixelFormat, dstBM->rowBytes, dstPoint, dstClip,
			&txco, &ejco, blendLevel, scratchMem);
	(void)FskBitmapWriteEnd(          dstBM);
	(void)FskBitmapReadEnd((FskBitmap)txtBM);
	FskMemPtrDispose(scratchMem);

bail:
	return err;
}
