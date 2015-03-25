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
#include "FskPixelOps.h"
#include "FskBlitDispatchDef.h"

#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /* !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif // !PRAGMA_MARK_SUPPORTED
#endif /* PRAGMA_MARK_SUPPORTED */

#define UNUSED(x)				(void)(x)


/********************************************************************************
 ********************************************************************************
 **		Macros for blending and interpolation
 ********************************************************************************
 ********************************************************************************/


#define AlphaNScale(c, a, b)				(c *= a, c += (1 << (b - 1)), c += c >> b, c >>= b)
#define GetField(p, n, b)					(((p) >> (n)) & ((1 << (b)) - 1))
#define AlphaScale(c, a)					AlphaNScale(c, a, 8)
#define AlphaLerp(c0, c1, a)				(c1 -= c0, c1 = c1 * a + (c0 << 8) - c0 + 128, c1 += c1 >> 8, c1 >>= 8)
#define AlphaLerpCField(b, f, r, a, n, cz, az) do { SInt32 p, q; p = GetField(f, n, cz); q = GetField(b, n, cz); p = (p-q)*(SInt32)(a) + (q<<(az))-q+(1<<((az)-1)); p += p>>(az); r &= ~(((1 << (cz)) - 1) << (n)); r |= ((p>>(az))<<(n)); } while(0)
#define AlphaLerpAField(b,    r, a, n, cz, az) do { SInt32 q = GetField(b, n, cz);       q = q * (SInt32)(((1<<(az))-1) - (a))  + ((a)<<(cz)) - (a) +(1<<((az)-1)); q += q>>(az); r &= ~(((1 << (cz)) - 1) << (n)); r |= ((q>>(az))<<(n)); } while(0)
#define MulComp(p0, p1, p2, n0, n1, n2, b)	(c0 = GetField(p0, n0, b), c1 = GetField(p1, n1, b), AlphaNScale(c0, c1, b), p2 |= (SInt32)c0 << n2)
#define FracLerp(f, p0, p1)					(((p1 - p0) * f) + (p0 << 4))


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***		Exportable pixelwise functions
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 ********************************************************************************
 **		Pixel Conversions
 ********************************************************************************
 ********************************************************************************/

#define COEFF_Y		1192
#define COEFF_RV	1634
#define COEFF_GU	(-401)
#define COEFF_GV	(-832)
#define COEFF_BU	2066
#define COEFF_BITS	10



/********************************************************************************
 * FskConvertYUV42032ARGB
 ********************************************************************************/

UInt32
FskConvertYUV42032ARGB(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	register UInt32	pix		= fskDefaultAlpha << fsk32ARGBAlphaPosition;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ARGBRedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ARGBGreenPosition;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ARGBBluePosition;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42032BGRA
 ********************************************************************************/

UInt32
FskConvertYUV42032BGRA(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	register UInt32	pix		= fskDefaultAlpha << fsk32BGRAAlphaPosition;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32BGRARedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32BGRAGreenPosition;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32BGRABluePosition;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42032RGBA
 ********************************************************************************/

UInt32
FskConvertYUV42032RGBA(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	register UInt32	pix		= fskDefaultAlpha << fsk32RGBAAlphaPosition;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32RGBARedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32RGBAGreenPosition;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32RGBABluePosition;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42032A16RGB565SE
 ********************************************************************************/

UInt32
FskConvertYUV42032A16RGB565SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	register UInt32	pix		= fskDefaultAlpha << fsk32A16RGB565SEAlphaPosition;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix  = i << fsk16BGR565SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-6-1))) >> (COEFF_BITS+8-6);	if (i<0) i=0;	else if (i>63) i=63;	pix |= i << fsk16BGR565SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR565SEBluePosition;
	return(pix);
}

/********************************************************************************
 * FskConvertYUV42032ABGR
 ********************************************************************************/

UInt32
FskConvertYUV42032ABGR(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	register UInt32	pix		= fskDefaultAlpha << fsk32ABGRAlphaPosition;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ABGRRedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ABGRGreenPosition;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix |= i << fsk32ABGRBluePosition;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42024RGB
 ********************************************************************************/

Fsk24BitType
FskConvertYUV42024RGB(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	Fsk24BitType	pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24RGBRedPosition  ] = (UInt8)i;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24RGBGreenPosition] = (UInt8)i;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24RGBBluePosition ] = (UInt8)i;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42024BGR
 ********************************************************************************/

Fsk24BitType
FskConvertYUV42024BGR(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y + (1 << (COEFF_BITS - 1));	/* Pre-round Y */
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	Fsk24BitType	pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24BGRRedPosition  ] = (UInt8)i;
	i = (yp + COEFF_GU * up + COEFF_GV * vp) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24BGRGreenPosition] = (UInt8)i;
	i = (yp + COEFF_BU * up                ) >> COEFF_BITS;	if (i<0) i=0;	else if (i>255) i=255;	pix.c[fsk24BGRBluePosition ] = (UInt8)i;
	return(pix);
}


/********************************************************************************
 * FskConvertYUV42016RGB565SE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGB565SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix  = i << fsk16RGB565SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-6-1))) >> (COEFF_BITS+8-6);	if (i<0) i=0;	else if (i>63) i=63;	pix |= i << fsk16RGB565SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB565SEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016BGR565SE
 ********************************************************************************/

UInt16
FskConvertYUV42016BGR565SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix  = i << fsk16BGR565SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-6-1))) >> (COEFF_BITS+8-6);	if (i<0) i=0;	else if (i>63) i=63;	pix |= i << fsk16BGR565SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR565SEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016RGB565DE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGB565DE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix  = i << fsk16RGB565DERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-6-1))) >> (COEFF_BITS+8-6);	if (i<0) i=0;	else if (i>63) i=63;	pix |= i << fsk16RGB565DEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB565DEBluePosition;
	pix |= pix >> 16;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016BGR565DE
 ********************************************************************************/

UInt16
FskConvertYUV42016BGR565DE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix  = i << fsk16BGR565DERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-6-1))) >> (COEFF_BITS+8-6);	if (i<0) i=0;	else if (i>63) i=63;	pix |= i << fsk16BGR565DEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR565DEBluePosition;
	pix |= pix >> 16;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016RGB5515SE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGB5515SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha1 << fsk16RGB5515SEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515SEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016BGR5515SE
 ********************************************************************************/

UInt16
FskConvertYUV42016BGR5515SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha1 << fsk16BGR5515SEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515SEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016RGB5515DE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGB5515DE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha1 << fsk16RGB5515DEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515DERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515DEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16RGB5515DEBluePosition;
	pix |= pix >> 16;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016BGR5515DE
 ********************************************************************************/

UInt16
FskConvertYUV42016BGR5515DE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha1 << fsk16BGR5515DEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515DERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515DEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-5-1))) >> (COEFF_BITS+8-5);	if (i<0) i=0;	else if (i>31) i=31;	pix |= i << fsk16BGR5515DEBluePosition;
	pix |= pix >> 16;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016RGBA4444SE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGBA4444SE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha4 << fsk16RGBA4444SEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444SERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444SEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444SEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV42016RGBA4444DE
 ********************************************************************************/

UInt16
FskConvertYUV42016RGBA4444DE(UInt8 y, UInt8 u, UInt8 v)
{
	SInt32			yp		= ((SInt32)(y) - 16) * COEFF_Y;
	SInt32			up		= ( SInt32)(u) - 128;
	SInt32			vp		= ( SInt32)(v) - 128;
	UInt32			pix;
	register SInt32	i;

	pix = fskDefaultAlpha4 << fsk16RGBA4444DEAlphaPosition;
	i = (yp                 + COEFF_RV * vp + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444DERedPosition;
	i = (yp + COEFF_GU * up + COEFF_GV * vp + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444DEGreenPosition;
	i = (yp + COEFF_BU * up                 + (1<<(COEFF_BITS+8-4-1))) >> (COEFF_BITS+8-4);	if (i<0) i=0;	else if (i>15) i=15;	pix |= i << fsk16RGBA4444DEBluePosition;
	return((UInt16)pix);
}


/********************************************************************************
 * FskConvertYUV4208G
 ********************************************************************************/

UInt8
FskConvertYUV4208G(UInt8 y, UInt8 u, UInt8 v)
{
	register UInt32	pix;

	UNUSED(u);
	UNUSED(v);

	if (y <= 16)
		pix = 0;
	else {
		pix = ((y - 16) * COEFF_Y + (1 << (COEFF_BITS - 1))) >> COEFF_BITS;
		if (pix > 255)
			pix = 255;
	}

	return (UInt8)pix;
}


/********************************************************************************
 * FskConvertYUV42016AG
 ********************************************************************************/

UInt16
FskConvertYUV42016AG(UInt8 y, UInt8 u, UInt8 v)
{
	register UInt32	pix;

	UNUSED(u);
	UNUSED(v);

	if (y <= 16)
		pix = 0;
	else {
		pix = ((y - 16) * COEFF_Y + (1 << (COEFF_BITS - 1))) >> COEFF_BITS;
		if (pix > 255)
			pix = 255;
	}
	pix <<= fsk16AGGrayPosition;
	pix |= fskDefaultAlpha << fsk16AGAlphaPosition;

	return (UInt16)pix;
}


/********************************************************************************
 * FskConvertYUV42016GA
 ********************************************************************************/

UInt16
FskConvertYUV42016GA(UInt8 y, UInt8 u, UInt8 v)
{
	register UInt32	pix;

	UNUSED(u);
	UNUSED(v);

	if (y <= 16)
		pix = 0;
	else {
		pix = ((y - 16) * COEFF_Y + (1 << (COEFF_BITS - 1))) >> COEFF_BITS;
		if (pix > 255)
			pix = 255;
	}
	pix <<= fsk16GAGrayPosition;
	pix |= fskDefaultAlpha << fsk16GAAlphaPosition;

	return (UInt16)pix;
}


/********************************************************************************
 * FskConvertRGBHSL
 *
 *	float v = MAX(r, g, b);
 *	float m = MIN(r, g, b);
 *	if ((*l = (m + v) / 2.0) <= 0.0)
 *		return;
 *	if ((*s = vm = v - m) > 0.0)
 *		*s /= (*l <= 0.5) ? (v + m) : (2.0 - v - m);
 *	else
 *		return;
 *	float r2 = (v - r) / vm;
 *	float g2 = (v - g) / vm;
 *	float b2 = (v - b) / vm;
 *
 *	if      (r == v)	*h = (g == m ? 5.0 + b2 : 1.0 - g2);
 *	else if (g == v)	*h = (b == m ? 1.0 + r2 : 3.0 - b2);
 *	else				*h = (r == m ? 3.0 + g2 : 5.0 - r2);
 *	*h /= 6;
 ********************************************************************************/

void
FskConvertRGBHSL(const UInt8 *rgb, UInt8 *hsl)
{
	int v, m, vm, r2, g2, b2, hue;

	/* Compute max */
	v = rgb[0];
	if (v < rgb[1])
		v = rgb[1];
	if (v < rgb[2])
		v = rgb[2];

	/* Compute min */
	m = rgb[0];
	if (m > rgb[1])
		m = rgb[1];
	if (m > rgb[2])
		m = rgb[2];

	/* Compute lightness */
	if ((hsl[2] = (m + v) >> 1) <= 0)
		return;		/* When lightness is 0, hue and saturation are immaterial */

	/* Compute saturation */
	if ((hsl[1] = vm = v - m) > 0) {
		int den = ((int)hsl[2] <= 255/2) ? (v + m) : (2 * 255 - v - m);
		hsl[1] = (255 * (int)hsl[1] + (den >> 1)) / den;
	}
	else
		return;		/* When saturation is 0, hue is immaterial */

	r2 = ((v - (int)rgb[0]) * 255 + (vm >> 1)) / vm;
	g2 = ((v - (int)rgb[1]) * 255 + (vm >> 1)) / vm;
	b2 = ((v - (int)rgb[2]) * 255 + (vm >> 1)) / vm;

	/* Compute hue */
	if      ((int)rgb[0] == v)	hue = ((int)rgb[1] == m) ? (5 * 255 + b2) : (1 * 255 - g2);
	else if ((int)rgb[1] == v)	hue = ((int)rgb[2] == m) ? (1 * 255 + r2) : (3 * 255 - b2);
	else						hue = ((int)rgb[0] == m) ? (3 * 255 + g2) : (5 * 255 - r2);
	hue /= 6;
	hsl[0] = hue;	/* This assignment is modulo 256 */
}


/********************************************************************************
 *	FskConvertHSLRGB
 *
 *	float v = (L <= 0.5) ? L * (1.0 + S) : (L + S - L * S);
 *	if (v <= 0) {
 *		R = G = B = 0;
 *	}
 *	else {
 *		float m       = 2 * L - v;
 *		float sv      = (v - m) / v;
 *		int   sextant = (int)(H *= 6);
 *		float fract   = H - sextant;
 *		float vsf     = v * sv * fract;
 *		float mid1    = m + vsf;
 *		float mid2    = v - vsf;
 *		switch (sextant) {
 *			case 0:		R = v;		G = mid1;	B = m;		break;
 *			case 1:		R = mid2;	G = v;		B = m;		break;
 *			case 2:		R = m;		G = v;		B = mid1;	break;
 *			case 3:		R = m;		G = mid2;	B = v;		break;
 *			case 4:		R = mid1;	G = m;		B = v;		break;
 *			case 5:		R = v;		G = m;		B = mid2;	break;
 *		}
 *	}
 ********************************************************************************/

void
FskConvertHSLRGB(const UInt8 *hsl, UInt8 *rgb)
{
	int v;

	v = (hsl[2] <= 255/2)	? (((int)hsl[2] * (255 + (int)hsl[1])) + (255 >> 1)) / 255
							: (int)hsl[2] + (int)hsl[1] - ((int)hsl[2] * (int)hsl[1] + (255 >> 1)) / 255;
	if (v <= 0) {
		rgb[0] = rgb[1] = rgb[2] = 0;
	}
	else {
		int m, sv, h, fract, vsf, mid1, mid2, sextant;
		m = (int)hsl[2] + (int)hsl[2] - v;
		sv = (((v - m) << 8) + (v >> 1)) / v;	/* 24.8: [0, 255/256] */
		h = (int)hsl[0] * 6 * 257;				/* 16.16 */
		h = (h + (1 << (8 - 1))) >> 8;			/* 24.8: [0, 5 + 255/256] */
		sextant = h >> 8;
		if (sextant > 5)
			sextant -= 6;
		fract = h & 0xFF;						/* 24.8: [0, 255/256] */
		vsf = (v * sv * fract + (1 << (16 - 1))) >> 16;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant) {
			case 0:		rgb[0] = v;		rgb[1] = mid1;	rgb[2] = m;		break;
			case 1:		rgb[0] = mid2;	rgb[1] = v;		rgb[2] = m;		break;
			case 2:		rgb[0] = m;		rgb[1] = v;		rgb[2] = mid1;	break;
			case 3:		rgb[0] = m;		rgb[1] = mid2;	rgb[2] = v;		break;
			case 4:		rgb[0] = mid1;	rgb[1] = m;		rgb[2] = v;		break;
			case 5:		rgb[0] = v;		rgb[1] = m;		rgb[2] = mid2;	break;
		}
	}
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 **		Bilinear interpolation
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskBilerp32
 * 4 bit fractions
 ********************************************************************************/

#if 0 //move the asm part to function my_FskBilinearCopy32sameSrcDst_neon in file FskRectBlit.c
UInt32
FskBilerp32_neon_di_dj(UInt32 di, UInt32 dj, const UInt32 *s, SInt32 rb) {
	UInt32 p = 0, di0 = 0, dj0 = 0;
#if !__clang__
	asm volatile(".fpu neon\n");
#endif
	asm volatile(
	"vld1.32	d0,		[%[s]],	%[rb]	\n"
	"vld1.32	d1,		[%[s]]			\n"
	"vzip.32	d0,		d1				\n"
	"rsb		%[di0],	%[di],	#16		\n"
	"vdup.8		d3,		%[di0]			\n"
	"vdup.8		d2,		%[di]			\n"
	"rsb		%[dj0],	%[dj],	#16		\n"
	"vdup.16	d5,		%[dj0] 			\n"
	"vdup.16	d4,		%[dj] 			\n"
	"vmull.u8	q4,		d0,		d3		\n"
	"vmlal.u8	q4,		d1,		d2		\n"
	"vmul.u16	d0,		d8,		d5		\n"
	"vmla.u16	d0,		d9,		d4		\n"
	"vrshr.u16	d0,		d0,		#8		\n"
	"vmovn.i16	d10,	q0				\n"
	"vmov.32	%[p],	d10[0]			\n"
	:[p] "+&r" (p),[s] "+&r" (s),[di0] "+&r" (di0),[dj0] "+&r" (dj0)
	:[di] "r" (di),[dj] "r" (dj),[rb] "r" (rb)
	: "cc", "memory",
	"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
	"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
	"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
	"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
	);
	return p;
}

UInt32
FskBilerp32_neon_dj(UInt32 dj, const UInt32 *s, SInt32 rb) {
	UInt32 p = 0, dj0 = 0;
#if !__clang__
	asm volatile(".fpu neon\n");
#endif
	asm volatile(
	"ldr		%[p],	[%[s]]			\n"
	"ldr		%[dj0],	[%[s],%[rb]]	\n"
	"vmov		d0,		%[p],	%[dj0]	\n"
	"rsb		%[dj0],	%[dj],	#16		\n"
	"vdup.8		d5,		%[dj0] 			\n"
	"vdup.8		d4,		%[dj] 			\n"
	"vext.8		d4,		d5,		d4, #4	\n"
	"vmull.u8	q4,		d0,		d4		\n"
	"vadd.u16	d8,		d8,		d9		\n"
	"vrshr.u16	d8,		d8,		#4		\n"
	"vmovn.i16	d10,	q4				\n"
	"vmov.32	%[p],	d10[0]			\n"
	:[p] "+&r" (p),[s] "+&r" (s),[dj0] "+&r" (dj0)
	:[dj] "r" (dj),[rb] "r" (rb)
	: "cc", "memory",
	"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
	"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
	"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
	"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
	);
	return p;
}

UInt32
FskBilerp32_neon_di(UInt32 di, const UInt32 *s) {
	UInt32 p = 0, di0 = 0;
#if !__clang__
	asm volatile(".fpu neon\n");
#endif
	asm volatile(
	"vld1.32	d0,		[%[s]]			\n"
	"rsb		%[di0],	%[di],	#16		\n"
	"vdup.8		d5,		%[di0] 			\n"
	"vdup.8		d4,		%[di] 			\n"
	"vext.8		d4,		d5,		d4, #4	\n"
	"vmull.u8	q4,		d0,		d4		\n"
	"vadd.u16	d8,		d8,		d9		\n"
	"vrshr.u16	d8,		d8,		#4		\n"
	"vmovn.i16	d10,	q4				\n"
	"vmov.32	%[p],	d10[0]			\n"
	:[p] "+&r" (p)
	:[di] "r" (di),[di0] "r" (di0),[s] "r" (s)
	: "cc", "memory",
	"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
	"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
	"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
	"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
	);
	return p;
}
#endif //defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
UInt32
FskBilerp32(UInt32 di, UInt32 dj, const UInt32 *s, SInt32 rb)
{
#if 0
	UInt32 p00, p01, p10, p11, p;
	UInt32 di0, dj0, mask;

	p00 = *s;
	p01 = sizeof(UInt32);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt32*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt32*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt32*)(((const char*)(s)) + (SInt32)p11));

	di0 = (1 << 4)  - di;
	dj0 = (1 << 4) - dj;

	mask = 0x00FF00FF;
	p   = ((((((p00 >> 0) & mask) * di0) + (((p01 >> 0) & mask) * di)) * dj0
		+   ((((p10 >> 0) & mask) * di0) + (((p11 >> 0) & mask) * di)) * dj
		+   0x00800080
		) >> 8) & mask;
	p  |= ((((((p00 >> 8) & mask) * di0) + (((p01 >> 8) & mask) * di)) * dj0
		+   ((((p10 >> 8) & mask) * di0) + (((p11 >> 8) & mask) * di)) * dj
		+   0x00800080
		) >> 0) & (~mask);

	return(p);
#elif 0 //neon code for reference
	if(dj != 0) {
		if(di!=0)
			return FskBilerp32_neon_di_dj(di,dj,s,rb);
		else
			return FskBilerp32_neon_dj(dj,s,rb);
	}
	else {
		if(di!=0)
			return FskBilerp32_neon_di(di,s);
		else
			return *s;
	}
#else	/* This looks like it should be faster, but it uses branches whereas the above uses conditionals, so it is a wash for speed, though it does take less power */
	const UInt32 m00FF00FF = 0x00FF00FF;
	const UInt32 m0FF00FF0 = 0x0FF00FF0;
	UInt32 p00, p01, p10, p11, p, di0, dj0;

	p00 = *s;
	di0 = (1 << 4) - di;

	if (dj != 0) {
		p10 = *((UInt32*)(((const char*)s) + rb));
		dj0 = (1 << 4) - dj;
		if (di != 0) {																		/* Interpolate in X and Y */
			p01 = s[1];
			p11 = *((UInt32*)(((const char*)s) + rb + sizeof(UInt32)));
			p   = ((((((p00 >> 0) & m00FF00FF) * di0) + (((p01 >> 0) & m00FF00FF) * di)) * dj0
				+   ((((p10 >> 0) & m00FF00FF) * di0) + (((p11 >> 0) & m00FF00FF) * di)) * dj
				+   0x00800080
				) >> 8) & m00FF00FF;
			p  |= ((((((p00 >> 8) & m00FF00FF) * di0) + (((p01 >> 8) & m00FF00FF) * di)) * dj0
				+   ((((p10 >> 8) & m00FF00FF) * di0) + (((p11 >> 8) & m00FF00FF) * di)) * dj
				+   0x00800080
				) >> 0) & (~m00FF00FF);
		}
		else {																				/* Interpolate in Y */
			p  = ((((p00 >> 0) & m00FF00FF) * dj0 + ((p10 >> 0) & m00FF00FF) * dj + 0x00080008) >> 4) &  m00FF00FF;
			p |= ((((p00 >> 4) & m0FF00FF0) * dj0 + ((p10 >> 4) & m0FF00FF0) * dj + 0x00800080) >> 0) & ~m00FF00FF;
		}
	}
	else {
		if (di != 0) {																		/* Interpolate in X */
			p01 = s[1];
			p  = ((((p00 >> 0) & m00FF00FF) * di0 + ((p01 >> 0) & m00FF00FF) * di + 0x00080008) >> 4) &  m00FF00FF;
			p |= ((((p00 >> 4) & m0FF00FF0) * di0 + ((p01 >> 4) & m0FF00FF0) * di + 0x00800080) >> 0) & ~m00FF00FF;
		}
		else {																				/* No interpolation necessary */
			p = p00;
		}
	}
	return p;

#endif
}


/********************************************************************************
 * FskBilerp32A16RGB565SE
 * 4 bit fractions
 ********************************************************************************/

UInt32
FskBilerp32A16RGB565SE(UInt32 di, UInt32 dj, const UInt32 *s, SInt32 rb)
{
#if 0
	UInt32 p00, p01, p10, p11, di0, dj0, alpha, mask1, mask2;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt32);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt32*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt32*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt32*)(((const char*)(s)) + (SInt32)p11));

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* serial alpha calculation */
	alpha = (((p00 >> 24) * di0) + ((p01 >> 24) * di)) * dj0
		  + (((p10 >> 24) * di0) + ((p11 >> 24) * di)) * dj
		  + (1 << (8 - 1));
	alpha >>= 8;

	/* strip alpha */
	mask1 = 0x0ffff;
	p00 &= mask1;
	p01 &= mask1;
	p10 &= mask1;
	p11 &= mask1;

	/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
	mask1 = 0x07E0F81F;
	p00 |= p00 << 16;	p00 &= mask1;
	p01 |= p01 << 16;	p01 &= mask1;
	p10 |= p10 << 16;	p10 &= mask1;
	p11 |= p11 << 16;	p11 &= mask1;

	/* Interpolate R,G,B in parallel */
 	mask2 = 0x0FE1F83F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
	p00 = ((p00 + 0x02008010) >> 5) & mask1;
	p00 |= (p00 >> 16);	/* Weave G between R and B */

	return ((UInt16)p00) | (alpha << 24);
#else
	const UInt32 m16	= 0x0000FFFF;	/* ----------------1111111111111111 */
	const UInt32 mGRB	= 0x07E0F81F;	/* -----GGGGGG-----RRRRR------BBBBB */
	UInt32 p00, p01, p10, p11, di0, dj0, alpha;

	p00 = *s;																		/* Get upper left pixel */
	di0 = (1 << 4) - di;

	if (dj != 0) {
		p10 = *((UInt32*)(((const char*)s) + rb));									/* Get lower left pixel */
		dj0 = (1 << 4) - dj;
		if (di != 0) {																/* Interpolate in X and Y */
			p01 = s[1];																/* Get upper right pixel */
			p11 = *((UInt32*)(((const char*)s) + rb + sizeof(UInt32)));				/* Get lower right pixel */
			alpha = ((((p00 >> 24) * di0) + ((p01 >> 24) * di)) * dj0 +
					 (((p10 >> 24) * di0) + ((p11 >> 24) * di)) * dj  +
					 (1 << (8 - 1))) >> 8;
			p00 &= m16;	p00 |= p00 << 16;	p00 &= mGRB;							/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p01 &= m16;	p01 |= p01 << 16;	p01 &= mGRB;
			p10 &= m16;	p10 |= p10 << 16;	p10 &= mGRB;
			p11 &= m16;	p11 |= p11 << 16;	p11 &= mGRB;
			p00	= (((p00 * di0 + p01 * di) >> 3) & 0x0FE1F83F) * dj0				/* Interpolate R,G,B in parallel */
				+ (((p10 * di0 + p11 * di) >> 3) & 0x0FE1F83F) * dj;
			p00 = ((p00 + 0x02008010) >> 5) & mGRB;
			p00 |= (p00 >> 16);	p00 &= m16;	p00 |= alpha << 24;						/* Reassemble */
		}
		else {																		/* Interpolate in Y */
			alpha = ((p00 >> 24) * dj0 + (p10 >> 24) * dj + (1 << (4 - 1))) >> 4;	/* Interpolate alpha */
			p00 &= m16;	p00 |= p00 << 16;	p00 &= mGRB;							/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p10 &= m16;	p10 |= p10 << 16;	p10 &= mGRB;
			p00  = ((p00 * dj0 + p10 * dj + 0x01004008) >> 4) & mGRB;				/* Interpolate R,G,B in parallel */
			p00 |= (p00 >> 16);	p00 &= m16;	p00 |= alpha << 24;						/* Reassemble */
		}
	}
	else {
		if (di != 0) {																/* Interpolate in X */
			p01 = s[1];																/* Get upper right pixel */
			alpha = ((p00 >> 24) * di0 + (p01 >> 24) * di + (1 << (4 - 1))) >> 4;	/* Interpolate alpha */
			p00 &= m16;	p00 |= p00 << 16;	p00 &= mGRB;							/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p01 &= m16;	p01 |= p01 << 16;	p01 &= mGRB;
			p00  = ((p00 * di0 + p01 * di + 0x01004008) >> 4) & mGRB;				/* Interpolate R,G,B in parallel */
			p00 |= (p00 >> 16);	p00 &= m16;	p00 |= alpha << 24;						/* Reassemble */
		}
		else {																		/* No interpolation necessary */
		}
	}
	return p00;
#endif
}

/********************************************************************************
 * FskBilerp24
 * 4 bit fractions
 ********************************************************************************/

Fsk24BitType
FskBilerp24(UInt32 di, UInt32 dj, const Fsk24BitType *s, SInt32 rb)
{
	UInt32			di0, dj0;
	const UInt8		*cs			= (const UInt8*)s;
	Fsk24BitType	d;

	di0 = (1 << 4) - di;

	if (dj != 0) {
		dj0 = (1 << 4) - dj;
		if (di != 0) {
			d.c[0] = (UInt8)(((cs[0+0+0] * di0 + cs[0+3+0] * di) * dj0 + (cs[rb+0+0] * di0 + cs[rb+3+0] * di) * dj + 0x80) >> 8);
			d.c[1] = (UInt8)(((cs[0+0+1] * di0 + cs[0+3+1] * di) * dj0 + (cs[rb+0+1] * di0 + cs[rb+3+1] * di) * dj + 0x80) >> 8);
			d.c[2] = (UInt8)(((cs[0+0+2] * di0 + cs[0+3+2] * di) * dj0 + (cs[rb+0+2] * di0 + cs[rb+3+2] * di) * dj + 0x80) >> 8);
		}
		else {
			d.c[0] = (UInt8)((cs[0+0+0] * dj0 + cs[rb+0+0] * dj + 0x08) >> 4);
			d.c[1] = (UInt8)((cs[0+0+1] * dj0 + cs[rb+0+1] * dj + 0x08) >> 4);
			d.c[2] = (UInt8)((cs[0+0+2] * dj0 + cs[rb+0+2] * dj + 0x08) >> 4);
		}
	}
	else {
		if (di != 0) {
			d.c[0] = (UInt8)((cs[0+0+0] * di0 + cs[0+3+0] * di + 0x08) >> 4);
			d.c[1] = (UInt8)((cs[0+0+1] * di0 + cs[0+3+1] * di + 0x08) >> 4);
			d.c[2] = (UInt8)((cs[0+0+2] * di0 + cs[0+3+2] * di + 0x08) >> 4);
		}
		else {
			d.c[0] = cs[0];
			d.c[1] = cs[1];
			d.c[2] = cs[2];
		}
	}
	return(d);
}


/********************************************************************************
 * FskBilerp565SE
 * 4 bit fractions
 * ----------------RRRRRGGGGGGBBBBB = input format
 * -----GGGGGG-----RRRRR------BBBBB = initial computation format
 * 00000111111000001111100000011111 = mask1 = 0x07E0F81F
 * -GGGGGGGGGG-RRRRRRRRR--BBBBBBBBB = first intermediate computation format
 * --------1---------1----------1-- = first rounding
 * 00000000100000000010000000000100 = first rounding  = 0x00802004
 * ----GGGGGGG----RRRRRR-----BBBBBB = second intermediate computation format
 * 00001111111000011111100000111111 = mask2 = 0x0FE1F83F
 * GGGGGGGGGGGRRRRRRRRRR-BBBBBBBBBB = third intermediate computation format
 * ------1---------1----------1---- = second rounding
 * 00000010000000001000000000010000 = second rounding = 0x02008010
 * -----GGGGGG-----RRRRR------BBBBB = fourth computation format (same as the first)
 * ----------------RRRRRGGGGGGBBBBB = resultant format (same as the input)
 ********************************************************************************/

UInt16
FskBilerp565SE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
#if 0
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask1, mask2;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
	mask1 = 0x07E0F81F;
	p00 |= p00 << 16;	p00 &= mask1;
	p01 |= p01 << 16;	p01 &= mask1;
	p10 |= p10 << 16;	p10 &= mask1;
	p11 |= p11 << 16;	p11 &= mask1;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate */
 	mask2 = 0x0FE1F83F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
	p00 = ((p00 + 0x02008010) >> 5) & mask1;
	p00 |= (p00 >> 16);	/* Weave G between R and B */

	return (UInt16)p00;
#else	/* This looks like it should be faster, but it uses branches whereas the above uses conditionals, so it is a wash for speed, though it does take less power */
	const UInt32 mGRB	= 0x07E0F81F;	/* -----GGGGGG-----RRRRR------BBBBB */
	UInt32 p00, p01, p10, p11, di0, dj0;

	p00 = *s;	p00 |= p00 << 16;	p00 &= mGRB;									/* Get upper left pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
	di0 = (1 << 4) - di;

	if (dj != 0) {
		p10 = *((UInt16*)(((const char*)s) + rb)); p10 |= p10 << 16; p10 &= mGRB;	/* Get lower left pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
		dj0 = (1 << 4) - dj;
		if (di != 0) {																/* Interpolate in X and Y */
			p01 = s[1];	p01 |= p01 << 16;	p01 &= mGRB;							/* Get upper right pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p11 = *((UInt16*)(((const char*)s) + rb + sizeof(UInt16))); p11 |= p11 << 16; p11 &= mGRB;	/* Get lower right pixel, formatted ... */
			p00	= (((p00 * di0 + p01 * di) >> 3) & 0x0FE1F83F) * dj0				/* Interpolate R,G,B in parallel */
				+ (((p10 * di0 + p11 * di) >> 3) & 0x0FE1F83F) * dj;
			p00 = ((p00 + 0x02008010) >> 5) & mGRB;
		}
		else {																		/* Interpolate in Y */
			p00  = ((p00 * dj0 + p10 * dj + 0x01004008) >> 4) & mGRB;				/* Interpolate R,G,B in parallel */
		}
	}
	else {
		if (di != 0) {																/* Interpolate in X */
			p01 = s[1];	p01 |= p01 << 16;	p01 &= mGRB;							/* Get upper right pixel, formatted as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
			p00  = ((p00 * di0 + p01 * di + 0x01004008) >> 4) & mGRB;				/* Interpolate R,G,B in parallel */
		}
		else {																		/* No interpolation necessary */
		}
	}
	p00 |= (p00 >> 16);																/* Reassemble */
	return (UInt16)p00;
#endif
}


/********************************************************************************
 * FskBilerp565DE
 * 4 bit fractions
 * ----------------gggbbbbbrrrrrGGG = input format
 * ------bbbbb-----GGGggg-----rrrrr = initial computation format
 * 00000011111000001111110000011111 = mask1 = 0x03E0FC1F
 * --bbbbbbbbb-gggggggggg-rrrrrrrrr = second computation format
 * --------1----------1---------1-- = first rounding
 * 00000000100000000001000000000100 = first rounding  = 0x00801004
 * -----bbbbbb----ggggggg----rrrrrr = third computation format
 * 00000111111000011111110000111111 = mask2 = 0x07E1FC3F
 * -bbbbbbbbbbgggggggggggrrrrrrrrrr = fourth computation format
 * ------1----------1---------1---- = second rounding
 * 00000010000000000100000000010000 = second rounding = 0x02004010
 ********************************************************************************/

UInt16
FskBilerp565DE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask1, mask2;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 000000bbbbb00000GGGggg00000rrrrr */
	mask1 = 0x03E0FC1F;
	p00 |= p00 << 16;	p00 >>= 3;	p00 &= mask1;
	p01 |= p01 << 16;	p01 >>= 3;	p01 &= mask1;
	p10 |= p10 << 16;	p10 >>= 3;	p10 &= mask1;
	p11 |= p11 << 16;	p11 >>= 3;	p11 &= mask1;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate */
 	mask2 = 0x07E1FC3F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
	p00 = ((p00 + 0x02004010) >> 5) & mask1;
	p00 <<= 3;			/* Make room for GGG at the bottom */
	p00 |= (p00 >> 16);	/* Weave B between g and R, and put G in MSB's */

	return((UInt16)p00);
}


/********************************************************************************
 * FskBilerp5515SE
 * 4 bit fractions
 * ----------------RRRRRGGGGGABBBBB = input format
 * -----GGGGG------RRRRR------BBBBB = initial computation format
 * 00000111110000001111100000011111 = mask1 = 0x07C0F81F
 * -GGGGGGGGG--RRRRRRRRR--BBBBBBBBB = first intermediate computation format
 * --------1---------1----------1-- = first rounding
 * 00000000100000000010000000000100 = first rounding  = 0x00802004
 * ----GGGGGGG----RRRRRR-----BBBBBB = second intermediate computation format
 * 00001111111000011111100000111111 = mask2 = 0x0FE1F83F
 * GGGGGGGGGGGRRRRRRRRRR-BBBBBBBBBB = third intermediate computation format
 * -----1----------1----------1---- = second rounding
 * 00000100000000001000000000010000 = second rounding = 0x04008010
 * -----GGGGG------RRRRR------BBBBB = fourth computation format (same as the first)
 * ----------------RRRRRGGGGG-BBBBB = resultant format (same as the input)
 ********************************************************************************/

UInt16
FskBilerp5515SE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask1, mask2;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 00000 GGGGG0 00000 RRRRR 000000 BBBBB */
	mask1 = 0x07C0F81F;
	p00 |= p00 << 16;	p00 &= mask1;
	p01 |= p01 << 16;	p01 &= mask1;
	p10 |= p10 << 16;	p10 &= mask1;
	p11 |= p11 << 16;	p11 &= mask1;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate RGB */
 	mask2 = 0x0FE1F83F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
	p00 = ((p00 + 0x04008010) >> 5) & mask1;

	/* Interpolate alpha */
	p00 |= (((	(((*s  >> fsk16RGB5515SEAlphaPosition) & 1) * di0 + ((p01 >> fsk16RGB5515SEAlphaPosition) & 1) * di) * dj0
			+	(((p10 >> fsk16RGB5515SEAlphaPosition) & 1) * di0 + ((p11 >> fsk16RGB5515SEAlphaPosition) & 1) * di) * dj
	) + (1 << 7)) >> 8) << fsk16RGB5515SEAlphaPosition;

	p00 |= (p00 >> 16);	/* Weave G between R and B */

	return((UInt16)p00);
}


/********************************************************************************
 * FskBilerp5515DE
 * 4 bit fractions
 * ----------------ggAbbbbbrrrrrGGG = input format
 * ------bbbbb-----GGGgg------rrrrr = initial computation format
 * 00000011111000001111100000011111 = mask1 = 0x03E0F81F
 * --bbbbbbbbb-gggggggggg-rrrrrrrrr = second computation format
 * --------1----------1---------1-- = first rounding
 * 00000000100000000001000000000100 = first rounding  = 0x00801004
 * -----bbbbbb----ggggggg----rrrrrr = third computation format
 * 00000111111000011111110000111111 = mask2 = 0x07E1FC3F
 * -bbbbbbbbbbgggggggggggrrrrrrrrrr = fourth computation format
 * ------1---------1----------1---- = second rounding
 * 00000010000000001000000000010000 = second rounding = 0x02008010
 ********************************************************************************/

UInt16
FskBilerp5515DE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask1, mask2;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 000000bbbbb00000GGGgg000000rrrrr */
	mask1 = 0x03E0F81F;
	p00 |= p00 << 16;	p00 >>= 3;	p00 &= mask1;
	p01 |= p01 << 16;	p01 >>= 3;	p01 &= mask1;
	p10 |= p10 << 16;	p10 >>= 3;	p10 &= mask1;
	p11 |= p11 << 16;	p11 >>= 3;	p11 &= mask1;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate */
 	mask2 = 0x07E1FC3F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask2) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask2) * dj;
	p00 = ((p00 + 0x02008010) >> 5) & mask1;

	/* Interpolate alpha */
	p00 |= (((	(((*s  >>  fsk16RGB5515SEAlphaPosition     ) & 1) * di0 + ((p01 >> (fsk16RGB5515SEAlphaPosition - 3)) & 1) * di) * dj0
			+	(((p10 >> (fsk16RGB5515SEAlphaPosition - 3)) & 1) * di0 + ((p11 >> (fsk16RGB5515SEAlphaPosition - 3)) & 1) * di) * dj
	) + (1 << 7)) >> 8) << (fsk16RGB5515SEAlphaPosition - 3);

	p00 <<= 3;			/* Make room for GGG at the bottom */
	p00 |= (p00 >> 16);	/* Weave B between g and R, and put G in MSB's */

	return((UInt16)p00);
}


/********************************************************************************
 * FskBilerp16RGB565SE32ARGB
 * 4 bit fractions
 * ----------------RRRRRGGGGGGBBBBB = input format
 * -----GGGGGG-----RRRRR------BBBBB = initial computation format
 * 00000111111000001111100000011111 = mask1 = 0x07E0F81F
 * -GGGGGGGGGG-RRRRRRRRR--BBBBBBBBB = first intermediate computation format
 * --------1---------1----------1-- = first rounding
 * 00000000100000000010000000000100 = first rounding  = 0x00802004
 * ----GGGGGGG----RRRRRR-----BBBBBB = second intermediate computation format
 * 00001111111000011111100000111111 = mask2 = 0x0FE1F83F
 * GGGGGGGGGGGRRRRRRRRRR-BBBBBBBBBB = third intermediate computation format
 * 00000000100000000001000000000010 = second rounding = 0x00801002
 * 098765432109876543210-9876543210 => 11 bit G, 10 bit R, 10 bit B
 * 33222222222211111111110000000000 => ...at 21, ...at 11, ...at 0
 * 10987654321098765432109876543210 => 8G at 24, 8R at 13, 8B at 2
 ********************************************************************************/

UInt32
FskBilerp16RGB565SE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 00000 GGGGGG 00000 RRRRR 000000 BBBBB */
	mask = 0x07E0F81F;
	p00 |= p00 << 16;	p00 &= mask;
	p01 |= p01 << 16;	p01 &= mask;
	p10 |= p10 << 16;	p10 &= mask;
	p11 |= p11 << 16;	p11 &= mask;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate */
 	mask = 0x0FE1F83F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask) * dj
		+ 0x00801002;
 	p00	= (fskDefaultAlpha      << fsk32ARGBAlphaPosition)
 		| (((p00 >> 24) & 0xFF) << fsk32ARGBGreenPosition)
 		| (((p00 >> 13) & 0xFF) << fsk32ARGBRedPosition)
 		| (((p00 >>  2) & 0xFF) << fsk32ARGBBluePosition);

	return(p00);
}


/********************************************************************************
 * FskBilerp16RGB565DE32ARGB
 * 4 bit fractions
 * ----------------gggbbbbbrrrrrGGG = input format
 * ------bbbbb-----GGGggg-----rrrrr = initial computation format
 * 00000011111000001111110000011111 = mask1 = 0x03E0FC1F
 * --bbbbbbbbb-gggggggggg-rrrrrrrrr = second computation format
 * --------1----------1---------1-- = first rounding
 * 00000000100000000001000000000100 = first rounding  = 0x00801004
 * -----bbbbbb----ggggggg----rrrrrr = third computation format
 * 00000111111000011111110000111111 = mask2 = 0x07E1FC3F
 * -bbbbbbbbbbgggggggggggrrrrrrrrrr = fourth computation format
 * 00000000010000000001000000000010 = second rounding = 0x00401002
 * -9876543210098765432109876543210 => 10 bit B, 11 bit G, 10 bit R
 * 33222222222211111111110000000000 => ...at 21, ...at 10, ...at 0
 * 10987654321098765432109876543210 => 8B at 23, 8G at 13, 8R at 2
 ********************************************************************************/

UInt32
FskBilerp16RGB565DE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 000000bbbbb00000GGGggg00000rrrrr */
	mask = 0x03E0FC1F;
	p00 |= p00 << 16;	p00 >>= 3;	p00 &= mask;
	p01 |= p01 << 16;	p01 >>= 3;	p01 &= mask;
	p10 |= p10 << 16;	p10 >>= 3;	p10 &= mask;
	p11 |= p11 << 16;	p11 >>= 3;	p11 &= mask;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate */
 	mask = 0x07E1FC3F;
	p00	= (((p00 * di0 + p01 * di) >> 3) & mask) * dj0
		+ (((p10 * di0 + p11 * di) >> 3) & mask) * dj
		+ 0x00401002;
 	p00	= (fskDefaultAlpha      << fsk32ARGBAlphaPosition)
 		| (((p00 >> 23) & 0xFF) << fsk32ARGBBluePosition)
 		| (((p00 >> 13) & 0xFF) << fsk32ARGBGreenPosition)
 		| (((p00 >>  2) & 0xFF) << fsk32ARGBRedPosition);

	return(p00);
}

UInt32
FskBilerp16RGB565SE32A16RGB565SE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p = (UInt32)FskBilerp16RGB565SE(di, dj, s, rb);
	p |= (fskDefaultAlpha << fsk32A16RGB565SEAlphaPosition);
	return p;
}

/********************************************************************************
 * FskBilerp16XXXX - 4 4-bit components
 * 4 bit fractions
 * ----------------rrrrggggbbbbaaaa = input format
 * ----gggg----aaaa----rrrr----bbbb = initial computation format
 * 00001111000011110000111100001111 = mask1 = 0x0F0F0F0F
 * ggggggggaaaaaaaarrrrrrrrbbbbbbbb = first interpolation
 * ----aaaaaaaaaaaa----bbbbbbbbbbbb = second interpolation, part 1
 * ----gggggggggggg----rrrrrrrrrrrr = second interpolation, part 2
 * 00000000100000000000000010000000 = 0x00800080, rounding
 * ----aaaa------------bbbb--------
 * ----gggg------------rrrr--------
 ********************************************************************************/

UInt16
FskBilerp16XXXX(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 0000gggg0000aaaa0000rrrr0000bbbb */
	mask = 0x0F0F0F0F;
	p00 = ((p00 << 16) | (p00 >> 4)) & mask;
	p01 = ((p01 << 16) | (p01 >> 4)) & mask;
	p10 = ((p10 << 16) | (p10 >> 4)) & mask;
	p11 = ((p11 << 16) | (p11 >> 4)) & mask;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate horizontally */
	p00 = p00 * di0 + p01 * di;	/* ggggggggaaaaaaaarrrrrrrrbbbbbbbb */
	p10 = p10 * di0 + p11 * di;

	/* Interpolate vertically and round */
	mask = 0x00FF00FF;
	p01 = ((p00 >> 0) & mask) * dj0 + ((p10 >> 0) & mask) * dj + 0x00800080;	/* 0000aaaaaaaaaaaa0000bbbbbbbbbbbb */
	p11 = ((p00 >> 8) & mask) * dj0 + ((p10 >> 8) & mask) * dj + 0x00800080;	/* 0000gggggggggggg0000rrrrrrrrrrrr */

	/* Assemble */
	p00 = FskMoveField(p11, 4,  8, 12)		/* R */
		| FskMoveField(p11, 4, 24,  8)		/* G */
		| FskMoveField(p01, 4,  8,  4)		/* B */
		| FskMoveField(p01, 4, 24,  0);		/* A */

	return((UInt16)p00);
}


/********************************************************************************
 * FskBilerp16XXXX32XXXX - 4 4-bit components --> 4 8-bit components
 * 4 bit fractions
 * ----------------rrrrggggbbbbaaaa = input format
 * ----gggg----aaaa----rrrr----bbbb = initial computation format
 * 00001111000011110000111100001111 = mask1 = 0x0F0F0F0F
 * ggggggggaaaaaaaarrrrrrrrbbbbbbbb = first interpolation
 * ----aaaaaaaaaaaa----bbbbbbbbbbbb = second interpolation, part 1
 * ----gggggggggggg----rrrrrrrrrrrr = second interpolation, part 2
 * aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb = extrapolation to 8 bits, part 1
 * ggggggggggggggggrrrrrrrrrrrrrrrr = extrapolation to 8 bits, part 2
 * 00000000100000000000000010000000 = 0x00800080, rounding
 * aaaaaaaa--------bbbbbbbb--------
 * gggggggg--------rrrrrrrr--------
 ********************************************************************************/

UInt32
FskBilerp16XXXX32XXXX(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0, mask;

	/* Get pixels */
	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	/* Format as 0000gggg0000aaaa0000rrrr0000bbbb */
	mask = 0x0F0F0F0F;
	p00 = ((p00 << 16) | (p00 >> 4)) & mask;
	p01 = ((p01 << 16) | (p01 >> 4)) & mask;
	p10 = ((p10 << 16) | (p10 >> 4)) & mask;
	p11 = ((p11 << 16) | (p11 >> 4)) & mask;

	di0 = (1 << 4) - di;
	dj0 = (1 << 4) - dj;

	/* Interpolate horizontally */
	p00 = p00 * di0 + p01 * di;	/* ggggggggaaaaaaaarrrrrrrrbbbbbbbb */
	p10 = p10 * di0 + p11 * di;

	/* Interpolate vertically */
	mask = 0x00FF00FF;
	p01 = ((p00 >> 0) & mask) * dj0 + ((p10 >> 0) & mask) * dj;	/* 0000aaaaaaaaaaaa0000bbbbbbbbbbbb */
	p11 = ((p00 >> 8) & mask) * dj0 + ((p10 >> 8) & mask) * dj;	/* 0000gggggggggggg0000rrrrrrrrrrrr */

	/* Extrapolate to 8 bit precision and round */
	p01 += (p01 << 4) + 0x00800080;								/* aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb */
	p11 += (p11 << 4) + 0x00800080;								/* ggggggggggggggggrrrrrrrrrrrrrrrr */

	/* Assemble */
	p00 = FskMoveField(p11, 8,  8, 24)		/* R */
		| FskMoveField(p11, 8, 24, 16)		/* G */
		| FskMoveField(p01, 8,  8,  8)		/* B */
		| FskMoveField(p01, 8, 24,  0);		/* A */

	return(p00);
}


/********************************************************************************
 * FskBilerp16XX
 * 4 bit fractions
 ********************************************************************************/

UInt16
FskBilerp16XX(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 p00, p01, p10, p11;
	UInt32 di0, dj0;

	p00 = *s;
	p01 = sizeof(UInt16);	if (di == 0)	p01 = 0;	/* This avoids segmentation faults */
	p10 = (UInt32)rb;		if (dj == 0)	p10 = 0;	/* This avoids segmentation faults */
	p11 = p01 + p10;
	p01 = *((const UInt16*)(((const char*)(s)) + (SInt32)p01));
	p10 = *((const UInt16*)(((const char*)(s)) + (SInt32)p10));
	p11 = *((const UInt16*)(((const char*)(s)) + (SInt32)p11));

	p00 = ((p00 & 0xFF00) << 8) | (p00 & 0xFF);		/* Separate components with a space */
	p01 = ((p01 & 0xFF00) << 8) | (p01 & 0xFF);
	p10 = ((p10 & 0xFF00) << 8) | (p10 & 0xFF);
	p11 = ((p11 & 0xFF00) << 8) | (p11 & 0xFF);		/* Separate components with a space */

	di0 = (1 << 4)  - di;
	dj0 = (1 << 4) - dj;

	p00 = ((((p00 * di0) + (p01 * di)) * dj0
		+   ((p10 * di0) + (p11 * di)) * dj
		+   0x00800080
		) >> 8);
	p00 = ((p00 >> 8) & 0xFF00) | (p00 & 0xFF);

	return (UInt16)p00;
}


/********************************************************************************
 * FskBilerp8
 * 4 bit fractions
 ********************************************************************************/

UInt8
FskBilerp8(UInt32 di, UInt32 dj, const UInt8 *s, SInt32 rb)
{
	SInt32 p00, p10;

	p00 = s[0];
	if (di != 0) {
		p00 = (p00 << 4) + (s[1] - p00) * di;
		if (dj != 0) {
			p10 = s[rb];
			p10 =  (p10 << 4) + (s[rb+1] - p10) * di;
			p00 = ((p00 << 4) + (p10 - p00) * dj + (1 << 7)) >> 8;
		}
		else {
			p00 = (p00 + (1 << 3)) >> 4;
		}
	}
	else if (dj != 0) {
		p00 = ((p00 << 4) + (s[rb] - p00) * dj + (1 << 3)) >> 4;
	}

	return((UInt8)p00);
}


/********************************************************************************
 * FskBilerpUV
 * 4 bit fractions
 ********************************************************************************/

UInt16
FskBilerpUV(UInt32 di, UInt32 dj, const UInt8 *u, const UInt8 *v, SInt32 rb)
{
	UInt32 di0	= (1 << 4) - di;
	UInt32 dj0	= (1 << 4) - dj;
	UInt32 p	= (u[0] << 16) | v[0];

	if (di != 0) {
		p = p * di0 + ((u[1] << 16) | v[1]) * di;
		if (dj != 0) {
			p = p * dj0 + (((u[rb] << 16) | v[rb]) * di0 + ((u[rb+1] << 16) | v[rb+1]) * di) * dj;
			p = (p + 0x00800080) >> 8;
		}
		else {
			p = (p + 0x00080008) >> 4;
		}
	}
	else if (dj != 0) {
		p = p * dj0 + ((u[rb] << 16) | v[rb]) * dj;
		p = (p + 0x00080008) >> 4;
	}
	p &= 0x00FF00FF;
	p |= p >> 8;

	return((UInt16)p);
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 **		Blending
 **
 **		Cross-fading controlled by the alpha parameter.
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskBlend32
 ********************************************************************************/

void
FskBlend32(UInt32 *d, UInt32 src, UInt8 alpha)
{
	const UInt32 msk = 0x00FF00FF;
	UInt32 dag, drb;
	dag = *d;																	/* For component combing, we assume ARGB without loss of generality */
	drb =  dag       & msk;														/* -R-B: destination */
	dag = (dag >> 8) & msk;														/* -A-G: destination */
	drb  = (( src       & msk) - drb) * alpha + (drb << 8) - drb + 0x00800080;	/* RrBb: This product is scaled by 255 */
	dag  = (((src >> 8) & msk) - dag) * alpha + (dag << 8) - dag + 0x00800080;	/* AaGg: This product is scaled by 255 */
	drb += (drb >> 8) & msk; drb >>= 8; drb &=  msk;							/* -R-B, after dividing by 255 */
	dag += (dag >> 8) & msk;            dag &= ~msk;							/* A-G-, after dividing by 255 */
	src = dag | drb;															/* ARGB: reassembled. */
	*d = src;
}

/********************************************************************************
 * FskBlend32A16RGB565SE
 * This uses a 6 bit alpha to interpolate all components.
 ********************************************************************************/

void
FskBlend32A16RGB565SE(UInt32 *d, UInt32 p, UInt8 alpha)
{
	SInt32	sAlpha, dstrb, dstg;

	dstrb  = *d;				/* AAAAAAAA--------RRRRRGGGGGGBBBBB */
	sAlpha = alpha >> 2;		/*                 ----------AAAAAA | 6-bit alpha */
	dstg   = 0x07E0 & dstrb;	/* ---------------------GGGGGG----- | 6 bit green */
	dstrb &= 0xF81F;			/* ----------------RRRRR------BBBBB | 5 bit red and blue */

	dstrb = ((p  & 0xF81F) - dstrb) * sAlpha + (dstrb << 6) - dstrb + 0x10020; dstrb += (dstrb >> 6) & 0xF81F; dstrb = (dstrb >> 6) & 0xF81F;	/* ----------------RRRRR------BBBBB */
	dstg  = ((p  & 0x07E0) - dstg ) * sAlpha + (dstg  << 6) - dstg  + 0x00400; dstg  += (dstg  >> 6) & 0x07E0; dstg  = (dstg  >> 6) & 0x07E0;	/* ---------------------GGGGGG----- */
	dstrb |= dstg;																																/* ----------------RRRRRGGGGGGBBBBB */
	dstg  = *d >> 24U;																															/* ------------------------AAAAAAAA */
	sAlpha *= ((SInt32)(p >> 24U) - dstg); sAlpha += (1 << (6-1)); sAlpha += sAlpha >> 6; sAlpha >>= 6; dstg += sAlpha;							/* ------------------------AAAAAAAA */
	dstrb |= dstg << 24;																														/* AAAAAAAA--------RRRRRGGGGGGBBBBB */
	*d = dstrb;
}

/********************************************************************************
 * FskBlend24
 ********************************************************************************/

void
FskBlend24(Fsk24BitType *d, Fsk24BitType p, UInt8 alpha)
{
	SInt32	c0, c1;
	SInt32	sAlpha		= alpha;

	c0 = ((UInt8*)d)[0];  c1 = ((UInt8*)(&p))[0];  ((UInt8*)d)[0] = (UInt8)AlphaLerp(c0, c1, sAlpha);
	c0 = ((UInt8*)d)[1];  c1 = ((UInt8*)(&p))[1];  ((UInt8*)d)[1] = (UInt8)AlphaLerp(c0, c1, sAlpha);
	c0 = ((UInt8*)d)[2];  c1 = ((UInt8*)(&p))[2];  ((UInt8*)d)[2] = (UInt8)AlphaLerp(c0, c1, sAlpha);
}


/********************************************************************************
 * FskBlend565SE
 ********************************************************************************/

void
FskBlend565SE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	sAlpha, dstrb, dstg;

	dstrb  = *d;				/* RRRRRGGGGGGBBBBB */
	sAlpha = alpha >> 2;		/* ----------AAAAAA | 6-bit alpha: base 63 */
	dstg   = 0x07E0 & dstrb;	/* -----GGGGGG----- | 6 bit green;        after alphamul:   -----GGGGGGgggggg----- */
	dstrb &= 0xF81F;			/* RRRRR------BBBBB | 5 bit red and blue; after alphamul:	RRRRRrrrrrrBBBBBbbbbbb */

	/*
	 * Note: because we subtract (src - dst), we can generate negative numbers. If this happens in the blue channel,
	 * the sign extension will interfere with the red channel, since we hold them in the same word.  Therefore,
	 * we need to add the destination back in to avoid channel crosstalk, so that all numbers are positive and non-interfering.
	 * Since alpha=1 is represented by 63, we need to multiply the dst by 63=64-1, before rounding and dividing.
	 *
	 *       extract_src  diff_dst    scale   add_dst_scaled_by_63   round    multiply_by_64/63       clean           divide_by_64   clean
	 */
	dstrb = ((p & 0xF81F) - dstrb) * sAlpha + (dstrb << 6) - dstrb + 0x10020; dstrb += (dstrb >> 6) & 0xF81F; dstrb = (dstrb >> 6) & 0xF81F;
	dstg  = ((p & 0x07E0) - dstg ) * sAlpha + (dstg  << 6) - dstg  + 0x00400; dstg  += (dstg  >> 6) & 0x07E0; dstg  = (dstg  >> 6) & 0x07E0;

	*d = (UInt16)(dstrb | dstg);
}


#if 1
/********************************************************************************
 * FskFastBlend565SE
 * This uses a 5 bit rather than a 6 bit alpha.
 ********************************************************************************/

void
FskFastBlend565SE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	/* 	33222222222211111111110000000000
	 *	10987654321098765432109876543210
	 *	----------------RRRRRGGGGGGBBBBB	15RGB565SE
	 *	-----GGGGGG-----RRRRR------BBBBB	split
	 *	00000111111000001111100000011111	mask 07E0F81F
	 *	+++++GGGGGG+++++RRRRR++++++BBBBB	difference, where + indicates sign
	 *	1----------1---------1----------	Mask for sign bits
	 *	10000000000100000000010000000000	Mask to grab sign bits 80100400 or 00100400
	 *	00000000001100000000110000000001	300C01
	 *	11111111111011111111101111111111	Mask to remove sign bits FFEFFBFF or ~00100400
	 *	++++++GGGGGG+++++RRRRR++++++BBBB	Difference halved
	 *	++++++GGGGG-+++++RRRR-++++++BBBB	Halved difference with garbage removed from the sign bits
	 *	++++++GGGGG++++++RRRR++++++BBBBB	Halved difference with sign bits restored
	 *	GGGGGGgggggRRRRRrrrrr+BBBBBbbbbb	Scaled by alpha
	 *	GGGGGG-----RRRRR------BBBBB-----	64 * bg
	 *	GGGGGGgggggRRRRRrrrrr+BBBBBbbbbb
	 *	------1----------1----------1---
	 *	00000010000000000100000000001000	02004008 rounding for 6 bits
	 *	00000010000000001000000000010000	02008010 rounding for 5 bits
	 *	GGGGGGgggggRRRRRrrrrr+BBBBBbbbbb
	 *	00000011111000001111100000011111	3E0F81F
	 *	------GGGGG------RRRR-------BBBB
	 *	00000011111000000111100000001111	3E0780F
	 *	-----GGGGGGgggggRRRRRrrrrr+BBBBB
	 *	00000111111000001111100000011111	7E0F81F
	 */

	SInt32	sAlpha, dst, src;

	sAlpha = alpha >> (8-5);									/* ---------------------------AAAAA | 5-bit alpha: base 31 */
	dst = *d;													/* ----------------RRRRRGGGGGGBBBBB 16RGB565SE dst */
	dst |= dst << 16;											/* RRRRRGGGGGGBBBBBRRRRRGGGGGGBBBBB replicated dst */
	dst &= 0x07E0F81F;											/* -----GGGGGG-----RRRRR------BBBBB interbits cleared */
	src = p;													/* ----------------RRRRRGGGGGGBBBBB 16RGB565SE src */
	src |= src << 16;											/* RRRRRGGGGGGBBBBBRRRRRGGGGGGBBBBB replicated src*/
	src &= 0x07E0F81F;											/* -----GGGGGG-----RRRRR------BBBBB interbits cleared */
	src -= dst;													/* +++++GGGGGG+++++RRRRR++++++BBBBB difference with sign */
	dst = sAlpha * src + (dst << 5) - dst;						/* GGGGGGgggggRRRRRrrrrr-BBBBBbbbbb 31 times bigger */
	if (0 && alpha & 0x4) {											/* ------------------------AAAAAa-- the 6th bit (a) of alpha is 1 */
		src = (src & 0x00100400) | ((src >> 1) & ~0x00100400);	/* ++++++GGGGG++++++RRRR+++++++BBBB	halved difference, preserving the sign */
		dst += src;												/* GGGGGGgggggRRRRRrrrrr-BBBBBbbbbb */
	}
	dst += 0x02008010;											/* GGGGGGgggggRRRRRrrrrr-BBBBBbbbbb round for 5 bits */
	dst += (dst >> 5) & 0x3E0F81F;								/* GGGGGGgggggRRRRRrrrrr-BBBBBbbbbb multiply by 64/63 */
	dst >>= 5;													/* +++++GGGGGGgggggRRRRRrrrrr-BBBBB divide by 32 (already divided by 2, yielding division by 64) */
	dst &= 0x7E0F81F;											/* -----GGGGGG-----RRRRR------BBBBB */
	dst |= dst >> 16;											/* -----GGGGGG-----RRRRRGGGGGGBBBBB */
	*d = (UInt16)dst;
}
#endif


/********************************************************************************
 * FskBlend565DE
 ********************************************************************************/

void
FskBlend565DE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	sAlpha		= alpha;
	SInt32	p0, p1, pc;

	p0 = *d;	p0 |= p0 << 16;
	p1 = p;		p1 |= p1 << 16;
	pc = 0;
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB565DERedPosition,   fsk16RGB565DERedBits,   8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB565DEGreenPosition, fsk16RGB565DEGreenBits, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB565DEBluePosition,  fsk16RGB565DEBlueBits,  8);
	pc &= 0x7FFF8;
	pc |= (UInt32)pc >> 16;
	*d = (UInt16)pc;
}


/********************************************************************************
 * FskBlend5515SE
 ********************************************************************************/

void
FskBlend5515SE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	sAlpha	= alpha;
	SInt32	p0		= *d;
	SInt32	p1		= p;
	SInt32	pc		= 0;
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515SERedPosition,   fsk16RGB5515SERedBits,   8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515SEGreenPosition, fsk16RGB5515SEGreenBits, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515SEBluePosition,  fsk16RGB5515SEBlueBits,  8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515SEAlphaPosition, fsk16RGB5515SEAlphaBits, 8);
	*d = (UInt16)pc;
}


/********************************************************************************
 * FskBlend5515DE
 ********************************************************************************/

void
FskBlend5515DE(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	sAlpha		= alpha;
	SInt32	p0, p1, pc;

	p0 = *d;	p0 |= p0 << 16;
	p1 = p;		p1 |= p1 << 16;
	pc = 0;
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515DERedPosition,   fsk16RGB5515DERedBits,   8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515DEGreenPosition, fsk16RGB5515DEGreenBits, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515DEBluePosition,  fsk16RGB5515DEBlueBits,  8);
	AlphaLerpCField(p0, p1, pc, sAlpha, fsk16RGB5515DEAlphaPosition, fsk16RGB5515DEAlphaBits, 8);
	pc |= (UInt32)pc >> 16;
	*d = (UInt16)pc;
}


/********************************************************************************
 * FskBlend4444
 ********************************************************************************/

void
FskBlend4444(UInt16 *d, UInt16 p, UInt8 alpha)
{

	SInt32	sAlpha	= alpha;
	SInt32	p0		= *d;
	SInt32	p1		= p;
	SInt32	pc		= 0;
	AlphaLerpCField(p0, p1, pc, sAlpha, 12, 4, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha,  8, 4, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha,  4, 4, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha,  0, 4, 8);
	*d = (UInt16)pc;
}


/********************************************************************************
 * FskBlend88
 ********************************************************************************/

void
FskBlend88(UInt16 *d, UInt16 p, UInt8 alpha)
{
	SInt32	sAlpha	= alpha;
	SInt32	p0		= *d;
	SInt32	p1		= p;
	SInt32	pc		= 0;
	AlphaLerpCField(p0, p1, pc, sAlpha, 0, 8, 8);
	AlphaLerpCField(p0, p1, pc, sAlpha, 8, 8, 8);
	*d = (UInt16)pc;
}


/********************************************************************************
 * FskBlend8
 ********************************************************************************/

void
FskBlend8(UInt8 *d, UInt8 p, UInt8 alpha)
{
	SInt32	sAlpha	= alpha;
	SInt32	c0		= *d;
	SInt32	c1		= p;
	*d = (UInt8)AlphaLerp(c0, c1, sAlpha);
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 **		Alpha and alpha blend
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskAlphaA32
 ********************************************************************************/

void
FskAlphaA32(UInt32 *d, UInt32 src)
{
	SInt32 alpha = src >> 24;
	if (alpha != 0) {	/* If the source is not transparent */
		if (alpha < 255) {
			const UInt32 msk = 0x00FF00FF;
			UInt32 dag, drb;
			dag = *d;																	/* For component combing, we assume ARGB without loss of generality */
			drb =  dag       & msk;														/* -R-B: destination */
			dag = (dag >> 8) & msk;														/* -A-G: destination */
			src |= 255 << 24;															/* Avoid scaling src alpha */
			drb  = (( src       & msk) - drb) * alpha + (drb << 8) - drb + 0x00800080;	/* RrBb: This product is scaled by 255 */
			dag  = (((src >> 8) & msk) - dag) * alpha + (dag << 8) - dag + 0x00800080;	/* AaGg: This product is scaled by 255 */
			drb += (drb >> 8) & msk; drb >>= 8; drb &=  msk;							/* -R-B, after dividing by 255 */
			dag += (dag >> 8) & msk;            dag &= ~msk;							/* A-G-, after dividing by 255 */
			src = dag | drb;															/* ARGB: reassembled. */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAlpha32A
 ********************************************************************************/

void
FskAlpha32A(UInt32 *d, UInt32 src)
{
	SInt32 alpha = src & 0xFF;

	if (alpha != 0) {
		if (alpha < 255) {
			const UInt32 msk = 0x00FF00FF;
			UInt32 dag, drb;
			dag = *d;																	/* For component combing, we assume ARGB without loss of generality */
			drb =  dag       & msk;														/* -R-B: destination */
			dag = (dag >> 8) & msk;														/* -A-G: destination */
			src |= 255 << 0;															/* Avoid scaling src alpha */
			drb  = (( src       & msk) - drb) * alpha + (drb << 8) - drb + 0x00800080;	/* RrBb: This product is scaled by 255 */
			dag  = (((src >> 8) & msk) - dag) * alpha + (dag << 8) - dag + 0x00800080;	/* AaGg: This product is scaled by 255 */
			drb += (drb >> 8) & msk; drb >>= 8; drb &=  msk;							/* -R-B, after dividing by 255 */
			dag += (dag >> 8) & msk;            dag &= ~msk;							/* A-G-, after dividing by 255 */
			src = dag | drb;															/* ARGB: reassembled. */
		}
		*d = src;
	}
}

/********************************************************************************
 * FskAlpha32A16RGB565SE
 ********************************************************************************/

void
FskAlpha32A16RGB565SE(UInt32 *d, UInt32 p)
{
	SInt32 alpha = p >> 24;

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32 pc;
			{	UInt16 d16 = (UInt16)(*d);
				FskBlend565SE(&d16, (UInt16)p, (UInt8)alpha);	/* Interpolate the last significant 16 bits, using a 6 bit alpha */
				pc = (SInt32)d16;								/* Place the result in the LS 16 bits & zero the MS 16 bits */
			}
			{	SInt32 p0 = *d;
				alpha &= ~3; alpha |= alpha >> 6;				/* Truncate to 6 bit alpha */
				AlphaLerpAField(p0, pc, alpha, 24, 8, 8);		/* Interpolate the most significant 16 bits (i.e. alpha) */
			}
			*d = pc;
		}
	}
}

/********************************************************************************
 * FskAlpha16XXXA
 ********************************************************************************/

void
FskAlpha16XXXA(UInt16 *d, UInt16 p)
{
	SInt32 alpha;

	alpha = p & 0xF;		/* Get 4 bit alpha */
	alpha |= alpha << 4;	/* Convert to 8 bit */

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32 p0 = *d;
			SInt32 p1 = p;
			SInt32 pc = 0;
			AlphaLerpCField(p0, p1, pc, alpha, 12, 4, 8);
			AlphaLerpCField(p0, p1, pc, alpha,  8, 4, 8);
			AlphaLerpCField(p0, p1, pc, alpha,  4, 4, 8);
			AlphaLerpAField(p0,     pc, alpha,  0, 4, 8);

			*d = (UInt16)pc;
		}
	}
}


/********************************************************************************
 * FskAlpha16XAXX
 ********************************************************************************/

void
FskAlpha16XAXX(UInt16 *d, UInt16 p)
{
	SInt32 alpha;

	alpha = (p >> 8) & 0xF;		/* Get 4 bit alpha */
	alpha |= alpha << 4;		/* Convert to 8 bit */

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32 p0 = *d;
			SInt32 p1 = p;
			SInt32 pc = 0;
			AlphaLerpCField(p0, p1, pc, alpha, 12, 4, 8);
			AlphaLerpAField(p0,     pc, alpha,  8, 4, 8);
			AlphaLerpCField(p0, p1, pc, alpha,  4, 4, 8);
			AlphaLerpCField(p0, p1, pc, alpha,  0, 4, 8);
			*d = (UInt16)pc;
		}
	}
}


/********************************************************************************
 * FskAlpha16AG
 ********************************************************************************/

void
FskAlpha16AG(UInt16 *d, UInt16 p)
{
	SInt32 alpha = p >> 8;

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32	p0 = *d;
			SInt32	p1 = p;
			SInt32	pc = 0;
			AlphaLerpCField(p0, p1, pc, alpha, 0, 8, 8);
			AlphaLerpAField(p0,     pc, alpha, 8, 8, 8);
			*d = (UInt16)pc;
		}
	}
}


/********************************************************************************
 * FskAlpha16GA
 ********************************************************************************/

void
FskAlpha16GA(UInt16 *d, UInt16 p)
{
	SInt32 alpha = p & 0xFF;

	if (alpha != 0) {
		if (alpha == 255) {
			*d = p;
		}
		else {
			SInt32	p0 = *d;
			SInt32	p1 = p;
			SInt32	pc = 0;
			AlphaLerpCField(p0, p1, pc, alpha, 8, 8, 8);
			AlphaLerpAField(p0,     pc, alpha, 0, 8, 8);
			*d = (UInt16)pc;
		}
	}
}


/********************************************************************************
 * FskAlphaStraightOver32AXXX
 ********************************************************************************/

UInt32
FskAlphaStraightOver32AXXX(UInt32 dst, UInt32 src)
{
	const UInt32 mask = 0x00ff00ff;
	UInt32 sa, da, na;
	if ((sa = (src >> 24) & 0xFF) == 0) {												/* If the source is transparent ... */
		;																				/* ... the destination remains unchanged */
	} else if  (((na = 255 - sa) == 0)		||											/* If the source is opaque, ...	*/
				((da = (dst >> 24) & 0xFF) == 0)										/* ... or the destination is transparent, ... */
	) {
		dst = src;																		/* ... the source replaces the destination. */
	} else if (da == 255) {																/* If the destination is opaque, we can avoid division, just using two multiplications. */
		src |= 255 << 24;																/* Avoid multiplying alpha by alpha */
		na  =  dst       & mask;														/* -R-B: destination */
		da  = (dst >> 8) & mask;														/* -A-G: destination */
		na  = (((src >> 0) & mask) - na) * sa + (na << 8) - na + 0x00800080;			/* RrBb: This product is scaled by 255 */
		da  = (((src >> 8) & mask) - da) * sa + (da << 8) - da + 0x00800080;			/* AaGg: This product is scaled by 255 */
		na += (na >> 8) & mask;															/* RrBb: Scale by 256/255, yielding an overall svale of 256 */
		da += (da >> 8) & mask;															/* AaGg: Scale by 256/255, yielding an overall svale of 256 */
		na  = (na >> 8) & mask;															/* -R-B: Scale by 1/256, yielding an overall svale of 1 */
		da  = (da >> 8) & mask;															/* -A-G: Scale by 1/256, yielding an overall svale of 1 */
		dst = (da << 8) | na;															/* ARGB: reassembled. */
		/* Destination alpha is already 255 */
	} else {																			/* The general case requires one division and four multiplications */
		da *= na; da += 0x80; da += da >> 8; da >>= 8;									/* Scale destination_alpha by (1 - source_alpha) */
		na = sa + da;																	/* Result_alpha = source_alpha + (1 - source_alpha) * destination_alpha */
		sa = ((sa << 8) + (na >> 1)) / na;												/* Source      fraction, where one is represented by 256. */
		da = 256 - sa;																	/* Destination fraction, where one is represented by 256. */
		dst = (((src &       mask) * sa + (dst &       mask) * da + 0x00800080) & ~mask)		/* R0B0 +		*/
			| (((src & 0x0000FF00) * sa + (dst & 0x0000FF00) * da + 0x00008000) & 0x00FF0000);	/* 0G00 -> RGB0	*/
		dst >>= 8;																				/* 0RGB 		*/
		dst |= na << 24;																		/* ARGB			*/
	}

	return dst;
}


/********************************************************************************
 * FskAlphaStraightOver32XXXA
 ********************************************************************************/

UInt32
FskAlphaStraightOver32XXXA(UInt32 dst, UInt32 src)
{
	const UInt32 mask = 0x00ff00ff;
	UInt32 sa, da, na;
	if ((sa = (src >> 0) & 0xFF) == 0) {												/* If the source is transparent ... */
		;																				/* ... the destination remains unchanged */
	} else if  (((na = 255 - sa) == 0)		||											/* If the source is opaque, ...	*/
				((da = (dst >> 0) & 0xFF) == 0)											/* ... or the destination is transparent, ... */
	) {
		dst = src;																		/* ... the source replaces the destination. */
	} else if (da == 255) {																/* If the destination is opaque, we can avoid division, just using two multiplications. */
		src |= 255 << 0;																/* Avoid multiplying alpha by alpha */
		na  =  dst       & mask;														/* -R-B: destination */
		da  = (dst >> 8) & mask;														/* -A-G: destination */
		na  = (((src >> 0) & mask) - na) * sa + (na << 8) - na + 0x00800080;			/* RrBb: This product is scaled by 255 */
		da  = (((src >> 8) & mask) - da) * sa + (da << 8) - da + 0x00800080;			/* AaGg: This product is scaled by 255 */
		na += (na >> 8) & mask;															/* RrBb: Scale by 256/255, yielding an overall svale of 256 */
		da += (da >> 8) & mask;															/* AaGg: Scale by 256/255, yielding an overall svale of 256 */
		na  = (na >> 8) & mask;															/* -R-B: Scale by 1/256, yielding an overall svale of 1 */
		da  = (da >> 8) & mask;															/* -A-G: Scale by 1/256, yielding an overall svale of 1 */
		dst = (da << 8) | na;															/* ARGB: reassembled. */
		/* Destination alpha is already 255 */
	} else {																			/* The general case requires one division and four multiplications */
		da *= na; da += 0x80; da += da >> 8; da >>= 8;									/* Scale destination_alpha by (1 - source_alpha) */
		na = sa + da;																	/* Result_alpha = source_alpha + (1 - source_alpha) * destination_alpha */
		sa = ((sa << 8) + (na >> 1)) / na;												/* Source      fraction, where one is represented by 256. */
		da = 256 - sa;																	/* Destination fraction, where one is represented by 256. */
		dst >>= 8;	src >>= 8;																	/* 0RGB */
		dst = (((src &       mask) * sa + (dst &       mask) * da + 0x00800080) & ~mask)		/* R0B0 +		*/
			| (((src & 0x0000FF00) * sa + (dst & 0x0000FF00) * da + 0x00008000) & 0x00FF0000);	/* 0G00 -> RGB0	*/
		dst |= na << 0;																			/* RGBA			*/
	}

	return dst;
}


/********************************************************************************
 * FskAlphaStraightOver32A16RGB565SE
 *
 * 33222222222211111111110000000000
 * 10987654321098765432109876543210
 * aaaaaaaa00000000rrrrrggggggbbbbb input
 * 00000000gggggg0000000000000bbbbb gb split: move(6,5,18) + move(5, 0, 0)
 * GGGGGGGGgggggg00000BBBBBBBBbbbbb after lerp
 * 00000010000000000000000010000000	round (0x02000080)
 * 11111100000000000001111100000000 mask  (0xFC001F00) or move(6, 26, 5) + move(5, 8, 0)
 * GGGGGG0000000000000BBBBB00000000 after mask
 * 00000000000000001111100000000000 red mask (0x0000F800)
 * 0000000000000000rrrrr00000000000 input
 * 00000000RRRRRRRRrrrrr00000000000 after lerp
 * 00000000000001000000000000000000 round (0x00040000)
 * 00000000111110000000000000000000 red mask (0x00F80000)
 * 00000000RRRRR0000000000000000000 after mask
 * GGGGGG00RRRRR000000BBBBB00000000 after merge
 * 00000000GGGGGG00RRRRR000000BBBBB after shift
 * 0000000000000000RRRRRGGGGGGBBBBB after reassembly
 * AAAAAAAA00000000RRRRRGGGGGGBBBBB after alpha insertion
 * N.B. dc is handy for converting binary to hexadecimal: 16o 2i 0110...p
 ********************************************************************************/

UInt32
FskAlphaStraightOver32A16RGB565SE(UInt32 dst, UInt32 src)
{
	UInt32 sa, na, da;

	if ((sa = (src >> fsk32A16RGB565SEAlphaPosition) & 0xFF) == 0) {		/* If the source is transparent ... */
		;																	/* ... the destination remains unchanged */
	} else if  (((na = 255 - sa) == 0)		||								/* If the source is opaque, ... */
				((da = (dst >> fsk32A16RGB565SEAlphaPosition) & 0xFF) == 0)	/* ... or the destination is transparent, ... */
	) {
		dst = src;															/* ... the source replaces the destination */
	} else if (da == 255) {						/* If the destination is opaque, we can avoid division, just using three multiplications. */
		AlphaLerpCField(dst, src, dst, sa, fsk32A16RGB565SERedPosition,   fsk32A16RGB565SERedBits,   8);
		AlphaLerpCField(dst, src, dst, sa, fsk32A16RGB565SEGreenPosition, fsk32A16RGB565SEGreenBits, 8);
		AlphaLerpCField(dst, src, dst, sa, fsk32A16RGB565SEBluePosition,  fsk32A16RGB565SEBlueBits,  8);
		/* destination alpha is already 255 */
	} else {									/* The general case requires one division and four multiplications */
		AlphaScale(da, na);						/* Scale destination_alpha by (1 - source_alpha) */
		na = sa + da;							/* Result_alpha = source_alpha + (1 - source_alpha) * destination_alpha */
		sa = ((sa << 8) + (na >> 1)) / na;		/* Source      fraction, where one is represented by 256. */
		da = 256 - sa;							/* Destination fraction, where one is represented by 256. */

		dst = (((FskMoveField(src, 6, 5, 18) | FskMoveField(src, 5, 0, 0)) * sa +
				(FskMoveField(dst, 6, 5, 18) | FskMoveField(dst, 5, 0, 0)) * da  + 0x02000080) & 0xFC001F00)	/* GGGGGG0000000000000BBBBB00000000 */
			+ (((src & 0x0000F800) * sa    +         (dst & 0x0000F800) * da  + 0x00040000) & 0x00F80000);		/* 00000000RRRRR0000000000000000000 */
		dst >>= 8;																								/* 00000000GGGGGG00RRRRR000000BBBBB */
		dst = FskMoveField(dst, 6, 18, 5) | (dst & 0xFFFF);														/* 0000000000000000RRRRRGGGGGGBBBBB */
		dst |= na << 24;																						/* AAAAAAAA00000000RRRRRGGGGGGBBBBB */
	}

	return dst;
}


/*******************************************************************************
 * FskPremultiplyColorRGBA
 *******************************************************************************/

void
FskPremultiplyColorRGBA(FskConstColorRGBA fr, FskColorRGBA to) {
	*to = *fr;
	if (to->a == 255)
		return;
	to->r = FskAlphaMul(to->r, to->a);
	to->g = FskAlphaMul(to->g, to->a);
	to->b = FskAlphaMul(to->b, to->a);
}


/********************************************************************************
 * FskAlphaBlackA32
 ********************************************************************************/

void
FskAlphaBlackA32(UInt32 *d, UInt32 src)
{
	UInt8 alpha;

	if ((alpha = src >> 24) != 0) {				/* If alpha is nonzero, we add the source */
		if ((alpha = 255 - alpha) != 0) {		/* If alpha less than one, add a fraction of the destination */
#define NO_CHECKS
#ifdef NO_CHECKS
			const UInt32 msk = 0x00FF00FF;
			UInt32 drb, dag;
			dag = *d;																									/* ARGB */
			drb = dag; drb &= msk; drb *= alpha; drb += 0x00800080; drb += (drb >> 8) & msk; drb >>= 8; drb &=  msk;	/* Scale dst R & B by src alpha */
			dag >>= 8; dag &= msk; dag *= alpha; dag += 0x00800080; dag += (dag >> 8) & msk;            dag &= ~msk;	/* Scale dst A & G src alpha */
			src += (drb |= dag);	/* Add scaled dst to src */
#else /* NO_CHECKS */
			UInt32 dst, pix;
			dst = *d;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24;
#endif /* NO_CHECKS */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAlphaBlack32A
 ********************************************************************************/

void
FskAlphaBlack32A(UInt32 *d, UInt32 src)
{
	UInt8 alpha;

	if ((alpha = src & 0xFF) != 0) {		/* If alpha is nonzero, we add the source */
		if ((alpha = 255 - alpha) != 0) {	/* If alpha less than one, add a fraction of the destination */
#ifdef NO_CHECKS
			const UInt32 msk = 0x00FF00FF;
			UInt32 drb, dag;
			dag = *d;																									/* ARGB */
			drb = dag; drb &= msk; drb *= alpha; drb += 0x00800080; drb += (drb >> 8) & msk; drb >>= 8; drb &=  msk;	/* Scale dst R & B by src alpha */
			dag >>= 8; dag &= msk; dag *= alpha; dag += 0x00800080; dag += (dag >> 8) & msk;            dag &= ~msk;	/* Scale dst A & G src alpha */
			src += (drb |= dag);	/* Add scaled dst to src */
#else /* NO_CHECKS */
			UInt32 dst, pix;
			dst = *d;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24; dst >>= 8;
			pix = dst & 0xFF; pix *= alpha; pix += 128; pix += pix >> 8; pix >>= 8; pix += src & 0xFF; if (pix > 255) pix = 255; src >>= 8; src |= pix << 24;
#endif /* NO_CHECKS */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAlphaBlack16AG
 ********************************************************************************/

void
FskAlphaBlack16AG(UInt16 *d, UInt16 src)
{
	UInt8 alpha;

	if ((alpha = src >> 8) != 0) {			/* If alpha is nonzero, we add the source */
		if ((alpha = 255 - alpha) != 0) {	/* If alpha less than one, add a fraction of the destination */
			UInt32 dst = *d;	dst |= dst << 8;	dst &= 0x00FF00FF;			/* --AG --> -AXG --> -A-G */
			dst *= alpha;	dst += 0x00800080;	dst += (dst >> 8) & 0x00FF00FF;	/* AaGg */
			dst = ((dst >> 16) & 0xFF00) | ((dst >> 8) & 0xFF);					/* --AG */
			src += (UInt16)dst;													/* Add scaled dst to src */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAlphaBlack16GA
 ********************************************************************************/

void
FskAlphaBlack16GA(UInt16 *d, UInt16 src)
{
	UInt8 alpha;

	if ((alpha = src & 0xFF) != 0) {		/* If alpha is nonzero, we add the source */
		if ((alpha = 255 - alpha) != 0) {	/* If alpha less than one, add a fraction of the destination */
			UInt32 dst = *d;	dst |= dst << 8;	dst &= 0x00FF00FF;			/* --GA --> -GXA --> -G-A */
			dst *= alpha;	dst += 0x00800080;	dst += (dst >> 8) & 0x00FF00FF;	/* GgAa */
			dst = ((dst >> 16) & 0xFF00) | ((dst >> 8) & 0xFF);					/* --GA */
			src += (UInt16)dst;													/* Add scaled dst to src */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAlphaBlack32A16RGB565SE
 * 		32A16RGB565SE --> 32A16RGB565SE
 ********************************************************************************/

void FskAlphaBlack32A16RGB565SE(UInt32 *d, UInt32 src)
{
	UInt8 alpha;

	alpha = src >> (24 + 2);				/* Six bit alpha */
	if (alpha != 0) {						/* If alpha is nonzero, we add the source */
		if ((alpha = 63 - alpha) != 0) {	/* If alpha less than one, add a fraction of the destination */
			UInt32 t, dst;
			dst = *d;
#ifdef NO_CHECKS
			t = FskMoveField(dst, fsk32A16RGB565SERedBits,     fsk32A16RGB565SERedPosition,     0);              AlphaNScale(t, alpha, 6); src += t << fsk32A16RGB565SERedPosition;
			t = FskMoveField(dst, fsk32A16RGB565SEGreenBits,   fsk32A16RGB565SEGreenPosition,   0);              AlphaNScale(t, alpha, 6); src += t << fsk32A16RGB565SEGreenPosition;
			t = FskMoveField(dst, fsk32A16RGB565SEBlueBits,    fsk32A16RGB565SEBluePosition,    0);              AlphaNScale(t, alpha, 6); src += t << fsk32A16RGB565SEBluePosition;
			t = FskMoveField(dst, fsk32A16RGB565SEAlphaBits-2, fsk32A16RGB565SEAlphaPosition+2, 2); t |= t >> 6; AlphaNScale(t, alpha, 6); src += t << fsk32A16RGB565SEAlphaPosition;
#else /* NO_CHECKS */
			t = FskMoveField(dst, fsk32A16RGB565SERedBits,      fsk32A16RGB565SERedPosition,   0);				/* Get red dst */
			AlphaNScale(t, alpha, 6);																			/* Scale dst by alpha */
			t += FskMoveField(src, fsk32A16RGB565SERedBits,     fsk32A16RGB565SERedPosition, 0);				/* Add red src */
			if (t > fskFieldMask(fsk32A16RGB565SERedBits, 0))   t = fskFieldMask(fsk32A16RGB565SERedBits, 0);	/* Saturate */
			src &= ~fskFieldMask(fsk32A16RGB565SERedBits,       fsk32A16RGB565SERedPosition);					/* Clear red position */
			src |= t << fsk32A16RGB565SERedPosition;															/* Insert new value of red */

			t = FskMoveField(dst, fsk32A16RGB565SEGreenBits,    fsk32A16RGB565SEGreenPosition,   0);			/* Get green dst */
			AlphaNScale(t, alpha, 6);																			/* Scale dst by alpha */
			t += FskMoveField(src, fsk32A16RGB565SEGreenBits,   fsk32A16RGB565SEGreenPosition, 0);				/* Add green src */
			if (t > fskFieldMask(fsk32A16RGB565SEGreenBits,0))  t = fskFieldMask(fsk32A16RGB565SEGreenBits, 0);	/* Saturate */
			src &= ~fskFieldMask(fsk32A16RGB565SEGreenBits,     fsk32A16RGB565SEGreenPosition);					/* Clear green position */
			src |= t << fsk32A16RGB565SEGreenPosition;															/* Insert new value of green */

			t = FskMoveField(dst, fsk32A16RGB565SEBlueBits,     fsk32A16RGB565SEBluePosition,   0);				/* Get blue dst */
			AlphaNScale(t, alpha, 6);																			/* Scale dst by alpha */
			t += FskMoveField(src, fsk32A16RGB565SEBlueBits,    fsk32A16RGB565SEBluePosition, 0);				/* Add blue src */
			if (t > fskFieldMask(fsk32A16RGB565SEBlueBits, 0))  t = fskFieldMask(fsk32A16RGB565SEBlueBits, 0);	/* Saturate */
			src &= ~fskFieldMask(fsk32A16RGB565SEBlueBits,      fsk32A16RGB565SEBluePosition);					/* Clear blue position */
			src |= t << fsk32A16RGB565SEBluePosition;															/* Insert new value of blue */

			t = FskMoveField(dst, fsk32A16RGB565SEAlphaBits-2,  fsk32A16RGB565SEAlphaPosition+2,   2);			/* Get alpha dst, MS 6 bits */
			t |= t >> 6;																						/* Extend dst alpha 6 to 8 bits */
			AlphaNScale(t, alpha, 6);																			/* Scale dst by alpha */
			t += FskMoveField(src, fsk32A16RGB565SEAlphaBits-2, fsk32A16RGB565SEAlphaPosition+2, 2);			/* Add alpha src, MS 6 bits */
			t += FskMoveField(src, fsk32A16RGB565SEAlphaBits-6, fsk32A16RGB565SEAlphaPosition+6, 0);			/* Add alpha src, LS 2 bits */
			if (t > fskFieldMask(fsk32A16RGB565SEAlphaBits, 0))	t = fskFieldMask(fsk32A16RGB565SEAlphaBits, 0);	/* Saturate */
			src &= ~fskFieldMask(fsk32A16RGB565SEAlphaBits,     fsk32A16RGB565SEAlphaPosition);					/* Clear alpha position */
			src |= t << fsk32A16RGB565SEAlphaPosition;															/* Insert new value of alpha */
#endif /* NO_CHECKS */
		}
		*d = src;
	}
}


/********************************************************************************
 * FskAXPY16RGB565SE
 *	return alpha6 * x + y
 ********************************************************************************/

UInt16 FskAXPY16RGB565SE(UInt8 alpha6, UInt16 x, UInt16 y)
{
#ifdef NO_CHECKS
	UInt32 x20, x1;
	x1  = x;																						/* RRRRRGGGGGGBBBBB */
	x20 = 0xF81F & x1;																				/* RRRRR------BBBBB */
	x1 &= 0x07E0;																					/* -----GGGGGG----- */
	x20 *= alpha6; x20 += 0x10020; x20 += (x20 >> 6) & 0xF81F; x20 >>= 6; x20 &= 0xF81F;			/* RRRRR------BBBBB */
	x1  *= alpha6; x1  += 0x00400; x1  += (x1  >> 6) & 0x07E0; x1  >>= 6; x1  &= 0x07E0;			/* -----GGGGGG----- */
	y += (UInt16)(x20 |= x1);																		/* RRRRRGGGGGGBBBBB */
#else /* NO_CHECKS */
	UInt32 t;
	t = FskMoveField(x, fsk16RGB565SERedBits,		fsk16RGB565SERedPosition,   0);					/* Get red x */
	AlphaNScale(t, alpha6, 6);																		/* Scale x by alpha */
	t += FskMoveField(y, fsk16RGB565SERedBits,		fsk16RGB565SERedPosition,   0);					/* Add red y */
	if (t > fskFieldMask(fsk16RGB565SERedBits, 0))	t = fskFieldMask(fsk16RGB565SERedBits,   0);	/* Saturate */
	y &= ~fskFieldMask(fsk16RGB565SERedBits,		fsk16RGB565SERedPosition);						/* Clear red position */
	y |= t << fsk16RGB565SERedPosition;																/* Insert new value of red */

	t = FskMoveField(x, fsk16RGB565SEGreenBits,		fsk16RGB565SEGreenPosition, 0);					/* Get green x */
	AlphaNScale(t, alpha6, 6);																		/* Scale x by alpha */
	t += FskMoveField(y, fsk16RGB565SEGreenBits,	fsk16RGB565SEGreenPosition, 0);					/* Add green y */
	if (t > fskFieldMask(fsk16RGB565SEGreenBits,0))	t = fskFieldMask(fsk16RGB565SEGreenBits, 0);	/* Saturate */
	y &= ~fskFieldMask(fsk16RGB565SEGreenBits,		fsk16RGB565SEGreenPosition);					/* Clear green position */
	y |= t << fsk16RGB565SEGreenPosition;															/* Insert new value of green */

	t = FskMoveField(x, fsk16RGB565SEBlueBits,		fsk16RGB565SEBluePosition,  0);					/* Get blue x */
	AlphaNScale(t, alpha6, 6);																		/* Scale x by alpha */
	t += FskMoveField(y, fsk16RGB565SEBlueBits,		fsk16RGB565SEBluePosition,  0);					/* Add blue y */
	if (t > fskFieldMask(fsk16RGB565SEBlueBits, 0))	t = fskFieldMask(fsk16RGB565SEBlueBits,  0);	/* Saturate */
	y &= ~fskFieldMask(fsk16RGB565SEBlueBits,		fsk16RGB565SEBluePosition);						/* Clear blue position */
	y |= t << fsk16RGB565SEBluePosition;															/* Insert new value of blue */
#endif /* NO_CHECKS */
	return y;
}


/********************************************************************************
 * FskAXPY16RGB565DE
 *	return alpha6 * x + y
 ********************************************************************************/

UInt16 FskAXPY16RGB565DE(UInt8 alpha6, UInt16 x, UInt16 y)
{
	x = FskAXPY16RGB565SE(alpha6, (x << 8) | (x >> 8), (y << 8) | (y >> 8));					/* Compute a * x + y in the same-endian format */
	return (x << 8) | (x >> 8);																	/* Return the different-endian result */
}


/********************************************************************************
 * FskAXPY8888
 *	return alpha * x + y
 ********************************************************************************/

UInt32 FskAXPY8888(UInt8 alpha, UInt32 x, UInt32 y)
{
#ifdef NO_CHECKS
	const UInt32 msk = 0x00FF00FF;
	UInt32 z;
	z   = x; z &= msk; z *= alpha; z += 0x00800080; z += (z >> 8) & msk; z >>= 8; z &=  msk;	/* 00000000GGGGGGGG00000000AAAAAAAA */
	x >>= 8; x &= msk; x *= alpha; x += 0x00800080; x += (x >> 8) & msk;          x &= ~msk;	/* BBBBBBBB00000000RRRRRRRR00000000 */
	y += (x |= z);																				/* BBBBBBBBGGGGGGGGRRRRRRRRAAAAAAAA */
#else /* NO_CHECKS */
	UInt32 t;
	t = x & 0xFF; AlphaNScale(t, alpha, 8); t += y & 0xFF; if (t > 255) t = 255; y >>= 8; y |= t << 24; x >>= 8;
	t = x & 0xFF; AlphaNScale(t, alpha, 8); t += y & 0xFF; if (t > 255) t = 255; y >>= 8; y |= t << 24; x >>= 8;
	t = x & 0xFF; AlphaNScale(t, alpha, 8); t += y & 0xFF; if (t > 255) t = 255; y >>= 8; y |= t << 24; x >>= 8;
	t = x & 0xFF; AlphaNScale(t, alpha, 8); t += y & 0xFF; if (t > 255) t = 255; y >>= 8; y |= t << 24;
#endif /* NO_CHECKS */
	return y;
}


/********************************************************************************
 * FskAXPY24
 *	return alpha * x + y
 ********************************************************************************/

Fsk24BitType FskAXPY24(UInt8 alpha, Fsk24BitType x, Fsk24BitType y)
{
	UInt32 t;
#ifdef NO_CHECKS
	t = x.c[0]; AlphaScale(t, alpha); y.c[0] += (UInt8)t;
	t = x.c[1]; AlphaScale(t, alpha); y.c[1] += (UInt8)t;
	t = x.c[2]; AlphaScale(t, alpha); y.c[2] += (UInt8)t;
#else /* NO_CHECKS */
	t = x.c[0]; AlphaScale(t, alpha); t += y.c[0]; if (t > 255) t = 255; y.c[0] = t;
	t = x.c[1]; AlphaScale(t, alpha); t += y.c[1]; if (t > 255) t = 255; y.c[1] = t;
	t = x.c[2]; AlphaScale(t, alpha); t += y.c[2]; if (t > 255) t = 255; y.c[2] = t;
#endif /* NO_CHECKS */
	return y;
}


/********************************************************************************
 * FskAlphaBlendA32
 ********************************************************************************/

void
FskAlphaBlendA32(UInt32 *d, UInt32 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p >> 24;
	b = opacity;
	AlphaScale(a, b);	/* Scale alpha by opacity */
	p &= 0x00FFFFFF;	/* Clear old alpha */
	p |= a << 24;		/* Insert scaled alpha */
	FskAlphaA32(d, p);
}


/********************************************************************************
 * FskAlphaBlend32A
 ********************************************************************************/

void
FskAlphaBlend32A(UInt32 *d, UInt32 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p & 0xFF;
	b = opacity;
	AlphaScale(a, b);	/* Scale alpha by opacity */
	p &= 0xFFFFFF00;	/* Clear old alpha */
	p |= a;				/* Insert scaled alpha */
	FskAlpha32A(d, p);
}


/********************************************************************************
 * FskAlphaBlend32A16RGB565SE
 ********************************************************************************/

void
FskAlphaBlend32A16RGB565SE(UInt32 *d, UInt32 src, UInt8 opacity)
{
	UInt32	dst = *d, res;
	SInt32	c0, c1;

	c0 = FskMoveField(dst, fsk32A16RGB565SERedBits, fsk32A16RGB565SERedPosition, 0);
	c1 = FskMoveField(src, fsk32A16RGB565SERedBits, fsk32A16RGB565SERedPosition, 0);
	c1 -= c0; c1 *= (SInt32)opacity; c1 += 128; c1 += c1 >> 8; c1 >>=8; c1 += c0;
	res = c1 << fsk32A16RGB565SERedPosition;

	c0 = FskMoveField(dst, fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition, 0);
	c1 = FskMoveField(src, fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition, 0);
	c1 -= c0; c1 *= (SInt32)opacity; c1 += 128; c1 += c1 >> 8; c1 >>=8; c1 += c0;
	res |= c1 << fsk32A16RGB565SEGreenPosition;

	c0 = FskMoveField(dst, fsk32A16RGB565SEBlueBits, fsk32A16RGB565SEBluePosition, 0);
	c1 = FskMoveField(src, fsk32A16RGB565SEBlueBits, fsk32A16RGB565SEBluePosition, 0);
	c1 -= c0; c1 *= (SInt32)opacity; c1 += 128; c1 += c1 >> 8; c1 >>=8; c1 += c0;
	res |= c1 << fsk32A16RGB565SEBluePosition;

	c0 = FskMoveField(dst, fsk32A16RGB565SEAlphaBits, fsk32A16RGB565SEAlphaPosition, 0);
	c1 = FskMoveField(src, fsk32A16RGB565SEAlphaBits, fsk32A16RGB565SEAlphaPosition, 0);
	c1 -= c0; c1 *= (SInt32)opacity; c1 += 128; c1 += c1 >> 8; c1 >>=8; c1 += c0;
	res |= c1 << fsk32A16RGB565SEAlphaPosition;

	*d = res;
}


/********************************************************************************
 * FskAlphaBlend16AG
 ********************************************************************************/

void
FskAlphaBlend16AG(UInt16 *d, UInt16 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p >> 8;
	b = opacity;
	AlphaScale(a, b);	/* Scale alpha by opacity */
	p &= 0x00FF;		/* Clear old alpha */
	p |= a << 8;		/* Insert scaled alpha */
	FskAlpha16AG(d, p);
}


/********************************************************************************
 * FskAlphaBlend16GA
 ********************************************************************************/

void
FskAlphaBlend16GA(UInt16 *d, UInt16 p, UInt8 opacity)
{
	SInt32 a, b;

	a = p & 0xFF;
	b = opacity;
	AlphaScale(a, b);	/* Scale alpha by opacity */
	p &= 0xFF00;		/* Clear old alpha */
	p |= a;				/* Insert scaled alpha */
	FskAlpha16GA(d, p);
}


/********************************************************************************
 * FskAlphaBlend16XXXA
 ********************************************************************************/

void
FskAlphaBlend16XXXA(UInt16 *d, UInt16 p, UInt8 opacity)
{
	SInt32 a, b;

	a = FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, 0);
	b = opacity;
	AlphaScale(a, b);										/* Scale alpha by opacity */
	p &= ~fskFieldMask(4,fsk16RGBA4444SEAlphaPosition);		/* Clear old alpha */
	p |= a << fsk16RGBA4444SEAlphaPosition;					/* Insert scaled alpha */
	FskAlpha16XXXA(d, p);
}


/********************************************************************************
 * FskAlphaBlend16XAXX
 ********************************************************************************/

void
FskAlphaBlend16XAXX(UInt16 *d, UInt16 p, UInt8 opacity)
{
	SInt32 a, b;

	a = FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, 0);
	b = opacity;
	AlphaScale(a, b);										/* Scale alpha by opacity */
	p &= ~fskFieldMask(4,fsk16RGBA4444DEAlphaPosition);		/* Clear old alpha */
	p |= a << fsk16RGBA4444DEAlphaPosition;					/* Insert scaled alpha */
	FskAlpha16XAXX(d, p);
}


/********************************************************************************
 * FskAlphaScale16GA
 ********************************************************************************/

UInt16 FskAlphaScale16GA(UInt8 a, UInt16 p)
{
	UInt32 t;
	t = p >> 8; AlphaScale(t, a); p &= 0x00FF; p |= t << 8;
	t = p >> 0; AlphaScale(t, a); p &= 0xFF00; p |= t << 0;
	return p;
}


/********************************************************************************
 * FskAlphaScale24
 ********************************************************************************/

Fsk24BitType FskAlphaScale24(UInt8 a, Fsk24BitType p)
{
	UInt32 t;
	t = p.c[0]; AlphaScale(t, a); p.c[0] = (UInt8)t;
	t = p.c[1]; AlphaScale(t, a); p.c[1] = (UInt8)t;
	t = p.c[2]; AlphaScale(t, a); p.c[2] = (UInt8)t;
	return p;
}


/********************************************************************************
 * FskAlphaAXPBY32
 * Compute
 *	a * x + b * y
 * where a and b are alphas with [0, 255] mapping to [0, 1],
 * x and y are 32-bit pixels with four 8-bit components.
 ********************************************************************************/

static UInt32 FskAlphaAXPBY32(UInt8 a, UInt32 x, UInt8 b, UInt32 y)
{
	UInt32 d;
	unsigned i, k;
	const UInt8 *xp = (const UInt8*)(&x);
	const UInt8 *yp = (const UInt8*)(&y);
	UInt8       *dp = (      UInt8*)(&d);

	for (i = 4; i--; ++xp, ++yp, ++dp) {
		k = (unsigned)a * (unsigned)(*xp) + (unsigned)b * (unsigned)(*yp) + 128U;
		k += k >> 8;
		k >>= 8;
		if (k > 255)
			k = 255;
		*dp = k;
	}
	return d;
}


/********************************************************************************
 * FskAlphaScale32
 ********************************************************************************/

UInt32 FskAlphaScale32(UInt8 a, UInt32 p)
{
	UInt32 m = 0x00FF00FF;	/* mask */
	UInt32 r = 0x00800080;	/* round */
	UInt32 q;

	q = p & m;				/* ........RRRRRRRR........BBBBBBBB */
	q *= a;					/* RRRRRRRRrrrrrrrrBBBBBBBBbbbbbbbb */
	q += r;					/* RRRRRRRRrrrrrrrrBBBBBBBBbbbbbbbb */
	q += (q >> 8) & m;		/* RRRRRRRRrrrrrrrrBBBBBBBBbbbbbbbb */
	q >>= 8;				/* ........RRRRRRRRrrrrrrrrBBBBBBBB */
	q &= m;					/* ........RRRRRRRR........BBBBBBBB */

	p >>= 8;				/* ........AAAAAAAARRRRRRRRGGGGGGGG */
	p &= m;					/* ........AAAAAAAA........GGGGGGGG */
	p *= a;					/* AAAAAAAAaaaaaaaaGGGGGGGGgggggggg */
	p += r;					/* AAAAAAAAaaaaaaaaGGGGGGGGgggggggg */
	p += (p >> 8) & m;		/* AAAAAAAAaaaaaaaaGGGGGGGGgggggggg */
	p &= ~m;				/* AAAAAAAA........GGGGGGGG........ */

	p |= q;					/* AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB */
	return p;
}


/********************************************************************************
 * FskAlphaBlackDestinationOverA32
 ********************************************************************************/

void FskAlphaBlackDestinationOverA32(UInt32 *d, UInt32 s)
{
	FskAlphaBlackA32(&s, *d);
	*d = s;
}


/********************************************************************************
 * FskAlphaBlackDestinationOver32A
 ********************************************************************************/

void FskAlphaBlackDestinationOver32A(UInt32 *d, UInt32 s)
{
	FskAlphaBlack32A(&s, *d);
	*d = s;
}


/********************************************************************************
 * FskAlphaBlackSourceInA32
 ********************************************************************************/

void FskAlphaBlackSourceInA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(*d >> 24);				/* dstAlpha */
	if		(a == 0)	*d = 0;
	else if	(a == 255)	*d = s;
	else				*d = FskAlphaScale32(a, s);
}


/********************************************************************************
 * FskAlphaBlackSourceIn32A
 ********************************************************************************/

void FskAlphaBlackSourceIn32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(*d);					/* dstAlpha */
	if		(a == 0)	*d = 0;
	else if	(a == 255)	*d = s;
	else				*d = FskAlphaScale32(a, s);
}


/********************************************************************************
 * FskAlphaBlackDestinationInA32
 ********************************************************************************/

void FskAlphaBlackDestinationInA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(s >> 24);				/* srcAlpha */
	if		(a == 0)	*d = 0;
	else if	(a != 255)	*d = FskAlphaScale32(a, *d);
}


/********************************************************************************
 * FskAlphaBlackDestinationIn32A
 ********************************************************************************/

void FskAlphaBlackDestinationIn32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)s;						/* srcAlpha */
	if		(a == 0)	*d = 0;
	else if	(a != 255)	*d = FskAlphaScale32(a, *d);
}


/********************************************************************************
 * FskAlphaBlackSourceOutA32
 ********************************************************************************/

void FskAlphaBlackSourceOutA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d >> 24));			/* 1 - dstAlpha */
	if		(a == 0)	*d = 0;
	else if	(a == 255)	*d = s;
	else				*d = FskAlphaScale32(a, s);
}


/********************************************************************************
 * FskAlphaBlackSourceOut32A
 ********************************************************************************/

void FskAlphaBlackSourceOut32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d));				/* 1 - dstAlpha */
	if		(a == 0)	*d = 0;
	else if	(a == 255)	*d = s;
	else				*d = FskAlphaScale32(a, s);
}


/********************************************************************************
 * FskAlphaBlackDestinationOutA32
 ********************************************************************************/

void FskAlphaBlackDestinationOutA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(s >> 24));			/* 1 - srcAlpha */
	if		(a == 0)	*d = 0;
	else if	(a != 255)	*d = FskAlphaScale32(a, *d);
}


/********************************************************************************
 * FskAlphaBlackDestinationOut32A
 ********************************************************************************/

void FskAlphaBlackDestinationOut32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(s));				/* 1 - srcAlpha */
	if		(a == 0)	*d = 0;
	else if	(a != 255)	*d = FskAlphaScale32(a, *d);
}


/********************************************************************************
 * FskAlphaBlackSourceAtopA32
 ********************************************************************************/

void FskAlphaBlackSourceAtopA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(*d >> 24);			/* dstAlpha */
	UInt8 b = (UInt8)(~(s >> 24));			/* 1 - srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


/********************************************************************************
 * FskAlphaBlackSourceAtop32A
 ********************************************************************************/

void FskAlphaBlackSourceAtop32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(*d);					/* dstAlpha */
	UInt8 b = (UInt8)(~s);					/* 1 - srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


/********************************************************************************
 * FskAlphaBlackDestinationAtopA32
 ********************************************************************************/

void FskAlphaBlackDestinationAtopA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d >> 24));			/* 1 - dstAlpha */
	UInt8 b = (UInt8)(s >> 24);				/* srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


/********************************************************************************
 * FskAlphaBlackDestinationAtop32A
 ********************************************************************************/

void FskAlphaBlackDestinationAtop32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d));				/* 1 - dstAlpha */
	UInt8 b = (UInt8)s;						/* srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


/********************************************************************************
 * FskAlphaBlackLighter
 ********************************************************************************/

void FskAlphaBlackLighter(UInt32 *d, UInt32 s)
{
	UInt32 dd = *d;							/* Get d */
	UInt8 *dp = (UInt8*)(&dd);				/* Pointer to d pixels */
	UInt8 *sp = (UInt8*)(&s);				/* Pointer to s pixels */
	int i;
	for (i = 4; i--; ++dp, ++sp) {
		unsigned sum = (unsigned)(*dp) + (unsigned)(*sp);
		if (sum > 255)
			sum = 255;
		*dp = sum;
	}
	*d = dd;
}


/********************************************************************************
 * FskAlphaBlackXorA32
 ********************************************************************************/

void FskAlphaBlackXorA32(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d >> 24));			/* 1 - dstAlpha */
	UInt8 b = (UInt8)(~(s >> 24));			/* 1 - srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


/********************************************************************************
 * FskAlphaBlackXor32A
 ********************************************************************************/

void FskAlphaBlackXor32A(UInt32 *d, UInt32 s)
{
	UInt8 a = (UInt8)(~(*d));				/* 1 - dstAlpha */
	UInt8 b = (UInt8)(~s);					/* 1 - srcAlpha */
	*d = FskAlphaAXPBY32(a, s, b, *d);
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 **		Pixel multiplication
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskPixelMul32
 ********************************************************************************/

UInt32
FskPixelMul32(UInt32 p0, UInt32 p1)
{
	UInt32 p2		= 0;
	UInt32 c0, c1;

	MulComp(p0, p1, p2,  0,  0,  0,  8);
	MulComp(p0, p1, p2,  8,  8,  8,  8);
	MulComp(p0, p1, p2, 16, 16, 16,  8);
	MulComp(p0, p1, p2, 24, 24, 24,  8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul32Swap
 ********************************************************************************/

UInt32
FskPixelMul32Swap(UInt32 p0, UInt32 p1)
{
	UInt32 p2		= 0;
	UInt32 c0, c1;

	MulComp(p0, p1, p2,  0,  0, 24,  8);
	MulComp(p0, p1, p2,  8,  8, 16,  8);
	MulComp(p0, p1, p2, 16, 16,  8,  8);
	MulComp(p0, p1, p2, 24, 24,  0,  8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul32A16RGB565SE
 ********************************************************************************/

UInt32
FskPixelMul32A16RGB565SE(UInt32 p0, UInt32 p1)
{
	UInt32 c0, c1;
	UInt32 p2 = (UInt32)FskPixelMul16SE((UInt16)p0, (UInt16)p1);	/* Multiply least significant 16 bits */
	MulComp(p0, p1, p2, 24, 24, 0, 8);								/* Multiply  most significant  8 bits */
	return p2;														/* Note: the penultimate 8 bits have been zeroed */
}


/********************************************************************************
 * FskPixelMul24
 ********************************************************************************/

Fsk24BitType
FskPixelMul24( Fsk24BitType p0, Fsk24BitType p1)
{
	Fsk24BitType	p2;
	UInt16			c0, c1;

	c0 = p0.c[0];	c1 = p1.c[0];	AlphaNScale(c0, c1, 8);		p2.c[0] = (UInt8)c0;
	c0 = p0.c[1];	c1 = p1.c[1];	AlphaNScale(c0, c1, 8);		p2.c[1] = (UInt8)c0;
	c0 = p0.c[2];	c1 = p1.c[2];	AlphaNScale(c0, c1, 8);		p2.c[2] = (UInt8)c0;

	return(p2);
}


/********************************************************************************
 * FskPixelMul24Swap
 ********************************************************************************/

Fsk24BitType
FskPixelMul24Swap(Fsk24BitType p0, Fsk24BitType p1)
{
	Fsk24BitType	p2;
	UInt16			c0, c1;

	c0 = p0.c[0];	c1 = p1.c[0];	AlphaNScale(c0, c1, 8);		p2.c[2] = (UInt8)c0;
	c0 = p0.c[1];	c1 = p1.c[1];	AlphaNScale(c0, c1, 8);		p2.c[1] = (UInt8)c0;
	c0 = p0.c[2];	c1 = p1.c[2];	AlphaNScale(c0, c1, 8);		p2.c[0] = (UInt8)c0;

	return(p2);
}


/********************************************************************************
 * FskPixelMul16SE
 ********************************************************************************/

UInt16
FskPixelMul16SE(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0,  0,  5);
	MulComp(p0, p1, p2,  5,  5,  5,  6);
	MulComp(p0, p1, p2, 11, 11, 11,  5);

	return(p2);
}


/********************************************************************************
 * FskPixelMul16SESwap
 ********************************************************************************/

UInt16
FskPixelMul16SESwap(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0, 11,  5);
	MulComp(p0, p1, p2,  5,  5,  5,  6);
	MulComp(p0, p1, p2, 11, 11,  0,  5);

	return(p2);
}


/********************************************************************************
 * FskPixelMul16DE
 ********************************************************************************/

UInt16
FskPixelMul16DE(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	p0 = (p0 << 8) | (p0 >> 8);
	p1 = (p1 << 8) | (p1 >> 8);
	MulComp(p0, p1, p2,  0,  0,  0,  5);
	MulComp(p0, p1, p2,  5,  5,  5,  6);
	MulComp(p0, p1, p2, 11, 11, 11,  5);
	p2 = (p2 << 8) | (p2 >> 8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul16DESwap
 ********************************************************************************/

UInt16
FskPixelMul16DESwap(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	p0 = (p0 << 8) | (p0 >> 8);
	p1 = (p1 << 8) | (p1 >> 8);
	MulComp(p0, p1, p2,  0,  0, 11,  5);
	MulComp(p0, p1, p2,  5,  5,  5,  6);
	MulComp(p0, p1, p2, 11, 11,  0,  5);
	p2 = (p2 << 8) | (p2 >> 8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul5515SE
 ********************************************************************************/

UInt16
FskPixelMul5515SE(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0,  0,  5);
	MulComp(p0, p1, p2,  6,  6,  6,  5);
	MulComp(p0, p1, p2, 11, 11, 11,  5);

	return(p2);
}


/********************************************************************************
 * FskPixelMul5515SESwap
 ********************************************************************************/

UInt16
FskPixelMul5515SESwap(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0, 11,  5);
	MulComp(p0, p1, p2,  6,  6,  6,  5);
	MulComp(p0, p1, p2, 11, 11,  0,  5);

	return(p2);
}


/********************************************************************************
 * FskPixelMul5515DE
 ********************************************************************************/

UInt16
FskPixelMul5515DE(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	p0 = (p0 << 8) | (p0 >> 8);
	p1 = (p1 << 8) | (p1 >> 8);
	MulComp(p0, p1, p2,  0,  0,  0,  5);
	MulComp(p0, p1, p2,  6,  6,  6,  5);
	MulComp(p0, p1, p2, 11, 11, 11,  5);
	p2 = (p2 << 8) | (p2 >> 8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul5515DESwap
 ********************************************************************************/

UInt16
FskPixelMul5515DESwap(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	p0 = (p0 << 8) | (p0 >> 8);
	p1 = (p1 << 8) | (p1 >> 8);
	MulComp(p0, p1, p2,  0,  0, 11,  5);
	MulComp(p0, p1, p2,  6,  6,  6,  5);
	MulComp(p0, p1, p2, 11, 11,  0,  5);
	p2 = (p2 << 8) | (p2 >> 8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul4444
 ********************************************************************************/

UInt16
FskPixelMul4444(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0,  0,  4);
	MulComp(p0, p1, p2,  4,  4,  4,  4);
	MulComp(p0, p1, p2,  8,  8,  8,  4);
	MulComp(p0, p1, p2, 12, 12, 12,  4);

	return(p2);
}


/********************************************************************************
 * FskPixelMul88
 ********************************************************************************/

UInt16
FskPixelMul88(UInt16 p0, UInt16 p1)
{
	UInt16 p2		= 0;
	UInt16 c0, c1;

	MulComp(p0, p1, p2,  0,  0,  0,  8);
	MulComp(p0, p1, p2,  8,  8,  8,  8);

	return(p2);
}


/********************************************************************************
 * FskPixelMul8 - symmetric
 ********************************************************************************/

UInt8
FskPixelMul8(UInt8 p0, UInt8 p1)
{
	register unsigned int p;

	p = p0;
	AlphaScale(p, p1);

	return (UInt8)p;
}


/********************************************************************************
 * FskPixelScreen8 - symmetric
 * This is the operator that is used fror accumulating alpha.
 ********************************************************************************/

UInt8
FskPixelScreen8(UInt8 d, UInt8 s)
{
	int c = d;
	c += s;
	c -= FskPixelMul8(d, s);
	return (UInt8)c;
}


/********************************************************************************
 * FskPixelHardLight8 - asymmetric
 * Make the image darker or lighter, depending on whether s is less than or greater than 128.
 ********************************************************************************/

UInt8
FskPixelHardLight8(UInt8 d, UInt8 s)
{
	return (s < 128) ? FskPixelMul8(d, s << 1) : FskPixelScreen8(d, (UInt8)(((UInt32)s << 1) - 255));
}


/********************************************************************************
 * FskPixelOverlay8 - asymmetric
 ********************************************************************************/

UInt8
FskPixelOverlay8(UInt8 d, UInt8 s)
{
	return FskPixelHardLight8(s, d);
}


/********************************************************************************
 * FskPixelDarken8 - symmetric
 ********************************************************************************/

UInt8
FskPixelDarken8(UInt8 d, UInt8 s)
{
	return (d <= s) ? d : s;
}


/********************************************************************************
 * FskPixelLighten8 - symmetric
 ********************************************************************************/

UInt8
FskPixelLighten8(UInt8 d, UInt8 s)
{
	return (d >= s) ? d : s;
}


/********************************************************************************
 * FskPixelColorDodge8 - asymmetric
 ********************************************************************************/

UInt8
FskPixelColorDodge8(UInt8 d, UInt8 s)
{
	UInt32 r;
	if (s == 255)
		return 255;
	r = 255 - s;
	r = ((UInt32)d * 255 + (r >> 1)) / r;
	return (r < 255) ? (UInt8)r : 255;
}


/********************************************************************************
 * FskPixelColorBurn8 - asymmetric
 ********************************************************************************/

UInt8
FskPixelColorBurn8(UInt8 d, UInt8 s)
{
	UInt32 r;
	if (s == 0)
		return 0;
	r = ((UInt32)(255 - d) * 255 + (s >> 1)) / s;
	if (r > 255)
		r = 255;
	return (UInt8)(255 - r);
}


/********************************************************************************
 * FskPixelSoftLight8 - asymmetric
 * Like Overlay (not Hardlight), except softer (this matches Photoshop).
 ********************************************************************************/

UInt8
FskPixelSoftLight8(UInt8 d, UInt8 s)
{
	int r;
	if (s < 128) {		/* s <= 0.5 */
		r = d - ((255 - (s << 1)) * d * (255 - d) + (255 * 255 / 2)) / (255 * 255);
	}
	else {				/* s > 0.5 */
		int dd;
		if (d < 64) {	/* d <= 0.25 */
			/* dd = ((16 * d - 12) * d + 4) * d, if d < 0.25
			 * becomes, for d in [0, 255],
			 * ((16 * d - 3060) * d + 260100) * d / 65025
			 */
			dd = (((((int)d << 4) - 3060) * (int)d + 260100) * (int)d + (65025 >> 1)) / 65025;
		}
		else {			/* d > 0.25 */
			/* sqrt(d / 255) * 255 = sqrt(d * 255) */
			dd = FskFixedNSqrt(d * 255, 0);
		}
		/* d + (2 * s - 1) * (dd - d) */
		r = ((((UInt32)s << 1) - 255) * (dd - d) + (255 >> 1)) / 255 + d;
	}
	return (UInt8)r;
}


/********************************************************************************
 * FskPixelDifference8 - symmetric
 * Absolute value of the difference between two images
 ********************************************************************************/

UInt8
FskPixelDifference8(UInt8 d, UInt8 s)
{
	return (d > s) ? (d - s) : (s - d);
}


/********************************************************************************
 * FskPixelExclusion8 - symmetric
 * Like Difference, but not as strong
 ********************************************************************************/

UInt8
FskPixelExclusion8(UInt8 d, UInt8 s)
{
	return (UInt8)(d + s - (((UInt32)d << 1) * s + (255 >> 1)) / 255);
}


/********************************************************************************
 * FskAlphaMul
 ********************************************************************************/

UInt8
FskAlphaMul(UInt8 p0, UInt8 p1)
{
	register unsigned int p;

	p = p0;
	AlphaScale(p, p1);

	return (UInt8)p;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 **		Pixel conversions
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskConvertColorRGBAToBitmapPixel
 ********************************************************************************/

FskErr
FskConvertColorRGBAToBitmapPixel(FskConstColorRGBA rgba, FskBitmapFormatEnum pixelFormat, void *bitmapPixel)
{
	register UInt32 pix;
	FskErr			err		= kFskErrNone;

	pix =	(rgba->a << 24)
		|	(rgba->r << 16)
		|	(rgba->g <<  8)
		|	(rgba->b <<  0);

	switch (pixelFormat) {
		case kFskBitmapFormat32ARGB:		FskName2(fskConvert32ARGB,fsk32ARGBFormatKind)(      pix);	*((UInt32*)bitmapPixel) = (UInt32)pix;				break;
		case kFskBitmapFormat32BGRA:		FskName2(fskConvert32ARGB,fsk32BGRAFormatKind)(      pix);	*((UInt32*)bitmapPixel) = (UInt32)pix;				break;
		case kFskBitmapFormat32RGBA:		FskName2(fskConvert32ARGB,fsk32RGBAFormatKind)(      pix);	*((UInt32*)bitmapPixel) = (UInt32)pix;				break;
		case kFskBitmapFormat32ABGR:		FskName2(fskConvert32ARGB,fsk32ABGRFormatKind)(      pix);	*((UInt32*)bitmapPixel) = (UInt32)pix;				break;
		case kFskBitmapFormat32A16RGB565LE:	FskName2(fskConvert32ARGB,fsk32A16RGB565LEFormatKind)(pix);	*((UInt32*)bitmapPixel) = (UInt32)pix;				break;
		case kFskBitmapFormat16RGB565BE:	FskName2(fskConvert32ARGB,fsk16RGB565BEFormatKind)(  pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat16RGB565LE:	FskName2(fskConvert32ARGB,fsk16RGB565LEFormatKind)(  pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat16RGB5515LE:	FskName2(fskConvert32ARGB,fsk16RGB5515LEFormatKind)( pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat16BGR565LE:	FskName2(fskConvert32ARGB,fsk16BGR565LEFormatKind)(  pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat16RGBA4444LE:	FskName2(fskConvert32ARGB,fsk16RGBA4444LEFormatKind)(pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat16AG:			FskName2(fskConvert32ARGB,fsk16AGFormatKind)(        pix);	*((UInt16*)bitmapPixel) = (UInt16)pix;				break;
		case kFskBitmapFormat8G:			FskName2(fskConvert32ARGB,fsk8GFormatKind)(          pix);	*((UInt8 *)bitmapPixel) = (UInt8 )pix;				break;
		case kFskBitmapFormat24BGR:			FskName2(fskConvert32ARGB,fsk24BGRFormatKind)(       pix,		*((Fsk24BitType*)bitmapPixel));					break;
		case kFskBitmapFormat24RGB:			FskName2(fskConvert32ARGB,fsk24RGBFormatKind)(       pix,		*((Fsk24BitType*)bitmapPixel));					break;
		case kFskBitmapFormat8A:			((UInt8*)bitmapPixel)[0] = rgba->a;																				break;
#if 1 /* This is actually a conversion to YUV444, which has pixels as YUVYUV..., whereas YUV422 is UVYVYUYVY...and YUV420 is YYY...UUU...VVV... */
		case kFskBitmapFormatYUV420:
		case kFskBitmapFormatYUV422:
		case kFskBitmapFormatYUV420i:
		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
		case kFskBitmapFormatUYVY:
			((UInt8*)bitmapPixel)[0] = (UInt8)((( 263 * (signed int)rgba->r + 516 * (signed int)rgba->g + 100 * (signed int)rgba->b + (1 << (10 - 1))) >> 10) +  16);
			((UInt8*)bitmapPixel)[1] = (UInt8)(((-152 * (signed int)rgba->r - 298 * (signed int)rgba->g + 450 * (signed int)rgba->b + (1 << (10 - 1))) >> 10) + 128);
			((UInt8*)bitmapPixel)[2] = (UInt8)((( 450 * (signed int)rgba->r - 377 * (signed int)rgba->g -  73 * (signed int)rgba->b + (1 << (10 - 1))) >> 10) + 128);
			break;
#endif
		default:	err = kFskErrUnsupportedPixelType;	break;
	}
	return err;
}


/********************************************************************************
 * FskConvertColorRGBToBitmapPixel
 ********************************************************************************/

FskErr
FskConvertColorRGBToBitmapPixel(FskConstColorRGB rgb, FskBitmapFormatEnum pixelFormat, void *bitmapPixel)
{
	FskColorRGBARecord rgba;

	rgba.r = rgb->r;
	rgba.g = rgb->g;
	rgba.b = rgb->b;
	rgba.a = 255;

	return FskConvertColorRGBAToBitmapPixel(&rgba, pixelFormat, bitmapPixel);
}


/********************************************************************************
 * FskConvertBitmapPixelToColorRGBA
 ********************************************************************************/

FskErr
FskConvertBitmapPixelToColorRGBA(const void *bitmapPixel, FskBitmapFormatEnum pixelFormat, FskColorRGBA rgba)
{
	register UInt32 pix;
	FskErr			err		= kFskErrNone;

	switch (pixelFormat) {
		case kFskBitmapFormat32ARGB:		pix = *((const UInt32*)bitmapPixel);	FskName3(fskConvert,fsk32ARGBFormatKind,32ARGB)(pix);				break;
		case kFskBitmapFormat32BGRA:		pix = *((const UInt32*)bitmapPixel);	FskName3(fskConvert,fsk32BGRAFormatKind,32ARGB)(pix);				break;
		case kFskBitmapFormat32RGBA:		pix = *((const UInt32*)bitmapPixel);	FskName3(fskConvert,fsk32RGBAFormatKind,32ARGB)(pix);				break;
		case kFskBitmapFormat32ABGR:		pix = *((const UInt32*)bitmapPixel);	FskName3(fskConvert,fsk32ABGRFormatKind,32ARGB)(pix);				break;
		case kFskBitmapFormat32A16RGB565LE:	pix = *((const UInt32*)bitmapPixel);	FskName3(fskConvert,fsk32A16RGB565LEFormatKind,32ARGB)(pix);		break;
		case kFskBitmapFormat16RGB565BE:	pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16RGB565BEFormatKind,32ARGB)(pix);			break;
		case kFskBitmapFormat16RGB565LE:	pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16RGB565LEFormatKind,32ARGB)(pix);			break;
		case kFskBitmapFormat16RGB5515LE:	pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16RGB5515LEFormatKind,32ARGB)(pix);			break;
		case kFskBitmapFormat16BGR565LE:	pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16BGR565LEFormatKind,32ARGB)(pix);			break;
		case kFskBitmapFormat16RGBA4444LE:	pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16RGBA4444LEFormatKind,32ARGB)(pix);			break;
		case kFskBitmapFormat16AG:			pix = *((const UInt16*)bitmapPixel);	FskName3(fskConvert,fsk16AGFormatKind,32ARGB)(pix);					break;
		case kFskBitmapFormat8G:			pix = *((const UInt8 *)bitmapPixel);	FskName3(fskConvert,fsk8GFormatKind,32ARGB)(pix);					break;
		case kFskBitmapFormat8A:			pix = *((const UInt8 *)bitmapPixel) << 24;																	break;
		case kFskBitmapFormat24BGR:			FskName3(fskConvert,fsk24BGRFormatKind,32ARGB)(*((const Fsk24BitType*)bitmapPixel), pix);					break;
		case kFskBitmapFormat24RGB:			FskName3(fskConvert,fsk24RGBFormatKind,32ARGB)(*((const Fsk24BitType*)bitmapPixel), pix);					break;
#if 1 /* This is actually a conversion from YUV444, which has pixels as YUVYUV..., whereas YUV422 is UVYVYUYVY...and YUV420 is YYY...UUU...VVV... */
		case kFskBitmapFormatYUV420:
		case kFskBitmapFormatYUV422:		pix = FskConvertYUV42032ARGB(((UInt8*)bitmapPixel)[0], ((UInt8*)bitmapPixel)[1], ((UInt8*)bitmapPixel)[2]);	break;
		case kFskBitmapFormatYUV420i:
		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
		case kFskBitmapFormatUYVY:
#endif
		default:							pix = 0; err = kFskErrUnsupportedPixelType;																	break;
	}

	rgba->a = (UInt8)(pix >> 24);
	rgba->r = (UInt8)(pix >> 16);
	rgba->g = (UInt8)(pix >>  8);
	rgba->b = (UInt8)(pix >>  0);

	return err;
}


/********************************************************************************
 * FskConvertBitmapPixelToColorRGB
 ********************************************************************************/

FskErr
FskConvertBitmapPixelToColorRGB(const void *bitmapPixel, FskBitmapFormatEnum pixelFormat, FskColorRGB rgb)
{
	FskColorRGBARecord rgba;
	FskErr				err;

	err = FskConvertBitmapPixelToColorRGBA(bitmapPixel, pixelFormat, &rgba);
	rgb->r = rgba.r;
	rgb->g = rgba.g;
	rgb->b = rgba.b;

	return err;
}


/********************************************************************************
 * Pixel format info
 ********************************************************************************/

#define PIXEL_FORMAT_CODES_ARE_OUT_OF_SYNC ( \
	kFskBitmapFormat24BGR			!= (1U) 	|| \
	kFskBitmapFormat32ARGB			!= (2U)		|| \
	kFskBitmapFormat32BGRA			!= (3U)		|| \
	kFskBitmapFormat32RGBA			!= (4U)		|| \
	kFskBitmapFormat24RGB			!= (5U)		|| \
	kFskBitmapFormat16RGB565BE		!= (6U)		|| \
	kFskBitmapFormat16RGB565LE		!= (7U)		|| \
	kFskBitmapFormat16RGB5515LE		!= (8U)		|| \
	kFskBitmapFormatYUV420			!= (9U)		|| \
	kFskBitmapFormatYUV422			!= (10U)	|| \
	kFskBitmapFormat8A				!= (11U)	|| \
	kFskBitmapFormat8G				!= (12U)	|| \
	kFskBitmapFormatYUV420spuv		!= (13U)	|| \
	kFskBitmapFormat16BGR565LE		!= (14U)	|| \
	kFskBitmapFormat16AG			!= (15U)	|| \
	kFskBitmapFormat32ABGR			!= (16U)	|| \
	kFskBitmapFormat16RGBA4444LE	!= (17U)	|| \
	kFskBitmapFormatUYVY			!= (18U)	|| \
	kFskBitmapFormatYUV420i			!= (19U)	|| \
	kFskBitmapFormat32A16RGB565LE	!= (20U)	|| \
	kFskBitmapFormatYUV420spvu		!= (21U)	|| \
	kFskBitmapFormatGLRGB			!= (22U)	|| \
	kFskBitmapFormatGLRGBA			!= (23U)	)

typedef struct FskPixelInfo {
	UInt8	pixelBytes;
	UInt8	alphaPosition;
	UInt8	redPosition;
	UInt8	greenPosition;
	UInt8	bluePosition;
	UInt8	grayPosition;
	UInt8	alphaBits;
	UInt8	redBits;
	UInt8	greenBits;
	UInt8	blueBits;
	UInt8	grayBits;
	UInt8	canSrc;
	UInt8	canDst;
	UInt8	pixelPacking;
	const char *name;
} FskPixelInfo;


#define PIXELINFO(type) {											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),Bytes),				\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BluePosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GrayPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BlueBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GrayBits),			\
	FskName2(SRC_,type),											\
	FskName2(DST_,type),											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),PixelPacking),		\
	#type															}
#define PIXELINFO4(type) {											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),Bytes),				\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BluePosition),		\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BlueBits),			\
	0,																\
	FskName2(SRC_,type),											\
	FskName2(DST_,type),											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),PixelPacking),		\
	#type															}
#define PIXELINFO3(type) {											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),Bytes),				\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BluePosition),		\
	0,																\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BlueBits),			\
	0,																\
	FskName2(SRC_,type),											\
	FskName2(DST_,type),											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),PixelPacking),		\
	#type															}
#define PIXELINFO3B(type) {											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),Bytes),				\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedPosition)   << 3,	\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenPosition) << 3,	\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BluePosition)  << 3,	\
	0,																\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),RedBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GreenBits),			\
	FskName3(fsk,FskName3(fsk,type,FormatKind),BlueBits),			\
	0,																\
	FskName2(SRC_,type),											\
	FskName2(DST_,type),											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),PixelPacking),		\
	#type															}
#define PIXELINFO2(type) {											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),Bytes),				\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaPosition),		\
	0,																\
	0,																\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GrayPosition),		\
	FskName3(fsk,FskName3(fsk,type,FormatKind),AlphaBits),			\
	0,																\
	0,																\
	0,																\
	FskName3(fsk,FskName3(fsk,type,FormatKind),GrayBits),			\
	FskName2(SRC_,type),											\
	FskName2(DST_,type),											\
	FskName3(fsk,FskName3(fsk,type,FormatKind),PixelPacking),		\
	#type															}
static const FskPixelInfo gFskPixelInfo[] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				0,				fskUnknownPixelPacking,				"unknown"		},	// 0: Unknown
	PIXELINFO3B(24BGR),																											// 1
	PIXELINFO4(32ARGB),																											// 2
	PIXELINFO4(32BGRA),																											// 3
	PIXELINFO4(32RGBA),																											// 4
	PIXELINFO3B(24RGB),																											// 5
	PIXELINFO3(16RGB565BE),																										// 6
	PIXELINFO3(16RGB565LE),																										// 7
	PIXELINFO4(16RGB5515LE),																									// 8
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_YUV420,		DST_YUV420,		fskPlanarPixelPacking,				"YUV420"		},	// 9:  YUV420
	{ 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_YUV422,		DST_YUV422,		fskNonUniformChunkyPixelPacking,	"YUV422"		},	// 10: YUV422
	{ 1, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, SRC_8A,			DST_8A,			fskUniformChunkyPixelPacking,		"8A"			},	// 11: 8A
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, SRC_8G,			DST_8G,			fskUniformChunkyPixelPacking,		"8G"			},	// 12: 8G
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_YUV420spuv,	DST_YUV420spuv,	fskSemiPlanarPixelPacking,			"YUV420spuv"	},	// 13: YUV420spuv
	PIXELINFO3(16BGR565LE),																										// 14
	PIXELINFO2(16AG),																											// 15
	PIXELINFO4(32ABGR),																											// 16
	PIXELINFO4(16RGBA4444LE),																									// 17
	{ 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_UYVY,		DST_UYVY,		fskNonUniformChunkyPixelPacking,	"UYVY"			},	// 18: UYVY
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_YUV420i,		DST_YUV420i,	fskInterleavePixelPacking,			"YUV420i"		},	// 19: YUV420i
	PIXELINFO4(32A16RGB565LE),																									// 20
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SRC_YUV420spvu,	DST_YUV420spvu,	fskSemiPlanarPixelPacking,			"YUV420spvu"	},	// 21: YUV420spvu
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				0,				fskUnknownPixelPacking,				"GLRGB"			},	// 22: GLRGB   (only for rendering directly to a GL texture)
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				0,				fskUnknownPixelPacking,				"GLRGBA"		}	// 23: GLRGBA  (only for rendering directly to a GL texture)
};


/********************************************************************************
 * FskBitmapFormat Info
 ********************************************************************************/

static const FskPixelInfo* GetPixelInfo(FskBitmapFormatEnum pixelFormat) {
	if (PIXEL_FORMAT_CODES_ARE_OUT_OF_SYNC) {
		#if SUPPORT_INSTRUMENTATION
			FskInstrumentedSystemPrintfForLevel(kFskInstrumentationLevelNormal, "PIXEL FORMAT CODES ARE OUT OF SYNC");
		#endif /* SUPPORT_INSTRUMENTATION */
		return NULL;
	}
	if ((UInt32)pixelFormat >= (sizeof(gFskPixelInfo) / sizeof(gFskPixelInfo[0])))
		pixelFormat = kFskBitmapFormatUnknown;
	return &gFskPixelInfo[pixelFormat];
}


/********************************************************************************
 * FskBitmapFormat Info
 ********************************************************************************/

UInt8 FskBitmapFormatDepth        (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes * 8;	}
UInt8 FskBitmapFormatPixelBytes   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes;		}
UInt8 FskBitmapFormatAlphaPosition(FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->alphaPosition;	}
UInt8 FskBitmapFormatRedPosition  (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->redPosition;	}
UInt8 FskBitmapFormatGreenPosition(FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->greenPosition;	}
UInt8 FskBitmapFormatBluePosition (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->bluePosition;	}
UInt8 FskBitmapFormatGrayPosition (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->grayPosition;	}
UInt8 FskBitmapFormatAlphaBits    (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->alphaBits;		}
UInt8 FskBitmapFormatRedBits      (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->redBits;		}
UInt8 FskBitmapFormatGreenBits    (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->greenBits;		}
UInt8 FskBitmapFormatBlueBits     (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->blueBits;		}
UInt8 FskBitmapFormatGrayBits     (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->grayBits;		}
UInt8 FskBitmapFormatCanSrc       (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->canSrc;			}
UInt8 FskBitmapFormatCanDst       (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->canDst;			}
UInt8 FskBitmapFormatPixelPacking (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelPacking;	}
const char* FskBitmapFormatName   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->name;			}

#if TARGET_RT_LITTLE_ENDIAN
UInt8 FskBitmapFormatAlphaOffset  (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->alphaPosition >> 3;	}
UInt8 FskBitmapFormatRedOffset    (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->redPosition   >> 3;	}
UInt8 FskBitmapFormatGreenOffset  (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->greenPosition >> 3;	}
UInt8 FskBitmapFormatBlueOffset   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->bluePosition  >> 3;	}
UInt8 FskBitmapFormatGrayOffset   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->grayPosition  >> 3;	}
#else /* TARGET_RT_BIG_ENDIAN */
UInt8 FskBitmapFormatAlphaOffset  (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes - 1 - (GetPixelInfo(pixelFormat)->alphaPosition >> 3);	}
UInt8 FskBitmapFormatRedOffset    (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes - 1 - (GetPixelInfo(pixelFormat)->redPosition   >> 3);	}
UInt8 FskBitmapFormatGreenOffset  (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes - 1 - (GetPixelInfo(pixelFormat)->greenPosition >> 3);	}
UInt8 FskBitmapFormatBlueOffset   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes - 1 - (GetPixelInfo(pixelFormat)->bluePosition  >> 3);	}
UInt8 FskBitmapFormatGrayOffset   (FskBitmapFormatEnum pixelFormat) { return GetPixelInfo(pixelFormat)->pixelBytes - 1 - (GetPixelInfo(pixelFormat)->grayPosition  >> 3);	}
#endif /* TARGET_RT_BIG_ENDIAN */
