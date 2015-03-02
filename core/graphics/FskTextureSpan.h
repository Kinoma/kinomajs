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
	\file	FskTextureSpan.h
	\brief	Fill a span of a scanline, using a texture.
*/

#ifndef __FSKTEXTURESPAN__
#define __FSKTEXTURESPAN__

#ifndef __FSKSPAN__
# include "FskSpan.h"
#endif /* __FSKSPAN__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/********************************************************************************
 ********************************************************************************
 *****								FskTextureSpan							*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************/
/** Allocate and initialize the extra data for a texture span, storing it in
 * the FskSpan's fillData field.
 *
 *	\param[out]	span		The span to be initialized.
 *	\param[out]	dstBM		The dstination bitmap.
 *	\param[in]	pathMatrix	The matrix currently applied to the path. Can be NULL.
 *	\param[in]	quality		The desired quality: 0=low, 1=high.
 *	\param[in]	cs			The color source specfication.
 *	\return		kFskErrNone	if the span was initialzed successfully.
 ********************************************************************************/

FskErr	FskInitTextureSpan(FskSpan *span, FskBitmap dstBM, const FskFixedMatrix3x2 *pathMatrix, UInt32 quality, const struct FskColorSource *cs);


/********************************************************************************/
/** Deallocate the extra data for a texture span, stored in the span's fillData field.
 *	\param[in]	span	the span whose span data is to be disposed.
 ********************************************************************************/

void	FskDisposeTextureSpanData(FskSpan *span);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKTEXTURESPAN__ */


