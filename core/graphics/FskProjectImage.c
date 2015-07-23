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
	\file	FskProjectImage.c
	\brief	Transformation of an image using a 3x3 matrix.
*/
#if defined(__ARMCC_VERSION)
	#include "ArmTo68kGlue.h"
#endif

#include <stdlib.h>
#include <limits.h>
#include "FskProjectImage.h"
#include "Fsk.h"
#include "FskDHClipPolygon.h"
#include "FskFixedMath.h"
#include "FskMatrix.h"
#include "FskMemory.h"
#include "FskPixelOps.h"

//#define DUMP_PANO
#ifdef DUMP_PANO
	#include "FskFiles.h"
	#include "FskImage.h"
#endif /* DUMP_PANO */

/* 	In order to enable debug output on Android, it is necessary to:
 *	(1) #define SUPPORT_INSTRUMENTATION 1 in FskPlatform.android.h
 *	(2) put this into manifest.xml:
 *			<instrument platform="android" androidlog="true" threads="true" trace="false">
 *				<kind name="projectbitmap" messages="debug"/>
 *			</instrument>
 *			<instrument platform="mac" threads="true" trace="true">
 *				<kind name="projectbitmap" messages="debug"/>
 *			</instrument>
 *		On android, you can see the output by executing "adb logcat" on Android,
 *		while on the Mac, the output will go to stdout, but
 *	(3) Perhaps #define LOG_PARAMETERS or GL_DEBUG
 *	(4) fskbuild --clean [ --run ] [ --test ]
 *	(5a) To view the output on Android, type "adb logcat" in the host (Mac or Linux) Terminal window.
 *	(5b) To view the output on the Mac, you can
 *	     - launch from XCode, and the output will be displayed in the console window.
 *	     - launch from Terminal with:
 *	     	cd ${F_HOME}/bin/mac/debug/Kinoma Simulator.app/Contents/MacOS"
 *	     	./fsk"
 *	       and you will be able to see the output in the Terminal window.
 */


//#define LOG_EDGES				/**< Log the edges produced for each polygon processed. */
#define LOG_QFD_OVERFLOW		/**< Log any kind of overflow detected in the quadratic forward differencing. */
//#define LOG_BOUNDS_VIOLATIONS	/**< Log accesses outside the source bounds. */

#if SUPPORT_INSTRUMENTATION
	extern FskInstrumentedTypeRecord gProjectBitmapTypeInstrumentation;	 /* is in FskPerspective.c */
    FskInstrumentedTypePrintfsDefine(ProjectBitmap, gProjectBitmapTypeInstrumentation);
	#if	defined(LOG_EDGES)				|| \
		defined(LOG_QFD_OVERFLOW)		|| \
		defined(LOG_BOUNDS_VIOLATIONS)
			#define PERSPECTIVE_DEBUG	1
	#else /* LOG */
		#define PERSPECTIVE_DEBUG	0
	#endif /* LOG */
#else /* SUPPORT_INSTRUMENTATION */
	#define	PERSPECTIVE_DEBUG		0
#endif /* SUPPORT_INSTRUMENTATION */

#if !PERSPECTIVE_DEBUG
	#undef LOG_EDGES
	#undef LOG_QFD_OVERFLOW
	#undef LOG_BOUNDS_VIOLATIONS
#endif /* !PERSPECTIVE_DEBUG */

#if PERSPECTIVE_DEBUG
	#define	LOGD(...)	FskProjectBitmapPrintfDebug  (__VA_ARGS__)
	#define	LOGI(...)	FskProjectBitmapPrintfVerbose(__VA_ARGS__)
#endif /* PERSPECTIVE_DEBUG */
#define		LOGE(...)	FskProjectBitmapPrintfMinimal(__VA_ARGS__)
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
	static unsigned int		maxLength	= 0;
	static double			avgLength	= 0;
	static unsigned int		numSpans	= 0;
#endif /* GATHER_STATISTICS */


/* This checks for either 0x7FFFFFFF or 0x80000000 */
#define FIXED_OVERFLOW(x) (((UInt32)((x) - 0x7FFFFFFF)) <= 1)


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
	Edge			*next;					/**< Linked list of edges */
	Span			*span;					/**< The data structure used in filling spans */
	short			top, bottom;			/**< The activation and expiration Y of the edge */
	FskFixed		x, dx;					/**< The location of the edge on the current scanline, and its derivative w.r.t. y */
	FskFixed		u,  v,  w;				/**< FskFixed u; FskFixed v; FskFixed24 w; */
	FskFixed		du, dv, dw;				/**< Deltas w.r.t. the edge */
};

struct Span {
	const void			*baseAddr;				/**< The source texture map base address */
	int					rowBytes;				/**< The source texture map Y stride in bytes */
	int					width;					/**< The source texture map width */
	int					height;					/**< The source texture map height */

	FskFixed			M[3][3];				/**< FskFixed u; FskFixed v; FskFixed24 w; */
	FillSpanProc		fill;					/**< The fill span proc */

	void				*p;						/**< The pointer to the initial destination pixel */
	int					dx;						/**< The length of the span */
	int					qdx;					/**< Subspan for quadratic forward differencing */

	FskFixed			u, v, w;				/**< Current homogeneous texture coordinates */

	FskFixed			u0, u1, u2, v0, v1, v2;	/**< Forward differencing coefficients */
	FskFixed			un, vn;					/**< u and v at the end of the span */

	int					wFracBits;				/**< The number of fractional bits for w in canonical form */

	union {
		FskColorRGBARecord	rgba;
		UInt32				p32;
		UInt16				p16;
		UInt8				p8;
		Fsk24BitType		p24;
	}					opColor;				/**< Color used for the colorize operation. */
	UInt8				blendLevel;				/**< Blend level. */
	UInt8				isPremul;				/**< Whether the color in the src is premultiplied. */
//#define FLOAT_QFD_SCALE
#ifdef FLOAT_QFD_SCALE
	float				qfdScale;				/**< Scale for computing the interval length for quadratic forward differencing */
#else /* FLOAT_QFD_SCALE */
	FskFixed			qfdScale;				/**< Scale for computing the interval length for quadratic forward differencing */
	int					qfdScaleShift;			/**< [Negated] exponent of the product qfdScale * w^(4/3) */
#endif /* FLOAT_QFD_SCALE */
};


/********************************************************************************
 *****								BOUNDS_CHECK							*****
 ********************************************************************************/
#if defined(LOG_BOUNDS_VIOLATIONS)
	#define VIOLATION_COLOR		0xF81FF81F	/* 565 magenta x 2 */
	static void SrcBoundsViolation(unsigned int i, unsigned int j, Span *span, void *d, int pixelBytes)
	{
		static const int	violationColor = VIOLATION_COLOR;
		unsigned char *p;	const unsigned char *q;
		for (p = d, q = (const unsigned char*)(&violationColor); pixelBytes--; )
			*p++ = *q++;
		LOGD("BoundsCheck: (%3d, %3d) not in [%3d, %3d], span=%p", i, j, span->width, span->height, span);
	}
	#define CHECK_SRC_BOUNDS	if ((i >= (unsigned int)(span->width)) || (j >= (unsigned int)(span->height))) {	\
									SrcBoundsViolation(i, j, span, d, FskName3(fsk,DstPixelKind,Bytes));			\
								} else
	#define CHECK_SRC_SP_BOUNDS	if ( ((i >= (unsigned int)(span->width  - 1)) && ((i >= (unsigned int)(span->width)  || uf != 0)))		\
								||   ((j >= (unsigned int)(span->height - 1)) && ((j >= (unsigned int)(span->height) || vf != 0))) ) {	\
									SrcBoundsViolation(i, j, span, d, FskName3(fsk,DstPixelKind,Bytes));									\
								} else
#else /* !LOG_BOUNDS_VIOLATIONS */
#define CHECK_SRC_BOUNDS	/* nothing */
#define CHECK_SRC_SP_BOUNDS	/* nothing */
#endif /* !LOG_BOUNDS_VIOLATIONS */

#ifdef FSK_GATHER_COORDINATE_SIGNATURE
	struct FskSequenceSignatureStateRecord;
	FskAPI(struct FskSequenceSignatureStateRecord *) gFskSequenceSignature = NULL;
	typedef FskErr (*SequenceSignatureAddBytesProc)(struct FskSequenceSignatureStateRecord *st, const void *bytes, UInt32 numBytes);
	FskAPI(SequenceSignatureAddBytesProc) gSequenceSignatureAddBytesProc = NULL;
#endif /* FSK_GATHER_COORDINATE_SIGNATURE */


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
 *****																		*****
 ***** ERROR ANALYSIS														*****
 ***** Evaluating the coordinates from the QFD coefficients directly rather *****
 ***** than incrementally yields:											*****
 *****		u = u0 + (n * u1 + (n-1)*n/2 * u2) / Q							*****
 ***** where																*****
 *****		Q = (1 << QFD_EXTRA_BITS)										*****
 ***** The error is then													*****
 *****		E(n) = e * (2Q + n + n^2) / 2Q									*****
 ***** where																*****
 *****		e = 2^(-17)														*****
 ***** is the error due to rounding to 16.16 fixed point precision. For		*****
 ***** bilinear interpolation, which uses 4 fractional bits, we would like	*****
 *****		E < 2^(-5)														*****
 ***** Using these values, we have
 *****		2^(-5) = (n + n^2 + 2*Q) * 2^(-17) / (2 * Q)
 ***** 		1/32  =  (n + n^2 + 2*Q) / (262144 * Q)
 *****		Q = n * (n + 1) / 8190
 *****		n = (-1 + sqrt(1 + 32760 * Q)) / 2
 *****		n ~= 3 * sqrt(910) * sqrt(Q) - 0.5
 *****		n ~= sqrt(Q) * 90.5 - 0.5
 ***** If we choose a maximum length of
 *****		n = 512, then Q = 32.0703, or
 *****		Q =  64, then n = 723.489.
 *****		Q =  32, then n = 511.437
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define USE_SPAN_FORWARD_DIFFERENCING


#define THEORETICAL_MAXIMUM_TOLERANCE
#define QFD_TOLERANCE		0.125f						/* 1/8 pixel tolerance */
#define QFD_MIN_LENGTH		8							/* Minimum length of a QFD span */
#define QFD_EXTRA_BITS		6
//#define QFD_EXTRA_BITS		10
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
	FskFixed wOne = (1 << wFracBits);

	w = (w >= wOne) ? (FskFixedNMul((w - wOne), FRACT_FOUR_THIRDS, 30) + wOne)
					: (FskFixedNMul((w + wOne), w, wFracBits + 1));
	return w;
}


#if defined(LOG_QFD_OVERFLOW)
/**	Determine whether the sum has overflowed.
 *	We only need the signs for the first two arguments.
 *	\param[in]	a	one       addend, or actually any number that has its same sign.
 *	\param[in]	b	the other addend, or actually any number that has its same sign.
 *	\param[in]	sum	the sum.
 *	\return		true if the sum has overflowed, false otherwise.
 */
static Boolean FixedSumHasOverflowed(FskFixed a, FskFixed b, FskFixed sum) {
	return ((sum ^ a) & (sum ^ b)) < 0;
}
static Boolean FixedSumWillOverflow(FskFixed a, FskFixed b) {
	return FixedSumHasOverflowed(a, b, a + b);
}
#endif /* LOG_QFD_OVERFLOW */

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
		span->u1 = FskFixedNDiv(span->M[0][0], span->w, 24 + QFD_EXTRA_BITS);		span->u2 = 0;
		span->v1 = FskFixedNDiv(span->M[0][1], span->w, 24 + QFD_EXTRA_BITS);		span->v2 = 0;
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
		um       = FskFixedNDiv(um, wm, 24);		vm = FskFixedNDiv(vm, wm, 24);
		span->un = FskFixedNDiv(span->un, wn, 24);	span->vn = FskFixedNDiv(span->vn, wn, 24);
		/* Compute quadratic forward difference coefficients
		 *	| u0 |   |  1   0     0    |  |  1   0   0  |  | L |
		 *	| u1 | = |  0  1/N  1/N^2  |  | -3   4  -1  |  | M |
		 *	| u2 |   |  0   0   1/N^2  |  |  1  -2   1  |  | R |
		 */
		t = span->qdx * span->qdx;
		span->u2 = FskFixedNDiv((span->u0 - (um << 1) + span->un),              t,         QFD_EXTRA_BITS + 2);		/* u2 = (u0 - 2 * um + un) * 4 / (n * n) */
		span->v2 = FskFixedNDiv((span->v0 - (vm << 1) + span->vn),              t,         QFD_EXTRA_BITS + 2);		/* v2 = (v0 - 2 * vm + vn) * 4 / (n * n) */
		span->u1 = FskFixedNDiv((((um - span->u0) << 2) + span->u0 - span->un), span->qdx, QFD_EXTRA_BITS);			/* u1 = (4 * um - 3 * u0 - un) / n + u2 / 2 */
		span->v1 = FskFixedNDiv((((vm - span->v0) << 2) + span->v0 - span->vn), span->qdx, QFD_EXTRA_BITS);			/* v1 = (4 * vm - 3 * v0 - vn) / n + v2 / 2 */
		#if defined(LOG_QFD_OVERFLOW)
		{	FskFixed fx;
			float fl;
			if (FIXED_OVERFLOW(span->u1)) {
				fx = FskFixedNDiv(span->u - span->M[0][0], span->w - span->M[0][2], 8+16);
				LOGD("u1 overflowed. u0 was given as %f, but should have been %f", span->u0/65536., fx/65536.);
			}
			if (FIXED_OVERFLOW(span->v1)) {
				fx = FskFixedNDiv(span->v - span->M[0][1], span->w - span->M[0][2], 8+16);
				LOGD("v1 overflowed. v0 was given as %f, but should have been %f", span->v0/65536., fx/65536.);
			}
			if (FIXED_OVERFLOW(span->u2)) {
				fl = (span->u0 - 2.f * um + span->un) * 4 / t * (1 << QFD_EXTRA_BITS);
				LOGD("u2 overflowed. Computed as %f, but should have been %f", span->u2/(float)(1 << (16 + QFD_EXTRA_BITS)), fl/(float)(1 << (16 + QFD_EXTRA_BITS)));
			}
			if (FIXED_OVERFLOW(span->v2)) {
				fl = (span->v0 - 2.f * vm + span->vn) * 4 / t * (1 << QFD_EXTRA_BITS);
				LOGD("v2 overflowed. Computed as %f, but should have been %f", span->v2/(float)(1 << (16 + QFD_EXTRA_BITS)), fl/(float)(1 << (16 + QFD_EXTRA_BITS)));
			}
			if (FixedSumWillOverflow(span->u1, span->u2 >> 1))
				LOGD("u1+u2/2 overflow");
			if (FixedSumWillOverflow(span->v1, span->v2 >> 1))
				LOGD("v1+v2/2 overflow");
		}
		#endif /* LOG_QFD_OVERFLOW */
		span->u1 += (span->u2 >> 1);   																				/* Finish the above computation ... */
		span->v1 += (span->v2 >> 1);  																				/* ... after overflow check */
		#if defined(LOG_QFD_OVERFLOW)
		{	FskInt64 s0, s1, s2;
			static FskFixed maxErr = 0;

			s2 = (FskInt64)(span->qdx) * span->u2 + span->u1;														/*      u1 + n         * u2           */
			s1 = (FskInt64)(span->qdx) * span->u1 + (FskInt64)(span->qdx) * (span->qdx-1) / 2 * span->u2;			/*  n * u1 + n*(n-1)/2 * u2           */
			s0 = (FskInt64)(span->u0) + (s1 >> QFD_EXTRA_BITS);														/* (n * u1 + n*(n-1)/2 * u2) / Q + u0 */
			if (     FixedSumHasOverflowed(span->u1, span->u2, (FskFixed)s2))
				LOGD("u1 + n * u2 overflows: is %08lX, should be %llX @ n=%d", (FskFixed)s2, s2, span->qdx);
			else if (FixedSumHasOverflowed(span->u1, span->u2, (FskFixed)s1))
				LOGD("u1 * n + u2 * n*(n-1)/2 overflows: is %08lX, should be %llx @ n=%d", (FskFixed)s1, s1);
			else if (FixedSumHasOverflowed(span->u0, (FskFixed)(s1 >> QFD_EXTRA_BITS), (FskFixed)s0))
				LOGD("u0 + (u1 * n + u2 * n*(n-1)/2) / Q overflows: is %08lX, should be %llx @ n=%d", (FskFixed)s0, s0);

			s2 = (FskInt64)(span->qdx) * span->v2 + span->v1;														/*      v1 + n         * v2           */
			s1 = (FskInt64)(span->qdx) * span->v1 + (FskInt64)(span->qdx) * (span->qdx-1) / 2 * span->v2;			/*  n * v1 + n*(n-1)/2 * v2           */
			s0 = (FskInt64)(span->v0) + (s1 >> QFD_EXTRA_BITS);														/* (n * v1 + n*(n-1)/2 * v2) / Q + v0 */
			if (     FixedSumHasOverflowed(span->v1, span->v2, (FskFixed)s2))
				LOGD("v1 + n * v2 overflows: is %08lX, should be %llX @ n=%d", (FskFixed)s2, s2, span->qdx);
			else if (FixedSumHasOverflowed(span->v1, span->v2, (FskFixed)s1))
				LOGD("v1 * n + v2 * n*(n-1)/2 overflows: is %08lX, should be %llx @ n=%d", (FskFixed)s1, s1);
			else if (FixedSumHasOverflowed(span->v0, (FskFixed)(s1 >> QFD_EXTRA_BITS), (FskFixed)s0))
				LOGD("v0 + (v1 * n + v2 * n*(n-1)/2) / Q overflows: is %08lX, should be %llx @ n=%d", (FskFixed)s0, s0);

			for (t = 1; t <= span->qdx; ++t) {
				FskFixed ut = span->u + span->M[0][0] * t;
				FskFixed vt = span->v + span->M[0][1] * t;
				FskFixed wt = span->w + span->M[0][2] * t;
				FskFixed ue = FskFixedNDiv(ut, wt, 8+16);
				FskFixed ve = FskFixedNDiv(vt, wt, 8+16);
				FskFixed uq = span->u0 + ((t * span->u1 + ((t * (t - 1)) >> 1) * span->u2) >> QFD_EXTRA_BITS);
				FskFixed vq = span->v0 + ((t * span->v1 + ((t * (t - 1)) >> 1) * span->v2) >> QFD_EXTRA_BITS);
				um = abs(uq - ue);
				if (um > 0x8000)
					LOGD("QFD u=%.2f != PROJ u=%.2f diff=%.2f @ t=%d", uq/65536., ue/65536., um/65536., t);
				//else
				//	LOGD("QFD u=%.2f ~= PROJ u=%.2f @ t=%d", uq/65536., ue/65536., t);
				if (maxErr < um) {
					maxErr = um;
					LOGD("maxErr = %.2f", maxErr/65536.);
				}
				vm = abs(vq - ve);
				if (vm > 0x8000)
					LOGD("QFD v=%.2f != PROJ v=%.2f diff=%.2f @ t=%d", vq/65536., ve/65536., vm/65536., t);
				//else
				//	LOGD("QFD v=%.2f ~= PROJ v=%.2f @ t=%d", vq/65536., ve/65536., t);
				if (maxErr < vm) {
					maxErr = vm;
					LOGD("maxErr = %.2f", maxErr/65536.);
				}
			}
		}
		#endif /* LOG_QFD_OVERFLOW */

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


#ifdef SRC_YUV420
	#undef SRC_YUV420
	#define SRC_YUV420 0
#endif /* SRC_YUV420 */
#ifdef DST_YUV420
	#undef DST_YUV420
	#define DST_YUV420 0
#endif /* DST_YUV420 */
#ifdef SRC_YUV420i
	#undef SRC_YUV420i
	#define SRC_YUV420i 0
#endif /* SRC_YUV420i */

#include "FskBlitDispatchDef.h"

#define GENERAL_BLIT_PROTO_FILE "FskProjectiveSpan.c"
#include "FskBlitSrcDst.h"





/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***		Dispatcher
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * These make the dispatch table manageable
 ********************************************************************************/

#define NUM_OPS 					4
#define OP_0						Copy
#define OP_1						Blend
#define OP_2						Alpha
#define OP_3						AlphaBlend
#define OP_4						TintCopy
#define OP_INDEX_COPY				0
#define OP_INDEX_BLEND				1
#define OP_INDEX_ALPHA				2
#define OP_INDEX_ALPHABLEND			3
#define OP_INDEX_TINTCOPY			4

#define NUM_QUALS					2
#define QUAL_0						Fsk
#define QUAL_1						FskBilinear


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
 * ModeToProcTableOpIndex
 ********************************************************************************/

static SInt32
ModeToProcTableOpIndex(UInt32 mode, FskConstGraphicsModeParameters modeParams, FskConstColorRGBA opColor, const FskImage3D *dst, Span *span)
{
	SInt32 index = -1;

	span->blendLevel = (UInt8)((modeParams && (UInt32)(modeParams->blendLevel) < 255) ? modeParams->blendLevel : 255);
	span->isPremul   = (mode & kFskSrcIsPremul) ? 1 : 0;

	switch (mode & kFskGraphicsModeMask) {
		case kFskGraphicsModeColorize:
			if (opColor) {
				index = OP_INDEX_TINTCOPY;
				FskConvertColorRGBAToBitmapPixel(opColor, dst->pixelFormat, &span->opColor.p32);
				break;
			}
			/* If no color was supplied, fall through and use copy mode */
		case kFskGraphicsModeCopy:
			index = (span->blendLevel == 255) ? OP_INDEX_COPY  : OP_INDEX_BLEND;
			break;
		case kFskGraphicsModeAlpha:
			index = (span->blendLevel == 255) ? OP_INDEX_ALPHA : OP_INDEX_ALPHABLEND;
			break;
	}
	if (index < 0 || index >= NUM_OPS)
		index = OP_INDEX_COPY;

	return index;
}


/********************************************************************************
 * Dispatch table: dst, src, op, qual
 ********************************************************************************/

#define P_DSOQ(d,s,o,q) 	FskName4(q,o,s,d)
#define P_DSO(d,s,o) 		{ FskName4(ProjectiveTexSpan,o,s,d),FskName4(BilerpProjectiveTexSpan,o,s,d) }
#define P_DS(d,s)			{ P_DSO(d,s,OP_0),P_DSO(d,s,OP_1),P_DSO(d,s,OP_2),P_DSO(d,s,OP_3) }
#define A_DSOQ(d,s,o,q) 	FskName4(q,o,s,d)
#define A_DSO(d,s,o) 		{ FskName4(AffineTexSpan,o,s,d),FskName4(BilerpAffineTexSpan,o,s,d) }
#define A_DS(d,s)			{ A_DSO(d,s,OP_0),A_DSO(d,s,OP_1),A_DSO(d,s,OP_2),A_DSO(d,s,OP_3) }

#if   NUM_SRC_FORMATS == 1
	#define P_D(d)		{ P_DS(d,SRC_KIND_0) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0) }
#elif NUM_SRC_FORMATS == 2
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1) } }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1) } }
#elif NUM_SRC_FORMATS == 3
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2) }
#elif NUM_SRC_FORMATS == 4
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3) }
#elif NUM_SRC_FORMATS == 5
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4) }
#elif NUM_SRC_FORMATS == 6
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5) }
#elif NUM_SRC_FORMATS == 7
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6) }
#elif NUM_SRC_FORMATS == 8
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7) }
#elif NUM_SRC_FORMATS == 9
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8) }
#elif NUM_SRC_FORMATS == 10
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9) }
#elif NUM_SRC_FORMATS == 11
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10) }
#elif NUM_SRC_FORMATS == 12
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11) }
#elif NUM_SRC_FORMATS == 13
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12) }
#elif NUM_SRC_FORMATS == 14
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13) }
#elif NUM_SRC_FORMATS == 15
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14) }
#elif NUM_SRC_FORMATS == 16
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14),A_DS(d,SRC_KIND_15) }
#elif NUM_SRC_FORMATS == 17
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14),A_DS(d,SRC_KIND_15),A_DS(d,SRC_KIND_16) }
#elif NUM_SRC_FORMATS == 18
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14),A_DS(d,SRC_KIND_15),A_DS(d,SRC_KIND_16),A_DS(d,SRC_KIND_17) }
#elif NUM_SRC_FORMATS == 19
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17),P_DS(d,SRC_KIND_18) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14),A_DS(d,SRC_KIND_15),A_DS(d,SRC_KIND_16),A_DS(d,SRC_KIND_17),A_DS(d,SRC_KIND_18) }
#elif NUM_SRC_FORMATS == 20
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17),P_DS(d,SRC_KIND_18),P_DS(d,SRC_KIND_19) }
	#define A_D(d)		{ A_DS(d,SRC_KIND_0),A_DS(d,SRC_KIND_1),A_DS(d,SRC_KIND_2),A_DS(d,SRC_KIND_3),A_DS(d,SRC_KIND_4),A_DS(d,SRC_KIND_5),A_DS(d,SRC_KIND_6),A_DS(d,SRC_KIND_7),A_DS(d,SRC_KIND_8),A_DS(d,SRC_KIND_9),A_DS(d,SRC_KIND_10),A_DS(d,SRC_KIND_11),A_DS(d,SRC_KIND_12),A_DS(d,SRC_KIND_13),A_DS(d,SRC_KIND_14),A_DS(d,SRC_KIND_15),A_DS(d,SRC_KIND_16),A_DS(d,SRC_KIND_17),A_DS(d,SRC_KIND_18),A_DS(d,SRC_KIND_19) }
#else
	#error Unexpected large number of source pixel formats
#endif /* NUM_SRC_FORMATS */

static FillSpanProc	projectiveFillProcs[NUM_DST_FORMATS][NUM_SRC_FORMATS][NUM_OPS][NUM_QUALS] = {
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
		,P_D(DST_KIND_15)
	#endif
	#if NUM_DST_FORMATS > 16
		,P_D(DST_KIND_16)
	#endif
	#if NUM_DST_FORMATS > 17
		#error Unexpected large number of destination pixel formats
	#endif
 };

static FillSpanProc	affineFillProcs[NUM_DST_FORMATS][NUM_SRC_FORMATS][NUM_OPS][NUM_QUALS] = {
		A_D(DST_KIND_0)
	#if NUM_DST_FORMATS > 1
		,A_D(DST_KIND_1)
	#endif
	#if NUM_DST_FORMATS > 2
		,A_D(DST_KIND_2)
	#endif
	#if NUM_DST_FORMATS > 3
		,A_D(DST_KIND_3)
	#endif
	#if NUM_DST_FORMATS > 4
		,A_D(DST_KIND_4)
	#endif
	#if NUM_DST_FORMATS > 5
		,A_D(DST_KIND_5)
	#endif
	#if NUM_DST_FORMATS > 6
		,A_D(DST_KIND_6)
	#endif
	#if NUM_DST_FORMATS > 7
		,A_D(DST_KIND_7)
	#endif
	#if NUM_DST_FORMATS > 8
		,A_D(DST_KIND_8)
	#endif
	#if NUM_DST_FORMATS > 9
		,A_D(DST_KIND_9)
	#endif
	#if NUM_DST_FORMATS > 10
		,A_D(DST_KIND_10)
	#endif
	#if NUM_DST_FORMATS > 11
		,A_D(DST_KIND_11)
	#endif
	#if NUM_DST_FORMATS > 12
		,A_D(DST_KIND_12)
	#endif
	#if NUM_DST_FORMATS > 13
		,A_D(DST_KIND_13)
	#endif
	#if NUM_DST_FORMATS > 14
		,A_D(DST_KIND_14)
	#endif
	#if NUM_DST_FORMATS > 15
		,A_D(DST_KIND_15)
	#endif
	#if NUM_DST_FORMATS > 16
		,A_D(DST_KIND_16)
	#endif
	#if NUM_DST_FORMATS > 17
		#error Unexpected large number of destination pixel formats
	#endif
};



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
CountPointsInImage3D(const FskImage3D *s)
{
	return ((s->numPts > 0) && (s->pts != NULL)) ? s->numPts : 4;
}


/********************************************************************************
 * CountPointsInImages3D
 ********************************************************************************/

static int
CountPointsInImages3D(int n, const FskImage3D *s, int *maxPtsOneImage)
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
BoundingRectOfImage3D(const FskImage3D *im, FskRect *r)
{
	if ((im->numPts > 0) && (im->pts != NULL)) {		/* Clipping polygon given */
		int	n, i;
		float	(*p)[2];
		FskRect	re;

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
HomogeneousToEuclideanPoints(register FskDVector3D *v3, register int n)
{
	register FskDPoint2D	*p2;
	register double		z;

	for (p2 = (FskDPoint2D*)v3; n--; v3++, p2++) {
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
TransformProjectivePoints(int nPts, /*const*/float (*iPts)[2], /*const*/double M[3][3], FskDVector3D *oPts)
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
 * On input:	SetFixedMatrixAndPoints(const double *M, int numPts, FskDPoint2D     *xy, Span *span, FskDVector3D     *uvw);
 * On output:	SetFixedMatrixAndPoints(const double *M, int numPts, FskFixedPoint2D *xy, Span *span, FskFixedVector3D *uvw);
 * Returns 1 if successful 0 if unsuccessful.
 ********************************************************************************/

#define kScaling (30)

static int
SetFixedMatrixAndPoints(const double *M, int numPts, FskDPoint2D *xy, Span *span, FskDVector3D *uvw)
{
	double	Mi[3][3], t, uvMax, wMax, s16, s24, *d;
	int		i;
	FskFixed *x;

	FskDAdjointMatrix3x3(M, Mi[0]);	/* Don't need FskDInvertMatrix(M, 3, 3, Mi[0]) when adjoint will do, though is it as robust? */
	if (Mi[2][2] == 0)
		return 0;
	FskDScaleVector(1.0 / fabs(Mi[2][2]), Mi[0], Mi[0], 8);			/* Canonicalize matrix so that w is nominally equal to 1 */
	Mi[2][2] = (Mi[2][2] < 0) ? -1.0 : 1.0;							/* Assure that fabs9Mi[2][2] == 1 */

	/* Find range of matrix -- we might want to do something special with affine transforms */
	for (i = 3, d = Mi[0], uvMax = 0.0, wMax = 0.0; i--; ) {
		if (uvMax < (t = fabs(*d++))) uvMax = t;					/* Find the largest u or v */
		if (uvMax < (t = fabs(*d++))) uvMax = t;
		if ( wMax < (t = fabs(*d++)))  wMax = t;					/* Find the largest w */
	}

	/* Find range of uvw points */
	FskDAffineTransform(&xy->x, Mi[0], &uvw->x, numPts, 2, 3);		/* Find corresponding (u,v,w) for all (x,y) */
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
		t = p0;		p0 = p1;	p1 = (const FskFixedPoint2D*)t;		/* Swap xy's */
		t = u0;		u0 = u1;	u1 = (const FskFixedVector3D*)t;	/* Swap uvw's */
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
	const FskRect	*clipRect,
	Span			*span,
	Edge			*edges		/* Clip temp is tacked onto the end of the edges */
)
{
	FskDVector3D			*uvw, *xyz		= NULL;
	FskFixedVector3D	*pu, *lu;
	FskFixedPoint2D		*pp, *lp;
	int					n, numEdges;
	double				DM[3][3];

	/* Extract storage for xyz and uvw from the end of the edges */
	numEdges = MAX_CLIP_POINTS(numPts);								/* Clipping introduces more points */
	xyz      = (FskDVector3D*)ALIGNTO(edges + numEdges, double);	/* After the end of the edges is clipping storage for xyz ... */
	uvw      = (FskDVector3D*)(       xyz   + numEdges);			/* ... and uvw */

	/* Transform points */
	FskCopySingleToDoubleMatrix(M[0], DM[0], 3, 3);					/* Double is needed to get the full 32-bit precision for the scan converter, because single only has 24 */
	TransformProjectivePoints(numPts, pts, DM, xyz);				/* Perspective transforms Euclidean xy to homogeneous xyz */

	/* Clip points */
	n = numPts;
	FskDHClipPolygon(&n, xyz, uvw, clipRect);						/* Homogeneous clipping, using uvw as temporary storage */
	HomogeneousToEuclideanPoints(xyz, n);							/* Homogeneous xyz projected back to Euclidean xy in-place */

	/* Scale matrix and points so as to have the maximum resolution (30+ bits) for the scan converter */
	if (SetFixedMatrixAndPoints(DM[0], n, (FskDPoint2D*)xyz, span, uvw) == 0)	/* xy and uvw are converted from floating- to fixed-point in-place */
		return 0;

	for (numEdges = 0, lp = (pp = (FskFixedPoint2D*)xyz) + n - 1, lu = (pu = (FskFixedVector3D*)uvw) + n - 1; n--; lp = pp++, lu = pu++) {
		if (InitProjectiveEdge(lp, pp, lu, pu, span, edges)) {
			edges++;
			numEdges++;
		}
	}

	return numEdges;
}


/****************************************************************************//**
 * Make Projective Edges For Image3D.
 *	\param	src			the source bitmap.
 *	\param	dst			the destination bitmap.
 *	\param	dstClip		the destination clip rectangle, or NULL to draw to the whole dst bitmap.
 *	\param	span		the span, which abstracts information from the src bitmap.
 *	\param	mode		the graphics mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize, possibly ORed with kFskGraphicsModeBilinear.
 *	\param	modeParams	the mode parameters, which contains blendLevel; NULL implies blendLevel = 255.
 *	\param	edges		the edges to be initialized (already allocated).
 *	\return	a nonnegative number of edges	if successful, or
 *	\return	kFskErrUnsupportedPixelType		if either the src or dst pixel format is not accommodated.
 ********************************************************************************/

static FskErr
MakeProjectiveEdgesForImage3D(const FskImage3D *src, FskImage3D *dst, const FskRect *dstClip, Span *span, UInt32 mode, FskConstGraphicsModeParameters modeParams, Edge *edges)
{
	FskErr			numEdges	= 0;
	int				numPts;
	/*const*/float	(*pts)[2];
	float			rectPts[4][2];
	float			M[3][3];
	SInt32			srcKind, dstKind, opCode, quality;

	BAIL_IF_NULL(span->baseAddr = src->baseAddr, numEdges, kFskErrInvalidParameter);	/* Something went wrong with initializing the image -- abort early */
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

	srcKind = PixelFormatToProcTableSrcIndex(src->pixelFormat);
	dstKind = PixelFormatToProcTableDstIndex(dst->pixelFormat);
	quality = (mode & kFskGraphicsModeBilinear) ? 1 : 0;
	opCode = ModeToProcTableOpIndex(mode, modeParams, NULL, dst, span);
	BAIL_IF_FALSE(srcKind >=0 && dstKind >= 0, numEdges, kFskErrUnsupportedPixelType);
	span->fill = projectiveFillProcs[dstKind][srcKind][opCode][quality];
	FskSLinearTransform(src->M[0], dst->M[0], M[0], 3, 3, 3);

	numEdges = TransformClipAndMakeProjectiveEdges(numPts, pts, M, dstClip, span, edges);

bail:
	return numEdges;
}


/********************************************************************************
 * ScanConvertProjectiveEdges
 ********************************************************************************/

static void
ScanConvertProjectiveEdges(int numEdges, Edge *edges, FskImage3D *dst)
{
	Edge	*activeEdges, *e;
	int		y, nextBorn, nextExpired, nextEvent;
	Span	*span;
	char	*dstBaseAddr		= (char*)(dst->baseAddr);
	int		dstRowBytes			= dst->rowBytes;
	const int	dstPixelBytes	= FskBitmapFormatPixelBytes(dst->pixelFormat);


	/* Initialize edge list */
	LinkEdges(numEdges, edges);
	YSortEdges(&edges);
	RemoveDegenerateEdges(&edges);

	#if defined(LOG_EDGES)
		LOGD("ScanConvertProjectiveEdges(numEdges=%d, edges=%p, dst=%p)", numEdges, edges, dst);
		LOGD("\t    top bot         x        dx         u         v         w        du        dv        dw span");
		for (e = edges, y = 0; e; e = e->next, ++y)
			LOGD("\t[%2d] %3d %3d%10.4f%10.5f%10.4g%10.4g%10.4g%10.4g%10.4g%10.4g %p",
				y, e->top, e->bottom, e->x/65536., e->dx/65536., e->u/65536., e->v/65536., e->w/16777216., e->du/65536., e->dv/65536., e->dw/16777216., e->span);
	#endif /* LOG_EDGES */

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
HomogeneousAffineToEuclideanPoints(register FskDVector3D *v3, register int n)
{
	register FskDPoint2D	*p2;

	for (p2 = (FskDPoint2D*)v3; n--; v3++, p2++) {
		p2->x = v3->x;
		p2->y = v3->y;
	}
}


/********************************************************************************
 * TransformAffinePoints
 * as do row vectors.
 ********************************************************************************/

static void
TransformAffinePoints(int nPts, /*const*/float (*iPts)[2], /*const*/double M[3][3], FskDVector3D *oPts)
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
 * On input:	SetFixedAffineMatrixAndPoints(const double *M, int numPts, FskDPoint2D      *xy, Span *span, FskDVector3D      *uvw);
 * On output:	SetFixedAffineMatrixAndPoints(const double *M, int numPts, FskFixedPoint2D *xy, Span *span, FskFixedVector3D *uvw);
 ********************************************************************************/

static void
SetFixedAffineMatrixAndPoints(const double *M, int numPts, FskDPoint2D *xy, Span *span, FskFixedPoint2D *uv)
{
	double		Mi[3][2], *d;
	FskFixed	*x;
	int			i;
	double		di		= (1 << AFFINE_EXTRA_BITS) / (M[0*3+0] * M[1*3+1] - M[0*3+1] * M[1*3+0]);

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

	/* Convert matrix to fixed point with AFFINE_EXTRA_BITS fractional bits (n.b. the matrix is already scaled by (1 << AFFINE_EXTRA_BITS) */
	for (i = 3, x = span->M[0], d = Mi[0]; i--; ) {
		*x++ = RoundDoubleToInt(*d);	d++;
		*x++ = RoundDoubleToInt(*d);	d++;
		*x++ = 1 << AFFINE_EXTRA_BITS;
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
		const FskFixedPoint2D	*t;
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
	const FskRect	*clipRect,
	Span			*span,
	Edge			*edges		/* Clip temp is tacked onto the end of the edges */
)
{
	FskDVector3D		*xyz		= NULL;
	FskFixedPoint2D	*uv;
	FskFixedPoint2D	*pu, *lu;
	FskFixedPoint2D	*pp, *lp;
	int				n, numEdges;
	double			DM[3][3];

	/* Extract storage for xyz and uvwfrom the end of the edges */
	numEdges = MAX_CLIP_POINTS(numPts);								/* Clipping introduces more points */
	xyz      = (FskDVector3D*)ALIGNTO(edges + numEdges, double);	/* After the end of the edges is clipping storage for xyz ... */
	uv       = (FskFixedPoint2D*)(   xyz   + numEdges);				/* ... and uvw */

	/* Transform points */
	FskCopySingleToDoubleMatrix(M[0], DM[0], 3, 3);					/* Double is needed to get the full 32-bit precision for the scan converter, because single only has 24 */
	TransformAffinePoints(numPts, pts, DM, xyz);					/* Affine transforms Euclidean xy to homogeneous xyz. TODO Z is not really needed. */

	/* Clip points */
	n = numPts;
	FskDHClipPolygon(&n, xyz, (FskDVector3D*)uv, clipRect);			/* Homogeneous clipping, using uv as temporary storage. TODO we should use Barsky-Liang clipping for speed. */
	HomogeneousAffineToEuclideanPoints(xyz, n);						/* Homogeneous xy1 projected back to Euclidean xy in-place. TODO with Barsky-Liang, we wouldn't need this.  */

	/* Scale matrix and points so as to have the maximum resolution (30+ bits) for the scan converter */
	SetFixedAffineMatrixAndPoints(DM[0], n, (FskDPoint2D*)xyz, span, uv);	/* xy and uvw are converted from floating- to fixed-point in-place */

	for (numEdges = 0, lp = (pp = (FskFixedPoint2D*)xyz) + n - 1, lu = (pu = uv) + n - 1; n--; lp = pp++, lu = pu++) {
		if (InitAffineEdge(lp, pp, lu, pu, span, edges)) {
			edges++;
			numEdges++;
		}
	}

	return numEdges;
}


/****************************************************************************//**
 * Make Affine Edges For Image3D.
 *	\param	src			the source bitmap.
 *	\param	dst			the destination bitmap.
 *	\param	dstClip		the destination clip rectangle, or NULL to draw to the whole dst bitmap.
 *	\param	span		the span, which abstracts information from the src bitmap.
 *	\param	mode		the graphics mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize, possibly ORed with kFskGraphicsModeBilinear.
 *	\param	modeParams	the mode parameters, which contains blendLevel; NULL implies blendLevel = 255.
 *	\param	edges		the edges to be initialized (already allocated).
 *	\return	a nonnegative number of edges	if successful, or
 *	\return	kFskErrUnsupportedPixelType		if either the src or dst pixel format is not accommodated.
 ********************************************************************************/

static FskErr
MakeAffineEdgesForImage3D(const FskImage3D *src, FskImage3D *dst, const FskRect *dstClip, Span *span, UInt32 mode, FskConstGraphicsModeParameters modeParams, Edge *edges)
{
	FskErr			numEdges	= 0;
	int				numPts;
	/*const*/float	(*pts)[2];
	float			rectPts[4][2];
	float			M[3][3];
	SInt32			srcKind, dstKind, opCode, quality;

	BAIL_IF_NULL(span->baseAddr = src->baseAddr, numEdges, kFskErrInvalidParameter);	/* Something went wrong with initializing the image -- abort early */
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

	srcKind = PixelFormatToProcTableSrcIndex(src->pixelFormat);
	dstKind = PixelFormatToProcTableDstIndex(dst->pixelFormat);
	quality = (mode & kFskGraphicsModeBilinear) ? 1 : 0;
	opCode = ModeToProcTableOpIndex(mode, modeParams, NULL, dst, span);
	BAIL_IF_FALSE(srcKind >=0 && dstKind >= 0, numEdges, kFskErrUnsupportedPixelType);
	span->fill = affineFillProcs[dstKind][srcKind][opCode][quality];
	FskSLinearTransform(src->M[0], dst->M[0], M[0], 3, 3, 3);
	if (M[2][2] != 1.0f)
		FskSScaleMatrix(1.0f / M[2][2], M[0], M[0], 3, 3);	/* Normalize affine matrix */

	numEdges = TransformClipAndMakeAffineEdges(numPts, pts, M, dstClip, span, edges);

bail:
	return numEdges;
}


/********************************************************************************
 * ScanConvertAffineEdges
 ********************************************************************************/

static void
ScanConvertAffineEdges(int numEdges, Edge *edges, FskImage3D *dst)
{
	Edge	*activeEdges, *e;
	int		y, nextBorn, nextExpired, nextEvent;
	Span	*span;
	char	*dstBaseAddr			= (char*)(dst->baseAddr);
	int		dstRowBytes				= dst->rowBytes;
	const int	dstPixelBytes		= FskBitmapFormatPixelBytes(dst->pixelFormat);


	/* Initialize edge list */
	LinkEdges(numEdges, edges);
	YSortEdges(&edges);
	RemoveDegenerateEdges(&edges);

	#if defined(LOG_EDGES)
		LOGD("ScanConvertAffineEdges(numEdges=%d, edges=%p, dst=%p)", numEdges, edges, dst);
		for (e = edges, y = 0; e; e = e->next, ++y)
		LOGD("\t    top bot         x        dx         u         v        du        dv span");
		for (e = edges, y = 0; e; e = e->next, ++y)
			LOGD("\t[%2d] %3d %3d%10.4f%10.5f%10.4g%10.4g%10.4g%10.4g %p",
				y, e->top, e->bottom, e->x/65536., e->dx/65536., e->u/65536., e->v/65536., e->du/65536., e->dv/65536., e->span);
	#endif /* LOG_EDGES */

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
 * FskProjectImages
 ********************************************************************************/

FskErr
FskProjectImages(
#if defined(__ARMCC_VERSION)
			PalmState		state,
#endif /* __ARMCC_VERSION */
			SInt32 numSrcs, const FskImage3D *src, FskImage3D *dst, UInt32 mode, FskConstGraphicsModeParameters modeParams)
{
	FskErr	err			= kFskErrNone;
	Span	*spans		= NULL;
	int		affineXfm	= (mode & kFskGraphicsModeAffine)   ? 1 : 0;	/* Extract transformation type */
	int		numEdges, i, maxPolyClip;
	Edge	*edges;
	Edge	*edge;
	Span	*span;
	FskRect	clipRect;

#ifdef DUMP_PANO
	static Boolean dumped = 0;
	if (dumped == 0 && numSrcs >= 6) {

		for (i = 0; i < numSrcs; ++i) {
			FskImageCompress	comp	= NULL;
			FskFile				fref	= NULL;
			void				*data	= NULL;
			FskBitmap			tm		= NULL;
			char				path[1024];
			UInt32				dataSize;
			err = FskBitmapNewWrapper(src[i].width, src[i].height, src[i].pixelFormat, 32, src[i].baseAddr, src[i].rowBytes, &tm);
			err = FskImageCompressNew(&comp, 0, "image/bmp", tm->bounds.width, tm->bounds.height);
			err = FskImageCompressFrame(comp, tm, &data, &dataSize, NULL, NULL, NULL, NULL, NULL);
			getcwd(path, sizeof(path));
			snprintf(path + FskStrLen(path), sizeof(path) - FskStrLen(path), "/pano%02d.bmp", i);
			if ((err = FskFileCreate(path)) == kFskErrFileExists)
				err = kFskErrNone;
			err = FskFileOpen(path, kFskFilePermissionReadWrite, &fref);
			err = FskFileWrite(fref, dataSize, data, NULL);
			FskBitmapDispose(tm);
			FskFileClose(fref);
			FskMemPtrDispose(data);
			FskImageCompressDispose(comp);
			getcwd(path, sizeof(path));
			snprintf(path + FskStrLen(path), sizeof(path) - FskStrLen(path), "/panM%02d.txt", i);
			if ((err = FskFileCreate(path)) == kFskErrFileExists)
				err = kFskErrNone;
			err = FskFileOpen(path, kFskFilePermissionReadWrite, &fref);
			sprintf(path, "%12.6g %12.6g %12.6g\n%12.6g %12.6g %12.6g\n%12.6g %12.6g %12.6g\n",
					src[i].M[0][0],	src[i].M[0][1],	src[i].M[0][2],
					src[i].M[1][0],	src[i].M[1][1],	src[i].M[1][2],
					src[i].M[2][0],	src[i].M[2][1],	src[i].M[2][2]
			);
			err = FskFileWrite(fref, strlen(path), path, NULL);
			FskFileClose(fref);
		}
		dumped = 1;
	}
#endif /* DUMP_PANO */

	BAIL_IF_NULL(dst->baseAddr, err, kFskErrInvalidParameter);				/* Make sure there's something we can write to */

	/* Determine parameters for memory allocation, so we can do memory allocation only once */
	numEdges	= CountPointsInImages3D(numSrcs, src, &maxPolyClip);		/* The number of points: total and maximum per polygon */
	numEdges	= MAX_CLIP_POINTS(numEdges) + 1;							/* The maximum number of edges, plus one for alignment */
	maxPolyClip	= MAX_CLIP_POINTS(maxPolyClip);								/* The maximum number of clipped points in any one polygon */
	i			= numSrcs     * sizeof(Span)								/* One span for each source */
				+ numEdges    * sizeof(Edge)								/* The total number of edges */
				+ maxPolyClip * sizeof(FskDVector3D) * 2;					/* Clipped xyz and clipped uvw */

	/* Allocate spans, edges, and temporary clipped xyz and uvw */
	BAIL_IF_ERR(err = FskMemPtrNew(i, (FskMemPtr*)(void*)(&spans)));		/* We do one alloc for spans ... */
	edges = (Edge*)(spans + numSrcs);										/* ... and edges, and the temporary memory used for clipping */

	/* Get destination clip */
	BoundingRectOfImage3D(dst, &clipRect);

	#ifdef TEST_AFFINE
		affineXfm = 1;
	#endif /* TEST_AFFINE */

	if (!affineXfm) {																				/* Projective */
		/* Convert points to edges */
		for (numEdges = 0, edge = edges, span = spans; numSrcs--; src++, span++) {
			i = MakeProjectiveEdgesForImage3D(src, dst, &clipRect, span, mode, modeParams, edge);
			BAIL_IF_NEGATIVE(i, err, i);															/* Negative is an error code */
			numEdges += i;																			/* Nonnegative is the number of edges */
			edge     += i;
		}

		/* Scan-convert edges */
		ScanConvertProjectiveEdges(numEdges, edges, dst);
	}
	else {																							/* Affine */
		/* Convert points to edges */
		for (numEdges = 0, edge = edges, span = spans; numSrcs--; src++, span++) {
			i = MakeAffineEdgesForImage3D(src, dst, &clipRect, span, mode, modeParams, edge);
			BAIL_IF_NEGATIVE(i, err, i);															/* Negative is an error code */
			numEdges += i;																			/* Nonnegative is the number of edges */
			edge     += i;
		}

		/* Scan-convert edges */
		ScanConvertAffineEdges(numEdges, edges, dst);
	}

bail:
	if (spans != NULL)	FskMemPtrDispose(spans);													/* One free to free spans and edges */
	return err;
}


/********************************************************************************
 * FskProjectImage
 ********************************************************************************/

FskErr
FskProjectImage(
	/* Source */
	const void	*srcBaseAddr,
	UInt32		srcPixelFormat,
	SInt32		srcRowBytes,
	SInt32		srcWidth,
	SInt32		srcHeight,

	/* Transformation */
	const float	M[3][3],

	/* Source clip */
	SInt32		srcNumPts,			/* If numPts==0 then use default rectangle */
	const float	(*srcPts)[2],		/* Arbitrary polygon */

	UInt32		mode,
	FskConstGraphicsModeParameters	modeParams,

	/* Destination */
	void		*dstBaseAddr,
	UInt32		dstPixelFormat,
	SInt32		dstRowBytes,
	SInt32		dstWidth,
	SInt32		dstHeight,

	/* Destination clip */
	SInt32		dstNumPts,			/* If numPts==0 then use default rectangle */
	const float	(*dstPts)[2]		/* Arbitrary polygon */
)
{
	FskImage3D src, dst;

	src.baseAddr	= (void*)srcBaseAddr;	/* We need to cast this as non-const in order to assign it */
	src.pixelFormat	= srcPixelFormat;
	src.rowBytes	= srcRowBytes;
	src.width		= srcWidth;
	src.height		= srcHeight;
	src.numPts		= srcNumPts;
	src.pts			= (float(*)[2])srcPts;	/* We need to cast this as non-const in order to assign it */
	FskSCopyMatrix(M[0], src.M[0], 3, 3);

	dst.baseAddr	= dstBaseAddr;
	dst.pixelFormat	= dstPixelFormat;
	dst.rowBytes	= dstRowBytes;
	dst.width		= dstWidth;
	dst.height		= dstHeight;
	dst.numPts		= dstNumPts;
	dst.pts			= (float(*)[2])dstPts;	/* We need to cast this as non-const in order to assign it */
	FskSIdentityMatrix(dst.M[0], 3, 3);

	return FskProjectImages(
#if defined(__ARMCC_VERSION)
					NULL,
#endif
					1, &src, &dst, mode, modeParams);
}


