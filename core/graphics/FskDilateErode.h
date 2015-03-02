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
/**
	\file	FskDilateErode.h
	\brief	Grayscale dilation and erosion.
*/
#ifndef __FSKDILATEERODE__
#define __FSKDILATEERODE__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** FskDilate will enlarge the alpha matte by the given radius.
 *	\param[in]	radius	the amount by which the alpha is to be enlarged.
 *	\param[in]	srcBM	the source bitmap image, which must contain an alpha channel.
 *	\param[in]	srcRect	the [optional] region of the source bitmap which is to be dilated.
 *	\param[out]	dstBM	the destination bitmap image, in either 8G or 8A format.
 *						This should be the size of the srcRect or srcBM.
 *	\param[in]	dstLoc	the [optional] location of the resultant dilated alpha image.
 *	\return		kFskErrNone	if the operation was completed successfully.
 *	\note					In the current implementation, a square dilation region is used
 *							rather than a circular region, so that horizontal and vertical flattening
 *							may become apparent with larger radii.
 */
FskAPI(FskErr) FskDilate(int radius, FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc);


/** FskErode will shrink the alpha matte by the given radius.
 * Parameters:
 *	\param[in]	radius	the amount by which the alpha is to be shrunken.
 *	\param[in]	srcBM	the source bitmap image, which must contain an alpha channel.
 *	\param[in]	srcRect	the [optional] region of the source bitmap which is to be eroded.
 *	\param[out]	dstBM	the destination bitmap image, in either 8G or 8A format.
 *						This should be the size of the srcRect or srcBM.
 *	\param[in]	dstLoc	the [optional] location of the resultant eroded alpha image.
 */
FskAPI(FskErr) FskErode(int radius, FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKDILATEERODE__ */


