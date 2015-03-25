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
#define __FSKVIDEOSPRITE_PRIV__
#include "FskYUV420Copy.h"
#include "FskVideoSprite.h"
#include "FskArch.h"
#include "FskMemory.h"

#if TARGET_OS_KPL
	#include "FskPlatformImplementation.Kpl.h"
#elif TARGET_OS_IPHONE || !TARGET_CPU_ARM
	#define YUV420i_RGB_C_IMPLEMENTATION
#else
	#define YUV420i_RGB_ARM_IMPLEMENTATION
#endif

//***for debug purpose
#ifdef FORCE_YUV420i_RGB_C_IMPLEMENTATION
#ifndef YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_RGB_C_IMPLEMENTATION
#endif
#endif


#define FIXED_SUBBITS	16					/**< The number of fractional bits in a standard 16.16 fixed point number. */
#define FWD_SUBBITS		kFskRectBlitBits	/**< The number of fractional bits used for  luma  forward differencing, as passed in FskRectBlitParams. */
#define CHR_SUBBITS		(FWD_SUBBITS + 1)	/**< The number of fractional bits used for chroma forward differencing, as passed in FskRectBlitParams. */
#define FWD_ONE			(1 << FWD_SUBBITS)	/**< One, as represented in the forward differencing data type. */
#define ASM_SUBBITS		16					/**< The number of fractional bits for  luma  coordinates in the assembler patch API. */
#define CAS_SUBBITS		(ASM_SUBBITS + 1)	/**< The number of fractional bits for chroma coordinates in the assembler patch API. */

#ifdef YUV420i_RGB_ARM_IMPLEMENTATION
#ifndef SUPPORT_YUV420_WMMX_OPT
extern void doCopyPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw);
#else
void doCopyPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw)
{
	int drb, srb;
	drb = dBump + dw;
	srb = sBump + dw;
	for ( ; dh--; s += srb, d += drb) {
		memcpy(d, s, dw);
	}
}
#endif
#else


static void doCopyAlignedPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw)
{
	for ( ; dh--; s += sBump, d += dBump) {
		register SInt32	k = dw;
		for (; k >= 32; k -= 32) {
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
			*(UInt32 *)d = *(UInt32 *)s; s += 4, d += 4;
		}
		for (; k >= 4; k -= 4, s += 4, d += 4)
			*(UInt32 *)d = *(UInt32 *)s;
		for (; k--; s++, d++)
			*d = *s;
	}
}

static void doCopyMisalignedPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw)
{
		SInt32	k;

		for ( ; dh--; s += sBump, d += dBump)							/*		+----> X      +----> X			*/
			for (k = dw; k--; s++, d++)									/*		| src         | dst				*/
				*d = *s;												/*		V Y           V					*/
}

void doCopyPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw)
{
	if (3 & ((long)d | (long)s | (long)sBump | (long)dBump))
		doCopyMisalignedPlane(s, sBump, d, dBump, dh, dw);
	else
		doCopyAlignedPlane(s, sBump, d, dBump, dh, dw);
}
#endif

void	FskYUV420Copy(
			UInt32			width,
			UInt32			height,
			const			UInt8 *srcY,
			const			UInt8 *srcU,
			const			UInt8 *srcV,
			SInt32			srcYRowBytes,
			SInt32			srcUVRowBytes,
			UInt8			*dstY,
			UInt8			*dstU,
			UInt8			*dstV,
			SInt32			dstYRowBytes,
			SInt32			dstUVRowBytes
		)
{
	if (!height || !width)
		return;

	// y
	doCopyPlane(srcY, srcYRowBytes - width, dstY, dstYRowBytes - width, height, width);

	// u & v
	width = (width + 1) >> 1;
	height = (height + 1) >> 1;

	doCopyPlane(srcU, srcUVRowBytes - width, dstU, dstUVRowBytes - width, height, width);
	doCopyPlane(srcV, srcUVRowBytes - width, dstV, dstUVRowBytes - width, height, width);
}


void ( *FskYUV420Interleave_CW000_universal)	( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb )=NULL;
void ( *FskYUV420Interleave_CW090_universal)	( unsigned char *y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb )=NULL;
void ( *FskYUV420Interleave_CW180_universal)	( unsigned char *y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb )=NULL;
void ( *FskYUV420Interleave_CW270_universal)	( unsigned char *y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb )=NULL;

#define FskYUV420Interleave_CW000_x				(*FskYUV420Interleave_CW000_universal)
#define FskYUV420Interleave_CW090_x				(*FskYUV420Interleave_CW090_universal)
#define FskYUV420Interleave_CW180_x				(*FskYUV420Interleave_CW180_universal)
#define FskYUV420Interleave_CW270_x				(*FskYUV420Interleave_CW270_universal)

void ( *FskYUV420iRotate_CW090_universal)		(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb ) = NULL;
void ( *FskYUV420iRotate_CW180_universal)		(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb ) = NULL;
void ( *FskYUV420iRotate_CW270_universal)		(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb ) = NULL;

#define FskYUV420iRotate_CW090_x				(*FskYUV420iRotate_CW090_universal)
#define FskYUV420iRotate_CW180_x				(*FskYUV420iRotate_CW180_universal)
#define FskYUV420iRotate_CW270_x				(*FskYUV420iRotate_CW270_universal)

#ifdef YUV420i_RGB_C_IMPLEMENTATION

void FskYUV420Interleave_CW000_c
(
	unsigned char *y0,
	unsigned char *u0,
	unsigned char *v0,
	unsigned char *yuv0,
	int height,
	int width,
	int yrb,
	int uvrb,
	int yuvrb
)
{
	int i, j;
	unsigned char *y1 = y0 + yrb;
	int y_stride  = (yrb<<1) - width;
	int uv_stride = uvrb - (width>>1);
	int yuv_stride = yuvrb - (width*3);

	for( j = 0; j < height; j += 2 )
	{
		for( i = 0; i < width; i += 2)
		{
			*(yuv0++) = *(u0++);
			*(yuv0++) = *(v0++);
			*(yuv0++) = *(y0++);
			*(yuv0++) = *(y0++);
			*(yuv0++) = *(y1++);
			*(yuv0++) = *(y1++);
		}

		u0 += uv_stride;
		v0 += uv_stride;
		y0 += y_stride;
		y1 += y_stride;
		yuv0 += yuv_stride;
	}
}

void FskYUV420Interleave_CW090_c
(
	unsigned char *y0,
	unsigned char *u0,
	unsigned char *v0,
	unsigned char *yuv00,
	int height,
	int width,
	int yrb,
	int uvrb,
	int yuvrb
)
{
	int i, j;
	unsigned char	*yuv;
	unsigned char	*yuv0	   = yuv00 + (((height>>1)-1)*6);
	unsigned char	*y1		   = y0 + yrb;
	int				y_stride   = (yrb<<1) - width;
	int				uv_stride  = uvrb - (width>>1);

	for( j = 0; j < height; j += 2 )
	{
		yuv = yuv0;
		for( i = 0; i < width; i += 2)
		{
			*(yuv+0) = *(u0++);
			*(yuv+1) = *(v0++);		//    4    2
			*(yuv+3) = *(y0++);		//		23
			*(yuv+5) = *(y0++);		//		45
			*(yuv+2) = *(y1++);		//    5    3
			*(yuv+4) = *(y1++);

			yuv += yuvrb;
		}

		u0 += uv_stride;
		v0 += uv_stride;
		y0 += y_stride;
		y1 += y_stride;
		yuv0 -= 6;
	}
}


void FskYUV420Interleave_CW180_c
(
	unsigned char *y0,
	unsigned char *u0,
	unsigned char *v0,
	unsigned char *yuv,
	int height,
	int width,
	int yrb,
	int uvrb,
	int yuvrb
)
{
	int i, j;
	unsigned char	*y1		   = y0 + yrb;
	int				y_stride   = (yrb<<1) - width;
	int				uv_stride  = uvrb  - (width>>1);
	int				yuv_stride = yuvrb - ((width>>1)*6);

	yuv += yuvrb*(height>>1) - yuv_stride - 6;

	for( j = 0; j < height; j += 2 )
	{
		for( i = 0; i < width; i += 2)
		{
			*(yuv+0) = *(u0++);
			*(yuv+1) = *(v0++);		//    4    2
			*(yuv+5) = *(y0++);		//		23
			*(yuv+4) = *(y0++);		//		45
			*(yuv+3) = *(y1++);		//    5    3
			*(yuv+2) = *(y1++);

			yuv -= 6;
		}

		u0  += uv_stride;
		v0  += uv_stride;
		y0  += y_stride;
		y1  += y_stride;
		yuv -= yuv_stride;
	}
}


void FskYUV420Interleave_CW270_c
(
	unsigned char *y0,
	unsigned char *u0,
	unsigned char *v0,
	unsigned char *yuv00,
	int height,
	int width,
	int yrb,
	int uvrb,
	int yuvrb
)
{
	int i, j;
	unsigned char	*yuv;
	unsigned char	*yuv0	   = yuv00 + (((width>>1)-1)*yuvrb);
	unsigned char	*y1		   = y0 + yrb;
	int				y_stride   = (yrb<<1) - width;
	int				uv_stride  = uvrb - (width>>1);

	for( j = 0; j < height; j += 2 )
	{
		yuv = yuv0;
		for( i = 0; i < width; i += 2)
		{
			*(yuv+0) = *(u0++);
			*(yuv+1) = *(v0++);		//    3    5
			*(yuv+4) = *(y0++);		//		23
			*(yuv+2) = *(y0++);		//		45
			*(yuv+5) = *(y1++);		//    2    4
			*(yuv+3) = *(y1++);

			yuv -= yuvrb;
		}

		u0 += uv_stride;
		v0 += uv_stride;
		y0 += y_stride;
		y1 += y_stride;
		yuv0 += 6;
	}
}


void rotate_yuv420i_cw090_c
(
	int height,
	int width,
	unsigned char *src,
	unsigned char *dst00,
	int src_rb,
	int dst_rb
)
{
	int i, j;
	unsigned char	*dst;
	unsigned char	*dst0	    = dst00  + (((height>>1)-1)*6);
	int				src_stride  = src_rb - ((width>>1)*6);

	for( j = 0; j < height; j += 2 )
	{
		dst = dst0;
		for( i = 0; i < width; i += 2)
		{
			*(dst+0) = *(src++);
			*(dst+1) = *(src++);		//    4    2
			*(dst+3) = *(src++);		//		23
			*(dst+5) = *(src++);		//		45
			*(dst+2) = *(src++);		//    5    3
			*(dst+4) = *(src++);

			dst += dst_rb;
		}

		src  += src_stride;
		dst0 -= 6;
	}
}


void rotate_yuv420i_cw180_c
(
	int height,
	int width,
	unsigned char *src,
	unsigned char *dst,
	int src_rb,
	int dst_rb
)
{
	int i, j;
	int	src_stride  = src_rb - ((width>>1)*6);
	int	dst_stride  = dst_rb - ((width>>1)*6);

	dst	+= dst_rb*(height>>1) - dst_stride - 6;

	for( j = 0; j < height; j += 2 )
	{
		for( i = 0; i < width; i += 2)
		{
			*(dst+0) = *(src++);
			*(dst+1) = *(src++);		//    5    4
			*(dst+5) = *(src++);		//		23
			*(dst+4) = *(src++);		//		45
			*(dst+3) = *(src++);		//    3    2
			*(dst+2) = *(src++);

			dst -= 6;
		}

		src  += src_stride;
		dst  -= dst_stride;
	}
}


#if 0
//a destination driven variation, similar performance, turned off for now
void rotate_yuv420i_cw270_c
(
	int height,
	int width,
	unsigned char *src00,
	unsigned char *dst,
	int src_rb,
	int dst_rb
)
{
	int i, j;
	unsigned char	*src;
	unsigned char	*src0	    = src00  + (((width>>1)-1)*6);
	int				dst_stride  = dst_rb - ((height>>1)*6);

	for( j = 0; j < width; j += 2 )
	{
		src = src0;
		for( i = 0; i < height; i += 2)
		{
			*(dst++) = *(src+0);
			*(dst++) = *(src+1);		//    4    2
			*(dst++) = *(src+3);		//		23
			*(dst++) = *(src+5);		//		45
			*(dst++) = *(src+2);		//    5    3
			*(dst++) = *(src+4);

			src += src_rb;
		}

		dst  += dst_stride;
		src0 -= 6;
	}
}
#else
void rotate_yuv420i_cw270_c
(
	int height,
	int width,
	unsigned char *src,
	unsigned char *dst00,
	int src_rb,
	int dst_rb
)
{
	int i, j;
	unsigned char	*dst;
	unsigned char	*dst0	   = dst00 + (((width>>1)-1)*dst_rb);
	int				src_stride = src_rb - ((width>>1)*6);
	int				dst_stride = dst_rb;

	for( j = 0; j < height; j += 2 )
	{
		dst = dst0;
		for( i = 0; i < width; i += 2)
		{
			*(dst+0) = *(src++);
			*(dst+1) = *(src++);		//    3    5
			*(dst+4) = *(src++);		//		23
			*(dst+2) = *(src++);		//		45
			*(dst+5) = *(src++);		//    2    4
			*(dst+3) = *(src++);

			dst -= dst_stride;
		}

		src  += src_stride;
		dst0 += 6;
	}
}
#endif

void ConfigCopyYUV420iProcs_c()
{
	FskYUV420Interleave_CW000_universal = FskYUV420Interleave_CW000_c;
	FskYUV420Interleave_CW090_universal = FskYUV420Interleave_CW090_c;
	FskYUV420Interleave_CW180_universal = FskYUV420Interleave_CW180_c;
	FskYUV420Interleave_CW270_universal = FskYUV420Interleave_CW270_c;

	FskYUV420iRotate_CW090_universal	= rotate_yuv420i_cw090_c;
	FskYUV420iRotate_CW180_universal	= rotate_yuv420i_cw180_c;
	FskYUV420iRotate_CW270_universal	= rotate_yuv420i_cw270_c;
}

#endif	//YUV420i_RGB_C_IMPLEMENTATION

#ifdef YUV420i_RGB_ARM_IMPLEMENTATION

extern void interleave_yuv420_misaligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420_aligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

extern void interleave_yuv420i_cw090_misaligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw270_misaligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_misaligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

extern void interleave_yuv420i_cw090_aligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw270_aligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_aligned_arm_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

extern void rotate_yuv420i_cw090_arm_v4(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
extern void rotate_yuv420i_cw180_arm_v4(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
extern void rotate_yuv420i_cw270_arm_v4(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );

extern void interleave_yuv420i_cw090_misaligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw270_misaligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_misaligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

extern void interleave_yuv420i_cw090_aligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw270_aligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_aligned_arm_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

extern void rotate_yuv420i_cw090_arm_v5(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
extern void rotate_yuv420i_cw180_arm_v5(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
extern void rotate_yuv420i_cw270_arm_v5(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );


//extern void interleave_yuv420i_cw090_misaligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_misaligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
//extern void interleave_yuv420i_cw270_misaligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);

//extern void interleave_yuv420i_cw090_aligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
extern void interleave_yuv420i_cw180_aligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);
//extern void interleave_yuv420i_cw270_aligned_arm_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	);


//extern void rotate_yuv420i_cw090_arm_v6(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
extern void rotate_yuv420i_cw180_arm_v6(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );
//extern void rotate_yuv420i_cw270_arm_v6(int height, int width, unsigned char *src, unsigned char *dst, int src_rb, int dst_rb );

#define interleave_yuv420i_cw090_aligned_arm_v4		interleave_yuv420i_cw090_misaligned_arm_v4
#define interleave_yuv420i_cw180_aligned_arm_v4		interleave_yuv420i_cw180_misaligned_arm_v4
#define interleave_yuv420i_cw270_aligned_arm_v4		interleave_yuv420i_cw270_misaligned_arm_v4

#define interleave_yuv420i_cw090_aligned_arm_v5		interleave_yuv420i_cw090_misaligned_arm_v5
#define interleave_yuv420i_cw180_aligned_arm_v5		interleave_yuv420i_cw180_misaligned_arm_v5
#define interleave_yuv420i_cw270_aligned_arm_v5		interleave_yuv420i_cw270_misaligned_arm_v5

//#define interleave_yuv420i_cw090_aligned_arm_v6		interleave_yuv420i_cw090_misaligned_arm_v6
#define interleave_yuv420i_cw180_aligned_arm_v6		interleave_yuv420i_cw180_misaligned_arm_v6
//#define interleave_yuv420i_cw270_aligned_arm_v6		interleave_yuv420i_cw270_misaligned_arm_v6


void FskYUV420Interleave_CW000_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420_misaligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420_aligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


void FskYUV420Interleave_CW090_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw090_misaligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw090_aligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


void FskYUV420Interleave_CW090_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw090_misaligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw090_aligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


//void FskYUV420Interleave_CW090_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
//{
//	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
//		interleave_yuv420i_cw090_misaligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
//	else
//		interleave_yuv420i_cw090_aligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
//}


void FskYUV420Interleave_CW180_v4( unsigned char	*y0, unsigned char	*u0, unsigned char *v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw180_misaligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw180_aligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


void FskYUV420Interleave_CW180_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw180_misaligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw180_aligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}

#ifndef SUPPORT_YUV420_WMMX_OPT
void FskYUV420Interleave_CW180_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw180_misaligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw180_aligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}
#endif

void FskYUV420Interleave_CW270_v4( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw270_misaligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw270_aligned_arm_v4(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


void FskYUV420Interleave_CW270_v5( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
{
	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
		interleave_yuv420i_cw270_misaligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
	else
		interleave_yuv420i_cw270_aligned_arm_v5(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
}


//void FskYUV420Interleave_CW270_v6( unsigned char	*y0, unsigned char	*u0, unsigned char	*v0, unsigned char	*yuv0, int height, int width, int yrb, int uvrb, int yuvrb	)
//{
//	if((((int)y0)&3)|(((int)u0)&3)|(((int)v0)&3) | (((int)yrb)&3) | (((int)uvrb)&3) )
//		interleave_yuv420i_cw270_misaligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
//	else
//		interleave_yuv420i_cw270_aligned_arm_v6(y0,u0,v0,yuv0,height,width,yrb,uvrb,yuvrb);
//}


void ConfigCopyYUV420iProcs( int implementation )
{
#ifdef SUPPORT_NEON
	if( implementation >= FSK_ARCH_ARM_V7 )
	{
		FskYUV420Interleave_CW000_universal = FskYUV420Interleave_CW000_v4;
		FskYUV420Interleave_CW090_universal = FskYUV420Interleave_CW090_v5;
		FskYUV420Interleave_CW180_universal = FskYUV420Interleave_CW180_v6;
		FskYUV420Interleave_CW270_universal = FskYUV420Interleave_CW270_v5;

		FskYUV420iRotate_CW090_universal	= rotate_yuv420i_cw090_arm_v5;
		FskYUV420iRotate_CW180_universal	= rotate_yuv420i_cw180_arm_v6;
		FskYUV420iRotate_CW270_universal	= rotate_yuv420i_cw270_arm_v5;
	}
	else
#endif
#ifndef SUPPORT_YUV420_WMMX_OPT
	if( implementation >= FSK_ARCH_ARM_V6 )
	{
		FskYUV420Interleave_CW000_universal = FskYUV420Interleave_CW000_v4;
		FskYUV420Interleave_CW090_universal = FskYUV420Interleave_CW090_v5;
		FskYUV420Interleave_CW180_universal = FskYUV420Interleave_CW180_v6;
		FskYUV420Interleave_CW270_universal = FskYUV420Interleave_CW270_v5;

		FskYUV420iRotate_CW090_universal	= rotate_yuv420i_cw090_arm_v5;
		FskYUV420iRotate_CW180_universal	= rotate_yuv420i_cw180_arm_v6;
		FskYUV420iRotate_CW270_universal	= rotate_yuv420i_cw270_arm_v5;
	}
	else
#endif	//SUPPORT_YUV420_WMMX_OPT
	if( implementation >= FSK_ARCH_ARM_V5 )
	{
		FskYUV420Interleave_CW000_universal = FskYUV420Interleave_CW000_v4;
		FskYUV420Interleave_CW090_universal = FskYUV420Interleave_CW090_v5;
		FskYUV420Interleave_CW180_universal = FskYUV420Interleave_CW180_v5;
		FskYUV420Interleave_CW270_universal = FskYUV420Interleave_CW270_v5;

		FskYUV420iRotate_CW090_universal	= rotate_yuv420i_cw090_arm_v5;
		FskYUV420iRotate_CW180_universal	= rotate_yuv420i_cw180_arm_v5;
		FskYUV420iRotate_CW270_universal	= rotate_yuv420i_cw270_arm_v5;
	}
	else //( implementation == FSK_ARCH_ARM_V4 )
	{
		FskYUV420Interleave_CW000_universal = FskYUV420Interleave_CW000_v4;
		FskYUV420Interleave_CW090_universal = FskYUV420Interleave_CW090_v4;
		FskYUV420Interleave_CW180_universal = FskYUV420Interleave_CW180_v4;
		FskYUV420Interleave_CW270_universal = FskYUV420Interleave_CW270_v4;

		FskYUV420iRotate_CW090_universal	= rotate_yuv420i_cw090_arm_v4;
		FskYUV420iRotate_CW180_universal	= rotate_yuv420i_cw180_arm_v4;
		FskYUV420iRotate_CW270_universal	= rotate_yuv420i_cw270_arm_v4;
	}
}

#endif	//YUV420i_RGB_ARM_IMPLEMENTATION

void FskYUV420Interleave_Generic
(
	unsigned char *y0,
	unsigned char *u0,
	unsigned char *v0,
	unsigned char *yuv0,
	int height,
	int width,
	int yrb,
	int uvrb,
	int yuvrb,
	int	rotation
)
{
	if( rotation == kRotationNone )
		FskYUV420Interleave_CW000_x
				(
					y0,
					u0,
					v0,
					yuv0,
					height,
					width,
					yrb,
					uvrb,
					yuvrb
				);
	else if( rotation == kRotationCW90 )
		FskYUV420Interleave_CW090_x
				(
					y0,
					u0,
					v0,
					yuv0,
					height,
					width,
					yrb,
					uvrb,
					yuvrb
				);
	else if( rotation == kRotationCW180 )
		FskYUV420Interleave_CW180_x
				(
					y0,
					u0,
					v0,
					yuv0,
					height,
					width,
					yrb,
					uvrb,
					yuvrb
				);
	else if( rotation == kRotationCW270 )
		FskYUV420Interleave_CW270_x
				(
					y0,
					u0,
					v0,
					yuv0,
					height,
					width,
					yrb,
					uvrb,
					yuvrb
				);
}


void FskYUV420iRotate_Generic
(
	int height,
	int width,
	unsigned char *src,
	unsigned char *dst,
	int src_rb,
	int dst_rb,
	int	rotation
)
{
	if( rotation == kRotationCW90 )
		FskYUV420iRotate_CW090_x
				(
					height,
					width,
					src,
					dst,
					src_rb,
					dst_rb
				);
	else if( rotation == kRotationCW180 )
		FskYUV420iRotate_CW180_x
				(
					height,
					width,
					src,
					dst,
					src_rb,
					dst_rb
				);
	else if( rotation == kRotationCW270 )
		FskYUV420iRotate_CW270_x
				(
					height,
					width,
					src,
					dst,
					src_rb,
					dst_rb
				);
}


#include "FskRectBlitPatch.h"

MY_STATIC unsigned char *g_blockPatternPtr  = NULL;
MY_STATIC int			g_pattern_buf_size	= 0;
MY_STATIC int			g_clipped_dst_width	= 0;
MY_STATIC int			last_srcX0			= -1;
MY_STATIC int			last_srcY0			= -1;
MY_STATIC int			last_srcXInc		= -1;
MY_STATIC int			last_srcYInc		= -1;
MY_STATIC int			last_dstWidth		= -1;
MY_STATIC int			last_dstHeight		= -1;
MY_STATIC int			last_dstRowBytes	= -1;
MY_STATIC FskBitmapFormatEnum   last_dstPixelFormat	= (FskBitmapFormatEnum)(-1);

MY_STATIC int			g_current_is_upscale= -1;
MY_STATIC unsigned char *g_current_pattern  = NULL;
MY_STATIC int			g_current_pattern_size = 0;

MY_STATIC unsigned char *g_sprite_blockPatternPtr	= NULL;
MY_STATIC int			g_sprite_pattern_buf_size	= 0;
MY_STATIC unsigned char *g_sprite_back_buffer		= NULL;		//short offscreen buffer but as wide as the screen to do offscreen alpha blending
MY_STATIC unsigned char *g_sprite_back_buffer_x 	= NULL;		//short offscreen buffer but as wide as the screen to do offscreen alpha blending
MY_STATIC int			g_sprite_back_buffer_size	= 0;

static unsigned char downscale_pattern_width[22]     = {0};
static unsigned char downscale_pattern_height[22]    = {0};
static unsigned char upscale_pattern_width[52]       = {0};
static unsigned char upscale_pattern_height[52]		 = {0};
static unsigned char DOWNSCALE_SHAPE_TO_PATTERNS[16] = {0};
static unsigned char UPSCALE_SHAPE_TO_PATTERNS[  76] = {0};

void create_context()
{
	int i;
	int block_width;
	int block_height;

#define GET_BLOCK_WIDTH(c)  (((c>>3)&0x01)+((c>>2)&0x01) )
#define GET_BLOCK_HEIGHT(c) (((c>>1)&0x01)+((c>>0)&0x01) )

	//0x~1x
	for( i = 0; i < 16; i++ )
	{
		int pattern;

		block_width = GET_BLOCK_WIDTH(i);
		block_height= GET_BLOCK_HEIGHT(i);

		switch( i )
		{
			case kDownScaleShape___0:
			case kDownScaleShape___4:
			case kDownScaleShape___8:
			case kDownScaleShape___12:
				block_width  = 0;
				block_height = 0;
				pattern = kDownScalePattern_EOL_0;	break;
			case kDownScaleShape___1:
			case kDownScaleShape___2:
			case kDownScaleShape___3:
				block_width  = 0;
				block_height = 0;
				pattern = kDownScalePattern_SKIP;	break;
			case kDownScaleShape___5:
				pattern = kDownScalePattern_5;		break;
			case kDownScaleShape___6:
				pattern = kDownScalePattern_6;		break;
			case kDownScaleShape___7:
				pattern = kDownScalePattern_7;		break;
			case kDownScaleShape___9:
				pattern = kDownScalePattern_9;		break;
			case kDownScaleShape___10:
				pattern = kDownScalePattern_10;		break;
			case kDownScaleShape___11:
				pattern = kDownScalePattern_11;		break;
			case kDownScaleShape___13:
				pattern = kDownScalePattern_13;		break;
			case kDownScaleShape___14:
				pattern = kDownScalePattern_14;		break;
			case kDownScaleShape___15:
				pattern = kDownScalePattern_15;		break;
		}

		DOWNSCALE_SHAPE_TO_PATTERNS[i]   = pattern;
		downscale_pattern_width[pattern] = block_width;		//range: 0~15, 16,17,18,  19,20,21
		downscale_pattern_height[pattern]= block_height;
	}

	downscale_pattern_width[ kDownScalePattern_HQ_1]	 = 1;
	downscale_pattern_height[kDownScalePattern_HQ_1]	 = 1;
	downscale_pattern_width[ kDownScalePattern_HQ_2_HOR] = 2;
	downscale_pattern_height[kDownScalePattern_HQ_2_HOR] = 1;
	downscale_pattern_width[ kDownScalePattern_HQ_2_VER] = 1;
	downscale_pattern_height[kDownScalePattern_HQ_2_VER] = 2;

	//1x~2x
	for( i = 15; i <= 30; i++ )
	{
		int pattern;
		int blk = i/15;
		int iii = i%15;

		block_width = GET_BLOCK_WIDTH( iii) + (blk*2);
		block_height= GET_BLOCK_HEIGHT(iii) + (blk*2);

		pattern	= i - 15 + kUpScalePattern_1;	//simple mapping

		UPSCALE_SHAPE_TO_PATTERNS[i] = pattern;
		upscale_pattern_width [pattern]	 = block_width ;
		upscale_pattern_height[pattern]	 = block_height;
	}

	//2x +
	for( i = 31; i <= 75; i++ )
	{
		int pattern = 0;
		int blk = i/15;
		int iii = i%15;

		block_width = GET_BLOCK_WIDTH( iii) + (blk*2);
		block_height= GET_BLOCK_HEIGHT(iii) + (blk*2);

		switch( i )
		{
			case kUpScaleShape___1:
				pattern = kUpScalePattern_16_4x4;	break;
			case kUpScaleShape___2:
			case kUpScaleShape___3:
				pattern = kUpScalePattern_4x5;	break;
			case kUpScaleShape___4:
				pattern = kUpScalePattern_4x6;	break;
			case kUpScaleShape___5:
			case kUpScaleShape___9:
				pattern = kUpScalePattern_5x4;	break;
			case kUpScaleShape___6:
			case kUpScaleShape___7:
			case kUpScaleShape___10:
			case kUpScaleShape___11:
				pattern = kUpScalePattern_5x5;	break;
			case kUpScaleShape___8:
			case kUpScaleShape___12:
				pattern = kUpScalePattern_5x6;	break;
			case kUpScaleShape___13:
				pattern = kUpScalePattern_6x4;	break;
			case kUpScaleShape___14:
			case kUpScaleShape___15:
				pattern = kUpScalePattern_6x5;	break;
			case kUpScaleShape___16:
				pattern = kUpScalePattern_6x6;	break;
			case kUpScaleShape___17:
			case kUpScaleShape___18:
				pattern = kUpScalePattern_6x7;	break;
			case kUpScaleShape___19:
				pattern = kUpScalePattern_6x8;	break;
			case kUpScaleShape___24:
			case kUpScaleShape___20:
				pattern = kUpScalePattern_7x6;	break;
			case kUpScaleShape___21:
			case kUpScaleShape___22:
			case kUpScaleShape___25:
			case kUpScaleShape___26:
				pattern = kUpScalePattern_7x7;	break;
			case kUpScaleShape___23:
			case kUpScaleShape___27:
				pattern = kUpScalePattern_7x8;	break;
			case kUpScaleShape___28:
				pattern = kUpScalePattern_8x6;	break;
			case kUpScaleShape___29:
			case kUpScaleShape___30:
				pattern = kUpScalePattern_8x7;	break;
			case kUpScaleShape___31:
				pattern = kUpScalePattern_8x8;	break;
			case kUpScaleShape___32:
			case kUpScaleShape___33:
				pattern = kUpScalePattern_8x9;	break;
			case kUpScaleShape___34:
				pattern = kUpScalePattern_8x10;	break;
			case kUpScaleShape___35:
			case kUpScaleShape___39:
				pattern = kUpScalePattern_9x8;	break;
			case kUpScaleShape___36:
			case kUpScaleShape___37:
			case kUpScaleShape___40:
			case kUpScaleShape___41:
				pattern = kUpScalePattern_9x9;	break;
			case kUpScaleShape___38:
			case kUpScaleShape___42:
				pattern = kUpScalePattern_9x10;	break;
			case kUpScaleShape___43:
				pattern = kUpScalePattern_10x8;	break;
			case kUpScaleShape___44:
			case kUpScaleShape___45:
				pattern = kUpScalePattern_10x9;	break;
			case kUpScaleShape___46:
				pattern = kUpScalePattern_10x10;	break;
		}

		UPSCALE_SHAPE_TO_PATTERNS[i] = pattern;
		upscale_pattern_width[pattern] = block_width;
		upscale_pattern_height[pattern]= block_height;
	}
}

void upgrade_2x2_downscale_pattern(	unsigned char *p )
{
	p += 4;
	while(1)
	{
		unsigned char v = *p;

		if( v == kCommonScalePattern_EOF )
			break;

		if( v == kDownScalePattern_10 || v == kDownScalePattern_6 ||
		    v == kDownScalePattern_9  || v == kDownScalePattern_5 )
			*p = kDownScalePattern_HQ_1;		//1 in 4
		else if( v == kDownScalePattern_14 || v == kDownScalePattern_13 )
			*p = kDownScalePattern_HQ_2_HOR;		// 2 in 4 hor
		else if( v == kDownScalePattern_11 || v == kDownScalePattern_7 )
			*p = kDownScalePattern_HQ_2_VER;		// 2 in 4 ver

		p++;
	}
}


#define GENERIC_BLOCK_FLAG	0x80
#define GENERIC_BLOCK_MASK	0x7f

#define PACK_GENERIC_BLOCK( merge_block, h0, h1, v0, v1, pt )	\
{														\
	int x0 = h0;										\
	int y0 = v0;										\
	int x1 = h1;										\
	int y1 = v1;										\
														\
	if( h0&GENERIC_BLOCK_FLAG )							\
		x0 = h0&GENERIC_BLOCK_MASK;						\
	else if( h1 > h0 && merge_block)					\
	{													\
		x0 = h1;										\
		x1 = h0;										\
	}													\
														\
	if( v0&GENERIC_BLOCK_FLAG )							\
		y0 = v0&GENERIC_BLOCK_MASK;						\
	else if( v1 > v0 && merge_block)					\
	{													\
		y0 = v1;										\
		y1 = v0;										\
	}													\
														\
	if( (x0 + x1)>0 && (y0+y1)>0 )						\
	{													\
		*pt++ = kUpScalePattern_GENERIC_BLOCK;			\
		*pt++ = x0;										\
		*pt++ = x1;										\
		*pt++ = y0;										\
		*pt++ = y1;										\
	}													\
}


#define PACK_GENERIC_BLOCK2( h0, h1, pt )		\
{														\
	if( (h0 + h1)>0 )									\
	{													\
		*pt++ = kUpScalePattern_GENERIC_BLOCK;			\
		*pt++ = h0;										\
		*pt++ = h1;										\
	}													\
}

#define REORDER_LOW_TO_HIGH( v0, v1 )					\
{														\
	if( v0 > v1 )										\
	{													\
		int tmp = v1;									\
		v1 = v0;										\
		v0 = tmp;										\
	}													\
}

void fix_boundary( unsigned char *pix_x, int idx_x_max, unsigned char *pix_y, int idx_y_max, int *s_max, int *s_min )
{
	int i;
	int x_max = 0;
	int y_max = 0;
	int this_s_max = *s_max;

	for( i = 0; i < idx_x_max; i++ )
	{
		if( pix_x[i] > x_max )
			x_max = pix_x[i];
	}

	for( i = 0; i < idx_y_max; i++ )
	{
		if( pix_y[i] > y_max )
			y_max = pix_y[i];
	}

	if( this_s_max > x_max &&  this_s_max > y_max && this_s_max > 1 )
	{
		*s_max = this_s_max - 1;
		*s_min = this_s_max - 2;
	}
}



#ifdef SUPPORT_NEON

#define CREATE_DOWNSCALE_4X2														\
if( count_2x2 > 0 )																	\
{																					\
	int count_near = (count_2x2)/2;													\
	int count1	   = (count_2x2)%2;													\
																					\
	while( count_near )																\
	{																				\
		*dst++ = kDownScalePattern_4x2;												\
		count_near--;																\
	}																				\
																					\
	if( count1 )																	\
		*dst++ = kDownScalePattern_15;												\
																					\
	count_2x2 = 0;																	\
}


int combine_to_4x2_downscale_pattern
(
	unsigned char	*pattern_in_out,
	int				*buf_size_in_out
)
{
	unsigned char *src = pattern_in_out;
	unsigned char *dst = pattern_in_out;
	int	count_2x2 = 0;
	int err = 0;

	//first 4 bytes is width
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;

	while(1)
	{
		int this_pattern = *src++;

		if( this_pattern == kDownScalePattern_15 )	//2x2
			count_2x2++;
		else
		{
			if( count_2x2 > 0 )
			{
				int count_2 = (count_2x2)/2;
				int count_1 = (count_2x2)%2;

				while( count_2 )
				{
					*dst++ = kDownScalePattern_4x2;
					count_2--;
				}

				if( count_1 )
					*dst++ = kDownScalePattern_15;

				count_2x2 = 0;
			}
			*dst++ = this_pattern;
		}

		if( this_pattern == kCommonScalePattern_EOF )
			break;
	}

	*buf_size_in_out = dst - pattern_in_out;

	return err;
}

#define PACK_NEON1_GENERIC_2x2_BLOCK( h0, h1, pt )		\
{														\
		*pt++ = kUpScalePattern_Neon1_GENERIC_2x2;		\
		*pt++ = h0;										\
		*pt++ = h1;										\
}

#define PACK_NEON1_GENERIC_2x1_TOP_BLOCK( h0, h1, pt )	\
{														\
		*pt++ = kUpScalePattern_Neon1_GENERIC_2x1_TOP;		\
		*pt++ = h0;										\
		*pt++ = h1;										\
}

#define PACK_NEON1_GENERIC_2x1_BOT_BLOCK( h0, h1, pt )	\
{														\
		*pt++ = kUpScalePattern_Neon1_GENERIC_2x1_BOT;		\
		*pt++ = h0;										\
		*pt++ = h1;										\
}


int get_4x2_upscale_pattern
(
	unsigned char *pix_x,
	unsigned char *pix_y,
	int idx_x_max,
	int idx_y_max,
	unsigned char	**pattern_in_out
)
{
	unsigned char	*pattern = *pattern_in_out;
	int	j, i;
	int err = 0;

	for( j = 0; j < idx_y_max; j += 2 )
	{
		int v0 = pix_y[j]&GENERIC_BLOCK_MASK;
		int v1 = pix_y[j+1];
		int generic_v = pix_y[j]&GENERIC_BLOCK_FLAG;
		int sum_v = v0 + v1;
		if( sum_v < 1 )
			continue;

		if( v1 != 0 )
			REORDER_LOW_TO_HIGH( v0, v1 );

		//embed the structure
		*pattern++ = v0;
		*pattern++ = v1;

		i = 0;
		while( i <= idx_x_max - 2 )
		{
			int h0 = pix_x[i]&GENERIC_BLOCK_MASK;
			int h1 = pix_x[i+1];
			int sum_h = h0 + h1;
			int generic_h0 = pix_x[i]&GENERIC_BLOCK_FLAG;

			if( generic_v || generic_h0 || i >= idx_x_max - 2 )	//deal with last singled out 2x2
			{
				PACK_GENERIC_BLOCK2( h0, h1, pattern )

				i+= 2;
			}
			else
			{
				int h2 = pix_x[i+2]&GENERIC_BLOCK_MASK;
				int h3 = pix_x[i+3];
				int generic_h2 = pix_x[i+2]&GENERIC_BLOCK_FLAG;

				sum_h += h2 + h3;

				if( generic_h2 )
				{
					PACK_GENERIC_BLOCK2( h0, h1, pattern )
					PACK_GENERIC_BLOCK2( h2, h3, pattern )
				}
				else if( sum_h == 4 && sum_v == 2 ) *pattern++ = kUpScalePattern_Neon1_4x2;
				else if( sum_h == 4 && sum_v == 3 ) *pattern++ = kUpScalePattern_Neon1_4x3;
				else if( sum_h == 5 && sum_v == 2 ) *pattern++ = kUpScalePattern_Neon1_5x2;
				else if( sum_h == 5 && sum_v == 3 ) *pattern++ = kUpScalePattern_Neon1_5x3;
				else if( sum_h == 5 && sum_v == 4 ) *pattern++ = kUpScalePattern_Neon1_5x4;
				else if( sum_h == 6 && sum_v == 2 ) *pattern++ = kUpScalePattern_Neon1_6x2;
				else if( sum_h == 6 && sum_v == 3 ) *pattern++ = kUpScalePattern_Neon1_6x3;
				else if( sum_h == 6 && sum_v == 4 ) *pattern++ = kUpScalePattern_Neon1_6x4;
				else if( sum_h == 7 && sum_v == 3 ) *pattern++ = kUpScalePattern_Neon1_7x3;
				else if( sum_h == 7 && sum_v == 4 ) *pattern++ = kUpScalePattern_Neon1_7x4;
				else if( sum_h == 7 && sum_v == 5 ) *pattern++ = kUpScalePattern_Neon1_7x5;
				else if( sum_h == 8 && sum_v == 3 ) *pattern++ = kUpScalePattern_Neon1_8x3;
				else if( sum_h == 8 && sum_v == 4 ) *pattern++ = kUpScalePattern_Neon1_8x4;
				else if( sum_h == 8 && sum_v == 5 ) *pattern++ = kUpScalePattern_Neon1_8x5;
				else if( sum_h == 9 && sum_v == 4 ) *pattern++ = kUpScalePattern_Neon1_9x4;
				else if( sum_h == 9 && sum_v == 5 ) *pattern++ = kUpScalePattern_Neon1_9x5;
				else if( sum_h == 9 && sum_v == 6 ) *pattern++ = kUpScalePattern_Neon1_9x6;
				else
				{
					PACK_GENERIC_BLOCK2( h0, h1, pattern )
					PACK_GENERIC_BLOCK2( h2, h3, pattern )
				}

				i+= 4;
			}
		}

		*pattern++ = kUpScalePattern_GENERIC_EOL;
	}

	*pattern++ = kCommonScalePattern_EOF;//to fill up v0, v1 that are aways following following kUpScalePattern_GENERIC_COPY_EOL
	*pattern++ = kCommonScalePattern_EOF;

	*pattern_in_out = pattern;

	return err;
}

int get_4x2_upscale_pattern_copy
(
	unsigned char *pix_x,
	unsigned char *pix_y,
	int idx_x_max,
	int idx_y_max,
	unsigned char	**pattern_in_out
)
{
	unsigned char	*pattern = *pattern_in_out;
	int	j, i;
	int err = 0;

	for( j = 0; j < idx_y_max; j += 2 )
	{
		int v0 = pix_y[j]&GENERIC_BLOCK_MASK;
		int v1 = pix_y[j+1];

		if( v0 == 0 && v1 ==  0 )
			continue;

		//embed the structure
		*pattern++ = v0;
		*pattern++ = v1;
		if( v1 == 0 )
			for( i = 0; i < idx_x_max; i += 4 )
			{
				int h0 = pix_x[i]&GENERIC_BLOCK_MASK;
				int h1 = pix_x[i+1];
				int sum_h = h0 + h1;

				if( (i+2 >= idx_x_max) && (sum_h > 0) )
				{
					PACK_NEON1_GENERIC_2x1_TOP_BLOCK( h0, h1,pattern )
				}
				else
				{
					int h2 = pix_x[i+2]&GENERIC_BLOCK_MASK;
					int h3 = pix_x[i+3];

					PACK_NEON1_GENERIC_2x1_TOP_BLOCK( h0, h1,pattern )
					PACK_NEON1_GENERIC_2x1_TOP_BLOCK( h2, h3,pattern )
				}
			}
		else if( v0 == 0 )
			for( i = 0; i < idx_x_max; i += 4 )
			{
				int h0 = pix_x[i]&GENERIC_BLOCK_MASK;
				int h1 = pix_x[i+1];
				int sum_h = h0 + h1;

				if( (i+2 >= idx_x_max) && (sum_h > 0) )
				{
					PACK_NEON1_GENERIC_2x1_BOT_BLOCK( h0, h1,pattern )
				}
				else
				{
					int h2 = pix_x[i+2]&GENERIC_BLOCK_MASK;
					int h3 = pix_x[i+3];

					PACK_NEON1_GENERIC_2x1_BOT_BLOCK( h0, h1,pattern )
					PACK_NEON1_GENERIC_2x1_BOT_BLOCK( h2, h3,pattern )
				}
			}
		else
			for( i = 0; i < idx_x_max; i += 4 )
			{
				int h0 = pix_x[i]&GENERIC_BLOCK_MASK;
				int h1 = pix_x[i+1];
				int generic_h0 = pix_x[i]&GENERIC_BLOCK_FLAG;
				int sum_h = h0 + h1;

				if( (i+2 >= idx_x_max) && (sum_h > 0) )
				{
					PACK_NEON1_GENERIC_2x2_BLOCK( h0, h1,pattern )
				}
				else
				{
					int h2 = pix_x[i+2]&GENERIC_BLOCK_MASK;
					int h3 = pix_x[i+3];
					int generic_h2 = pix_x[i+2]&GENERIC_BLOCK_FLAG;

					sum_h += h2 + h3;

					if(
						sum_h >= kUpScalePattern_Neon1_Width_Offset &&
						sum_h <= kUpScalePattern_Neon1_HScale_Max	&&
						!generic_h0									&&
						!generic_h2
					   )
						*pattern++ = sum_h - kUpScalePattern_Neon1_Width_Offset;
					else
					{
						PACK_NEON1_GENERIC_2x2_BLOCK( h0, h1,pattern )
						PACK_NEON1_GENERIC_2x2_BLOCK( h2, h3,pattern )
					}
				}
			}

		if( v0 > 1 && v1 > 1 )
			*pattern++ = kUpScalePattern_GENERIC_COPY_BOTH_EOL;
		else if( v0 > 1 && v1 <= 1 )
			*pattern++ = kUpScalePattern_GENERIC_COPY_TOP_EOL;
		else if( v0 <= 1 && v1 > 1 )
			*pattern++ = kUpScalePattern_GENERIC_COPY_BOT_EOL;
		else
			*pattern++ = kUpScalePattern_GENERIC_EOL;
	}

	*pattern++ = kCommonScalePattern_EOF;//to fill up v0, v1 that are aways following following kUpScalePattern_GENERIC_COPY_EOL
	*pattern++ = kCommonScalePattern_EOF;
	*pattern++ = kCommonScalePattern_EOF;
	*pattern++ = kCommonScalePattern_EOF;

	*pattern_in_out = pattern;

	return err;
}

#endif

#if TARGET_OS_ANDROID
#include "FskArch.h"
#endif

int get_2x2_scale_pattern
(
	int				dst_width,
	int				dst_height,
	FskFixed		x0,	//whole offset: integer+frac, with FWD_SUBBITS fractional bits
	FskFixed		y0,
	FskFixed		xd,
	FskFixed		yd,
	unsigned char	**pattern_in_out,
	int				*buf_size_in_out,
	int				*clipped_dst_width_out,
	int				*is_upscale_out
)
{
	int	j, i;
	int is_upscale= 0;
	int is_over2x = 0;
	int is_over5x = 0;
	int	x		  = x0 - ((x0 >> CHR_SUBBITS) << CHR_SUBBITS);	//sub-even
	int y		  = y0 - ((y0 >> CHR_SUBBITS) << CHR_SUBBITS);
	int idx_x_max = (x + dst_width *xd) >> FWD_SUBBITS;
	//int idx_x_maxy= (x + dst_width *yd) >> FWD_SUBBITS;
	int idx_y_max = (y + dst_height*yd) >> FWD_SUBBITS;
	//int idx_y_maxx= (y + dst_height*xd) >> FWD_SUBBITS;
	int	mix_mode = 0;
	int s_min	  = 0;
	int s_max	  = 0;
	unsigned char *pix_x = 0, *pix_y = 0;
	unsigned char *pattern;
#if TARGET_OS_ANDROID
	int has_neon = FskHardwareGetARMCPU_All() == FSK_ARCH_ARM_V7;
#elif defined(SUPPORT_NEON)
	int has_neon = 0;
#endif

	int err = 0;

	if( UPSCALE_SHAPE_TO_PATTERNS[0] == 0 )
		create_context();

	{
		int med_mi;
		int med_ma = 0x7fffffff;
		for( i = 1; i <100; i++ )
		{
			med_mi = FWD_ONE / i;

			if( xd >= med_mi && xd <= med_ma )
			{
				s_max = i;
				s_min = s_max-1;
				break;
			}
			med_ma = med_mi;
		}
	}

	{
		int med_mi;
		int med_ma = 0x7fffffff;
		int s_max_y= 0;
		int s_min_y= 0;
		for( i = 1; i <100; i++ )
		{
			med_mi = FWD_ONE / i;

			if( yd >= med_mi && yd <= med_ma )
			{
				s_max_y = i;
				s_min_y = s_max_y-1;
				break;
			}
			med_ma = med_mi;
		}

		if( s_max_y - s_max > 1  ||
			s_max_y - s_max < -1 ||
			s_min_y - s_min > 1  ||
			s_min_y - s_min < -1 )
		{

			//fprintf( stderr, "failure: too much x, y sclaing difference!!!\n");
			return -1;
		}

		if( (xd > FWD_ONE && yd < FWD_ONE ) ||
			(xd < FWD_ONE && yd > FWD_ONE ) )
		{
			int ddd = (int)xd - (int)yd;

			if( ddd < 0 )
				ddd = -ddd;

			if( ddd > 0x00000800 )
				return -1;
			else
				mix_mode = 1;
		}

		if( s_max <= 1 && s_max_y > 1 )
		{	//no cross dressing!!!
			s_max = s_max_y;
			s_min = s_max-1;
		}

	}

	idx_x_max = ((idx_x_max+1)>>1)<<1;	//even width/height in yuv space
	idx_y_max = ((idx_y_max+1)>>1)<<1;
	idx_x_max += 2;							//plus 2 might cross boundary(removed if needed)
	idx_y_max += 2;

	{
		int x_size = idx_x_max;
		int y_size = idx_y_max;
		int pattern_size = 0;
		int total_size   = 0;

#define GENERIC_BLOCK_BYTES		5
#define GENERIC_EOL_BYTES		2

		if( s_min >= 5 || mix_mode )		//all generic blocks
			pattern_size = 4 +	//embedded src_width
						   (
								(idx_x_max>>1)*GENERIC_BLOCK_BYTES +
								GENERIC_EOL_BYTES
							) * (idx_y_max>>1) +
						   1;	//EOF
		else if( s_min >= 1 )	//mix
			pattern_size = 4 +	//embedded src_width
						   (
								(idx_x_max>>1)			+	//pattern
								(2*GENERIC_BLOCK_BYTES) +	//2 possible generic pattern
								GENERIC_EOL_BYTES
							) * (idx_y_max>>1)+
						   (2 * (idx_x_max>>1)*GENERIC_BLOCK_BYTES) + //2 rows of possible generic pattern
							1;	//EOF
		else					//no generic blocks
			pattern_size = 4 + ((idx_x_max>>1) + 1) * (idx_y_max>>1) + 1;//first 4 bytes to save src_width, 1 more EOL at the end of each line, 1 bytes at the end of of the frame

		total_size = pattern_size + x_size + y_size;

		if( *buf_size_in_out < total_size  )
		{
			if( *pattern_in_out != NULL )
			{
				FskMemPtrDispose( *pattern_in_out );
				*pattern_in_out = NULL;
			}

			pattern = FskMemPtrAlloc( total_size );
			if( pattern == NULL )
			{ err = -1;	goto bail;}

			*pattern_in_out	 = pattern;
			*buf_size_in_out = total_size;
		}

		pattern = *pattern_in_out;
		pix_x   = pattern + pattern_size;
		pix_y   = pix_x   + x_size;
	}

	//horizontal
	for( i = 0; i < idx_x_max; i++ )
		pix_x[i] = 0;

	for( i = 0; i < dst_width; i++ )
	{
		int adr = x >> FWD_SUBBITS;
		int v = pix_x[adr];
		v++;
		pix_x[adr] = v;
		x += xd;
	}

	//vertical
	for( i = 0; i < idx_y_max; i++ )
		pix_y[i] = 0;

	for( i = 0; i < dst_height; i++ )
	{
		int adr = y >> FWD_SUBBITS;
		int v = pix_y[adr];
		v++;
		pix_y[adr] = v;
		y += yd;
	}

	fix_boundary( pix_x, idx_x_max, pix_y, idx_y_max, &s_max, &s_min );

	is_upscale = s_min>=1;
	is_over2x  = s_min>=2;
	is_over5x  = s_min>=5;

	if( is_upscale )	//upscale cases
	{
		for( i = 0; i < idx_x_max; i+=2 )
		{
			int h0 = pix_x[i];
			int h1 = pix_x[i+1];

			if( h0 < s_min || h0 > s_max || h1 < s_min || h1 > s_max )
			{
				h0 |= GENERIC_BLOCK_FLAG;
				pix_x[i] = h0;
			}
		}

		for( i = 0; i < idx_y_max; i+=2 )
		{
			int v0 = pix_y[i];
			int v1 = pix_y[i+1];

			if( v0 < s_min || v0 > s_max || v1 < s_min || v1 > s_max )
			{
				v0 |= GENERIC_BLOCK_FLAG;
				pix_y[i] = v0;
			}
		}
	}

	if( (pix_x[idx_x_max-2]&GENERIC_BLOCK_MASK) + pix_x[idx_x_max-1] == 0 )
		idx_x_max -= 2;

	if( (pix_y[idx_y_max-2]&GENERIC_BLOCK_MASK) + pix_y[idx_y_max-1] == 0 )
		idx_y_max -= 2;

	//embed source width in the first 4 bytes of the pattern array
	*((SInt32 *)pattern) = idx_x_max;
	pattern += 4;

	if( is_upscale == 0 )
	{
		for( j = 0; j < idx_y_max; j += 2 )
		{
			int v0 = pix_y[j];
			int v1 = pix_y[j+1];
			int ver_count = v0 + v1;
			int this_pattern;

			for( i = 0; i < idx_x_max; i += 2 )
			{
				int h0 = pix_x[i];
				int h1 = pix_x[i+1];
				int p = PACK_4( h0,h1,v0,v1 );

				this_pattern = DOWNSCALE_SHAPE_TO_PATTERNS[p];
				if( this_pattern == kDownScalePattern_EOL_0 )
					break;

				*pattern++ = this_pattern;
			}

			if( ver_count == 0 )
				*pattern++ = kDownScalePattern_EOL_0;
			else if(  ver_count == 1 )
				*pattern++ = kDownScalePattern_EOL_1;
			else
				*pattern++ = kDownScalePattern_EOL_2;
		}
	}
	else
	{
#ifdef SUPPORT_NEON
		if( has_neon )
		{
			if( is_over2x )
				err = get_4x2_upscale_pattern_copy( pix_x, pix_y, idx_x_max, idx_y_max, &pattern );
			else
				err = get_4x2_upscale_pattern( pix_x, pix_y, idx_x_max, idx_y_max, &pattern );
			goto bail;
		}
#endif

		for( j = 0; j < idx_y_max; j += 2 )
		{
			int v0 = pix_y[j];
			int v1 = pix_y[j+1];
			int ver_count = (v0&GENERIC_BLOCK_FLAG) ? (v0&GENERIC_BLOCK_MASK) + v1 : v0 + v1;
			int this_pattern;

			for( i = 0; i < idx_x_max; i += 2 )
			{
				int h0 = pix_x[i];
				int h1 = pix_x[i+1];


				if( (v0&GENERIC_BLOCK_FLAG) || (h0&GENERIC_BLOCK_FLAG) || is_over5x )
				{
					PACK_GENERIC_BLOCK((is_over2x&&(!is_over5x)), h0, h1, v0, v1, pattern )

					//***
					//debug_generic_total++;
				}
				else
				{
					int p = PACK_4( h0,h1,v0,v1 );

					this_pattern = UPSCALE_SHAPE_TO_PATTERNS[p];
					*pattern++ = this_pattern;
				}
			}

			if( ver_count >= 1 )
			{
				*pattern++ = kUpScalePattern_GENERIC_EOL;
				*pattern++ = ver_count-1;
			}
			else
				goto bail;
		}
	}

bail:
	if( pattern != NULL )
		*pattern = kCommonScalePattern_EOF;

	*clipped_dst_width_out  = dst_width;
	*is_upscale_out			= is_upscale;

#ifdef SUPPORT_NEON
	if( has_neon && (!is_upscale) && (*pattern_in_out != NULL) )
		err = combine_to_4x2_downscale_pattern( *pattern_in_out, buf_size_in_out );
#endif

	//***
	//if( debug_generic_total != 0 )
	//{
	//	float percent = (float)debug_generic_total/(idx_y_max*idx_x_max/4);
	//	fprintf( stderr, "$$$$$$generic block percentage:%f\n", percent );
	//}

	return err;
}



int check_overlap_xy( int xx0, int yy0, int width, int height, FskVideoSprite s )
{
	s->to_blend = 0;		//reset every time
 	s->x0 = s->dst_x - xx0;
 	s->y0 = s->dst_y - yy0;
 	s->x1 = s->x0 + s->srcRect.width;
 	s->y1 = s->y0 + s->srcRect.height;

	//check no overlap condition
	if( s->x0 >= width || s->x1 < 0 || s->y0 >= height || s->y1 < 0)
	{
		s->in_frame = 0;
	}
	else
	{
		int ad=0;

		if( s->x0 < 0 )
		{
			ad = -s->x0 * (s->depth/8);
			s->x0 = 0;
		}

		if( s->x1 > width )
			s->x1 = width;

		if( s->y0 < 0 )
		{
			ad += -s->y0 * s->row_bytes;
			s->y0 = 0;
		}

		if( s->y1 > height )
			s->y1 = height;

		s->in_frame = 1;

		s->addr = s->baseAddr + ad;
	}

	return s->in_frame;
}


int check_overlap_y( int width, int y0, int y1, FskVideoSprite s )
{
	//check no overlap condition
	if( s->y0 >= y1 || s->y1 <= y0)
		s->in_line = 0;
	else
		s->in_line = 1;

	s->parsed   = 0;
	s->to_blend = 0;	//reset every time

	return s->in_line;
}

int frame_has_sprite(int xx0, int yy0, int width, int height, FskVideoSprite sprites )
{
	int frame_has_sprite = 0;
	FskVideoSprite this_sprite = sprites;

	//check overlap of the stripe
	while( this_sprite != NULL )
	{
		int in_frame = 0;

		in_frame = check_overlap_xy( xx0, yy0, width, height, this_sprite );

		if( in_frame )
			frame_has_sprite = 1;

		this_sprite = this_sprite->next;
	}

	return frame_has_sprite;
}


int line_has_sprite(int width, int y0, int y1, FskVideoSprite sprites )
{
	int	line_has_sprite = 0;
	FskVideoSprite this_sprite = sprites;

	//check overlap of the stripe
	while( this_sprite != NULL )
	{
		if( this_sprite->in_frame )
		{
			int in_line = check_overlap_y( width, y0, y1, this_sprite );
			if( in_line )
				line_has_sprite = 1;
		}

		this_sprite = this_sprite->next;
	}

	return line_has_sprite;
}

/*
int fix_line_end(int this_pattern, int *copy_and_sprite )
{
	//so that copy next line is extracted line end is redone
	*copy_and_sprite = 0;
	if( this_pattern == kUpScalePattern_EOL_2_COPY )
	{
		this_pattern = kUpScalePattern_EOL_2;
		*copy_and_sprite = 1;
	}
	else if( this_pattern == kUpScalePattern_EOL_3_COPY )
	{
		this_pattern = kUpScalePattern_EOL_3;
		*copy_and_sprite = 1;
	}

	return this_pattern;
}
*/

void switch_buffer(FskVideoSprite *sss, int x0, int x1, FskVideoSprite sprites, unsigned char **sprite_pattern_in_out )
{
	unsigned char *sprite_pattern = *sprite_pattern_in_out;
	FskVideoSprite this_sprite = sprites;

	while( this_sprite != NULL )
	{
		if( !this_sprite->in_line || this_sprite->parsed )
			goto next;

		if( this_sprite->x0 >= x0 && this_sprite->x0 < x1 )
		{
			*sss	= this_sprite;
			*(sprite_pattern++) = kCommonPattern_Switch_Buffer_SPRITE;
			break;
		}
next:
		this_sprite = this_sprite->next;
	}

	*sprite_pattern_in_out = sprite_pattern;
}

void blend_sprite( FskVideoSprite *sss, int sss_x0, int x0, int x1, int y0, int y1, int drb, int pix_bytes, FskVideoSprite sprites, unsigned char *dst, unsigned char **sprite_pattern_in_out )
{
	unsigned char *sprite_pattern = *sprite_pattern_in_out;
	FskVideoSprite this_sprite = sprites;

	while( this_sprite != NULL )
	{
		if( !this_sprite->in_line || this_sprite->parsed )
			goto next;

		if( this_sprite->x1 > x0 && this_sprite->x1 <= x1 )
		{
			int offset_x0   = this_sprite->x0 - sss_x0;
			int offset_y0   = this_sprite->y0 > y0 ? this_sprite->y0 - y0 : 0;
			int offset		= 0;
			int this_width  = this_sprite->x1 - this_sprite->x0;
			int top	        = this_sprite->y0 > y0 ? this_sprite->y0 : y0;
			int bottom      = this_sprite->y1 < y1 ? this_sprite->y1 : y1;
			int this_height = bottom - top;
			unsigned char *this_src = this_sprite->addr;
			unsigned char *this_dst = dst;

			if( top != this_sprite->y0 )
				this_src += (top - this_sprite->y0)*this_sprite->row_bytes;

			*(sprite_pattern++) = kCommonPattern_Blend_SPRITE;

			if( offset_y0 != 0 )
				offset += offset_y0*drb;

			if( offset_x0 != 0 )
				offset += offset_x0<<(pix_bytes>>1);

			this_dst += offset;
			SET_BLEND_SPRITE_PARAM(sprite_pattern, this_dst, this_src, this_width, this_height, this_sprite );

			this_sprite->parsed = 1;
		}

		//extend the right edge of spriting area
		if( this_sprite->x1 > (*sss)->x1  && this_sprite->x0 < x1 )
			*sss = this_sprite;
next:
		this_sprite = this_sprite->next;
	}

	*sprite_pattern_in_out = sprite_pattern;
}

int add_sprites
(
	int				pix_bytes,
	int				is_upscale,	//0 down, 1 up, 2 up_xx
	int				dy_in,
	int				xx0,
	int				yy0,
	int				dst_width,
	int				dst_height,
	int				dst_rowbytes,
	int				src_height,
	FskVideoSprite	sprites,
	unsigned char  *patterns,
	int			    pattern_size,
	unsigned char **sprite_pattern_in_out,
	int			   *sprite_pattern_size_in_out,
	unsigned char **sprite_back_buffer,
	unsigned char **sprite_back_buffer_x,
	int			   *sprite_back_buffer_size,
	int			   *updated
)
{
	int				y0 = 0;
	unsigned char	*sprite_pattern0 = *sprite_pattern_in_out;
	unsigned char	*sprite_pattern  = sprite_pattern0;
	int				sprite_pattern_size = *sprite_pattern_size_in_out;
	int				drb_abs = dst_rowbytes >= 0 ? dst_rowbytes : -dst_rowbytes;
	int				err = 0;
	int				maxSpriteBackBufferHeight;

	if( is_upscale == 0 )
		maxSpriteBackBufferHeight = kMaxSpriteBackBufferHeight0;
	else
		maxSpriteBackBufferHeight = (FWD_ONE / dy_in + 1 ) * 2;

	if( !frame_has_sprite(xx0, yy0, dst_width, dst_height, sprites ) )
	{
		*updated = 0;
		return err;
	}

	{
#define MAX_OVERHEAD_BYTES_PER_SPRITE_PER_BLOCK_LINE		(SWITCH_BUFFER_BYTES + BLEND_SPRITE_BYTES + COPY_SPRITE_BYTES)
		int size_wanted = pattern_size;
		int total_sprites = 0;
		FskVideoSprite this_sprite = sprites;

		while( this_sprite != NULL )
		{
			total_sprites++;
			this_sprite = this_sprite->next;
		}

		size_wanted += total_sprites * src_height * MAX_OVERHEAD_BYTES_PER_SPRITE_PER_BLOCK_LINE;	//all possible cost by sprites
		size_wanted += (total_sprites + 1) * (src_height>>1) * COPY_SPRITE_BYTES;					//all possible

		if( sprite_pattern_size < size_wanted || sprite_pattern == NULL )
		{
			sprite_pattern_size = size_wanted;
			if( sprite_pattern != NULL )
			{
				FskMemPtrDispose( sprite_pattern );
				sprite_pattern = NULL;
			}

			sprite_pattern0 = FskMemPtrAlloc( sprite_pattern_size );
			sprite_pattern  = sprite_pattern0;
			if( sprite_pattern0 == NULL )
			{ err = -1;	goto bail;}
		}

		size_wanted = (drb_abs+1)*pix_bytes*maxSpriteBackBufferHeight;
		if( *sprite_back_buffer_size < size_wanted || *sprite_back_buffer == NULL )
		{
			*sprite_back_buffer_size = size_wanted;
			if( *sprite_back_buffer != NULL )
			{
				FskMemPtrDispose( *sprite_back_buffer );
				*sprite_back_buffer = NULL;
			}

			*sprite_back_buffer = FskMemPtrAlloc( *sprite_back_buffer_size );
			if( *sprite_back_buffer == NULL )
			{ err = -1;	goto bail;}

			*sprite_back_buffer_x = dst_rowbytes >= 0 ? *sprite_back_buffer : (*sprite_back_buffer) + drb_abs * (maxSpriteBackBufferHeight - 1 );
		}
	}

	//copy first 4 bytes(width)
	*(sprite_pattern++) = *(patterns++);
	*(sprite_pattern++) = *(patterns++);
	*(sprite_pattern++) = *(patterns++);
	*(sprite_pattern++) = *(patterns++);

#define EOL_EXIT_TRATEGY								\
	(													\
		(is_upscale == 0) &&							\
		(												\
			this_pattern == kDownScalePattern_EOL_0 ||	\
			this_pattern == kDownScalePattern_EOL_1 ||	\
			this_pattern == kDownScalePattern_EOL_2		\
		)												\
	)													\
	||													\
	(													\
		(is_upscale) &&									\
		(												\
			this_pattern == kUpScalePattern_GENERIC_EOL	\
		)												\
	)													\


#define DOWN_SCALE_SKIP_PIX_CONTINUE_TRATEGY			\
		( is_upscale == 0 && this_pattern == kDownScalePattern_SKIP )

	while(1)
	{
		unsigned char	line_1st_pattern = *(patterns);
		int				dy, y1;
		int				x0 = 0;

		if( is_upscale == 0 )
			dy = downscale_pattern_height[line_1st_pattern];
		else
		{
			if( line_1st_pattern == kUpScalePattern_GENERIC_BLOCK )
			{
				int y0 = *(patterns+3);
				int y1 = *(patterns+4);
				dy = y0 + y1;
			}
			else
				dy = upscale_pattern_height[line_1st_pattern];
		}

		y1 = y0 + dy;

		if( line_1st_pattern == kCommonScalePattern_EOF )
		{
			*(sprite_pattern++) = kCommonScalePattern_EOF;
			goto bail;
		}

		if( !line_has_sprite( dst_width, y0, y1, sprites ) )
		{
			while( 1 )	//no single sprite in line, simply copy existing opcode
			{
				unsigned char	this_pattern   = *(patterns++);

				*(sprite_pattern++) = this_pattern;
				if( is_upscale && ( this_pattern == kUpScalePattern_GENERIC_BLOCK ) )
				{
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
				}
				if( this_pattern == kCommonScalePattern_EOF )
					goto bail;

				if( EOL_EXIT_TRATEGY )
				{
					if( is_upscale )	//over 2x scale EOL
						*(sprite_pattern++) = *(patterns++);

					break;
				}
			}

			y0 = y1;	//goto next block line
			continue;
		}

		{
			FskVideoSprite sss   = NULL;
			int				sss_x0 = 0;

			//process a whole line
			while( 1 )
			{
				unsigned char	this_pattern = *(patterns++);
				int				dx, x1;

				if( is_upscale == 0 )
					dx = downscale_pattern_width[this_pattern];
				else
				{
					if( this_pattern == kUpScalePattern_GENERIC_BLOCK )
					{
						int x0 = *(patterns+0);
						int x1 = *(patterns+1);
						dx = x0+x1;
					}
					else
						dx = upscale_pattern_width[this_pattern];
				}

				x1 = x0 + dx;

				if( this_pattern == kCommonScalePattern_EOF )
				{
					*(sprite_pattern++) = kCommonScalePattern_EOF;
					goto bail;
				}

				if(EOL_EXIT_TRATEGY)
				{
					*(sprite_pattern++) = this_pattern;
					if( is_upscale )
						*(sprite_pattern++) = *(patterns++);

					break;
				}

				if( DOWN_SCALE_SKIP_PIX_CONTINUE_TRATEGY )
				{
					*(sprite_pattern++) = this_pattern;
					continue;
				}

				if( sss == NULL )
				{
					switch_buffer( &sss, x0, x1, sprites, &sprite_pattern );
					if( sss != NULL )
						sss_x0 = x0;
				}

				*(sprite_pattern++) = this_pattern;//do color conversion
				if( is_upscale && ( this_pattern == kUpScalePattern_GENERIC_BLOCK ) )
				{
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
					*(sprite_pattern++) = *(patterns++);
				}

				if( sss == NULL )
				{
					x0 = x1;
					continue;
				}

				blend_sprite( &sss, sss_x0, x0, x1, y0, y1, dst_rowbytes, pix_bytes, sprites, *sprite_back_buffer_x, &sprite_pattern );

				if( sss->x1 <= x1 )
				{
					int this_width  = x1-sss_x0;
					int this_height = dy;

					*(sprite_pattern++) = kCommonPattern_Copy_Buffer_SPRITE;

					SET_COPY_SPRITE_PARAM(sprite_pattern, this_width, this_height, sprites)
					sss = NULL;
				}
				x0 = x1;
			}
		}

		y0 = y1;
	}

bail:
	*sprite_pattern_in_out		= sprite_pattern0;
	*sprite_pattern_size_in_out = sprite_pattern_size;
	*updated = 1;

	return err;
}

int has_context_change( const FskRectBlitParams *p )
{
	if
	(
		p->srcX0		!= last_srcX0		||
		p->srcY0		!= last_srcY0		||
		p->srcXInc		!= last_srcXInc		||
		p->srcYInc		!= last_srcYInc		||
		p->dstWidth		!= (UInt32)last_dstWidth	||
		p->dstHeight	!= (UInt32)last_dstHeight	||
		p->dstRowBytes	!= last_dstRowBytes	||
		p->dstPixelFormat!= last_dstPixelFormat
	)
	{
		last_srcX0			= p->srcX0;
		last_srcY0			= p->srcY0;
		last_srcXInc		= p->srcXInc;
		last_srcYInc		= p->srcYInc;
		last_dstWidth		= p->dstWidth;
		last_dstHeight		= p->dstHeight;
		last_dstRowBytes	= p->dstRowBytes;
		last_dstPixelFormat = p->dstPixelFormat;

		return 1;
	}
	return 0;
}


void reset_change_context_(void)
{
	last_srcX0		= -1;
	last_srcY0		= -1;
	last_srcXInc	= -1;
	last_srcYInc	= -1;
	last_dstWidth	= -1;
	last_dstHeight	= -1;
}


int parse_proc(unsigned char *d)
{
	unsigned char	*dst	= (unsigned char *)( *((long  *)(d +  0) ) );
	unsigned char	*src	= (unsigned char *)( *((long  *)(d +  4) ) );
	int				width	=			  (int)( *((short *)(d +  8) ) );
	int				height	=			  (int)( *((short *)(d + 10) ) );
	FskVideoSpriteRecord *s =(FskVideoSpriteRecord *)( *((long  *)(d + 12) ) );
	FskRectBlitParams    *p = &s->p;

	s->to_blend	   = 1;
	p->dstBaseAddr = dst;
	p->srcBaseAddr = src;
	p->srcWidth    = width;
	p->dstWidth    = width;
	p->srcHeight   = height;
	p->dstHeight   = height;

	return 0;
}

int blend_proc(FskVideoSpriteRecord *s)
{
	while( s != NULL )
	{
		if( s->to_blend)
		{
			s->blitter( &s->p );
			s->to_blend = 0;
		}
		s = s->next;
	}

	return 0;
}

#include "stdio.h"
#include "string.h"

//
//create scale patch functions from down, up and scale functions

#ifdef SUPPORT_YUV420_WMMX_OPT


extern void yuv420to16RGB565SE_up_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420to16RGB565SE_down_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420to16RGB565SE_generic_scale_bc_arm_wmmx(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420to32BGRA_up_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420to32BGRA_down_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420to32BGRA_generic_scale_bc_arm_wmmx(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);

//YUV420 to 16RGB ARM wmmx
#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define YUV420
#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420to16RGB565SE_generic_scale_bc_arm_wmmx
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420to16RGB565SE_up_scale_bc_p_arm_wmmx
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420to16RGB565SE_down_scale_bc_p_arm_wmmx
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV42016RGB565SE_scale_bc_arm_wmmx

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
#undef YUV420

#undef HIGH_QUALITY_DOWN_SCALE

//YUV420 to 32BGRA ARM WMMX
#define HIGH_QUALITY_DOWN_SCALE
#define YUV420

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420to32BGRA_generic_scale_bc_arm_wmmx
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420to32BGRA_up_scale_bc_p_arm_wmmx
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420to32BGRA_down_scale_bc_p_arm_wmmx
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV42032BGRA_scale_bc_arm_wmmx

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
#undef YUV420

#undef HIGH_QUALITY_DOWN_SCALE

#endif //SUPPORT_YUV420_WMMX_OPT


#ifdef YUV420i_RGB_C_IMPLEMENTATION

#ifdef APP_MODE
#define MYSTATIC
#else
#define MYSTATIC static
#endif

//create down, up and generic scale functions
MYSTATIC void yuv420ito16RGB565SE_down_scale_bc_p_c (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);
MYSTATIC void yuv420ito16RGB565SE_up_scale_bc_p_c   (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);
MYSTATIC void yuv420ito16RGB565SE_generic_scale_bc_c(unsigned char *,FskFixed,FskFixed,FskFixed,unsigned char *,int,FskFixed,FskFixed,int,int,FskFixed,int);

MYSTATIC void yuv420ito32BGRA_down_scale_bc_p_c	  (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);
MYSTATIC void yuv420ito32BGRA_up_scale_bc_p_c	  (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);
MYSTATIC void yuv420ito32BGRA_generic_scale_bc_c  (unsigned char *,FskFixed,FskFixed,FskFixed,unsigned char *,int,FskFixed,FskFixed,int,int,FskFixed,int);

//
#define YUV420i_RGB_C_IMPLEMENTATION
#undef  YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT			1										//1 for 16rgb
#define PIXEL_BYTES					(1<<PIXEL_BYTES_SHIFT)
#define DST_DATA_TYPE				short									//short for 16rgb
#define DOWNSCALE_PACK  			PACK_16RGB_lo							//PACK_16RGB_lo for 16rgb
#define UPSCALE_GET_Y_i				GET_Y_i									//GET_Y_i for 16rgb
#define DOWNSCALE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_c	//yuv420ito32BGRA_down_scale_bc_p_c					//16RGB565SE for 16rgb
#define UPSCALE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_c		//yuv420ito32BGRA_up_scale_bc_p_c					//16RGB565SE for 16rgb
#define GENERIC_SCALE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_c

#include "YUV420CopyTemplate.c"

#undef YUV420i_RGB_C_IMPLEMENTATION
#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef DST_DATA_TYPE
#undef DOWNSCALE_PACK
#undef UPSCALE_GET_Y_i
#undef DOWNSCALE_FUNCTION_NAME
#undef UPSCALE_FUNCTION_NAME
#undef GENERIC_SCALE_FUNCTION_NAME
//

//
#define YUV420i_RGB_C_IMPLEMENTATION
#undef  YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT			2									//1 for 16rgb
#define PIXEL_BYTES					(1<<PIXEL_BYTES_SHIFT)
#define DST_DATA_TYPE				long								//short for 16rgb
#define DOWNSCALE_PACK  			PACK_32BGRA							//PACK_16RGB_lo for 16rgb
#define UPSCALE_GET_Y_i				GET_Y_i_32BGRA						//GET_Y_i for 16rgb
#define DOWNSCALE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_c	//yuv420to32BGRA_down_scale_bc_p_c					//16RGB565SE for 16rgb
#define UPSCALE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_c		//yuv420to32BGRA_up_scale_bc_p_c					//16RGB565SE for 16rgb
#define GENERIC_SCALE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_c

#include "YUV420CopyTemplate.c"

#undef YUV420i_RGB_C_IMPLEMENTATION
#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef DST_DATA_TYPE
#undef DOWNSCALE_PACK
#undef UPSCALE_GET_Y_i
#undef DOWNSCALE_FUNCTION_NAME
#undef UPSCALE_FUNCTION_NAME
#undef GENERIC_SCALE_FUNCTION_NAME
//

//
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_c
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_c
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_c
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_c

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
//

//
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_c
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_c
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_c
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_c

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
//


//NEON

#ifdef SUPPORT_NEON

MYSTATIC void yuv420ito32BGRA_up_scale_bc_p_neon1_c	  (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);
MYSTATIC void yuv420ito16RGB565SE_up_scale_bc_p_neon1_c   (unsigned char *,unsigned char *,int,int,unsigned char *,int,int,int,unsigned char *);


//only cover upscale case
#undef  YUV420i_RGB_C_IMPLEMENTATION
#undef  YUV420i_PATCHES
#define YUV420i_RGB_NEON1_C_IMPLEMENTATION
#define PIXEL_BYTES_SHIFT				1											//1 for 16rgb
#define PIXEL_BYTES						(1<<PIXEL_BYTES_SHIFT)
#define DST_DATA_TYPE					short										//short for 16rgb
#define UPSCALE_GET_Y_i					GET_Y_i										//GET_Y_i for 16rgb
#define GET_Y_i_4x2						GET_Y_i_RGB565_4x2							//GET_Y_i for 16rgb
#define UPSCALE_FUNCTION_NAME_NEON1		yuv420ito16RGB565SE_up_scale_bc_p_neon1_c	//yuv420ito32BGRA_up_scale_bc_p_c					//16RGB565SE for 16rgb

#include "YUV420CopyTemplate.c"

#undef YUV420i_RGB_NEON1_C_IMPLEMENTATION
#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef DST_DATA_TYPE
#undef UPSCALE_GET_Y_i
#undef GET_Y_i_4x2
#undef UPSCALE_FUNCTION_NAME_NEON1
//

//
#define YUV420i_RGB_NEON1_C_IMPLEMENTATION
#undef  YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT				2											//1 for 16rgb
#define PIXEL_BYTES						(1<<PIXEL_BYTES_SHIFT)
#define DST_DATA_TYPE					long										//short for 16rgb
#define UPSCALE_GET_Y_i					GET_Y_i_32BGRA								//GET_Y_i for 16rgb
#define GET_Y_i_4x2						GET_Y_i_32BGRA_4x2							//GET_Y_i for 16rgb
#define UPSCALE_FUNCTION_NAME_NEON1		yuv420ito32BGRA_up_scale_bc_p_neon1_c		//yuv420to32BGRA_up_scale_bc_p_c					//16RGB565SE for 16rgb

#include "YUV420CopyTemplate.c"

#undef YUV420i_RGB_NEON1_C_IMPLEMENTATION
#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef DST_DATA_TYPE
#undef UPSCALE_GET_Y_i
#undef GET_Y_i_4x2
#undef UPSCALE_FUNCTION_NAME_NEON1
//

//patch
#undef  YUV420i_RGB_NEON1_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_c
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_neon1_c
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_c
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_neon1_c

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
//

//
#undef  YUV420i_RGB_C_IMPLEMENTATION
#undef  YUV420i_RGB_NEON0_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_c
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_neon1_c
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_c
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_neon1_c

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES
//
#endif	//neon

#endif	//c



#ifdef YUV420i_RGB_ARM_IMPLEMENTATION
extern void yuv420ito16RGB565SE_generic_scale_bc_arm_wmmx(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito16RGB565SE_generic_scale_bc_arm_v7(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito16RGB565SE_generic_scale_bc_arm_v6(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito16RGB565SE_generic_scale_bc_arm_v5(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito16RGB565SE_generic_scale_bc_arm_v4(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito16RGB565SE_up_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_up_scale_bc_p_arm_v7( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_up_scale_bc_p_arm_v6( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_up_scale_bc_p_arm_v5( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_up_scale_bc_p_arm_v4( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_down_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_down_scale_bc_p_arm_v7( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_down_scale_bc_p_arm_v6( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_down_scale_bc_p_arm_v5( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito16RGB565SE_down_scale_bc_p_arm_v4( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );

extern void yuv420ito32BGRA_generic_scale_bc_arm_wmmx(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito32BGRA_generic_scale_bc_arm_v7(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito32BGRA_generic_scale_bc_arm_v6(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito32BGRA_generic_scale_bc_arm_v5(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito32BGRA_generic_scale_bc_arm_v4(unsigned char	*dst,FskFixed Bx,FskFixed Cx, FskFixed xd, unsigned char	*Y0,int	width,FskFixed x0, FskFixed y0, int	yuvrb,int drb,FskFixed yd,int height);
extern void yuv420ito32BGRA_up_scale_bc_p_arm_wmmx(   unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_up_scale_bc_p_arm_v7(   unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_up_scale_bc_p_arm_v6(   unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_up_scale_bc_p_arm_v5(   unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_up_scale_bc_p_arm_v4(   unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_down_scale_bc_p_arm_wmmx( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_down_scale_bc_p_arm_v7( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_down_scale_bc_p_arm_v6( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_down_scale_bc_p_arm_v5( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );
extern void yuv420ito32BGRA_down_scale_bc_p_arm_v4( unsigned char *yuv, unsigned char *dst, int Bx, int Cx, unsigned char *pattern, int drb, int yuvrb, int dst_width, unsigned char *spriteBackbuffer );

//YUV420i to 16RGB ARM V4
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_arm_v4
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_arm_v4
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_arm_v4
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v4

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

//YUV420i to 16RGB ARM V5
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_arm_v5
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_arm_v5
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_arm_v5
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v5

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES


//YUV420i to 16RGB ARM V6
#ifndef SUPPORT_YUV420_WMMX_OPT			//no v6 for xscale device

#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_arm_v6
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_arm_v6
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_arm_v6
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v6

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE

#endif	//SUPPORT_YUV420_WMMX_OPT

#ifdef SUPPORT_WMMX


//YUV420i to 16RGB ARM wmmx
#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_arm_wmmx
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_arm_wmmx
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_arm_wmmx
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_arm_wmmx

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE



#endif

#ifdef SUPPORT_NEON

//YUV420i to 16RGB ARM V7
#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT						1										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito16RGB565SE_generic_scale_bc_arm_v7
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_up_scale_bc_p_arm_v7
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito16RGB565SE_down_scale_bc_p_arm_v7
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v7

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE

#endif

//YUV420i to 32BGRA ARM V4
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES
#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_arm_v4
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_arm_v4
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_arm_v4
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_arm_v4

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

//YUV420i to 32BGRA ARM V5
#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_arm_v5
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_arm_v5
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_arm_v5
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_arm_v5

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES


//YUV420i to 32BGRA ARM V6
#ifndef SUPPORT_YUV420_WMMX_OPT			//no v6 for xscale device

#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_arm_v6
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_arm_v6
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_arm_v6
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_arm_v6

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE

#endif	//SUPPORT_YUV420_WMMX_OPT

#ifdef SUPPORT_WMMX

//YUV420i to 32BGRA ARM V7
#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_arm_wmmx
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_arm_wmmx
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_arm_wmmx
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_arm_wmmx

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE


#endif


#ifdef SUPPORT_NEON

//YUV420i to 32BGRA ARM V7
#define HIGH_QUALITY_DOWN_SCALE

#undef  YUV420i_RGB_C_IMPLEMENTATION
#define YUV420i_PATCHES

#define PIXEL_BYTES_SHIFT						2										//1 for 16rgb
#define PIXEL_BYTES								(1<<PIXEL_BYTES_SHIFT)
#define GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME	yuv420ito32BGRA_generic_scale_bc_arm_v7
#define UP_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_up_scale_bc_p_arm_v7
#define DOWN_SCALE_BC_i_CASE_FUNCTION_NAME		yuv420ito32BGRA_down_scale_bc_p_arm_v7
#define PATCH_FUNCTION_NAME_SCALE_BC			my_FskCopyYUV420i32BGRA_scale_bc_arm_v7

#include "YUV420CopyTemplate.c"

#undef PIXEL_BYTES_SHIFT
#undef PIXEL_BYTES
#undef GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_BC_i_CASE_FUNCTION_NAME
#undef UP_SCALE_XX_BC_i_CASE_FUNCTION_NAME
#undef DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
#undef PATCH_FUNCTION_NAME_SCALE_BC
#undef YUV420i_PATCHES

#undef HIGH_QUALITY_DOWN_SCALE

#endif

#endif
