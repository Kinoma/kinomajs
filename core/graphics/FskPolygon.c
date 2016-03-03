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

#include "FskPolygon.h"
#include "FskClipPolygon2D.h"
#include "FskSpan.h"
#include "FskPixelOps.h"
#include "FskBitmap.h"
#include "FskClipLine2D.h"
#include "FskLine.h"
#include "FskUtilities.h"
#include "FskWideLineToPolygon.h"
#include "FskAAPolygon.h"
#include "FskGradientSpan.h"
#if defined(FSK_POLYGON_TEXTURE) && FSK_POLYGON_TEXTURE
	#include "FskTextureSpan.h"
#else /* FSK_POLYGON_TEXTURE */
	#define FskInitAffineTextureSpan NULL
#endif /* FSK_POLYGON_TEXTURE */


#include "limits.h"


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else // !PRAGMA_MARK_SUPPORTED
		#define PRAGMA_MARK_SUPPORTED 0
	#endif // !PRAGMA_MARK_SUPPORTED
#endif // PRAGMA_MARK_SUPPORTED


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							typedefs and macros							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define CeilFixedToInt(x)		(((x) + 0xFFFF) >> 16)
#define FloorFixedToInt(x)		(((x)         ) >> 16)

#define MAX_VERTICES			100
//#define CLIP_MULTIPLIER		4	// worst case, with a highly jagged, highly concave polygon
#define CLIP_MULTIPLIER			2	// typical worst case, with less than 25% concavities
#define MAX_CLIPPED_VERTICES	(MAX_VERTICES * CLIP_MULTIPLIER)
#define MAX_EDGES				MAX_CLIPPED_VERTICES
#define FIXED_ONE				(1 << 16)
#define UNUSED(x)				(void)(x)


typedef struct LinkedEdge LinkedEdge;
struct LinkedEdge {
	LinkedEdge	*next;
	FskEdge		edge;
};


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									Polygon								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * Init_LinkedEdge
 ********************************************************************************/

static UInt32
Init_LinkedEdge(register const FskFixedPoint2D *p0, register const FskFixedPoint2D *p1, const FskSpan *span, register LinkedEdge *e)
{
	FskFixed	dy, sy;

	dy = p1->y - p0->y;													/* Height of edge */
	if (dy == 0)														/* Degenerate edge */
		return 0;
	e->edge.orientation = 1;
	if (dy < 0) {														/* Reorient upside-down edges */
		const FskFixedPoint2D *p;
		p  =  p0;
		p0 =  p1;
		p1 =  p;
		dy = -dy;
		e->edge.orientation = -e->edge.orientation;
	}
	e->edge.top    = (short)CeilFixedToInt(p0->y);						/* Top and bottom scanlines */
	e->edge.bottom = (short)CeilFixedToInt(p1->y);
	if (e->edge.top == e->edge.bottom)									/* Degenerate edge */
		return 0;

	sy			= (e->edge.top << 16) - p0->y;							/* Adjust edge to the top scanline */
	e->edge.dx	= p1->x - p0->x;
	e->edge.x	= p0->x + (FskFixed)(((FskInt64)(e->edge.dx)) * sy / dy);
	if (dy > 0x8000)
		e->edge.dx = (FskFixed)(((FskInt64)(e->edge.dx) << 16) / dy);
	e->next = NULL;														/* Initialize link */

	if (span->initEdge != NULL)
		(*span->initEdge)(p0, p1, span, &e->edge);						/* Additional edge initialization */

	return 1;															/* Good edge */
}


/*******************************************************************************
 * Link_LinkedEdges
 *******************************************************************************/

static void
Link_LinkedEdges(register UInt32 nEdges, register LinkedEdge *edge, UInt32 edgeBytes)
{
	register LinkedEdge *next;
	for (nEdges--; nEdges-- > 0; edge = next) {
		next = (LinkedEdge*)((char*)edge + edgeBytes);
		edge->next = next;
	}
	edge->next = NULL;
}


/*******************************************************************************
 * XSortInsert_LinkedEdge
 *******************************************************************************/

static void
XSortInsert_LinkedEdge(LinkedEdge *edge, LinkedEdge **edges)
{
	register LinkedEdge **lastNext;
	register FskFixed x = edge->edge.x;

	for (lastNext = edges; *lastNext != NULL; lastNext = &((**lastNext).next))
		if ((x < (**lastNext).edge.x) || ((x == (**lastNext).edge.x) && (edge->edge.dx < (**lastNext).edge.dx)))
			break;
	edge->next = *lastNext;
	*lastNext = edge;
}

#if 0

/*******************************************************************************
 * PreviousNext_LinkedEdge
 *******************************************************************************/

static LinkedEdge**
PreviousNext_LinkedEdge(LinkedEdge **list, LinkedEdge *e)
{
	register LinkedEdge *p, **lastNext;

	for (p = *(lastNext = list); (p != NULL) && (p != e); p = *(lastNext = &p->next)) ;

	return(lastNext);
}
#endif

/*******************************************************************************
 * YSort_LinkedEdges
 *******************************************************************************/

static void
YSort_LinkedEdges(LinkedEdge **edges)
{
	register int sorted;
	register LinkedEdge **lastNext, *e0, *e1;

	/* Bubble sort */
	for (sorted = 0; !sorted; ) {
		sorted = 1;
		for (lastNext = edges; *lastNext != NULL; lastNext = &((**lastNext).next)) {
			e0 = *lastNext;
			if ((e1 = e0->next) == NULL)
				break;
			if (	( e1->edge.top <  e0->edge.top)
				||	((e1->edge.top == e0->edge.top)
						&& (	( e1->edge.x <  e0->edge.x)
							||	((e1->edge.x == e0->edge.x) && (e1->edge.dx < e0->edge.dx))
					)
				)
			) {
				e0->next = e1->next;
				e1->next = e0;
				*lastNext  = e1;
				sorted = 0;
			}
		}
	}
}


/*******************************************************************************
 * XSort_LinkedEdges
 *******************************************************************************/

static void
XSort_LinkedEdges(LinkedEdge **edges)
{
	register int sorted;
	register LinkedEdge **lastNext, *e0, *e1;

	/* Bubble sort */
	for (sorted = 0; !sorted; ) {
		sorted = 1;
		for (lastNext = edges; *lastNext != NULL; lastNext = &((**lastNext).next)) {
			e0 = *lastNext;
			if ((e1 = e0->next) == NULL)
				break;
			if (	( e1->edge.x <  e0->edge.x)
				||	((e1->edge.x == e0->edge.x) && (e1->edge.dx < e0->edge.dx))
			) {
				e0->next = e1->next;
				e1->next = e0;
				*lastNext = e1;
				sorted = 0;
			}
		}
	}
}


/*******************************************************************************
 * RemoveDegenerate_LinkedEdges
 *******************************************************************************/

static void
RemoveDegenerate_LinkedEdges(LinkedEdge **edges, SInt32 fillRule)
{
	register LinkedEdge **lastNext, *e0, *e1;
	for (lastNext = edges; *lastNext != NULL; ) {
		e0 = *lastNext;
		if ((e1 = e0->next) == NULL)
			break;
		if (	(e0->edge.top		== e1->edge.top)
			&&	(e0->edge.bottom	== e1->edge.bottom)
			&&	(e0->edge.x			== e1->edge.x)
			&&	(e0->edge.dx		== e1->edge.dx)
			&&	((fillRule == kFskFillRuleEvenOdd) || ((e0->edge.orientation + e1->edge.orientation) == 0))
		) {	/* Remove them both */
			*lastNext = e1->next;
		}
		else {
			lastNext = &(e0->next);
		}
	}
}


/*******************************************************************************
 * ScanConvertLinkedEdges
 *******************************************************************************/

static FskErr
ScanConvertLinkedEdges(
	UInt32		nEdges,
	LinkedEdge	*edges,
	SInt32		fillRule,
	FskSpan		*span
)
{
	LinkedEdge			*pendingEdges, *activeEdges;
	register LinkedEdge	*e;
	SInt32				y, left, right, windingNumber;
	int					rendered	= 0;					/* Keep track of whether we wrote pixels or not */


	/* Initialize edge list */
	pendingEdges = edges;
	Link_LinkedEdges(nEdges, pendingEdges, span->edgeBytes);
	YSort_LinkedEdges(&pendingEdges);
	RemoveDegenerate_LinkedEdges(&pendingEdges, fillRule);

	if (pendingEdges == NULL)
		return kFskErrNothingRendered;

	/* Initialize active edges */
	y = pendingEdges->edge.top;
	activeEdges = NULL;
	while ((pendingEdges != NULL) || (activeEdges != NULL)) {
		SInt32 nextBorn, nextExpired, nextEvent;

		/* Remove expired edges */
		{	register LinkedEdge **lastNext;
			for (lastNext = &activeEdges; (e = *lastNext) != NULL; ) {
				if (y >= e->edge.bottom)	*lastNext = e->next;	/* Edge is expired: remove from active list */
				else						lastNext = &e->next;	/* Edge still active: look at the next one */
			}
		}

		/* Collect newborn edges */
		while ((pendingEdges != NULL) && (pendingEdges->edge.top <= y)) {	/* Should ALWAYS be == */
			e = pendingEdges;
			pendingEdges = e->next;
			XSortInsert_LinkedEdge(e, &activeEdges);
		}
		nextBorn = (SInt32)((pendingEdges == NULL) ? kFskSInt32Max : pendingEdges->edge.top);

		/* Determine the next edge expiration */
		if (activeEdges == NULL) {
			nextExpired = kFskSInt32Max;
		}
		else {
			for (nextExpired = activeEdges->edge.bottom, e = activeEdges->next; e != NULL; e = e->next)
				if (nextExpired > e->edge.bottom)
					nextExpired = e->edge.bottom;
		}

		nextEvent = (nextBorn < nextExpired) ? nextBorn : nextExpired;

		/* Scan all active trapezoids until the next event */
		if (nextEvent < kFskSInt32Max) {
			register LinkedEdge *L, *R;
			SInt32 dy = nextEvent - y;

			/* Render all polygons in scanline order */
			for (windingNumber = 0; dy--; y++) {
				XSort_LinkedEdges(&activeEdges);
				for (L = activeEdges; L != NULL; L = R->next) {
					if (fillRule == kFskFillRuleNonZero) {
						windingNumber += L->edge.orientation;
						R = L;
						do {
							if ((R = R->next) == NULL)		/* Parity problem ... */
								goto nextLine;				/* ... game over */
							windingNumber += R->edge.orientation;
						} while (windingNumber != 0);
					}
					else {
						if ((R = L->next) == NULL)			/* Parity problem ... */
							break;							/* ... game over */
					}
					left  =  CeilFixedToInt(L->edge.x);
					right = FloorFixedToInt(R->edge.x);
					if ((span->dx = right - left + 1) > 0) {
						rendered = 1;
						span->p = (char*)(span->baseAddr) + y * span->rowBytes + left * span->pixelBytes;
						if (span->set != NULL)
							span->set(&L->edge, &R->edge, left, y, span);
						span->fill(span);					/* ... fill it */
					}
				}
				nextLine:

				/* Advance all polygon edges */
				if (span->advanceEdge == NULL) {							/* Simple advancement */
					for (L = activeEdges; L != NULL; L = L->next) {
						L->edge.x += L->edge.dx;							/* Advance the x crossing */
					}
				}
				else {														/* Special advancement */
					for (L = activeEdges; L != NULL; L = L->next) {
						L->edge.x += L->edge.dx;							/* Advance the x crossing */
						span->advanceEdge(&L->edge);						/* Any additional advancement */
					}
				}
			}
			y = nextEvent;
		}
		else
			break;
	}

	return(rendered ? kFskErrNone : kFskErrNothingRendered);
}


/*******************************************************************************
* TransformClipAndMakeEdges
*******************************************************************************/

static SInt32
TransformClipAndMakeEdges(UInt32 nPts, const FskFixedPoint2D *pts, const FskFixedMatrix3x2 *M,FskConstRectangle clipRect, const FskSpan *span, LinkedEdge *edges)
{
	register UInt32	i;
	UInt32			numClipPts, numEdges;
	FskFixedPoint2D	clipPts[MAX_CLIPPED_VERTICES], *pClipPts = clipPts;
	FskFixedPoint2D	*pp					= (FskFixedPoint2D*)edges, *lp;

	if ((nPts > MAX_VERTICES) && (FskMemPtrNew(nPts * CLIP_MULTIPLIER * sizeof(FskFixedPoint2D), (FskMemPtr*)(void*)(&pClipPts)) != kFskErrNone))
		return(-1);

	/* Copy/transform points to a temporary buffer, because the clipper needs to duplicate the first point. */
	if (M == NULL) {
		register const SInt32	*f;
		register SInt32			*t;
		for (i = nPts * sizeof(FskFixedPoint2D) / sizeof(SInt32), f = (const SInt32*)pts, t = (SInt32*)pp; i--; )
			*t++ = *f++;
	}
	else {
		FskTransformFixedRowPoints2D(pts, nPts, M, pp);
	}

	FskClipPolygon2D(nPts, pp, clipRect, &numClipPts, pClipPts);	/* This duplicates the first vertex to beyond the last */

	for (i = numClipPts, numEdges = 0, pp = pClipPts, lp = pClipPts + numClipPts - 1; i--; lp = pp++) {
		if (Init_LinkedEdge(lp, pp, span, edges)) {
			edges = (LinkedEdge*)((char*)edges + span->edgeBytes);
			numEdges++;
		}
	}

	if (pClipPts != clipPts)
		FskMemPtrDispose(pClipPts);	/* Deallocate big polygon clip points */

	return numEdges;
}


/*******************************************************************************
 * FrameJaggyNarrowPolyLine
 *******************************************************************************/

static FskErr
FrameJaggyNarrowPolyLine(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	UInt32					closed,
	FskConstColorRGB		frameColor,
	const FskFixedMatrix3x2	*M,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr					err;
	FskRectangleRecord		dstRect;
	FskFixedPoint2D			pt[3];
	const FskFixedPoint2D	*p0, *p1;
	UInt32					k;

	if (clipRect == NULL)	dstRect = dstBM->bounds;
	else					if (!FskRectangleIntersect(&dstBM->bounds, clipRect, &dstRect)) return kFskErrNothingRendered;

	if (closed) {
		p0 = pts + nPts - 1;
		p1 = pts;
	}
	else {
		p0 = pts;
		p1 = p0 + 1;
		nPts--;
	}

	if (M == NULL) {
		for (k = 0; nPts--; p0 = p1++) {
			pt[0] = *p0;
			pt[1] = *p1;
			if (FskClipLine2D(&dstRect, pt+0, pt+1)) {
				FskDrawClippedJaggedLine(pt+0, pt+1, frameColor, dstBM);
				k++;
			}
		}
	}
	else {
		FskTransformFixedRowPoints2D(p0, 1, M, pt+0);					/* Transform last/first point */
		for (k = 0; nPts--; pt[0] = pt[2], p1++) {
			FskTransformFixedRowPoints2D(p1, 1, M, pt+2);				/* Transform next point, and save it for the next iteration */
			pt[1] = pt[2];												/* Make copy of transformed point for clipper */
			if (FskClipLine2D(&dstRect, pt+0, pt+1)) {					/* Both pt[0] and pt[1] have been clipped */
				FskDrawClippedJaggedLine(pt+0, pt+1, frameColor, dstBM);
				k++;
			}
		}
	}

	err =  k ? kFskErrNone : kFskErrNothingRendered;

	return err;
}


/*******************************************************************************
 * ScaleStrokeWidth
 *******************************************************************************/

static FskFixed
ScaleStrokeWidth(FskFixed strokeWidth, const FskFixedMatrix3x2 *M)
{
	return FskFixMul(strokeWidth, FskFixedMatrixNorm2x2(M->M[0], 2));
}


#define CANT_USE_AUTO_EDGES(totPts)	((totPts * span.edgeBytes) > (MAX_VERTICES * sizeof(LinkedEdge)))

/*******************************************************************************
 * FillJaggyPolygonContours
 *******************************************************************************/

static FskErr
FillJaggyPolygonContours(
	UInt32					nContours,
	const UInt32			*nPts,
	const FskFixedPoint2D	*pts,
	const FskColorSource	*colorSource,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskRectangleRecord	dstRect;
	LinkedEdge			edges[MAX_EDGES], *pEdges = edges;	/* Small polygons use auto edges, large ones need to be alloc'ed */
	UInt32				numEdges, totPts;
	FskSpan				span;
	FskErr				err = kFskErrNone;
	SInt32				n;
	const UInt32		*np;
	LinkedEdge			*pe;
	static FskInitSpanProc	initProcs[] = {
								FskInitSolidColorSpan,
								FskInitLinearGradientSpan,
								FskInitRadialGradientSpan,
								FskInitTextureSpan,
								NULL						/* Procedure span is not yet implemented */
							};
	FskInitSpanProc		initSpan;

	if (clipRect == NULL)	dstRect = dstBM->bounds;
	else					if (!FskRectangleIntersect(&dstBM->bounds, clipRect, &dstRect)) return kFskErrNothingRendered;

	for (n = nContours, np = nPts, totPts = 0; n--; )
		totPts += *np++;

	span.disposeSpanData = span.spanData = NULL;
	FskInitSpan(&span, dstBM, sizeof(LinkedEdge));										/* Generic span init */
	initSpan =  initProcs[((n = colorSource->type) <= kFskColorSourceTypeMax) ? n : 0];
	if ((err = (*initSpan)(&span, dstBM, M, 0, colorSource)) != kFskErrNone)			/* Specialized span init - this may bump up the edge size */
		return err;
	span.edgeBytes = (span.edgeBytes + 3) & ~3;											/* Ceil up to multiple of 4 bytes */
	if (	CANT_USE_AUTO_EDGES(totPts)																							/* Either small enough for auto storage ... */
		&&	(err = (FskMemPtrNew(totPts * CLIP_MULTIPLIER * sizeof(LinkedEdge), (FskMemPtr*)(void*)(&pEdges))) != kFskErrNone)	/* ... or we can allocate memory */
	)
		return err;																		/* Polygon is too big to render */

	for (numEdges = 0, pe = pEdges; nContours--; pts += *nPts++, numEdges += n, pe += n)
		BAIL_IF_NEGATIVE(n = TransformClipAndMakeEdges(*nPts, pts, M, &dstRect, &span, pe), err, kFskErrMemFull);
	BAIL_IF_FALSE(numEdges >= 2, err, kFskErrNothingRendered);
	BAIL_IF_ERR(err = FskBitmapWriteBegin(dstBM, NULL, NULL, NULL));
	err = ScanConvertLinkedEdges(numEdges, pEdges, fillRule, &span);
	FskBitmapWriteEnd(dstBM);

bail:
	if (pEdges != edges)
		FskMemPtrDispose(pEdges);	/* Deallocate big polygon edges */

	if ((span.disposeSpanData != NULL) && (span.spanData != NULL))
		span.disposeSpanData(span.spanData);

	return err;
}


/********************************************************************************
 * DashBuddy
 ********************************************************************************/

typedef struct DashBuddy {
	FskFixed				strokeWidth;		/* Parameters for FskFramePolyLine */
	FskFixed				jointSharpness;
	UInt32					endCaps;
	FskColorSourceUnion		frameColor;
	const FskFixedMatrix3x2	*M;
	UInt32					quality;
	FskConstRectangle		clipRect;
	FskBitmap				dstBM;

	FskFixed				dashPhase;			/* The current phase */
	FskFixed				dashPeriod;			/* The period of the dash pattern */
	FskFixedVector2D		dashBasis[2];		/* The basis of the dash in the point coordinate system */
	UInt32					dashCycles;			/* The number of cycles in the dash pattern */
	const FskFixed			*dashSegments;		/* The durations of each phase of the dash pattern */
	FskFixed				*dashSegmentPhases;	/* The accumulated phase at this dash transition point */
	FskFixed				*resetSegmentPhase;	/* When this phase is reached, rest to the beginning */
	FskFixed				*thisPhaseSegment;	/* The current segment -- avoids search on each segment */

	FskFixedPoint2D			lastPt;				/* The point at the beginning of the current segment */
	FskGrowableArray		ptArray;			/* A working set of points */
} DashBuddy;



/********************************************************************************
 * DashFirstPoint
 ********************************************************************************/

static FskErr
DashFirstPoint(DashBuddy *db, const FskFixedPoint2D *pt)
{
	FskFixed	*p;
	UInt32		i;

	db->lastPt = *pt;
	FskGrowableArraySetItemCount(db->ptArray, 0);

	/* Find the correct segment */
	db->thisPhaseSegment = db->dashSegmentPhases;
	for (i = db->dashCycles, p = db->dashSegmentPhases; i--; p += 2) {
		if ((db->dashPhase >= p[0]) && (db->dashPhase < p[1])) {
			db->thisPhaseSegment = p;
			return FskGrowableArrayAppendItem(db->ptArray, pt);
		}
		if ((db->dashPhase >= p[1]) && (db->dashPhase < p[2])) {
			db->thisPhaseSegment = p + 1;
			return kFskErrNone;
		}
	}
	return kFskErrOutOfSequence;
}


/********************************************************************************
 * DrawDashSegment
 ********************************************************************************/

static void
DrawDashSegment(DashBuddy *db, const FskFixedPoint2D *pt)
{
	UInt32					nPts;
	const FskFixedPoint2D	*pts;

	if (pt != NULL)	(void)FskGrowableArrayAppendItem(db->ptArray, pt);								/* Append last point before drawing */
	if ((nPts = FskGrowableArrayGetItemCount(db->ptArray)) > 1) {									/* If there are at least two points, draw the polyline */
		FskGrowableArrayGetConstPointerToItem(db->ptArray, 0, (const void**)(const void*)(&pts));	/* Get points */
		FskFramePolyLine(nPts, pts, db->strokeWidth, db->jointSharpness, db->endCaps, &db->frameColor.so, db->M, db->quality, db->clipRect, db->dstBM);
	}
	FskGrowableArraySetItemCount(db->ptArray, 0);													/* Clear point storage */
}


/********************************************************************************
 * DashNextSegment
 ********************************************************************************/

static FskErr
DashNextSegment(DashBuddy *db, const FskFixedPoint2D *pt)
{
	FskErr				err;
	FskFixedPoint2D		q;
	FskFixedVector2D	v;
	FskFixed			segPhase, endPhase, deltaPhase;
	FskFract			t;
	UInt32				i;

	q = db->lastPt;																										/* Point of last state change */
	v.x = pt->x - db->lastPt.x;		v.y = pt->y - db->lastPt.y;															/* Vector along the line segment */
	segPhase = FskFixedHypot(FskFixedDotProduct2D(&v, db->dashBasis+0), FskFixedDotProduct2D(&v, db->dashBasis+1));		/* Phase delta for this segment (Riemannian metric) */
	endPhase = db->dashPhase + segPhase;																				/* Phase at end of this line segment */

	while (db->thisPhaseSegment[1] <= endPhase) {
		/* We know that we're going to cross a dash segment */
		deltaPhase = db->thisPhaseSegment[1] - db->dashPhase;															/* Phase change since the last state change */
		t = FskFracDiv(deltaPhase, segPhase);		q.x += FskFracMul(t, v.x);		q.y += FskFracMul(t, v.y);			/* Compute location of next transition */
		if ((i = FskGrowableArrayGetItemCount(db->ptArray)) > 0)	DrawDashSegment(db, &q);							/* ON transitioning to OFF: end and draw this dash segment */
		else														(void)FskGrowableArrayAppendItem(db->ptArray, &q);	/* OFF transition to ON: add the start point */
		if (++(db->thisPhaseSegment) >= db->resetSegmentPhase) {														/* Advance phase segment... */
			db->thisPhaseSegment = db->dashSegmentPhases;																/* ... and reset if at the end of the phase period */
			endPhase -= db->dashPeriod;																					/* The end is getting closer */
		}
		db->dashPhase = db->thisPhaseSegment[0];																		/* Set the phase */
	}

	/* The end is still within the same dash segment */
	if ((i = FskGrowableArrayGetItemCount(db->ptArray)) > 0)	err = FskGrowableArrayAppendItem(db->ptArray, pt);	/* In the ON  state: add a kink point */
	else														err = kFskErrNone;										/* In the OFF state */
	db->dashPhase = endPhase;																							/* Update the phase */
	db->lastPt = *pt;
	return err;
}


/********************************************************************************
 * FrameDashedPolyLine
 ********************************************************************************/

static FskErr
FrameDashedPolyLine(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr		err = kFskErrNone;
	DashBuddy	dashBuddy;
	FskFixed	*dur, *acc, scale;
	SInt32		i;

	dashBuddy.strokeWidth		= strokeWidth;
	dashBuddy.jointSharpness	= jointSharpness;
	dashBuddy.M					= M;
	dashBuddy.quality			= quality;
	dashBuddy.clipRect			= clipRect;
	dashBuddy.dstBM				= dstBM;
	dashBuddy.ptArray			= NULL;															/* We allocate these below */
	dashBuddy.dashSegmentPhases	= NULL;															/* We allocate these below */
	dashBuddy.dashCycles		= frameColor->dashCycles;
	dashBuddy.dashSegments		= frameColor->dash;
	dashBuddy.endCaps			= endCaps & ~kFskLineEndCapClosed;

	if (dashBuddy.dashCycles == 0)																/* We should never be called like this */
		return FskFramePolyLine( nPts, pts, strokeWidth, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM);

	/* Make a modified version of the color source, removing the dashing */
	switch (frameColor->type) {
		case kFskColorSourceTypeConstant:		dashBuddy.frameColor.cn = *((FskColorSourceConstant*      )frameColor);	break;
		case kFskColorSourceTypeLinearGradient:	dashBuddy.frameColor.lg = *((FskColorSourceLinearGradient*)frameColor);	break;
		case kFskColorSourceTypeRadialGradient:	dashBuddy.frameColor.rg = *((FskColorSourceRadialGradient*)frameColor);	break;
		case kFskColorSourceTypeProcedure:		dashBuddy.frameColor.pr = *((FskColorSourceProcedure*     )frameColor);	break;
	}
	dashBuddy.frameColor.so.dashCycles = 0;														/* Remove dashing from the sub-color-source */
	dashBuddy.frameColor.so.dash       = NULL;													/* Remove dashing from the sub-color-source */

	/* Allocate a point array */
	BAIL_IF_ERR(err = FskGrowableArrayNew(sizeof(FskFixedPoint2D), 10, &dashBuddy.ptArray));
	BAIL_IF_ERR(err = FskMemPtrNew((dashBuddy.dashCycles * 2 + 1) * sizeof(FskFixed), (FskMemPtr*)(void*)(&dashBuddy.dashSegmentPhases)));

	/* Compute dash period and accumulate dash phase */
	scale = M ? FskFixedMatrixNorm2x2(M->M[0], 3) : 0x10000;
	for (i = dashBuddy.dashCycles * 2, dur = frameColor->dash, acc = dashBuddy.dashSegmentPhases, *acc = dashBuddy.dashPeriod = 0; i--; )
		*++acc = dashBuddy.dashPeriod += FskFixMul(*dur++, scale);								/* Accumulate phase at each dash transition point */
	dashBuddy.dashPeriod = *acc;																/* Period is the last phase */
	BAIL_IF_ZERO(dashBuddy.dashPeriod, err, kFskErrNothingRendered);							/* Zero period causes problems: abort early */
	dashBuddy.resetSegmentPhase = acc;															/* Reset the phase at the end of the period */

	/* Compute principal value of starting phase */
	dashBuddy.dashPhase = frameColor->dashPhase % dashBuddy.dashPeriod;							/* Within a period of zero */
	if (dashBuddy.dashPhase < 0) dashBuddy.dashPhase += dashBuddy.dashPeriod;					/* Positive */

	/* Compute dash basis */
	if (M != NULL) {
		dashBuddy.dashBasis[0].x = M->M[0][0];	dashBuddy.dashBasis[0].y = M->M[1][0];
		dashBuddy.dashBasis[1].x = M->M[0][1];	dashBuddy.dashBasis[1].y = M->M[1][1];
		if (FskFixedMatrixInvert2x2(&dashBuddy.dashBasis[0].x, &dashBuddy.dashBasis[0].x) == 0)
			goto singularMatrix;
	}
	else {
	singularMatrix:	/* We do this rather than failing */
		dashBuddy.dashBasis[0].x = dashBuddy.dashBasis[1].y = 1 << 16;
		dashBuddy.dashBasis[0].y = dashBuddy.dashBasis[1].x = 0;
	}

	/* Draw dashes */
	DashFirstPoint(&dashBuddy, pts++);
	for (i = nPts - 1; i--; )
		DashNextSegment(&dashBuddy, pts++);                 /* Draw internal segments */
	if (endCaps & kFskLineEndCapClosed) {
		FskFixedPoint2D	smidge;
		DashNextSegment(&dashBuddy, pts -= nPts);           /* Draw closing segment, if desired */
		smidge.x = pts[1].x - pts[0].x;
		smidge.y = pts[1].y - pts[0].y;
		FskFixedVector2DNormalize(&smidge.x);
		smidge.x = ((smidge.x + (1 << (30 - 15 - 1))) >> (30 - 15)) + pts[0].x;					/* Extend another 1/2 pixel */
		smidge.y = ((smidge.y + (1 << (30 - 15 - 1))) >> (30 - 15)) + pts[0].y;
		DashNextSegment(&dashBuddy, &smidge);
	}
	DrawDashSegment(&dashBuddy, NULL);															/* Finish up any undrawn segments */

bail:
	if (dashBuddy.ptArray != NULL)				FskGrowableArrayDispose(dashBuddy.ptArray);
	if (dashBuddy.dashSegmentPhases != NULL)	FskMemPtrDispose(dashBuddy.dashSegmentPhases);

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




/*******************************************************************************
 * FskFillPolygon
 *******************************************************************************/

FskErr
FskFillPolygon(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr err;

	if (quality == 0) {
		err = FillJaggyPolygonContours(1, &nPts, pts,     fillColor,       fillRule, M, clipRect, dstBM);
	}
	else {
		const FskColorSource *frameFillColors[2];
		frameFillColors[0] = NULL;
		frameFillColors[1] = fillColor;
		err = FskAAPolygonContours(    1, &nPts, pts, -1, frameFillColors, fillRule, M, clipRect, dstBM);
	}

	return err;
}


/*******************************************************************************
 * FskFramePolygon
 *******************************************************************************/

FskErr
FskFramePolygon(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr					err;
	const FskColorSource	*frameFillColors[2];

	if (frameColor->dashCycles != 0)	/* Do special processing for dashed lines: @@@ should have endcap spec for dashes */
		return FrameDashedPolyLine(nPts, pts, strokeWidth, jointSharpness, kFskLineEndCapClosed|kFskLineEndCapButt, frameColor, M, quality, clipRect, dstBM);

	if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);

	if (	(strokeWidth <= FIXED_ONE)
		&&	(frameColor->type == kFskColorSourceTypeConstant)
	) {
		if (quality == 0) {
			FskConstColorRGB frColor = &((const FskColorSourceUnion*)frameColor)->rgb.color;
			err = FrameJaggyNarrowPolyLine(     nPts, pts,   1, frColor,              M, clipRect, dstBM);
		} else {
			frameFillColors[0] = frameColor;
			frameFillColors[1] = NULL;
			err = FskAAPolygonContours(1, &nPts, pts, FIXED_ONE, frameFillColors, -1, M, clipRect, dstBM);
		}
	}
	else {
		FskGrowableFixedPoint2DArray	pgon		= NULL;
		UInt32							n;
		FskFixedPoint2D					*p;

		BAIL_IF_ERR(err = FskWidePolyLineToPolygon(nPts, pts, strokeWidth, jointSharpness, kFskLineEndCapClosed, M, &pgon));
		BAIL_IF_ZERO((n = FskGrowableFixedPoint2DArrayGetItemCount(pgon)), err, kFskErrNothingRendered);
		BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetPointerToItem(pgon, 0, (void**)(void*)(&p)));
		if (quality == 0) {
			err = FillJaggyPolygonContours(1, &n, p,     frameColor,      kFskFillRuleNonZero, NULL, clipRect, dstBM);
		} else {
			frameFillColors[0] = NULL;
			frameFillColors[1] = frameColor;
			err = FskAAPolygonContours(    1, &n, p, -1, frameFillColors, kFskFillRuleNonZero, NULL, clipRect, dstBM);
		}

	bail:
		if (pgon != NULL)	FskGrowableFixedPoint2DArrayDispose(pgon);
	}

	return err;
}


/*******************************************************************************
 * FskFrameFillPolygon
 *******************************************************************************/

FskErr
FskFrameFillPolygon(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr	err;

	if ((err = FillJaggyPolygonContours(1, &nPts, pts, fillColor, fillRule, M, clipRect, dstBM)) >= 0)
		err = FskFramePolygon(nPts, pts, strokeWidth, jointSharpness, frameColor, M, quality, clipRect, dstBM);
	return err;
}


/********************************************************************************
 * FskFramePolyLine
 ********************************************************************************/

FskErr
FskFramePolyLine(
	UInt32					nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr					err;
	const FskColorSource	*frameFillColors[2];

	if (frameColor->dashCycles != 0)	/* Do special processing for dashed lines */
		return FrameDashedPolyLine(nPts, pts, strokeWidth, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM);

	/* Detect closed polylines */
	if ((pts[0].x == pts[nPts-1].x) && (pts[0].y == pts[nPts-1].y)) {
		nPts--;
		endCaps |= kFskLineEndCapClosed;
	}

	if (nPts < 2)
		return kFskErrNone;	/* Degenerate line segment */

	if (M)	strokeWidth = ScaleStrokeWidth(strokeWidth, M);

	if (	(strokeWidth <= FIXED_ONE)
		&&	((quality == 0) || ((endCaps & kFskLineEndCapClosed) != 0))
		&&	(frameColor->type == kFskColorSourceTypeConstant)
	) {
		if (quality == 0) {
			FskConstColorRGB frColor = &((const FskColorSourceUnion*)frameColor)->rgb.color;
			err = FrameJaggyNarrowPolyLine(nPts, pts, ((endCaps & kFskLineEndCapClosed) != 0), frColor, M, clipRect, dstBM);
		}
		else {
			frameFillColors[0] = frameColor;
			frameFillColors[1] = NULL;
			err = FskAAPolygonContours(1, &nPts, pts, FIXED_ONE, frameFillColors, -1, M, clipRect, dstBM);
		}
	}
	else {
		FskGrowableFixedPoint2DArray	pgon = NULL;
		UInt32							n;
		FskFixedPoint2D					*p;

		BAIL_IF_ERR(err = FskWidePolyLineToPolygon(nPts, pts, strokeWidth, jointSharpness, endCaps, M, &pgon));
		BAIL_IF_ZERO((n = FskGrowableFixedPoint2DArrayGetItemCount(pgon)), err, kFskErrNothingRendered);
		BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetPointerToItem(pgon, 0, (void**)(void*)(&p)));
		if (quality == 0) {
			err = FillJaggyPolygonContours(1, &n, p,     frameColor,      kFskFillRuleNonZero, NULL, clipRect, dstBM);
		} else {
			frameFillColors[0] = NULL;
			frameFillColors[1] = frameColor;
			err = FskAAPolygonContours(    1, &n, p, -1, frameFillColors, kFskFillRuleNonZero, NULL, clipRect, dstBM);
		}

	bail:
		if (pgon != NULL)	FskGrowableFixedPoint2DArrayDispose(pgon);
	}

	return err;
}


/********************************************************************************
 * FskFrameLine
 ********************************************************************************/

FskErr
FskFrameLine(
	const FskFixedPoint2D	*p0,
	const FskFixedPoint2D	*p1,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskFixedPoint2D		pt[2];

	pt[0] = *p0;
	pt[1] = *p1;
	return FskFramePolyLine(2, pt, strokeWidth, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM);
}


/*******************************************************************************
 * FskFillPolygonContours
 *******************************************************************************/

FskErr
FskFillPolygonContours(
	UInt32					nContours,
	const UInt32			*nPts,
	const FskFixedPoint2D	*pts,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr err;

	if (quality == 0) {
		err = FillJaggyPolygonContours(nContours, nPts, pts,     fillColor,       fillRule, M, clipRect, dstBM);
	}
	else {
		const FskColorSource *frameFillColors[2];
		frameFillColors[0] = NULL;
		frameFillColors[1] = fillColor;
		err = FskAAPolygonContours(    nContours, nPts, pts, -1, frameFillColors, fillRule, M, clipRect, dstBM);
	}

	return err;
}


/*******************************************************************************
 * FskFramePolygonContours
 *******************************************************************************/

FskErr
FskFramePolygonContours(
	UInt32					nContours,
	const UInt32			*nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr	thisErr, err	= kFskErrNothingRendered;

	for ( ; nContours--; pts += *nPts++)
		if ((thisErr = FskFramePolygon(*nPts, pts, strokeWidth, jointSharpness, frameColor, M, quality, clipRect, dstBM)) < err)
			if ((err = thisErr) < kFskErrNone)
				goto bail;

bail:
	return err;
}


/*******************************************************************************
 * FskFrameFillPolygonContours
 *******************************************************************************/

FskErr
FskFrameFillPolygonContours(
	UInt32					nContours,
	const UInt32			*nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
	SInt32					fillRule,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr	err, thisErr;

	if ((err = FillJaggyPolygonContours(nContours, nPts, pts, fillColor, fillRule, M, clipRect, dstBM)) >= kFskErrNone)
		if ((thisErr = FskFramePolygonContours(nContours, nPts, pts, strokeWidth, jointSharpness, frameColor, M, quality, clipRect, dstBM)) < err)
			err = thisErr;

	return err;
}


/*******************************************************************************
 * FskFramePolylineContours
 *******************************************************************************/

FskErr
FskFramePolylineContours(
	UInt32					nContours,
	const UInt32 			*nPts,
	const FskFixedPoint2D	*pts,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	UInt32					endCaps,
	const FskColorSource	*frameColor,
	const FskFixedMatrix3x2	*M,
	UInt32					quality,
	FskConstRectangle		clipRect,
	FskBitmap				dstBM
)
{
	FskErr	thisErr, err	= kFskErrNothingRendered;

	for ( ; nContours--; pts += *nPts++)
		if ((thisErr = FskFramePolyLine(*nPts, pts, strokeWidth, jointSharpness, endCaps, frameColor, M, quality, clipRect, dstBM)) < err)
			if ((err = thisErr) < kFskErrNone)
				goto bail;

bail:
	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */


/*******************************************************************************
 * FskDisposeColorSource
 *******************************************************************************/

void
FskDisposeColorSource(FskColorSource *cs)
{
	FskMemPtrDispose(cs);
}


/*******************************************************************************
 * FskNewColorSourceConstant
 *******************************************************************************/

FskColorSourceConstant*
FskNewColorSourceConstant(UInt32 dashCycles)
{
	FskColorSourceConstant *cs = NULL;

	if (FskMemPtrNewClear(sizeof(FskColorSourceConstant) + dashCycles * 2 * sizeof(FskFixed), &cs) == kFskErrNone) {
		cs->colorSource.type		= kFskColorSourceTypeConstant;
		if ((cs->colorSource.dashCycles = dashCycles) > 0)
			cs->colorSource.dash = (FskFixed*)(cs + 1);
		/* Default color is black */
	}
	return cs;
}


/*******************************************************************************
 * FskNewColorSourceLinearGradient
 *******************************************************************************/

FskColorSourceLinearGradient*
FskNewColorSourceLinearGradient(UInt32 numStops, UInt32 dashCycles)
{
	FskColorSourceLinearGradient *cs	= NULL;

	if (FskMemPtrNewClear((	sizeof(FskColorSourceLinearGradient)
						+	numStops * sizeof(FskGradientStop)
						+	sizeof(FskFixedMatrix3x2)
						+	dashCycles * 2 * sizeof(FskFixed)
						), &cs) == kFskErrNone
	) {
		cs->colorSource.type		= kFskColorSourceTypeLinearGradient;
		cs->colorSource.dashCycles	= dashCycles;
		cs->spreadMethod			= kFskSpreadPad;
		cs->numStops				= numStops;
		cs->gradientStops			= (FskGradientStop*)(cs + 1);
		cs->gradientMatrix			= (FskFixedMatrix3x2*)(cs->gradientStops + numStops);
		cs->gradientMatrix->M[0][0]	= 1 << 16;	cs->gradientMatrix->M[0][1]	= 0 << 16;
		cs->gradientMatrix->M[1][0]	= 0 << 16;	cs->gradientMatrix->M[1][1]	= 1 << 16;
		cs->gradientMatrix->M[2][0]	= 0 << 16;	cs->gradientMatrix->M[2][0]	= 0 << 16;
		cs->gradientVector[0].x		= 0;		cs->gradientVector[0].y		= 0;
		cs->gradientVector[1].x		= 1 << 16;	cs->gradientVector[1].y		= 1 << 16;
		numStops--;
		cs->gradientStops[numStops].offset	= 1 << 30;	/* By default, all offsets are 0 except the last, which is 1 */
		cs->gradientStops[numStops].color.r	= 255;		/* Bt default, all colors are black, except for the last, which is white */
		cs->gradientStops[numStops].color.g	= 255;
		cs->gradientStops[numStops].color.b	= 255;
		do {
			cs->gradientStops[numStops].color.a	= 255;	/* By default, all stops are opaque */
		} while (numStops--);
	}
	return cs;
}


/*******************************************************************************
 * FskNewColorSourceRadialGradient
 *******************************************************************************/

FskColorSourceRadialGradient*
FskNewColorSourceRadialGradient(UInt32 numStops, UInt32 dashCycles)
{
	FskColorSourceRadialGradient *cs	= NULL;

	if (FskMemPtrNewClear((	sizeof(FskColorSourceRadialGradient)
						+	numStops * sizeof(FskGradientStop)
						+	sizeof(FskFixedMatrix3x2)
						+	dashCycles * 2 * sizeof(FskFixed)
						), &cs) == kFskErrNone
	) {
		cs->colorSource.type		= kFskColorSourceTypeRadialGradient;
		cs->colorSource.dashCycles	= dashCycles;
		cs->spreadMethod			= kFskSpreadPad;
		cs->numStops				= numStops;
		cs->radius					= 64 << 16;
		cs->gradientStops			= (FskGradientStop*)(cs + 1);
		cs->gradientMatrix			= (FskFixedMatrix3x2*)(cs->gradientStops + numStops);
		cs->gradientMatrix->M[0][0]	= 1 << 16;	cs->gradientMatrix->M[0][1]	= 0 << 16;
		cs->gradientMatrix->M[1][0]	= 0 << 16;	cs->gradientMatrix->M[1][1]	= 1 << 16;
		cs->gradientMatrix->M[2][0]	= 0 << 16;	cs->gradientMatrix->M[2][0]	= 0 << 16;
		numStops--;
		cs->gradientStops[numStops].offset	= 1 << 30;	/* By default, all offsets are 0 except the last, which is 1 */
		cs->gradientStops[numStops].color.r	= 255;		/* Bt default, all colors are black, except for the last, which is white */
		cs->gradientStops[numStops].color.g	= 255;
		cs->gradientStops[numStops].color.b	= 255;
		do {
			cs->gradientStops[numStops].color.a = 255;	/* By default, all stops are opaque */
		} while (numStops--);
	}
	return cs;
}


/*******************************************************************************
 * FskNewColorSourceTexture
 *******************************************************************************/

FskColorSourceTexture*
FskNewColorSourceTexture(UInt32 dashCycles)
{
	FskColorSourceTexture *cs = NULL;

	if (FskMemPtrNewClear(sizeof(FskColorSourceTexture) + dashCycles * 2 * sizeof(FskFixed), &cs) == kFskErrNone) {
		cs->colorSource.type		= kFskColorSourceTypeTexture;
		if ((cs->colorSource.dashCycles = dashCycles) > 0)
			cs->colorSource.dash = (FskFixed*)(cs + 1);
		/* Default color is black */
	}
	return cs;
}


/*******************************************************************************
 * DefaultColorSourceProc
 *******************************************************************************/

static void
DefaultColorSourceProc(void *userData, const FskFixedPoint2D *uv, FskColorRGB color)
{
	UNUSED(userData);
	UNUSED(uv);
	color->r = 128;
	color->g = 128;
	color->b = 128;
}


/*******************************************************************************
 * FskNewColorSourceProcedure
 *******************************************************************************/

FskColorSourceProcedure*
FskNewColorSourceProcedure(UInt32 dashCycles)
{
	FskColorSourceProcedure	*cs	= NULL;

	if (FskMemPtrNewClear(sizeof(FskColorSourceProcedure) + dashCycles * 2 * sizeof(FskFixed), &cs) == kFskErrNone) {
		cs->colorSource.type		= kFskColorSourceTypeProcedure;
		cs->colorSourceProc			= DefaultColorSourceProc;
		if ((cs->colorSource.dashCycles	= dashCycles) > 0)
			cs->colorSource.dash = (FskFixed*)(cs + 1);
	}
	return cs;
}


/********************************************************************************
 * FskSetGradientMatrixFromFloatArray
 *	Set the gradient matrix from an array of floating point numbers.
 *	bounds can be NULL or fr can be NULL but not both.
 ********************************************************************************/

void
FskSetGradientMatrixFromFloatArray(FskFixedMatrix3x2 *to, const double *fr, FskConstFixedRectangle bounds)
{
	if (bounds != NULL) {
		if (fr != NULL) {					/* Gradient matrix specified relative to bounding box */
			to->M[0][0] = FskRoundAndSaturateFloatToNFixed(fr[0*2+0] * bounds->width,              0);	/* Normalize transformation to bbox */
			to->M[1][0] = FskRoundAndSaturateFloatToNFixed(fr[1*2+0] * bounds->width,              0);
			to->M[2][0] = FskRoundAndSaturateFloatToNFixed(fr[2*2+0] * bounds->width  + bounds->x, 0);
			to->M[0][1] = FskRoundAndSaturateFloatToNFixed(fr[0*2+1] * bounds->height,             0);
			to->M[1][1] = FskRoundAndSaturateFloatToNFixed(fr[1*2+1] * bounds->height,             0);
			to->M[2][1] = FskRoundAndSaturateFloatToNFixed(fr[2*2+1] * bounds->height + bounds->y, 0);
		}
		else {								/* Normalized to bounding box */
			to->M[0][0] = bounds->width;																/* Construct  the bounding box transformation */
			to->M[1][1] = bounds->height;
			to->M[2][0] = bounds->x;
			to->M[2][1] = bounds->y;
			to->M[0][1] = to->M[1][0] = 0;
		}
	}
	else {
		if (fr != NULL) {					/* Gradient matrix was given in absolute coordinates */
			int i;
			for (i = 0; i < 6; i++)
				to->M[0][i] = FskRoundAndSaturateFloatToNFixed(fr[i], 16);								/* Copy the given gradient matrix */
		}
		else {								/* Neither the bounds nor the floating-point gradient matrix were given */
			FskFixedIdentityMatrix3x2(to);																/* Identity */
		}
	}
}


/********************************************************************************
 * ConcatenateGradientMatrices
 *	This computes G * P -> H
 *	P and/or G can be NULL
 *	It works in-place.
 ********************************************************************************/

static void
ConcatenateGradientMatrices(const FskFixedMatrix3x2 *P, const FskFixedMatrix3x2 *G, FskFixedMatrix3x2 *H)
{
	if (P != NULL) {
		if (G != NULL) {	FskFixedMultiplyMatrix3x2(G, P, H);	}	/* P && G */
		else {				if (H != P)		*H = *P;			}	/* P && !G */
	}
	else {
		if (G != NULL) {	if (H != G)		*H = *G;			}	/* !P && G */
		else {				FskFixedIdentityMatrix3x2(H);		}	/* !P *&& !G */
	}
}


/********************************************************************************
 * TransformDash
 ********************************************************************************/

static void
FskTransformDash(const FskFixedMatrix3x2 *M, FskColorSource *cs, FskFixed **newDash)
{
	if (newDash == NULL)
		return;
	if ((cs->dashCycles > 0) && (FskMemPtrNew(cs->dashCycles * 2 * sizeof(FskFixed), (FskMemPtr*)newDash) == kFskErrNone)) {
		UInt32			i;
		const FskFixed	*fr;
		FskFixed		*to, scale;

		scale = FskFixedMatrixNorm2x2(M->M[0], 2);
		for (i = cs->dashCycles * 2, fr = cs->dash, to = *newDash; i--; )
			*to++ = FskFixMul(scale, *fr++);								/* Scale each dash segment */
		cs->dash = *newDash;												/* Replace the old dash with the scaled dash */
		cs->dashPhase = FskFixMul(scale, cs->dashPhase);					/* Scale the dash phase */
	}
	else {
		*newDash = NULL;
	}
}


/********************************************************************************
 * IsIdentityMatrix
 ********************************************************************************/

#define IsIdentityMatrix(m)	((m->M[0][1] | m->M[1][0] | m->M[2][0] | m->M[2][1] | (m->M[0][0] ^ 0x10000) | (m->M[1][1] ^ 0x10000)) == 0)


/********************************************************************************
 * FskTransformColorSource
 ********************************************************************************/

const FskColorSource*					/* The transformed color source: could be either cs0 or cs1 */
FskTransformColorSource(
	const FskColorSource	*cs0,		/* The original color source */
	const FskFixedMatrix3x2	*M0,		/* The geometry transformation matrix */
	FskFixedMatrix3x2		*M1,		/* A new transformation matrix to be provided to the transformed color source */
	FskColorSourceUnion		*cs1,		/* Storage for the new color source if needed */
	FskFixed				**newDash	/* Dash storage if necessary */
)
{
	if ((M0 == NULL) ||	IsIdentityMatrix(M0))
		return cs0;								/* Return the original if transforming doesn't affect it */

	switch (cs0->type) {
		case kFskColorSourceTypeConstant:
			if (cs0->dashCycles == 0)
				return cs0;
			cs1->cn = ((const FskColorSourceUnion*)cs0)->cn;
			FskTransformDash(M0, &cs1->so, newDash);
			return &cs1->so;
		case kFskColorSourceTypeLinearGradient:
			cs1->lg = ((const FskColorSourceUnion*)cs0)->lg;
			ConcatenateGradientMatrices(M0, cs1->lg.gradientMatrix, M1);
			cs1->lg.gradientMatrix = M1;
			FskTransformDash(M0, &cs1->so, newDash);
			return &cs1->so;
		case kFskColorSourceTypeRadialGradient:
			cs1->rg = ((const FskColorSourceUnion*)cs0)->rg;
			ConcatenateGradientMatrices(M0, cs1->rg.gradientMatrix, M1);
			cs1->rg.gradientMatrix = M1;
			FskTransformDash(M0, &cs1->so, newDash);
			return &cs1->so;
		case kFskColorSourceTypeTexture:
			cs1->tx = ((const FskColorSourceUnion*)cs0)->tx;
			if (cs1->tx.textureFrame)	FskFixedMultiplyMatrix3x2(M0, cs1->tx.textureFrame, M1);	//FskFixedMultiplyInverseMatrix3x2(M0, cs1->tx.textureFrame, M1);
			else						*M1 = *M0;
			cs1->tx.textureFrame = M1;
			FskTransformDash(M0, &cs1->so, newDash);
			return &cs1->so;

	}
	return NULL;		/* Not known how a transformation will affect these color sources */
}
