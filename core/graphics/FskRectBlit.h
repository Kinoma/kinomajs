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
	\file	FskRectBlit.h
	\brief	Pixel Transfers over a Rectangular Domain.
*/

#ifndef __FSKRECTBLIT__
#define __FSKRECTBLIT__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"		/* for FskFixed */
#endif /* __FSKFIXEDMATH__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* \fn FskErr FskBitmapDraw(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect, FskConstRectangle dstClip, FskConstColorRGBA opColor, UInt32 mode, FskConstGraphicsModeParameters modeParams)
 *	\brief	Transfer one bitmap to another with scaling, format conversion, and transfer operation.
 *
 *	This procedure specifies size transformations with the srcRect and dstRect, though allows neither reflections nor subpixel positioning.
 *	\param[in]		src			The source bitmap.
 *	\param[in]		srcRect		The rectangle within the source that is to be transferred to the destination.
 *								This can be NULL, in which case the src->bounds is used as the srcRect.
 *	\param[in,out]	dst			The destination bitmap.
 *	\param[in]		dstRect		The rectangle to which the srcRect is to be mapped. This implies a scaling transformation.
 *								This can be NULL, in which case the dst->bounds is used as the dstRect.
 *	\param[in]		dstClip		This restricts changes to the destination to this clipping rectangle.
 *								This can be NULL, in which case the dst->bounds is used as the clipping rectangle.
 *	\param[in]		opColor		The color to be used in the kFskGraphicsModeColorize mode.
 *								This can be NULL, in which case opaque white RGBA={255,255,255,255} is used.
 *	\param[in]		mode		The desired transfer mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize,
 *								optionally ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	Extra parameters needed by the selected transfer mode, currently only blendLevel.
 *								This can be NULL, in which case a blendLevel of 255 is used.
 *	\return			kFskErrNone					if successful.
 *	\return			kFskErrUnalignedYUV			if the source or destination was YUV and the location or width was not an even number.
 *	\return			kFskErrUnsupportedPixelType	if either the source or destination is not supported by the implementation on this platform.
 *	\return			kFskErrInvalidParameter		if the transfer mode is not recognized.
 **/
/* This is declared in FskBitmap.h, even though it is implemented in FskRectBlit.c.
 *		FskErr FskBitmapDraw(
 *			FskConstBitmap					src,
 *			FskConstRectangle				srcRect,
 *			FskBitmap						dst,
 *			FskConstRectangle				dstRect,
 *			FskConstRectangle				dstClip,
 *			FskConstColorRGBA				opColor,
 *			UInt32							mode,
 *			FskConstGraphicsModeParameters	modeParams
 *		);
 */


#define kFskScaleBits	24	/**< The number of fractional bits used in the API to represent scale.  */
#define kFskOffsetBits	16	/**< The number of fractional bits used in the API to represent offset. */


/** FskScaleOffset is used to specify arbitrary scales and offsets for FskScaleOffsetBitmap().
 *
 * Note that the scale factor is equal to the stride in destination pixels corresponding to a unit stride
 * in source pixels, NOT the ratio between the number of pixels. For example, scaling by a factor of 2.0 does not
 * yield twice the number of pixels, it yields 2*W-1 pixels, where W is the number of pixels in the source.
 * To get double the number of pixels, you would use a scale factor of FloatToFixed24((2.0*W-1)/(W-1)).
 * In general, to convert from the number of pixels to a scale factor, use (D-1)/(S-1),
 * where D and S are the number of pixels in the destination and source, respectively.
 **/
typedef struct FskScaleOffset {
	FskFixed24	scaleX;		/**< Horizontal scale factor, in fixed-point with kFskScaleBits  fractional bits. */
	FskFixed24	scaleY;		/**< Vertical   scale factor, in fixed-point with kFskScaleBits  fractional bits. */
	FskFixed	offsetX;	/**< Horizontal offset,       in fixed-point with kFskOffsetBits fractional bits. */
	FskFixed	offsetY;	/**< Vertical   offset,       in fixed-point with kFskOffsetBits fractional bits. */
} FskScaleOffset;


/** Transfer one bitmap to another with scaling, format conversion, and transfer operation.
 * This API specifies transformations by directly specifying the scales and offsets,
 * the specification of reflections and subpixel positioning.
 *	\param[in]		srcBM		The source bitmap.
 *	\param[in]		srcRect		The rectangle within the source that is to be transferred to the destination.
 *								This can be NULL, in which case the srcBM->bounds is used as the srcRect.
 *	\param[in,out]	dstBM		The destination bitmap.
 *	\param[in]		dstClip		This restricts changes to the destination to this clipping rectangle.
 *								This can be NULL, in which case the dstBM->bounds is used as the clipping rectangle.
 *	\param[in]		scaleOffset	The anisotropic scale and offset to be applied to the source bitmap, relative to the srcRect. This cannot be NULL.
 *								Note that the scale is limited to [-128.0000000, +127.9999999], and offset is limited to [-32768.00000, +32767.99998].
 *	\param[in]		opColor		The color to be used in the kFskGraphicsModeColorize mode.
 *								This can be NULL, in which case opaque white RGBA={255,255,255,255} is used.
 *	\param[in]		mode		The desired transfer mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize,
 *								optionally ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	Extra parameters needed by the selected transfer mode, currently only blendLevel.
 *								This can be NULL, in which case a blendLevel of 255 is used.
 *	\return			kFskErrNone					if successful.
 *					kFskErrUnalignedYUV			if the source or destination was YUV and the location or width was not an even number.
 *					kFskErrUnsupportedPixelType	if either the source or destination is not supported by the implementation on this platform.
 *					kFskErrInvalidParameter		if the transfer mode is not recognized.
 */

FskAPI(FskErr)
FskScaleOffsetBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const FskScaleOffset 			*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* blend level tint color */
);


/** Transfer one bitmap to another with tiling, format conversion, and transfer operation.
 *	\param[in]		srcBM		The source bitmap.
 *	\param[in]		srcRect		The rectangle within the source that is to be transferred to the destination.
 *								This can be NULL, in which case the srcBM->bounds is used as the srcRect.
 *	\param[in,out]	dstBM		The destination bitmap.
 *	\param[in]		dstRect		The rectangle to which the srcRect is to be mapped. This specifies the phase of the transfer.
 *								This can be NULL, in which case the dstBM->bounds is used as the dstRect.
 *	\param[in]		dstClip		This restricts changes to the destination to this clipping rectangle.
 *								This can be NULL, in which case the dstBM->bounds is used as the clipping rectangle.
 *	\param[in]		scale		The isotropic scale to be applied to the source bitmap during its transfer to the destination bitmap.
 *								This is typically 1.0, 1.5 or 2.0, but is not restricted to these values.
 *	\param[in]		opColor		The color to be used in the kFskGraphicsModeColorize mode.
 *								This can be NULL, in which case opaque white RGBA={255,255,255,255} is used.
 *	\param[in]		mode		The desired transfer mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize,
 *								optionally ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	Extra parameters needed by the selected transfer mode, currently only blendLevel.
 *								This can be NULL, in which case a blendLevel of 255 is used.
 *	\return			kFskErrNone					if successful.
 *					kFskErrUnalignedYUV			if the source or destination was YUV and the location or width was not an even number.
 *					kFskErrUnsupportedPixelType	if either the source or destination is not supported by the implementation on this platform.
 *					kFskErrInvalidParameter		if the transfer mode is not recognized.
 */
FskAPI(FskErr)
FskTileBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,
	FskConstRectangle				dstClip,
	FskFixed						scale,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* blend level tint color */
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKRECTBLIT__ */

