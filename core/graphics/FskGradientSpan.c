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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else // !PRAGMA_MARK_SUPPORTED
		#define PRAGMA_MARK_SUPPORTED 0
	#endif // !PRAGMA_MARK_SUPPORTED
#endif // PRAGMA_MARK_SUPPORTED


#include "FskGradientSpan.h"
#include "FskMemory.h"
#include "FskPixelOps.h"
#include "FskPolygon.h"

#include <limits.h>

#define UNUSED(x)	(void)(x)

#define kRampFillBits					24				/* The number of bits for the color forward differencing */
#define MIN_DELTA_OFFSET				1				/* The minimum difference between offsets to not be considered zero */

typedef FskFixed						FixedFwdDiff;	/* Fixed, with kRampFillBits fractional bits */
typedef SInt32							TOffset;
#define OFFSET_FRACBITS					24
#define OFFSET_ONE						(1 << OFFSET_FRACBITS)
#define FRACT_ONE						(1 << 30)


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Typedefs								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* Forward declarations */
struct GradientStop;			typedef struct GradientStop				GradientStop;
struct LinearGradientSpanData;	typedef struct LinearGradientSpanData	LinearGradientSpanData;
struct RadialGradientSpanData;	typedef struct RadialGradientSpanData	RadialGradientSpanData;

typedef void	(*SetPixelProc)(FskSpan *span);
typedef void	(*SetPixelFromRadiusProc)(FskSpan *span, TOffset r);
typedef TOffset	(*ComputeRadialDistanceProc)(const RadialGradientSpanData *gd);



/********************************************************************************
 *								GradientStop									*
 ********************************************************************************/

struct GradientStop {
	TOffset				offset;						/* Fractions, usually from 0 to 1, with OFFSET_FRACBITS fractional bits */
	FskColorRGBARecord	color;						/* r, g, b, a */
	FixedFwdDiff		drdx, dgdx, dbdx, dadx;		/* d(r,g,b,a)/dx between this stop and the next */
};



/********************************************************************************
 *							LinearGradientSpanData								*
 ********************************************************************************/

struct LinearGradientSpanData {				/* Data stored in FskSpan's spanData field */
	FskFillSpanProc		flatFill;			/* This method does a flat fill when needed for padding the boundaries */
	FskFillSpanProc		rampFill;			/* This method does a ramp fill when needed for padding the boundaries */

	FskFixedMatrix3x2	gradXform;			/* v(x,y) = g[0][0] * x + g[1][0] * y + g[2][0] */
	UInt32				spreadMethod;		/* kFskSpreadPad, kFskSpreadReflect, kFskSpreadRepeat */

	FixedFwdDiff		r,  g,  b,  a;		/* Ramp fill color (r,g,b,a) */
	FixedFwdDiff		dr, dg, db, da;		/* Ramp fill delta color: d(r,g,b,a)/dx */
	TOffset				v;					/* The gradient coordinate */
	UInt32				invertColorDeltas;	/* This is used when reflecting */

	UInt32				numStops;
	GradientStop		stops[1];			/* Actually stops[numStops] */
};


/********************************************************************************
 *							RadialGradientSpanData								*
 ********************************************************************************/

struct RadialGradientSpanData {
	FskFillSpanProc				flatFill;					/* This method does a flat fill when needed for padding the boundaries */
	SetPixelProc				setPixel;					/* This  sets  the pixel from the Fixed r, g, b below */
	SetPixelFromRadiusProc		setPixelFromRadius;
	ComputeRadialDistanceProc	computeRadialDistance;		/* Per pixel proc */

	FskFixedMatrix3x2			gradXform;					/* TOffset */
	FskFixed24					ks, k0, kx, kr0, krx, kry;	/* Coefficients for per-pixel computations. */
	TOffset						u, v;						/* The gradient coordinates */
	UInt8						red;						/* Pixel color */
	UInt8						green;
	UInt8						blue;
	UInt8						alpha;
	UInt32						spreadMethod;				/* kFskSpreadPad, kFskSpreadReflect, kFskSpreadRepeat */
	UInt32						numStops;
	GradientStop				stops[1];					/* Actually stops[numStops] */
};



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Utilities								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * ApplyGradientTransformations
 *	This computes L^-1 * R -> P -> R
 *	It does NOT work in-place.
 ********************************************************************************/

static void
ApplyGradientTransformations(const FskFixedMatrix3x2 *P, const FskFixedMatrix3x2 *G, FskFixedMatrix3x2 *X)
{
	FskFixedMatrix3x2 H;

	if (P != NULL) {	if (G != NULL)	{	FskFixedMultiplyMatrix3x2(G, P, &H);	G = &H;		}	/* Concatenate G and P matrices */
						else			{	G = P;												}	/* Set G to P */
	}
	else				if (G == NULL)	{	return;												}	/* No transformation */
	FskFixedMultiplyInverseMatrix3x2(G, X, X);														/* Apply gradient matrix to gradient transform */
}

/********************************************************************************
 * CopyGradientStops
 ********************************************************************************/

static void CopyGradientStops(unsigned numStops, const FskGradientStop *src, GradientStop *dst, FskConstBitmap dstBM) {
	for (; numStops--; src++, dst++) {
		dst->offset	= (src->offset + (1 << (30-OFFSET_FRACBITS-1))) >> (30 - OFFSET_FRACBITS);								/* Round from 2.30 to 4.28 */
		dst->color = src->color;
	}
}



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Gouraud Span Fill Procs						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

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

#include "FskBlitDispatchDef.h"


/********************************************************************************
 * Span proc generation
 ********************************************************************************/
#define BLIT_PROTO_FILE			"FskGradientSpanProto.c"
#include "FskBlitDst.h"
#undef BLIT_PROTO_FILE


/* NULLify YUV420 */
#define RampFillSpanYUV420			NULL
#define AlphaRampFillSpanYUV420		NULL
#define SetSpanPixelYUV420			NULL
#define BlendSpanPixelYUV420		NULL



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							FskLinearGradientSpan						*****
 *****																		*****
 ***** Intended Usage:														*****
 *****		FskNewLinearGradientSpanData(span, numStops);					*****
 *****		for (i = 0; i < numStops; i++)									*****
 *****			FskSetLinearGradientStop(i, offset[i], &color[i]);			*****
 *****		FskInitLinearGradientSpan(span, dstBM, edgeBytes, spreadMethod,	*****
 *****			gradVec, pathMtx, gradMtx									*****
 *****		);																*****
 *****		MyScanConvert(numEdges, edges, span);							*****
 *****		FskDisposeRadialGradientSpanData(span);							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * DisposeLinearGradientSpan					span->disposeSpanData() method
 ********************************************************************************/

static void
DisposeLinearGradientSpanData(void *spanData)
{
	FskMemPtrDispose(spanData);
}


/********************************************************************************
 * SetLinearGradientSpan:									span->set() method.
 ********************************************************************************/

static void
SetLinearGradientSpan(const FskEdge *L, const FskEdge *R, SInt32 x, SInt32 y, FskSpan *span)
{
	LinearGradientSpanData	*gd	= (LinearGradientSpanData*)(span->spanData);

	UNUSED(L);
	UNUSED(R);

	/* Convert into the gradient coordinate system */
	gd->v = gd->gradXform.M[2][0] + gd->gradXform.M[0][0] * x + gd->gradXform.M[1][0] * y;
}


/********************************************************************************
 * SetLinGradFromFskColor
 ********************************************************************************/

#define SetLinGradFromFskColor(lg, fc)	(lg)->r = ((fc)->color.r << (kRampFillBits - 8)) + (1 << (kRampFillBits - 9));	\
										(lg)->g = ((fc)->color.g << (kRampFillBits - 8)) + (1 << (kRampFillBits - 9));	\
										(lg)->b = ((fc)->color.b << (kRampFillBits - 8)) + (1 << (kRampFillBits - 9));	\
										(lg)->a = ((fc)->color.a << (kRampFillBits - 8)) + (1 << (kRampFillBits - 9))


/********************************************************************************
 * AugmentLinGradFromFskColor
 ********************************************************************************/

#define AugmentLinGradFromFskColor(lg, f, fc)												\
		(lg)->r += FskFixedNMul((fc)[1].color.r - (fc)[0].color.r, f, 8+30-kRampFillBits);	\
		(lg)->g += FskFixedNMul((fc)[1].color.g - (fc)[0].color.g, f, 8+30-kRampFillBits);	\
		(lg)->b += FskFixedNMul((fc)[1].color.b - (fc)[0].color.b, f, 8+30-kRampFillBits);	\
		(lg)->a += FskFixedNMul((fc)[1].color.a - (fc)[0].color.a, f, 8+30-kRampFillBits)


/********************************************************************************
 * ComputeSubSpanLength
 ********************************************************************************/

static SInt32
ComputeSubSpanLength(TOffset v1, TOffset v0, TOffset dv)
{
	SInt32	n;

	if (dv > 0)	n = ((v1 - v0) / dv + 1);
	else		n = kFskSInt32Max;
	return n;
}


/********************************************************************************
 * FillRampSpan
 ********************************************************************************/

static void
FillRampSpan(FskSpan *span, GradientStop *st, SInt32 *dx)
{
	FskFract	f;
	LinearGradientSpanData	*gd	= (LinearGradientSpanData*)(span->spanData);

	/* Compute the subspan width until the next stop */
	if      (gd->gradXform.M[0][0] > 0)	span->dx = ComputeSubSpanLength(st[1].offset, gd->v,  gd->gradXform.M[0][0]);	/* Gradient increases in X */
	else if (gd->gradXform.M[0][0] < 0)	span->dx = ComputeSubSpanLength(gd->v, st[0].offset, -gd->gradXform.M[0][0]);	/* Gradient decreases in X */
	else								span->dx = *dx;																	/* Gradient stays constant in X */
	if ((*dx -= span->dx) < 0) {							/* Update the span length by this subspan length */
		span->dx += *dx;									/* Oops: subspan longer than span: set subspan width equal to remaining span width ... */
		*dx = 0;											/* ... and zero the width of the remaining span */
	}

	/* Set the color for the first pixel in the span: we cannot use d(r,g,b)/dv for this because the ranges are too wild */
	SetLinGradFromFskColor(gd, &st[0]);						/* Color of the left stop */
	if ((f = gd->v - st[0].offset) > 0) {					/* Fraction is non-zero */
		f = FskFracDiv(f, st[1].offset - st[0].offset);		/* Fraction of the way from left to right */
		AugmentLinGradFromFskColor(gd, f, st);				/* Interpolate between left and right colors */
	}

	/* Set the increment between pixels */
	if (gd->invertColorDeltas == 0) {
		gd->dr = st[0].drdx;								/* Copy color deltas for this subspan */
		gd->dg = st[0].dgdx;								/* Color deltas are constant for each subspan ... */
		gd->db = st[0].dbdx;								/* ... so we only need to copy, not compute */
		gd->da = st[0].dadx;
	}
	else {
		gd->dr = -st[0].drdx;								/* Negate color deltas for this subspan */
		gd->dg = -st[0].dgdx;								/* Color deltas are constant for each subspan ... */
		gd->db = -st[0].dbdx;								/* ... so we only need to negate, not compute */
		gd->da = -st[0].dadx;
	}

	gd->v += span->dx * gd->gradXform.M[0][0];				/* Update the value of the gradient parameter v for the next subspan */

	gd->rampFill(span);										/* Call the ramp fill proc */
}


/********************************************************************************
 * FillConstantSpan
 ********************************************************************************/

static void
FillConstantSpan(FskSpan *span, GradientStop *st, SInt32 dx)
{
	LinearGradientSpanData	*gd	= (LinearGradientSpanData*)(span->spanData);

	SetLinGradFromFskColor(gd, &st[0]);						/* Color of the left stop */
	// TODO I think that we need to interpolate
	gd->dr = gd->dg = gd->db =gd->da = 0;					/* Constant, so increment is zero */
	span->dx = dx;											/* We're gobbling up the whole span */
	gd->v += span->dx * gd->gradXform.M[0][0];				/* Update the value of the gradient parameter v for the next subspan */
	gd->rampFill(span);										/* Call the ramp fill proc. TODO N.B. maybe use an optimized flat fill instead? */
}


/********************************************************************************
 * FillLinearGradientSpanPeriod
 * The dx here is guaranteed to be positive and remain within the limits of the first and last stop.
 ********************************************************************************/

static void
FillLinearGradientSpanPeriod(FskSpan *span)
{
	LinearGradientSpanData	*gd	= (LinearGradientSpanData*)(span->spanData);
	SInt32					dx, numStops;
	GradientStop			*st;

	dx = span->dx;

	/* Find the starting interval */
	for (numStops = gd->numStops - 1, st = gd->stops; numStops--; st++)
		if ((st[0].offset <= gd->v) && (gd->v < st[1].offset))
			break;

	if   (gd->gradXform.M[0][0] >= 0)	{ for ( ; dx > 0; st++)	FillRampSpan(span, st, &dx); }	/* Fill forward subspans,  increasing gradient coordinate */
	else								{ for ( ; dx > 0; st--)	FillRampSpan(span, st, &dx); }	/* Fill backward subspans, decreasing gradient coordinate */
}


/********************************************************************************
 * FillLinearGradientSpan									span->fill method
 ********************************************************************************/

static void
FillLinearGradientSpan(FskSpan *span)
{
	LinearGradientSpanData	*gd			= (LinearGradientSpanData*)(span->spanData);
	SInt32					numStops	= gd->numStops;
	SInt32					dx;
	GradientStop			*st;
	TOffset					period, phase0, phase;
	TOffset					gradXform0;

	if ((dx = span->dx) == 0)
		return;

	switch (gd->spreadMethod) {
		case kFskSpreadPad:
			while (dx > 0) {													/* This should execute no more than 2 times */
				/* Do flat fill on the left of the boundary */
				st = &gd->stops[0];
				if (gd->v < st->offset) {										/* In the "left" regime */
					if (gd->gradXform.M[0][0] <= 0) {							/* Stuck in the "left" regime */
						FillConstantSpan(span, st, dx);							/* Fill the entire span with a constant color */
						dx = 0;													/* Leaving nothing for anyone else */
					}
					else {														/* Might come out of the "left" regime */
						span->dx = ComputeSubSpanLength(st[0].offset, gd->v, gd->gradXform.M[0][0]);
						if ((dx -= span->dx) < 0) {	span->dx += dx; dx = 0;	}	/* Adjust the remaining span length: Won't come out of the "left" regime in time */
						FillConstantSpan(span, st, span->dx);					/* Fill the leading subspan with a constant color */
					}
					if (dx <= 0)
						return;
				}

				/* Do flat fill on the right */
				st = &gd->stops[gd->numStops-1];
				if (gd->v > st->offset) {										/* In the "right" regime */
					if (gd->gradXform.M[0][0] >= 0) {							/* Stuck in the "right" regime */
						FillConstantSpan(span, st, dx);							/* Fill the entire span with a constant color */
						dx = 0;													/* Leaving nothing for anyone else */
					}
					else {														/* Might come out of the "right" regime" */
						span->dx = ComputeSubSpanLength(gd->v, st[0].offset, -gd->gradXform.M[0][0]);
						if ((dx -= span->dx) < 0) {	span->dx += dx; dx = 0;	}	/* Adjust the remaining span length: Won't come out of the "right" regime in time */
						FillConstantSpan(span, st, span->dx);					/* Fill the trailing subspan with a constant color */
					}
					if (dx <= 0)
						return;
				}

				/* Determine ramp length in the middle */
				if      (gd->gradXform.M[0][0] > 0)	span->dx = ComputeSubSpanLength(gd->stops[numStops-1].offset, gd->v,  gd->gradXform.M[0][0]);	/* Gradient increases in X */
				else if (gd->gradXform.M[0][0] < 0)	span->dx = ComputeSubSpanLength(gd->v,          gd->stops[0].offset, -gd->gradXform.M[0][0]);	/* Gradient decreases in X */
				else								span->dx = dx;																					/* Gradient stays constant in X */
				if ((dx -= span->dx) < 0) {	span->dx += dx; dx = 0;	}			/* Adjust the remaining span length */
				FillLinearGradientSpanPeriod(span);
			}
			break;

		case kFskSpreadRepeat:
			/* Get canonical phase */
			phase0 = gd->stops[0].offset;										/* The beginning of the period */
			period = gd->stops[numStops-1].offset - phase0;						/* The period length */
			phase  = gd->v                        - phase0;						/* The initial phase, relative to phase0 ... */
			phase  %= period;													/* ... demodulated to +/- period ... */
			if (phase < 0) phase += period;										/* ... and made positive */
			gd->v = phase + phase0;												/* Reinsert the phase offset reinserted to yield the Canonical Phase */
			if (gd->gradXform.M[0][0] < 0) period = -period;					/* Transfer sign to period */

			while (dx > 0) {
				if      (gd->gradXform.M[0][0] > 0)	span->dx = ComputeSubSpanLength(gd->stops[numStops-1].offset, gd->v,  gd->gradXform.M[0][0]);	/* Gradient increases in X */
				else if (gd->gradXform.M[0][0] < 0)	span->dx = ComputeSubSpanLength(gd->v,          gd->stops[0].offset, -gd->gradXform.M[0][0]);	/* Gradient decreases in X */
				else								span->dx = dx;																					/* Gradient stays constant in X */
				if ((dx -= span->dx) < 0) {	span->dx += dx; dx = 0;	}			/* Adjust the remaining span length */
				FillLinearGradientSpanPeriod(span);
				gd->v -= period;												/* Remove offset to restore canonical phase */
			}
			break;

		case kFskSpreadReflect:
			/* Get canonical phase */
			phase0 = gd->stops[0].offset;										/* The beginning of the period */
			period = (gd->stops[numStops-1].offset - phase0) << 1;				/* The compound period length */
			phase  = gd->v                        - phase0;						/* The initial phase, relative to phase0 ... */
			phase  %= period;													/* ... demodulated to +/- compound period ... */
			if (phase < 0)	phase += period;									/* ... and made positive */
			period >>= 1;														/* The simple period */
			gradXform0 = gd->gradXform.M[0][0];									/* Save gradXform.M[0][0] */
			gd->invertColorDeltas = 0;											/* Assume forward deltas */
			if (phase >= period) {												/* If outside of the first half of the canonical phase... */
				phase = (period << 1) - phase;									/* ... reverse it ... */
				gd->gradXform.M[0][0] = -gd->gradXform.M[0][0];					/* ... and its increment */
				gd->invertColorDeltas = 1;
			}
			gd->v = phase + phase0;												/* Reinsert the phase offset reinserted to yield the Canonical Phase */

			while (dx > 0) {
				if      (gd->gradXform.M[0][0] > 0)	span->dx = ComputeSubSpanLength(gd->stops[numStops-1].offset, gd->v,  gd->gradXform.M[0][0]);
				else if (gd->gradXform.M[0][0] < 0)	span->dx = ComputeSubSpanLength(gd->v,          gd->stops[0].offset, -gd->gradXform.M[0][0]);
				else								span->dx = dx;
				if ((dx -= span->dx) < 0) {	span->dx += dx; dx = 0;	}			/* Adjust the remaining span length */
				FillLinearGradientSpanPeriod(span);
				if (dx <= 0)
					break;
				gd->gradXform.M[0][0] = -gd->gradXform.M[0][0];					/* Reverse direction */
				gd->invertColorDeltas = !gd->invertColorDeltas;					/* Reverse direction of gradient */
				phase = (phase0 - gd->v) % period;								/* Reflect phase */
				if (phase < 0)	phase += period;
				gd->v = phase + phase0;											/* Reflect to restore canonical phase */
			}

			gd->gradXform.M[0][0] = gradXform0;									/* Restore gradXform.M[0][0] */
			break;
	}
}



/********************************************************************************
 * Dispatch tables
 ********************************************************************************/

#define P_D(d)	FskName2(RampFillSpan,d)
static FskFillSpanProc rampDispatch[NUM_DST_FORMATS] = {
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
#undef P_D

#define P_D(d)	FskName2(AlphaRampFillSpan,d)
static FskFillSpanProc alphaRampDispatch[NUM_DST_FORMATS] = {
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
#undef P_D


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
 * GetRampFillProc
 ********************************************************************************/

static FskFillSpanProc
GetRampFillProc(UInt32 pixelFormat)
{
	FskFillSpanProc	proc	= NULL;
	int				index;

	if ((index = PixelFormatToProcTableDstIndex(pixelFormat)) >= 0)
		proc = rampDispatch[index];

	return proc;
}


/********************************************************************************
 * GetAlphaRampFillProc
 ********************************************************************************/

static FskFillSpanProc
GetAlphaRampFillProc(UInt32 pixelFormat)
{
	FskFillSpanProc	proc	= NULL;
	int				index;

	if ((index = PixelFormatToProcTableDstIndex(pixelFormat)) >= 0)
		proc = alphaRampDispatch[index];

	return proc;
}


/********************************************************************************
 * GradientNeedsBlending
 ********************************************************************************/

static int
GradientNeedsBlending(UInt32 n, FskGradientStop *s)
{
	for ( ; n--; s++)
		if (s->color.a != 255)
			return 1;
	return 0;
}


/********************************************************************************
 * FskInitLinearGradientSpan								InitSpan() method
 * The stops must be set before calling this.
 ********************************************************************************/

FskErr
FskInitLinearGradientSpan(
	FskSpan					*span,
	FskBitmap				dstBM,
	const FskFixedMatrix3x2	*pathMatrix,		/* Can be NULL */
	UInt32					quality,
	const struct FskColorSource *cs
)
{
	FskErr					err;
	LinearGradientSpanData	*gd;
	const FskColorSourceLinearGradient	*lin	= &((const FskColorSourceUnion*)cs)->lg;


	UNUSED(quality);

	/*
	 * Allocate linear gradient data structure and copy initialization parameters.
	 */
	span->spanData		= NULL;
	FskInitSolidColorSpan(span, dstBM, pathMatrix, 0, cs);								/* Get the flat fill proc ... */
	BAIL_IF_FALSE((lin->numStops > 1), err, kFskErrBadData);							/* .. and defer to that if there is only one stop */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(LinearGradientSpanData) + (lin->numStops - 1) * sizeof(GradientStop), (FskMemPtr*)(void*)(&span->spanData)));	/* allocate */
	gd					= (LinearGradientSpanData*)(span->spanData);					/* Get a local pointer to the span auxiliary data structure */
	gd->flatFill		= span->fill;													/* Save the flat fill proc */
	gd->numStops		= lin->numStops;												/* Record the number of stops */
	gd->spreadMethod	= (lin->spreadMethod >> kFskSpreadPosX) & kFskSpreadMask;		/* Extract the X spread method ... */
	gd->spreadMethod	= (gd->spreadMethod << kFskSpreadPosX) | (gd->spreadMethod << kFskSpreadPosY);	/* ... and duplicate it to Y, since we only do 1D */
	span->fill			= FillLinearGradientSpan;
	span->set			= SetLinearGradientSpan;										/* Initialize the gradient coordinate for the beginning of the span on each scanline */
	span->initEdge		= NULL;
	span->advanceEdge	= NULL;
	span->disposeSpanData	= DisposeLinearGradientSpanData;

	if (!GradientNeedsBlending(lin->numStops, lin->gradientStops))	gd->rampFill = GetRampFillProc(     dstBM->pixelFormat);
	else															gd->rampFill = GetAlphaRampFillProc(dstBM->pixelFormat);
	BAIL_IF_NULL(gd->rampFill, err, kFskErrUnsupportedPixelType);


	/*
	 * Initialize linear gradient coordinate system
	 */
	{	FskFract	d[2];
		FskFixed	r;

		d[0] = lin->gradientVector[1].x - lin->gradientVector[0].x;																												/* lin->gradientFracBits fractional bits */
		d[1] = lin->gradientVector[1].y - lin->gradientVector[0].y;																												/* lin->gradientFracBits fractional bits */
		r = FskFixedVectorNormalize(d, 2);																																		/* r=lin->gradientFracBits, d=2.30 */
		gd->gradXform.M[1][1] =  (gd->gradXform.M[0][0] = FskFixedNDiv(d[0], r, OFFSET_FRACBITS-30+lin->gradientFracBits));														/* OFFSET_FRACBITS fractional bits */
		gd->gradXform.M[0][1] = -(gd->gradXform.M[1][0] = FskFixedNDiv(d[1], r, OFFSET_FRACBITS-30+lin->gradientFracBits));														/* OFFSET_FRACBITS fractional bits */
		gd->gradXform.M[2][0] = FskFixedNLinear2D(-lin->gradientVector[0].x, gd->gradXform.M[0][0], -lin->gradientVector[0].y, gd->gradXform.M[1][0], lin->gradientFracBits);	/* OFFSET_FRACBITS fractional bits */
		gd->gradXform.M[2][1] = FskFixedNLinear2D( lin->gradientVector[0].x, gd->gradXform.M[1][0], -lin->gradientVector[1].y, gd->gradXform.M[0][0], lin->gradientFracBits);	/* OFFSET_FRACBITS fractional bits */
	}
	ApplyGradientTransformations(pathMatrix, lin->gradientMatrix, &gd->gradXform);


	/*
	 * Init color deltas
	 */
	{
		UInt32					i;
		FskFract				idx;
		TOffset					dv;
		GradientStop			*dst;

		/* Copy gradient stops over */
		CopyGradientStops(gd->numStops, lin->gradientStops, gd->stops, dstBM);

		/* Compute color deltas */
		for (i = gd->numStops - 1, dst = gd->stops; i--; dst++) {
			if ((dv = dst[1].offset - dst[0].offset) != 0) {
				idx = FskFracDiv(gd->gradXform.M[0][0], dv);																	/* dv/dx * 1/dv --> 1/dx (4.28 / 4.28 --> 2.30 */
				dst[0].drdx = FskFixedNMul(((SInt32)(dst[1].color.r) - (SInt32)(dst[0].color.r)), idx, 30+8-kRampFillBits);		/* Red   d/dx as .kRampFillBits (8.24)	*/
				dst[0].dgdx = FskFixedNMul(((SInt32)(dst[1].color.g) - (SInt32)(dst[0].color.g)), idx, 30+8-kRampFillBits);		/* Green d/dx as .kRampFillBits (8.24)	*/
				dst[0].dbdx = FskFixedNMul(((SInt32)(dst[1].color.b) - (SInt32)(dst[0].color.b)), idx, 30+8-kRampFillBits);		/* Blue  d/dx as .kRampFillBits (8.24)	*/
				dst[0].dadx = FskFixedNMul(((SInt32)(dst[1].color.a) - (SInt32)(dst[0].color.a)), idx, 30+8-kRampFillBits);		/* Alpha d/dx as .kRampFillBits (8.24)	*/
			}
			else {
				dst[0].drdx = dst[0].dgdx = dst[0].dbdx = 0;
			}
		}
		dst[0].drdx = dst[0].dgdx = dst[0].dbdx = 0;
	}


bail:
	if (err != kFskErrNone) {
		if (span->spanData != NULL) {
			DisposeLinearGradientSpanData(span->spanData);
			span->spanData = NULL;
		}
	}

	return err;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							RadialGradientSpan							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * DisposeRadialGradientSpanData				span->disposeSpanData() method
 ********************************************************************************/

static void
DisposeRadialGradientSpanData(void *spanData)
{
	FskMemPtrDispose(spanData);
}


/********************************************************************************
 * SetRadialGradientSpan									span->set() method
 ********************************************************************************/

static void
SetRadialGradientSpan(const FskEdge *L, const FskEdge *R, SInt32 x, SInt32 y, FskSpan *span)
{
	RadialGradientSpanData	*gd	= (RadialGradientSpanData*)(span->spanData);

	UNUSED(L);
	UNUSED(R);

	gd->u = gd->gradXform.M[0][0] * x + gd->gradXform.M[1][0] * y + gd->gradXform.M[2][0];
	gd->v = gd->gradXform.M[0][1] * x + gd->gradXform.M[1][1] * y + gd->gradXform.M[2][1];
}


/********************************************************************************
 * SetSpanPixelFromRadius
 ********************************************************************************/

static void
SetSpanPixelFromRadius(FskSpan *span, TOffset r)
{
	RadialGradientSpanData	*gd	= (RadialGradientSpanData*)(span->spanData);
	UInt32					i;
	GradientStop			*st;
	FskFract				t;

	for (i = gd->numStops, st = gd->stops; i--; st++)
		if (r <= st[1].offset)
			break;
	t = FskFracDiv(r - st[0].offset, st[1].offset - st[0].offset);
	gd->red   = (UInt8)(st[0].color.r + FskFracMul((int)(st[1].color.r) - (int)(st[0].color.r), t));
	gd->green = (UInt8)(st[0].color.g + FskFracMul((int)(st[1].color.g) - (int)(st[0].color.g), t));
	gd->blue  = (UInt8)(st[0].color.b + FskFracMul((int)(st[1].color.b) - (int)(st[0].color.b), t));
	gd->setPixel(span);
}



/********************************************************************************
 * BlendSpanPixelFromRadius
 ********************************************************************************/

static void
BlendSpanPixelFromRadius(FskSpan *span, TOffset r)
{
	RadialGradientSpanData	*gd	= (RadialGradientSpanData*)(span->spanData);
	UInt32					i;
	GradientStop			*st;
	FskFract				t;

	for (i = gd->numStops, st = gd->stops; i--; st++)
		if (r <= st[1].offset)
			break;
	t = FskFracDiv(r - st[0].offset, st[1].offset - st[0].offset);
	gd->red   = (UInt8)(st[0].color.r + FskFracMul((int)(st[1].color.r) - (int)(st[0].color.r), t));
	gd->green = (UInt8)(st[0].color.g + FskFracMul((int)(st[1].color.g) - (int)(st[0].color.g), t));
	gd->blue  = (UInt8)(st[0].color.b + FskFracMul((int)(st[1].color.b) - (int)(st[0].color.b), t));
	gd->alpha = (UInt8)(st[0].color.a + FskFracMul((int)(st[1].color.a) - (int)(st[0].color.a), t));
	gd->setPixel(span);
}


/********************************************************************************
 * ComputeCanvasRadialDistanceIsotropic
 * Compute the radial distance when the gradient is isotropic.
 * t = k0 + sqrt(u^2 + v^2)
 ********************************************************************************/

static TOffset
ComputeCanvasRadialDistanceIsotropic(register const RadialGradientSpanData *gd)
{
	TOffset r;
	r  = FskFixedVectorNorm(&gd->u, 2);
	r += gd->k0;
	return r;
}


/********************************************************************************
 * ComputeCanvasRadialDistanceTangent
 * Compute the radial distance when the two circles are tangent.
 * t = k0 + u + v^2 / (u + kr0)
 ********************************************************************************/

static TOffset
ComputeCanvasRadialDistanceTangent(register const RadialGradientSpanData *gd)
{
	TOffset r;
	if ((r = gd->u + gd->kr0) >= 0) {
		r = FskFixedRatio(gd->v, gd->v, r);
		if (r != (TOffset)0x7FFFFFFF && r != (TOffset)0x80000000)
			r += gd->u + gd->k0;
	}
	else {
		r = 0x7FFFFFFF;
	}
	return r;
}


/********************************************************************************
 * ComputeCanvasRadialDistanceSkewCone
 * Compute the radial distance when rendering a nonsingular skew cone.
 * t = k0 + kx * u - sqrt(u^2 + v^2)
 ********************************************************************************/

static TOffset
ComputeCanvasRadialDistanceSkewCone(register const RadialGradientSpanData *gd)
{
	TOffset r = gd->k0 + FskFixedNMul(gd->kx, gd->u, OFFSET_FRACBITS) + FskFixedVectorNorm(&gd->u, 2);
	return r;
}


/********************************************************************************
 * ComputeCanvasRadialDistanceCircleSlide
 * Compute the radial distance when the two circles are the same size.
 * t = u - sqrt(kr0^2 - v^2)
 ********************************************************************************/

static TOffset
ComputeCanvasRadialDistanceCircleSlide(register const RadialGradientSpanData *gd)
{
	TOffset r, s;
	r = gd->kr0 - gd->v;
	s = gd->kr0 + gd->v;
	if ((r ^ s) >= 0) {								/* We only want the positive k^2-v^2 = (k-v)*(k+v) */
		r = FskFixedSqrt64to32((FskInt64)r * s);
		if ((r - gd->u) >= OFFSET_ONE)				/* This clips to the outer circle. */
			return -(gd->u + r);
	}
	return 0x7FFFFFFF;
}


/********************************************************************************
 * ComputeCanvasRadialCircleShrink
 * Compute the radial distance when the circles change sizes.
 * t = k0 + kx * u + sqrt(u^2 - v^2)
 ********************************************************************************/

static TOffset
ComputeCanvasRadialDistanceCircleShrink(register const RadialGradientSpanData *gd)
{
	TOffset r, s;

	r = gd->u - gd->v;
	s = gd->u + gd->v;
	if ((r | s) >= 0) {	/* We only want the positive u^2-v^2 = (u-v)*(u+v) */
		r = FskFixedSqrt64to32((FskInt64)r * s);
		s = gd->k0 + FskFixedNMul(gd->kx, gd->u, OFFSET_FRACBITS);
		if ((r + s) >= OFFSET_ONE)
			return s - r;
	}
	return 0x7FFFFFFF;
}


/********************************************************************************
 * FillRadialGradientSpan									span->fill() method
 ********************************************************************************/

static void
FillRadialGradientSpan(FskSpan *span)
{
	RadialGradientSpanData	*gd	= (RadialGradientSpanData*)(span->spanData);
	SInt32					dx	= span->dx;
	TOffset					r;
	TOffset					phase0, phase1, period;

	phase0 = gd->stops[0].offset;
	phase1 = gd->stops[gd->numStops-1].offset;
	period = phase1 - phase0;

	switch (gd->spreadMethod & kFskSpreadMask) {
		case kFskSpreadPadX:
			for ( ; dx--; gd->u += gd->gradXform.M[0][0], gd->v += gd->gradXform.M[0][1]) {
				r = gd->computeRadialDistance(gd);
				if (r <= phase0) {
					gd->red   = gd->stops[0].color.r;
					gd->green = gd->stops[0].color.g;
					gd->blue  = gd->stops[0].color.b;
					gd->alpha = gd->stops[0].color.a;
					gd->setPixel(span);
				}
				else if (r >= phase1) {
					gd->red   = gd->stops[gd->numStops-1].color.r;
					gd->green = gd->stops[gd->numStops-1].color.g;
					gd->blue  = gd->stops[gd->numStops-1].color.b;
					gd->alpha = gd->stops[gd->numStops-1].color.a;
					gd->setPixel(span);
				}
				else {
					gd->setPixelFromRadius(span, r);
				}
			}
			break;
		case kFskSpreadReflectX:
			for ( ; dx--; gd->u += gd->gradXform.M[0][0], gd->v += gd->gradXform.M[0][1]) {
				r = gd->computeRadialDistance(gd);
				r = (r - phase0) % (period << 1);
				if (r < 0)			r += (period << 1);
				if (r > period)		r = (period << 1) - r;
				gd->setPixelFromRadius(span, r);
			}
			break;
		case kFskSpreadRepeatX:
			for ( ; dx--; gd->u += gd->gradXform.M[0][0], gd->v += gd->gradXform.M[0][1]) {
				r = gd->computeRadialDistance(gd);
				r = (r - phase0) % period;
				if (r < 0)
					r += period;
				r += phase0;
				gd->setPixelFromRadius(span, r);
			}
			break;
	}
}

#define P_D(d)	FskName2(SetSpanPixel,d)
static FskFillSpanProc setPixelDispatch[NUM_DST_FORMATS] = {
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
#undef P_D

#define P_D(d)	FskName2(BlendSpanPixel,d)
static FskFillSpanProc blendPixelDispatch[NUM_DST_FORMATS] = {
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
#undef P_D


/********************************************************************************
 * GetSetPixelProc
 ********************************************************************************/

static SetPixelProc
GetSetPixelProc(UInt32 pixelFormat)
{
	FskFillSpanProc	proc	= NULL;
	int				index;

	if ((index = PixelFormatToProcTableDstIndex(pixelFormat)) >= 0)
		proc = setPixelDispatch[index];

	return proc;
}


/********************************************************************************
 * GetBlendPixelProc
 ********************************************************************************/

static SetPixelProc
GetBlendPixelProc(UInt32 pixelFormat)
{
	FskFillSpanProc	proc	= NULL;
	int				index;

	if ((index = PixelFormatToProcTableDstIndex(pixelFormat)) >= 0)
		proc = blendPixelDispatch[index];

	return proc;
}


/********************************************************************************
 * FiniteMatrix
 ********************************************************************************/

static Boolean
FiniteMatrix(FskFixedMatrix3x2 *M) {
	FskFixed *p;
	int i;
	for (i = 6, p = M->M[0]; i--; ++p)
		if (*p == (FskFixed)0x7FFFFFFF || *p == (FskFixed)0x80000000)
			return false;
	return true;
}


/********************************************************************************
 * NotFiniteFixed: not 0x80000000, 0x80000001, or 0x7FFFFFFF
 ********************************************************************************/

static Boolean
NotFiniteFixed(FskFixed x) {
	return (-2147483647 < x && x < 2147483647) ? false : true;
}


/********************************************************************************
 * FskInitRadialGradientSpan								InitSpan() method()
 ********************************************************************************/

FskErr
FskInitRadialGradientSpan(
	FskSpan					*span,
	FskBitmap				dstBM,
	const FskFixedMatrix3x2	*pathMatrix,		/* Can be NULL */
	UInt32					quality,
	const struct FskColorSource *cs
)
{
	const FskColorSourceRadialGradient	*rad	= &((const FskColorSourceUnion*)cs)->rg;
	FskErr								err;
	RadialGradientSpanData				*gd;
	FskFractVector2D					sku;
	FskFixed							skum, dr;
	FskFixedPoint2D						transfoc;
	UNUSED(quality);


	/*
	 * Allocate radial gradient data structure and copy initialization parameters.
	 */
	span->spanData		= NULL;
	FskInitSolidColorSpan(span, dstBM, pathMatrix, 0, cs);								/* Get the flat fill proc */
	BAIL_IF_FALSE((rad->numStops > 1), err, kFskErrBadData);							/* And defer to it if there is only one stop */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(RadialGradientSpanData) + (rad->numStops - 1) * sizeof(GradientStop), (FskMemPtr*)(void*)(&span->spanData)));
	gd					= (RadialGradientSpanData*)(span->spanData);					/* Get a local copy of the pointer to the span auxiliary data structure */
	gd->flatFill		= span->fill;													/* Save the flat fill proc */
	gd->numStops		= rad->numStops;												/* Record the number of stops */
	gd->spreadMethod	= (rad->spreadMethod >> kFskSpreadPosX) & kFskSpreadMask;		/* Extract the X spread method ... */
	gd->spreadMethod	= (gd->spreadMethod << kFskSpreadPosX) | (gd->spreadMethod << kFskSpreadPosY);	/* ... and duplicate it to Y, since we only do 1D */
	span->set			= SetRadialGradientSpan;										/* Initialize the gradient coordinate for the beginning of the span on each scanline */
	span->fill			= FillRadialGradientSpan;
	span->initEdge		= NULL;
	span->advanceEdge	= NULL;
	span->disposeSpanData	= DisposeRadialGradientSpanData;							/* (FskDisposeSpanDataProc)FskMemPtrDispose would work as well, I think */

	if (!GradientNeedsBlending(rad->numStops, rad->gradientStops)) 	{	gd->setPixel = GetSetPixelProc(  dstBM->pixelFormat);	gd->setPixelFromRadius = SetSpanPixelFromRadius;	}
	else															{	gd->setPixel = GetBlendPixelProc(dstBM->pixelFormat);	gd->setPixelFromRadius = BlendSpanPixelFromRadius;	}
	BAIL_IF_NULL(gd->setPixel, err, kFskErrUnsupportedPixelType);


	/*
	 * Determine the type of gradient to draw, initialize the radial gradient coordinate system, and computational coefficients.
	 * There are 6 cases:
	 * dx == 0 && dr == 0:	degenerate. Draw nothing.
	 * dx == 0:				isotropic cone
	 * dr == 0:				circle slider
	 * dx == dr:			tangent circles
	 * dx < dr:				skew cone
	 * dx > dr:				shrinking ellipse slider
	 */

	sku.x = rad->center.x - rad->focus.x;												/* Skew vector, with rad->gradientFracBits fractional bits */
	sku.y = rad->center.y - rad->focus.y;												/* rad->gradientFracBits fractional bits */
	skum  = FskFixedVectorNormalize(&sku.x, 2);											/* Unit vector in the direction of the skew, skum=rad->gradientFracBits, sku=2.30 */
	dr    = rad->radius - rad->focalRadius;												/* We assume dr >= 0. TODO: Should we allow dr < 0? */
	transfoc.x = FskFracMul(rad->focus.x, sku.x) + FskFracMul(rad->focus.y, sku.y);		/* Transformation of the focus point */
	transfoc.y = FskFracMul(rad->focus.y, sku.x) - FskFracMul(rad->focus.x, sku.y);


	if (skum == 0) {																	/* Isotropic */
		BAIL_IF_FALSE(dr != 0, err, kFskErrSingular);									/* Degenerate */
		/* With focus as the origin, we have:	t = -r1/dr + sqrt((x^2 + y^2) / dr^2
		 * With change of variables:			u = (x - xf) / dr, v = (y - yf) / dr
		 * we have:								t = -r1/dr + sqrt(u^2 + v^2)
		 */
		gd->gradXform.M[2][0] = -FskFixedNDiv(rad->focus.x, dr, OFFSET_FRACBITS);
		gd->gradXform.M[2][1] = -FskFixedNDiv(rad->focus.y, dr, OFFSET_FRACBITS);
		gd->gradXform.M[0][0] = gd->gradXform.M[1][1] = FskFixedNDiv(1 << rad->gradientFracBits, dr, OFFSET_FRACBITS);
		gd->gradXform.M[1][0] = gd->gradXform.M[0][1] = 0;
		BAIL_IF_FALSE(FiniteMatrix(&gd->gradXform), err, kFskErrSingular);				/* dr is too small */
		gd->k0 = -FskFixedNDiv(rad->focalRadius, dr, OFFSET_FRACBITS);
		gd->computeRadialDistance = ComputeCanvasRadialDistanceIsotropic;
		//#define PRINT_RADIAL_SETUP
		#ifdef PRINT_RADIAL_SETUP
			printf("CV                   %+10.4f=dr %+10.4f=skum %+10.4f=skum-dr          Isotropic:   MT[[%+9.3g%+10.3g%+10.3g][%+9.3g%+10.3g%+10.3g]], kx=%.3g, k0=%.3g\n",
				ldexp(dr, -rad->gradientFracBits),
				ldexp(skum, -rad->gradientFracBits),
				ldexp(skum - dr, -rad->gradientFracBits),
				ldexp(gd->gradXform.M[0][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[0][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][1], -OFFSET_FRACBITS),
				ldexp(gd->kx, -OFFSET_FRACBITS),
				ldexp(gd->k0, -OFFSET_FRACBITS)
			);
		#endif /* PRINT_RADIAL_SETUP */
	}
	else if (dr == 0) {																	/* Circle slide */
	tinyDR:
		/* With focus as the origin, and x pointing from focus to center,
		 * we have:						t = x / dx - sqrt(R^2 - y^2) / dx
		 * with change of variables:	u = x / dx, v = y / dx
		 * we have:						t = u - sqrt((r/dx)^2 - v^2)
		 */
		gd->gradXform.M[0][0] = -FskFixedNDiv(sku.x,      skum, OFFSET_FRACBITS-30+rad->gradientFracBits);
		gd->gradXform.M[1][0] = -FskFixedNDiv(sku.y,      skum, OFFSET_FRACBITS-30+rad->gradientFracBits);
		gd->gradXform.M[2][0] =  FskFixedNDiv(transfoc.x, skum, OFFSET_FRACBITS);
		gd->gradXform.M[0][1] =  FskFixedNDiv(sku.y,      skum, OFFSET_FRACBITS-30+rad->gradientFracBits);
		gd->gradXform.M[1][1] = -FskFixedNDiv(sku.x,      skum, OFFSET_FRACBITS-30+rad->gradientFracBits);
		gd->gradXform.M[2][1] =  FskFixedNDiv(transfoc.y, skum, OFFSET_FRACBITS);
		gd->kr0 = FskFixedNDiv(rad->radius, skum, OFFSET_FRACBITS);
		gd->computeRadialDistance = ComputeCanvasRadialDistanceCircleSlide;
		#ifdef PRINT_RADIAL_SETUP
			printf("CV                   %+10.4f=dr %+10.4f=skum %+10.4f=skum-dr              Slide:   MT[[%+9.3g%+10.3g%+10.3g][%+9.3g%+10.3g%+10.3g]], kr0=%.3g, kr0^2=%.3g\n",
				ldexp(dr, -rad->gradientFracBits),
				ldexp(skum, -rad->gradientFracBits),
				ldexp(skum - dr, -rad->gradientFracBits),
				ldexp(gd->gradXform.M[0][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[0][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][1], -OFFSET_FRACBITS),
				ldexp(gd->kr0, -OFFSET_FRACBITS),
				pow(ldexp(gd->kr0, -OFFSET_FRACBITS), 2.)
			);
		#endif /* PRINT_RADIAL_SETUP */
	}
	else if (FskFixDiv(abs(skum - dr), rad->radius) <= (1 << (16 - 6))) {			/* Tangent if abs(skum - dr) / radius < 1/64 */
		/* With focus as the origin, and x pointing from focus to center,
		 * we have:						t = (x - r1 + y^2/(x+r1)) / (2 * dr)
		 * with change of variables:	u = x/(2*dr), v = y/(2*dr)
		 * we have:						t = u - r1/(2*dr) + v^2 / (u + r1/(2*dr))
		 */
	xtrm:
 		gd->gradXform.M[2][0] = -FskFixedNDiv(transfoc.x, dr, OFFSET_FRACBITS-1);
 		gd->gradXform.M[2][1] = -FskFixedNDiv(transfoc.y, dr, OFFSET_FRACBITS-1);
		gd->gradXform.M[0][0] =  (gd->gradXform.M[1][1] = FskFixedNDiv(sku.x, dr, OFFSET_FRACBITS-30+rad->gradientFracBits-1));
		gd->gradXform.M[0][1] = -(gd->gradXform.M[1][0] = FskFixedNDiv(sku.y, dr, OFFSET_FRACBITS-30+rad->gradientFracBits-1));
		gd->k0 = -(gd->kr0 = FskFixedNDiv(rad->focalRadius, dr, OFFSET_FRACBITS-1));
		gd->computeRadialDistance = ComputeCanvasRadialDistanceTangent;
		#ifdef PRINT_RADIAL_SETUP
			printf("CV                   %+10.4f=dr %+10.4f=skum %+10.4f=skum-dr            Tangent:   MT[[%+9.3g%+10.3g%+10.3g][%+9.3g%+10.3g%+10.3g]], kr0=%.3g, k0=%.3g\n",
				ldexp(dr, -rad->gradientFracBits),
				ldexp(skum, -rad->gradientFracBits),
				ldexp(skum - dr, -rad->gradientFracBits),
				ldexp(gd->gradXform.M[0][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][0], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[0][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[1][1], -OFFSET_FRACBITS),
				ldexp(gd->gradXform.M[2][1], -OFFSET_FRACBITS),
				ldexp(gd->kr0, -OFFSET_FRACBITS),
				ldexp(gd->k0,  -OFFSET_FRACBITS)
			);
		#endif /* PRINT_RADIAL_SETUP */
	}
	else {
		/* First we place the focus at the origin with the center on the positive x axis.
		 * Then we have a change of variable:	u = (dx * r1 + dr * (x - x1)) / abs(dr^2 - dx^2), v = y / sqrt((abs(dr^2 - dx^2))
		 */
		FskFixed den = FskFixedSqrt64to32((FskInt64)(dr + skum) * (dr - skum));		/* rad->gradientFracBits */
		FskFixed28 edr, edx, eux, euy, efx, efy, er1;
		if (den < 0) den = -den;		/* Get the magnitude of the complex number for normalization */
		edr = FskFixedNDiv(dr,               den, OFFSET_FRACBITS);				/* OFFSET_FRACBITS fractional bits */
		edx = FskFixedNDiv(skum,             den, OFFSET_FRACBITS);				/* OFFSET_FRACBITS fractional bits */
		efx = FskFixedNDiv(transfoc.x,       den, OFFSET_FRACBITS);				/* OFFSET_FRACBITS fractional bits */
		efy = FskFixedNDiv(transfoc.y,	     den, OFFSET_FRACBITS);				/* OFFSET_FRACBITS fractional bits */
		er1 = FskFixedNDiv(rad->focalRadius, den, OFFSET_FRACBITS);				/* OFFSET_FRACBITS fractional bits */
		eux = FskFixedNDiv(sku.x,            den, rad->gradientFracBits);		/* 30 fractional bits */
		euy = FskFixedNDiv(sku.y,            den, rad->gradientFracBits);		/* 30 fractional bits */
		gd->gradXform.M[0][0] = FskFracMul(edr, eux);
		gd->gradXform.M[1][0] = FskFracMul(edr, euy);
		gd->gradXform.M[2][0] = FskFixedNMul(edx, er1, OFFSET_FRACBITS) - FskFixedNMul(edr, efx, OFFSET_FRACBITS);
		gd->gradXform.M[0][1] = -(euy + (1 << (30 - OFFSET_FRACBITS - 1))) >> (30 - OFFSET_FRACBITS);	/* Round from 30 to 24 fractional bits */
		gd->gradXform.M[1][1] =  (eux + (1 << (30 - OFFSET_FRACBITS - 1))) >> (30 - OFFSET_FRACBITS);	/* Round from 30 to 24 fractional bits */
		gd->gradXform.M[2][1] = -efy;
		if (!FiniteMatrix(&gd->gradXform))	/* If the coefficients are too big for fixed point... */
			goto xtrm;						/* ... assume the extreme degenerate case with tangent corcles. */
		gd->k0 = -FskFixedNDiv(rad->focalRadius, dr, OFFSET_FRACBITS);
		gd->kx = -FskFixedNDiv(skum,             dr, OFFSET_FRACBITS);
		if (NotFiniteFixed(gd->k0) || NotFiniteFixed(gd->kx))
			goto tinyDR;

		if (skum < dr) {																/* Skew cone */
			/* First we place the focus at the origin with the center on the positive x axis.
			 * Then we have a change of variable:	u = (dx * r1 + dr * (x - x1)) / abs(dr^2 - dx^2), v = y / sqrt((abs(dr^2 - dx^2))
			 * Yielding:							t = -r1/dr - dx/dr * u - sqrt(u^2 + v^2)
			 */
			gd->computeRadialDistance = ComputeCanvasRadialDistanceSkewCone;
			#ifdef PRINT_RADIAL_SETUP
				printf("CV                   %+10.4f=dr %+10.4f=skum %+10.4f=skum-dr           SkewCone:   MT[[%+9.3g%+10.3g%+10.3g][%+9.3g%+10.3g%+10.3g]], kx=%.3g, k0=%.3g\n",
					ldexp(dr, -rad->gradientFracBits),
					ldexp(skum, -rad->gradientFracBits),
					ldexp(skum - dr, -rad->gradientFracBits),
					ldexp(gd->gradXform.M[0][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[1][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[2][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[0][1], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[1][1], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[2][1], -OFFSET_FRACBITS),
					ldexp(gd->kx, -OFFSET_FRACBITS),
					ldexp(gd->k0, -OFFSET_FRACBITS)
				);
			#endif /* PRINT_RADIAL_SETUP */
		}
		else {																			/* Shrinking circle slider */
			/* First we place the focus at the origin with the center on the positive x axis.
			 * Then we have a change of variable:	u = -(dx * r1 + dr * (x - x1)) / (dr^2 - dx^2), v = y / sqrt(abs(dx^2 - dr^2))
			 * Yielding:							t = -r1/dr - dx/dr * u + sqrt(u^2 - v^2)
			 */
			gd->kx = -gd->kx;
			gd->computeRadialDistance = ComputeCanvasRadialDistanceCircleShrink;
			#ifdef PRINT_RADIAL_SETUP
				printf("CV                   %+10.4f=dr %+10.4f=skum %+10.4f=skum-dr       CircleShrink:   MT[[%+9.3g%+10.3g%+10.3g][%+9.3g%+10.3g%+10.3g]], kx=%.3g, k0=%.3g\n",
					ldexp(dr, -rad->gradientFracBits),
					ldexp(skum, -rad->gradientFracBits),
					ldexp(skum - dr, -rad->gradientFracBits),
					ldexp(gd->gradXform.M[0][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[1][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[2][0], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[0][1], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[1][1], -OFFSET_FRACBITS),
					ldexp(gd->gradXform.M[2][1], -OFFSET_FRACBITS),
					ldexp(gd->kx, -OFFSET_FRACBITS),
					ldexp(gd->k0, -OFFSET_FRACBITS)
				);
			#endif /* PRINT_RADIAL_SETUP */
		}
	}
	ApplyGradientTransformations(pathMatrix, rad->gradientMatrix, &gd->gradXform);


	CopyGradientStops(gd->numStops, rad->gradientStops, gd->stops, dstBM);


bail:
	if (err != kFskErrNone) {
		if (span->spanData != NULL) {
			DisposeRadialGradientSpanData(span->spanData);
			span->spanData = NULL;
		}
	}

	return err;
}
