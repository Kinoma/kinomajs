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
#ifndef __FSKSPAN__
#define __FSKSPAN__

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"			/* FskFixed */
#endif /* __FSKFIXEDMATH__ */
#ifndef __FSKGRAPHICS_H__
# include "FskGraphics.h"			/* FskConstColorRGBA */
#endif /* __FSKGRAPHICS_H__ */
#ifndef __FSKBITMAP__
# include "FskBitmap.h"				/* FskBitmap */
#endif /* __FSKBITMAP__ */
#ifndef __FSKPIXELOPS__
# include "FskPixelOps.h"			/* FskPixelType */
#endif /* __FSKPIXELOPS__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Forward declarations */
typedef struct FskSpan			FskSpan;
typedef struct FskEdge			FskEdge;
struct FskColorSource;


/********************************************************************************
 ********************************************************************************
 ** Span procs
 ********************************************************************************
 ********************************************************************************/

/********************************************************************************
 * Init span - The span is initialized with default values when called. It is up to the InitSpan procedure
 * to modify these as needed. Here's a prototype
 *
 *	FskErr
 *	MyInitSpan(const void *data, FskSpan *span)
 *	{
 *		const MyData	*d	= (const MyData*)data;
 *		FskErr			err	= kFskErrNone;
 *
 *		err = FskMemPtrNewClear(sizeof(MySpanData), (FskMemPtr*)(&span->spanData)));	// optional
 *		if (err != kFskErrNone) goto bail;
 *		disposeSpanData	= (FskDisposeSpanDataProc)FskMemPtrDispose;	// necessary if span->spanData is allocated
 *
 *		span->fillColor		= d->fillColor;							// as needed
 *		span->altColor		= d->altColor;							// as needed
 *		span->edgeBytes		+= kMyExtraEdgeBytes;					// if extra bytes are needed
 *		span->fill			= MyFillSpan;							// definitely needed
 *		span->set			= MySetSpan;							// only if needed
 *		span->initEdge		= MyInitEdge;							// only if needed
 *		span->advanceEdge	= MyAdvanceEdge;						// only if needed
 *
 *		/// (*span->setPixel) is available for use by MyFillSpan(); do not replace
 *
 *	bail:
 *		return err;
 *	}
 ********************************************************************************/

typedef FskErr	(*FskInitSpanProc)(FskSpan *span, FskBitmap dstBM, const FskFixedMatrix3x2 *M, UInt32 quality, const struct FskColorSource *cs);

/********************************************************************************
 * Dispose Span Data (optional) - Typically, this is (FskDisposeSpanDataProc)FskMemPtrDispose;
 ********************************************************************************/

typedef void	(*FskDisposeSpanDataProc)(void *spanData);

/* Fill Span (mandatory) - This procedure is called to fill the span staring at "p", for "d" pixels. */
typedef void	(*FskFillSpanProc)(FskSpan *span);

/* Set Span (optional)  - "p" and "dx" are set prior to this call, but if any other span setup needs to be made, it is done here */
typedef void	(*FskSetSpanProc)(const FskEdge *L, const FskEdge *R, SInt32 x, SInt32 y, FskSpan *span);

/* Set Pixel - set the pixel */
typedef void					(*FskSetPixelProc)(FskSpan *span);
typedef void	(*FskBlendPixelProc)(FskSpan *span, UInt8 opacity);


/********************************************************************************
 * Edge procs
 ********************************************************************************/

/* Advance Edge (optional) - x += dx will alread be called, but if any other edge advancement is needed, it is done here */
typedef void	(*FskAdvanceEdgeProc)(FskEdge *e);

/* Init Edge (optional) - "top", "botttom", "orientation" and "drawEdge" are set prior to this call,
 * "x", "dx" are set for jagged polygons, and eventually set for anti-aliased polygons.
 */
typedef SInt32	(*FskInitEdgeProc)(const FskFixedPoint2D *p0, const FskFixedPoint2D *p1, const FskSpan *span, FskEdge *edge);


/********************************************************************************
 * Span base class definition
 ********************************************************************************/

struct FskSpan {
	/******************** These are initialized once ********************/

	/* These are initialized by the scan converter */
	void						*baseAddr;			/* This is used to initialize "p" on each span */
	SInt32						rowBytes;			/* This is used to initialize "p" on each span */
	SInt32						pixelBytes;			/* This is used to initialize "p" on each span */
	FskSetPixelProc				setPixel;			/* This writes the fillColor to the destination, and can be called from the fill proc */
	FskBlendPixelProc			blendPixel;			/* This blends the fillColor to the destination, and can be called from the fill proc */

	/* The FskInitSpanProc Initializes the rest */
	void						*spanData;			/* Extra data used by the fill method (optional) */
	FskDisposeSpanDataProc		disposeSpanData;	/* This disposes spanData (should be NULL if spanData is not allocated) */
	Boolean						colorIsPremul;		/* The color is premultiplied */

	/* Span stuff */
	FskPixelType				fillColor;			/* This is the color used by the fill proc for filling */
	FskPixelType				altColor;			/* This is an alternate color for whatever use */
	FskFillSpanProc				fill;				/* Fill the span (required) */
	FskSetSpanProc				set;				/* Set up a span from the edges, in addition to the ordinary setup (optional) */

	/* Edge stuff */
	SInt32						edgeBytes;			/* The total number of bytes per edge */
	FskInitEdgeProc				initEdge;			/* This initializes an edge from a couple of points */
	FskAdvanceEdgeProc			advanceEdge;		/* This advances one active edge */

	/******************** These are updated on each span ********************/
	void						*p;					/* The first pixel in the span */
	SInt32						dx;					/* The number of pixels in the span */
};


/********************************************************************************
 * Edge base class definition
 ********************************************************************************/

struct FskEdge {
	SInt16			top, bottom;		/* The range of y where the edge is active */
	FskFixed		x;					/* The intersection of this edge with this scanline */
	FskFixed		dx;					/* dx/dy */
	signed char		orientation;		/* +1 = clockwise, -1 = counterclockwise, 0 = insignificant */
	char			drawEdge;			/* 1 to draw, 0 no not draw */
	char			unused[2];			/* pad to sizeof(UInt32) */

	/* Any additional subclass members go here */
};


/* Scan-converters call this to initialize the base class span fields to somthing sane */
void	FskInitSpan(FskSpan *span, FskBitmap dstBM, UInt32 baseEdgeBytes);


/* Initialize the base class span fields */
FskErr	FskInitSolidColorSpan(FskSpan *span, FskBitmap dstBM, const FskFixedMatrix3x2 *M, UInt32 quality, const struct FskColorSource *cs);
FskErr	FskInitSolidColorSpanFromColor(FskSpan *span, FskBitmap dstBM, const FskFixedMatrix3x2 *M, UInt32 quality, FskConstColorRGBA color);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKSPAN__ */


