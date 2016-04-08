/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskAAPolygon.h"
#include "FskAATable.h"
#include "FskClipPolygon2D.h"
#include "FskPixelOps.h"
#include "FskGrowableStorage.h"
#include "FskUtilities.h"
#include "FskSpan.h"
#include "FskGradientSpan.h"
#if defined(FSK_POLYGON_TEXTURE) && FSK_POLYGON_TEXTURE
	#include "FskTextureSpan.h"
#else /* FSK_POLYGON_TEXTURE */
	#define FskInitTextureSpan NULL
#endif /* FSK_POLYGON_TEXTURE */


#include <limits.h>
#include <string.h>


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else // !PRAGMA_MARK_SUPPORTED
		#define PRAGMA_MARK_SUPPORTED 0
	#endif // !PRAGMA_MARK_SUPPORTED
#endif // PRAGMA_MARK_SUPPORTED




/********************************************************************************
 * Platform configuration
 ********************************************************************************/
#ifndef __FSKPLATFORM__  /* these will go away sometime soon */
	#define DST_32ARGB				1
	#define DST_32BGRA				1
	#define DST_32RGBA				1
	#define DST_24BGR				1
	#define DST_24RGB				1
	#define DST_16RGB565LE			1
	#define DST_16RGB565BE			1
	#define DST_16BGR565LE			1
	#define DST_16BGR565BE			1
	#define DST_16RGB5515LE			1
	#define DST_16RGB5515BE			1
	#define DST_16RGBA4444LE		1
	#define DST_16RGBA4444BE		1
	#define DST_8G					1
#endif /* __FSKPLATFORM__ -- these will go away sometime soon */
#undef DST_8G
#define DST_8G	1

#include "FskBlitDispatchDef.h"

#if SUPPORT_INSTRUMENTATION
	//#define LOG_EDGES
	//#define LOG_ACTIVE_EDGES
	//#define LOG_CROSSING_EDGES
	//#define LOG_SPAN_EDGES
	#ifndef NULL
		#define NULL 0
	#endif /* NULL */
	FskInstrumentedSimpleType(AAPolygon, aapolygon);
	#define LOGD(...)  do { FskInstrumentedTypePrintfDebug  (&gAAPolygonTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGI(...)  do { FskInstrumentedTypePrintfVerbose(&gAAPolygonTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGE(...)  do { FskInstrumentedTypePrintfMinimal(&gAAPolygonTypeInstrumentation, __VA_ARGS__); } while(0)
#endif /* SUPPORT_INSTRUMENTATION */
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */


/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****							Macros and constants						****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/


/* Constants for standard fixed point data types */
#define FRACT_ONE			0x40000000
#define FRACT_HALF			(FRACT_ONE >> 1)
#define FRACT_QUARTER		(FRACT_ONE >> 2)
#define FIXED_ONE			0x00010000

/* Constants for our custom fixed-point data types */
#define AACOORD_FRACBITS	16//20	/* This MUST be >= 16 */
#define AAFIXED_ONE			(1 << AACOORD_FRACBITS)
#define AAFIXEDFLOOR(x)		((x) >> AACOORD_FRACBITS)
#define AAFIXEDCEIL(x)		(((x) + ((1 << AACOORD_FRACBITS) - 1)) >> AACOORD_FRACBITS)
#define AAFIXEDROUND(x)		(((x) + (1 << (AACOORD_FRACBITS - 1))) >> AACOORD_FRACBITS)

/* When clipping, we may multiply the number of points by this factor */
#define CLIP_MULTIPLIER		2	/* typical worst case, with less than 25% concavities */

/* The chosen point-spread function tables */
#define theLinePSF			gFskGaussRectConvLineFilter255_32
#define theEdgePSF			gFskGaussianNarrowPolygonFilter127_32

/* Convert from AAFixed distance to an index into the AATable */
#define AATABLE_FRACBITS	5	/* The number of fractional bits in the AAtable */
#define AADISTANCE_TO_INDEX(d)	(((d) + (1 << (AACOORD_FRACBITS-AATABLE_FRACBITS-1))) >> (AACOORD_FRACBITS-AATABLE_FRACBITS))


/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****					typedefs and data structures						****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/


typedef SInt32	AAFixed;														/* Our own Fixed point data type with AACOORD_FRACBITS fractional bits */
typedef struct	AAFixedPoint2D	{	AAFixed	x, y;	}		AAFixedPoint2D;		/* 2D point  with AACOORD_FRACBITS fractional bits */
typedef struct	AAFixedVector2D	{	AAFixed	x, y;	}		AAFixedVector2D;	/* 2D vector with AACOORD_FRACBITS fractional bits */
typedef struct	AABounds		{	SInt32	x[2], y[2];	}	AABounds;			/* Left, right, top, bottom */
typedef struct	AAEdge			AAEdge;											/* Inactive edges */
typedef struct	AAActiveEdge	AAActiveEdge;									/* Active edges */
typedef struct	AASpan			AASpan;											/* Span for filling */
typedef void	(*AABlendPix)(FskSpan *span, UInt8 alpha);						/* Proc for blending the fill color into the frame buffer */
typedef void	(*AASetPix  )(FskSpan *span, UInt8 alpha);						/* Set a pixel at the location (unused) */

/* Inactive edges */
struct AAEdge {
	AAFixedPoint2D		endPoint[2];		/* Endpoints of the edge */
	FskFractVector2D	U;					/* Unit vector of the edge */
	AAFixed				length;				/* Length of the edge */
	FskEdge				edge;
};

/* Active edges */
struct AAActiveEdge {
	AAActiveEdge		*next;				/* Link to the next currently active edge */
	AAActiveEdge		*nextCross;			/* Link to the next edge crossing */

	AAFixedPoint2D		spanPt;				/* Pixel in the coordinates of the edge */
	AAFixedVector2D		inc[2];				/* Increments in spanPt corresponding to increment in X and Y, respectively */

	AAFixed				aaEdgeHalfWidth;	/* The x-interval spanned by an edge on a scanline = aaRadius / sin(angle from vertical) */

	SInt16				boundLeft;			/* Bounding box of segment */
	SInt16				boundRight;			/* Bounding box of segment */
	SInt16				crossX;				/* Transition between inside and outside */
	SInt16				firstX;				/* Left end of active span */
	SInt16				lastX;				/* Right end of active span */
	SInt16				topCross;			/* The first scanline that the edge crosses */
	SInt16				bottomCross;		/* The first scanline that the edge does not cross */
	char				crosses;			/* There is a crossing on this scanline */
	char				steep;				/* Steep or shallow? */

	AAEdge				aaEdge;				/* Copy the inactive edge info into here */
};

/* AASpan */
struct AASpan {
	FskSpan						span;				/* We subclass the generic span */
	AAFixed						aaRadius;			/* Radius of anti-aliasing */
	AAFixed						aaAuraRad;			/* aaRadius, truncated to AATABLE_FRACBITS bits */
	AAFixed						aaAuraRadSquared;	/* Square of aaAuraRad */
	AABlendPix					blendPix;			/* Function to blend the fillColor into the frame buffer */
	FskSetPixelProc				setInsidePixel;		/* Proc used to write the pixel inside the boundary */
	FskSetPixelProc				setOutsidePixel;	/* Proc used to write the pixel outside the boundary */
	FskBlendPixelProc			blendInsidePixel;	/* Proc used to blend the pixel inside the boundary */
	FskBlendPixelProc			blendOutsidePixel;	/* Proc used to blend the pixel outside the boundary */
	FskFillSpanProc				flatFill;			/* Proc used to fill the interior of a polygon, far from any edge */
	const UInt8					*aaTable;			/* The table used for anti-aliasing */
	FskConstRectangle			clipRect;			/* Clip to this rectangle */
	AAActiveEdge				*edges;				/* The linked list of edges active in this subspan */
};


#if SUPPORT_INSTRUMENTATION
static void LogEdge(const AAEdge *e, UInt32 i, const char *extra) {
	if (!extra)	extra = "";
	LOGD("\t[%2u]: pt0:(%5.1f,%5.1f) pt1:(%5.1f,%5.1f) u:(%+.3f,%+.3f) L:%5.1f topBot:(%3d,%3d) x:%5.1f dx:%+6.3g or:%+d draw:%d%s",
		(unsigned)i, e->endPoint[0].x / 65536., e->endPoint[0].y / 65536., e->endPoint[1].x / 65536., e->endPoint[1].y / 65536.,
		e->U.x / 1073741824., e->U.y / 1073741824., e->length / 65536.,
		e->edge.top, e->edge.bottom, e->edge.x / 65536., e->edge.dx / 65536., e->edge.orientation, e->edge.drawEdge, extra);
}
static void LogEdges(SInt32 n, const AAEdge *e, const char *name) {
	SInt32 i;
	if (!name)	name = "EDGES";
	LOGD("%s[%d]", name, (int)n);
	for (i = 0; i < n; ++i)
		LogEdge(e + i, i, NULL);
}
static void LogActiveEdge(const AAActiveEdge *A, SInt32 index) {
	LogEdge(&A->aaEdge, index, NULL);
}
static void LogActiveEdges(FskConstGrowableArray activeEdgeArray, SInt32 y0, SInt32 y1, const char *name) {
	UInt32			i, n;
	AAActiveEdge	*ae;

	if (!name)	name = "ACTIVE_EDGES";
	n = FskGrowableArrayGetItemCount(activeEdgeArray);
	LOGD("%s(y0=%-3d y1=%-3d dy=%-3d) [%d]", name, (int)y0, (int)y1, (int)(y1 - y0), (int)n);
	for (i = 0; i < n; ++i) {
		FskGrowableArrayGetConstPointerToItem(activeEdgeArray, i, (const void**)(const void*)(&ae));
		LogActiveEdge(ae, i);
	}
}
static void LogCrossingEdges(const AAActiveEdge *A, SInt32 y, const char *name) {
	UInt32 i;
	if (!name)	name = "CROSSING_EDGES";
	LOGD("%s(y=%d):", name, (int)y);
	for (i = 0; A; ++i, A = A->nextCross)
		LogEdge(&A->aaEdge, i, NULL);
}
static void LogSpanEdges(const AAActiveEdge *A, SInt32 y, SInt32 x, SInt32 dx, SInt32 windingNumber, const char *name) {
	UInt32 i;
	if (!name)	name = "SPAN_EDGES";
	LOGD("%s(y=%d x=%d dx=%d wn=%d):", name, (int)y, (int)x, (int)dx, (int)windingNumber);
	for (i = 0; A; ++i, A = A->next)
		LogEdge(&A->aaEdge, i, NULL);
}
#endif /* SUPPORT_INSTRUMENTATION */


/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****						Blending Functions								****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/

static AAFixed MinDistanceToAASpanActiveEdgeListAndIncrement(AASpan *aaSpan);


/********************************************************************************
 * Blending function generation
 ********************************************************************************/
#define SRC_KIND 8A				/* Needed for FskBlitDst.h, though not used */
#define BLIT_PROTO_FILE			"FskAAPolygonBlendProto.c"
#include "FskBlitDst.h"
#undef BLIT_PROTO_FILE


#define BlendFillColorYUV420	NULL


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
 * Dispatch table
 ********************************************************************************/

#define P_D(d)	FskName2(BlendFillColor,d)
static FskBlendPixelProc procDispatch[NUM_DST_FORMATS] = {
		P_D(DST_KIND_0)
	#if NUM_DST_FORMATS > 1
		,P_D(DST_KIND_1)
	#endif
	#if NUM_DST_FORMATS > 2
		,P_D(DST_KIND_2)
	#endif
	#if NUM_DST_FORMATS > 3
		,P_D(DST_KIND_3)
	#endif
	#if NUM_DST_FORMATS > 4
		,P_D(DST_KIND_4)
	#endif
	#if NUM_DST_FORMATS > 5
		,P_D(DST_KIND_5)
	#endif
	#if NUM_DST_FORMATS > 6
		,P_D(DST_KIND_6)
	#endif
	#if NUM_DST_FORMATS > 7
		,P_D(DST_KIND_7)
	#endif
	#if NUM_DST_FORMATS > 8
		,P_D(DST_KIND_8)
	#endif
	#if NUM_DST_FORMATS > 9
		,P_D(DST_KIND_9)
	#endif
	#if NUM_DST_FORMATS > 10
		,P_D(DST_KIND_10)
	#endif
	#if NUM_DST_FORMATS > 11
		,P_D(DST_KIND_11)
	#endif
	#if NUM_DST_FORMATS > 12
		,P_D(DST_KIND_12)
	#endif
	#if NUM_DST_FORMATS > 13
		,P_D(DST_KIND_13)
	#endif
	#if NUM_DST_FORMATS > 14
		,P_D(DST_KIND_14)
	#endif
	#if NUM_DST_FORMATS > 15
		#error Unexpected large number of destination pixel formats
	#endif
 };



/********************************************************************************
 * SetAASpanBlendProc
 ********************************************************************************/

static FskErr
SetAASpanBlendProc(AASpan *span, FskBitmap dstBM)
{
	FskErr	err = kFskErrNone;
	SInt32	dstIndex;

	BAIL_IF_NEGATIVE((dstIndex = PixelFormatToProcTableDstIndex(dstBM->pixelFormat)), err, kFskErrUnsupportedPixelType);
	if ((span->blendPix = procDispatch[dstIndex]) == NULL)
		err = kFskErrUnsupportedPixelType;

bail:
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 ****							Set pixel procs								****
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/


/*******************************************************************************
 * SetPixelOutsideAuraFill
 *******************************************************************************/

static void
SetPixelOutsideAuraFill(FskSpan *span)
{
	AASpan	*aaSpan		= (AASpan*)(span);
	AAFixed minDistance;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {
		aaSpan->blendPix(span, aaSpan->aaTable[minDistance]);		/* Yes! Close to the edge: blend a dip */
	}
}


/*******************************************************************************
 * SetPixelInsideAuraFill
 *******************************************************************************/

static void
SetPixelInsideAuraFill(FskSpan *span)
{
	AASpan	*aaSpan		= (AASpan*)(span);
	AAFixed	minDistance;
	UInt8	alpha;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	alpha = 255;
	if (minDistance <= aaSpan->aaAuraRad) {
		alpha -= aaSpan->aaTable[minDistance];
	}
	aaSpan->blendPix(span, alpha);
}


/*******************************************************************************
 * SetPixelAuraFrame
 *******************************************************************************/

static void
SetPixelAuraFrame(FskSpan *span)
{
	AASpan	*aaSpan		= (AASpan*)(span);
	AAFixed	minDistance;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {
		FskPixelType fillColor	= aaSpan->span.fillColor;
		aaSpan->span.fillColor	= aaSpan->span.altColor;
		aaSpan->blendPix(span, aaSpan->aaTable[minDistance]);
		aaSpan->span.fillColor = fillColor;
	}
}
#define SetPixelOutsideAuraFrame		SetPixelAuraFrame		/* Computations are identical for inside and outside the edge of a frame */
#define SetPixelInsideAuraFrame			SetPixelAuraFrame
#define SetPixelOutsideAuraFillFrame	SetPixelAuraFrame		/* Computations for the outside aura of frame&fill are identical to that of frame */


/*******************************************************************************
 * SetPixelInsideAuraFillFrame
 *******************************************************************************/

static void
SetPixelInsideAuraFillFrame(FskSpan *span)
{
	AASpan			*aaSpan		= (AASpan*)(span);
	AAFixed	minDistance;

	aaSpan->blendPix(span, 255);												/* Inside fill */
	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {										/* If drawing frame */
		FskPixelType	fillColor	= aaSpan->span.fillColor;					/* Save the fill color */
		aaSpan->span.fillColor = aaSpan->span.altColor;							/* Use the alt color for the frame color */
		aaSpan->blendPix(span, aaSpan->aaTable[minDistance]);					/* Draw the frame */
		aaSpan->span.fillColor = fillColor;										/* Restore the fill color */
	}
}


/*******************************************************************************
 * BlendPixelOutsideAuraFill
 *******************************************************************************/

static void
BlendPixelOutsideAuraFill(FskSpan *span, UInt8 opacity)
{
	AASpan	*aaSpan			= (AASpan*)(span);
	AAFixed minDistance;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {
		aaSpan->blendPix(span, FskAlphaMul(aaSpan->aaTable[minDistance], opacity));		/* Yes! Close to the edge: blend a dip */
	}
}


/*******************************************************************************
 * BlendPixelInsideAuraFill
 *******************************************************************************/

static void
BlendPixelInsideAuraFill(FskSpan *span, UInt8 opacity)
{
	AASpan		*aaSpan		= (AASpan*)(span);
	AAFixed		minDistance;
	UInt8		alpha;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	alpha = 255;
	if (minDistance <= aaSpan->aaAuraRad) {
		alpha -= aaSpan->aaTable[minDistance];
	}
	aaSpan->blendPix(span, FskAlphaMul(alpha, opacity));
}


/*******************************************************************************
 * BlendPixelAuraFrame
 *******************************************************************************/

static void
BlendPixelAuraFrame(FskSpan *span, UInt8 opacity)
{
	AASpan		*aaSpan		= (AASpan*)(span);
	AAFixed		minDistance;

	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {
		FskPixelType fillColor	= aaSpan->span.fillColor;							/* Save the fill color. TODO: why? */
		aaSpan->span.fillColor	= aaSpan->span.altColor;							/* Use the alt color for the frame color */
		aaSpan->blendPix(span, FskAlphaMul(aaSpan->aaTable[minDistance], opacity));	/* Draw the frame */
		aaSpan->span.fillColor = fillColor;											/* Restore the fill color */
	}
}
#define BlendPixelOutsideAuraFrame		BlendPixelAuraFrame		/* Computations are identical for inside and outside the edge of a frame */
#define BlendPixelInsideAuraFrame		BlendPixelAuraFrame
#define BlendPixelOutsideAuraFillFrame	BlendPixelAuraFrame		/* Computations for the outside aura of frame&fill are identical to that of frame */


/*******************************************************************************
 * BlendPixelInsideAuraFillFrame
 *******************************************************************************/

static void
BlendPixelInsideAuraFillFrame(FskSpan *span, UInt8 opacity)
{
	AASpan			*aaSpan		= (AASpan*)(span);
	AAFixed			minDistance;

	aaSpan->blendPix(span, 255);														/* Always fill inside with the fill color */
	minDistance = MinDistanceToAASpanActiveEdgeListAndIncrement(aaSpan);
	if (minDistance <= aaSpan->aaAuraRad) {												/* If part of the frame */
		FskPixelType fillColor = aaSpan->span.fillColor;								/* Save the fill color */
		aaSpan->span.fillColor = aaSpan->span.altColor;									/* Use the alt color for framing */
		aaSpan->blendPix(span, FskAlphaMul(aaSpan->aaTable[minDistance], opacity));		/* Draw frame */
		aaSpan->span.fillColor = fillColor;												/* Restore fill color */
	}
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							AAActiveEdges								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/*******************************************************************************
 * MinDistanceToAAActiveEdgeListAndIncrement
 *******************************************************************************/

static AAFixed MinDistanceToAASpanActiveEdgeListAndIncrement(AASpan *aaSpan)
{
	/* Variable declaration */
	AAFixed			d, x, minDistance, d2, minSquaredDistance;
	AAActiveEdge	*edge = aaSpan->edges;

	/* Initialize distance and square distance to an appropriate maximum */
	minDistance = 46340;
	minSquaredDistance = 2147483647;

	/* Find the minimum distances to the edge and minimum squared distances to the endpoints */
	for (; edge != NULL; edge->spanPt.x += edge->inc[0].x, edge->spanPt.y += edge->inc[0].y, edge = edge->next) {
		d = edge->spanPt.y;														/* Signed transverse distance */
		d = (d + (1 << (AACOORD_FRACBITS - AATABLE_FRACBITS - 1))) >> (AACOORD_FRACBITS - AATABLE_FRACBITS);		/* Round */
		if (	((x  = edge->spanPt.x     ) < 0)								/* If left  of endpoint 0 */
			||	((x -= edge->aaEdge.length) > 0)								/* or right of endpoint 1 */
		) {
			x = (x + (1 << (AACOORD_FRACBITS - AATABLE_FRACBITS - 1))) >> (AACOORD_FRACBITS - AATABLE_FRACBITS);	/* Round */
			d2 = x * x + d * d;													/* Squared distance from the closest endpoint */
			if (minSquaredDistance > d2)
				minSquaredDistance = d2;
		}
		else {
			if (d < 0)															/* Convert signed transverse distance ... */
				d = -d;															/* ... to unsigned */
			if (minDistance > d)
				minDistance = d;
		}
	}

	if ((minSquaredDistance <= aaSpan->aaAuraRadSquared) && (minDistance * minDistance > minSquaredDistance))
		minDistance = FskFixedNSqrt(minSquaredDistance, 0);						/* Find minimum of transverse and endpoint distances */

	return minDistance;
}


/*******************************************************************************
 * SwapEdges
 *		This assumes that the edges have a size that is an integral number of UInt32s.
 *******************************************************************************/

static void
SwapEdges(void *e0, void *e1, UInt32 edgeBytes)
{
	register UInt32	*l0		= (UInt32*)e0;
	register UInt32	*l1		= (UInt32*)e1;
	register UInt32	n		= edgeBytes / sizeof(UInt32);
	register UInt32	t;

	while (n--) {
		t = *l0;
		*l0++ = *l1;
		*l1++ = t;
	}
}


/*******************************************************************************
 * XSortAAActiveEdgeActivation
 *		Sort by first activity
 *******************************************************************************/

static void
XSortAAActiveEdgeActivation(SInt32 numEdges, AAActiveEdge *edges, UInt32 edgeBytes)
{
	register int			sorted;
	SInt32					i;
	register AAActiveEdge	*e0, *e1;

	/* Bubble sort */
	for (sorted = 0; !sorted; ) {
		for (sorted = 1, i = numEdges-1, e0 = edges; i-- > 0; e0 = (AAActiveEdge*)((char*)e0 + edgeBytes)) {			/* Forward ripple */
			e1 = (AAActiveEdge*)((char*)e0 + edgeBytes);
			if (	( e1->firstX <  e0->firstX)
				||	((e1->firstX == e0->firstX) && (e1->aaEdge.U.x < e0->aaEdge.U.x))
			) {
				SwapEdges(e0, e1, edgeBytes);
				sorted = 0;
			}
		}
		/* The backward ripple eliminates the worst case performance of the bubble sort */
		if (sorted)
			break;
		/* The last two are already sorted */
		for (sorted = 1, i = numEdges-2, e0 = edges+numEdges-3; i-- > 0; e0 = (AAActiveEdge*)((char*)e0 - edgeBytes)) {	/* Backward ripple */
			e1 = (AAActiveEdge*)((char*)e0 + edgeBytes);
			if (	( e1->firstX <  e0->firstX)
				||	((e1->firstX == e0->firstX) && (e1->aaEdge.U.x < e0->aaEdge.U.x))
			) {
				SwapEdges(e0, e1, edgeBytes);
				sorted = 0;
			}
		}
	}
}


/*******************************************************************************
 * Remove
 *******************************************************************************/

static void RemoveActiveEdgeFromNextList(AAActiveEdge *e, AAActiveEdge **list) {
	AAActiveEdge *l;
	for (; (l = *list) != NULL; list = &l->next) {
		if (l == e) {
			*list = l->next;
			return;
		}
	}
}


/********************************************************************************
 * XSortAAActiveEdgeCrossings
 *		Insertion sort
 ********************************************************************************/

static AAActiveEdge*
XSortAAActiveEdgeCrossings(AAActiveEdge **pEdges, SInt32 fillRule)
{
	AAActiveEdge	*edges	= *pEdges;
	AAActiveEdge	*crossings, *e, **lastCross;
	SInt16			crossX;

	if (fillRule == kFskFillRuleEvenOdd) {
		for (crossings = NULL; edges != NULL; edges = edges->next) {
			if (edges->crosses) {
				crossX = edges->crossX;
				for (lastCross = &crossings; (e = *lastCross) != NULL; lastCross = &e->nextCross)
					if (e->crossX >= crossX)
						break;
				edges->nextCross = e;
				*lastCross = edges;
			}
		}
	}
	else {	/* fillRule = kFskFillRuleNonZero */
		SInt32			windingNumber, nextWindingNumber;
		AAActiveEdge	*nearings;
		for (crossings = NULL, nearings = NULL; edges != NULL; edges = edges->next) {
			if (edges->crosses) {
				crossX = edges->crossX;
				for (lastCross = &crossings; (e = *lastCross) != NULL; lastCross = &e->nextCross)
					if (e->crossX >= crossX)
						break;
				edges->nextCross = e;
				*lastCross = edges;
			}
			else {
				for (lastCross = &nearings; (e = *lastCross) != NULL; lastCross = &e->nextCross)
					if (e->crossX >= crossX)
						break;
				edges->nextCross = e;
				*lastCross = edges;
			}
		}
#if 0	/* Remove internal edges, without removing those that are close to an external edge */
		for (lastCross = &crossings, windingNumber = nextWindingNumber = 0; (e = *lastCross) != NULL; windingNumber = nextWindingNumber) {
			nextWindingNumber += e->aaEdge.edge.orientation;
			if (nextWindingNumber && windingNumber) {	/* TODO: we need a fuzzier decision than this */
				*lastCross = e->nextCross;
				RemoveActiveEdgeFromNextList(e, pEdges);
			}
			else {
				lastCross = &e->nextCross;
			}
		}
		/* TODO: We also need to process the "nearings" list for fuzzy removal */
#endif
	}

	return crossings;
}


/********************************************************************************
 * SetSpanPointOfAAActiveEdge
 ********************************************************************************/

static void
SetSpanPointOfAAActiveEdge(SInt32 x, SInt32 y, AAActiveEdge *ae)
{
	AAFixedVector2D v;

	v.x = (x << AACOORD_FRACBITS) - ae->aaEdge.endPoint[0].x;
	v.y = (y << AACOORD_FRACBITS) - ae->aaEdge.endPoint[0].y;
	ae->spanPt.x = (AAFixed)(((FskInt64)v.x * ae->inc[0].x + (FskInt64)v.y * ae->inc[1].x + (1 << (AACOORD_FRACBITS - 1))) >> AACOORD_FRACBITS);
	ae->spanPt.y = (AAFixed)(((FskInt64)v.x * ae->inc[0].y + (FskInt64)v.y * ae->inc[1].y + (1 << (AACOORD_FRACBITS - 1))) >> AACOORD_FRACBITS);
}


/*******************************************************************************
 * ComputeIntervalsOnScanlineForAAActiveEdge
 * For filled polygons, we have
 *	{ramp out, ramp in, flat fill, ramp in, ramp out}
 * For filled and framed polygons we have:
 *	{ramp out, flat frame, ramp in, flat fill. ramp in, flat frame, ramp out}
 * For framed polygons we have
 *	{ramp out, flat frame, ramp out, skip, ramp out, flat frame, ramp out}
 *******************************************************************************/

static void
ComputeIntervalsOnScanlineForAAActiveEdge(SInt32 y, AAActiveEdge *ae)
{

	ae->crossX  = (SInt16)AAFIXEDCEIL(ae->aaEdge.edge.x);									/* At this point, we have crossed over the edge */
	ae->crosses =	((y >= ae->topCross) && (y < ae->bottomCross));							/* The edge crosses this scanline */

	if (ae->steep) {
		ae->firstX = (SInt16)AAFIXEDCEIL( ae->aaEdge.edge.x - ae->aaEdgeHalfWidth);			/* The first pixel to be used for anti-aliasing computations */
		ae->lastX  = (SInt16)AAFIXEDFLOOR(ae->aaEdge.edge.x + ae->aaEdgeHalfWidth);			/* The last  pixel to be used for anti-aliasing computations */
		if      (ae->firstX < ae->boundLeft)	ae->firstX = ae->boundLeft;					/* Edge starts to the left  of the clip */
		else if (ae->firstX > ae->boundRight)	ae->firstX = ae->boundRight;				/* Edge starts to the right of the clip */
		if      (ae->lastX  > ae->boundRight)	ae->lastX  = ae->boundRight;				/* Edge ends   to the right of the clip */
		else if (ae->lastX  < ae->boundLeft)	ae->lastX  = ae->boundLeft;					/* Edge ends   to the left  of the clip */
		if      (ae->firstX > ae->lastX)		ae->firstX  = ae->lastX;					/* Edge starts to the right of the end */
	}
	else {
		ae->firstX = ae->boundLeft;															/* The first pixel to be used for anti-aliasing computations */
		ae->lastX  = ae->boundRight;														/* The last  pixel to be used for anti-aliasing computations */
	}
}


/*******************************************************************************
 * CopyEdge
 *		This assumes that the edges have a size that is an integral number of UInt32s.
 *******************************************************************************/

static void
CopyEdge(const AAEdge *fr, AAEdge *to, UInt32 edgeBytes)
{
	register const UInt32	*f		= (const UInt32*)fr;
	register UInt32			*t		= (UInt32*)to;
	register UInt32			z		= edgeBytes / sizeof(UInt32);

	while (z--)
		*t++ = *f++;
}


/*******************************************************************************
 * InsertAAActiveEdge
 *
 *	This converts an edge into an active edge,
 *	and inserts it at an appropriate position in the edgeArray.
 *******************************************************************************/

static FskErr
InsertAAActiveEdge(const AAEdge *edge, UInt32 edgeBytes, SInt32 y, AAFixed aaRadius, const AABounds *bounds, FskGrowableArray edgeArray)
{
	FskErr				err;
	AAActiveEdge		*aej, newEdge;
	register UInt32		n;
	register SInt32		t;
	UInt32				activeEdgeBytes		= FskGrowableArrayGetItemSize(edgeArray);


	/*
	 * Initialize the active edge for this scanline
	 */

	/* Copy the unadorned edge, so we can toss the original */
	newEdge.next = NULL;
	CopyEdge(edge, &newEdge.aaEdge, edgeBytes);

	/* Find x bounds (including aura), clipped to the clip region */
	if (newEdge.aaEdge.endPoint[0].x <= newEdge.aaEdge.endPoint[1].x) {
		newEdge.boundLeft  = (SInt16)AAFIXEDCEIL (newEdge.aaEdge.endPoint[0].x - aaRadius);
		newEdge.boundRight = (SInt16)AAFIXEDFLOOR(newEdge.aaEdge.endPoint[1].x + aaRadius);
	}
	else {
		newEdge.boundLeft  = (SInt16)AAFIXEDCEIL (newEdge.aaEdge.endPoint[1].x - aaRadius);
		newEdge.boundRight = (SInt16)AAFIXEDFLOOR(newEdge.aaEdge.endPoint[0].x + aaRadius);
	}
	if      (newEdge.boundLeft  < bounds->x[0])	newEdge.boundLeft  = (SInt16)(bounds->x[0]);
	else if (newEdge.boundLeft  > bounds->x[1])	newEdge.boundLeft  = (SInt16)(bounds->x[1]);
	if      (newEdge.boundRight > bounds->x[1])	newEdge.boundRight = (SInt16)(bounds->x[1]);
	else if (newEdge.boundRight < bounds->x[0])	newEdge.boundRight = (SInt16)(bounds->x[0]);

	newEdge.topCross    = (SInt16)AAFIXEDCEIL(newEdge.aaEdge.endPoint[0].y);
	newEdge.bottomCross = (SInt16)AAFIXEDCEIL(newEdge.aaEdge.endPoint[1].y);

	/* Determine whether this edge is almost flat by seeing whether we get overflow, and compute intercept and inverse slope */
	if (	( newEdge.aaEdge.U.y == 0)
		||	((newEdge.aaEdge.edge.dx = FskFixedNDiv(newEdge.aaEdge.U.x, newEdge.aaEdge.U.y, AACOORD_FRACBITS)) == kFskSInt32Min)
		||	( newEdge.aaEdge.edge.dx == kFskSInt32Max)
	) {
		newEdge.aaEdge.edge.x   = ((newEdge.aaEdge.endPoint[1].x + newEdge.aaEdge.endPoint[0].x) >> 1);
		newEdge.aaEdgeHalfWidth = ((newEdge.aaEdge.endPoint[1].x - newEdge.aaEdge.endPoint[0].x) >> 1) + aaRadius;
		newEdge.aaEdge.edge.dx  = 0;														/* It should be infinity, but we set it to zero */
		newEdge.steep           = 0;
	}
	else {
		newEdge.aaEdge.edge.x = FskFixedNMul(((y << AACOORD_FRACBITS) - newEdge.aaEdge.endPoint[0].y), newEdge.aaEdge.edge.dx, AACOORD_FRACBITS);
		if ((newEdge.aaEdge.edge.x != kFskSInt32Min) && (newEdge.aaEdge.edge.x != kFskSInt32Max))
			newEdge.aaEdge.edge.x += newEdge.aaEdge.endPoint[0].x;
		if ((t = newEdge.aaEdge.U.y) < 0)	t = -t;
		newEdge.aaEdgeHalfWidth	= FskFixedNDiv(aaRadius, t, 30);
		newEdge.steep = (newEdge.aaEdgeHalfWidth != kFskSInt32Max);
	}

	/* Set up the edge coordinate system */
	newEdge.inc[1].y =  (newEdge.inc[0].x = (newEdge.aaEdge.U.x + (1 << (30 - AACOORD_FRACBITS - 1))) >> (30 - AACOORD_FRACBITS));
	newEdge.inc[0].y = -(newEdge.inc[1].x = (newEdge.aaEdge.U.y + (1 << (30 - AACOORD_FRACBITS - 1))) >> (30 - AACOORD_FRACBITS));

	/* Compute the active intervals */
	ComputeIntervalsOnScanlineForAAActiveEdge(y, &newEdge);


	/*
	 * Do an insertion sort
	 */

	if ((n = FskGrowableArrayGetItemCount(edgeArray)) > 0) {
		BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(edgeArray, n - 1, (void**)(void*)(&aej)));
		for (t = newEdge.firstX; n--; aej = (AAActiveEdge*)((char*)aej - activeEdgeBytes)) {
			if ((t > aej->firstX) || ((t == aej->firstX) && (newEdge.aaEdge.U.x > aej->aaEdge.U.x)))
				break;
		}
		n++;
		err = FskGrowableArrayInsertItemAtPosition(edgeArray, n, &newEdge);
	}
	else {
		err = FskGrowableArrayAppendItem(edgeArray, &newEdge);
	}

bail:
	return err;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								AAEdges									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * InitAAEdge
 ********************************************************************************/

static AAFixed
InitAAEdge(const FskFixedPoint2D *pt0, const FskFixedPoint2D *pt1, const FskSpan *span, register AAEdge *e)
{
	const AASpan		*aaSpan		= (AASpan*)span;
	FskConstRectangle	clipRect	= aaSpan->clipRect;

	e->edge.orientation	= 1;																	/* Initial orientation is positive */
	e->edge.drawEdge	= 1;																	/* Default is to draw; changed if the close of a frame-fill */
	e->endPoint[0].x 	= pt0->x << (AACOORD_FRACBITS - 16);									/* Store endpoints for later use */
	e->endPoint[0].y	= pt0->y << (AACOORD_FRACBITS - 16);
	e->endPoint[1].x	= pt1->x << (AACOORD_FRACBITS - 16);
	e->endPoint[1].y	= pt1->y << (AACOORD_FRACBITS - 16);
	if (	( e->endPoint[1].y <  e->endPoint[0].y)												/* Orient so that the first point is top ... */
		||	((e->endPoint[1].y == e->endPoint[0].y) && (e->endPoint[1].x < e->endPoint[0].x))	/* ... or left if same */
	) {
		AAFixedPoint2D t;
		t = e->endPoint[0];																		/* Swap points */
		e->endPoint[0]		= e->endPoint[1];
		e->endPoint[1]		= t;
		e->edge.orientation	= -1;																/* Indicate change in orientation */
	}
	//if (e->endPoint[1].y == e->endPoint[0].y)
	//	e->edge.orientation = 0;
	e->edge.top		= (SInt16)AAFIXEDCEIL(e->endPoint[0].y - aaSpan->aaRadius);					/* Edges becomes active when affected by the aura */
	e->edge.bottom	= (SInt16)AAFIXEDCEIL(e->endPoint[1].y + aaSpan->aaRadius);					/* And stays active until unaffected by aura */
	e->U.x			= (e->endPoint[1].x - e->endPoint[0].x);
	e->U.y			= (e->endPoint[1].y - e->endPoint[0].y);
	e->length		= FskFixedVector2DNormalize(&e->U.x);										/* Length and unit vector in the direction of the line */

	if (e->length) {
		if (e->edge.top >= (clipRect->y + clipRect->height))
			e->length = 0;
		if (e->edge.top < clipRect->y)
			e->edge.top = (SInt16)(clipRect->y);
		if (e->edge.bottom > (clipRect->y + clipRect->height))
			e->edge.bottom = (SInt16)(clipRect->y + clipRect->height);
		if (span->initEdge)
			(*span->initEdge)(pt0, pt1, span, &e->edge);										/* Extra initialization */
	}

	return e->length;
}


/*******************************************************************************
 * RemoveDegenerateAAEdges
 *******************************************************************************/

static void
RemoveDegenerateAAEdges(SInt32 *pNEdges, AAEdge *edges, SInt32 edgeBytes, SInt32 fillRule)
{
	SInt32			i, nEdges = *pNEdges;
	register AAEdge	*e0, *e1;

	for (i = nEdges - 1, e0 = edges; i-- > 0; e0 = (AAEdge*)((char*)e0 + edgeBytes)) {
		e1 = (AAEdge*)((char*)e0 + edgeBytes);
		if (	(e0->endPoint[0].x == e1->endPoint[0].x)	&&	(e0->endPoint[0].y == e1->endPoint[0].y)
			&&	(e0->endPoint[1].x == e1->endPoint[1].x)	&&	(e0->endPoint[1].y == e1->endPoint[1].y)
			&&	((fillRule == kFskFillRuleEvenOdd) || ((e0->edge.orientation + e1->edge.orientation) == 0))
		) {
			if (i > 0)
				FskMemMove(e0, (char*)e1 + edgeBytes, i * edgeBytes);
			nEdges -= 2;
			i--;
			e0 = (AAEdge*)((char*)e0 - edgeBytes);
		}
	}
	*pNEdges = nEdges;
}


/********************************************************************************
 * TransformClipAndMakeAAEdges
 ********************************************************************************/

static FskErr
TransformClipAndMakeAAEdges(
	SInt32					nPts,
	const FskFixedPoint2D	*pts,
	const FskFixedMatrix3x2	*M,
	const AASpan			*span,
	AAEdge					*edges
)
{
	FskErr				err;
	SInt32				i;
	UInt32				numClipPts, numEdges;
	FskRectangleRecord	aaRect;
	FskFixedPoint2D		*clipPts, *pp, *lp;
	AAFixed				aaRadius			= span->aaRadius;
	UInt32				edgeBytes			= span->span.edgeBytes;


	if ((err = FskMemPtrNew(nPts * CLIP_MULTIPLIER * sizeof(FskFixedPoint2D), (FskMemPtr*)(void*)(&clipPts))) != kFskErrNone)
		return err;

	/* Copy/transform points to a temporary buffer, because the clipper needs to duplicate the first point.
	 * We use edges as a temporary buffer, because      pts --> (pp = edges) --> clipPts --> edges
	 */
	pp = (FskFixedPoint2D*)edges;
	if (M == NULL) {
		register const SInt32	*f;
		register SInt32		*t;
		for (i = nPts * sizeof(FskFixedPoint2D) / sizeof(SInt32), f = (const SInt32*)pts, t = (SInt32*)pp; i--; )
			*t++ = *f++;
	}
	else {
		FskTransformFixedRowPoints2D(pts, nPts, M, pp);
	}

	/* Enlarge the clipping region to accommodate the aura of anti-aliasing */
	i = (aaRadius >> AACOORD_FRACBITS) + 1;
	aaRect = *span->clipRect;
	FskRectangleInset(&aaRect, -i, -i);

	FskClipPolygon2D(nPts, pp, &aaRect, &numClipPts, clipPts);	/* This duplicates the first vertex to beyond the last */

	for (i = numClipPts, numEdges = 0, pp = clipPts, lp = clipPts + numClipPts - 1; i--; lp = pp++) {
		if (InitAAEdge(lp, pp, &span->span, edges)) {
			edges = (AAEdge*)((char*)edges + edgeBytes);
			numEdges++;
		}
	}

	FskMemPtrDispose(clipPts);	/* Deallocate polygon clip points */

	return numEdges;
}


/********************************************************************************
 * YCompareAAEdges
 ********************************************************************************/

static int
YCompareAAEdges(const void *v0, const void *v1)
{
	const AAEdge	*e0 = (const AAEdge*)v0;
	const AAEdge	*e1 = (const AAEdge*)v1;

#if TARGET_CPU_ARM
	int				result;
	if		(e0->edge.top		< e1->edge.top)			result = -1;
	else if	(e0->edge.top		> e1->edge.top)			result =  1;
	else if (e0->endPoint[0].x	< e1->endPoint[0].x)	result = -1;
	else if (e0->endPoint[0].x	> e1->endPoint[0].x)	result =  1;
	else if (e0->U.x			< e1->U.x)				result = -1;
	else if (e0->U.x			> e1->U.x)				result =  1;
	else												result =  0;
	return result;

#else /* !TARGET_CPU_ARM */
	if (e0->edge.top      != e1->edge.top)		return (e0->edge.top          <         e1->edge.top) ? -1 : +1;
	if (e0->endPoint[0].x != e1->endPoint[0].x)	return (e0->endPoint[0].x     <    e1->endPoint[0].x) ? -1 : +1;
	return                                             (e0->U.x == e1->U.x) ? 0 : (e0->U.x < e1->U.x) ? -1 : +1;

#endif /* !TARGET_CPU_ARM */
}


/********************************************************************************
 * YSortAAEdges
 ********************************************************************************/

static void
YSortAAEdges(UInt32 numEdges, AAEdge *edges, UInt32 edgeSize)
{
	FskQSort(edges, numEdges, edgeSize, YCompareAAEdges);		/* We use QSort because we typically have hundreds of edges */
}


/*******************************************************************************
 * ScanConvertAAEdges
 *******************************************************************************/

static FskErr
ScanConvertAAEdges(
	SInt32				numEdges,
	AAEdge				*edges,
	SInt32				fillRule,
	FskConstRectangle	clipRect,
	AASpan				*span
)
{
	AAActiveEdge			*ae, **lastNext;
	register AAActiveEdge	*L;
	SInt32					y, left, right, windingNumber, i, numActiveEdges;
	SInt32					activeEdgeBytes;
	AABounds				bounds;
	FskErr					err;
	FskGrowableArray		activeEdgeArray = NULL;
	int						rendered		= 0;	/* Keep track of whether we wrote pixels or not */


	/* Sort edges */
	YSortAAEdges(numEdges, edges, span->span.edgeBytes);
	#ifdef LOG_EDGES
		LogEdges(numEdges, edges, "Sorted Edges");
	#endif /* LOG_EDGES */

	/* Remove "invisible seam" edges, though there should be no need for these with multiple contours */
	RemoveDegenerateAAEdges(&numEdges, edges, span->span.edgeBytes, fillRule);
	BAIL_IF_FALSE(numEdges > 0, err, kFskErrNothingRendered);

	/* Convert from rectangle to bounds */
	bounds.x[1] = (bounds.x[0] = clipRect->x) + clipRect->width  - 1;
	bounds.y[1] = (bounds.y[0] = clipRect->y) + clipRect->height - 1;

	/* Initialize active edges */
	y = (edges[0].edge.top >= clipRect->y) ? edges[0].edge.top : clipRect->y;
	activeEdgeBytes = sizeof(AAActiveEdge) - sizeof(AAEdge) + span->span.edgeBytes;
	BAIL_IF_ERR(err = FskGrowableArrayNew(activeEdgeBytes, 0, &activeEdgeArray));

	while (((numActiveEdges = FskGrowableArrayGetItemCount(activeEdgeArray)) != 0) || (numEdges != 0)) {
		SInt32 nextBorn, nextExpired, nextEvent;

		/* Remove expired edges */
		if (numActiveEdges > 0) {
			BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(activeEdgeArray, numActiveEdges-1, (void**)(void*)(&ae)));
			for (i = numActiveEdges, L = ae; i--; L = (AAActiveEdge*)((char*)L - activeEdgeBytes)) {
				if (y >= L->aaEdge.edge.bottom) {						/* Edge is expired */
					FskGrowableArrayRemoveItem(activeEdgeArray, i);	/* Remove */
				}
			}
		}

		/* Collect newborn edges */
		for ( ; (numEdges != 0) && (edges->edge.top <= y); (edges = (AAEdge*)((char*)edges + span->span.edgeBytes)), numEdges--) {	/* Should ALWAYS be ==, except maybe at the top */
			if (edges->edge.bottom >= y) {
				BAIL_IF_ERR(err = InsertAAActiveEdge(edges, span->span.edgeBytes, y, span->aaRadius, &bounds, activeEdgeArray));
			}
		}
		nextBorn = (SInt32)((numEdges == 0) ? kFskSInt32Max : edges->edge.top);

		/* Determine the next edge expiration */
		nextExpired = kFskSInt32Max;
		FskGrowableArrayGetPointerToItem(activeEdgeArray, 0, (void**)(void*)(&ae));
		if ((numActiveEdges = FskGrowableArrayGetItemCount(activeEdgeArray)) > 0)
			for (L = ae, i = numActiveEdges; i--; L = (AAActiveEdge*)((char*)L + activeEdgeBytes))
				if (nextExpired > L->aaEdge.edge.bottom)
					nextExpired = L->aaEdge.edge.bottom;

		nextEvent = (nextBorn < nextExpired) ? nextBorn : nextExpired;
		#ifdef LOG_ACTIVE_EDGES
			LogActiveEdges(activeEdgeArray, y, nextEvent, "ActiveEdges");
		#endif /* LOG_ACTIVE_EDGES */

		/* Scan all active edges until the next event */
		if (nextEvent < kFskSInt32Max) {
			SInt32	dy	= nextEvent - y;

			/* Render all polygons in scanline order */
			for (; dy--; y++) {

				if (numActiveEdges <= 0)
					continue;

				/* Compute crossings and active intervals for each active edge, and sort by activation point */
				FskGrowableArrayGetPointerToItem(activeEdgeArray, 0, (void**)(void*)(&ae));
				for (i = numActiveEdges, L = ae; i-- > 0; L = (AAActiveEdge*)((char*)L + activeEdgeBytes))
					ComputeIntervalsOnScanlineForAAActiveEdge(y, L);
				XSortAAActiveEdgeActivation(numActiveEdges, ae, activeEdgeBytes);	/* Array */

				/* Make into a linked list, rebuilding every scanline */
				for (i = numActiveEdges - 1, L = ae, left = kFskSInt32Max, right = kFskSInt32Min; i-- > 0; L = L->next)
					L->next = (AAActiveEdge*)((char*)L + activeEdgeBytes);
				L->next = NULL;

				/********************************************************************************
				 * Frame only
				 ********************************************************************************/
				if (fillRule < 0) {
					for (left = 0, span->edges = NULL; (ae != NULL) || (span->edges != NULL); left = right + 1) {
						/* If all edges are napping, skip to the next to awake */
						if (span->edges == NULL)
							left = ae->firstX;

						/* Collect awake edges */
						for ( ; ((L = ae) != NULL) && (L->firstX <= left); ) {
							ae = L->next;
							if (L->aaEdge.edge.drawEdge) {
								L->next = span->edges;
								span->edges = L;
								SetSpanPointOfAAActiveEdge(left, y, L);
							}
						}

						/* Determine interval */
						for (L = span->edges, right = ((ae != NULL) ? (ae->firstX-1) : kFskSInt32Max); L != NULL; L = L->next)
							if (right > L->lastX)
								right = L->lastX;
						if ((span->span.dx = right - left + 1) > 0) {
							rendered = 1;
							span->span.p = (char*)(span->span.baseAddr) + y * span->span.rowBytes + left * span->span.pixelBytes;
//							span->auraInsideFill(&span->span);
							span->span.setPixel   = span->setInsidePixel;
							span->span.blendPixel = span->blendInsidePixel;
							span->span.fill(&span->span);
						}

						/* Remove napping edges */
						for (lastNext =  &span->edges; (L = *lastNext) != NULL; ) {
							if (L->lastX <= right)	*lastNext = L->next;					/* Remove */
							else					lastNext = &L->next;					/* Skip */
						}
					}
				}

				/********************************************************************************
				 * Fill or fill and frame
				 ********************************************************************************/

				/* We need to maintain three lists:
				 * (1) the list of pending edges; this is maintained sorted by activation point in the activeEdgeArray.
				 * (2) the list of edge crossings, stored as a linked list
				 * (3) the list of active edges, for anti-aliasing purposes; this is stored as a linked list
				 */
				else {
					AAActiveEdge	*crossings;
					SInt32			nextCrossX, nextWindingNumber;

					crossings = XSortAAActiveEdgeCrossings(&ae, fillRule);	/* ae linked list->next, crossing linked list->nextCross */

					#ifdef LOG_CROSSING_EDGES
						LogCrossingEdges(crossings, y, "CrossingEdges");
					#endif /* LOG_CROSSING_EDGES */

					for (windingNumber = 0, left = bounds.x[0], nextCrossX = kFskSInt32Min, span->edges = NULL; (ae != NULL) || (span->edges != NULL); left = right + 1) {

						/* Collect awake edges */
						for ( ; ((L = ae) != NULL) && (L->firstX <= left); ) {
							ae = L->next;
							if (L->lastX >= left) {
								L->next = span->edges;
								span->edges = L;
								SetSpanPointOfAAActiveEdge(left, y, L);
							}
						}

						/* Determine fill state */
						for ( ; (crossings != NULL) && (crossings->crossX <= left); crossings = crossings->nextCross) {
							if (fillRule == kFskFillRuleNonZero)	windingNumber += crossings->aaEdge.edge.orientation;
							else									windingNumber = !windingNumber;
						}

						/* If all edges are napping, skip to the next to awake */
						if ((span->edges == NULL) && (windingNumber == 0)) {
							#ifdef SCAN_DEBUG
								printf("\n");
							#endif /* SCAN_DEBUG */
							if (ae != NULL) {
								right = ae->firstX - 1;
								continue;
							}
							break;
						}

						/* Determine the next crossing */
						if (nextCrossX < left) {
							for (	nextCrossX = bounds.x[1] + 1, nextWindingNumber = windingNumber, L = crossings;
									(L != NULL) && ((L->crossX <= nextCrossX) || (!nextWindingNumber == !windingNumber));
									L = L->nextCross
							) {
								nextCrossX = L->crossX;
								if (fillRule == kFskFillRuleNonZero)	nextWindingNumber += L->aaEdge.edge.orientation;
								else									nextWindingNumber = !nextWindingNumber;
							}
							if (!nextWindingNumber == !windingNumber)
								nextCrossX = bounds.x[1] + 1;
						}
						#ifdef SCAN_DEBUG
							printf(" (%+d[%d], %+d[%d])", (int)windingNumber, (int)left, (int)nextWindingNumber, (int)nextCrossX);
						#endif /* SCAN_DEBUG */

						/* Determine the next state transition point */
						right = nextCrossX;								/* Minimum of next crossing ... */
						if ((ae != NULL) && (right > ae->firstX))
							right = ae->firstX;							/* ... next waking edge ... */
						right--;
						for (L = span->edges; L != NULL; L = L->next)
							if (right > L->lastX)
								right = L->lastX;						/* ... or next edge nap */
						if (right < left)								/* I still don't understand how this might happen */
							right = left;

						/* Non-null span */
						if ((span->span.dx = right - left + 1) > 0) {
							#ifdef LOG_SPAN_EDGES
								LogSpanEdges(span->edges, y, left, span->span.dx, windingNumber, "SpanEdges");
							#endif /* LOG_SPAN_EDGES */
							rendered = 1;
							span->span.p = (char*)(span->span.baseAddr) + y * span->span.rowBytes + left * span->span.pixelBytes;
							if (span->span.set != NULL)												/* If the span needs additional setup ... */
								span->span.set(&L->aaEdge.edge, NULL, left, y, &span->span);		/* ... do it */
							if (windingNumber != 0) {	/* In fill state */
								if (span->edges != NULL) {	span->span.setPixel   = SetPixelInsideAuraFill;		/* In anti-aliased fill state */
															span->span.blendPixel = BlendPixelInsideAuraFill;
								}
								else					{	span->span.setPixel   = NULL;						/* In a flat fill state */
															span->span.blendPixel = NULL;
								}
								span->span.fill(&span->span);
							}
							else {						/* Not in a fill state */
								if (span->edges != NULL) {	span->span.setPixel   = SetPixelOutsideAuraFill;	/* In anti-aliased border state */
															span->span.blendPixel = BlendPixelOutsideAuraFill;
															span->span.fill(&span->span);
								}
								else					{	FskDebugStr("oops: in a skip state");
								}
							}
						}

						/* Remove napping edges */
						for (lastNext =  &span->edges; (L = *lastNext) != NULL; ) {
							if (L->lastX <= right)	*lastNext = L->next;					/* Remove */
							else					lastNext = &L->next;					/* Skip */
						}

					}
				}
				#ifdef SCAN_DEBUG
					printf("\n");
				#endif /* SCAN_DEBUG */

				/* Advance all active edges */
				FskGrowableArrayGetPointerToItem(activeEdgeArray, 0, (void**)(void*)(&ae));
				if (span->span.advanceEdge == NULL) {										/* Simple edge advancement */
					for (L = ae, i = numActiveEdges; i--; L = (AAActiveEdge*)((char*)L + activeEdgeBytes)) {
						L->aaEdge.edge.x += L->aaEdge.edge.dx;								/* Advance x crossing */
					}
				}
				else {																		/* Specialized edge advancement */
					for (L = ae, i = numActiveEdges; i--; L = (AAActiveEdge*)((char*)L + activeEdgeBytes)) {
						L->aaEdge.edge.x += L->aaEdge.edge.dx;								/* Advance x crossing */
						(*span->span.advanceEdge)(&L->aaEdge.edge);							/* Additional advancement */
					}
				}
			}
			y = nextEvent;
		}
		else
			break;
	}

bail:
	if (activeEdgeArray != NULL)					FskGrowableArrayDispose(activeEdgeArray);
	if ((err == kFskErrNone) && (rendered == 0))	err = kFskErrNothingRendered;
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskAAPolygonContours
 ********************************************************************************/

FskErr
FskAAPolygonContours(
	UInt32					nContours,
	const UInt32			*nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	const FskColorSource	**frameFillColors,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr					err			= kFskErrNone;
	AAEdge					*edges		= NULL;
	AAEdge					*e;
	SInt32					nc, np, numEdges;
	FskRectangleRecord		dstRect;
	AASpan					span;
	static FskInitSpanProc	initProcs[] = {
								FskInitSolidColorSpan,
								FskInitLinearGradientSpan,
								FskInitRadialGradientSpan,
								FskInitTextureSpan,
								NULL						/* Procedure span is not yet implemented */
							};
	FskInitSpanProc			initSpan;

	if (kFskErrNone != (err = FskBitmapWriteBegin(dstBM, NULL, NULL, NULL)))
		return err;

	/* Determine composite clipping region */
	if (clipRect == NULL)	dstRect = dstBM->bounds;
	else					if (!FskRectangleIntersect(&dstBM->bounds, clipRect, &dstRect)) return kFskErrNothingRendered;

	/* Set fill and frame (alt) colors, and initialize the span */
	nc = (frameFillColors[1] != NULL) ? frameFillColors[1]->type : frameFillColors[0]->type;
	initSpan =  initProcs[(nc <= kFskColorSourceTypeMax) ? nc : 0];
	FskInitSpan(&span.span, dstBM, sizeof(AAEdge));
	span.flatFill = NULL;
	if (frameFillColors[0] != NULL) {
		if (frameFillColors[1] != NULL) {
			if (frameFillColors[0]->type == kFskColorSourceTypeConstant) {
				FskConstColorRGBA frColor = &(((const FskColorSourceConstant*)(frameFillColors[0]))->color);
				err = FskConvertColorRGBAToBitmapPixel(frColor, dstBM->pixelFormat, &span.span.altColor);	/* Only want 1 init */
			}
		}
		else {
			err = (*initSpan)(&span.span, dstBM, M, 1, frameFillColors[0]);
			span.span.altColor = span.span.fillColor;
		}
		BAIL_IF_ERR(err);
	}
	else
		strokeWidth = -1;
	if (frameFillColors[1] != NULL) {
		BAIL_IF_ERR(err = (*initSpan)(&span.span, dstBM, M, 1, frameFillColors[1]));
	} else {
		fillRule = -1;
	}
	if (span.flatFill == NULL)
		span.flatFill = span.span.fill;	/* It's so nice, we store it twice! */

	/* Set blend proc appropriate for the destination pixel format */
	BAIL_IF_ERR(err = SetAASpanBlendProc(&span, dstBM));

	/* Choose point spread function based on whether filling or framing or framing&filling */
	if (fillRule < 0) {
		BAIL_IF_NEGATIVE(strokeWidth, err, kFskErrNothingRendered);		/* Don't Frame, Don't Fill */
		/* strokeWidth>=0 */ {											/* Frame only */
			nc                     = sizeof(theLinePSF);				/* This doesn't take wide lines into account */
			span.aaTable           = theLinePSF;
			span.setInsidePixel    = SetPixelInsideAuraFrame;
			span.setOutsidePixel   = SetPixelOutsideAuraFrame;
			span.blendInsidePixel  = BlendPixelInsideAuraFrame;
			span.blendOutsidePixel = BlendPixelOutsideAuraFrame;
		}
	}
	else {
		if (strokeWidth < 0) {											/* Fill only */
			nc                     = sizeof(theEdgePSF);
			span.aaTable           = theEdgePSF;
			span.setInsidePixel    = SetPixelInsideAuraFill;
			span.setOutsidePixel   = SetPixelOutsideAuraFill;
			span.blendInsidePixel  = BlendPixelInsideAuraFill;
			span.blendOutsidePixel = BlendPixelOutsideAuraFill;
		}
		else {															/* Frame and Fill */
			nc                     = sizeof(theLinePSF);				/* This doesn't take wide lines into account */
			span.aaTable           = theLinePSF;
			span.setInsidePixel    = SetPixelInsideAuraFillFrame;
			span.setOutsidePixel   = SetPixelOutsideAuraFillFrame;
			span.blendInsidePixel  = BlendPixelInsideAuraFillFrame;
			span.blendOutsidePixel = BlendPixelOutsideAuraFillFrame;
		}
	}

	span.clipRect = &dstRect;

	/* Compute the anti-aliasing aura radius */
	nc--;
	span.aaRadius  = nc << (AACOORD_FRACBITS - AATABLE_FRACBITS);		/* Aura radius in coordinate precision */
	span.aaAuraRad = nc;												/* Aura radius in aatable precision */
	span.aaAuraRadSquared = span.aaAuraRad * span.aaAuraRad;			/* Aura radius squared */

	/* Allocate edges */
	for (nc = nContours, np = 0; nc--; )								/* Accumulate total number of points */
		np += *nPts++;
	nPts -= nContours;													/* Reset numPts to its initial value */
	BAIL_IF_ERR(err = FskMemPtrNew(np * CLIP_MULTIPLIER * span.span.edgeBytes, (FskMemPtr*)(void*)(&edges)));	/* Allocate edges */

	/* Make edges */
	for (nc = nContours, e = edges; nc--; pts += *nPts++) {
		BAIL_IF_NEGATIVE((np = TransformClipAndMakeAAEdges(*nPts, pts, M, &span, e)), err, np);
		e = (AAEdge*)((char*)e + np * span.span.edgeBytes);				/* Advance edge pointer */
	}
	numEdges = (SInt32)(e - edges);										/* Compute the number of edges */

	if ((numEdges == 2) && (nContours == 1) && (nPts[-1] == 2))			/* Detect a single line */
		numEdges--;

	if (numEdges > 0)
		err = ScanConvertAAEdges(numEdges, edges, fillRule, &dstRect, &span);
	else
		err = kFskErrNothingRendered;


bail:
	if (edges != NULL)
		FskMemPtrDispose((FskMemPtr)edges);
	if ((span.span.disposeSpanData != NULL) && (span.span.spanData != NULL))
		span.span.disposeSpanData(span.span.spanData);

	FskBitmapWriteEnd(dstBM);
	return err;
}
