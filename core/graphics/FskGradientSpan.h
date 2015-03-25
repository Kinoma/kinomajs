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
#ifndef __FSKGRADIENTSPAN__
#define __FSKGRADIENTSPAN__

#ifndef __FSKSPAN__
# include "FskSpan.h"
#endif /* __FSKSPAN__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/********************************************************************************
 ********************************************************************************
 *****							FskLinearGradientSpan						*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * Allocate and initialize the extra data for a linear gradient span, storing it in
 * the FskSpan's fillData field.
 ********************************************************************************/

FskErr		FskInitLinearGradientSpan(
				FskSpan					*span,
				FskBitmap				dstBM,
				const FskFixedMatrix3x2	*pathMatrix,		/* Can be NULL */
				UInt32					quality,			/* Ignored */
				const struct FskColorSource *cs
);


/********************************************************************************
 * Deallocate the extra data for a linear gradient span, stored in the span's fillData field.
 ********************************************************************************/

void		FskDisposeLinearGradientSpanData(FskSpan *span);



/********************************************************************************
 ********************************************************************************
 *****							FskRadialGradientSpan						*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * Deallocate a linear gradient span
 ********************************************************************************/

void		FskDisposeRadialGradientSpanData(FskSpan *span);


/********************************************************************************
 * Allocate a radial gradient span, and set the transform relating imaging space to gradient space.
 * The coordinates given are in the same space as the coordinates of the Polygon or Path.
 * The matrices are optional. The pathMatrix is common, and is the same as the one
 * used for transforming the Path. The gradientMatrix is an optional transformation,
 * which further modifies the effect of the radial gradient specified by (cx, cy, fx, fy, r).
 ********************************************************************************/

FskErr		FskInitRadialGradientSpan(
				FskSpan					*span,
				FskBitmap				dstBM,
				const FskFixedMatrix3x2	*pathMatrix,		/* Can be NULL */
				UInt32					quality,			/* Ignored */
				const struct FskColorSource *cs
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGRADIENTSPAN__ */


