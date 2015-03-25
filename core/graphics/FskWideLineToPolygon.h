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
#ifndef __FSKWIDEPOLYLINETOPOLYGON__
#define __FSKWIDEPOLYLINETOPOLYGON__


#ifndef __FSKGROWABLEFIXEDPOINT2DARRAY__
# include "FskGrowableFixedPoint2DArray.h"
#endif /* __FSKGROWABLEFIXEDPOINT2DARRAY__ */

#ifndef __FSKPOLYGON__
# include "FskPolygon.h"
#endif /* __FSKPOLYGON__ */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/*******************************************************************************
 * WidePolyLineToPolygon
 *
 * For rounded joints, use a jointSharpness of 0.
 * For bevelled joints, use a jointSharpness of fixed one = 0x10000
 * For mitered joints, use a jointShasrpness greater than fixed one.
 *
 * If *polygon==NULL, a new FskGrowableFixedPoint2DArray will be allocated,
 * otherwise the old one will be reset and used instead.
 *******************************************************************************/

FskAPI(FskErr)
FskWidePolyLineToPolygon(
	UInt32							nPts,
	const FskFixedPoint2D			*pts,
	FskFixed						strokeWidth,
	FskFixed						jointSharpness,
	UInt32							endCaps,
	const FskFixedMatrix3x2			*M,
	FskGrowableFixedPoint2DArray	*polygonHandle
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKWIDEPOLYLINETOPOLYGON__ */

