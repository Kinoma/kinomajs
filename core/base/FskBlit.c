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
#define __FSKBITMAP_PRIV__

#include "FskBlit.h"
#include "FskBitmap.h"
#include "FskEndian.h"
#include "FskPixelOps.h"
#include "FskUtilities.h"
#include "FskFrameBuffer.h"
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */
#include "FskArch.h"

#define UNUSED(x)		(void)(x)

#define defineFillProc(name) void name(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)

FskBitmapCopyProc gFillColor16Proc;
FskBitmapCopyProc gFillColor32Proc;

defineFillProc(fillColor24);
defineFillProc(fillColor32);
defineFillProc(fillColor16);

#ifdef SUPPORT_NEON
defineFillProc(fillColor32_arm_v7);
defineFillProc(fillColor16_arm_v7);
#endif

#ifdef SUPPORT_WMMX
defineFillProc(fillColor16_arm_wmmx);
defineFillProc(fillColor32_arm_wmmx);
#endif

defineFillProc(fill8G);
defineFillProc(fillColorYUV420);

defineFillProc(blendColor24);
defineFillProc(blendColor32);
defineFillProc(blendColor16LE);
defineFillProc(blend8G);

defineFillProc(colorize24);
defineFillProc(colorize32);
defineFillProc(colorize16LE);
defineFillProc(colorize8G);

defineFillProc(applyMaskAndValue32);
defineFillProc(applyMaskAndValue24);
defineFillProc(applyMaskAndValue16);
defineFillProc(applyMaskAndValue8);

void FskBlitInitialize(void)
{
#if defined(SUPPORT_NEON) || defined(SUPPORT_WMMX)
	int implementation = FskHardwareGetARMCPU_All();
#endif
    
#ifdef SUPPORT_NEON
	if (FSK_ARCH_ARM_V7 == implementation) {
		gFillColor16Proc = fillColor16_arm_v7;
		gFillColor32Proc = fillColor32_arm_v7;
		return;
	}
#endif

#ifdef SUPPORT_WMMX
	if (FSK_ARCH_XSCALE == implementation) {
		gFillColor16Proc = fillColor16_arm_wmmx;
		gFillColor32Proc = fillColor32_arm_wmmx;
		return;
	}
#endif

	gFillColor16Proc = fillColor16;
	gFillColor32Proc = fillColor32;
}


FskErr FskRectangleFill(FskBitmap dst, FskConstRectangle r, FskConstColorRGBA color, UInt32 mode, FskConstGraphicsModeParameters modeParams)
{
	FskErr err = kFskErrNone;
	unsigned char *dstBits;
	SInt32 rowBytes;
	FskBitmapFormatEnum pixelFormat;
	FskBitmapCopyProc scanProc;
	unsigned char pixel[4];
	SInt32 blendLevel;
	void *state = NULL;
	FskRectangleRecord outRect;

	blendLevel = 255;
	if (NULL != modeParams) {
		blendLevel = modeParams->blendLevel;
        if (blendLevel <= 0)											/* Totally transparent: */
            goto bail;													/* all done! */
    }

	#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dst))
		return FskGLRectangleFill(dst, r, color, mode, modeParams);
	#endif /* FSKBITMAP_OPENGL */

	// clip against the bitmap
	if (false == FskRectangleIntersect(r, &dst->bounds, &outRect))
		return kFskErrNothingRendered;

	if (kFskErrNone == FskFrameBufferFillRect(dst, color, modeParams, &outRect))
		return kFskErrNone;

	// calculate the base address
	FskBitmapWriteBegin(dst, (void**)(void*)&dstBits, &rowBytes, &pixelFormat);
	dstBits += (outRect.y * rowBytes);
	if (r->x > 0)
		dstBits += (outRect.x * dst->depth) / 8;

	if (kFskErrNone != FskConvertColorRGBAToBitmapPixel(color, pixelFormat, pixel))
		goto unsupported;

	switch (pixelFormat) {
		case kFskBitmapFormat24BGR:
		case kFskBitmapFormat24RGB:
			if (kFskGraphicsModeColorize == mode)
				scanProc = colorize24;
			else
			if (blendLevel >= 255)
				scanProc = fillColor24;
			else
				scanProc = blendColor24;
			break;

		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32ABGR:
		case kFskBitmapFormat32RGBA:
		case kFskBitmapFormat32A16RGB565LE:
			if (kFskGraphicsModeColorize == mode)
				scanProc = colorize32;
			else
			if (blendLevel >= 255)
				scanProc = gFillColor32Proc;
			else
				scanProc = blendColor32;
			break;

		case kFskBitmapFormat16RGB565LE:
			if (kFskGraphicsModeColorize == mode)
				scanProc = colorize16LE;
			else
			if (blendLevel >= 255)
				scanProc = gFillColor16Proc;
			else
				scanProc = blendColor16LE;
			break;

		case kFskBitmapFormat16AG:
		case kFskBitmapFormat16RGB5515LE:
		case kFskBitmapFormat16RGBA4444LE:
			if (kFskGraphicsModeAlpha == mode)							/* We don't implement alpha for these formats with alpha */
				goto unimplemented;
		case kFskBitmapFormat16BGR565LE:
		case kFskBitmapFormat16RGB565BE:
			if (kFskGraphicsModeColorize == mode || blendLevel < 255)	/* We don't do any blending for these 16-bit formats */
				goto unimplemented;
			scanProc = gFillColor16Proc;
			break;

		case kFskBitmapFormat8G:
		case kFskBitmapFormat8A:
			if (kFskGraphicsModeColorize == mode)
				scanProc = colorize8G;
			else if (blendLevel >= 255)
				scanProc = fill8G;
			else
				scanProc = blend8G;
			break;

		case kFskBitmapFormatYUV420:
			if (mode > kFskGraphicsModeCopy || blendLevel < 255)
				goto unimplemented;
			scanProc = fillColorYUV420;
			state = makeYUV420BlitInfo(dst);
			break;

		default:
			goto unsupported;
	}

	// do that blit
	//dlog( "height:%d, drb:%d, *src:%x, dst:%x, width:%d, \n",
	//		 (int)outRect.height, (int)(rowBytes - ((outRect.width * dst->depth) >> 3)), (int)(*((short *)pixel)), (int)dstBits, (int)outRect.width);


	CALL_COPYPROC(scanProc, outRect.height, rowBytes - ((outRect.width * dst->depth) >> 3), pixel, dstBits, outRect.width, (UInt16)dst->depth, state ? state : (void *)blendLevel);

	//if( scanProc == fillColor32Proc )
	//{
	//	dlog( "calling fillColor32_verify()\n");
	//	CALL_COPYPROC(fillColor32_verify, outRect.height, rowBytes - ((outRect.width * dst->depth) >> 3), pixel, dstBits, outRect.width, (UInt16)dst->depth, state ? state : (void *)blendLevel);
	//}

bail:
	FskBitmapWriteEnd(dst);

	FskMemPtrDispose(state);

	return err;

unsupported:
	BAIL(kFskErrUnsupportedPixelType);

unimplemented:
	BAIL(kFskErrUnimplemented);
}

void fillColor24(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	unsigned char *c4 = (unsigned char *)src;
	unsigned char c0, c1, c2;
	UInt32 p0, p1, p2;
    UInt32 stragglers = width & 15;
    UNUSED(bpp);
    UNUSED(state);

	c0 = *c4++;
	c1 = *c4++;
	c2 = *c4++;

//@@ We could/should always use aligned case on processors that support it (x86, powerpc)
#if !FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE
									/* If misaligned memory access is not implemented in hardware, ... */
	if (0 == (((UInt32)dst) & 3)) {	/* ... then if the destination is aligned, ... */
		width -= stragglers;		/* ... write as many blocks as possible ... */
		width >>= 4;				/* ... in batches of 16 */
	}
	else {							/* whereas if the destination is misaligned, ... */
		stragglers = width;			/* ... copy byte-by-byte, ,,, */
		width = 0;					/* ... and no blocks of 16 */
	}
#else /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */
	width -= stragglers;
	width >>= 4;
#endif /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */

#if TARGET_RT_BIG_ENDIAN
	p0 = (c0 << 24) | (c1 << 16) | (c2 << 8) | c0;
	p1 = (c1 << 24) | (c2 << 16) | (c0 << 8) | c1;
	p2 = (c2 << 24) | (c0 << 16) | (c1 << 8) | c2;
#else
	p0 = (c0 << 24) | (c2 << 16) | (c1 << 8) | c0;
	p1 = (c1 << 24) | (c0 << 16) | (c2 << 8) | c1;
	p2 = (c2 << 24) | (c1 << 16) | (c0 << 8) | c2;
#endif

	while (height--) {
		UInt32 s = stragglers, w = width;

		while (w--) {
			*(UInt32 *)&dst[0] = p0;
			*(UInt32 *)&dst[4] = p1;
			*(UInt32 *)&dst[8] = p2;
			dst += 12;
			*(UInt32 *)&dst[0] = p0;
			*(UInt32 *)&dst[4] = p1;
			*(UInt32 *)&dst[8] = p2;
			dst += 12;
			*(UInt32 *)&dst[0] = p0;
			*(UInt32 *)&dst[4] = p1;
			*(UInt32 *)&dst[8] = p2;
			dst += 12;
			*(UInt32 *)&dst[0] = p0;
			*(UInt32 *)&dst[4] = p1;
			*(UInt32 *)&dst[8] = p2;
			dst += 12;
		}

		while (s--) {
			*dst++ = c0; *dst++ = c1; *dst++ = c2;
		}

		dst = (unsigned char *)(dstRowBump + (char *)dst);
	}
}

void fillColor32(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt32 *dst32 = (UInt32 *)dst;
	UInt32 c4 = *(UInt32 *)src;
    UInt32 stragglers = width & 15;
    UNUSED(bpp);
    UNUSED(state);

    width -= stragglers;
    width >>= 4;

	do {
		UInt32 s = stragglers, w = width;

		while (s--)
			*dst32++ = c4;

		while (w--) {
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
			*dst32++ = c4;
		}
		dst32 = (UInt32 *)(dstRowBump + (char *)dst32);
	} while (--height);
}


void fillColor16(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt16 *dst16 = (UInt16 *)dst;
	UInt16 c2 = *(UInt16 *)src;
	UInt32 twoPixels = (c2 << 16) | c2;
    UNUSED(bpp);
    UNUSED(state);

	do {
		UInt32 w = width;
		UInt32 width2;
		UInt32 *dst32;

		// make sure we are long aligned
		if (2 & (long)dst16) {
			*dst16++ = c2;
			w--;
		}

		// blast longs
		width2 = w >> 1;
		dst32 = (UInt32 *)dst16;
		while (width2 & 7) {
			*dst32++ = twoPixels;
			width2--;
		}

		width2 = width2 >> 3;
		while (width2--) {
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
			*dst32++ = twoPixels;
		}

		// pick up the straggler
		if (w & 1) {
			*(UInt16 *)dst32 = c2;
			dst32 = (UInt32 *)(2 + (char *)dst32);
		}

		dst16 = (UInt16 *)(dstRowBump + (char *)dst32);
	} while (--height);
}

void fill8G(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt8 pixel = *src;
    UNUSED(bpp);
    UNUSED(state);

	while (height--) {
		UInt32 w = width;

		while (w--)
			*dst++ = pixel;

		dst += dstRowBump;
	}
}

void fillColorYUV420(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	YUV420Blit blt = (YUV420Blit)state;
	unsigned char *y, *u, *v;
	UInt32 lineNumber, planeOffset;
	UInt32 i;
	unsigned char yVal = src[0], uVal = src[1], vVal = src[2];
    UNUSED(bpp);

	while (height--) {
		lineNumber = (dst - blt->y) / blt->width;
		planeOffset = (lineNumber >> 1) * (blt->width >> 1);				// vertical offset into uv planes
		planeOffset += ((dst - blt->y) - (blt->width * lineNumber)) >> 1;	// horizontal offset into uv planes

		y = dst;
		u = blt->u + planeOffset;
		v = blt->v + planeOffset;

		i = width;
		while (i--)
			*y++ = yVal;

		i = width >> 1;
		while (i--) {
			*u++ = uVal;
			*v++ = vVal;
		}

		dst = (unsigned char *)(width + dstRowBump + (char *)dst);
	}
}

void blendColor24(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	SInt32 blendLevel = (SInt32)state;
	SInt32 unblendLevel = (255 - blendLevel);
	unsigned char *c4 = (unsigned char *)src;
	SInt32 c0, c1, c2;
    UNUSED(bpp);

	c0 = blendLevel * *c4++;
	c1 = blendLevel * *c4++;
	c2 = blendLevel * *c4++;

	do {
		UInt32 w = width;
		do {
			dst[0] = (unsigned char)(((dst[0] * unblendLevel) + c0) >> 8);
			dst[1] = (unsigned char)(((dst[1] * unblendLevel) + c1) >> 8);
			dst[2] = (unsigned char)(((dst[2] * unblendLevel) + c2) >> 8);

			dst += 3;
		} while (--w);
		dst = (unsigned char *)(dstRowBump + (char *)dst);
	} while (--height);
}

void blendColor32(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	const UInt32	blendLevel	= (UInt32)state;
	const UInt32	mask		= 0x00FF00FF;
	UInt32			p			= *(UInt32 *)src;
	const UInt32	srcrb		=  p       & mask;
	const UInt32	srcag		= (p >> 8) & mask;
    UNUSED(bpp);

	do {
		UInt32 w = width;
		do {
			UInt32 p0 = *(UInt32 *)dst;
			UInt32 dstrb =  p0       & mask;
			UInt32 dstag = (p0 >> 8) & mask;

#if 0
			UInt32 drb = (srcrb - dstrb) * blendLevel;
			UInt32 dag = (srcag - dstag) * blendLevel;

			p0  =  ((drb >> 8) + dstrb) & mask;
			p   = (((dag >> 8) + dstag) & mask) << 8;
#else		/* Match GL computations */
			/*   Interpolate difference            Add dst*255 and round                    Multiply by 256/255     Divide by 256   Mask */
			p0 = (srcrb - dstrb) * blendLevel;	p0 += (dstrb << 8) - dstrb + 0x00800080;	p0 += (p0 >> 8) & mask; p0 >>= 8; p0 &= mask;
			p  = (srcag - dstag) * blendLevel;	p  += (dstag << 8) - dstag + 0x00800080;	p  += (p  >> 8) & mask; p  >>= 8; p  &= mask; p <<= 8;
#endif
			*(UInt32 *)dst = p0 | p;

			dst += 4;
		} while (--w);
		dst = (unsigned char *)(dstRowBump + (char *)dst);
	} while (--height);
}

void blendColor16LE(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	SInt32 sAlpha = ((SInt32)state) >> 3;
	UInt16 p = *(UInt16 *)src;
	const UInt32 srcrb = p  & 0x0F81F;
	const UInt32 srcg  = p  & 0x007E0;
	UInt16 *d = (UInt16 *)dst;
    UNUSED(bpp);

	do {
		UInt32 w = width;
		do {
			SInt32	p0 = *d, p1;

#if TARGET_RT_BIG_ENDIAN
			p0 = FskEndianU16_LtoN(p0);
#endif

			const UInt32 dstrb = p0 & 0x0F81F;
			const UInt32 dstg  = p0 & 0x007E0;

			p0 = (dstrb + (((srcrb - dstrb) * sAlpha + 0x8010) >> 5)) & 0xf81f;
			p1 = (dstg  + (((srcg  - dstg ) * sAlpha + 0x0200) >> 5)) & 0x07e0;

#if TARGET_RT_LITTLE_ENDIAN
			*d++ = (UInt16)(p0 | p1);
#else
			p0 = (UInt16)(p0 | p1);
			*d++ = FskEndianU16_NtoL(p0);
#endif
		} while (--w);
		d = (UInt16 *)(dstRowBump + (char *)d);
	} while (--height);
}

void blend8G(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	SInt32 blendLevel = (SInt32)state;
	SInt32 unblendLevel = (255 - blendLevel);
	SInt32 pixel = blendLevel * *src;
    UNUSED(bpp);

	while (height--) {
		UInt32 w = width;

		while (w--) {
			unsigned char d = *dst;
			*dst++ = (unsigned char)(((d * unblendLevel) + pixel) >> 8);
		}

		dst += dstRowBump;
	}
}

#define AlphaNScale(c, a, b)				(c *= a, c += (1 << (b - 1)), c += c >> b, c >>= b)

void colorize24(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	unsigned char *c4 = (unsigned char *)src;
	unsigned short c0, c1, c2, t, tt;
    UNUSED(bpp);
    UNUSED(state);

	c0 = *c4++;
	c1 = *c4++;
	c2 = *c4++;

	while (height--) {
		UInt32 w = width;

		while (w--) {
			t = *dst;	tt = AlphaNScale(t, c0, 8);	*dst++ = (unsigned char)tt;
			t = *dst;	tt = AlphaNScale(t, c1, 8);	*dst++ = (unsigned char)tt;
			t = *dst;	tt = AlphaNScale(t, c2, 8);	*dst++ = (unsigned char)tt;
		}
		dst = (unsigned char *)(dstRowBump + (char *)dst);
	}
}

void colorize32(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	unsigned char *c4 = (unsigned char *)src;
	unsigned short c0, c1, c2, c3, t, tt;
    UNUSED(bpp);
    UNUSED(state);

	c0 = *c4++;
	c1 = *c4++;
	c2 = *c4++;
	c3 = *c4++;

	while (height--) {
		UInt32 w = width;

		while (w--) {
			t = *dst;	tt = AlphaNScale(t, c0, 8);	*dst++ = (unsigned char)tt;
			t = *dst;	tt = AlphaNScale(t, c1, 8);	*dst++ = (unsigned char)tt;
			t = *dst;	tt = AlphaNScale(t, c2, 8);	*dst++ = (unsigned char)tt;
			t = *dst;	tt = AlphaNScale(t, c3, 8);	*dst++ = (unsigned char)tt;
		}
		dst = (unsigned char *)(dstRowBump + (char *)dst);
	}
}

void colorize16LE(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt16 *d = (UInt16 *)dst;
	unsigned short r, g, b, c2;
    UNUSED(bpp);

	c2 = *(UInt16 *)src;
	r = c2 >> 11;
	g = (c2 >> 5) & 0x3f;
	b = c2 & 0x1f;

	while (height--) {
		UInt32 w = width;
		while (w--) {
			UInt16 c1 = *d;
			UInt16 out, t;

#if TARGET_RT_BIG_ENDIAN
			c1 = FskEndianU16_LtoN(c1);
#endif

			t = c1 >> 11; out = AlphaNScale(t, r, 5) << 11;
			t = (c1 >> 5) & 0x3f; out |= AlphaNScale(t, g, 6) << 5;
			t = c1 & 0x1f; out |= AlphaNScale(t, b, 5);

#if TARGET_RT_BIG_ENDIAN
			out = FskEndianU16_NtoL(out);
#endif
			*d++ = out;
		}
		d = (UInt16 *)(dstRowBump + (char *)d);
	}
}

void colorize8G(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	const unsigned char *c4 = (const unsigned char *)src;
	unsigned short c0, t, tt;
    UNUSED(bpp);
    UNUSED(state);

	c0 = *c4++;

	while (height--) {
		UInt32 w = width;

		while (w--) {
			t = *dst;	tt = AlphaNScale(t, c0, 8);	*dst++ = (unsigned char)tt;
		}
		dst = (unsigned char *)(dstRowBump + (char *)dst);
	}
}

YUV420Blit makeYUV420BlitInfo(FskBitmap bits)
{
	YUV420Blit yuv;

	FskMemPtrNewClear(sizeof(YUV420BlitRecord), &yuv);
	yuv->width = bits->bounds.width;

	yuv->y = (unsigned char*)bits->bits;
	yuv->u = yuv->y + (bits->bounds.width * bits->bounds.height);
	yuv->v = yuv->u + ((bits->bounds.width * bits->bounds.height) >> 2);

	return yuv;
}

