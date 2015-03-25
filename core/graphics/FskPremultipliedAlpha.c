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
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskPremultipliedAlpha.h"
#include "FskPixelOps.h"

#define ALPHA_DIV_BITS	16U	/**< The number of fractional bits used in the numerator to implement a division as a reciprocal and multiply. */

/** This multiplies the pixel component p by alpha, where alpha has a specified number of bits.
 * This implements the equation:
 *			p = (int)((p * a) / ((1 << aBits) - 1) + 0.5);
 *	\param[in]	p		the pixel component.
 *	\param[in]	a		alpha.
 *	\param[in]	aBits	the number of bits in alpha.
 */
#define ALPHA_MUL(p, a, aBits)							do { (p) *= (a); (p) += (1 << ((aBits)-1)); (p) += (p) >> (aBits); (p) >>= (aBits); } while(0)

/** This determines if a pixel with heterogeneous components is an illegal premultiplied color.
 * This implements the boolean test:
 *			(p / (float)((1 << pBits) - 1)) > (a / (float)((1 << aBits) - 1))
 *	\param[in]	p		the pixel component, allegedly premultiplied.
 *	\param[in]	pBits	the number of bits in p.
 *	\param[in]	a		alpha.
 *	\param[in]	aBits	the number of bits in alpha.
 */
#define HETERO_IS_ILLEGAL_PREMUL(p, pBits, a, aBits)	(((p) << (aBits)) - (p)) > (((a) << (pBits)) - (a))

/** This converts pixels from straight alpha to premultiplied alpha, when the components are heterogeneous.
 *	\param[in,out]	p		the pixel component, straight in, premultiplied out.
 *	\param[in]		pBits	the number of bits in p.
 *	\param[in]		a		alpha.
 *	\param[in]		aBits	the number of bits in alpha.
 */
#define HETERO_STRAIGHT_TO_PREMUL(p, pBits, a, aBits)	do { ALPHA_MUL(p, a, aBits); while (HETERO_IS_ILLEGAL_PREMUL(p, pBits, a, aBits)) --(p); } while(0)

/** This transfers a straight component in the src to a premultiplied component in the dst.
 *	\param[in]	s		the straight source pixel. A component will be extracted from here.
 *	\param[in]	sBits	the number of bits in the selected component of s and d.
 *	\param[in]	sPos	the    position    of the selected component of s and d.
 *	\param[in]	a		alpha.
 *	\param[in]	aBits	the number of bits in alpha.
 *	\param[out]	d		the premultiplied destination pixel, with space already cleared for ORing in the premultiplied component.
 */
#define HETERO_STRAIGHT_SRC_TO_PREMUL_DST(s, sBits, sPos, a, aBits, d, tmp)	do { tmp = FskMoveField(s, sBits, sPos, 0); HETERO_STRAIGHT_TO_PREMUL(tmp, sBits, a, aBits); d |= tmp << sPos; } while(0)


/*******************************************************************************
 * FskStraightAlphaToPremultipliedAlpha
 *******************************************************************************/

FskErr FskStraightAlphaToPremultipliedAlpha(FskBitmap bm, FskConstRectangle r)
{
	FskErr	err				= kFskErrNone;
	void	*p0;
	UInt8	*a;
	SInt32	bump, pixBytes;
	UInt32	w, h, alpha, pix;
	FskRectangleRecord dstRect;

	if (!bm)
		return kFskErrInvalidParameter;
	if (!bm->hasAlpha || bm->alphaIsPremultiplied)
		return kFskErrNone;
	if (r == NULL)
		dstRect = bm->bounds;
	else if (!FskRectangleIntersect(r, &bm->bounds, &dstRect))
		return kFskErrNothingRendered;

	FskBitmapWriteBegin(bm, &p0, &bump, NULL);
	pixBytes = bm->depth >> 3;
	p0       = (void*)(((char*)p0) + (dstRect.y - bm->bounds.y) * bump + (dstRect.x - bm->bounds.x) * pixBytes);
	bump    -= dstRect.width * pixBytes;

	switch (bm->pixelFormat) {
		case kFskBitmapFormat32ARGB:																/* AAAAAAAA, RRRRRRRR, GGGGGGGG, BBBBBBBB */
		case kFskBitmapFormat32ABGR:	alpha = 0; pix = 1;	goto loop32;							/* AAAAAAAA, BBBBBBBB, GGGGGGGG, RRRRRRRR */
		case kFskBitmapFormat32BGRA:																/* BBBBBBBB, BBBBBBBB, RRRRRRRR, AAAAAAAA */
		case kFskBitmapFormat32RGBA:	alpha = 3; pix = 0;											/* RRRRRRRR, GGGGGGGG, BBBBBBBB, AAAAAAAA */
		loop32: {
			UInt8 *rgb	= (UInt8*)p0 + pix;
			a			= (UInt8*)p0 + alpha;
			for (h = dstRect.height; h--; rgb += bump, a += bump) for (w = dstRect.width; w--; rgb += 4, a += 4) {
				alpha = *a;
				pix = rgb[0]; ALPHA_MUL(pix, alpha, 8); rgb[0] = (UInt8)pix;
				pix = rgb[1]; ALPHA_MUL(pix, alpha, 8); rgb[1] = (UInt8)pix;
				pix = rgb[2]; ALPHA_MUL(pix, alpha, 8); rgb[2] = (UInt8)pix;
			}
		}	break;


		case kFskBitmapFormat16AG:																	/* AAAAAAAA, GGGGGGGG */
		{	UInt8	*a	= (UInt8*)p0 + 0,
					*g	= (UInt8*)p0 + 1;
			for (h = dstRect.height; h--; g += bump, a += bump) for (w = dstRect.width; w--; g += 2, a += 2) {
				alpha = *a;
				pix = *g; ALPHA_MUL(pix, alpha, 8); *g = (UInt8)pix;
			}
		}	break;

		/* LE: 32: AAAAAAAA--------RRRRRGGGGGGBBBBB		16:	RRRRRGGGGGGBBBBB, AAAAAAAA--------		8: GGGBBBBB, RRRRRGGG, --------, AAAAAAAA */
		/* BE: 32: GGGBBBBBRRRRRGGG--------AAAAAAAA		16: GGGBBBBBRRRRRGGG, --------AAAAAAAA		8: GGGBBBBB, RRRRRGGG, --------, AAAAAAAA */
		case kFskBitmapFormat32A16RGB565LE:
		{	UInt32	*p = (UInt32*)p0;
			for (h = dstRect.height; h--; p = (UInt32*)((UInt8*)p + bump)) for (w = dstRect.width; w--; ++p) {
				UInt32	src	= *p;
				UInt32	dst;
				#if TARGET_RT_BIG_ENDIAN
					fskConvert32A16RGB565DE32A16RGB565SE(src);
				#endif /* TARGET_RT_BIG_ENDIAN */
				alpha = FskMoveField(src, fsk32A16RGB565SEAlphaBits, fsk32A16RGB565SEAlphaPosition, 0);		/* Extract alpha from the source ... */
				dst   = alpha << fsk32A16RGB565SEAlphaPosition;												/* ... and deposit it into the destination */
				HETERO_STRAIGHT_SRC_TO_PREMUL_DST(src, fsk32A16RGB565SERedBits,   fsk32A16RGB565SERedPosition,   alpha, fsk32A16RGB565SEAlphaBits, dst, pix);	/* src.r * alpha --> dst.r */
				HETERO_STRAIGHT_SRC_TO_PREMUL_DST(src, fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition, alpha, fsk32A16RGB565SEAlphaBits, dst, pix);	/* src.g * alpha --> dst.g */
				HETERO_STRAIGHT_SRC_TO_PREMUL_DST(src, fsk32A16RGB565SEBlueBits,  fsk32A16RGB565SEBluePosition,  alpha, fsk32A16RGB565SEAlphaBits, dst, pix);	/* src.b * alpha --> dst.b */
				#if TARGET_RT_BIG_ENDIAN
					fskConvert32A16RGB565SE32A16RGB565DE(src);
				#endif /* TARGET_RT_BIG_ENDIAN */
				*p = dst;
			}
		}	break;

		case kFskBitmapFormat16RGBA4444LE:
		default:
			err = kFskErrUnsupportedPixelType;
			goto bail;
	}

	bm->alphaIsPremultiplied = true;

bail:
	FskBitmapWriteEnd(bm);
	return err;
}


/*******************************************************************************
 * FskPremultipliedAlphaToStraightAlpha
 *******************************************************************************/

FskErr FskPremultipliedAlphaToStraightAlpha(FskBitmap bm, FskColorRGB fskBgColor, FskConstRectangle r)
{
	FskErr	err	= kFskErrNone;
	UInt8	*rgb, *a, *bgRGB;
	SInt32	pixBytes, bump;
	UInt32	w, h, alpha, pix;
	FskRectangleRecord dstRect;
	union { UInt32 i; UInt16 s[2]; UInt8 c[4]; } bgColor;	/* We want to assure that this is properly aligned. */

	if (!bm)
		return kFskErrInvalidParameter;
	if (!bm->hasAlpha || !bm->alphaIsPremultiplied)
		return kFskErrNone;
	if (r == NULL)
		dstRect = bm->bounds;
	else if (!FskRectangleIntersect(r, &bm->bounds, &dstRect))
		return kFskErrNothingRendered;

	if (fskBgColor == NULL)
		bgColor.i = 0;
	else
		BAIL_IF_ERR(err = FskConvertColorRGBToBitmapPixel(fskBgColor, bm->pixelFormat, bgColor.c));
	bgRGB = bgColor.c;

	FskBitmapWriteBegin(bm, (void**)(void*)&rgb, &bump, NULL);
	pixBytes = bm->depth >> 3;
	rgb     += (dstRect.y - bm->bounds.y) * bump + (dstRect.x - bm->bounds.x) * pixBytes;
	a        = rgb;
	bump    -= dstRect.width * pixBytes;

	switch (bm->pixelFormat) {
		case kFskBitmapFormat32ARGB:
		case kFskBitmapFormat32ABGR:		++rgb;	++bgRGB;	goto loop32;
		case kFskBitmapFormat32BGRA:
		case kFskBitmapFormat32RGBA:		a += 3;
		loop32:
			for (h = dstRect.height; h--; rgb += bump, a += bump) for (w = dstRect.width; w--; rgb += 4, a += 4) {
				alpha = *a;
				if (alpha == 255)
					continue;
				if (alpha == 0) {	/* Instead of dividing by zero, we replace it with the specified background color */
					rgb[0] = bgRGB[0];
					rgb[1] = bgRGB[1];
					rgb[2] = bgRGB[2];
					continue;
				}
				alpha = ((255U << ALPHA_DIV_BITS) + (alpha >> 1)) / alpha;	/* 255 / alpha * 256, rounded */
				pix = (rgb[0] * alpha + (1 << (ALPHA_DIV_BITS-1))) >> ALPHA_DIV_BITS; if (pix > 255) pix = 255; rgb[0] = (UInt8)pix;
				pix = (rgb[1] * alpha + (1 << (ALPHA_DIV_BITS-1))) >> ALPHA_DIV_BITS; if (pix > 255) pix = 255; rgb[1] = (UInt8)pix;
				pix = (rgb[2] * alpha + (1 << (ALPHA_DIV_BITS-1))) >> ALPHA_DIV_BITS; if (pix > 255) pix = 255; rgb[2] = (UInt8)pix;
			}
			break;
		case kFskBitmapFormat16AG:
			for (h = dstRect.height; h--; rgb += bump, a += bump) for (w = dstRect.width; w--; rgb += 2, a += 2) {
				alpha = *a;
				if (alpha == 255)
					continue;
				if (alpha == 0) {	/* Instead of dividing by zero, we replace it with the specified background color */
					rgb[0] = bgRGB[1];
					continue;
				}
				pix = ((UInt32)(rgb[0]) * 255U + (alpha >> 1)) / alpha; if (pix > 255) pix = 255; rgb[0] = (UInt8)pix;
			}
			break;
		case kFskBitmapFormat32A16RGB565LE:	/* This was implemented, but the quantization was so bad, support has been withdrawn. */
		default:
			err = kFskErrUnsupportedPixelType;
			goto bail;
	}
	bm->alphaIsPremultiplied = false;

bail:
	FskBitmapWriteEnd(bm);
	return err;
}
