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
#ifndef __FSKPICK__
#define __FSKPICK__

#ifndef __FSKPATH__
# include "FskPath.h"
#endif /* __FSKPATH__ */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** The result of picking is either false, true, or an error result. */
typedef int FskPickResult;

/** The result of picking is either false, true, or an error result. */
enum {
	/* negative numbers are error conditions */
	kFskHitFalse	= 1,			/**< No hit was detected. */
	kFskHitTrue		= kFskErrNone	/**< A  hit was detected. */
};


/** Specify a rectangular pick region.
 *	\param[in]	path				The path to be hit-tested.
 *	\param[in]	strokeWidth			Frame thickness, or negative for no frame.
 *	\param[in]	jointSharpness		0 => round, 1 => bevel, >1 => miter.
 *	\param[in]	endCaps				One of { kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt }.
 *	\param[in]	fillRule			One of { kFskFillRuleNonZero, kFskFillRuleEvenOdd, or negative for no fill }.
 *	\param[in]	frameColor			Frame color - can be NULL if the pixel color is not needed.
 *	\param[in]	fillColor			Fill  color - can be NULL if the pixel color is not needed.
 *	\param[in]	M					Transformation matrix; NULL implies identity.
 *	\param[in]	pickRect			Test to see if the path hits this rect.
 *	\param[in]	pixelColor			The color of the pixel under the pick (can be NULL if not needed).
 */
FskAPI(FskPickResult)	FskPickPath(
	FskConstPath			path,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	int						endCaps,
	int						fillRule,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
	const FskFixedMatrix3x2	*M,
	FskConstRectangle		pickRect,
	FskColorRGBA			pixelColor
);

/** Specify a point and hit radius.
 *	\param[in]	path				The path to be hit-tested.
 *	\param[in]	strokeWidth			Frame thickness, or negative for no frame.
 *	\param[in]	jointSharpness		0 => round, 1 => bevel, >1 => miter.
 *	\param[in]	endCaps				One of { kFskLineEndCapRound, kFskLineEndCapSquare, kFskLineEndCapButt }.
 *	\param[in]	fillRule			One of { kFskFillRuleNonZero, kFskFillRuleEvenOdd, or negative for no fill }.
 *	\param[in]	frameColor			Frame color - can be NULL if the pixel color is not needed.
 *	\param[in]	fillColor			Fill  color - can be NULL if the pixel color is not needed.
 *	\param[in]	M					Transformation matrix; NULL implies identity.
 *	\param[in]	pickRadius			Within this radius (roughly) of the given point.
 *	\param[in]	pickPoint			Does something hit this point?
 *	\param[in]	pixelColor			The color of the pixel under the pick (can be NULL if not needed).
 */
FskAPI(FskPickResult)	FskPickPathRadius(
	FskConstPath			path,
	FskFixed				strokeWidth,
	FskFixed				jointSharpness,
	int						endCaps,
	int						fillRule,
	const FskColorSource	*frameColor,
	const FskColorSource	*fillColor,
	const FskFixedMatrix3x2	*M,
	int						pickRadius,
	FskConstPoint			pickPoint,
	FskColorRGBA			pixelColor
);

/** Specify a rectangular pick region.
 *	\param[in]	bm					The bitmap to be hit-tested.
 *	\param[in]	transparentThresh	An alpha greater than this level is a hit.
 *	\param[in]	M					Transformation matrix; NULL implies identity.
 *	\param[in]	pickRect			Test to see if the path hits this rect.
 *	\param[in]	pixelColor			The color of the pixel under the pick (can be NULL if not needed).
 */
FskAPI(FskPickResult)	FskPickBitmap(
	FskBitmap				bm,
	UInt8					transparentThresh,
	const FskFixedMatrix3x2	*M,
	FskConstRectangle		pickRect,
	FskColorRGBA			pixelColor
);

/** Specify a point and hit radius.
 *	\param[in]	bm					The bitmap to be hit-tested.
 *	\param[in]	transparentThresh	An alpha greater than this level is a hit.
 *	\param[in]	M					Transformation matrix; NULL implies identity.
 *	\param[in]	pickRadius			Within this radius (roughly) of the given point.
 *	\param[in]	pickPoint			Does something hit this point?
 *	\param[in]	pixelColor			The color of the pixel under the pick (can be NULL if not needed).
 */
FskAPI(FskPickResult)	FskPickBitmapRadius(
	FskBitmap				bm,
	UInt8					transparentThresh,
	const FskFixedMatrix3x2	*M,
	int						pickRadius,
	FskConstPoint			pickPoint,
	FskColorRGBA			pixelColor
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPICK__ */


