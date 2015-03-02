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
#ifndef __FSKROTATE90__
#define __FSKROTATE90__

#ifndef __FSKRECTBLIT__
	#include "FskRectBlit.h"
#endif /* __FSKRECTBLIT__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 * FskRotate90
 * srcClipRect is transformed by scaleOffset to dstBM->bounds and then clipped by dstClipRect
 * The transformation applied is this:
 *  dstX = scaleOffset->scaleX * srcY + scaleOffset->offsetX;
 *  dstY = scaleOffset->scaleY * srcX + scaleOffset->offsetY;
 * where (srcX, srcY) are specified relative to srcClipRect,
 * and (dstX, dstY) are specified relative to dstBM->bounds.
 * dstClipRect is intended to be used for window updates
 * when only part of the window needs refreshing.
 * Either scaleOffset->scaleX or scaleOffset->scaleY should be negative
 * in order to produce an image rotation; otherwise a reflection will occur.
 ********************************************************************************/

FskAPI(FskErr)
FskRotate90(
	FskBitmap				srcBM,			/* The source image */
	FskConstRectangle		srcClipRect,	/* This can be NULL, this effectively being srcBM->bounds */
	FskBitmap				dstBM,			/* The destination image */
	FskConstRectangle		dstClipRect,	/* This can be NULL, thus effectively being dstBM->bounds */
	const FskScaleOffset 	*scaleOffset,	/* The scale and offset are specified relative to dstBM->bounds */
	UInt32					quality			/* 0 -> point-sampling, 1 -> bilinear interpolation */
);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __FSKROTATE90__ */
