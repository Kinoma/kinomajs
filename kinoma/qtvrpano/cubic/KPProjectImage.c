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

#if defined(__ARMCC_VERSION)
	#include "ArmTo68kGlue.h"
#endif

#include "Fsk.h"
#include "KPProjectImage.h"
#include "KPDHClipPolygon.h"
#include "FskFixedMath.h"
#include "KPMatrix.h"
#include "FskPixelOps.h"
#include <stdlib.h>
#include <limits.h>


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Platform Tuning								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* Platform identification and math prototypes */
#if defined(__ARMCC_VERSION) || defined(_WIN32)		/* This would be appropriate for ARM or THUMB code */
	#define TARGET_RT_BIG_ENDIAN	0
	#define TARGET_RT_LITTLE_ENDIAN	1
	#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#include <math.h>
#elif defined(__linux__) || defined(ANDROID)
	#include "Fsk.h"
#elif TARGET_OS_KPL
	#include <math.h>
#else	/* !TARGET_OS_KPL */							/* Neither Palm nor KPL nor ARM: could be Mac or Windows */
	#include <math.h>
#endif	/* !TARGET_OS_KPL */

#ifdef MARVELL_SOC_PXA168
	#define USE_WMMX_BILERP565SE
#endif /* MARVELL_SOC_PXA168 */
#ifdef USE_WMMX_BILERP565SE
	extern void FskBilerp565SESetImm_arm_wMMX_s(void);
	extern UInt16 FskBilerp565SE_arm_wMMX_dj(UInt32 dj,const UInt16 *s, SInt32 rb);
	extern UInt16 FskBilerp565SE_arm_wMMX_di(UInt32 di,const UInt16 *s);
	extern UInt16 FskBilerp565SE_arm_wMMX_didj(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb);
	#define FskBilerp565SE_arm_wMMX(uf, vf, s, srb)	((vf)	? ((uf)	?	FskBilerp565SE_arm_wMMX_didj((uf), (vf), (const UInt16*)(s), (srb))	\
																	:	FskBilerp565SE_arm_wMMX_dj((vf), (const UInt16*)(s), (srb)))		\
															: ((uf)	?	FskBilerp565SE_arm_wMMX_di((uf), (const UInt16*)(s))				\
																	:	*(s)))
#endif /* USE_WMMX_BILERP565SE */


/* ALIGNTO: Platform-dependent macro for alignment coercion */
#if defined(_WIN32)				/* List compilers for which stdint.h is not available */
	#include <stddef.h>
	#define ALIGNTO(ptr, type)	(type*)(((intptr_t)(ptr) + sizeof(type) - 1) & ~(sizeof(type) - 1))	/* This only works when sizeof(type) == pow(2, n) */
#else	/* Those compilers for which stdint.h is available */
	#include <stdint.h>
	#define ALIGNTO(ptr, type)	(type*)(((intptr_t)(ptr) + sizeof(type) - 1) & ~(sizeof(type) - 1))	/* This only works when sizeof(type) == pow(2, n) */
#endif


/* Single precision transcendental functions are faster, so we'd like to use them if available */
#if !defined(USE_SINGLE_PRECISION_TRANSCENDENTALS)
	#if defined(_WIN32)
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#else /* !_WIN32 */
		#define USE_SINGLE_PRECISION_TRANSCENDENTALS 0
	#endif /* _WIN32 */
#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */


/* Make it pretty in the editor */
#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh) || defined(__MWERKS__)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /* !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /* !PRAGMA_MARK_SUPPORTED */
#endif /* PRAGMA_MARK_SUPPORTED */


/* For statistics gathering */
//#define GATHER_STATISTICS
#ifdef GATHER_STATISTICS
	static unsigned long	maxLength	= 0;
	static double			avgLength	= 0;
	static unsigned long	numSpans	= 0;
#endif /* GATHER_STATISTICS */


/* For checking source image bounds violations */
//#define BOUNDS_CHECK


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Typedefs and Macros							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

typedef struct Edge	Edge;
typedef struct Span	Span;
typedef void		(*FillSpanProc)(Span *span);

struct Edge {
	Edge			*next;					/* Linked list of edges */
	Span			*span;					/* The data structure used in filling spans */
	short			top, bottom;			/* The activation and expiration Y of the edge */
	FskFixed		x, dx;					/* The location of the edge on the current scanline, and its derivative w.r.t. y */
	FskFixed		u,  v,  w;				/* FskFixed u; FskFixed v; FskFixed24 w; */
	FskFixed		du, dv, dw;				/* Deltas w.r.t. the edge */
};

struct Span {
	const void		*baseAddr;				/* The source texture map base address */
	long			rowBytes;				/* The source texture map Y stride in bytes */
	int				width;					/* The source texture map width */
	int				height;					/* The source texture map height */

	FskFixed		M[3][3];				/* FskFixed u; FskFixed v; FskFixed24 w; */
	FillSpanProc	fill;					/* The fill span proc */

	void			*p;						/* The pointer to the initial destination pixel */
	int				dx;						/* The length of the span */
	int				qdx;					/* Subspan for quadratic forward differencing */

	FskFixed		u, v, w;				/* Current homogeneous texture coordinates */

	FskFixed		u0, u1, u2, v0, v1, v2;	/* Forward differencing coefficients */
	FskFixed		un, vn;					/* u and v at the end of the span */

	int				wFracBits;				/* The number of fractional bits for w in canonical form */
//#define FLOAT_QFD_SCALE
#ifdef FLOAT_QFD_SCALE
	float			qfdScale;				/* Scale for computing the intervale length for quadratic forward differencing */
#else /* FLOAT_QFD_SCALE */
	FskFixed		qfdScale;				/* Scale for computing the intervale length for quadratic forward differencing */
	int				qfdScaleShift;			/* [Negated] exponent of the product qfdScale * w^(4/3) */
#endif /* FLOAT_QFD_SCALE */
};


/********************************************************************************
 *****								BOUNDS_CHECK							*****
 ********************************************************************************/
#ifdef BOUNDS_CHECK
		#if TARGET_OS_MAC
			#include "TextUtils.h"
		#elif TARGET_OS_WINDOWS */
		#endif /* TARGET_OS_WINDOWS */
		#define VIOLATION_COLOR		0xF81FF81F	/* 565 magenta x 2 */
		#include <stdio.h>
		#include <stdarg.h>
		static void MyError(char *fmt, ...)
		{	va_list argList;
			va_start(argList, fmt);

			#if TARGET_OS_MAC
			{	char message[512];					/* Message to Macintosh debug console */
				vsprintf(message, fmt, argList);
				CopyCStringToPascal(message, (unsigned char*)message);
				DebugStr((unsigned char*)message);
			}
			#else /* !TARGET_OS_MAC */
				vfprintf(stderr, fmt, argList);		/* Message to stderr */
			#endif /* TARGET_OS_MAC */

			va_end(argList);
		}

	static void SrcBoundsViolation(unsigned int i, unsigned int j, Span *span, void *d, int pixelBytes)
	{
		static const int	violationColor = VIOLATION_COLOR;
		unsigned char *p;	const unsigned char *q;
		for (p = d, q = (const unsigned char*)(&violationColor); pixelBytes--; )
			*p++ = *q++;
		MyError("BoundsCheck: (%d, %d) not in [%d, %d]\n", i, j, span->width, span->height);
	}
	#define CHECK_SRC_BOUNDS	if ((i >= (unsigned int)(span->width)) || (j >= (unsigned int)(span->height))) {	\
									SrcBoundsViolation(i, j, span, d, FskName3(fsk,DstPixelKind,Bytes));			\
								} else
	#define CHECK_SRC_SP_BOUNDS	if ( ((i >= (unsigned int)(span->width  - 1)) && ((i >= (unsigned int)(span->width)  || uf != 0)))		\
								||   ((j >= (unsigned int)(span->height - 1)) && ((j >= (unsigned int)(span->height) || vf != 0))) ) {	\
									SrcBoundsViolation(i, j, span, d, FskName3(fsk,DstPixelKind,Bytes));									\
								} else
#else /* BOUNDS_CHECK */
#define CHECK_SRC_BOUNDS	/* nothing */
#define CHECK_SRC_SP_BOUNDS	/* nothing */
#endif /* BOUNDS_CHECK */


#define MAX_CLIP_POINTS(n)					(int)((n) * 4)

static int RoundDoubleToInt(double x)		{ return (int)(x + ((x < 0) ? -0.5 : 0.5)); }

#define RoundDoubleToPositiveInt(x)			(int)((x) + 0.5)
#define TruncFixedToInt(x)					((int)(x) >> 16)
#define FloorFixedToInt(x)					TruncFixedToInt(x)
#define CeilFixedToInt(x)					TruncFixedToInt((x) + 0xFFFF)
#define RoundFixedToInt(x)					TruncFixedToInt((x) + 0x8000)

#ifndef IntToFixed
	#define IntToFixed(x)					((FskFixed)(x) << 16)
#endif /* IntToFixed */

#if __FSK_LAYER__
	#include "FskMemory.h"
	#define	KPAlloc(z)	FskMemPtrAlloc(z)
	#define KPFree(p)	FskMemPtrDispose(p)
#elif !defined(__ARMCC_VERSION)
	#define	KPAlloc(z)	malloc(z)
	#define KPFree(p)	free(p)
#else
	#define	KPAlloc(z)	kmalloc(state, z)
	#define KPFree(p)	kfree(state, p)
#endif

#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****																		*****
 *****					Quadratic Forward Differencing						*****
 *****																		*****
 ********************************************************************************
 *****																		*****
 ***** The coefficients are given as										*****
 *****																		*****
 *****	| C0 |     | 1  0   0   |  |  1  0  0 |  | F(t0) |					*****
 *****	| C1 |  =  | 0  a  a^2  |  | -3  4 -1 |  | F(t1) |					*****
 *****	| C2 |     | 0  0  2a^2 |  |  1 -2  1 |  | F(t2) |					*****
 *****																		*****
 ***** where																*****
 *****		F() is the function to be evaluated,							*****
 *****		t1 = (t0 + t2) is the midpoint between							*****
 *****		the left (t0) and the right (t1)								*****
 *****		a = 1 / N														*****
 *****		N is the number of evaluations between the left and right.		*****
 *****																		*****
 ***** The maximum interval length can be computed from:					*****
 *****		N = floor(1/delta * cubrt(128 * tol / f3max))					*****
 ***** where																*****
 *****		delta is the distance between evaluations (we use delta=1)		*****
 *****		tol is the tolerance											*****
 *****		f3max is the maximum magnitude of the third derivative			*****
 *****		of the function													*****
 *****		between the left and the right of the interval					*****
 ***** The projective function,												*****
 *****		u = (U + x * Ux) / (W + x * Wx)									*****
 ***** has the third derivative,											*****
 *****		6 * Wx * (Ux - Wx * U0/W0) / (1 + x * (Wx/W0) / (W0 + Wx * x)^3	*****
 ***** Yield the expression													*****
 *****		N = cbrt((64/3) * tol / (Wx^2 * (Ux*W0-Wx*U0)) * Wmin^(4/3)		*****
 ***** This is reasonable because the O() analysis yields:					*****
 *****	W^(4/3) * W^(-3/3) * U^(-1/3)) = W^(1/3) * U^(-1/3) =(W/U)^(1/3)	*****
 ***** which is dimensionless.												*****
 ***** To accommodate v, we compute the max of the invariants				*****
 *****		det = max{ Ux*W0-Wx*U0 , Vx*W0-Wx*V0 }							*****
 ***** when precomputing the scale factor									*****
 *****		S =  cbrt((64/3) * tol / (Wx^2 * det)) )						*****
 *****																		*****
 ***** The maximum interval length is then given by							*****
 *****		N =  S * Wmin^(4/3)												*****
 *****																		*****
 ***** When W is close to 1, W^(4/3) can be approximated by					*****
 *****		1 + 4/3 * dw + 2/9 dw^2 + ...									*****
 ***** giving us															*****
 *****		N = S * (1 + 4/3 * (Wmin - 1))									*****
 *****																		*****
 ***** Let us try to determine the range for the scale factor.				*****
 ***** If w is represented in the 8.24 fixed point format,					*****
 ***** then its nonzero magnitude range is [2^(-24), 2^8 - 2^(-24)],		*****
 ***** though as a practical matter, we assume a max of 2.8.				*****
 ***** This will be raised to the 4/3 power, so the range [2^(-24), 2^2],	*****
 ***** assuming that fixed point computations flush							*****
 ***** really small w's to zero, thus yielding the range					*****
 *****		[w^(4/3)] = [2^(-24), 2^2]										*****
 ***** If the window size if 2^10 max, and the minimum subspan is 2^3,		*****
 ***** then the scale subspan length N takes on the range					*****
 *****		[N] = [2^3, 2^10]												*****
 ***** The scale factor S then takes on the range							*****
 *****		[S] = [N] / [w^(4/3)]											*****
 *****			= [2^3, 2^10] / [2^(-24), 2^2]								*****
 *****			= [2^1, 2^34]												*****
 ***** Unfortunately, this requires 33 bits, so we need to make further		*****
 ***** restrictions, choosing to reduce this to 31 bits, so that we can		*****
 ***** use a signed rather than unsigned representation.					*****
 ***** Really small w's imply really extreme perspective, which is			*****
 ***** improbable, so we choose to trim the range from the weeny w end.		*****
 ***** With these decisions, we arrive at the range							*****
 *****		[S] = [2^1, 2^32]												*****
 ***** implying that S has (-1) fractional bits.							*****
 *****																		*****
 ***** Since w is stored with maximum precision, it is not guaranteed to	*****
 ***** be in the format 8.24, and may be as extreme as 2.30 or 16.16		*****
 ***** or even more. So we bite the bullet and store an extra field			*****
 ***** indicating how much to shift by when computing the subspan length.	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define USE_SPAN_FORWARD_DIFFERENCING


#define THEORETICAL_MAXIMUM_TOLERANCE
#define QFD_TOLERANCE		0.125f						/* 1/8 pixel tolerance */
#define QFD_MIN_LENGTH		8							/* Minimum length of a QFD span */
#define QFD_MAX_LOG_LENGTH	10
#define QFD_MAX_LENGTH		(1L << QFD_MAX_LOG_LENGTH)	/* Maximum length of a QFD span */
#define QFD_EXTRA_BITS		QFD_MAX_LOG_LENGTH
#define AFFINE_EXTRA_BITS	18

/* t must obviously be an lvalue for this to work */
#define SCALEBYFOURTHIRDS(t)	t += t >> 2; t += t >> 4; t += t >> 8; t += t >> 16


/********************************************************************************
 * CubeRootApprox
 *	Only an approximate cube root is required. 12 bits more than suffices.
 *	And cbrt isn't available from <math.h> on Windows.
 ********************************************************************************/

static float
CubeRootApprox(float x)
{
	float	fr, r;
	int		shx;
	int		ex;

	/* Argument reduction */
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		fr = (float)frexp(x, &ex);		/* separate into mantissa and exponent */
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		fr = frexpf(x, &ex);			/* separate into mantissa and exponent */
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
	shx = ex % 3;
	if (shx > 0)
		shx -= 3;			/* compute shx such that (ex - shx) is divisible by 3 */
	ex = (ex - shx) / 3;	/* exponent of cube root */
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		fr = (float)ldexp(fr, shx);									/* 0.125 <= fr < 1.0 */
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		fr = ldexpf(fr, shx);										/* 0.125 <= fr < 1.0 */
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */

	/* Compute seed with a quadratic approximation */
	fr = (-0.46946116F * fr + 1.072302F) * fr + 0.3812513F;			/* 0.5   <= fr < 1.0 */
	#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
		r = (float)ldexp(fr, ex);									/* 6 bits of precision */
	#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		r = ldexpf(fr, ex);											/* 6 bits of precision */
	#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */

	/* Newton-Raphson iterations */
	r = (float)(2.0/3.0) * r + (float)(1.0/3.0) * x / (r * r);		/* 12 bits of precision */
	//r = (float)(2.0/3.0) * r + (float)(1.0/3.0) * x / (r * r);	/* 24 bits of precision */

	return(r);
}


/********************************************************************************
 * ComputeForwardDifferenceScale
 *
 * S =  cubrt((64/3) * tol / (Wx^2 * max{ Ux*W0-Wx*U0 , Vx*W0-Wx*V0 } )) )
 ********************************************************************************/

static float
ComputeForwardDifferenceScale(double M[3][3])
{
	float a, b;

	if (M[0][2] == 0) {
		b = (float)INT_MAX;
	}
	else {
		#if !USE_SINGLE_PRECISION_TRANSCENDENTALS
			a = (float)fabs(M[0][0] * M[2][2] - M[2][0] * M[0][2]);		/* U det = Ux * W0 - U0 * Wx */
			b = (float)fabs(M[0][1] * M[2][2] - M[2][1] * M[0][2]);		/* V det = Vx * W0 - V0 * Wx */
		#else /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
			a = fabsf((float)(M[0][0] * M[2][2] - M[2][0] * M[0][2]));	/* U det = Ux * W0 - U0 * Wx */
			b = fabsf((float)(M[0][1] * M[2][2] - M[2][1] * M[0][2]));	/* V det = Vx * W0 - V0 * Wx */
		#endif /* USE_SINGLE_PRECISION_TRANSCENDENTALS */
		if (a < b)
			a = b;			/* get max of u and v determinant magnitudes */
		/* The following falls out of the computations... */
		#ifdef THEORETICAL_MAXIMUM_TOLERANCE
			b = CubeRootApprox(QFD_TOLERANCE * (64.0f / 3.0f) / ((float)M[0][2] * (float)M[0][2] * a));	/* cbrt(eps * 64/3 / (Wx^2 * (Ux*W0-U0*Wx)) = O(U^(-1/3) * W^(-1)) */
		#else /* EMPIRICAL_TYPICAL_TOLERANCE */
			b = CubeRootApprox(QFD_TOLERANCE * 64.0f / ((float)M[0][2] * (float)M[0][2] * a));			/* cbrt(eps * 64 / (Wx^2 * (Ux*W0-U0*Wx)) = O(U^(-1/3) * W^(-1)) */
		#endif /* EMPIRICAL_TYPICAL_TOLERANCE */
	}
	return b;
}


/********************************************************************************
 * FourThirdsPower: approximate x^(4/3)
 *		if (w < 1)	p = 0.5 * w + 0.5 * w^2;
 *		else		p = (w - 1) * 4/3 + 1;
 ********************************************************************************/

#define FRACT_FOUR_THIRDS			0x71C71C72	/* 4/3 = 1.333... */

static FskFixed24
FourThirdsPower(FskFixed w, int wFracBits)
{
	FskFixed wOne = (1L << wFracBits);

	w = (w >= wOne) ? (FskFixedNMul((w - wOne), FRACT_FOUR_THIRDS, 30) + wOne)
					: (FskFixedNMul((w + wOne), w, wFracBits + 1));
	return w;
}


/********************************************************************************
 * ComputeForwardDifferenceCoefficients
 ********************************************************************************/

static int
ComputeForwardDifferenceCoefficients(register Span *span)
{
	FskFixed	um, vm, wm, wn;
	int			t;

	/* Affine transformation */
	if (span->M[0][2] == 0) {
		span->u1 = FskFixedNDiv(span->M[0][0], span->w, 24 + QFD_EXTRA_BITS);       span->u2 = 0;
		span->v1 = FskFixedNDiv(span->M[0][1], span->w, 24 + QFD_EXTRA_BITS);       span->v2 = 0;
		span->qdx = span->dx;	/* Eat the whole span */
		span->dx  = 0;
		return 1;				/* Indicate an affine transformation */
	}

	/* Projective transformation */
	else {
		/* Compute the span length at the left end. This is most conservative when Wx = M[0][2] >= 0 */
		#ifdef FLOAT_QFD_SCALE
			span->qdx = span->w * span->qfdScale;
		#else /* FIXED_QFD_SCALE */
			span->qdx = FskFixedNMul(span->w, span->qfdScale, span->qfdScaleShift);
		#endif /* FIXED_QFD_SCALE */

		if (span->M[0][2] < 0) {	/* W is shrinking: conservative computation should use the right end of w for wMin */
			/* Limit this result for the following computations */
			if (span->qdx > span->dx)
				span->qdx = span->dx;	/* Should make sure that qdx < 32768 */

			/* First we do an adjustment to the end of the interval by linear approximation.
			 * Nright = S * (Wleft + Nleft * Wx) ^ (4/3)
			 * For small d, then
			 *	(w+d)^(4/3) ~= w^(4/3) * (1 + 4/3 * d/w)
			 * giving:
			 * Nright = S * (Wleft + Nleft * Wx) ^ (4/3)
			 *       ~= S * Wleft^(4/3) * (1 + 4/3 * Nleft * Wx / WLeft)
			 *        = NLeft * (1 + 4/3 * Nleft * Wx / WLeft)
			 *        = NLeft + NLeft * 4/3 * Nleft * Wx / WLeft
			 *        = NLeft + 4/3 * Nleft^2 * Wx / WLeft
			 * and if Wleft is close to 1, we can eliminate the division.
			 *
			 * But maybe it's more trouble than it's worth.
			 */
			#ifdef W_CLOSE_TO_ONE
				t = span->qdx + FskFixedNMul(FskFixedNMul(FRACT_FOUR_THIRDS, span->M[0][2], 30), span->qdx * span->qdx, span->wFracBits);
			#else /* !W_CLOSE_TO_ONE */
				t = span->qdx + FskFixedNRatio(span->qdx * span->qdx, span->M[0][2], 3 * span->w, 2);
			#endif /* !W_CLOSE_TO_ONE */
			if (t >= (span->qdx >> 2)) {	/* Shouldn't reduce the subspan length to less than 25% */
				span->qdx = t;
			}
			else {							/* If it does, try a more accurate iteration */
				/* Iterate */
				#ifdef FLOAT_QFD_SCALE
					span->qdx = FourThirdsPower(span->w + span->qdx * span->M[0][2], span->wFracBits) * span->qfdScale;
				#else /* FIXED_QFD_SCALE */
					span->qdx = FskFixedNMul(FourThirdsPower(span->w + span->qdx * span->M[0][2], span->wFracBits), span->qfdScale, span->qfdScaleShift);
				#endif /* FIXED_QFD_SCALE */

				/* Guard against negatives -- we shouldn't need this */
//				#ifdef PARANOID
					if (span->qdx <= 0)
						span->qdx = 1;
//				#endif /* PARANOID */
			}
		}

		/* Adjust span length by the subspan length */
		if (span->qdx <= 0)
			span->qdx = 1;
		if ((span->dx -= span->qdx) < 0) {
			span->qdx += span->dx;
			span->dx = 0;
		}

		/* Don't waste time with QFD for short spans */
		if (span->qdx < QFD_MIN_LENGTH) {
			if (span->dx != 0) {			/* There's still some left */
				if (	(span->M[0][2] > 0)	/* It will get better */
					&&  (span->qdx > 0)		/* We have a good estimate */
					&&	((t = (QFD_MIN_LENGTH - span->qdx) * (span->w - (span->w >> 2)) / (span->qdx * span->M[0][2]) - span->qdx) < (span->dx - QFD_MIN_LENGTH))	/* in time */
                    &&  (t > 0)
				) {							/* Things will get better in time */
					span->qdx += t;			/* Only do per pixel divisions until QFD is advantageous */
					span->dx  -= t;
				}
				else {						/* Things won't get better in time */
					span->qdx += span->dx;	/* Do all the remaining span with per pixel divisions */
					span->dx   = 0;
				}
//				#ifdef PARANOID
					if (span->qdx <= 0)
						span->qdx = 1;
//				#endif /* PARANOID */
			}
			return 0;	/* Use per-pixel projective evaluation */
		}

		/* Compute mid and right function values */
		um       = span->qdx * span->M[0][0];		vm = span->qdx * span->M[0][1];				wm = span->qdx * span->M[0][2];	/* d(u,v,w) to the right */
		span->un = um + span->u;					span->vn = vm + span->v;					wn = wm + span->w;				/* right (u,v,w) */
		um       = (um >> 1) + span->u;				vm = (vm >> 1) + span->v;					wm = (wm >> 1) + span->w;		/* middle (u,v,w) */
		um       = FskFixedNDiv(um, wm, 24);        vm = FskFixedNDiv(vm, wm, 24);
		span->un = FskFixedNDiv(span->un, wn, 24);  span->vn = FskFixedNDiv(span->vn, wn, 24);
		/* Compute quadratic forward difference coefficients
		 *	| u0 |   |  1   0     0    |  |  1   0   0  |  | L |
		 *	| u1 | = |  0  1/N  1/N^2  |  | -3   4  -1  |  | M |
		 *	| u2 |   |  0   0   1/N^2  |  |  1  -2   1  |  | R |
		 */
		t = span->qdx * span->qdx;
		span->u2 = FskFixedNDiv((span->u0 - (um << 1) + span->un),              t,         QFD_EXTRA_BITS + 2);                 /* u2 = (u0 - 2 * um + un) * 4 / (n * n) */
		span->v2 = FskFixedNDiv((span->v0 - (vm << 1) + span->vn),              t,         QFD_EXTRA_BITS + 2);                 /* v2 = (v0 - 2 * vm + vn) * 4 / (n * n) */
		span->u1 = FskFixedNDiv((((um - span->u0) << 2) + span->u0 - span->un), span->qdx, QFD_EXTRA_BITS) + (span->u2 >> 1);   /* u1 = (4 * um - 3 * u0 - un) / n + u2 / 2 */
		span->v1 = FskFixedNDiv((((vm - span->v0) << 2) + span->v0 - span->vn), span->qdx, QFD_EXTRA_BITS) + (span->v2 >> 1);   /* v1 = (4 * vm - 3 * v0 - vn) / n + v2 / 2 */

		/* Advance projective coordinates */
		span->u += span->qdx * span->M[0][0];
		span->v += span->qdx * span->M[0][1];
		span->w += span->qdx * span->M[0][2];

		#ifdef GATHER_STATISTICS
			if (maxLength < span->qdx)
				maxLength = span->qdx;
			if (numSpans != 0xFFFFFFFF) {
				avgLength = ((avgLength * numSpans + span->qdx) / (numSpans + 1));
				numSpans++;
			}
			else {
				avgLength = ((avgLength + span->qdx) * 0.5);
				numSpans = 1;
			}
		#endif /* GATHER_STATISTICS */

		return 2;	/* Use QFD loop */
	}
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Texture span procs							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * This things need to be kept in sync
 ********************************************************************************/

/* These are #defined in the interface */
#if		(kpFormat32BGRA			!= 0)	\
	||	(kpFormat32RGBA			!= 1)	\
	||	(kpFormat32ARGB			!= 2)	\
	||	(kpFormat32A16RGB565LE	!= 3)	\
	||	(kpFormat24BGR			!= 4)	\
	||	(kpFormat16RGB565LE		!= 5)
	#error kpFormats are out of sync
#endif
#define kpNumPixelFormats	6

#if	  TARGET_RT_BIG_ENDIAN
	/*		table index		pixel kind */
	#define PF0				32BGRA
	#define PF1				32RGBA
	#define PF2				32ARGB
	#define PF3				32A16RGB565DE
	#define PF4				24BGR
	#define PF5				16RGB565DE

	/*		enable blit proc	enable system pixel format */
	#define src_32BGRA			SRC_32BGRA
	#define src_32RGBA			SRC_32RGBA
	#define src_32ARGB			SRC_32ARGB
	#define src_32ABGR			SRC_32ABGR
	#define src_24BGR			SRC_24BGR
	#define src_16RGB565DE		SRC_16RGB565LE
	#define src_16RGB565SE		SRC_16RGB565BE
	#define src_32A16RGB565SE	0				/* unsupported */

	/*		enable blit proc	enable system pixel format */
	#define dst_32BGRA			DST_32BGRA
	#define dst_32RGBA			DST_32RGBA
	#define dst_32ARGB			DST_32ARGB
	#define dst_32ABGR			DST_32ABGR
	#define dst_24BGR			DST_24BGR
	#define dst_16RGB565SE		DST_16RGB565BE
	#define dst_16RGB565DE		DST_16RGB565LE
	#define dst_32A16RGB565SE	0				/* unsupported */
#elif TARGET_RT_LITTLE_ENDIAN
	/*		table index		pixel kind */
	#define PF0				32ARGB
	#define PF1				32ABGR
	#define PF2				32BGRA
	#define PF3				32A16RGB565SE
	#define PF4				24BGR
	#define PF5				16RGB565SE

	/*		enable blit proc	enable system pixel format */
	#define src_32ARGB			SRC_32BGRA
	#define src_32ABGR			SRC_32RGBA
	#define src_32BGRA			SRC_32ARGB
	#define src_32RGBA			SRC_32ABGR
	#define src_32A16RGB565SE	SRC_32A16RGB565LE
	#define src_24BGR			SRC_24BGR
	#define src_16RGB565SE		SRC_16RGB565LE
	#define src_16RGB565DE		SRC_16RGB565BE

	/*		enable blit proc	enable system pixel format */
	#define dst_32ARGB			DST_32BGRA
	#define dst_32ABGR			DST_32RGBA
	#define dst_32BGRA			DST_32ARGB
	#define dst_32RGBA			DST_32ABGR
	#define dst_24BGR			DST_24BGR
	#define dst_16RGB565SE		DST_16RGB565LE
	#define dst_16RGB565DE		DST_16RGB565BE
	#define dst_32A16RGB565SE	DST_32A16RGB565SE
#else /* TARGET_RT_OTHER_ENDIAN */
	#error Unknown endian
#endif /* TARGET_RT_OTHER_ENDIAN */


/********************************************************************************
 ********************************************************************************
 *****						The span procs themselves						*****
 ********************************************************************************
 ********************************************************************************/

#if defined(__ARMCC_VERSION)

	#if TARGET_RT_LITTLE_ENDIAN
		#define SrcPixelKind	16RGB565SE	/* Little-endian 565 */
		#define DstPixelKind	16RGB565SE
	#else /* TARGET_RT_BIG_ENDIAN */
		#define SrcPixelKind	16RGB565DE	/* Little-endian 565 */
		#define DstPixelKind	16RGB565DE
	#endif /* TARGET_RT_BIG_ENDIAN */

	#include "KPProjectiveSpan.c"
#else /* !__ARMCC_VERSION */



/* 32BGRA source */
#define SrcPixelKind	32BGRA
#define DstPixelKind	32BGRA
#if src_32BGRA && dst_32BGRA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32BGRA
#define DstPixelKind	32RGBA
#if src_32BGRA && dst_32RGBA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32BGRA
#define DstPixelKind	32ARGB
#if src_32BGRA && dst_32ARGB
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32BGRA
#define DstPixelKind	32ABGR
#if src_32BGRA && dst_32ABGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32BGRA
#define DstPixelKind	24BGR
#if src_32BGRA && dst_24BGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#if TARGET_RT_LITTLE_ENDIAN
	#define SrcPixelKind	32BGRA
	#define DstPixelKind	32A16RGB565SE
	#if src_32BGRA && dst_32A16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32BGRA
	#define DstPixelKind	16RGB565SE
	#if src_32BGRA && dst_16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#else /* TARGET_RT_BIG_ENDIAN */
	#define SrcPixelKind	32BGRA
	#define DstPixelKind	32A16RGB565DE
	#if src_32BGRA && dst_32A16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32BGRA
	#define DstPixelKind	16RGB565DE
	#if src_32BGRA && dst_16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#endif /* TARGET_RT_BIG_ENDIAN */


/* 32RGBA source */
#define SrcPixelKind	32RGBA
#define DstPixelKind	32BGRA
#if src_32RGBA && dst_32BGRA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32RGBA
#define DstPixelKind	32RGBA
#if src_32RGBA && dst_32RGBA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32RGBA
#define DstPixelKind	32ARGB
#if src_32RGBA && dst_32ARGB
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32RGBA
#define DstPixelKind	32ABGR
#if src_32RGBA && dst_32ABGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32RGBA
#define DstPixelKind	24BGR
#if src_32RGBA && dst_24BGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#if TARGET_RT_LITTLE_ENDIAN
	#define SrcPixelKind	32RGBA
	#define DstPixelKind	32A16RGB565SE
	#if src_32RGBA && dst_32A16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32RGBA
	#define DstPixelKind	16RGB565SE
	#if src_32RGBA && dst_16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#else /* TARGET_RT_BIG_ENDIAN */
	#define SrcPixelKind	32RGBA
	#define DstPixelKind	32A16RGB565DE
	#if src_32RGBA && dst_32A16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32RGBA
	#define DstPixelKind	16RGB565DE
	#if src_32RGBA && dst_16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#endif /* TARGET_RT_BIG_ENDIAN */


/* 32ARGB source */
#define SrcPixelKind	32ARGB
#define DstPixelKind	32BGRA
#if src_32ARGB && dst_32BGRA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ARGB
#define DstPixelKind	32RGBA
#if src_32ARGB && dst_32RGBA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ARGB
#define DstPixelKind	32ARGB
#if src_32ARGB && dst_32ARGB
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ARGB
#define DstPixelKind	32ABGR
#if src_32ARGB && dst_32ABGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ARGB
#define DstPixelKind	24BGR
#if src_32ARGB && dst_24BGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#if TARGET_RT_LITTLE_ENDIAN
	#define SrcPixelKind	32ARGB
	#define DstPixelKind	32A16RGB565SE
	#if src_32ARGB && dst_32A16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32ARGB
	#define DstPixelKind	16RGB565SE
	#if src_32ARGB && dst_16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#else /* TARGET_RT_BIG_ENDIAN */
	#define SrcPixelKind	32ARGB
	#define DstPixelKind	32A16RGB565DE
	#if src_32ARGB && dst_32A16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32ARGB
	#define DstPixelKind	16RGB565DE
	#if src_32ARGB && dst_16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#endif /* TARGET_RT_BIG_ENDIAN */



/* 32ABGR source */
#define SrcPixelKind	32ABGR
#define DstPixelKind	32BGRA
#if src_32ABGR && dst_32BGRA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ABGR
#define DstPixelKind	32RGBA
#if src_32ABGR && dst_32RGBA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ABGR
#define DstPixelKind	32ARGB
#if src_32ABGR && dst_32ARGB
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ABGR
#define DstPixelKind	32ABGR
#if src_32ABGR && dst_32ABGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	32ABGR
#define DstPixelKind	24BGR
#if src_32ABGR && dst_24BGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#if TARGET_RT_LITTLE_ENDIAN
	#define SrcPixelKind	32ABGR
	#define DstPixelKind	32A16RGB565SE
	#if src_32ABGR && dst_32A16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32ABGR
	#define DstPixelKind	16RGB565SE
	#if src_32ABGR && dst_16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#else /* TARGET_RT_BIG_ENDIAN */
	#define SrcPixelKind	32ABGR
	#define DstPixelKind	32A16RGB565DE
	#if src_32ABGR && dst_32A16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32ABGR
	#define DstPixelKind	16RGB565DE
	#if src_32ABGR && dst_16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#endif /* TARGET_RT_BIG_ENDIAN */


/* 24BGR source */
#define SrcPixelKind	24BGR
#define DstPixelKind	32BGRA
#if src_24BGR && dst_32BGRA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	24BGR
#define DstPixelKind	32RGBA
#if src_24BGR && dst_32RGBA
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	24BGR
#define DstPixelKind	32ARGB
#if src_24BGR && dst_32ARGB
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	24BGR
#define DstPixelKind	32ABGR
#if src_24BGR && dst_32ABGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#define SrcPixelKind	24BGR
#define DstPixelKind	24BGR
#if src_24BGR && dst_24BGR
	#include "KPProjectiveSpan.c"
#else
	#include "KPProjectiveSpanStub.c"
#endif

#if TARGET_RT_LITTLE_ENDIAN
	#define SrcPixelKind	24BGR
	#define DstPixelKind	32A16RGB565SE
	#if src_24BGR && dst_32A16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	24BGR
	#define DstPixelKind	16RGB565SE
	#if src_24BGR && dst_16RGB565SE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#else /* TARGET_RT_BIG_ENDIAN */
	#define SrcPixelKind	24BGR
	#define DstPixelKind	32A16RGB565DE
	#if src_24BGR && dst_32A16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	24BGR
	#define DstPixelKind	16RGB565DE
	#if src_24BGR && dst_16RGB565DE
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif
#endif /* TARGET_RT_BIG_ENDIAN */


#if TARGET_RT_LITTLE_ENDIAN

	/* 16RGB565SE source */
	#define SrcPixelKind	16RGB565SE
	#define DstPixelKind	32BGRA
	#if src_16RGB565SE && dst_32BGRA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565SE
	#define DstPixelKind	32RGBA
	#if src_16RGB565SE && dst_32RGBA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565SE
	#define DstPixelKind	32ARGB
	#if src_16RGB565SE && dst_32ARGB
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565SE
	#define DstPixelKind	32ABGR
	#if src_16RGB565SE && dst_32ABGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565SE
	#define DstPixelKind	24BGR
	#if src_16RGB565SE && dst_24BGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#if TARGET_RT_LITTLE_ENDIAN
		#define SrcPixelKind	16RGB565SE
		#define DstPixelKind	32A16RGB565SE
		#if src_16RGB565SE && dst_32A16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	16RGB565SE
		#define DstPixelKind	16RGB565SE
		#if src_16RGB565SE && dst_16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#else /* TARGET_RT_BIG_ENDIAN */
		#define SrcPixelKind	16RGB565SE
		#define DstPixelKind	32A16RGB565DE
		#if src_16RGB565SE && dst_32A16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	16RGB565SE
		#define DstPixelKind	16RGB565DE
		#if src_16RGB565SE && dst_16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#endif /* TARGET_RT_BIG_ENDIAN */


	/* 32A16RGB565SE source */
	#define SrcPixelKind	32A16RGB565SE
	#define DstPixelKind	32BGRA
	#if src_32A16RGB565SE && dst_32BGRA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565SE
	#define DstPixelKind	32RGBA
	#if src_32A16RGB565SE && dst_32RGBA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565SE
	#define DstPixelKind	32ARGB
	#if src_32A16RGB565SE && dst_32ARGB
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565SE
	#define DstPixelKind	32ABGR
	#if src_32A16RGB565SE && dst_32ABGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565SE
	#define DstPixelKind	24BGR
	#if src_32A16RGB565SE && dst_24BGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#if TARGET_RT_LITTLE_ENDIAN
		#define SrcPixelKind	32A16RGB565SE
		#define DstPixelKind	32A16RGB565SE
		#if src_32A16RGB565SE && dst_32A16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	32A16RGB565SE
		#define DstPixelKind	16RGB565SE
		#if src_32A16RGB565SE && dst_16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#else /* TARGET_RT_BIG_ENDIAN */
		#define SrcPixelKind	32A16RGB565SE
		#define DstPixelKind	32A16RGB565DE
		#if src_32A16RGB565SE && dst_32A16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	32A16RGB565SE
		#define DstPixelKind	16RGB565DE
		#if src_32A16RGB565SE && dst_16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#endif /* TARGET_RT_BIG_ENDIAN */

#else /* TARGET_RT_BIG_ENDIAN */

	/* 16RGB565DE source */
	#define SrcPixelKind	16RGB565DE
	#define DstPixelKind	32BGRA
	#if src_16RGB565DE && dst_32BGRA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565DE
	#define DstPixelKind	32RGBA
	#if src_16RGB565DE && dst_32RGBA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565DE
	#define DstPixelKind	32ARGB
	#if src_16RGB565DE && dst_32ARGB
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565DE
	#define DstPixelKind	32ABGR
	#if src_16RGB565DE && dst_32ABGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	16RGB565DE
	#define DstPixelKind	24BGR
	#if src_16RGB565DE && dst_24BGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#if TARGET_RT_LITTLE_ENDIAN
		#define SrcPixelKind	16RGB565DE
		#define DstPixelKind	32A16RGB565SE
		#if src_16RGB565DE && dst_32A16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	16RGB565DE
		#define DstPixelKind	16RGB565SE
		#if src_16RGB565DE && dst_16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#else /* TARGET_RT_BIG_ENDIAN */
		#define SrcPixelKind	16RGB565DE
		#define DstPixelKind	32A16RGB565DE
		#if src_16RGB565DE && dst_32A16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	16RGB565DE
		#define DstPixelKind	16RGB565DE
		#if src_16RGB565DE && dst_16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#endif /* TARGET_RT_BIG_ENDIAN */


	/* 32A16RGB565DE source */
	#define SrcPixelKind	32A16RGB565DE
	#define DstPixelKind	32BGRA
	#if src_32A16RGB565DE && dst_32BGRA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565DE
	#define DstPixelKind	32RGBA
	#if src_32A16RGB565DE && dst_32RGBA
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565DE
	#define DstPixelKind	32ARGB
	#if src_32A16RGB565DE && dst_32ARGB
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565DE
	#define DstPixelKind	32ABGR
	#if src_32A16RGB565DE && dst_32ABGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#define SrcPixelKind	32A16RGB565DE
	#define DstPixelKind	24BGR
	#if src_32A16RGB565DE && dst_24BGR
		#include "KPProjectiveSpan.c"
	#else
		#include "KPProjectiveSpanStub.c"
	#endif

	#if TARGET_RT_LITTLE_ENDIAN
		#define SrcPixelKind	32A16RGB565DE
		#define DstPixelKind	32A16RGB565SE
		#if src_32A16RGB565DE && dst_32A16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	32A16RGB565DE
		#define DstPixelKind	16RGB565SE
		#if src_32A16RGB565DE && dst_16RGB565SE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#else /* TARGET_RT_BIG_ENDIAN */
		#define SrcPixelKind	32A16RGB565DE
		#define DstPixelKind	32A16RGB565DE
		#if src_32A16RGB565DE && dst_32A16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif

		#define SrcPixelKind	32A16RGB565DE
		#define DstPixelKind	16RGB565DE
		#if src_32A16RGB565DE && dst_16RGB565DE
			#include "KPProjectiveSpan.c"
		#else
			#include "KPProjectiveSpanStub.c"
		#endif
	#endif /* TARGET_RT_BIG_ENDIAN */

#endif /* TARGET_RT_BIG_ENDIAN */

#endif /* !__ARMCC_VERSION */


/********************************************************************************
 * Pixel size table, for convenience
 ********************************************************************************/

#if !defined(__ARMCC_VERSION)		// palm only uses 16 bit pixels

static const unsigned char myPixelSize[kpNumPixelFormats] = {
	FskName3(fsk,PF0,Bytes),
	FskName3(fsk,PF1,Bytes),
	FskName3(fsk,PF2,Bytes),
	FskName3(fsk,PF3,Bytes),
	FskName3(fsk,PF4,Bytes),
	FskName3(fsk,PF5,Bytes)
};

#endif

/********************************************************************************
 * Texture span fill proc dispatcher table
 ********************************************************************************/

#if !defined(__ARMCC_VERSION)
	/* Projective span procs */
	#define _FP(s,d)	{ FskName3(ProjectiveTexSpan,s,d),FskName3(BilerpProjectiveTexSpan,s,d) }
	#define _FP_D(s)	{ _FP(s,PF0),_FP(s,PF1),_FP(s,PF2),_FP(s,PF3),_FP(s,PF4),_FP(s,PF5) }

	static const FillSpanProc projectiveFillProcs[kpNumPixelFormats][kpNumPixelFormats][2] = {
					_FP_D(PF0),_FP_D(PF1),_FP_D(PF2),_FP_D(PF3),_FP_D(PF4),_FP_D(PF5)
	};
	#undef _FP
	#undef _FP_D

	/* Affine span procs */
	#define _FP(s,d)	{ FskName3(AffineTexSpan,s,d),FskName3(BilerpAffineTexSpan,s,d) }
	#define _FP_D(s)	{ _FP(s,PF0),_FP(s,PF1),_FP(s,PF2),_FP(s,PF3),_FP(s,PF4),_FP(s,PF5) }

	static const FillSpanProc affineFillProcs[kpNumPixelFormats][kpNumPixelFormats][2] = {
					_FP_D(PF0),_FP_D(PF1),_FP_D(PF2),_FP_D(PF3),_FP_D(PF4),_FP_D(PF5)
	};
	#undef _FP
	#undef _FP_D
#endif /* __ARMCC_VERSION */


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						Scan-Conversion Utilities						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/*******************************************************************************
 * LinkEdges
 *******************************************************************************/

static void
LinkEdges(register int numEdges, register Edge *edge)
{
	register Edge *n;

	for (numEdges--; numEdges-- > 0; edge = n) {
		n = edge + 1;
		edge->next = n;
	}
	edge->next = NULL;
}


/*******************************************************************************
 * YSortEdges
 *******************************************************************************/

static void
YSortEdges(Edge **edges)
{
	register int	sorted;
	register Edge	**lastNext, *e, *n;

	/* Bubble sort */
	for (sorted = 0; !sorted; ) {
		sorted = 1;
		for (lastNext = edges; *lastNext != NULL; lastNext = &((*lastNext)->next)) {
			e = *lastNext;
			if ((n = e->next) == NULL)
				break;
			if (
				(n->top < e->top) ||
				((n->top == e->top) && (
						(n->x < e->x) ||
						((n->x == e->x) && (n->dx < e->dx))
					)
				)
			) {
				e->next = n->next;
				n->next = e;
				*lastNext = n;
				sorted = 0;
			}
		}
	}
}


/*******************************************************************************
 * RemoveDegenerateEdges
 *******************************************************************************/

static void
RemoveDegenerateEdges(Edge **edges)
{
	register Edge **lastNext, *e, *n;

	for (lastNext = edges; *lastNext != NULL; ) {
		e = *lastNext;
		if ((n = e->next) == NULL)
			break;
		if (	(e->top    == n->top)
			&&	(e->bottom == n->bottom)
			&&	(e->x      == n->x)
			&&	(e->dx     == n->dx)
			&&	(e->span   == n->span)
		)
			*lastNext = n->next;	/* Remove both e and n */
		else
			lastNext = &(e->next);	/* Skip to the next one */
	}
}


/*******************************************************************************
 * XSortInsertEdge
 *******************************************************************************/

static void
XSortInsertEdge(Edge *edge, Edge **edges)
{
	register Edge		**lastNext;
	register FskFixed	x				= edge->x;

	for (lastNext = edges; *lastNext != NULL; lastNext = &((*lastNext)->next))
		if ((x < (*lastNext)->x) || ((x == (*lastNext)->x) && (edge->dx < (*lastNext)->dx)))
			break;
	edge->next = *lastNext;
	*lastNext = edge;
}


/*******************************************************************************
 * MoveAfterEdge
 *******************************************************************************/

static void
MoveAfterEdge(Edge *first, Edge **secondPreviousNext)
{
	Edge *second	= *secondPreviousNext;

	*secondPreviousNext = second->next;
	second->next        = first->next;
	first->next         = second;
}


/*******************************************************************************
 * PairEdges
 *******************************************************************************/

static int
PairEdges(Edge *edges)
{
	Edge **lastNext, *n;

	for ( ; edges != NULL; edges = edges->next->next) {
		if ((n = edges->next) == NULL)
			return(0);
		if (edges->span == n->span)											/* Good: properly paired */
			continue;

		for (lastNext = &n->next; ; lastNext = &(**lastNext).next) {
			if (*lastNext == NULL)
				return(0);
			if (edges->span == (**lastNext).span) {
				MoveAfterEdge(edges, lastNext);								/* Pair them up properly */
				break;
			}
		}
	}
	return(1);
}


/*******************************************************************************
 * PreviousNextEdge
 *******************************************************************************/

static Edge**
PreviousNextEdge(Edge **list, Edge *e)
{
	register Edge *p, **lastNext;

	for (p = *(lastNext = list); (p != NULL) && (p != e); p = *(lastNext = &p->next)) ;

	return(lastNext);
}


/*******************************************************************************
 * XSortEdges
 *******************************************************************************/

#ifdef EXPECT_CROSSOVERS

static void
XSortEdges(Edge **edges)
{

	register int	sorted;
	register Edge	**lastNext, *e, *n;

	/* Bubble sort */
	for (sorted = 0; !sorted; ) {
		sorted = 1;
		for (lastNext = edges; *lastNext != NULL; lastNext = &((*lastNext)->next)) {
			e = *lastNext;
			if ((n = e->next) == NULL)
				break;
			if (
				(n->x < e->x) ||
				((n->x == e->x) && (n->dx < e->dx))
			) {
				e->next = n->next;
				n->next = e;
				*lastNext = n;
				sorted = 0;
			}
		}
	}
}

#endif /* EXPECT_CROSSOVERS */


/********************************************************************************
 * CountPointsInImage3D
 ********************************************************************************/

static int
CountPointsInImage3D(const KPImage3D *s)
{
	return ((s->numPts > 0) && (s->pts != NULL)) ? s->numPts : 4;
}


/********************************************************************************
 * CountPointsInImages3D
 ********************************************************************************/

static int
CountPointsInImages3D(int n, const KPImage3D *s, int *maxPtsOneImage)
{
	int	k, m, t;

	for (k = 0, m = 0; n-- > 0; s++) {
		if (m < (t = CountPointsInImage3D(s)))
			m = t;								/* Max in any one image */
		k += t;									/* Total for all images */
	}

	*maxPtsOneImage = m;						/* Max in any one image */
	return k;									/* Total for all images */
}


/********************************************************************************
 * BoundingRectOfImage3D
 ********************************************************************************/

static void
BoundingRectOfImage3D(const KPImage3D *im, KPRect *r)
{
	if ((im->numPts > 0) && (im->pts != NULL)) {		/* Clipping polygon given */
		int	n, i;
		float	(*p)[2];
		KPRect	re;

		re.xMin = INT_MAX;
		re.yMin = INT_MAX;
		re.xMax = INT_MIN;
		re.yMax = INT_MIN;

		for (n = im->numPts, p = im->pts; n--; p++) {	/* FInd the bounding box of the polygon */
			if (re.xMin > (i = (int)((*p)[0])))			/* Truncate from float to int */
				re.xMin = i;
			if (re.xMax < i)
				re.xMax = i;
			if (re.yMin > (i = (int)((*p)[1])))			/* Truncate from float to int */
				re.yMin = i;
			if (re.yMax < i)
				re.yMax = i;
		}

		if (re.xMin < 0)			re.xMin = 0;		/* Clip against the image bounds */
		if (re.yMin < 0)			re.yMin = 0;
		if (re.xMax > im->width)	re.xMax = im->width;
		if (r->yMax > im->height)	re.yMax = im->height;

		*r = re;
	}
	else {												/* No clipping polygon: use image rect */
		r->xMin = 0;
		r->yMin = 0;
		r->xMax = im->width;
		r->yMax = im->height;
	}
}


/********************************************************************************
 * MakeRectPolygon
 ********************************************************************************/

static void
MakeRectPolygon(int width, int height, float rectPts[4][2])
{
	const float		zeroish		= (float)(-1.0 / 16384.0);
	const float		oneish		= (float)(1.0 - 1.0 / 16384.0);

	rectPts[0][0] = rectPts[1][0] = rectPts[0][1] = rectPts[3][1] = zeroish;
	rectPts[2][0] = rectPts[3][0] = width  - oneish;
	rectPts[1][1] = rectPts[2][1] = height - oneish;
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Projective								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * HomogeneousToEuclideanPoints
 ********************************************************************************/

static void
HomogeneousToEuclideanPoints(register KPDVector3D *v3, register int n)
{
	register KPDPoint2D	*p2;
	register double		z;

	for (p2 = (KPDPoint2D*)v3; n--; v3++, p2++) {
		z = v3->z;
		p2->x = v3->x / z;
		p2->y = v3->y / z;
	}
}


/********************************************************************************
 * TransformProjectivePoints
 * as do row vectors.
 ********************************************************************************/

static void
TransformProjectivePoints(int nPts, /*const*/float (*iPts)[2], /*const*/double M[3][3], KPDVector3D *oPts)
{
	for ( ; nPts--; iPts++, oPts++) {
		oPts->x = (*iPts)[0] * M[0][0] + (*iPts)[1] * M[1][0] + M[2][0];
		oPts->y = (*iPts)[0] * M[0][1] + (*iPts)[1] * M[1][1] + M[2][1];
		oPts->z = (*iPts)[0] * M[0][2] + (*iPts)[1] * M[1][2] + M[2][2];
	}
}


/********************************************************************************
 * SetFixedMatrixAndPoints
 *
 * This scales the matrix and the points so that they fit into fixed point numbers.
 * There are 16 fractional bits for u and v, and 24 for w.
 *
 * On input:	SetFixedMatrixAndPoints(const double *M, int numPts, KPDPoint2D     *xy, Span *span, KPDVector3D     *uvw);
 * On output:	SetFixedMatrixAndPoints(const double *M, int numPts, FskFixedPoint2D *xy, Span *span, FskFixedVector3D *uvw);
 * Returns 1 if successful 0 if unsuccessful.
 ********************************************************************************/

#define kScaling (30)

static int
SetFixedMatrixAndPoints(const double *M, int numPts, KPDPoint2D *xy, Span *span, KPDVector3D *uvw)
{
	double	Mi[3][3], t, uvMax, wMax, s16, s24, *d;
	int		i;
	FskFixed *x;

	KPDAdjointMatrix3x3(M, Mi[0]);	/* Don't need KPDInvertMatrix(M, 3, 3, Mi[0]) when adjoint will do, though is it as robust? */
	if (Mi[2][2] == 0)
		return 0;
	KPDScaleVector(1.0 / fabs(Mi[2][2]), Mi[0], Mi[0], 8);		/* Canonicalize matrix so that w is nominally equal to 1 */
	Mi[2][2] = (Mi[2][2] < 0) ? -1.0 : 1.0;						/* Assure that fabs9Mi[2][2] == 1 */

	/* Find range of matrix -- we might want to do something special with affine transforms */
	for (i = 3, d = Mi[0], uvMax = 0.0, wMax = 0.0; i--; ) {
		if (uvMax < (t = fabs(*d++))) uvMax = t;					/* Find the largest u or v */
		if (uvMax < (t = fabs(*d++))) uvMax = t;
		if ( wMax < (t = fabs(*d++)))  wMax = t;					/* Find the largest w */
	}

	/* Find range of uvw points */
	KPDAffineTransform(&xy->x, Mi[0], &uvw->x, numPts, 2, 3);		/* Find corresponding (u,v,w) for all (x,y) */
	for (i = numPts, d = &uvw->x; i--; ) {
		if (uvMax < (t = fabs(*d++))) uvMax = t;					/* Find the largest u or v */
		if (uvMax < (t = fabs(*d++))) uvMax = t;
		if ( wMax < (t = fabs(*d++)))  wMax = t;					/* Find the largest w */
	}
	if (uvMax < (t = wMax*256.0)) uvMax = t;						/* Max the range */
	frexp(uvMax, &i);
	s16 = ldexp(1.0, kScaling - i);									/* For scaling u and v */
	s24 = s16 * 256.0;												/* For scaling w (with 8 more bits than u or v) */
	span->wFracBits = kScaling + 8 - i;								/* Determine the fractional bits of w */

	/* Compute the scale factor used to determine subspan length with quadratic forward differencing */
	#ifdef FLOAT_QFD_SCALE
		span->qfdScale = ComputeForwardDifferenceScale(Mi) / s24;
	#else /* FIXED_QFD_SCALE */
		t = ComputeForwardDifferenceScale(Mi) / s24;
		frexp(t, &i);												/* Get exponent of scale */
		span->qfdScaleShift = kScaling - i;							/* With a 30 bit mantissa, this is the scale adjustment */
		span->qfdScale = (FskFixed)ldexp(t, span->qfdScaleShift);	/* Mantissa of scale */
	#endif /* FIXED_QFD_SCALE */

	for (i = 3, x = span->M[0], d = Mi[0]; i--; ) {					/* Convert matrix to fixed point */
		*x++ = RoundDoubleToInt(*d * s16);	d++;					/* 16 fractional bits for u ... */
		*x++ = RoundDoubleToInt(*d * s16);	d++;					/* ... and v */
		*x++ = RoundDoubleToInt(*d * s24);	d++;					/* 24 fractional bits for w */
	}

	for (i = numPts, x = (FskFixed*)uvw, d = &uvw->x; i--; ) {		/* Convert uvw to fixed point in place */
		*x++ = RoundDoubleToInt(*d * s16);	d++;					/* 16 fractional bits for u ... */
		*x++ = RoundDoubleToInt(*d * s16);	d++;					/* ... and v */
		*x++ = RoundDoubleToInt(*d * s24);	d++;					/* 24 fractional bits for w */
	}

	s16 = 65536.0;
	for (i = 2 * numPts, x = (FskFixed*)xy, d = &xy->x; i--; ) {	/* Convert xy to fixed point in place */
		*x++ = RoundDoubleToPositiveInt(*d * s16); d++;
	}

	return 1;
}


/********************************************************************************
 * InitProjectiveEdge
 ********************************************************************************/

static int
InitProjectiveEdge(const FskFixedPoint2D *p0, const FskFixedPoint2D *p1, const FskFixedVector3D *u0, const FskFixedVector3D *u1, Span *span, Edge *e)
{
	FskFixed	dy;
	FskFract	fr;

	if ((dy = p1->y - p0->y) == 0)									/* No need to remember horizontal edges */
		return 0;

	if (dy < 0) {													/* Assure that p0 is on the top */
		const void	*t;
		t = p0;		p0 = p1;	p1 = t;								/* Swap xy's */
		t = u0;		u0 = u1;	u1 = t;								/* Swap uvw's */
		dy = -dy;													/* dy positive */
	}

	e->top		= (short)CeilFixedToInt(p0->y);						/* Birth scanline */
	e->bottom	= (short)CeilFixedToInt(p1->y);						/* Expiration scanline */
	if (e->top == e->bottom)
		return 0;													/* No need to remember edges that quantize to horizontal */

	fr = FskFracDiv(IntToFixed(e->top) - p0->y, dy);					/* Proportion of top scanline from top of edge */

	e->dx = p1->x - p0->x;	e->x = p0->x + FskFracMul(e->dx, fr);	/* Compute deltas and evaluate functions at the top scanline */
	e->du = u1->x - u0->x;	e->u = u0->x + FskFracMul(e->du, fr);
	e->dv = u1->y - u0->y;	e->v = u0->y + FskFracMul(e->dv, fr);
	e->dw = u1->z - u0->z;	e->w = u0->z + FskFracMul(e->dw, fr);

	if (dy > 0x8000) {												/* Compute derivatives if dy is not close to zero */
		e->dx = FskFixDiv(e->dx, dy);
		e->du = FskFixDiv(e->du, dy);
		e->dv = FskFixDiv(e->dv, dy);
		e->dw = FskFixDiv(e->dw, dy);
	}

	e->span	= span;
	e->next	= NULL;

	return 1;
}


/********************************************************************************
 * TransformClipAndMakeProjectiveEdges
 ********************************************************************************/

static int
TransformClipAndMakeProjectiveEdges(
	int				numPts,
	/*const*/float	(*pts)[2],
	/*const*/float	M[3][3],
	const KPRect	*clipRect,
	Span			*span,
	Edge			*edges		/* Clip temp is tacked onto the end of the edges */
)
{
	KPDVector3D			*uvw, *xyz		= NULL;
	FskFixedVector3D	*pu, *lu;
	FskFixedPoint2D		*pp, *lp;
	int					n, numEdges;
	double				DM[3][3];

	/* Extract storage for xyz and uvw from the end of the edges */
	numEdges = MAX_CLIP_POINTS(numPts);								/* Clipping introduces more points */
	xyz      = (KPDVector3D*)ALIGNTO(edges + numEdges, double);		/* After the end of the edges is clipping storage for xyz ... */
	uvw      = (KPDVector3D*)(       xyz   + numEdges);				/* ... and uvw */

	/* Transform points */
	KPCopySingleToDoubleMatrix(M[0], DM[0], 3, 3);					/* Double is needed to get the full 32-bit precision for the scan converter, because single only has 24 */
	TransformProjectivePoints(numPts, pts, DM, xyz);				/* Perspective transforms Euclidean xy to homogeneous xyz */

	/* Clip points */
	n = numPts;
	KPDHClipPolygon(&n, xyz, uvw, clipRect);						/* Homogeneous clipping, using uvw as temporary storage */
	HomogeneousToEuclideanPoints(xyz, n);							/* Homogeneous xyz projected back to Euclidean xy in-place */

	/* Scale matrix and points so as to have the maximum resolution (30+ bits) for the scan converter */
	if (SetFixedMatrixAndPoints(DM[0], n, (KPDPoint2D*)xyz, span, uvw) == 0)	/* xy and uvw are converted from floating- to fixed-point in-place */
		return 0;

	for (numEdges = 0, lp = (pp = (FskFixedPoint2D*)xyz) + n - 1, lu = (pu = (FskFixedVector3D*)uvw) + n - 1; n--; lp = pp++, lu = pu++) {
		if (InitProjectiveEdge(lp, pp, lu, pu, span, edges)) {
			edges++;
			numEdges++;
		}
	}

	return numEdges;
}


/********************************************************************************
 * MakeProjectiveEdgesForImage3D
 ********************************************************************************/

static int
MakeProjectiveEdgesForImage3D(const KPImage3D *src, KPImage3D *dst, const KPRect *dstClip, Span *span, int quality, Edge *edges)
{
	int				numEdges			= 0;
	int				numPts;
	/*const*/float	(*pts)[2];
	float			rectPts[4][2];
	float			M[3][3];

	if ((span->baseAddr = src->baseAddr) == NULL)		/* Something went wrong with initializing the image -- abort early */
		goto bail;
	span->rowBytes	= src->rowBytes;
	span->width		= src->width;
	span->height	= src->height;
	if ((src->numPts > 0) && (src->pts != NULL)) {		/* Arbitrary polygon */
		numPts = src->numPts;
		pts    = src->pts;
	}
	else {												/* No points -- use the whole rectangle */
		MakeRectPolygon(src->width, src->height, rectPts);
		numPts = 4;
		pts    = rectPts;
	}

	#if defined(__ARMCC_VERSION)		/* Only 16RGB565LE accommodated for Palm, so we dispatch inline rather than in a table */
		#if TARGET_RT_LITTLE_ENDIAN
			span->fill = (quality == 0) ? ProjectiveTexSpan16RGB565SE16RGB565SE : BilerpProjectiveTexSpan16RGB565SE16RGB565SE;
		#else /* TARGET_RT_BIG_ENDIAN */
			span->fill = (quality == 0) ? ProjectiveTexSpan16RGB565DE16RGB565DE : BilerpProjectiveTexSpan16RGB565DE16RGB565DE;
		#endif /* TARGET_RT_BIG_ENDIAN */
	#else /* !__ARMCC_VERSION */							/* We dispatch through a table */
		span->fill		= projectiveFillProcs[src->pixelFormat][dst->pixelFormat][quality];
	#endif /* !__ARMCC_VERSION */
	KPSLinearTransform(src->M[0], dst->M[0], M[0], 3, 3, 3);

	numEdges = TransformClipAndMakeProjectiveEdges(numPts, pts, M, dstClip, span, edges);

bail:
	return numEdges;
}


/********************************************************************************
 * ScanConvertProjectiveEdges
 ********************************************************************************/

static void
ScanConvertProjectiveEdges(int numEdges, Edge *edges, KPImage3D *dst)
{
	Edge	*activeEdges, *e;
	int		y, nextBorn, nextExpired, nextEvent;
	Span	*span;
	char	*dstBaseAddr			= dst->baseAddr;
	long	dstRowBytes				= dst->rowBytes;
#if defined(__ARMCC_VERSION)				/* Only 16RGB565LE accommodated for Palm */
	const int	dstPixelBytes		= 2;
#else
	const int		dstPixelBytes	= myPixelSize[dst->pixelFormat];
#endif


	/* Initialize edge list */
	LinkEdges(numEdges, edges);
	YSortEdges(&edges);
	RemoveDegenerateEdges(&edges);

	if (edges == NULL)
		return;

	/* Initialize active edges */
	y = edges->top;
	activeEdges = NULL;
	while ((edges != NULL) || (activeEdges != NULL)) {

		/* Remove expired edges */
		{	register Edge **lastNext;
			for (lastNext = &activeEdges; *lastNext != NULL; ) {
				e = *lastNext;
				if (y >= e->bottom)																/* Edge is expired: remove */
					*lastNext = e->next;
				else																			/* Edge still active: look at the next one */
					lastNext = &e->next;
			}
		}

		/* Collect newborn edges */
		while ((edges != NULL) && (edges->top <= y)) {											/* Should ALWAYS be == */
			e = edges;
			edges = e->next;
			XSortInsertEdge(e, &activeEdges);
		}
		nextBorn = (int)((edges == NULL) ? INT_MAX : edges->top);								/* Since edges are sorted, the next edge birth is the next pending edge */

		/* Determine the next edge expiration */
		if (activeEdges == NULL) {																/* All edges accounted for */
			nextExpired = INT_MAX;
		}
		else {																					/* Look for the next edge to expire */
			for (nextExpired = activeEdges->bottom, e = activeEdges->next; e != NULL; e = e->next)
				if (nextExpired > e->bottom)
					nextExpired = e->bottom;
		}

		nextEvent = (nextBorn < nextExpired) ? nextBorn : nextExpired;							/* Stop at the next event, whether birth or expiration of an edge */

		/* Make sure that pairs of edges are from the same polygon */
		if (PairEdges(activeEdges) == 0)
			return;																				/* Numerical inconsistencies forces us to abort */

		/* Scan all active trapezoids until the next event */
		if (nextEvent < INT_MAX) {
			register Edge *L, *R;
			int dy = nextEvent - y;

			/* Render all polygons in scanline order */
			while (dy--) {
				#ifdef EXPECT_CROSSOVERS
					XSortEdges(&activeEdges);													/* We need to sort every line if we expect crossovers */
					PairEdges(activeEdges);														/* Resorting might unpair edges, so we need to repair */
				#endif /* EXPECT_CROSSOVERS */
				for (L = activeEdges; (L != NULL) && ((R = L->next) != NULL); L = R->next) {
					int left, right;
					FskFixed subx;

					#ifndef EXPECT_CROSSOVERS
						if (R->x < L->x) {														/* Edges crossed over */
							L->next = R->next;													/* Swap L and R */
							R->next = L;
							*PreviousNextEdge(&activeEdges, L) = R;
							L = R;
							R = L->next;
						}
					#endif /* !EXPECT_CROSSOVERS */

					left  = CeilFixedToInt(L->x);												/* Compute span width */
					subx  = IntToFixed(left) - L->x;
					right = FloorFixedToInt(R->x);
					span  = L->span;
					if ((span->dx = right - left + 1) > 0) {									/* If the span is at least one pixel */
						span->u = L->u + FskFixMul(subx, span->M[0][0]);						/* Compute texture coordinates */
						span->v = L->v + FskFixMul(subx, span->M[0][1]);
						span->w = L->w + FskFixMul(subx, span->M[0][2]);
						span->p = dstBaseAddr + (int)(L->top) * dstRowBytes + left * dstPixelBytes;	/* Set dst pointer to first pixel in span */
						L->top++; L->x += L->dx; L->u += L->du; L->v += L->dv; L->w += L->dw;	/* Update edges while still in the cache */
						R->top++; R->x += R->dx; R->u += R->du; R->v += R->dv; R->w += R->dw;
						(*span->fill)(span);
					}
					else {
						L->top++; L->x += L->dx; L->u += L->du; L->v += L->dv; L->w += L->dw;	/* Update edges */
						R->top++; R->x += R->dx; R->u += R->du; R->v += R->dv; R->w += R->dw;
					}
				}
			}
			y = nextEvent;
		}
		else
			break;
	}
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Affine								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * HomogeneousAffineToEuclideanPoints
 * No division is necessary since we know that Z == 1
 ********************************************************************************/

static void
HomogeneousAffineToEuclideanPoints(register KPDVector3D *v3, register int n)
{
	register KPDPoint2D	*p2;

	for (p2 = (KPDPoint2D*)v3; n--; v3++, p2++) {
		p2->x = v3->x;
		p2->y = v3->y;
	}
}


/********************************************************************************
 * TransformAffinePoints
 * as do row vectors.
 ********************************************************************************/

static void
TransformAffinePoints(int nPts, /*const*/float (*iPts)[2], /*const*/double M[3][3], KPDVector3D *oPts)
{
	for ( ; nPts--; iPts++, oPts++) {
		oPts->x = (*iPts)[0] * M[0][0] + (*iPts)[1] * M[1][0] + M[2][0];
		oPts->y = (*iPts)[0] * M[0][1] + (*iPts)[1] * M[1][1] + M[2][1];
		oPts->z = 1.0f;
	}
}


/********************************************************************************
 * SetFixedAffineMatrixAndPoints
 *
 * This scales the matrix and the points so that they fit into fixed point numbers.
 * There are AFFINE_EXTRA_BITS fractional bits for u and v.
 *
 * On input:	SetFixedAffineMatrixAndPoints(const double *M, int numPts, KPDPoint2D      *xy, Span *span, KPDVector3D      *uvw);
 * On output:	SetFixedAffineMatrixAndPoints(const double *M, int numPts, FskFixedPoint2D *xy, Span *span, FskFixedVector3D *uvw);
 ********************************************************************************/

static void
SetFixedAffineMatrixAndPoints(const double *M, int numPts, KPDPoint2D *xy, Span *span, FskFixedPoint2D *uv)
{
	double		Mi[3][2], *d;
	FskFixed	*x;
	int			i;
	double		di		= (1L << AFFINE_EXTRA_BITS) / (M[0*3+0] * M[1*3+1] - M[0*3+1] * M[1*3+0]);

	/* Compute inverse matrix, scaled by (1 << AFFINE_EXTRA_BITS */
	Mi[0][0] =  M[1*3+1] * di;										Mi[0][1] = -M[0*3+1] * di;
	Mi[1][0] = -M[1*3+0] * di;										Mi[1][1] =  M[0*3+0] * di;
	Mi[2][0] = -(Mi[1][0] * M[2*3+1] + Mi[0][0] * M[2*3+0] + 0.5);	Mi[2][1] = -(Mi[0][1] * M[2*3+0] + Mi[1][1] * M[2*3+1] + 0.5);
//	Mi[2][0] = (M[1*3+0] * M[2*3+1] - M[1*3+1] * M[2*3+0]) * di;	Mi[2][1] = (M[0*3+1] * M[2*3+0] - M[0*3+0] * M[2*3+1]) * di;

	/* Compute fixed-point UV from XY and inverse matrix, with AFFINE_EXTRA_BITS fractional bits */
	for (i = numPts; i--; xy++, uv++) {
		uv->x = (FskFixed)(Mi[2][0] + Mi[0][0] * xy->x + Mi[1][0] * xy->y);
		uv->y = (FskFixed)(Mi[2][1] + Mi[0][1] * xy->x + Mi[1][1] * xy->y);
	}
	xy -= numPts;	/* Reset xy pointer */

	/* Convert xy to fixed-point in-place, with 16 fractional bits */
	di = 65536.0;
	for (i = numPts, x = (FskFixed*)xy; i--; xy++) {
		*x++ = RoundDoubleToPositiveInt(xy->x * di);
		*x++ = RoundDoubleToPositiveInt(xy->y * di);
	}

	/* Convert matrix to fixed point with AFFINE_EXTRA_BITS fractional bits (n.b. the matrix is already scaled by (1L << AFFINE_EXTRA_BITS) */
	for (i = 3, x = span->M[0], d = Mi[0]; i--; ) {
		*x++ = RoundDoubleToInt(*d);	d++;
		*x++ = RoundDoubleToInt(*d);	d++;
		*x++ = 1L << AFFINE_EXTRA_BITS;
	}
}


/********************************************************************************
 * InitAffineEdge
 ********************************************************************************/

static int
InitAffineEdge(const FskFixedPoint2D *p0, const FskFixedPoint2D *p1, const FskFixedPoint2D *u0, const FskFixedPoint2D *u1, Span *span, Edge *e)
{
	FskFixed	dy;
	FskFract	fr;

	if ((dy = p1->y - p0->y) == 0)									/* No need to remember horizontal edges */
		return 0;

	if (dy < 0) {													/* Assure that p0 is on the top */
		const void	*t;
		t = p0;		p0 = p1;	p1 = t;								/* Swap xy's */
		t = u0;		u0 = u1;	u1 = t;								/* Swap uvw's */
		dy = -dy;													/* dy positive */
	}

	e->top		= (short)CeilFixedToInt(p0->y);						/* Birth scanline */
	e->bottom	= (short)CeilFixedToInt(p1->y);						/* Expiration scanline */
	if (e->top == e->bottom)
		return 0;													/* No need to remember edges that quantize to horizontal */

	fr = FskFracDiv(IntToFixed(e->top) - p0->y, dy);				/* Proportion of top scanline from top of edge */

	e->dx = p1->x - p0->x;	e->x = p0->x + FskFracMul(e->dx, fr);	/* Compute deltas and evaluate functions at the top scanline */
	e->du = u1->x - u0->x;	e->u = u0->x + FskFracMul(e->du, fr);
	e->dv = u1->y - u0->y;	e->v = u0->y + FskFracMul(e->dv, fr);

	if (dy > 0x8000) {												/* Compute derivatives if dy is not close to zero */
		e->dx = FskFixDiv(e->dx, dy);
		e->du = FskFixDiv(e->du, dy);
		e->dv = FskFixDiv(e->dv, dy);
	}

	e->span	= span;
	e->next	= NULL;

	return 1;
}


/********************************************************************************
 * TransformClipAndMakeAffineEdges
 ********************************************************************************/

static int
TransformClipAndMakeAffineEdges(
	int				numPts,
	/*const*/float	(*pts)[2],
	/*const*/float	M[3][3],
	const KPRect	*clipRect,
	Span			*span,
	Edge			*edges		/* Clip temp is tacked onto the end of the edges */
)
{
	KPDVector3D		*xyz		= NULL;
	FskFixedPoint2D	*uv;
	FskFixedPoint2D	*pu, *lu;
	FskFixedPoint2D	*pp, *lp;
	int				n, numEdges;
	double			DM[3][3];

	/* Extract storage for xyz and uvwfrom the end of the edges */
	numEdges = MAX_CLIP_POINTS(numPts);								/* Clipping introduces more points */
	xyz      = (KPDVector3D*)ALIGNTO(edges + numEdges, double);		/* After the end of the edges is clipping storage for xyz ... */
	uv       = (FskFixedPoint2D*)(   xyz   + numEdges);				/* ... and uvw */

	/* Transform points */
	KPCopySingleToDoubleMatrix(M[0], DM[0], 3, 3);					/* Double is needed to get the full 32-bit precision for the scan converter, because single only has 24 */
	TransformAffinePoints(numPts, pts, DM, xyz);					/* Affine transforms Euclidean xy to homogeneous xyz @@@ Z is not really needed @@@ */

	/* Clip points */
	n = numPts;
	KPDHClipPolygon(&n, xyz, (KPDVector3D*)uv, clipRect);			/* Homogeneous clipping, using uv as temporary storage @@@ we should use Barsky-Liang clipping for speed @@@ */
	HomogeneousAffineToEuclideanPoints(xyz, n);						/* Homogeneous xy1 projected back to Euclidean xy in-place @@@ with Barsky-Liang, we wouldn't need this @@@ */

	/* Scale matrix and points so as to have the maximum resolution (30+ bits) for the scan converter */
	SetFixedAffineMatrixAndPoints(DM[0], n, (KPDPoint2D*)xyz, span, uv);	/* xy and uvw are converted from floating- to fixed-point in-place */

	for (numEdges = 0, lp = (pp = (FskFixedPoint2D*)xyz) + n - 1, lu = (pu = uv) + n - 1; n--; lp = pp++, lu = pu++) {
		if (InitAffineEdge(lp, pp, lu, pu, span, edges)) {
			edges++;
			numEdges++;
		}
	}

	return numEdges;
}


/********************************************************************************
 * MakeAffineEdgesForImage3D
 ********************************************************************************/

static int
MakeAffineEdgesForImage3D(const KPImage3D *src, KPImage3D *dst, const KPRect *dstClip, Span *span, int quality, Edge *edges)
{
	int				numEdges			= 0;
	int				numPts;
	/*const*/float	(*pts)[2];
	float			rectPts[4][2];
	float			M[3][3];

	if ((span->baseAddr = src->baseAddr) == NULL)		/* Something went wrong with initializing the image -- abort early */
		goto bail;
	span->rowBytes	= src->rowBytes;
	span->width		= src->width;
	span->height	= src->height;
	if ((src->numPts > 0) && (src->pts != NULL)) {		/* Arbitrary polygon */
		numPts = src->numPts;
		pts    = src->pts;
	}
	else {												/* No points -- use the whole rectangle */
		MakeRectPolygon(src->width, src->height, rectPts);
		numPts = 4;
		pts    = rectPts;
	}

	#if defined(__ARMCC_VERSION)		/* Only 16RGB565LE accommodated for Palm, so we dispatch inline rather than in a table */
		#if TARGET_RT_LITTLE_ENDIAN
			span->fill = (quality == 0) ? AffineTexSpan16RGB565SE16RGB565SE : BilerpAffineTexSpan16RGB565SE16RGB565SE;
		#else /* TARGET_RT_BIG_ENDIAN */
			span->fill = (quality == 0) ? AffineTexSpan16RGB565DE16RGB565DE : BilerpAffineTexSpan16RGB565DE16RGB565DE;
		#endif /* TARGET_RT_BIG_ENDIAN */
	#else /* !__ARMCC_VERSION */							/* We dispatch through a table */
		span->fill		= affineFillProcs[src->pixelFormat][dst->pixelFormat][quality];
	#endif /* !__ARMCC_VERSION */
	KPSLinearTransform(src->M[0], dst->M[0], M[0], 3, 3, 3);
	if (M[2][2] != 1.0f)
		KPSScaleMatrix(1.0f / M[2][2], M[0], M[0], 3, 3);	/* Normalize affine matrix */

	numEdges = TransformClipAndMakeAffineEdges(numPts, pts, M, dstClip, span, edges);

bail:
	return numEdges;
}


/********************************************************************************
 * ScanConvertAffineEdges
 ********************************************************************************/

static void
ScanConvertAffineEdges(int numEdges, Edge *edges, KPImage3D *dst)
{
	Edge	*activeEdges, *e;
	int		y, nextBorn, nextExpired, nextEvent;
	Span	*span;
	char	*dstBaseAddr			= dst->baseAddr;
	long	dstRowBytes				= dst->rowBytes;
#if defined(__ARMCC_VERSION)				/* Only 16RGB565LE accommodated for Palm */
	const int	dstPixelBytes		= 2;
#else
	const int	dstPixelBytes		= myPixelSize[dst->pixelFormat];
#endif


	/* Initialize edge list */
	LinkEdges(numEdges, edges);
	YSortEdges(&edges);
	RemoveDegenerateEdges(&edges);

	if (edges == NULL)
		return;

	/* Initialize active edges */
	y = edges->top;
	activeEdges = NULL;
	while ((edges != NULL) || (activeEdges != NULL)) {

		/* Remove expired edges */
		{	register Edge **lastNext;
			for (lastNext = &activeEdges; *lastNext != NULL; ) {
				e = *lastNext;
				if (y >= e->bottom)																/* Edge is expired: remove */
					*lastNext = e->next;
				else																			/* Edge still active: look at the next one */
					lastNext = &e->next;
			}
		}

		/* Collect newborn edges */
		while ((edges != NULL) && (edges->top <= y)) {											/* Should ALWAYS be == */
			e = edges;
			edges = e->next;
			XSortInsertEdge(e, &activeEdges);
		}
		nextBorn = (int)((edges == NULL) ? INT_MAX : edges->top);								/* Since edges are sorted, the next edge birth is the next pending edge */

		/* Determine the next edge expiration */
		if (activeEdges == NULL) {																/* All edges accounted for */
			nextExpired = INT_MAX;
		}
		else {																					/* Look for the next edge to expire */
			for (nextExpired = activeEdges->bottom, e = activeEdges->next; e != NULL; e = e->next)
				if (nextExpired > e->bottom)
					nextExpired = e->bottom;
		}

		nextEvent = (nextBorn < nextExpired) ? nextBorn : nextExpired;							/* Stop at the next event, whether birth or expiration of an edge */

		/* Make sure that pairs of edges are from the same polygon */
		if (PairEdges(activeEdges) == 0)
			return;																				/* Numerical inconsistencies forces us to abort */

		/* Scan all active trapezoids until the next event */
		if (nextEvent < INT_MAX) {
			register Edge *L, *R;
			int dy = nextEvent - y;

			/* Render all polygons in scanline order */
			while (dy--) {
				#ifdef EXPECT_CROSSOVERS
					XSortEdges(&activeEdges);													/* We need to sort every line if we expect crossovers */
					PairEdges(activeEdges);														/* Resorting might unpair edges, so we need to repair */
				#endif /* EXPECT_CROSSOVERS */
				for (L = activeEdges; (L != NULL) && ((R = L->next) != NULL); L = R->next) {
					int left, right;
					FskFixed subx;

					#ifndef EXPECT_CROSSOVERS
						if (R->x < L->x) {														/* Edges crossed over */
							L->next = R->next;													/* Swap L and R */
							R->next = L;
							*PreviousNextEdge(&activeEdges, L) = R;
							L = R;
							R = L->next;
						}
					#endif /* !EXPECT_CROSSOVERS */

					left  = CeilFixedToInt(L->x);												/* Compute span width */
					subx  = IntToFixed(left) - L->x;
					right = FloorFixedToInt(R->x);
					span  = L->span;
					if ((span->dx = right - left + 1) > 0) {									/* If the span is at least one pixel */
						span->u = L->u + FskFixMul(subx, span->M[0][0]);						/* Compute texture coordinates */
						span->v = L->v + FskFixMul(subx, span->M[0][1]);
						span->p = dstBaseAddr + (int)(L->top) * dstRowBytes + left * dstPixelBytes;	/* Set dst pointer to first pixel in span */
						L->top++; L->x += L->dx; L->u += L->du; L->v += L->dv;					/* Update edges while still in the cache */
						R->top++; R->x += R->dx; R->u += R->du; R->v += R->dv;
						(*span->fill)(span);
					}
					else {
						L->top++; L->x += L->dx; L->u += L->du; L->v += L->dv;					/* Update edges */
						R->top++; R->x += R->dx; R->u += R->du; R->v += R->dv;
					}
				}
			}
			y = nextEvent;
		}
		else
			break;
	}
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
 * KPProjectImages
 ********************************************************************************/

#if !defined(SUPPORT_QTVR_ARMLET) || (defined (SUPPORT_QTVR_ARMLET) && !SUPPORT_QTVR_ARMLET)

int
KPProjectImages(
#if defined(__ARMCC_VERSION)
			PalmState		state,
#endif
			int numSrcs, const KPImage3D *src, KPImage3D *dst, int qualFlag)
{
	int		numEdges, i, maxPolyClip;
	Edge	*edges;
	Edge	*edge;
	Span	*span;
	KPRect	clipRect;
	Span	*spans		= NULL;
	int		status		= 0;
	int		quality		= (qualFlag >> kpQualityPosition       ) & ((1L << kpQualityBits       ) - 1);	/* Extract quality */
	int		xfmType		= (qualFlag >> kpTransformationPosition) & ((1L << kpTransformationBits) - 1);	/* Extract transformation type */

	BAIL_IF_NULL(dst->baseAddr, status, -2);							/* Make sure there's something we can write to */

	/* Determine parameters for memory allocation, so we can do memory allocation only once */
	numEdges	= CountPointsInImages3D(numSrcs, src, &maxPolyClip);	/* The number of points: total and maximum per polygon */
	numEdges	= MAX_CLIP_POINTS(numEdges) + 1;						/* The maximum number of edges, plus one for alignment */
	maxPolyClip	= MAX_CLIP_POINTS(maxPolyClip);							/* The maximum number of clipped points in any one polygon */
	i			= numSrcs     * sizeof(Span)							/* One span for each source */
				+ numEdges    * sizeof(Edge)							/* The total number of edges */
				+ maxPolyClip * sizeof(KPDVector3D) * 2;				/* Clipped xyz and clipped uvw */

	/* Allocate spans, edges, and temporary clipped xyz and uvw */
	BAIL_IF_NULL((spans = (Span*)KPAlloc(i)), status, -1);				/* We do one alloc for spans ... */
	edges = (Edge*)(spans + numSrcs);									/* ... and edges, and the temporary memory used for clipping */

	/* Get destination clip */
	BoundingRectOfImage3D(dst, &clipRect);

	#ifdef TEST_AFFINE
		xfmType = 1;
	#endif /* TEST_AFFINE */

	if (xfmType == (kpTransformationPerspective >> kpTransformationPosition)) {		/* Projective */
		/* Convert points to edges */
		for (numEdges = 0, edge = edges, span = spans; numSrcs--; src++, span++) {
			if ((i = MakeProjectiveEdgesForImage3D(src, dst, &clipRect, span, quality, edge)) >= 0) {
				numEdges += i;
				edge     += i;
			}
			else {
				status = -1;
				goto bail;
			}
		}

		/* Scan-convert edges */
		ScanConvertProjectiveEdges(numEdges, edges, dst);
	}
	else {																			/* Affine */
		/* Convert points to edges */
		for (numEdges = 0, edge = edges, span = spans; numSrcs--; src++, span++) {
			if ((i = MakeAffineEdgesForImage3D(src, dst, &clipRect, span, quality, edge)) >= 0) {
				numEdges += i;
				edge     += i;
			}
			else {
				status = -1;
				goto bail;
			}
		}

		/* Scan-convert edges */
		ScanConvertAffineEdges(numEdges, edges, dst);
	}

bail:
	if (spans != NULL)	KPFree(spans);						/* One free to free spans and edges */
	return	status;
}

#endif

/********************************************************************************
 * KPProjectImage
 ********************************************************************************/

int
KPProjectImage(
	/* Source */
	const void	*srcBaseAddr,
	int			srcPixelFormat,
	long		srcRowBytes,
	int			srcWidth,
	int			srcHeight,

	/* Transformation */
	const float	M[3][3],

	/* Source clip */
	int			srcNumPts,			/* If numPts==0 then use default rectangle */
	const float	(*srcPts)[2],		/* Arbitrary polygon */

	int			qualFlag,

	/* Destination */
	void		*dstBaseAddr,
	int			dstPixelFormat,
	long		dstRowBytes,
	int			dstWidth,
	int			dstHeight,

	/* Destination clip */
	int			dstNumPts,			/* If numPts==0 then use default rectangle */
	const float	(*dstPts)[2]		/* Arbitrary polygon */
)
{
	KPImage3D src, dst;

	src.baseAddr	= (void*)srcBaseAddr;	/* We need to cast this as non-const in order to assign it */
	src.pixelFormat	= srcPixelFormat;
	src.rowBytes	= srcRowBytes;
	src.width		= srcWidth;
	src.height		= srcHeight;
	src.numPts		= srcNumPts;
	src.pts			= (float(*)[2])srcPts;	/* We need to cast this as non-const in order to assign it */
	KPSCopyMatrix(M[0], src.M[0], 3, 3);

	dst.baseAddr	= dstBaseAddr;
	dst.pixelFormat	= dstPixelFormat;
	dst.rowBytes	= dstRowBytes;
	dst.width		= dstWidth;
	dst.height		= dstHeight;
	dst.numPts		= dstNumPts;
	dst.pts			= (float(*)[2])dstPts;	/* We need to cast this as non-const in order to assign it */
	KPSIdentityMatrix(dst.M[0], 3, 3);

	return KPProjectImages(
#if defined(__ARMCC_VERSION)
					NULL,
#endif
					1, &src, &dst, qualFlag);
}


