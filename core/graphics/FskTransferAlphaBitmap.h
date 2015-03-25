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
	\file	FskTransferAlphaBitmap.h
	\brief	Pixel Transfers over a Rectangular Domain with pure alpha as a source.
*/
#ifndef __FSKTRANSFERALPHABITMAP__
#define __FSKTRANSFERALPHABITMAP__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Use the given alpha map to modulate the color and apply to the destination.
 *	\param[in]	srcBM		the source bitmap. Must be pixel format 8G or 8A.
 *	\param[in]	srcRect		portion of the source bitmap to be transferred (may be NULL).
 *	\param[in]	dstBM		the proxy destination bitmap.
 *	\param[in]	dstLocation	the location where the src is to be copied
 *	\param[in]	dstClip		the destination clipping rectangle (may be NULL).
 *	\param[in]	fgColor		the color that is given to the source pixels with full value.
 *	\param[in]	modeParams	additional mode parameters.
 *	\return		kFskErrNone	if the operation was successful.
**/
FskAPI(FskErr)
FskTransferAlphaBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstPoint					dstLocation,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				opColor,
	FskConstGraphicsModeParameters	modeParams
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKTRANSFERALPHABITMAP__ */

