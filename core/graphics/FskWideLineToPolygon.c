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
/**
	\file	FskWideLineToPolygon.c
	\brief	Convert a wide line to a polygon.
*/

#include "FskWideLineToPolygon.h"
#include "FskPath.h"				/* We need this to tessellate arcs */


#define FRACT_ONE			0x40000000
#define FRACT_HALF			(FRACT_ONE >> 1)
#define FRACT_QUARTER		(FRACT_ONE >> 2)
#define FIXED_ONE			0x00010000L
#define LINEAR_TOLERANCE	(FIXED_ONE / 8)

enum {
	ROUNDED_JOIN	= 0,
	BEVELED_JOIN	= 1,
	MITERED_JOIN	= 2
};


typedef struct LineSeg {
	FskFixedPoint2D		pt[2];
	FskFractVector2D	par;
	FskFractVector2D	perp;
	FskFixed			length;
	FskFixed			strokeWidth;
	FskFract			arcRelTol;
	FskFract			sinMiterLimit;
} LineSeg;


typedef FskErr	(*MakeEndCap)(const LineSeg *seg, UInt32 end, FskGrowableFixedPoint2DArray array);
typedef FskErr	(*MakeJoin)(const LineSeg *seg0, const LineSeg *seg1, FskGrowableFixedPoint2DArray fwdPoly, FskGrowableFixedPoint2DArray revPoly);


/*******************************************************************************
 * InitLineSeg
 *******************************************************************************/


static int
InitLineSeg(const FskFixedPoint2D *p0, const FskFixedPoint2D *p1, const FskFixedMatrix3x2 *M, register LineSeg *seg)
{
	int	ok = 1;

	if (M == NULL) {
		seg->pt[0] = *p0;
		seg->pt[1] = *p1;
	}
	else {
		FskTransformFixedRowPoints2D(p0, 1, M, &seg->pt[0]);
		FskTransformFixedRowPoints2D(p1, 1, M, &seg->pt[1]);
	}
	seg->par.x = seg->pt[1].x - seg->pt[0].x;
	seg->par.y = seg->pt[1].y - seg->pt[0].y;
	if ((seg->length = FskFixedVector2DNormalize(&seg->par.x)) < (FIXED_ONE >> 7)) {	/* Consider segments shorter than 1/128 pixel to be degenerate */
		seg->par.x = FRACT_ONE;
		seg->par.y = 0;
		ok = 0;
	}
	seg->perp.x = -seg->par.y;
	seg->perp.y =  seg->par.x;

	return ok;
}


/*******************************************************************************
 * RoundEndCap
 *******************************************************************************/

static FskErr
RoundEndCap(const LineSeg *seg, UInt32 end, FskGrowableFixedPoint2DArray array)
{
	FskErr					err;
	FskFixedPoint2D			start;
	const FskFixedPoint2D	*center;
	FskFixed				strokeWidth		= seg->strokeWidth;

	if (!end) {	/* Starting endpoint */
		center = &seg->pt[0];
	}
	else {		/* Ending endpoint */
		strokeWidth = -strokeWidth;
		center = &seg->pt[1];
	}

	start.x = center->x - FskFixedNMul(strokeWidth, seg->perp.x, 31);
	start.y = center->y - FskFixedNMul(strokeWidth, seg->perp.y, 31);
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(array, &start));
	err = FskTessellateCircularArcStartCenterAngle(&start, center, (-180L << 16), seg->arcRelTol, array);

bail:
	return err;
}


/*******************************************************************************
 * SquareEndCap
 *******************************************************************************/

static FskErr
SquareEndCap(const LineSeg *seg, UInt32 end, FskGrowableFixedPoint2DArray array)
{
	FskErr					err;
	FskFixedVector2D		v;
	FskFixedPoint2D			p;
	const FskFixedPoint2D	*center;

	v.x = FskFixedNMul((seg->par.x + seg->perp.x), seg->strokeWidth, 31);
	v.y = FskFixedNMul((seg->par.y + seg->perp.y), seg->strokeWidth, 31);

	if (!end) {	/* Starting endpoint */
		center = &seg->pt[0];
	}
	else {		/* Ending endpoint */
		v.x = -v.x;
		v.y = -v.y;
		center = &seg->pt[1];
	}

	p.x = center->x - v.x;
	p.y = center->y - v.y;
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(array, &p));

	p.x = center->x - v.y;
	p.y = center->y + v.x;
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(array, &p));

bail:
	return err;
}


/*******************************************************************************
 * ButtEndCap
 *******************************************************************************/

static FskErr
ButtEndCap(const LineSeg *seg, UInt32 end, FskGrowableFixedPoint2DArray array)
{
	FskErr					err;
	FskFixedVector2D		v;
	FskFixedPoint2D			p;
	const FskFixedPoint2D	*center;

	v.x = FskFixedNMul(seg->perp.x, seg->strokeWidth, 31);
	v.y = FskFixedNMul(seg->perp.y, seg->strokeWidth, 31);

	if (!end) {	/* Starting endpoint */
		center = &seg->pt[0];
	}
	else {		/* Ending endpoint */
		v.x = -v.x;
		v.y = -v.y;
		center = &seg->pt[1];
	}

	p.x = center->x - v.x;
	p.y = center->y - v.y;
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(array, &p));

	p.x = center->x + v.x;
	p.y = center->y + v.y;
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(array, &p));

bail:
	return err;
}


/*******************************************************************************
 * RoundedJoin
 *******************************************************************************/

static FskErr
RoundedJoin(const LineSeg *seg0, const LineSeg *seg1, FskGrowableFixedPoint2DArray fwdPoly, FskGrowableFixedPoint2DArray revPoly)
{
	FskErr				err			= kFskErrNone;
	FskFract			sinAng, cosAng, snmHaf, sinHaf, cosHaf;
	FskFixed			mitLength, maxElbow;
	FskFractVector2D	u;
	FskFixedVector2D	v0, v1, e;
	FskFixedPoint2D		p[4];

	sinAng = FskFractCrossProduct2D(&seg0->par, &seg1->par);			/* Sine of change in trajectory */
	if (sinAng == 0)													/* Collinear */
		goto bail;														/* All done */
	cosAng = FskFractDotProduct(&seg0->par.x, &seg1->par.x, 2);			/* Cosine of change in trajectory */
	if      (cosAng >  FRACT_ONE)	cosAng =  FRACT_ONE;
	else if (cosAng < -FRACT_ONE)	cosAng = -FRACT_ONE;
	cosHaf =  FskFracSqrt(FRACT_HALF - (cosAng >> 1));					/* Cosine of half the subtended angle */
	snmHaf =  FskFracSqrt(FRACT_HALF + (cosAng >> 1));					/* Sine   of half the subtended angle */
	if (snmHaf != 0) {
		sinHaf = snmHaf;
		if (sinAng < 0)	sinHaf = -sinHaf;
		v0.x = FskFixedNMul(seg0->perp.x, seg0->strokeWidth, 31);		/* Perpendicular excursion from seg0 */
		v0.y = FskFixedNMul(seg0->perp.y, seg0->strokeWidth, 31);
		v1.x = FskFixedNMul(seg1->perp.x, seg0->strokeWidth, 31);		/* Perpendicular excursion from seg1 */
		v1.y = FskFixedNMul(seg1->perp.y, seg0->strokeWidth, 31);
		u.x = FskFixedNLinear2D(seg0->par.x, cosHaf,  seg0->par.y, sinHaf, 30);	/* Unit vector to elbow, inside or outside */
		u.y = FskFixedNLinear2D(seg0->par.y, cosHaf, -seg0->par.x, sinHaf, 30);
		mitLength = FskFixedNDiv(seg0->strokeWidth, snmHaf, 29);
		if ((maxElbow = (seg0->strokeWidth)) < seg0->length)			/* Limit the extent of the inside bevel joint */
			maxElbow = seg0->length;
		if (maxElbow < seg1->length)
			maxElbow = seg1->length;
		if (mitLength > maxElbow)
			mitLength = maxElbow;
		e.x = FskFracMul(mitLength, u.x);								/* Scaled vector to elbow, inside */
		e.y = FskFracMul(mitLength, u.y);

		if (sinAng > 0) {	/* Positive rotation */
			p[0].x = seg0->pt[1].x - v0.x;								/* Start of arc */
			p[0].y = seg0->pt[1].y - v0.y;
			p[2].x = seg0->pt[1].x - v1.x;								/* End of arc */
			p[2].y = seg0->pt[1].y - v1.y;
			p[1].x = seg0->pt[1].x +  e.x;								/* Outside elbow point */
			p[1].y = seg0->pt[1].y +  e.y;
			p[3].x = seg0->pt[1].x -  e.x;								/* Inside elbow point */
			p[3].y = seg0->pt[1].y -  e.y;
			BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p[0], &p[1], &snmHaf, 3, NULL, revPoly));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(fwdPoly, &p[3]));
		}
		else {				/* Negative rotation */
			p[0].x = seg0->pt[1].x + v0.x;								/* Start of arc */
			p[0].y = seg0->pt[1].y + v0.y;
			p[2].x = seg0->pt[1].x + v1.x;								/* End of arc */
			p[2].y = seg0->pt[1].y + v1.y;
			p[1].x = seg0->pt[1].x +  e.x;								/* Outside elbow point */
			p[1].y = seg0->pt[1].y +  e.y;
			p[3].x = seg0->pt[1].x -  e.x;								/* Inside elbow point */
			p[3].y = seg0->pt[1].y -  e.y;
			BAIL_IF_ERR(err = FskTessellateRationalBezier2DRegularly(&p[0], &p[1], &snmHaf, 3, NULL, fwdPoly));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(revPoly, &p[3]));
		}
	}
	/* else 0 degrees subtended (180 degree about face): what to do? */

bail:
	return err;
}


/*******************************************************************************
 * BeveledJoin
 *******************************************************************************/

static FskErr
BeveledJoin(const LineSeg *seg0, const LineSeg *seg1, FskGrowableFixedPoint2DArray fwdPoly, FskGrowableFixedPoint2DArray revPoly)
{
	FskErr				err			= kFskErrNone;
	FskFract			sinAng, cosAng, snmHaf, sinHaf, cosHaf;
	FskFixed			bevHafWidth, mitLength, maxElbow;
	FskFractVector2D	u;
	FskFixedVector2D	b, c, e;
	FskFixedPoint2D		p[3];

	sinAng = FskFractCrossProduct2D(&seg0->par, &seg1->par);		/* Sine of change in trajectory */
	if (sinAng == 0)												/* Collinear */
		goto bail;													/* All done */
	cosAng = FskFractDotProduct(&seg0->par.x, &seg1->par.x, 2);		/* Cosine of change in trajectory */
	if      (cosAng >  FRACT_ONE)	cosAng =  FRACT_ONE;
	else if (cosAng < -FRACT_ONE)	cosAng = -FRACT_ONE;
	cosHaf =  FskFracSqrt(FRACT_HALF - (cosAng >> 1));				/* Cosine of half the subtended angle */
	snmHaf =  FskFracSqrt(FRACT_HALF + (cosAng >> 1));				/* Sine   of half the subtended angle */
	if (snmHaf != 0) {
		sinHaf = snmHaf;
		if (sinAng < 0)	sinHaf = -sinHaf;
		u.x = FskFixedNLinear2D(seg0->par.x, cosHaf,  seg0->par.y, sinHaf, 30);	/* Unit vector to elbow, inside or outside */
		u.y = FskFixedNLinear2D(seg0->par.y, cosHaf, -seg0->par.x, sinHaf, 30);
		bevHafWidth = FskFixedNRatio(seg0->strokeWidth, cosHaf, FRACT_ONE + snmHaf, -1);
		c.x = FskFixedNMul(seg0->strokeWidth, u.x, 31);				/* Midpoint of bevel */
		c.y = FskFixedNMul(seg0->strokeWidth, u.y, 31);
		b.x = FskFracMul(bevHafWidth, -u.y);						/* Half-vector of bevel */
		b.y = FskFracMul(bevHafWidth,  u.x);
		mitLength = FskFixedNDiv(seg0->strokeWidth, snmHaf, 29);
		if ((maxElbow = (seg0->strokeWidth)) < seg0->length)		/* Limit the extent of the inside bevel joint */
			maxElbow = seg0->length;
		if (maxElbow < seg1->length)
			maxElbow = seg1->length;
		if (mitLength > maxElbow)
			mitLength = maxElbow;
		e.x = FskFracMul(mitLength, u.x);							/* Scaled vector to elbow, inside */
		e.y = FskFracMul(mitLength, u.y);

		if (sinAng > 0) {	/* Positive rotation */
			p[0].x = seg0->pt[1].x + c.x - b.x;						/* First bevel point */
			p[0].y = seg0->pt[1].y + c.y - b.y;
			p[1].x = seg0->pt[1].x + c.x + b.x;						/* Second bevel point */
			p[1].y = seg0->pt[1].y + c.y + b.y;
			p[2].x = seg0->pt[1].x - e.x;							/* Inside elbow */
			p[2].y = seg0->pt[1].y - e.y;
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItems(revPoly, &p[0], 2));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem( fwdPoly, &p[2]));
		}
		else {				/* Negative rotation */
			p[0].x = seg0->pt[1].x + c.x + b.x;						/* First bevel point */
			p[0].y = seg0->pt[1].y + c.y + b.y;
			p[1].x = seg0->pt[1].x + c.x - b.x;						/* Second bevel point */
			p[1].y = seg0->pt[1].y + c.y - b.y;
			p[2].x = seg0->pt[1].x - e.x;							/* Inside elbow */
			p[2].y = seg0->pt[1].y - e.y;
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItems(fwdPoly, &p[0], 2));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem( revPoly, &p[2]));
		}
	}
	/* else 0 degrees subtended (180 degree about face): what to do? */

bail:
	return err;
}


/*******************************************************************************
 * MiteredJoin
 *******************************************************************************/

static FskErr
MiteredJoin(const LineSeg *seg0, const LineSeg *seg1, FskGrowableFixedPoint2DArray fwdPoly, FskGrowableFixedPoint2DArray revPoly)
{
	FskErr				err			= kFskErrNone;
	FskFract			sinAng, cosAng, snmHaf, sinHaf, cosHaf;
	FskFixed			bevHafWidth, mitLength, maxElbow;
	FskFractVector2D	u;
	FskFixedVector2D	b, c, e;
	FskFixedPoint2D		p[3];

	sinAng = FskFractCrossProduct2D(&seg0->par, &seg1->par);		/* Sine of change in trajectory */
	if (sinAng == 0)												/* Collinear */
		goto bail;													/* All done */
	cosAng = FskFractDotProduct(&seg0->par.x, &seg1->par.x, 2);		/* Cosine of change in trajectory */
	if      (cosAng >  FRACT_ONE)	cosAng =  FRACT_ONE;
	else if (cosAng < -FRACT_ONE)	cosAng = -FRACT_ONE;
	cosHaf =  FskFracSqrt(FRACT_HALF - (cosAng >> 1));				/* Cosine of half the subtended angle */
	snmHaf =  FskFracSqrt(FRACT_HALF + (cosAng >> 1));				/* Sine   of half the subtended angle */
	if (snmHaf != 0) {
		sinHaf = snmHaf;
		if (sinAng < 0)	sinHaf = -sinHaf;
		u.x = FskFixedNLinear2D(seg0->par.x, cosHaf,  seg0->par.y, sinHaf, 30);	/* Unit vector to elbow, inside or outside */
		u.y = FskFixedNLinear2D(seg0->par.y, cosHaf, -seg0->par.x, sinHaf, 30);
		mitLength = FskFixedNDiv(seg0->strokeWidth, snmHaf, 29);
		if (snmHaf < seg0->sinMiterLimit) {							/* Do a bevel instead */
			if ((maxElbow = (seg0->strokeWidth)) < seg0->length)	/* Limit the extent of the inside bevel joint */
					maxElbow = seg0->length;
			if (maxElbow < seg1->length)
				maxElbow = seg1->length;
			if (mitLength > maxElbow)
				mitLength = maxElbow;
			e.x = FskFracMul(mitLength, u.x);						/* Scaled vector to elbow, inside */
			e.y = FskFracMul(mitLength, u.y);
			bevHafWidth = FskFixedNRatio(seg0->strokeWidth, cosHaf, (FRACT_ONE + snmHaf), -1);
			c.x = FskFixedNMul(seg0->strokeWidth, u.x, 31);			/* Midpoint of bevel */
			c.y = FskFixedNMul(seg0->strokeWidth, u.y, 31);
			b.x = FskFracMul(bevHafWidth, -u.y);					/* Half-vector of bevel */
			b.y = FskFracMul(bevHafWidth,  u.x);
			if (sinAng > 0) {	/* Positive rotation */
				p[0].x = seg0->pt[1].x + c.x - b.x;					/* First bevel point */
				p[0].y = seg0->pt[1].y + c.y - b.y;
				p[1].x = seg0->pt[1].x + c.x + b.x;					/* Second bevel point */
				p[1].y = seg0->pt[1].y + c.y + b.y;
				p[2].x = seg0->pt[1].x - e.x;						/* Inside elbow */
				p[2].y = seg0->pt[1].y - e.y;
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItems(revPoly, &p[0], 2));
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem( fwdPoly, &p[2]));
			}
			else {				/* Negative rotation */
				p[0].x = seg0->pt[1].x + c.x + b.x;					/* First bevel point */
				p[0].y = seg0->pt[1].y + c.y + b.y;
				p[1].x = seg0->pt[1].x + c.x - b.x;					/* Second bevel point */
				p[1].y = seg0->pt[1].y + c.y - b.y;
				p[2].x = seg0->pt[1].x - e.x;						/* Inside elbow */
				p[2].y = seg0->pt[1].y - e.y;
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItems(fwdPoly, &p[0], 2));
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem( revPoly, &p[2]));
			}
		}
		else {	/* Miter */
			e.x = FskFracMul(mitLength, u.x);						/* Scaled vector to elbow, inside */
			e.y = FskFracMul(mitLength, u.y);
			p[0].x = seg0->pt[1].x + e.x;
			p[0].y = seg0->pt[1].y + e.y;
			p[1].x = seg0->pt[1].x - e.x;
			p[1].y = seg0->pt[1].y - e.y;
			if (sinAng > 0) {	/* Positive rotation */
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(revPoly, &p[0]));
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(fwdPoly, &p[1]));
			}
			else {				/* Negative rotation */
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(fwdPoly, &p[0]));
				BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(revPoly, &p[1]));
			}
		}
	}
	/* else 0 degrees subtended (180 degree about face): what to do? */

bail:
	return err;
}


/*******************************************************************************
 * WidePolyLineToPolygon
 *
 * For rounded joints, use a jointSharpness of 0.
 * For bevelled joints, use a jointSharpness of fixed one = 0x10000
 * For mitered joints, use a jointSharpness greater than fixed one.
 *
 * If *polygon==NULL, a new FskGrowableFixedPoint2DArray will be allocated,
 * otherwise the old one will be reset and used instead.
 *******************************************************************************/

FskErr
FskWidePolyLineToPolygon(
	UInt32							nPts,
	const FskFixedPoint2D			*pts,
	FskFixed						strokeWidth,
	FskFixed						jointSharpness,
	UInt32							endCaps,
	const FskFixedMatrix3x2			*M,
	FskGrowableFixedPoint2DArray	*polygonHandle
)
{
	FskErr							err;
	FskGrowableFixedPoint2DArray	poly, back = NULL;
	SInt32							n;
	const FskFixedPoint2D			*p0, *p1;
	FskFixedPoint2D					pt;
	LineSeg							seg0, seg1;
	MakeEndCap						endCap;
	MakeJoin						join;

	BAIL_IF_TRUE((jointSharpness < 0), err, kFskErrInvalidParameter);
	BAIL_IF_NULL(polygonHandle, err, kFskErrInvalidParameter);

	/* Select the join proc */
	if (jointSharpness < FIXED_ONE) {			/* Rounded */
		join = RoundedJoin;
		n = nPts * 8;
	}
	else if (jointSharpness == FIXED_ONE) {		/* Beveled */
		join = BeveledJoin;
		n = nPts * 3;
	}
	else {										/* Mitered */
		join = MiteredJoin;
		n = nPts * 2;
		seg0.sinMiterLimit = seg1.sinMiterLimit = FskFixDiv(FRACT_ONE, jointSharpness);
	}

	/* Convert closed 2 point polylines into open ones with compatible endcaps */
	if ((nPts == 2) && ((endCaps & kFskLineEndCapClosed) != 0)) {
		endCaps = (jointSharpness < FIXED_ONE) ? kFskLineEndCapRound : kFskLineEndCapButt;
	}

	/* Make sure a polygon is allocated */
	if (*polygonHandle == NULL) {
		BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayNew(n, polygonHandle));
	}
	else {
		BAIL_IF_ERR(err = FskGrowableFixedPoint2DArraySetItemCount(*polygonHandle, 0));
	}
	poly = *polygonHandle;

	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayNew(n, &back));

	seg0.strokeWidth = seg1.strokeWidth = strokeWidth;

	/* Generate the polygon from the polyline description */
	switch (endCaps) {
		case kFskLineEndCapRound:
			endCap = RoundEndCap;
			seg0.arcRelTol = seg1.arcRelTol = FskFracSqrt(FskFixedNDiv(LINEAR_TOLERANCE, strokeWidth, 31));
			goto openLine;

		case kFskLineEndCapSquare:
			endCap = SquareEndCap;
			goto openLine;

		case kFskLineEndCapButt:
			endCap = ButtEndCap;

		openLine:
			p0 = pts;
			p1 = p0 + 1;
			for (n = nPts - 2; !InitLineSeg(p0, p1, M, &seg0) && (n > 0); p1++, n--) ;	/* Get the first nondegenerate segment */
			BAIL_IF_ERR(err = (*endCap)(&seg0, 0, poly));
			seg1 = seg0;	/* This should take care of the case with just 2 points */
			for (p0 = p1++; n-- > 0; p0 = p1++) {
				if (InitLineSeg(p0, p1, M, &seg1)) {
					BAIL_IF_ERR(err = (*join)(&seg0, &seg1, poly, back));
					seg0 = seg1;	/* Only shift nondegenerate segments */
				}
			}
			BAIL_IF_ERR(err = (*endCap)(&seg1, 1, poly));
			break;

		case kFskLineEndCapClosed:
//		case kFskLineEndCapClosed|kFskLineEndCapRound:	// round == 0
		case kFskLineEndCapClosed|kFskLineEndCapSquare:
		case kFskLineEndCapClosed|kFskLineEndCapButt:
			p0 = pts + nPts - 2;
			p1 = p0 + 1;
			for (n = nPts; !InitLineSeg(p0, p1, M, &seg0) && (n > 0); p0--, n--) ;	/* Nondegenerate segment coming in to the last point */
			for (p0 = p1, p1 = pts; n-- > 0; p0 = p1++) {
				if (InitLineSeg(p0, p1, M, &seg1)) {
					BAIL_IF_ERR(err = (*join)(&seg0, &seg1, poly, back));
					seg0 = seg1;	/* Only shift nondegenerate segments */
				}
			}
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetItem(poly, 0, &pt));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(poly, &pt));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayGetItem(back, 0, &pt));
			BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendItem(back, &pt));
			break;
	}

	n = FskGrowableFixedPoint2DArrayGetItemCount(back);
	FskGrowableFixedPoint2DArrayGetConstPointerToItem(back, 0, (const void**)(const void*)(&p1));
	BAIL_IF_ERR(err = FskGrowableFixedPoint2DArrayAppendReversedItems(poly, p1, n));

bail:
	if (back != NULL)	FskGrowableFixedPoint2DArrayDispose(back);
	return err;
}


