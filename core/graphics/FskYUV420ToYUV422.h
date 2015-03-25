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
	\file	FskYUV420ToYUV422.h
	\brief	Conversion of YUV 4:2:0 to YUV 4:2:2.
*/
#ifndef __FSKYUV420TO422__
#define __FSKYUV420TO422__


#ifndef __FSK__
	#include "Fsk.h"
#endif /* __FSK__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** Convert from planar YUV 4:2:0 format to the 8-bit chunky YUV 4:2:2 format called
 *		'yuvs' or kYUVSPixelFormat on the Macintosh, and
 *		'YUY2' on Windows.
 *	It is an 8-bit 4:2:2 Component Y'CbCr format, with the component ordering of: Y0, Cb, Y1, Cr,
 *	though this procedure can be used to convert to the component ordering of: Y0 Cr Y1 Cb
 *	by swapping the srcU and srcV pointers.
 *	This is most prevalent YUV 4:2:2 format on both Mac and Win32 platforms.
 *
 *	\param[in]	width			the number of horizontal samples in the luminance (Y) image;
 *								the chrominance (U,V) images have half the number of samples horizontally.
 *	\param[in]	height			the number of  vertical  samples in the luminance (Y) image;
 *								the chrominance (U,V) images have half the number of samples vertically.
 *	\param[in]	srcY			pointer to the Y luminance samples.
 *	\param[in]	srcU			pointer to the Cb' chrominance samples.
 *	\param[in]	srcV			pointer to the Cr' chrominance samples.
 *	\param[in]	srcYRowBytes	the byte stride between  luminance   (Y)  samples vertically adjacent.
 *	\param[in]	srcUVRowBytes	the byte stride between chrominance (U,V) samples vertically adjacent.
 *	\param[out]	dst				pointer to the resultant Y Cb' Y Cr' image
 *	\param[in]	dstRowBytes		the byte stride between  YCbYCr samples vertically adjacent.
 **/
FskAPI(void)	FskYUV420toYUYV(
					UInt32			width,
					UInt32			height,
					const UInt8		*srcY,
					const UInt8		*srcU,
					const UInt8		*srcV,
					SInt32			srcYRowBytes,
					SInt32			srcUVRowBytes,
					UInt32			*dst,
					SInt32			dstRowBytes
				);



/** Convert from planar YUV 4:2:0 format to the 8-bit YUV 4:2:2 format called
 *		'2vuy' or k2vuyPixelFormat pixel on the Macintosh, and
 *		'UYVY' on Windows.
 *	The resultant components are ordered in memory: Cb, Y0, Cr, Y1,
 *	though this procedure can be used to convert to the component ordering of: Cr Y0 Cb Y1
 *	by swapping the srcU and srcV pointers.
 *	The luminance components have a range of [16, 235], while the chroma value has a range of [16, 240].
 *	This is consistent with the CCIR601 spec.
 *	This format is fairly prevalent on both Mac and Win32 platforms.
 *
 *	\param[in]	width			the number of horizontal samples in the luminance (Y) image;
 *								the chrominance (U,V) images have half the number of samples horizontally.
 *	\param[in]	height			the number of  vertical  samples in the luminance (Y) image;
 *								the chrominance (U,V) images have half the number of samples vertically.
 *	\param[in]	srcY			pointer to the Y luminance samples.
 *	\param[in]	srcU			pointer to the Cb' chrominance samples.
 *	\param[in]	srcV			pointer to the Cr' chrominance samples.
 *	\param[in]	srcYRowBytes	the byte stride between  luminance   (Y)  samples vertically adjacent.
 *	\param[in]	srcUVRowBytes	the byte stride between chrominance (U,V) samples vertically adjacent.
 *	\param[out]	dst				pointer to the resultant Cb Y0 Cr Y1 image
 *	\param[in]	dstRowBytes		the byte stride between  CbYCrY samples vertically adjacent.
 **/
FskAPI(void)	FskYUV420toUYVY(
					UInt32			width,
					UInt32			height,
					const UInt8		*srcY,
					const UInt8		*srcU,
					const UInt8		*srcV,
					SInt32			srcYRowBytes,
					SInt32			srcUVRowBytes,
					UInt32			*dst,
					SInt32			dstRowBytes
				);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKYUV420TO422__ */

