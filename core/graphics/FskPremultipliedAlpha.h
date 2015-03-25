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
	\file	FskPremultipliedAlpha.h
	\brief	Conversion between premultiplied and straight alpha.
*/

#ifndef __FSKPREMULTIPLIEDALPHA__
#define __FSKPREMULTIPLIEDALPHA__

#include "FskBitmap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** Convert from straight alpha to premultiplied alpha.
 * Does nothing either if there is no alpha, or if it is already in premultiplied format.
 * Implemented for pixel formats 16AG, 32ARGB, 32ABGR, 32BGRA, 32RGBA, 32A16RGB565LE.
 * This is idempotent for 16AG, 32ARGB, 32ABGR, 32BGRA, 32RGBA in the sense that
 *		FskStraightAlphaToPremultipliedAlpha * FskPremultipliedAlphaToStraightAlpha * FskStraightAlphaToPremultipliedAlpha ==
 *		FskStraightAlphaToPremultipliedAlpha
 *	\param[in]	bm							The bitmap to convert in-place.
 *	\param[in]	r							The sub-rectangle to convert (may be NULL).
 *	\return		kFskErrNone					If the conversion was successful, or no conversion was necessary.
 *	\return		kFskErrInvalidParameter		If the bitmap was NULL.
 *	\return		kFskErrUnsupportedPixelType	If the format has an alpha but conversion has not been implemented.
 *	\bug		Unimplemented for pixel formats 16RGBA4444LE.
 */
FskAPI(FskErr)	FskStraightAlphaToPremultipliedAlpha(FskBitmap bm, FskConstRectangle r);



/** Convert from premultiplied alpha to straight alpha.
 * Does nothing either if there is no alpha, or if it is already in premultiplied format.
 * Implemented for pixel formats 16AG, 32ARGB, 32ABGR, 32BGRA, 32RGBA.
 *	\param[in]	bm							The bitmap to convert in-place.
 *	\param[in]	r							The sub-rectangle to convert (may be NULL).
 *	\param[in]	bgColor						This color is used when alpha is zero.
 *	\return		kFskErrNone					If the conversion was successful, or no conversion was necessary.
 *	\return		kFskErrInvalidParameter		If the bitmap was NULL.
 *	\return		kFskErrUnsupportedPixelType	If the format has an alpha but conversion has not been implemented.
 *	\bug		Unimplemented for pixel formats 16RGBA4444LE, 32A16RGB565LE.
 */
FskAPI(FskErr) FskPremultipliedAlphaToStraightAlpha(FskBitmap bm, FskColorRGB bgColor, FskConstRectangle r);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPREMULTIPLIEDALPHA__ */
