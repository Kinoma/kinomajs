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

#ifndef __KPPROJECTIMAGE__
#define __KPPROJECTIMAGE__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * Pixel formats
 ********************************************************************************/

#define kpFormat32BGRA			0
#define kpFormat32RGBA			1
#define kpFormat32ARGB			2
#define kpFormat32A16RGB565LE	3
#define kpFormat24BGR			4
#define kpFormat16RGB565LE		5


/********************************************************************************
 * Quality
 ********************************************************************************/

#define kpQualityPosition				0
#define kpQualityBits					1
#define kpQualityPointSample			(0 << kpQualityPosition)
#define kpQualityBilinear				(1 << kpQualityPosition)

#define kpTransformationPosition		4
#define kpTransformationBits			1
#define kpTransformationPerspective		(0 << kpTransformationPosition)
#define kpTransformationAffine			(1 << kpTransformationPosition)


/********************************************************************************
 * 3D Images
 ********************************************************************************/

typedef struct KPImage3D {
	/* Image descriptor */
	void	*baseAddr;
	int		pixelFormat;
	long	rowBytes;
	int		width;
	int		height;

	/* Immersion matrix for sources, projection matrix for destination */
	float	M[3][3];

	/* Clipping polygon.
	 * If none is supplied (i.e. (numPts <= 0) or (pts == NULL),
	 * then the whole image rect is used.
	 * Arbitrary polygons are accommodated for sources,
	 * but only integral axis-aligned rects are accommodated for the destination.
	 */
	int		numPts;			/* 0 implies the whole rect (0 or 4 for dst) */
	float	(*pts)[2];		/* NULL implies the whole rect (only rects for dst) */

	void	*userData;		/* Use to store platform- and application-specific info, such as GWorldPtr */
} KPImage3D;


/********************************************************************************
 * Project 3D Images
 ********************************************************************************/

FskAPI(int)
KPProjectImages(
#if defined(__ARMCC_VERSION)
	PalmState		state,
#endif
	int				numSrcs,
	const KPImage3D	*srcs,
	KPImage3D		*dst,
	int				quality
);


/********************************************************************************
 * Project a single image
 ********************************************************************************/

FskAPI(int)
KPProjectImage(
	/* Source */
	const void	*srcBaseAddr,
	int			srcPixelFormat,
	long		srcRowBytes,
	int			srcWidth,
	int			srcHeight,

	/* Transformation */
	const float	M[3][3],

	/* Source clip */
	int			srcNumPts,			/* If numPoints==0 then use default rectangle */
	const float	(*srcPts)[2],		/* Arbitrary polygon */

	int			quality,

	/* Destination */
	void		*dstBaseAddr,
	int			dstPixelFormat,
	long		dstRowBytes,
	int			dstWidth,
	int			dstHeight,

	/* Destination clip */
	int			dstNumPts,			/* If numPts==0 then use defualt rectangle */
	const float	(*dstPts)[2]		/* Arbitrary polygon */
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPPROJECTIMAGE__ */


