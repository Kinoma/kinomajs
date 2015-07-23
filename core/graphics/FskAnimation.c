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
#include "FskAnimation.h"
#include "FskUtilities.h"
#include "FskFixedMath.h"


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							macros and typedefs							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define FRACT_ONE			0x40000000


//#define BITS_OF_ACCURACY	27	/* 7.45058e-9 */
//#define BITS_OF_ACCURACY	26	/* 1.49012e-8 */
//#define BITS_OF_ACCURACY	25	/* 2.98023e-8 */
#define BITS_OF_ACCURACY	24	/* 5.96046e-8 */	/* Here we declare that the accuracy of single-precision floating-point is OK */
//#define BITS_OF_ACCURACY	23	/* 1.19209e-7 */
//#define BITS_OF_ACCURACY	22	/* 2.38419e-7 */
//#define BITS_OF_ACCURACY	21	/* 4.76837e-7 */
//#define BITS_OF_ACCURACY	20	/* 9.53674e-7 */
#define ALMOST_FLAT			(1 << (30 - BITS_OF_ACCURACY))


typedef struct FskFixedPoint2D FskFractPoint2D;


typedef struct TimingKey {
	FskFract			ti;
	FskFract			to;
	FskFractVector2D	c0;
	FskFractVector2D	c1;
} TimingKey;


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						Key Spline Utilities							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * CopyCubicBezier2D
 ********************************************************************************/

struct _bz4x2 {	FskFractPoint2D p[4];	};
#define CopyCubicBezier2D(fr, to)	*((struct _bz4x2*)(void*)(&to[0])) = *((struct _bz4x2*)(void*)(&fr[0]))


/********************************************************************************
 * FskEvaluateFractKeySplineSegment
 ********************************************************************************/

FskFract
FskEvaluateFractKeySplineSegment(FskFract cx0, FskFract cy0, FskFract cx1, FskFract cy1, FskFract x)
{
	FskFractPoint2D	B[4], *b;
	FskFract		t;

	if ((cx0 | cy0 | (cx1 ^ FRACT_ONE) | (cy1 ^ FRACT_ONE)) == 0)	return x;			/* control(0,0,1,1) --> default --> identity  */
	if (x <= 0)														return 0;			/* 0 --> 0 */
	if (x >= FRACT_ONE)												return FRACT_ONE;	/* 1 --> 1 */

	/* Solve by bisection (we should sometime try Newton) */
	#ifndef BEZIER_BISECTION_CAN_OVERFLOW						/* No need to do this if AVOID_DECASTELJAU_OVERFLOW is #defined in FskFixedMath.c */
		B[0].x = 0;				B[0].y = 0;
		B[1].x = cx0;			B[1].y = cy0;
		B[2].x = cx1;			B[2].y = cy1;
		B[3].x = FRACT_ONE;		B[3].y = FRACT_ONE;
	#else /* BEZIER_BISECTION_CAN_OVERFLOW */					/* Need to do this if AVOID_DECASTELJAU_OVERFLOW is not #defined in FskFixedMath.c */
		B[0].x = 0         >> 1;	B[0].y = 0         >> 1;	/* Insert extra headroom to avoid bisection overflow */
		B[1].x = cx0       >> 1;	B[1].y = cy0       >> 1;
		B[2].x = cx1       >> 1;	B[2].y = cy1       >> 1;
		B[3].x = FRACT_ONE >> 1;	B[3].y = FRACT_ONE >> 1;
		x >>= 1;
	#endif /* BEZIER_BISECTION_CAN_OVERFLOW */

	while (FskFixedDeviationOfBezierControlPoints2D(B, 4) > ALMOST_FLAT) {	/* If not flat enough, split Bezier */
		FskFractPoint2D L[4], R[4];
		FskFixedBisectBezier(&B[0].x, 4, 2, &L[0].x, &R[0].x);				/* Bisect Bezier, diminishing variation quadratically */
		if (x > R[0].x)	CopyCubicBezier2D(R, B);							/* Since the curve is monotonic, we can easily choose the proper half */
		else			CopyCubicBezier2D(L, B);
	}

	if (x >= B[1].x) {
		if (x <= B[2].x)	b = &B[1];										/* The usual case: middle segment */
		else				b = &B[2];										/* Last segment */
	}	else				b = &B[0];										/* First segment */

	if ((t = b[1].x - b[0].x) > 0) {
		#ifdef BEZIER_BISECTION_CAN_OVERFLOW
			b[0].y <<= 1;	b[1].y <<= 1;									/* Remove computational headroom */
		#endif /* BEZIER_BISECTION_CAN_OVERFLOW */
		t = FskFixedRatio(b[1].y - b[0].y, x - b[0].x, t) + b[0].y;			/* Interpolate on non-null domain */
	}
	else {
		#ifdef BEZIER_BISECTION_CAN_OVERFLOW
			t = b[1].y + b[0].y;											/* Average on null domain, remembering that the values have already been halved */
		#else /* BEZIER_BISECTION_CAN_OVERFLOW */
			t = ((b[1].y - b[0].y) >> 1) + b[0].y;							/* Average on null domain, avoiding overflow */
		#endif /* BEZIER_BISECTION_CAN_OVERFLOW */
	}

	return t;
}


/********************************************************************************
 * FskEvaluateASegmentOfAKeySpline
 * This is accurate to 30 fractional bits,
 * which is more than the 24 bits of single-precision floating-point,
 * but less than the 53 of double-precision floating-point
 ********************************************************************************/

double
FskEvaluateASegmentOfAKeySpline(double cx0, double cy0, double cx1, double cy1, double x)
{

	if (x <= 0)	return 0;		/* Return the closest if x is out of the domain */
	if (x >= 1)	return 1;

	return FskFractToFloat(FskEvaluateFractKeySplineSegment(
		FskRoundAndSaturateFloatToUnityFract(cx0), FskRoundAndSaturateFloatToUnityFract(cy0),
		FskRoundAndSaturateFloatToUnityFract(cx1), FskRoundAndSaturateFloatToUnityFract(cy1),
		FskRoundAndSaturateFloatToUnityFract(x)
	));

}


/********************************************************************************
 * IsFractable
 ********************************************************************************/

#define IsFractable(f)	((f >= 0) && (f <= 1))


/********************************************************************************
 * IsMonotonicAndFractableArray
 ********************************************************************************/

static int
IsMonotonicAndFractableArray(register const double *f, int n)
{
	register double g;
	for (n--; n--; f++) {
		g = f[0];
		if (!(	IsFractable(g)	/* This checks the first N-1 and will work with NaN's */
			&&	(g  <= f[1])
		))
			return 0;
	}
	g = f[0];
	if (!IsFractable(g))			/* This checks the first 1 and will work with NaN's */
		return 0;
	return 1;
}


/********************************************************************************
 * IsFractableArray
 ********************************************************************************/

static int
IsFractableArray(const double *f, int n)
{
	register double g;
	for ( ; n--; f++) {
		g = *f;
		if (!IsFractable(g))
			return 0;
	}
	return 1;
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							FskKeySplineSegment							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* This data structure is opaque from the API */
typedef struct FskKeySplineSegmentRecord { FskFract x0, y0, x1, y1;	} FskKeySplineSegmentRecord;


/********************************************************************************
 * FskKeySplineSegmentNew
 ********************************************************************************/

FskErr
FskKeySplineSegmentNew(double x0, double y0, double x1, double y1, FskKeySplineSegment *seg)
{
	FskErr				err;
	FskKeySplineSegment	s;

	BAIL_IF_FALSE((	IsFractable(x0)
				&&	IsFractable(y0)
				&&	IsFractable(x1)
				&&	IsFractable(y1)
	), err, kFskErrBadData);
	BAIL_IF_ERR(err = FskMemPtrNew(sizeof(FskKeySplineSegmentRecord), (FskMemPtr*)seg));
	s = *seg;
	s->x0 = FskRoundPositiveFloatToFract(x0);
	s->y0 = FskRoundPositiveFloatToFract(y0);
	s->x1 = FskRoundPositiveFloatToFract(x1);
	s->y1 = FskRoundPositiveFloatToFract(y1);

bail:
	return err;
}


/********************************************************************************
 * FskKeySplineSegmentDispose
 ********************************************************************************/

void
FskKeySplineSegmentDispose(FskKeySplineSegment seg)
{
	FskMemPtrDispose(seg);
}


/********************************************************************************
 * FskKeySplineSegmentEvaluate
 ********************************************************************************/

double
FskKeySplineSegmentEvaluate(FskKeySplineSegment seg, double t)
{
	return FskFractToFloat(FskEvaluateFractKeySplineSegment(seg->x0, seg->y0, seg->x1, seg->y1, FskRoundAndSaturateFloatToUnityFract(t)));
}



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Key Animation								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


struct FskKeyAnimationRecord {
	UInt32				calcMode;		/* kFskAnimationDiscreteInterpolation, kFskAnimationLinearInterpolation, kFskAnimationPacedInterpolation, kFskAnimationSplineInterpolation */
	UInt32				numKeys;		/* The number of keys */
	TimingKey			*timingKeys;	/* timingKeys[numValues] */
};
typedef struct FskKeyAnimationRecord FskKeyAnimationRecord;


/********************************************************************************
 * KeyAnimationInit
 ********************************************************************************/

static FskErr
KeyAnimationInit(
	UInt32			calcMode,		/* kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation */
	UInt32			numKeys,		/* The number of values */
	const double	*keyTimes,		/* keyTimes[numValues] */
	const double	*keyValues,		/* keyValues[numValues] */
	const double	*keySplines,	/* keySplines[numValues-1][4] */
	FskKeyAnimation	anim
)
{
	FskErr	err	= kFskErrNone;

	anim->calcMode = calcMode;
	if ((anim->numKeys = numKeys) != 0) {
		TimingKey	*key;
		UInt32		i, j;

		if (keyTimes != NULL) {
			BAIL_IF_FALSE(IsMonotonicAndFractableArray(keyTimes, numKeys), err, kFskErrOutOfSequence);
			for (i = numKeys, key = anim->timingKeys; i--; key++, keyTimes++)
				key->ti = FskRoundPositiveFloatToFract(*keyTimes);		/* Copy key times */
		}
		else {
			if (calcMode == kFskAnimationDiscreteInterpolation) {
				for (i = 0, j = numKeys, key = anim->timingKeys; i < numKeys; i++, key++)
					key->ti = FskFracDiv(i, j);								/* Equal intervals */
			}
			else {
				for (i = 0, j = numKeys - 1, key = anim->timingKeys; i < numKeys; i++, key++)
					key->ti = FskFracDiv(i, j);								/* Equal intervals */
			}
		}
		if (keyValues != NULL) {
			BAIL_IF_FALSE(IsFractableArray(keyValues, numKeys), err, kFskErrBadData);
			for (i = numKeys, key = anim->timingKeys; i--; key++, keyValues++)
				key->to = FskRoundPositiveFloatToFract(*keyValues);		/* Copy key points */
		}
		else if (keyTimes != NULL) {
			for (i = numKeys, key = anim->timingKeys; i--; key++)
				key->to = key->ti;										/* to = ti */
		}
		else {
			for (i = 0, j = numKeys - 1, key = anim->timingKeys; i < numKeys; i++, key++)
				key->to = FskFracDiv(i, j);								/* Equal intervals */
		}
		if (keySplines != NULL) {
			BAIL_IF_FALSE(IsFractableArray(keySplines, (numKeys - 1) * 4), err, kFskErrBadData);
			for (i = numKeys - 1, key = anim->timingKeys; i--; key++) {	/* Copy key splines */
				key->c0.x = FskRoundPositiveFloatToFract(*keySplines++);
				key->c0.y = FskRoundPositiveFloatToFract(*keySplines++);
				key->c1.x = FskRoundPositiveFloatToFract(*keySplines++);
				key->c1.y = FskRoundPositiveFloatToFract(*keySplines++);
			}
		}
		else {
			for (i = numKeys - 1, key = anim->timingKeys; i--; key++) {	/* Set to identity */
				key->c0.x = 0;
				key->c0.y = 0;
				key->c1.x = FRACT_ONE;
				key->c1.y = FRACT_ONE;
			}
		}
	}

bail:
	return err;
}


/********************************************************************************
 * KeyAnimationAdjustPace
 ********************************************************************************/

static FskFract
KeyAnimationAdjustPace(FskConstKeyAnimation anim, FskFract s)
{
	TimingKey	*key = NULL;
	UInt32		i;

	if (anim->timingKeys != NULL) {
		for (i = anim->numKeys - 1, key = anim->timingKeys; i--; key++)
			if ((s >= key[0].ti) && (s < key[1].ti))
				break;

		switch (anim->calcMode) {
			case kFskAnimationDiscreteInterpolation:
				s = key->to;
				break;
			case kFskAnimationLinearInterpolation:
				s = FskFixedRatio(key[1].to - key[0].to, s - key[0].ti, key[1].ti - key[0].ti) + key[0].to;
				break;
			case kFskAnimationPacedInterpolation:
				/* s = s */
				break;
			case kFskAnimationSplineInterpolation:
				s = FskFracDiv(s - key[0].ti, key[1].ti - key[0].ti);
				s = FskEvaluateFractKeySplineSegment(key->c0.x, key->c0.y, key->c1.x, key->c1.y, s);
				s = FskFracMul(s, key[1].to - key[0].to) + key[0].to;
				break;
		}
	}

	return s;
}


/********************************************************************************
 * FskKeyAnimationDispose
 ********************************************************************************/

void
FskKeyAnimationDispose(FskKeyAnimation anim)
{
	FskMemPtrDispose(anim);
}


/********************************************************************************
 * FskKeyAnimationNew
 ********************************************************************************/

FskErr
FskKeyAnimationNew(
	UInt32			calcMode,		/* kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation */
	UInt32			numKeys,		/* The number of values */
	const double	*keyTimes,		/* keyTimes[numValues] */
	const double	*keyValues,		/* keyValues[numValues] */
	const double	*keySplines,	/* keySplines[numValues-1][4] */
	FskKeyAnimation	*anim
)
{
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskKeyAnimationRecord)+(numKeys+1)*sizeof(TimingKey), (FskMemPtr*)anim)) == kFskErrNone) {
		(**anim).timingKeys = (TimingKey*)(*anim + 1);
		KeyAnimationInit(calcMode, numKeys, keyTimes, keyValues, keySplines, *anim);
	}

	return err;
}


/********************************************************************************
 * FskKeyAnimationEvaluate
 ********************************************************************************/

double
FskKeyAnimationEvaluate(FskConstKeyAnimation anim, double t)
{
	return FskFractToFloat(KeyAnimationAdjustPace(anim, FskRoundAndSaturateFloatToUnityFract(t)));
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Path Animation								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


struct FskPathAnimationRecord {
	FskKeyAnimationRecord	keyAnim;
	FskConstPath			path;
	FskArcLengthTable		arcLengthTab;
};
typedef struct FskPathAnimationRecord FskPathAnimationRecord;


/********************************************************************************
 * FskPathAnimationDispose
 ********************************************************************************/

void
FskPathAnimationDispose(FskPathAnimation anim)
{
	if (anim != NULL) {
		if (anim->arcLengthTab != NULL)
			FskPathArcLengthTableDispose(anim->arcLengthTab);
		FskMemPtrDispose(anim);
	}
}


/********************************************************************************
 * FskKeyPathAnimationNew
 ********************************************************************************/

FskErr
FskKeyPathAnimationNew(
	FskConstPath		path,			/* Not copied!!! */
	UInt32				calcMode,		/* kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation */
	UInt32				numKeys,		/* The number of keys */
	const double		*keyTimes,		/* keyTimes[numKeys] */
	const double		*keyPoints,		/* keyPoints[numKeys] */
	const double		*keySplines,	/* keySplines[numKeys-1][4] */
	FskPathAnimation	*anim			/* The result */
)
{
	FskErr				err;
	FskArcLengthTable	arcTab				= NULL;
	double				*defaultKeyPoints	= NULL;

	*anim = NULL;
	BAIL_IF_NULL(path, err, kFskErrInvalidParameter);
	BAIL_IF_ERR(err = FskPathArcLengthTableNew(path, &arcTab, NULL));	/* Generate arc length table in case we need it for the default keys */
	if (	((numKeys == 0) || (keyPoints == NULL))															/* Check for keyPoints ... */
		&&	(FskPathGetVisibleSegmentDistances(path, arcTab, &numKeys, &defaultKeyPoints) == kFskErrNone)	/* ... or allocate some defaults */
	) {
		keyTimes  = NULL;												/* This triggers a default uniform interval between key times */
		keyPoints = defaultKeyPoints;									/* Normalized segment distances */
	}
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskPathAnimationRecord)+(numKeys+1)*sizeof(TimingKey), (FskMemPtr*)anim));	/* Allocate anim */
	(**anim).path = path;																		/* Initialize anim */
	(**anim).keyAnim.timingKeys = (TimingKey*)(*anim + 1);
	(**anim).arcLengthTab = arcTab;										/* Transfer ownership of arcTab to *anim, ... */
	arcTab = NULL;														/* ... and indicate that we no longer look after it */
	KeyAnimationInit(calcMode, numKeys, keyTimes, keyPoints, keySplines, &((**anim).keyAnim));

bail:
	FskMemPtrDispose(defaultKeyPoints);
	FskMemPtrDispose(arcTab);

	return err;
}


/********************************************************************************
 * FskPathAnimationNew
 ********************************************************************************/

FskErr
FskPathAnimationNew(
	FskConstPath		path,			/* Not copied!!! */
	FskPathAnimation	*anim			/* The result */
)
{
	return FskKeyPathAnimationNew(path, kFskAnimationPacedInterpolation, 0, NULL, NULL, NULL, anim);
}


/********************************************************************************
 * FskPathAnimationCountPathSegments
 ********************************************************************************/

UInt32
FskPathAnimationCountPathSegments(FskConstPathAnimation anim)
{
	return FskPathGetSegmentCount(anim->path);
}


/********************************************************************************
 * ConvertFractFractFixedToDoubleMatrices
 ********************************************************************************/

static void
ConvertFractFractFixedToDoubleMatrices(const FskFixedMatrix3x2 *M, double *Mtrn, double *Mrot)
{
	if (Mtrn != NULL) {
		Mtrn[2*2+0] = FskFixedToFloat(M->M[2][0]);	Mtrn[2*2+1] = FskFixedToFloat(M->M[2][1]);
		if (Mtrn != Mrot) {
			Mtrn[0*2+0] = 1;						Mtrn[0*2+1] = 0;
			Mtrn[1*2+0] = 0;						Mtrn[1*2+1] = 1;
		}
	}
	if (Mrot != NULL) {
		Mrot[0*2+0] = FskFractToFloat(M->M[0][0]);	Mrot[0*2+1] = FskFractToFloat(M->M[0][1]);
		Mrot[1*2+0] = FskFractToFloat(M->M[1][0]);	Mrot[1*2+1] = FskFractToFloat(M->M[1][1]);
		if (Mtrn != Mrot) {
			Mrot[2*2+0] = 0;						Mrot[2*2+1] = 0;
		}
	}
}


/********************************************************************************
 * FskPathAnimationEvaluate
 ********************************************************************************/

FskErr
FskPathAnimationEvaluate(FskConstPathAnimation anim, double s, double *Mtrn, double *Mrot)
{
	FskErr				err;
	FskFixedMatrix3x2	M;
	FskFract			f;

	f = FskRoundAndSaturateFloatToUnityFract(s);
	f = KeyAnimationAdjustPace(&anim->keyAnim, f);
	err = FskPathEvaluate(anim->path, anim->arcLengthTab, f, &M);
	ConvertFractFractFixedToDoubleMatrices(&M, Mtrn, Mrot);

	return err;
}


/********************************************************************************
 * FskPathAnimationEvaluateSegment
 ********************************************************************************/

FskErr
FskPathAnimationEvaluateSegment(FskConstPathAnimation anim, UInt32 segIndex, double s, double *Mtrn, double *Mrot)
{

	FskErr				err;
	FskFixedMatrix3x2	M;
	FskFract			f;

	f = FskRoundAndSaturateFloatToUnityFract(s);
	err = FskPathEvaluateSegment(anim->path, anim->arcLengthTab, segIndex, f, &M);
	ConvertFractFractFixedToDoubleMatrices(&M, Mtrn, Mrot);

	return err;
}


/********************************************************************************
 * FskPathAnimationGetSegmentDistances
 ********************************************************************************/

FskErr
FskPathAnimationGetSegmentDistances(FskConstPathAnimation anim, UInt32 seg, double startEnd[2])
{
	FskErr		err		= kFskErrNone;
	UInt32		numSegs;
	FskFract	dist[2];

	BAIL_IF_FALSE(	(	((numSegs = FskPathGetSegmentCount(anim->path)) > 0)
					&&	(seg < numSegs)
					), err, kFskErrUnknownElement
	);
	FskPathArcLengthTableGetDistancesOfSegment(anim->arcLengthTab, seg, dist);
	startEnd[0] = FskFractToFloat(dist[0]);
	startEnd[1] = FskFractToFloat(dist[1]);

bail:
	return err;
}

