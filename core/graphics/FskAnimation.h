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
#ifndef __FSKANIMATION__
#define __FSKANIMATION__

#ifndef __FSKPATH__
# include "FskPath.h"
#endif /* __FSKPATH__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Spline Utilities							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


				/* Evaluate a segment of a key spline, given its parameters explicitly   --   Fixed-point */
FskAPI(FskFract)	FskEvaluateFractKeySplineSegment(FskFract cx0, FskFract cy0, FskFract cx1, FskFract cy1, FskFract x);
												/*  ----------------- control points -------------------  eval point */

				/* Evaluate a segment of a key spline, given its parameters explicitly   --   Floating-point */
FskAPI(double)	FskEvaluateASegmentOfAKeySpline(double cx0, double cy0, double cx1, double cy1, double x);
												/*  ------------ control points --------------  eval point */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							FskKeySplineSegment							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

				/* FskKeySplineSegment object */
struct			FskKeySplineSegmentRecord;
typedef struct	FskKeySplineSegmentRecord *FskKeySplineSegment;

				/* FskKeySplineSegment constructor */
FskAPI(FskErr)	FskKeySplineSegmentNew(double x0, double y0, double x1, double y1, FskKeySplineSegment *seg);

				/* FskKeySplineSegment destructor */
void			FskKeySplineSegmentDispose(FskKeySplineSegment seg);

				/* FskKeySplineSegment evaluator */
double			FskKeySplineSegmentEvaluate(FskKeySplineSegment seg, double t);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								FskKeyAnimation							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/* Calc modes */
enum {
	kFskAnimationDiscreteInterpolation		= 0,
	kFskAnimationLinearInterpolation		= 1,
	kFskAnimationPacedInterpolation			= 2,
	kFskAnimationSplineInterpolation		= 3
};

/* FskKeyAnimation object */
struct					FskKeyAnimationRecord;
typedef struct			FskKeyAnimationRecord *FskKeyAnimation;
typedef const struct	FskKeyAnimationRecord *FskConstKeyAnimation;



				/* FskKeyAnimation constructor */
FskAPI(FskErr)	FskKeyAnimationNew(
					UInt32			calcMode,		/* kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation */
					UInt32			numKeys,		/* The number of values */
					const double	*keyTimes,		/* keyTimes[numValues] */
					const double	*keyValues,		/* keyValues[numValues] */
					const double	*keySplines,	/* keySplines[numValues-1][4] */
					FskKeyAnimation	*anim
				);

				/* FskKeyAnimation destructor */
FskAPI(void)	FskKeyAnimationDispose(FskKeyAnimation anim);

				/* FskKeyAnimation evaluator */
FskAPI(double)	FskKeyAnimationEvaluate(FskConstKeyAnimation anim, double t);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								FskPathAnimation						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


struct					FskPathAnimationRecord;
typedef struct			FskPathAnimationRecord	*FskPathAnimation;
typedef const struct	FskPathAnimationRecord	*FskConstPathAnimation;



				/* Constructor - This just stores a reference to the path */
FskAPI(FskErr)	FskPathAnimationNew(
					FskConstPath		path,			/* Not copied!!! */
					FskPathAnimation	*anim			/* The result */
				);

				/* Constructor - This just stores a reference to the path */
FskAPI(FskErr)	FskKeyPathAnimationNew(
					FskConstPath		path,			/* Not copied!!! */
					UInt32				calcMode,		/* kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation */
					UInt32				numKeys,		/* The number of keys */
					const double		*keyTimes,		/* keyTimes[numKeys] */
					const double		*keyPoints,		/* keyPoints[numKeys] */
					const double		*keySplines,	/* keySplines[numKeys-1][4] */
					FskPathAnimation	*anim			/* The result */
				);



				/* Destructor - This does not dispose of the path */
FskAPI(void)	FskPathAnimationDispose(FskPathAnimation anim);

				/* Count Segments */
FskAPI(UInt32)	FskPathAnimationCountPathSegments(FskConstPathAnimation anim);

				/* Evaluate at a fraction of total arc length */
FskAPI(FskErr)	FskPathAnimationEvaluate(FskConstPathAnimation anim, double s, double *Mtrn, double *Mrot);

				/* Evaluate the given segment at a fraction of its normalized arc length */
FskAPI(FskErr)	FskPathAnimationEvaluateSegment(FskConstPathAnimation anim, UInt32 seg, double s, double *Mtrn, double *Mrot);

				/* Get the start and end distances of a segment */
FskAPI(FskErr)	FskPathAnimationGetSegmentDistances(FskConstPathAnimation anim, UInt32 seg, double startEnd[2]);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKANIMATION__ */


