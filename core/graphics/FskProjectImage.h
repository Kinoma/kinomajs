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
#ifndef __FSKPROJECTIMAGE__
#define __FSKPROJECTIMAGE__

#ifndef __FSKGRAPHICS_H__
	#include "FskGraphics.h"
#endif /* __FSKGRAPHICS_H__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define kFskSrcIsPremul	0x04000000	/**< This is ORed with the mode to indicate that the sources are premultiplied. */


/****************************************************************************//**
 * Images immersed in 3D.
 ********************************************************************************/

typedef struct FskImage3D {
	/* Image descriptor */
	void	*baseAddr;		/**< The address of pixel(0,0). */
	int		pixelFormat;	/**< The pixel format. */
	int		rowBytes;		/**< The byte stride between pixel(x,y) and pixel(x,y+1). */
	int		width;			/**< The number of pixels horizontally. */
	int		height;			/**< The number of pixels vertically. */


	float	M[3][3];		/**< The immersion matrix for sources, projection matrix for destination. */

	/* Clipping polygon.
	 * If none is supplied (i.e. (numPts <= 0) or (pts == NULL),
	 * then the whole image rect is used.
	 * Arbitrary polygons are accommodated for sources,
	 * but only integral axis-aligned rects are accommodated for the destination.
	 */
	int		numPts;			/**< The number of points in the polygon. 0 implies the whole rect (0 or 4 for dst). */
	float	(*pts)[2];		/**< The points defining the polygon.  NULL implies the whole rect (only rects for dst). */

	void	*userData;		/**< Use to store platform- and application-specific info, such as GWorldPtr */
} FskImage3D;


/****************************************************************************//**
 * Project 3D Images.
 *	\param[in]	state		Used on the Palm device to point to the global state.
 *	\param[in]	numSrcs		The number of source images.
 *	\param[in]	srcs		The source images.
 *	\param[out]	dst			The destination image.
 *	\param[in]	mode		The compositional mode: ( kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize }
 *							optionally ORed with { kFskGraphicsModeBilinear, kFskGraphicsModeAffine, kFskSrcIsPremul }.
 *	\param[in]	modeParams	Extra parameters used with the mode, notably, the blendLevel.
 *	\return		kFskErrNone					if the images were rendered successfully.
 *	\return		kFskErrUnsupportedPixelType	if either the src or dst pixel format is not accommodated.
 ********************************************************************************/

FskAPI(FskErr)
FskProjectImages(
#if defined(__ARMCC_VERSION)
	PalmState			state,
#endif
	SInt32				numSrcs,
	const FskImage3D	*srcs,
	FskImage3D			*dst,
	UInt32				mode,
	FskConstGraphicsModeParameters	modeParams
);


/****************************************************************************//**
 * Project a single image in perspective.
 *	\param[in]	srcBaseAddr		The address of source pixel(0,0).
 *	\param[in]	srcPixelFormat	The pixel format of the source image.
 *	\param[in]	srcRowBytes		The byte stride between srcPixel(x,y) and srcPixel(x,y+1).
 *	\param[in]	srcWidth		The number of pixels horizontally in the source image.
 *	\param[in]	srcHeight		The number of pixels  vertically  in the source image.
 *	\param[in]	M				The transformation matrix.
 *	\param[in]	srcNumPts		The number of points inthe source clipping polygon; the whole source if 0.
 *	\param[in]	srcPts			The points defining the source clipping polygon; the whole source if NULL.
 *	\param[in]	mode			The compositional mode: ( kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize }
 *								optionally ORed with { kFskGraphicsModeBilinear, kFskGraphicsModeAffine, kFskSrcIsPremul }.
 *	\param[in]	modeParams		Extra parameters used with the mode, notably, the blendLevel.
 *	\param[in]	dstBaseAddr		The address of destination pixel(0,0).
 *	\param[in]	dstPixelFormat	The pixel format of the destination image.
 *	\param[in]	dstRowBytes		The byte stride between dstPixel(x,y) and dstPixel(x,y+1).
 *	\param[in]	dstWidth		The number of pixels horizontally in the destination image.
 *	\param[in]	dstHeight		The number of pixels  vertically  in the destination image.
 *	\param[in]	dstNumPts		The number of points in the destination clipping polygon; the whole destination if 0.
 *	\param[in]	dstPts			The points definingthe destination clipping polygon; the whole destination, in NULL.
 *	\return		kFskErrNone					if the images were rendered successfully.
 *	\return		kFskErrUnsupportedPixelType	if either the src or dst pixel format is not accommodated.
 ********************************************************************************/

FskAPI(FskErr)
FskProjectImage(
	/* Source */
	const void	*srcBaseAddr,
	UInt32		srcPixelFormat,
	SInt32		srcRowBytes,
	SInt32		srcWidth,
	SInt32		srcHeight,

	/* Transformation */
	const float	M[3][3],

	/* Source clip */
	SInt32		srcNumPts,			/* If numPoints==0 then use default rectangle */
	const float	(*srcPts)[2],		/* Arbitrary polygon */

	UInt32		mode,
	FskConstGraphicsModeParameters	modeParams,

	/* Destination */
	void		*dstBaseAddr,
	UInt32		dstPixelFormat,
	SInt32		dstRowBytes,
	SInt32		dstWidth,
	SInt32		dstHeight,

	/* Destination clip */
	SInt32		dstNumPts,			/* If numPts==0 then use defualt rectangle */
	const float	(*dstPts)[2]		/* Arbitrary polygon */
);

// #define FSK_GATHER_COORDINATE_SIGNATURE	// This is for debugging only.


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPROJECTIMAGE__ */


