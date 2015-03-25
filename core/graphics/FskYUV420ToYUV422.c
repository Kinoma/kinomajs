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
#include "FskYUV420ToYUV422.h"


/********************************************************************************
 * FskYUV420toYUYV
 *
 *	This assumes that the src and dst have the same ranges.
 *
 *	The 'yuvs' or kYUVSPixelFormat is an 8-bit 4:2:2 Component Y'CbCr format.
 *	Identical to the k2vuyPixelFormat except each 16 bit word has been byte
 *	swapped. This results in a component ordering of; Y0, Cb, Y1, Cr. This is
 *	most prevalent yuv 4:2:2 format on both Mac and Win32 platforms. The
 *	equivalent Microsoft fourCC is 'YUY2'.  This format is supported with the
 *	UNSIGNED_SHORT_8_8_APPLE type for pixel storage operations.
 ********************************************************************************/

void
FskYUV420toYUYV(
	UInt32			width,
	UInt32			height,
	const UInt8		*srcY,
	const UInt8		*srcU,				/* Cb' */
	const UInt8		*srcV,				/* Cr' */
	SInt32			srcYRowBytes,
	SInt32			srcCRowBytes,
	UInt32			*dst32,				/* Y Cb' Y Cr' */
	SInt32			dstRowBytes
)
{
	UInt8		*dst1		= (UInt8*)dst32;
	UInt8		*dst0		= dst1  + dstRowBytes;
	const UInt8	*srcY1		= srcY;
	const UInt8	*srcY0		= srcY1 + srcYRowBytes;
	const UInt8	*srcU1		= srcU;
	const UInt8	*srcU0		= srcU;
	const UInt8	*srcV1		= srcV;
	const UInt8	*srcV0		= srcV;
	SInt32		dstBump, srcYBump, srcCBump, c0, c1, cd;
	UInt32		i;

	if (	((width  &= ~1) == 0)	/* Make sure the dimensions are even ... */
		||	((height &= ~1) == 0)	/* ... and at least 2x2 */
	)
		return;

	dstBump		= (dstRowBytes  << 1) - width * sizeof(UInt16);	/* Jump 2 lines at a time in the dst */
	srcYBump	= (srcYRowBytes << 1) - width * sizeof(UInt8);	/* Jump 2 lines at a time in the src Y */

	width  = (width  >> 1);		/* Do 2 pixels at a time horizontally */
	height = (height >> 1) - 1;	/* Do 2 pixels at a time vertically, except the first and last lines, which are done separately */

	srcCBump	=  srcCRowBytes       - width * sizeof(UInt8);	/* Jump 1 line  at a time in the src U and V */

	/* Do the first line, with no interpolation of chrominance */
	for (i = width; i--; ) {
		*dst1++ = *srcY1++;							/* Y(0,0) */
		*dst1++ = *srcU1++;							/* U(0,0) */
		*dst1++ = *srcY1++;							/* Y(0,1) */
		*dst1++ = *srcV1++;							/* V(0,0) */
	}

	/* Do the middle lines two at a time. We interpolate U and V to be 1/4 and 3/4 weights of the closest samples vertically */
	for(srcY1 += srcYBump, srcU1 += srcCBump, srcV1 += srcCBump, dst1 += dstBump; height--;
		srcY1 += srcYBump, srcU1 += srcCBump, srcV1 += srcCBump, dst1 += dstBump,
		srcY0 += srcYBump, srcU0 += srcCBump, srcV0 += srcCBump, dst0 += dstBump
	) {
		for (i = width; i--; ) {
			*dst0++ = *srcY0++;						/* Y(0,0) */
			*dst1++ = *srcY1++;						/* Y(1,0) */
			c0 = (SInt32)(*srcU0++);
			c1 = (SInt32)(*srcU1++);
			cd = ((c1 - c0 + (1 << (2 - 1))) >> 2);	/* (c1 - c0) / 4 */
			*dst0++ = c0 + cd;						/* U(0,0) */
			*dst1++ = c1 - cd;						/* U(1,0) */
			*dst0++ = *srcY0++;						/* Y(0,1) */
			*dst1++ = *srcY1++;						/* Y(1,1) */
			c0 = (SInt32)(*srcV0++);
			c1 = (SInt32)(*srcV1++);
			cd = ((c1 - c0 + (1 << (2 - 1))) >> 2);	/* (c1 - c0) / 4 */
			*dst0++ = c0 + cd;						/* V(0,0) */
			*dst1++ = c1 - cd;						/* V(1,0) */
		}
	}

	/* Do the last line, with no interpolation of chrominance */
	for (i = width; i--; ) {
		*dst0++ = *srcY0++;							/* Y(0,0) */
		*dst0++ = *srcU0++;							/* U(0,0) */
		*dst0++ = *srcY0++;							/* Y(0,1) */
		*dst0++ = *srcV0++;							/* V(0,0) */
	}
}


/********************************************************************************
 * FskYUV420toUYVY
 *
 *	This assumes that the src and dst have the same ranges.
 *
 *	The '2vuy' or k2vuyPixelFormat pixel format is an 8-bit 4:2:2 Component
 *	Y'CbCr format. Each 16 bit pixel is represented by an unsigned eight bit
 *	luminance component and two unsigned eight bit chroma components. Each pair
 *	of pixels shares a common set of chroma values. The components are ordered
 *	in memory; Cb, Y0, Cr, Y1. The luminance components have a range of [16,
 *	235], while the chroma value has a range of [16, 240]. This is consistent
 *	with the CCIR601 spec. This format is fairly prevalent on both Mac and Win32
 *	platforms. The equivalent Microsoft fourCC is 'UYVY'.  This format is
 *	supported with the UNSIGNED_SHORT_8_8_REV_APPLE type for pixel storage
 *	operations.
 ********************************************************************************/

void
FskYUV420toUYVY(
	UInt32			width,
	UInt32			height,
	const UInt8		*srcY,
	const UInt8		*srcU,				/* Cb' */
	const UInt8		*srcV,				/* Cr' */
	SInt32			srcYRowBytes,
	SInt32			srcCRowBytes,
	UInt32			*dst32,				/* Cb' Y Cr' Y */
	SInt32			dstRowBytes
)
{
	UInt8		*dst1		= (UInt8*)dst32;
	UInt8		*dst0		= dst1  + dstRowBytes;
	const UInt8	*srcY1		= srcY;
	const UInt8	*srcY0		= srcY1 + srcYRowBytes;
	const UInt8	*srcU1		= srcU;
	const UInt8	*srcU0		= srcU;
	const UInt8	*srcV1		= srcV;
	const UInt8	*srcV0		= srcV;
	SInt32		dstBump, srcYBump, srcCBump, c0, c1, cd;
	UInt32		i;

	if (	((width  &= ~1) == 0)	/* Make sure the dimensions are even ... */
		||	((height &= ~1) == 0)	/* ... and at least 2x2 */
	)
		return;

	dstBump		= (dstRowBytes  << 1) - width * sizeof(UInt16);	/* Jump 2 lines at a time in the dst */
	srcYBump	= (srcYRowBytes << 1) - width * sizeof(UInt8);	/* Jump 2 lines at a time in the src Y */

	width  = (width  >> 1);		/* Do 2 pixels at a time horizontally */
	height = (height >> 1) - 1;	/* Do 2 pixels at a time vertically, except the first and last lines, which are done separately */

	srcCBump	=  srcCRowBytes       - width * sizeof(UInt8);	/* Jump 1 line  at a time in the src U and V */

	/* Do the first line, with no interpolation of chrominance */
	for (i = width; i--; ) {
		*dst1++ = *srcU1++;							/* U(0,0) */
		*dst1++ = *srcY1++;							/* Y(0,0) */
		*dst1++ = *srcV1++;							/* V(0,0) */
		*dst1++ = *srcY1++;							/* Y(0,1) */
	}

	/* Do the middle lines two at a time. We interpolate U and V to be 1/4 and 3/4 weights of the closest samples vertically */
	for(srcY1 += srcYBump, srcU1 += srcCBump, srcV1 += srcCBump, dst1 += dstBump; height--;
		srcY1 += srcYBump, srcU1 += srcCBump, srcV1 += srcCBump, dst1 += dstBump,
		srcY0 += srcYBump, srcU0 += srcCBump, srcV0 += srcCBump, dst0 += dstBump
	) {
		for (i = width; i--; ) {
			c0 = (SInt32)(*srcU0++);
			c1 = (SInt32)(*srcU1++);
			cd = ((c1 - c0 + (1 << (2 - 1))) >> 2);	/* (c1 - c0) / 4 */
			*dst0++ = c0 + cd;						/* U(0,0) */
			*dst1++ = c1 - cd;						/* U(1,0) */
			*dst0++ = *srcY0++;						/* Y(0,0) */
			*dst1++ = *srcY1++;						/* Y(1,0) */
			c0 = (SInt32)(*srcV0++);
			c1 = (SInt32)(*srcV1++);
			cd = ((c1 - c0 + (1 << (2 - 1))) >> 2);	/* (c1 - c0) / 4 */
			*dst0++ = c0 + cd;						/* V(0,0) */
			*dst1++ = c1 - cd;						/* V(1,0) */
			*dst0++ = *srcY0++;						/* Y(0,1) */
			*dst1++ = *srcY1++;						/* Y(1,1) */
		}
	}

	/* Do the last line, with no interpolation of chrominance */
	for (i = width; i--; ) {
		*dst0++ = *srcU0++;							/* U(0,0) */
		*dst0++ = *srcY0++;							/* Y(0,0) */
		*dst0++ = *srcV0++;							/* V(0,0) */
		*dst0++ = *srcY0++;							/* Y(0,1) */
	}
}
