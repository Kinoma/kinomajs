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
	\file	FskShadow.h
	\brief	Shadow Effect.
*/
#ifndef __FSKSHADOW__
#define __FSKSHADOW__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Apply a shadow effect to the given bitmap.
 *
 * The bitmap must contain an alpha channel, and the bitmap should have a large enough transparent border
 * so that it can accommodate the generated shadow without clipping.
 *
 * The alpha channel is offset and blurred, then it is used to apply a color under the bitmap.
 * This effect can be applied to a large image with a lot of icons, but there should be
 * enough space between the icons so that their shadows do not interfere.
 *
 * \param[in]		offset						the vector indicating the direction and length of the shadow.
 * \param[in]		softness					the Gaussian sigma used to blur the alpha channel,
 *												typically between 1.0 and the length of the offset.
 * \param[in]		colorRGBA					the color of the shadow; note that alpha is usually less than 255
 *												to provide shade rather than total darkness.
 *												This color is not premultiplied.
 * \param[in]		rect						the region of the bitmap onto which the shadow effect is applied. Can be NULL.
 * \param[in]		premul						indicates whether the bitmap is premultiplied or not.
 * \param[in,out]	bitmap						the image onto which the shadow effect is applied.
 * \return			kFskErrNone					if no error.
 *					kFskErrUnsupportedPixelType	if the pixel type is not supported (it needs alpha).
 */
FskAPI(FskErr)	FskShadow(FskConstPoint		offset,
						  float				softness,
						  FskConstColorRGBA	colorRGBA,
						  FskConstRectangle	rect,
						  Boolean			premul,
						  FskBitmap			bitmap);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKSHADOW__ */


