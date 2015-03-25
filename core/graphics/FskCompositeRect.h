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
	\file	FskCompositeRect.h
	\brief	Adobe and Porter-Duff compositional modes.
*/
#ifndef __FSKCOMPOSITERECT__
#define __FSKCOMPOSITERECT__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 * Adobe composition modes, for straight alpha
 ********************************************************************************/

#define kFskCompositeNormal				0	/**< Alpha blend over: a*F + (1-a)*B. */
#define kFskCompositeMultiply			1	/**< Multiply components: F*B. */
#define kFskCompositeScreen				2	/**< Adobe Screen: (1-(1-F)*(1-B)). */
#define kFskCompositeOverlay			3	/**< Adobe Overlay. */
#define kFskCompositeDarken				4	/**< Adobe Darken, or subtract with saturation. */
#define kFskCompositeLighten			5	/**< Adobe Lighten, or add with saturation. */
#define kFskCompositeColorDodge			6	/**< Adobe Color dodge. */
#define kFskCompositeColorBurn			7	/**< Adobe Color burn. */
#define kFskCompositeHardLight			8	/**< Adobe Hard light. */
#define kFskCompositeSoftLight			9	/**< Adobe Soft light. */
#define kFskCompositeDifference			10	/**< Adobe Difference: abs(B - F). */
#define kFskCompositeExclusion			11	/**< Adobe Exclusion: like difference, but not as strong. */
#define kFskNumAdobeCompositeModes		(kFskCompositeExclusion - kFskCompositeNormal + 1)	/**< The number of Adobe composition modes. */

/********************************************************************************
 * Porter-Duff composition modes, for premultiplied alpha.
 ********************************************************************************/

#define kFskCompositePreSourceOver		12	/**< Premultiplied Porter-Duff source over destination. */
#define kFskCompositePreDestinationOver	13	/**< Premultiplied Porter-Duff destination over source. */
#define kFskCompositePreSourceIn		14 	/**< Premultiplied Porter-Duff source in destination. */
#define kFskCompositePreDestinationIn	15	/**< Premultiplied Porter-Duff destination in source. */
#define kFskCompositePreSourceOut		16	/**< Premultiplied Porter-Duff source out of destination. */
#define kFskCompositePreDestinationOut	17	/**< Premultiplied Porter-Duff destination out of source. */
#define kFskCompositePreSourceAtop		18	/**< Premultiplied Porter-Duff source atop destination. */
#define kFskCompositePreDestinationAtop	19	/**< Premultiplied Porter-Duff destination atop source. */
#define kFskCompositePreLighter			20	/**< Premultiplied Porter-Duff lighter (add). */
#define kFskCompositePreXor				21	/**< Premultiplied Porter-Duff exclusive-OR. */
#define kFskNumPorterDuffCompositeModes	(kFskCompositePreXor - kFskCompositePreSourceOver + 1)	/**< The number of Porter-Duff composition modes. */


/**
 * Composite one bitmap into another.
 *
 *	\param[in]	compositionMode	The desired composition operation, any of the Adobe or Porter-Duff modes.
 *	\param[in]	srcBM			The source bitmap.
 *								If the composition operation is an Adobe operator, it should be straight alpha.
 *								If the composition operator is Porter-Duff, it should be premultiplied alpha.
 *	\param[in]	srcRect			If not NULL, the region of the source to participate in the composition;
 *								otherwise, the whole src->bounds.
 *	\param[out]	dstBM			The bitmap to be modified by composition with the source bitmap.
 *	\param[in]	dstLoc			The location of where the srcRect is to be placed in the destination.
 *	\return		kFskErrNone					if there were no errors.
 *	\return		kFskErrUnimplemented		if the source and destination are not the same pixel format.
 *	\return		kFskErrInvalidParameter		if the composition mode is out of the implemented range.
 *	\return		kFskErrUnsupportedPixelType	if the pixel was not 32-bit with alpha.
 */
FskAPI(FskErr)	FskCompositeRect(
					int					compositionMode,
					FskConstBitmap		srcBM,
					FskConstRectangle	srcRect,
					FskBitmap			dstBM,
					FskConstPoint		dstLoc
				);

/**
 * Composite one bitmap into another, using a matte and a global alpha.
 *
 *	\param[in]	compositionMode	The desired composition operation. Only Porter-Duff modes are accommodated.
 *	\param[in]	globalAlpha		The overall transparency that modulates other transparencies.
 *	\param[in]	matteBM			The matte, of the same size as the srcBM. Can be NULL.
 *	\param[in]	srcBM			The source bitmap, premultiplied to black.
 *	\param[in]	srcRect			If not NULL, the region of the source to participate in the composition;
 *								otherwise, the whole src->bounds.
 *	\param[out]	dstBM			The bitmap to be modified by composition with the source bitmap.
 *	\param[in]	dstLoc			If non NULL, the location where the srcRect is to be placed in the destination;
 *								otherwise, it is placed at the same place as srcRect.
 *	\return		kFskErrNone					if there were no errors.
 *	\return		kFskErrUnimplemented		if the source and destination are not the same pixel format.
 *	\return		kFskErrInvalidParameter		if the composition mode is not one of the Porter-Duff modes.
 *	\return		kFskErrUnsupportedPixelType	if the pixel was not 32-bit with alpha.
 *	\return		FskErrMismatch				if the matte and src bitmap dimensions were different.
 */
FskAPI(FskErr)	FskMatteCompositeRect(
					int					compositionMode,
					UInt8				globalAlpha,
					FskConstBitmap		matteBM,
					FskConstBitmap		srcBM,
					FskConstRectangle	srcRect,
					FskBitmap			dstBM,
					FskConstPoint		dstLoc
				);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCOMPOSITERECT__ */


