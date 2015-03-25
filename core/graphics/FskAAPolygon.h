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
	\file	FskAAPolygon.h
	\brief	Anti-aliased polygons and polylines.
*/
#ifndef __FSKAAPOLYGON__
#define __FSKAAPOLYGON__

#ifndef __FSKPOLYGON__
# include "FskPolygon.h"
#endif /* __FSKPOLYGON__ */

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifndef __FSKSPAN__
# include "FskSpan.h"
#endif /* __FSKSPAN__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** Frame and/or fill an anti-aliasied polygon with multiple contours.
 *	\param[in]		nContours			The number of contours in the polygon.
 *	\param[in]		nPts				An array of the number of points in each contour.
 *	\param[in]		pts					An array of points for the contours.
 *	\param[in]		strokeWidth			The stroke width to be used for framing the polygon. Usually set to 0 or -1 if not framing.
 *	\param[in]		frameFillColors		A pointer to a pair of colorsource pointers.
 *										The first color source is to be used for framing, the second for filling.
 *										Use NULL as a color source to indicate that the polygon is not to be framed of filled, respectively.
 *	\param[in]		fillRule			The rule to be used for filling contours:	kFskFillRuleNonZero === kFskFillRuleWindingNumber or
 *																					kFskFillRuleEvenOdd === kFskFillRuleParity
 *	\param[in]		M					Transformation matrix, or NULL which indicates the identity.
 *	\param[in]		clipRect			A destination-aligned clipping rectangle, or NULL, if no clipping is desired.
 *	\param[in,out]	dstBM				The destination bitmap.
 *	\return			kFskErrNone					if there were no errors;
 *					kFskErrNothingRendered		if no pixels were actually rendered (not really an error);
 *					kFskErrUnsupportedPixelType	if the estination pixel format is not supported;
 *					...
 */

FskAPI(FskErr)
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
);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKAAPOLYGON__ */


