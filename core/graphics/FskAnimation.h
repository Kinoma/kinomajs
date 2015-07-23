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
	\file	FskAnimation.h
	\brief	Functions to facilitate animation.
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


/** Evaluate a segment of a key spline, given its parameters explicitly   --   Fixed-point.
 *	The control points { cx0, cy0, cx1, cy1 } are Bezier points used to control the velocity of x and y at 0 and 1.
 *	A building block, not intended for the end user.
 *	\param[in]	cx0		The X control point at 0.
 *	\param[in]	cy0		The Y control point at 0.
 *	\param[in]	cx1		The X control point at 1.
 *	\param[in]	cy1		The Y control point at 1.
 *	\param[in]	x		The value of x at which to evaluate the value of y.
 *	\return				The value of y corresponding to the value of x.
 */
FskAPI(FskFract)	FskEvaluateFractKeySplineSegment(FskFract cx0, FskFract cy0, FskFract cx1, FskFract cy1, FskFract x);
													/*  ----------------- control points -------------------  eval point */

/** Evaluate a segment of a key spline, given its parameters explicitly   --   Floating-point.
 *	The control points { cx0, cy0, cx1, cy1 } are Bezier points used to control the velocity of x and y at 0 and 1.
 *	A building block, not intended for the end user.
 *	\param[in]	cx0		The X control point at 0.
 *	\param[in]	cy0		The Y control point at 0.
 *	\param[in]	cx1		The X control point at 1.
 *	\param[in]	cy1		The Y control point at 1.
 *	\param[in]	x		The value of x at which to evaluate the value of y.
 *	\return				The value of y corresponding to the value of x.
 */
FskAPI(double)	FskEvaluateASegmentOfAKeySpline(double cx0, double cy0, double cx1, double cy1, double x);
												/*  ------------ control points --------------  eval point */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							FskKeySplineSegment							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/** FskKeySplineSegment object record. */
struct			FskKeySplineSegmentRecord;

/** FskKeySplineSegment object. */
typedef struct	FskKeySplineSegmentRecord *FskKeySplineSegment;

/** FskKeySplineSegment constructor.
 *	Allocate a new key spline segment, and set its value to (x0, y0, x1, y1).
 *	These are not necessarily intended for the end user.
 *	\param[in]	x0	The X control point at 0.
 *	\param[in]	y0	The Y control point at 0.
 *	\param[in]	x1	The X control point at 1.
 *	\param[in]	y1	The Y control point at 1.
 *	\param[out]	seg	The resulting segment, initialized as specified.
 *	\return			kFskErrNone		if the operation was completed successfully.
 */
FskAPI(FskErr)	FskKeySplineSegmentNew(double x0, double y0, double x1, double y1, FskKeySplineSegment *seg);

/* FskKeySplineSegment destructor.
 *	\param[in]	seg	The segment to dispose.
 */
void			FskKeySplineSegmentDispose(FskKeySplineSegment seg);

/** FskKeySplineSegment evaluator.
 *	\param[in]	seg	The segment to evaluate.
 *	\param[in]	t	The parametric value at which to evaluate the segment, in [0, 1].
 *	\return		the evaluation of the key spline at the parameter t.
 */
double			FskKeySplineSegmentEvaluate(FskKeySplineSegment seg, double t);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								FskKeyAnimation							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/** Calculation modes for keyframe animation.
 *	These are used to warp time, adjust pace, or otherwise nonlinearly map [0,1] to [0,1].
 */
enum {
	kFskAnimationDiscreteInterpolation		= 0,	/**< Piecewise constant interpolation. */
	kFskAnimationLinearInterpolation		= 1,	/**< Piecewise linear interpolation. */
	kFskAnimationPacedInterpolation			= 2,	/**< Paced interpolation, according to arc length. */
	kFskAnimationSplineInterpolation		= 3		/**< Spline interpolation, with velocity controlled by the spline. */
};

/** FskKeyAnimation object record. */
struct					FskKeyAnimationRecord;

/** FskKeyAnimation object. */
typedef struct			FskKeyAnimationRecord *FskKeyAnimation;

/** const FskKeyAnimation object. */
typedef const struct	FskKeyAnimationRecord *FskConstKeyAnimation;



/** FskKeyAnimation constructor.
 *	\param[in]	calcMode		The mode of calculation, one of { kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation}.
 *	\param[in]	numKeys,		The number of keys.
 *	\param[in]	keyTimes,		the key times:  keyTimes[numValues].
 *	\param[in]	keyValues,		the key values: keyValues[numValues].
 *	\param[in]	keySplines		the key splines to control the pace: keySplines[numValues-1][4].
 *	\param[out]	anim			A place to store the resulting animation.
 *	\return		kFskErrNone		if the operation completed successfully.
 */
FskAPI(FskErr)	FskKeyAnimationNew(UInt32 calcMode, UInt32 numKeys, const double *keyTimes, const double *keyValues, const double *keySplines, FskKeyAnimation *anim );

/** FskKeyAnimation destructor.
 *	\param[in]	anim	The key animation to dispose.
 */
FskAPI(void)	FskKeyAnimationDispose(FskKeyAnimation anim);

/* FskKeyAnimation evaluator.
 *	\param[in]	anim	the key animation to be evaluated.
 *	\param[in]	t		the time at which the animation is to be evaluated.
 *	\return				the evaluation of the key animation at the time t.
 */
FskAPI(double)	FskKeyAnimationEvaluate(FskConstKeyAnimation anim, double t);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								FskPathAnimation						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** Path animation object record. */
struct					FskPathAnimationRecord;

/** Path animation object. */
typedef struct			FskPathAnimationRecord	*FskPathAnimation;

/** const Path animation object. */
typedef const struct	FskPathAnimationRecord	*FskConstPathAnimation;



/** Constructor for a path animation.
 * This just stores a reference to the path.
 * The speed is constant, parameterized by arc length.
 *	\param[in]	path	the path to be used for animation. This remains a reference and is not copied!
 *	\param[out]	anim	the resulting animation.
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)	FskPathAnimationNew(FskConstPath path, FskPathAnimation *anim);

/** Constructor for a path animation.
 * This just stores a reference to the path.
 *	\param[in]	path			the path to be used for animation. This remains a reference and is not copied!
 *	\param[in]	calcMode		the calculation mode, one of {kFskDiscreteInterpolation, kFskLinearInterpolation, kFskPacedInterpolation, kFskSplineInterpolation}.
 *	\param[in]	numKeys			The number of keys.
 *	\param[in]	keyTimes		keyTimes[numKeys]: the times where the keys are specified.
 *	\param[in]	keyPoints		keyPoints[numKeys]: the value specified at the key times.
 *	\param[in]	keySplines		keySplines[numKeys-1][4]: splines to control speed and acceleration along the curve.
 *	\param[out]	anim			the resulting animation.
 */
FskAPI(FskErr)	FskKeyPathAnimationNew(FskConstPath path, UInt32 calcMode, UInt32 numKeys, const double *keyTimes, const double *keyPoints, const double *keySplines, FskPathAnimation *anim);

/** Destructor.
 * This does not dispose of the path.
 *	\param[in]	anim	the animation to be disposed.
 */
FskAPI(void)	FskPathAnimationDispose(FskPathAnimation anim);

/** Count the number of segments in a path animation.
 *	\param[in]	anim	the animation whose segments are to be counted.
 *	\return		the number of segments in the path animation.
 */
FskAPI(UInt32)	FskPathAnimationCountPathSegments(FskConstPathAnimation anim);

/** Evaluate at a fraction of total arc length.
 *	\param[in]	anim
 *	\param[in]	s
 *	\param[out]	Mtrn	a 3x2 matrix, to store the translation result in a form [ [ 1  0 ] [ 0  1 ] [ x  y ] ], i.e. the translation is the bottom row. Can be NULL.
 *	\param[out]	Mrot 	a 3x2 matrix, to store the  rotation   result in a form [ [ c +s ] [ -s c ] [ 0  0 ] ], i.e. the tangent is the first row, and the normal is the second row. Can be NULL.
 *						If (Mtrn == Mrot), the two matrices are merged as  [  [ c +s ] [ -s c ] [ x  y ] ].
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)	FskPathAnimationEvaluate(FskConstPathAnimation anim, double s, double *Mtrn, double *Mrot);

/** Evaluate the given segment at a fraction of its normalized arc length.
 *	\param[in]	anim	the animation whose segment is to be evaluated.
 *	\param[in]	seg		the segment index to be evaluated.
 *	\param[in]	s		the parametric value (in [0, 1]) at which the segment is to be evaluated.
 *	\param[out]	Mtrn	a 3x2 matrix, to store the translation result in a form [ [ 1  0 ] [ 0  1 ] [ x  y ] ], i.e. the translation is the bottom row. Can be NULL.
 *	\param[out]	Mrot 	a 3x2 matrix, to store the  rotation   result in a form [ [ c +s ] [ -s c ] [ 0  0 ] ], i.e. the tangent is the first row, and the normal is the second row. Can be NULL.
 *						If (Mtrn == Mrot), the two matrices are merged as  [  [ c +s ] [ -s c ] [ x  y ] ].
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)	FskPathAnimationEvaluateSegment(FskConstPathAnimation anim, UInt32 seg, double s, double *Mtrn, double *Mrot);

/** Get the start and end distances of a segment.
 *	\param[in]	anim		the animation to be queried.
 *	\param[in]	seg			the segment index to be queried.
 *	\param[out]	startEnd	a place to store the start and end distances of the specified segment.
 */
FskAPI(FskErr)	FskPathAnimationGetSegmentDistances(FskConstPathAnimation anim, UInt32 seg, double startEnd[2]);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKANIMATION__ */


