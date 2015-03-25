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
	\file	FskPixelOps.h
	\brief	Operations on a pixel.
*/

#ifndef __FSKPIXELOPS__
#define __FSKPIXELOPS__

#include "FskBitmap.h"
#include "FskGraphics.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* This is a definition of a 24-bit data type.
 * Beware, for many compilers, sizeof(Fsk24BitType) != 3
 */
typedef struct Fsk24BitType { UInt8 c[3]; } Fsk24BitType;

/* This union can be fed to FskConvertColorRGBToBitmapPixel() or FskConvertColorRGBAToBitmapPixel,
 * and the appropriate element be extracted reliably.
 * The C99 standard, section 6.7.2.1, "Structure and union specifiers, paragraph 14, says:
 * A pointer to a union object, suitably converted, points to each of its members
 * (or if a member is a bitfield, then to the unit in which it resides), and vice versa.
 */
typedef union FskPixelType { UInt8 p8; UInt16 p16; Fsk24BitType p24; UInt32 p32; FskColorRGBARecord rgba; FskColorRGBRecord rgb; UInt8 b4[4];  } FskPixelType;

#define fskDefaultAlpha			255						/* Default 8 bit alpha */
#define fskDefaultAlpha1		(fskDefaultAlpha >> 7)	/* Default 1 bit alpha */
#define fskDefaultAlpha4		(fskDefaultAlpha >> 4)	/* Default 4 bit alpha */



/********************************************************************************
 * ANSI C preprocessors will not expand the arguments to a macro;
 * so we need to add a level of indirection to allow macro expansion of
 * arguments.  (Reiser preprocessors allowed the first arg to be expanded;
 * this method will allow both to be expanded, which is better than none.)
 ********************************************************************************/

#define FskName2(a,b)				_FskName2_aux(a,b)
#define _FskName2_aux(a,b)			a##b
#define FskName3(a,b,c)				_FskName3_aux(a,b,c)
#define _FskName3_aux(a,b,c)		a##b##c
#define FskName4(a,b,c,d)			_FskName4_aux(a,b,c,d)
#define _FskName4_aux(a,b,c,d)		a##b##c##d
#define FskName5(a,b,c,d,e)			_FskName5_aux(a,b,c,d,e)
#define _FskName5_aux(a,b,c,d,e)	a##b##c##d##e
#define FskName6(a,b,c,d,e)			_FskName6_aux(a,b,c,d,e,f)
#define _FskName6_aux(a,b,c,d,e)	a##b##c##d##e##f


/********************************************************************************
 * Styles of pixel packing
 ********************************************************************************/

#define fskUnknownPixelPacking			0		/* Unknown */
#define fskUniformChunkyPixelPacking	3		/* Uniformly sampled [RGB] */
#define fskNonUniformChunkyPixelPacking	4		/* Nonuniformly sampled [YUYV]  */
#define fskPlanarPixelPacking			111		/* Unpacked, at several locations [Y][U][V] */
#define fskSemiPlanarPixelPacking		12		/* Planar, but some components are chunked into a plane [Y][UV] */
#define	fskInterleavePixelPacking		6		/* uvyyyy [UVYYYY] */


/********************************************************************************
 * The following macro definitions are for our internal representation,
 * and may not correspond to the naming conventions in other API's.
 * In particular, for 32 and 16 bit formats, we use big-endian component order
 * as viewed in a word.
 ********************************************************************************/


/********************************************************************************
 * 32ARGB
 ********************************************************************************/

#define fsk32ARGBPixelPacking			fskUniformChunkyPixelPacking
#define fsk32ARGBAlphaPosition			24	/* Little-endian bit offsets */
#define fsk32ARGBRedPosition			16
#define fsk32ARGBGreenPosition			8
#define fsk32ARGBBluePosition			0
#define fsk32ARGBAlphaBits				8
#define fsk32ARGBRedBits				8
#define fsk32ARGBGreenBits				8
#define fsk32ARGBBlueBits				8
#define fsk32ARGBBytes					4
#define Fsk32ARGBType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32ARGBKindFormat			32ARGB
# define fsk32ARGBFormatKind			32ARGB
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32ARGBKindFormat			32BGRA
# define fsk32BGRAFormatKind			32ARGB
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 32BGRA
 ********************************************************************************/

#define fsk32BGRAPixelPacking			fskUniformChunkyPixelPacking
#define fsk32BGRAAlphaPosition			0	/* Little-endian bit offsets */
#define fsk32BGRARedPosition			8
#define fsk32BGRAGreenPosition			16
#define fsk32BGRABluePosition			24
#define fsk32BGRAAlphaBits				8
#define fsk32BGRARedBits				8
#define fsk32BGRAGreenBits				8
#define fsk32BGRABlueBits				8
#define fsk32BGRABytes					4
#define Fsk32BGRAType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32BGRAKindFormat			32BGRA
# define fsk32BGRAFormatKind			32BGRA
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32BGRAKindFormat			32ARGB
# define fsk32ARGBFormatKind			32BGRA
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 32ABGR
 ********************************************************************************/

#define fsk32ABGRPixelPacking			fskUniformChunkyPixelPacking
#define fsk32ABGRAlphaPosition			24	/* Little-endian bit offsets */
#define fsk32ABGRRedPosition			0
#define fsk32ABGRGreenPosition			8
#define fsk32ABGRBluePosition			16
#define fsk32ABGRAlphaBits				8
#define fsk32ABGRRedBits				8
#define fsk32ABGRGreenBits				8
#define fsk32ABGRBlueBits				8
#define fsk32ABGRBytes					4
#define Fsk32ABGRType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32ABGRKindFormat			32ABGR
# define fsk32ABGRFormatKind			32ABGR
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32ABGRKindFormat			32RGBA
# define fsk32RGBAFormatKind			32ABGR
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 32RGBA
 ********************************************************************************/

#define fsk32RGBAPixelPacking			fskUniformChunkyPixelPacking
#define fsk32RGBAAlphaPosition			0	/* Little-endian bit offsets */
#define fsk32RGBARedPosition			24
#define fsk32RGBAGreenPosition			16
#define fsk32RGBABluePosition			8
#define fsk32RGBAAlphaBits				8
#define fsk32RGBARedBits				8
#define fsk32RGBAGreenBits				8
#define fsk32RGBABlueBits				8
#define fsk32RGBABytes					4
#define Fsk32RGBAType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32RGBAKindFormat			32RGBA
# define fsk32RGBAFormatKind			32RGBA
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32RGBAKindFormat			32ABGR
# define fsk32ABGRFormatKind			32RGBA
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 24RGB
 ********************************************************************************/

#define fsk24RGBPixelPacking			fskUniformChunkyPixelPacking
#define fsk24RGBRedPosition				0	/* Big-endian byte offsets */
#define fsk24RGBGreenPosition			1
#define fsk24RGBBluePosition			2
#define fsk24RGBAlphaBits				0
#define fsk24RGBRedBits					8
#define fsk24RGBGreenBits				8
#define fsk24RGBBlueBits				8
#define fsk24RGBBytes					3
#define Fsk24RGBType					Fsk24BitType	/* You cannot rely on this to be 3 bytes */
#define fsk24RGBKindFormat				24RGB
#define fsk24RGBFormatKind				24RGB


/********************************************************************************
 * 24BGR
 ********************************************************************************/

#define fsk24BGRPixelPacking			fskUniformChunkyPixelPacking
#define fsk24BGRRedPosition				2	/* Big-endian byte offsets */
#define fsk24BGRGreenPosition			1
#define fsk24BGRBluePosition			0
#define fsk24BGRAlphaBits				0
#define fsk24BGRRedBits					8
#define fsk24BGRGreenBits				8
#define fsk24BGRBlueBits				8
#define fsk24BGRBytes					3
#define Fsk24BGRType					Fsk24BitType	/* You cannot rely on this to be 3 bytes */
#define fsk24BGRKindFormat				24BGR
#define fsk24BGRFormatKind				24BGR


/********************************************************************************
 * 16RGB565SE												RRRRR GGGGGG BBBBB
 *															11111 100000 00000
 *															54321 098765 43210
 ********************************************************************************/

#define fsk16RGB565SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB565SERedPosition			11	/* Little-endian bit offsets */
#define fsk16RGB565SEGreenPosition			5
#define fsk16RGB565SEBluePosition			0
#define fsk16RGB565SEAlphaBits				0
#define fsk16RGB565SERedBits				5
#define fsk16RGB565SEGreenBits				6
#define fsk16RGB565SEBlueBits				5
#define fsk16RGB565SEBytes					2
#define Fsk16RGB565SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB565SEKindFormat			16RGB565BE
# define fsk16RGB565BEFormatKind			16RGB565SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB565SEKindFormat			16RGB565LE
# define fsk16RGB565LEFormatKind			16RGB565SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGB565DE												ggg BBBBB RRRRR GGG
 *															111 11100 00000 000
 *															543 21098 76543 210
 ********************************************************************************/

#define fsk16RGB565DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB565DERedPosition			3	/* Little-endian bit offsets */
#define fsk16RGB565DEGreenPosition			13
#define fsk16RGB565DEBluePosition			8
#define fsk16RGB565DEAlphaBits				0
#define fsk16RGB565DERedBits				5
#define fsk16RGB565DEGreenBits				6
#define fsk16RGB565DEBlueBits				5
#define fsk16RGB565DEBytes					2
#define Fsk16RGB565DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB565DEKindFormat			16RGB565LE
# define fsk16RGB565LEFormatKind			16RGB565DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB565DEKindFormat			16RGB565BE
# define fsk16RGB565BEFormatKind			16RGB565DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16BGR565SE												BBBBB GGGGGG RRRRR
 *															11111 100000 00000
 *															54321 098765 43210
 ********************************************************************************/

#define fsk16BGR565SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16BGR565SERedPosition			0	/* Little-endian bit offsets */
#define fsk16BGR565SEGreenPosition			5
#define fsk16BGR565SEBluePosition			11
#define fsk16BGR565SEAlphaBits				0
#define fsk16BGR565SERedBits				5
#define fsk16BGR565SEGreenBits				6
#define fsk16BGR565SEBlueBits				5
#define fsk16BGR565SEBytes					2
#define Fsk16BGR565SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16BGR565SEKindFormat			16BGR565BE
# define fsk16BGR565BEFormatKind			16BGR565SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16BGR565SEKindFormat			16BGR565LE
# define fsk16BGR565LEFormatKind			16BGR565SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16BGR565DE												ggg RRRRR BBBBB GGG
 *															111 11100 00000 000
 *															543 21098 76543 210
 ********************************************************************************/

#define fsk16BGR565DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16BGR565DERedPosition			8	/* Little-endian bit offsets */
#define fsk16BGR565DEGreenPosition			13
#define fsk16BGR565DEBluePosition			3
#define fsk16BGR565DEAlphaBits				0
#define fsk16BGR565DERedBits				5
#define fsk16BGR565DEGreenBits				6
#define fsk16BGR565DEBlueBits				5
#define fsk16BGR565DEBytes					2
#define Fsk16BGR565DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16BGR565DEKindFormat			16BGR565LE
# define fsk16BGR565LEFormatKind			16BGR565DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16BGR565DEKindFormat			16BGR565BE
# define fsk16BGR565BEFormatKind			16BGR565DE
#endif /* TARGET_RT_LITTLE_ENDIAN */



/********************************************************************************
 * 16RGB555SE												A RRRRR GGGGG BBBBB
 *															1 11111 00000 00000
 *															5 43210 98765 43210
 ********************************************************************************/

#define fsk16RGB555SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB555SEAlphaPosition			15
#define fsk16RGB555SERedPosition			10	/* Little-endian bit offsets */
#define fsk16RGB555SEGreenPosition			5
#define fsk16RGB555SEBluePosition			0
#define fsk16RGB555SEAlphaBits				1
#define fsk16RGB555SERedBits				5
#define fsk16RGB555SEGreenBits				5
#define fsk16RGB555SEBlueBits				5
#define fsk16RGB555SEBytes					2
#define fsk16RGB555SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB555SEKindFormat			16RGB555BE
# define fsk16RGB555BEFormatKind			16RGB555SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB555SEKindFormat			16RGB555LE
# define fsk16RGB555LEFormatKind			16RGB555SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGB555DE												ggg BBBBB A RRRRR GG
 *															111 11100 0 00000 00
 *															543 21098 7 65432 10
 ********************************************************************************/

#define fsk16RGB555DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB555DEAlphaPosition			7
#define fsk16RGB555DERedPosition			2	/* Little-endian bit offsets */
#define fsk16RGB555DEGreenPosition			13
#define fsk16RGB555DEBluePosition			8
#define fsk16RGB555DEAlphaBits				1
#define fsk16RGB555DERedBits				5
#define fsk16RGB555DEGreenBits				5
#define fsk16RGB555DEBlueBits				5
#define fsk16RGB555DEBytes					2
#define fsk16RGB555DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB555DEKindFormat			16RGB555LE
# define fsk16RGB555LEFormatKind			16RGB555DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB555DEKindFormat			16RGB555BE
# define fsk16RGB555BEFormatKind			16RGB555DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGB5515SE											RRRRR GGGGG A BBBBB
 *														11111 10000 0 00000
 *														54321 09876 5 43210
 ********************************************************************************/

#define fsk16RGB5515SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB5515SEAlphaPosition			5	/* Little-endian bit offsets */
#define fsk16RGB5515SERedPosition			11
#define fsk16RGB5515SEGreenPosition			6
#define fsk16RGB5515SEBluePosition			0
#define fsk16RGB5515SEAlphaBits				1
#define fsk16RGB5515SERedBits				5
#define fsk16RGB5515SEGreenBits				5
#define fsk16RGB5515SEBlueBits				5
#define fsk16RGB5515SEBytes					2
#define Fsk16RGB5515SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB5515SEKindFormat			16RGB5515BE
# define fsk16RGB5515BEFormatKind			16RGB5515SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB5515SEKindFormat			16RGB5515LE
# define fsk16RGB5515LEFormatKind			16RGB5515SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGB5515DE											gg A BBBBB RRRRR GGG
 *														11 1 11100 00000 000
 *														54 3 21098 76543 210
 ********************************************************************************/

#define fsk16RGB5515DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGB5515DEAlphaPosition			13	/* Little-endian bit offsets */
#define fsk16RGB5515DERedPosition			3
#define fsk16RGB5515DEGreenPosition			14
#define fsk16RGB5515DEBluePosition			8
#define fsk16RGB5515DEAlphaBits				1
#define fsk16RGB5515DERedBits				5
#define fsk16RGB5515DEGreenBits				5
#define fsk16RGB5515DEBlueBits				5
#define fsk16RGB5515DEBytes					2
#define Fsk16RGB5515DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGB5515DEKindFormat			16RGB5515LE
# define fsk16RGB5515LEFormatKind			16RGB5515DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGB5515DEKindFormat			16RGB5515BE
# define fsk16RGB5515BEFormatKind			16RGB5515DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16BGR5515SE											BBBBB GGGGG A RRRRR
 *														11111 10000 0 00000
 *														54321 09876 5 43210
 ********************************************************************************/

#define fsk16BGR5515SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16BGR5515SEAlphaPosition			5	/* Little-endian bit offsets */
#define fsk16BGR5515SERedPosition			0
#define fsk16BGR5515SEGreenPosition			6
#define fsk16BGR5515SEBluePosition			11
#define fsk16BGR5515SEAlphaBits				1
#define fsk16BGR5515SERedBits				5
#define fsk16BGR5515SEGreenBits				5
#define fsk16BGR5515SEBlueBits				5
#define fsk16BGR5515SEBytes					2
#define Fsk16BGR5515SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16BGR5515SEKindFormat			16BGR5515BE
# define fsk16BGR5515BEFormatKind			16BGR5515SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16BGR5515SEKindFormat			16BGR5515LE
# define fsk16BGR5515LEBitmapFormatKind		16BGR5515SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16BGR5515DE											gg A RRRRR BBBBB GGG
 *														11 1 11100 00000 000
 *														54 3 21098 76543 210
 ********************************************************************************/

#define fsk16BGR5515DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16BGR5515DEAlphaPosition			13	/* Little-endian bit offsets */
#define fsk16BGR5515DERedPosition			8
#define fsk16BGR5515DEGreenPosition			14
#define fsk16BGR5515DEBluePosition			3
#define fsk16BGR5515DEAlphaBits				1
#define fsk16BGR5515DERedBits				5
#define fsk16BGR5515DEGreenBits				5
#define fsk16BGR5515DEBlueBits				5
#define fsk16BGR5515DEBytes					2
#define Fsk16BGR5515DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16BGR5515DEKindFormat			16BGR5515LE
# define fsk16BGR5515LEFormatKind			16BGR5515DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16BGR5515DEKindFormat			16BGR5515BE
# define fsk16BGR5515BEFormatKind			16BGR5515DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGBA4444SE											RRRR GGGG BBBB AAAA
 *														1111 1100 0000 0000
 *														5432 1098 7654 3210
 ********************************************************************************/

#define fsk16RGBA4444SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGBA4444SEAlphaPosition		0	/* Little-endian bit offsets */
#define fsk16RGBA4444SERedPosition			12
#define fsk16RGBA4444SEGreenPosition		8
#define fsk16RGBA4444SEBluePosition			4
#define fsk16RGBA4444SEAlphaBits			4
#define fsk16RGBA4444SERedBits				4
#define fsk16RGBA4444SEGreenBits			4
#define fsk16RGBA4444SEBlueBits				4
#define fsk16RGBA4444SEBytes				2
#define Fsk16RGBA4444SEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGBA4444SEKindFormat			16RGBA4444BE
# define fsk16RGBA4444BEFormatKind			16RGBA4444SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGBA4444SEKindFormat			16RGBA4444LE
# define fsk16RGBA4444LEFormatKind			16RGBA4444SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16RGBA4444DE											BBBB AAAA RRRR GGGG
 *														1111 1100 0000 0000
 *														5432 1098 7654 3210
 ********************************************************************************/

#define fsk16RGBA4444DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk16RGBA4444DEAlphaPosition		8	/* Little-endian bit offsets */
#define fsk16RGBA4444DERedPosition			4
#define fsk16RGBA4444DEGreenPosition		0
#define fsk16RGBA4444DEBluePosition			12
#define fsk16RGBA4444DEAlphaBits			4
#define fsk16RGBA4444DERedBits				4
#define fsk16RGBA4444DEGreenBits			4
#define fsk16RGBA4444DEBlueBits				4
#define fsk16RGBA4444DEBytes				2
#define Fsk16RGBA4444DEType					UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16RGBA4444DEKindFormat			16RGBA4444LE
# define fsk16RGBA4444LEFormatKind			16RGBA4444DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16RGBA4444DEKindFormat			16RGBA4444BE
# define fsk16RGBA4444BEFormatKind			16RGBA4444DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16AG	- 8 bit alpha + 8 bit grayscale
 ********************************************************************************/

#define fsk16AGPixelPacking				fskUniformChunkyPixelPacking
#define fsk16AGAlphaPosition			8
#define fsk16AGGrayPosition				0
#define fsk16AGAlphaBits				8
#define fsk16AGGrayBits					8
#define fsk16AGBytes					2
#define Fsk16AGType						UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16AGKindFormat				16AG
# define fsk16AGFormatKind				16AG
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16AGKindFormat				16GA
# define fsk16GAFormatKind				16AG
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 16GA	- 8 bit grayscale + 8 bit alpha
 ********************************************************************************/

#define fsk16GAPixelPacking				fskUniformChunkyPixelPacking
#define fsk16GAAlphaPosition			0
#define fsk16GAGrayPosition				8
#define fsk16GAAlphaBits				8
#define fsk16GAGrayBits					8
#define fsk16GABytes					2
#define Fsk16GAType						UInt16
#if TARGET_RT_BIG_ENDIAN
# define fsk16GAKindFormat				16GA
# define fsk16GAFormatKind				16GA
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk16GAKindFormat				16AG
# define fsk16AGFormatKind				16GA
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 8G	- 8 bit grayscale
 ********************************************************************************/

#define fsk8GPixelPacking			fskUniformChunkyPixelPacking
#define fsk8GAlphaBits				0
#define fsk8GGrayBits				8
#define fsk8GBytes					1
#define Fsk8GType					UInt8
#define fsk8GKindFormat				8G
#define fsk8GFormatKind				8G


/********************************************************************************
 * 8A	- 8 bit alpha
 ********************************************************************************/

#define fsk8APixelPacking			fskUniformChunkyPixelPacking
#define fsk8AAlphaBits				8
#define fsk8ABytes					1
#define Fsk8AType					UInt8
#define fsk8AKindFormat				8A
#define fsk8AFormatKind				8A


/********************************************************************************
 * 32A16RGB565SE											AAAAAAAA xxxxxxxx RRRRR GGGGGG BBBBB
 *															                  11111 100000 00000
 *															                  54321 098765 43210
 ********************************************************************************/

#define fsk32A16RGB565SEPixelPacking			fskUniformChunkyPixelPacking
#define fsk32A16RGB565SEAlphaPosition			24	/* Little-endian bit offsets */
#define fsk32A16RGB565SERedPosition				11
#define fsk32A16RGB565SEGreenPosition			5
#define fsk32A16RGB565SEBluePosition			0
#define fsk32A16RGB565SEAlphaBits				8
#define fsk32A16RGB565SERedBits					5
#define fsk32A16RGB565SEGreenBits				6
#define fsk32A16RGB565SEBlueBits				5
#define fsk32A16RGB565SEBytes					4
#define Fsk32A16RGB565SEType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32A16RGB565SEKindFormat			32A16RGB565BE
# define fsk32A16RGB565BEFormatKind			32A16RGB565SE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32A16RGB565SEKindFormat			32A16RGB565LE
# define fsk32A16RGB565LEFormatKind			32A16RGB565SE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * 32A16RGB565DE										ggg BBBBB RRRRR GGG xxxxxxxx AAAAAAAA
 *														332 22222 22221 111 11111100 00000000
 *														109 87654 32109 876 54321098 76543210
 ********************************************************************************/

#define fsk32A16RGB565DEPixelPacking			fskUniformChunkyPixelPacking
#define fsk32A16RGB565DEAlphaPosition			0	/* Little-endian bit offsets */
#define fsk32A16RGB565DERedPosition				19
#define fsk32A16RGB565DEGreenPosition			29	/* Need to be careful with this, because green is split */
#define fsk32A16RGB565DEBluePosition			24
#define fsk32A16RGB565DEAlphaBits				8
#define fsk32A16RGB565DERedBits					5
#define fsk32A16RGB565DEGreenBits				6
#define fsk32A16RGB565DEBlueBits				5
#define fsk32A16RGB565DEBytes					4
#define Fsk32A16RGB565DEType					UInt32
#if TARGET_RT_BIG_ENDIAN
# define fsk32A16RGB565DEKindFormat			32A16RGB565LE
# define fsk32A16RGB565LEFormatKind			32A16RGB565DE
#else /* TARGET_RT_LITTLE_ENDIAN */
# define fsk32A16RGB565DEKindFormat			32A16RGB565BE
# define fsk32A16RGB565BEFormatKind			32A16RGB565DE
#endif /* TARGET_RT_LITTLE_ENDIAN */


/********************************************************************************
 * Color to Gray
 ********************************************************************************/

#define LUMA601
#ifdef LUMA601				/* CCIR 601 & JPEG/JFIF Luminance */
	#define fskRtoYCoeff		306		/* .299 */
	#define fskGtoYCoeff		601		/* .587 */
	#define fskBtoYCoeff		117		/* .114 */
#elif defined(LUMA709)		/* 709 */
	#define fskRtoYCoeff		218		/* .2126 */
	#define fskGtoYCoeff		732		/* .7152 */
	#define fskBtoYCoeff		74		/* .0722 */
#else
	#error Please choose a standard for luminance.
#endif
#define fskToYCoeffShift	10

/* Note that the following has a different form than the other converters, because the source is not a pixel proper */
#define fskConvertRGBtoLuminance(r, g, b, lum)	( lum =	(	fskRtoYCoeff * (r)				\
														+	fskGtoYCoeff * (g)				\
														+	fskBtoYCoeff * (b)				\
														+	(1 << (fskToYCoeffShift - 1))	\
														) >> fskToYCoeffShift				\
												)
#define fskConvertRGBtoGray(r, g, b, gray) fskConvertRGBtoLuminance(r, g, b, gray)


#define fskIsDifferentEndian(t)		(FskName3(fsk,t,GreenPosition) == 13)



/********************************************************************************
 * YUV420 - planar pixels
 *
 * | Y |     1  |  2097   4128    802 |  | R |   |  16 |
 * | U | = ____ | -1212  -2383   3596 |  | G | + | 128 |
 * | V |   8192 |  3596   3014   -581 |  | B |   | 128 |
 *
 *
 * | R |    1   | 9539      0  13075 | /  | Y |   |  16 |  \
 * | G | = ____ | 9539	-3209  -6660 | |  | U | - | 128 |   |
 * | B |   8192 | 9539  16525      0 | \  | V |   | 128 |  /
 *
 *
 * | R |    1   | 1192    0   1634 | /  | Y |   |  16 |  \
 * | G | = ____ | 1192  -401  -832 | |  | U | - | 128 |   |
 * | B |   1024 | 1192  2066     0 | \  | V |   | 128 |  /
 *
 *
 ********************************************************************************/


/********************************************************************************
 * YUV420 (4:2:0 planar, ordered { [Y], [Cb], [Cr] } )
 ********************************************************************************/

#define fskYUV420PixelPacking		fskPlanarPixelPacking
#define fskYUV420AlphaBits			0
#define fskYUV420GrayBits			8
#define fskYUV420KindFormat			YUV420
#define fskYUV420FormatKind			YUV420

/********************************************************************************
 * YUV420i (4:2:0 interleaved, ordered { U, V, Y00, Y01, Y10, Y11 } )
 ********************************************************************************/

#define fskYUV420iPixelPacking		fskInterleavePixelPacking
#define fskYUV420iAlphaBits			0
#define fskYUV420iGrayBits			8
#define fskYUV420iKindFormat		YUV420i
#define fskYUV420iFormatKind		YUV420i

/********************************************************************************
 * YUV420spuv (4:2:0 semi-planar, ordered { [Y], [Cb, Cr] } )
 ********************************************************************************/

#define fskYUV420spuvPixelPacking	fskPlanarPixelPacking
#define fskYUV420spuvAlphaBits		0
#define fskYUV420spuvGrayBits		8
#define fskYUV420spuvKindFormat		YUV420spuv
#define fskYUV420spuvFormatKind		YUV420spuv

/********************************************************************************
 * YUV420spvu (4:2:0 semi-planar, ordered { [Y], [Cr, Cb] } )
 ********************************************************************************/

#define fskYUV420spvuPixelPacking	fskPlanarPixelPacking
#define fskYUV420spvuAlphaBits		0
#define fskYUV420spvuGrayBits		8
#define fskYUV420spvuKindFormat		YUV420spvu
#define fskYUV420spvuFormatKind		YUV420spvu


/********************************************************************************
 * YUV422 (4:2:2 planar, ordered {Y, Cb, Cr} )
 ********************************************************************************/

#define fskYUV422PixelPacking		fskPlanarPixelPacking
#define fskYUV422AlphaBits			0
#define fskYUV422FormatKind			YUV422
#define fskYUV422KindFormat			YUV422


/********************************************************************************
 * UYVY (4:2:2 chunky, ordered {Cb, Y, Cr, Y}
 ********************************************************************************/

#define fskUYVYPixelPacking			fskNonUniformChunkyPixelPacking
#define fskUYVYAlphaBits			0
#define fskUYVYFormatKind			UYVY
#define fskUYVYKindFormat			UYVY


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***	Macros
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#define FskMoveField(src, width, from, to)	((((UInt32)(src) >> (from)) & ((1 << (width)) - 1)) << (to))
#define fskFieldMask(width, position)		(((1 << (width)) - 1) << (position))

#define fskSwapEndian4Bytes(p)	( p = FskMoveField(p, 8,24, 0) | FskMoveField(p, 8,16, 8) | FskMoveField(p, 8, 8,16) | FskMoveField(p, 8, 0,24) )
#define fskSwapEndian2Bytes(p)	( p = FskMoveField(p, 8, 8, 0) | FskMoveField(p, 8, 0, 8) )


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***	Conversions
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/* Identity transformations */
#define fskConvert32ARGB32ARGB(p)
#define fskConvert32BGRA32BGRA(p)
#define fskConvert32ABGR32ABGR(p)
#define fskConvert32RGBA32RGBA(p)
#define fskConvert16RGB565SE16RGB565SE(p)
#define fskConvert16RGB565DE16RGB565DE(p)
#define fskConvert16BGR565SE16BGR565SE(p)
#define fskConvert16BGR565DE16BGR565DE(p)
#define fskConvert16RGB5515SE16RGB5515SE(p)
#define fskConvert16RGB5515DE16RGB5515DE(p)
#define fskConvert16BGR5515SE16BGR5515SE(p)
#define fskConvert16BGR5515DE16BGR5515DE(p)
#define fskConvert8G8G(p)
#define fskConvert16RGBA4444SE16RGBA4444SE(p)
#define fskConvert16RGBA4444DE16RGBA4444DE(p)

#define fskConvert32A16RGB565SE32A16RGB565SE(p)
#define fskConvert32A16RGB565DE32A16RGB565DE(p)
#define fskConvert32A16BGR565SE32A16BGR565SE(p)
#define fskConvert32A16BGR565DE32A16BGR565DE(p)
//#define fskConvert24RGB24RGB(p,q) defined below
//#define fskConvert24BGR24BGR(p,q) defined below



/********************************************************************************
 * 32 -> 32
 ********************************************************************************/


/* --> 32 BGRA */
#define fskConvert32ARGB32BGRA(p)		(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBRedPosition,   fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 8, fsk32ARGBGreenPosition, fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBBluePosition,  fsk32BGRABluePosition )	\
										)

#define fskConvert32ABGR32BGRA(p)		(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRRedPosition,   fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 8, fsk32ABGRGreenPosition, fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRBluePosition,  fsk32BGRABluePosition )	\
										)

#define fskConvert32RGBA32BGRA(p)		(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32RGBARedPosition,   fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 8, fsk32RGBAGreenPosition, fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32RGBABluePosition,  fsk32BGRABluePosition )	\
										)

#define fskConvert32A16RGB565SE32BGRA(p) \
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition, fsk32BGRAAlphaPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition,   8-5 + fsk32BGRARedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition, 8-6 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition,  8-5 + fsk32BGRABluePosition),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											                |	fskFieldMask(8-5, fsk32BGRABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32BGRAGreenPosition))						\
										)

#define fskConvert32A16RGB565DE32BGRA(p) \
										(	fskConvert32A16RGB565DE32A16RGB565SE(p),	\
											fskConvert32A16RGB565SE32BGRA(p)			\
										)

/* --> 32 ARGB */
#define fskConvert32BGRA32ARGB(p)		(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32BGRARedPosition,   fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 8, fsk32BGRAGreenPosition, fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 8, fsk32BGRABluePosition,  fsk32ARGBBluePosition )	\
										)

#define fskConvert32ABGR32ARGB(p)		(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRRedPosition,   fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 8, fsk32ABGRGreenPosition, fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRBluePosition,  fsk32ARGBBluePosition )	\
										)

#define fskConvert32RGBA32ARGB(p)		(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32RGBARedPosition,   fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 8, fsk32RGBAGreenPosition, fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 8, fsk32RGBABluePosition,  fsk32ARGBBluePosition )	\
										)

#define fskConvert32A16RGB565SE32ARGB(p) \
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition, fsk32ARGBAlphaPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition,   8-5 + fsk32ARGBRedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition, 8-6 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition,  8-5 + fsk32ARGBBluePosition),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											                |	fskFieldMask(8-5, fsk32ARGBBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ARGBGreenPosition))						\
										)

#define fskConvert32A16RGB565DE32ARGB(p)	(fskConvert32A16RGB565DE32A16RGB565SE(p),	\
											 fskConvert32A16RGB565SE32ARGB(p)			\
											)

/* --> 32 RGBA */
#define fskConvert32ARGB32RGBA(p)		(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBRedPosition,   fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 8, fsk32ARGBGreenPosition, fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBBluePosition,  fsk32RGBABluePosition )	\
										)

#define fskConvert32BGRA32RGBA(p)		(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32BGRARedPosition,   fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 8, fsk32BGRAGreenPosition, fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32BGRABluePosition,  fsk32RGBABluePosition )	\
										)

#define fskConvert32ABGR32RGBA(p)		(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRRedPosition,   fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 8, fsk32ABGRGreenPosition, fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ABGRBluePosition,  fsk32RGBABluePosition )	\
										)

#define fskConvert32A16RGB565SE32RGBA(p) \
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition, fsk32RGBAAlphaPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition,   8-5 + fsk32RGBARedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition, 8-6 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition,  8-5 + fsk32RGBABluePosition),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											                |	fskFieldMask(8-5, fsk32RGBABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32RGBAGreenPosition))						\
										)

#define fskConvert32A16RGB565DE32RGBA(p)	(fskConvert32A16RGB565DE32A16RGB565SE(p),	\
											 fskConvert32A16RGB565SE32RGBA(p)			\
											)

/* --> 32 ABGR */
#define fskConvert32ARGB32ABGR(p)		(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBRedPosition,   fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 8, fsk32ARGBGreenPosition, fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 8, fsk32ARGBBluePosition,  fsk32ABGRBluePosition )	\
										)

#define fskConvert32BGRA32ABGR(p)		(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32BGRARedPosition,   fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 8, fsk32BGRAGreenPosition, fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 8, fsk32BGRABluePosition,  fsk32ABGRBluePosition )	\
										)

#define fskConvert32RGBA32ABGR(p)		(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 8, fsk32RGBARedPosition,   fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 8, fsk32RGBAGreenPosition, fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 8, fsk32RGBABluePosition,  fsk32ABGRBluePosition )	\
										)

#define fskConvert32A16RGB565SE32ABGR(p) \
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition, fsk32ABGRAlphaPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition,   8-5 + fsk32ABGRRedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition, 8-6 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition,  8-5 + fsk32ABGRBluePosition),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											                |	fskFieldMask(8-5, fsk32ABGRBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ABGRGreenPosition))						\
										)

#define fskConvert32A16RGB565DE32ABGR(p)	(fskConvert32A16RGB565DE32A16RGB565SE(p),	\
											 fskConvert32A16RGB565SE32ABGR(p)			\
											)

/* --> 32 A 16 RGB 565 SE */
#define fskConvert32ARGB32A16RGB565SE(p)\
										(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition,     fsk32A16RGB565SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBRedPosition + 3,   fsk32A16RGB565SERedPosition  )	\
											|	FskMoveField(p, 6, fsk32ARGBGreenPosition + 2, fsk32A16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition + 3,  fsk32A16RGB565SEBluePosition )	\
										)

#define fskConvert32ABGR32A16RGB565SE(p)\
										(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition,     fsk32A16RGB565SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRRedPosition + 3,   fsk32A16RGB565SERedPosition  )	\
											|	FskMoveField(p, 6, fsk32ABGRGreenPosition + 2, fsk32A16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition + 3,  fsk32A16RGB565SEBluePosition )	\
										)

#define fskConvert32BGRA32A16RGB565SE(p)\
										(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition,     fsk32A16RGB565SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32BGRARedPosition + 3,   fsk32A16RGB565SERedPosition  )	\
											|	FskMoveField(p, 6, fsk32BGRAGreenPosition + 2, fsk32A16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition + 3,  fsk32A16RGB565SEBluePosition )	\
										)

#define fskConvert32RGBA32A16RGB565SE(p)\
										(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition,     fsk32A16RGB565SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32RGBARedPosition + 3,   fsk32A16RGB565SERedPosition  )	\
											|	FskMoveField(p, 6, fsk32RGBAGreenPosition + 2, fsk32A16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition + 3,  fsk32A16RGB565SEBluePosition )	\
										)

#define fskConvert32A16RGB565DE32A16RGB565SE(p)	(p = (((p) >> 8) & 0x0000FF00) | ((UInt32)(p) << 24) | ((UInt32)(p) >> 24))	/* gggBBBBBRRRRRGGG00000000AAAAAAAA --> AAAAAAAA00000000RRRRRGGGgggBBBBB */

/* --> 32 A 16 RGB 565 DE */
#define fskConvert32ARGB32A16RGB565DE(p)	(fskConvert32ARGB32A16RGB565SE(p),			\
											 fskConvert32A16RGB565SE32A16RGB565DE(p)	\
											)
#define fskConvert32ABGR32A16RGB565DE(p)	(fskConvert32ABGR32A16RGB565SE(p),			\
											 fskConvert32A16RGB565SE32A16RGB565DE(p)	\
											)
#define fskConvert32BGRA32A16RGB565DE(p)	(fskConvert32BGRA32A16RGB565SE(p),			\
											 fskConvert32A16RGB565SE32A16RGB565DE(p)	\
											)
#define fskConvert32RGBA32A16RGB565DE(p)	(fskConvert32RGBA32A16RGB565SE(p),			\
											 fskConvert32A16RGB565SE32A16RGB565DE(p)	\
											)

#define fskConvert32A16RGB565SE32A16RGB565DE(p)	(p = (((p) << 8) & 0x00FF0000) | ((UInt32)(p) << 24) | ((UInt32)(p) >> 24))	/* AAAAAAAA00000000RRRRRGGGgggBBBBB --> gggBBBBBRRRRRGGG00000000AAAAAAAA */


/********************************************************************************
 * 32 -> 24
 ********************************************************************************/


/* --> 24 RGB */
#define fskConvert32ARGB24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)((p) >> fsk32ARGBRedPosition),		\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)((p) >> fsk32ARGBGreenPosition),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)((p) >> fsk32ARGBBluePosition)		\
										)

#define fskConvert32BGRA24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)((p) >> fsk32BGRARedPosition),		\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)((p) >> fsk32BGRAGreenPosition),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)((p) >> fsk32BGRABluePosition)		\
										)

#define fskConvert32ABGR24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)((p) >> fsk32ABGRRedPosition),		\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)((p) >> fsk32ABGRGreenPosition),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)((p) >> fsk32ABGRBluePosition)		\
										)

#define fskConvert32RGBA24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)((p) >> fsk32RGBARedPosition),		\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)((p) >> fsk32RGBAGreenPosition),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)((p) >> fsk32RGBABluePosition)		\
										)

#define fskConvert32A16RGB565SE24RGB(p,q)\
										(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)((p) >> fsk32A16RGB565SERedPosition) << 3,		\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)((p) >> fsk32A16RGB565SEGreenPosition) << 2,	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)((p) >> fsk32A16RGB565SEBluePosition) << 3		\
										)


/* --> 24 BGR */
#define fskConvert32ARGB24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)((p) >> fsk32ARGBRedPosition),		\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)((p) >> fsk32ARGBGreenPosition),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)((p) >> fsk32ARGBBluePosition)		\
										)

#define fskConvert32BGRA24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)((p) >> fsk32BGRARedPosition),		\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)((p) >> fsk32BGRAGreenPosition),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)((p) >> fsk32BGRABluePosition)		\
										)

#define fskConvert32ABGR24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)((p) >> fsk32ABGRRedPosition),		\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)((p) >> fsk32ABGRGreenPosition),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)((p) >> fsk32ABGRBluePosition)		\
										)

#define fskConvert32RGBA24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)((p) >> fsk32RGBARedPosition),		\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)((p) >> fsk32RGBAGreenPosition),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)((p) >> fsk32RGBABluePosition)		\
										)

#define fskConvert32A16RGB565SE24BGR(p,q)\
										(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)((p) >> fsk32A16RGB565SERedPosition) << 3,		\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)((p) >> fsk32A16RGB565SEGreenPosition) << 2,	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)((p) >> fsk32A16RGB565SEBluePosition) << 3		\
										)


/********************************************************************************
 * 32 -> 16
 ********************************************************************************/


/* --> 16 RGB 565 SE */
#define fskConvert32ARGB16RGB565SE(p)	(p =	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32ARGBGreenPosition+2, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16RGB565SEBluePosition)	\
										)

#define fskConvert32BGRA16RGB565SE(p)	(p =	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32BGRAGreenPosition+2, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16RGB565SEBluePosition)	\
										)

#define fskConvert32ABGR16RGB565SE(p)	(p =	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32ABGRGreenPosition+2, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16RGB565SEBluePosition)	\
										)

#define fskConvert32RGBA16RGB565SE(p)	(p =	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32RGBAGreenPosition+2, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16RGB565SEBluePosition)	\
										)

#define fskConvert32A16RGB565SE16RGB565SE(p)	/* identity */

/* --> 16 RGB 565 DE */
#define fskConvert32ARGB16RGB565DE(p)	(p =	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32ARGBGreenPosition+2, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32BGRA16RGB565DE(p)	(p =	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32BGRAGreenPosition+2, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32ABGR16RGB565DE(p)	(p =	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32ABGRGreenPosition+2, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32RGBA16RGB565DE(p)	(p =	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32RGBAGreenPosition+2, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32A16RGB565SE16RGB565DE(p) (p = FskMoveField(p, 8, 8, 0) | FskMoveField(p, 8, 0, 8))

/* --> 16 BGR 565 SE */
#define fskConvert32ARGB16BGR565SE(p)	(p =	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32ARGBGreenPosition+2, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16BGR565SEBluePosition)	\
										)

#define fskConvert32BGRA16BGR565SE(p)	(p =	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32BGRAGreenPosition+2, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16BGR565SEBluePosition)	\
										)

#define fskConvert32ABGR16BGR565SE(p)	(p =	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32ABGRGreenPosition+2, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16BGR565SEBluePosition)	\
										)

#define fskConvert32RGBA16BGR565SE(p)	(p =	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32RGBAGreenPosition+2, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16BGR565SEBluePosition)	\
										)

#define fskConvert32A16RGB565SE16BGR565SE(p)\
										(p =	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  +3,   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition+2, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition +3,  fsk16BGR565SEBluePosition)	\
										)


/* --> 16 BGR 565 DE */
#define fskConvert32ARGB16BGR565DE(p)	(p =	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32ARGBGreenPosition+2, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32BGRA16BGR565DE(p)	(p =	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32BGRAGreenPosition+2, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32ABGR16BGR565DE(p)	(p =	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32ABGRGreenPosition+2, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32RGBA16BGR565DE(p)	(p =	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32RGBAGreenPosition+2, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32A16RGB565RGBA16BGR565DE(p)	\
										(p =	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  +3,   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition+2, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition +3,  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)


/* --> 16 RGB 5515 SE */
#define fskConvert32ARGB16RGB5515SE(p)	(p =	FskMoveField(p, 1, fsk32ARGBAlphaPosition+7, fsk16RGB5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBGreenPosition+3, fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16RGB5515SEBluePosition)	\
										)

#define fskConvert32BGRA16RGB5515SE(p)	(p =	FskMoveField(p, 1, fsk32BGRAAlphaPosition+7, fsk16RGB5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32BGRAGreenPosition+3, fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16RGB5515SEBluePosition)	\
										)

#define fskConvert32ABGR16RGB5515SE(p)	(p =	FskMoveField(p, 1, fsk32ABGRAlphaPosition+7, fsk16RGB5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRGreenPosition+3, fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16RGB5515SEBluePosition)	\
										)

#define fskConvert32RGBA16RGB5515SE(p)	(p =	FskMoveField(p, 1, fsk32RGBAAlphaPosition+7, fsk16RGB5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32RGBAGreenPosition+3, fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16RGB5515SEBluePosition)	\
										)

#define fskConvert32A16RGB565SE16RGB5515SE(p)	\
										(p =	FskMoveField(p, 1, fsk32A16RGB565SEAlphaPosition+7, fsk16RGB5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  +3,   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEGreenPosition+3, fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition +3,  fsk16RGB5515SEBluePosition)	\
										)


/* --> 16 RGB 5515 DE */
#define fskConvert32ARGB16RGB5515DE(p)	(p =	FskMoveField(p, 1, fsk32ARGBAlphaPosition+7, fsk16RGB5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBGreenPosition+3, fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32BGRA16RGB5515DE(p)	(p =	FskMoveField(p, 1, fsk32BGRAAlphaPosition+7, fsk16RGB5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32BGRAGreenPosition+3, fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32ABGR16RGB5515DE(p)	(p =	FskMoveField(p, 1, fsk32ABGRAlphaPosition+7, fsk16RGB5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRGreenPosition+3, fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32RGBA16RGB5515DE(p)	(p =	FskMoveField(p, 1, fsk32RGBAAlphaPosition+7, fsk16RGB5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32RGBAGreenPosition+3, fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32A16RGB565SE16RGB5515DE(p)	\
										(p =	FskMoveField(p, 1, fsk32A16RGB565SEAlphaPosition+7, fsk16RGB5515DEAlphaPosition)\
											|	FskMoveField(p, 5, fsk32A16RGB565SEPosition  +3,   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEPosition+3, fsk16RGB5515DEGreenPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SEPosition +3,  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */													\
										)


/* --> 16 BGR 5515 SE */
#define fskConvert32ARGB16BGR5515SE(p)	(p =	FskMoveField(p, 1, fsk32ARGBAlphaPosition+7, fsk16BGR5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBGreenPosition+3, fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16BGR5515SEBluePosition)	\
										)

#define fskConvert32BGRA16BGR5515SE(p)	(p =	FskMoveField(p, 1, fsk32BGRAAlphaPosition+7, fsk16BGR5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32BGRAGreenPosition+3, fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16BGR5515SEBluePosition)	\
										)

#define fskConvert32ABGR16BGR5515SE(p)	(p =	FskMoveField(p, 1, fsk32ABGRAlphaPosition+7, fsk16BGR5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRGreenPosition+3, fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16BGR5515SEBluePosition)	\
										)

#define fskConvert32RGBA16BGR5515SE(p)	(p =	FskMoveField(p, 1, fsk32RGBAAlphaPosition+7, fsk16BGR5515SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32RGBAGreenPosition+3, fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16BGR5515SEBluePosition)	\
										)

#define fskConvert32A16RGB565SERGBA16BGR5515SE(p)	\
										(p =	FskMoveField(p, 1, fsk32A16RGB565SEAlphaPosition+7, fsk16BGR5515SEAlphaPosition)		\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  +3,   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEGreenPosition+3, fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition +3,  fsk16BGR5515SEBluePosition)	\
										)


/* --> 16 BGR 5515 DE */
#define fskConvert32ARGB16BGR5515DE(p)	(p =	FskMoveField(p, 1, fsk32ARGBAlphaPosition+7, fsk16BGR5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBRedPosition  +3,   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBGreenPosition+3, fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ARGBBluePosition +3,  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32BGRA16BGR5515DE(p)	(p =	FskMoveField(p, 1, fsk32BGRAAlphaPosition+7, fsk16BGR5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32BGRARedPosition  +3,   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32BGRAGreenPosition+3, fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32BGRABluePosition +3,  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32ABGR16BGR5515DE(p)	(p =	FskMoveField(p, 1, fsk32ABGRAlphaPosition+7, fsk16BGR5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRRedPosition  +3,   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRGreenPosition+3, fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32ABGRBluePosition +3,  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\
										)

#define fskConvert32RGBA16BGR5515DE(p)	(p =	FskMoveField(p, 1, fsk32RGBAAlphaPosition+7, fsk16BGR5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32RGBARedPosition  +3,   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32RGBAGreenPosition+3, fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32RGBABluePosition +3,  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */												\

#define fskConvert32A16RGB565SE16BGR5515DE(p)	\
										(p =	FskMoveField(p, 1, fsk32A16RGB565SEAlphaPosition+7, fsk16BGR5515DEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  +3,   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEGreenPosition+3, fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition +3,  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */														\


/* --> 16 RGBA 4444 SE */
#define fskConvert32ARGB16RGBA4444SE(p)	(p =	FskMoveField(p, 4, fsk32ARGBAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBRedPosition  +4,   fsk16RGBA4444SERedPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBGreenPosition+4, fsk16RGBA4444SEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBBluePosition +4,  fsk16RGBA4444SEBluePosition)	\
										)

#define fskConvert32BGRA16RGBA4444SE(p)	(p =	FskMoveField(p, 4, fsk32BGRAAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32BGRARedPosition  +4,   fsk16RGBA4444SERedPosition)	\
											|	FskMoveField(p, 4, fsk32BGRAGreenPosition+4, fsk16RGBA4444SEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32BGRABluePosition +4,  fsk16RGBA4444SEBluePosition)	\
										)

#define fskConvert32ABGR16RGBA4444SE(p)	(p =	FskMoveField(p, 4, fsk32ABGRAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRRedPosition  +4,   fsk16RGBA4444SERedPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRGreenPosition+4, fsk16RGBA4444SEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRBluePosition +4,  fsk16RGBA4444SEBluePosition)	\
										)

#define fskConvert32RGBA16RGBA4444SE(p)	(p =	FskMoveField(p, 4, fsk32RGBAAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32RGBARedPosition  +4,   fsk16RGBA4444SERedPosition)	\
											|	FskMoveField(p, 4, fsk32RGBAGreenPosition+4, fsk16RGBA4444SEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32RGBABluePosition +4,  fsk16RGBA4444SEBluePosition)	\
										)

#define fskConvert32A16RGB565SE16RGBA4444SE(p)	\
										(p =	FskMoveField(p, 4, fsk32A16RGB565SEAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SERedPosition  +4,   fsk16RGBA4444SERedPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SEGreenPosition+4, fsk16RGBA4444SEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SEBluePosition +4,  fsk16RGBA4444SEBluePosition)	\
										)


/* --> 16 RGBA 4444 DE */
#define fskConvert32ARGB16RGBA4444DE(p)	(p =	FskMoveField(p, 4, fsk32ARGBAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBRedPosition  +4,   fsk16RGBA4444DERedPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBGreenPosition+4, fsk16RGBA4444DEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32ARGBBluePosition +4,  fsk16RGBA4444DEBluePosition)	\
										)

#define fskConvert32BGRA16RGBA4444DE(p)	(p =	FskMoveField(p, 4, fsk32BGRAAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32BGRARedPosition  +4,   fsk16RGBA4444DERedPosition)	\
											|	FskMoveField(p, 4, fsk32BGRAGreenPosition+4, fsk16RGBA4444DEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32BGRABluePosition +4,  fsk16RGBA4444DEBluePosition)	\
										)

#define fskConvert32ABGR16RGBA4444DE(p)	(p =	FskMoveField(p, 4, fsk32ABGRAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRRedPosition  +4,   fsk16RGBA4444DERedPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRGreenPosition+4, fsk16RGBA4444DEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32ABGRBluePosition +4,  fsk16RGBA4444DEBluePosition)	\
										)

#define fskConvert32RGBA16RGBA4444DE(p)	(p =	FskMoveField(p, 4, fsk32RGBAAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32RGBARedPosition  +4,   fsk16RGBA4444DERedPosition)	\
											|	FskMoveField(p, 4, fsk32RGBAGreenPosition+4, fsk16RGBA4444DEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32RGBABluePosition +4,  fsk16RGBA4444DEBluePosition)	\
										)

#define fskConvert32A16RGB565SE16RGBA4444DE(p)	\
										(p =	FskMoveField(p, 4, fsk32A16RGB565SEAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SERedPosition  +4,   fsk16RGBA4444DERedPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SEGreenPosition+4, fsk16RGBA4444DEGreenPosition)	\
											|	FskMoveField(p, 4, fsk32A16RGB565SEBluePosition +4,  fsk16RGBA4444DEBluePosition)	\
										)


/* --> 16AG */
#define fskConvert32ARGB16AG(p)			(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition,fsk16AGAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32ARGBRedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ARGBGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ARGBBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)						\
										)

#define fskConvert32BGRA16AG(p)			(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition,fsk16AGAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32BGRARedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32BGRAGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32BGRABluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)						\
										)

#define fskConvert32ABGR16AG(p)			(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition,fsk16AGAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32ABGRRedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ABGRGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ABGRBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)						\
										)

#define fskConvert32RGBA16AG(p)			(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition,fsk16AGAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32RGBARedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32RGBAGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32RGBABluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)						\
										)

#define fskConvert32A16RGB565SE16AG(p)		\
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition,fsk16AGAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32A16RGB565SERedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32A16RGB565SEGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32A16RGB565SEBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)						\
										)

/* --> 16GA */
#define fskConvert32ARGB16GA(p)			(p =	FskMoveField(p, 8, fsk32ARGBAlphaPosition,fsk16GAAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32ARGBRedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ARGBGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ARGBBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)						\
										)

#define fskConvert32BGRA16GA(p)			(p =	FskMoveField(p, 8, fsk32BGRAAlphaPosition,fsk16GAAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32BGRARedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32BGRAGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32BGRABluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)						\
										)

#define fskConvert32ABGR16GA(p)			(p =	FskMoveField(p, 8, fsk32ABGRAlphaPosition,fsk16GAAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32ABGRRedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ABGRGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32ABGRBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)						\
										)

#define fskConvert32RGBA16GA(p)			(p =	FskMoveField(p, 8, fsk32RGBAAlphaPosition,fsk16GAAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32RGBARedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32RGBAGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32RGBABluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)						\
										)

#define fskConvert32A16RGB565SE16GA(p)		\
										(p =	FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition,fsk16GAAlphaPosition)		\
											|	(((	FskMoveField(p, 8, fsk32A16RGB565SERedPosition  , 0) * fskRtoYCoeff	\
												+	FskMoveField(p, 8, fsk32A16RGB565SEGreenPosition, 0) * fskGtoYCoeff	\
												+	FskMoveField(p, 8, fsk32A16RGB565SEBluePosition , 0) * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)						\
										)


/********************************************************************************
 * 32 -> 8
 ********************************************************************************/


/* --> 8 gray */
#define fskConvert32ARGB8G(p)			(p = (UInt8)(															\
											(	FskMoveField(p, 8, fsk32ARGBRedPosition  , 0) * fskRtoYCoeff	\
											+	FskMoveField(p, 8, fsk32ARGBGreenPosition, 0) * fskGtoYCoeff	\
											+	FskMoveField(p, 8, fsk32ARGBBluePosition , 0) * fskBtoYCoeff	\
											+	(1 << (fskToYCoeffShift - 1))									\
											) >> fskToYCoeffShift)												\
										)

#define fskConvert32BGRA8G(p)			(p = (UInt8)(															\
											(	FskMoveField(p, 8, fsk32BGRARedPosition  , 0) * fskRtoYCoeff	\
											+	FskMoveField(p, 8, fsk32BGRAGreenPosition, 0) * fskGtoYCoeff	\
											+	FskMoveField(p, 8, fsk32BGRABluePosition , 0) * fskBtoYCoeff	\
											+	(1 << (fskToYCoeffShift - 1))									\
											) >> fskToYCoeffShift)												\
										)

#define fskConvert32ABGR8G(p)			(p = (UInt8)(															\
											(	FskMoveField(p, 8, fsk32ABGRRedPosition  , 0) * fskRtoYCoeff	\
											+	FskMoveField(p, 8, fsk32ABGRGreenPosition, 0) * fskGtoYCoeff	\
											+	FskMoveField(p, 8, fsk32ABGRBluePosition , 0) * fskBtoYCoeff	\
											+	(1 << (fskToYCoeffShift - 1))									\
											) >> fskToYCoeffShift)												\
										)

#define fskConvert32RGBA8G(p)			(p = (UInt8)(															\
											(	FskMoveField(p, 8, fsk32RGBARedPosition  , 0) * fskRtoYCoeff	\
											+	FskMoveField(p, 8, fsk32RGBAGreenPosition, 0) * fskGtoYCoeff	\
											+	FskMoveField(p, 8, fsk32RGBABluePosition , 0) * fskBtoYCoeff	\
											+	(1 << (fskToYCoeffShift - 1))									\
											) >> fskToYCoeffShift)												\
										)

#define fskConvert32A16RGB565SE8G(p)		\
										(p = (UInt8)(																\
											(	FskMoveField(p, 5, fsk32A16RGB565SERedPosition  , 0) * (fskRtoYCoeff * 255 / 31)	\
											+	FskMoveField(p, 6, fsk32A16RGB565SEGreenPosition, 0) * (fskGtoYCoeff * 255 / 63)	\
											+	FskMoveField(p, 5, fsk32A16RGB565SEBluePosition , 0) * (fskBtoYCoeff * 255 / 31)	\
											+	(1 << (fskToYCoeffShift - 1))										\
											) >> fskToYCoeffShift)													\
										)

/* --> 8 alpha */
#define fskConvert32ARGB8A(p)			(p = (UInt8)FskMoveField(p, 8, fsk32ARGBAlphaPosition, 0))
#define fskConvert32BGRA8A(p)			(p = (UInt8)FskMoveField(p, 8, fsk32BGRAAlphaPosition, 0))
#define fskConvert32ABGR8A(p)			(p = (UInt8)FskMoveField(p, 8, fsk32ABGRAlphaPosition, 0))
#define fskConvert32RGBA8A(p)			(p = (UInt8)FskMoveField(p, 8, fsk32RGBAAlphaPosition, 0))
#define fskConvert32A16RGB565SE8A(p)	(p = (UInt8)FskMoveField(p, 8, fsk32A16RGB565SEAlphaPosition, 0))



/********************************************************************************
 * 24 -> 32
 ********************************************************************************/


/* --> 32 ARGB */
#define fskConvert24RGB32ARGB(p,q)		(q =	(fskDefaultAlpha                               << fsk32ARGBAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] << fsk32ARGBRedPosition  )	\
											|	(((const UInt8*)(&(p)))[fsk24RGBGreenPosition] << fsk32ARGBGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBBluePosition ] << fsk32ARGBBluePosition )	\
										)

#define fskConvert24BGR32ARGB(p,q)		(q =	(fskDefaultAlpha                               << fsk32ARGBAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] << fsk32ARGBRedPosition)		\
											|	(((const UInt8*)(&(p)))[fsk24BGRGreenPosition] << fsk32ARGBGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRBluePosition ] << fsk32ARGBBluePosition)	\
										)

/* --> 32 BGRA */
#define fskConvert24RGB32BGRA(p,q)		(q =	(fskDefaultAlpha                               << fsk32BGRAAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] << fsk32BGRARedPosition  )	\
											|	(((const UInt8*)(&(p)))[fsk24RGBGreenPosition] << fsk32BGRAGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBBluePosition ] << fsk32BGRABluePosition )	\
										)

#define fskConvert24BGR32BGRA(p,q)		(q =	(fskDefaultAlpha                               << fsk32BGRAAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] << fsk32BGRARedPosition)		\
											|	(((const UInt8*)(&(p)))[fsk24BGRGreenPosition] << fsk32BGRAGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRBluePosition ] << fsk32BGRABluePosition)	\
										)

/* --> 32 ABGR */
#define fskConvert24RGB32ABGR(p,q)		(q =	(fskDefaultAlpha                               << fsk32ABGRAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] << fsk32ABGRRedPosition  )	\
											|	(((const UInt8*)(&(p)))[fsk24RGBGreenPosition] << fsk32ABGRGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBBluePosition ] << fsk32ABGRBluePosition )	\
										)

#define fskConvert24BGR32ABGR(p,q)		(q =	(fskDefaultAlpha                               << fsk32ABGRAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] << fsk32ABGRRedPosition)		\
											|	(((const UInt8*)(&(p)))[fsk24BGRGreenPosition] << fsk32ABGRGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRBluePosition ] << fsk32ABGRBluePosition)	\
										)

/* --> 32 RGBA */
#define fskConvert24RGB32RGBA(p,q)		(q =	(fskDefaultAlpha                               << fsk32RGBAAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] << fsk32RGBARedPosition  )	\
											|	(((const UInt8*)(&(p)))[fsk24RGBGreenPosition] << fsk32RGBAGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24RGBBluePosition ] << fsk32RGBABluePosition )	\
										)

#define fskConvert24BGR32RGBA(p,q)		(q =	(fskDefaultAlpha                               << fsk32RGBAAlphaPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] << fsk32RGBARedPosition)		\
											|	(((const UInt8*)(&(p)))[fsk24BGRGreenPosition] << fsk32RGBAGreenPosition)	\
											|	(((const UInt8*)(&(p)))[fsk24BGRBluePosition ] << fsk32RGBABluePosition)	\
										)

/* --> 32 A 16 RGB 565 SE */

#define fskConvert24RGB32A16RGB565SE(p,q)\
										(q =	(fskDefaultAlpha			                          << fsk32A16RGB565SEAlphaPosition)	\
											|	((((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] >> 3) << fsk32A16RGB565SERedPosition  )	\
											|	((((const UInt8*)(&(p)))[fsk24RGBGreenPosition] >> 2) << fsk32A16RGB565SEGreenPosition)	\
											|	((((const UInt8*)(&(p)))[fsk24RGBBluePosition ] >> 3) << fsk32A16RGB565SEBluePosition )	\
										)

#define fskConvert24BGR32A16RGB565SE(p,q)\
										(q =	(fskDefaultAlpha									  << fsk32A16RGB565SEAlphaPosition)	\
											|	((((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] >> 3) << fsk32A16RGB565SERedPosition)		\
											|	((((const UInt8*)(&(p)))[fsk24BGRGreenPosition] >> 2) << fsk32A16RGB565SEGreenPosition)	\
											|	((((const UInt8*)(&(p)))[fsk24BGRBluePosition ] >> 3) << fsk32A16RGB565SEBluePosition)		\
										)



/********************************************************************************
 * 24 -> 24
 ********************************************************************************/


/* --> 24 BGR */
#define fskConvert24RGB24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = ((const UInt8*)(&(p)))[fsk24RGBRedPosition  ],	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = ((const UInt8*)(&(p)))[fsk24RGBGreenPosition],	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = ((const UInt8*)(&(p)))[fsk24RGBBluePosition ]		\
										)

#define fskConvert24BGR24BGR(p,q)		(	((UInt8*)(&(q)))[0] = ((const UInt8*)(&(p)))[0],	\
											((UInt8*)(&(q)))[1] = ((const UInt8*)(&(p)))[1],	\
											((UInt8*)(&(q)))[2] = ((const UInt8*)(&(p)))[2]		\
										)

/* --> 24 RGB */
#define fskConvert24BGR24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = ((const UInt8*)(&(p)))[fsk24BGRRedPosition  ],	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = ((const UInt8*)(&(p)))[fsk24BGRGreenPosition],	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = ((const UInt8*)(&(p)))[fsk24BGRBluePosition ]		\
										)

#define fskConvert24RGB24RGB(p,q)		(	((UInt8*)(&(q)))[0] = ((const UInt8*)(&(p)))[0],	\
											((UInt8*)(&(q)))[1] = ((const UInt8*)(&(p)))[1],	\
											((UInt8*)(&(q)))[2] = ((const UInt8*)(&(p)))[2]		\
										)



/********************************************************************************
 * 24 -> 16
 ********************************************************************************/


/* --> 16 RGB 565 SE */
#define fskConvert24RGB16RGB565SE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16RGB565SERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 6, 8 - 6, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16RGB565SEBluePosition )	\
										)

#define fskConvert24BGR16RGB565SE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16RGB565SERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 6, 8 - 6, fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16RGB565SEBluePosition )	\
										)


/* --> 16 RGB 565 DE */
#define fskConvert24RGB16RGB565DE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16RGB565DERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 6, 8 - 6, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16RGB565DEBluePosition ),	\
											q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
										)

#define fskConvert24BGR16RGB565DE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16RGB565DERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 6, 8 - 6, fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16RGB565DEBluePosition ),	\
											q |= (q >> 16)	/* , q &= 0xFFFF; */																		\
										)

/* --> 16 BGR 565 SE */
#define fskConvert24RGB16BGR565SE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16BGR565SERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 6, 8 - 6, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16BGR565SEBluePosition )	\
										)

#define fskConvert24BGR16BGR565SE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16BGR565SERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 6, 8 - 6, fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16BGR565SEBluePosition )	\
										)

/* --> 16 BGR 565 DE */
#define fskConvert24RGB16BGR565DE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16BGR565DERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 6, 8 - 6, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16BGR565DEBluePosition ),	\
											q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
										)

#define fskConvert24BGR16BGR565DE(p, q)	(q =	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16BGR565DERedPosition  )	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 6, 8 - 6, fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16BGR565DEBluePosition ),	\
											q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
										)

/* --> 16 RGB 5515 SE */
#define fskConvert24RGB16RGB5515SE(p, q)	(q =	(fskDefaultAlpha1                                                  << fsk16RGB5515SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16RGB5515SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 5, 8 - 5, fsk16RGB5515SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16RGB5515SEBluePosition )	\
											)

#define fskConvert24BGR16RGB5515SE(p, q)	(q =	(fskDefaultAlpha1                                                  << fsk16RGB5515SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16RGB5515SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 5, 8 - 5, fsk16RGB5515SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16RGB5515SEBluePosition )	\
											)

/* --> 16 RGB 5515 DE */
#define fskConvert24RGB16RGB5515DE(p, q)	(q =	(fskDefaultAlpha1                                                  << fsk16RGB5515DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16RGB5515DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 5, 8 - 5, fsk16RGB5515DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16RGB5515DEBluePosition ),	\
												q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
											)

#define fskConvert24BGR16RGB5515DE(p, q)	(q =	(fskDefaultAlpha1                                                  << fsk16RGB5515DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16RGB5515DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 5, 8 - 5, fsk16RGB5515DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16RGB5515DEBluePosition ),	\
												q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
											)

/* --> 16 BGR 5515 SE */
#define fskConvert24RGB16BGR5515SE(p, q)	(q =	(fskDefaultAlpha1                                                   << fsk16BGR5515SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16BGR5515SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 5, 8 - 5, fsk16BGR5515SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16BGR5515SEBluePosition )	\
											)

#define fskConvert24BGR16BGR5515SE(p, q)	(q =	(fskDefaultAlpha1                                                   << fsk16BGR5515SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16BGR5515SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 5, 8 - 5, fsk16BGR5515SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16BGR5515SEBluePosition )	\
											)

/* --> 16 BGR 5515 DE */
#define fskConvert24RGB16BGR5515DE(p, q)	(q =	(fskDefaultAlpha1                                                   << fsk16BGR5515DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 5, 8 - 5, fsk16BGR5515DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 5, 8 - 5, fsk16BGR5515DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 5, 8 - 5, fsk16BGR5515DEBluePosition ),	\
												q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
											)

#define fskConvert24BGR16BGR5515DE(p, q)	(q =	(fskDefaultAlpha1                                                  << fsk16BGR5515DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 5, 8 - 5, fsk16BGR5515DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 5, 8 - 5, fsk16BGR5515DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 5, 8 - 5, fsk16BGR5515DEBluePosition ),	\
												q |= (q >> 16)	/* , q &= 0xFFFF; */																	\
											)

/* --> 16 RGBA 4444 SE */
#define fskConvert24RGB16RGBA4444SE(p, q)	(q =	(fskDefaultAlpha4                                                  << fsk16RGBA4444SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 4, 8 - 4, fsk16RGBA4444SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 4, 8 - 4, fsk16RGBA4444SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 4, 8 - 4, fsk16RGBA4444SEBluePosition )	\
											)

#define fskConvert24BGR16RGBA4444SE(p, q)	(q =	(fskDefaultAlpha4                                                  << fsk16RGBA4444SEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 4, 8 - 4, fsk16RGBA4444SERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 4, 8 - 4, fsk16RGBA4444SEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 4, 8 - 4, fsk16RGBA4444SEBluePosition )	\
											)

/* --> 16 RGBA 4444 DE */
#define fskConvert24RGB16RGBA4444DE(p, q)	(q =	(fskDefaultAlpha4                                                  << fsk16RGBA4444DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBRedPosition  ], 4, 8 - 4, fsk16RGBA4444DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBGreenPosition], 4, 8 - 4, fsk16RGBA4444DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24RGBBluePosition ], 4, 8 - 4, fsk16RGBA4444DEBluePosition )	\
											)

#define fskConvert24BGR16RGBA4444DE(p, q)	(q =	(fskDefaultAlpha4                                                  << fsk16RGBA4444DEAlphaPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRRedPosition  ], 4, 8 - 4, fsk16RGBA4444DERedPosition  )	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRGreenPosition], 4, 8 - 4, fsk16RGBA4444DEGreenPosition)	\
												|	FskMoveField(((const UInt8*)(&(p)))[fsk24BGRBluePosition ], 4, 8 - 4, fsk16RGBA4444DEBluePosition )	\
											)


/* --> 16 AG */
#define fskConvert24RGB16AG(p, q)			(q =	(fskDefaultAlpha1 << fsk16AGAlphaPosition)							\
												|	(((	((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] * fskRtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24RGBGreenPosition] * fskGtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24RGBBluePosition ] * fskBtoYCoeff	\
													+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
											)

#define fskConvert24BGR16AG(p, q)			(q =	(fskDefaultAlpha1 << fsk16AGAlphaPosition)							\
												|	(((	((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] * fskRtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24BGRGreenPosition] * fskGtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24BGRBluePosition ] * fskBtoYCoeff	\
													+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
											)


/* --> 16 GA */
#define fskConvert24RGB16GA(p, q)			(q =	(fskDefaultAlpha1 << fsk16GAAlphaPosition)							\
												|	(((	((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] * fskRtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24RGBGreenPosition] * fskGtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24RGBBluePosition ] * fskBtoYCoeff	\
													+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
											)

#define fskConvert24BGR16GA(p, q)			(q =	(fskDefaultAlpha1 << fsk16GAAlphaPosition)							\
												|	(((	((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] * fskRtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24BGRGreenPosition] * fskGtoYCoeff	\
													+	((const UInt8*)(&(p)))[fsk24BGRBluePosition ] * fskBtoYCoeff	\
													+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
											)


/********************************************************************************
 * 24 -> 8 gray
 ********************************************************************************/


/* --> 8 gray */
#define fskConvert24RGB8G(p, q)				(q = (UInt8)(															\
												(	((const UInt8*)(&(p)))[fsk24RGBRedPosition  ] * fskRtoYCoeff	\
												+	((const UInt8*)(&(p)))[fsk24RGBGreenPosition] * fskGtoYCoeff	\
												+	((const UInt8*)(&(p)))[fsk24RGBBluePosition ] * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift)												\
											)

#define fskConvert24BGR8G(p, q)				(q = (UInt8)(															\
												(	((const UInt8*)(&(p)))[fsk24BGRRedPosition  ] * fskRtoYCoeff	\
												+	((const UInt8*)(&(p)))[fsk24BGRGreenPosition] * fskGtoYCoeff	\
												+	((const UInt8*)(&(p)))[fsk24BGRBluePosition ] * fskBtoYCoeff	\
												+	(1 << (fskToYCoeffShift - 1))									\
												) >> fskToYCoeffShift)												\
											)


#define fskConvert24RGB8A(p, q)				(q = (UInt8)fskDefaultAlpha)
#define fskConvert24BGR8A(p, q)				(q = (UInt8)fskDefaultAlpha)


/********************************************************************************
 * 16 -> 32
 ********************************************************************************/


/* 16 RGB 565 SE --> */
#define fskConvert16RGB565SE32ARGB(p)	(p =	(fskDefaultAlpha <<                                  fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 6, fsk16RGB565SEGreenPosition, 8-6 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ARGBGreenPosition))						\
										)

#define fskConvert16RGB565SE32BGRA(p)	(p =	(fskDefaultAlpha <<                                  fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 6, fsk16RGB565SEGreenPosition, 8-6 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											                |	fskFieldMask(8-5, fsk32BGRABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32BGRAGreenPosition))						\
										)

#define fskConvert16RGB565SE32ABGR(p)	(p =	(fskDefaultAlpha <<                                  fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 6, fsk16RGB565SEGreenPosition, 8-6 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											                |	fskFieldMask(8-5, fsk32ABGRBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ABGRGreenPosition))						\
										)

#define fskConvert16RGB565SE32RGBA(p)	(p =	(fskDefaultAlpha <<                                  fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 6, fsk16RGB565SEGreenPosition, 8-6 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB565SEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											                |	fskFieldMask(8-5, fsk32RGBABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32RGBAGreenPosition))						\
										)

#define fskConvert16RGB565SE32A16RGB565SE(p)\
										(p =	(fskDefaultAlpha									  << fsk32RGBAAlphaPosition)	\
											|	p																					\
										)

/* 16 RGB 565 DE --> */
#define fskConvert16RGB565DE32ARGB(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                                  fsk32ARGBAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
												|	FskMoveField(p, 6, fsk16RGB565DEGreenPosition, 8-6 + fsk32ARGBGreenPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)								\
											                |	fskFieldMask(8-5, fsk32ARGBBluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ARGBGreenPosition))							\
										)

#define fskConvert16RGB565DE32BGRA(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32BGRAAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DERedPosition,   8-5 + fsk32BGRARedPosition  )	\
												|	FskMoveField(p, 6, fsk16RGB565DEGreenPosition, 8-6 + fsk32BGRAGreenPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)								\
											                |	fskFieldMask(8-5, fsk32BGRABluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32BGRAGreenPosition))							\
										)

#define fskConvert16RGB565DE32ABGR(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32ABGRAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
												|	FskMoveField(p, 6, fsk16RGB565DEGreenPosition, 8-6 + fsk32ABGRGreenPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)								\
											                |	fskFieldMask(8-5, fsk32ABGRBluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ABGRGreenPosition))							\
										)

#define fskConvert16RGB565DE32RGBA(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32RGBAAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DERedPosition,   8-5 + fsk32RGBARedPosition  )	\
												|	FskMoveField(p, 6, fsk16RGB565DEGreenPosition, 8-6 + fsk32RGBAGreenPosition)	\
												|	FskMoveField(p, 5, fsk16RGB565DEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)								\
											                |	fskFieldMask(8-5, fsk32RGBABluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32RGBAGreenPosition))							\
										)


/* 16 BGR 565 SE --> */
#define fskConvert16BGR565SE32ARGB(p)	(p =	(fskDefaultAlpha <<                                  fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 6, fsk16BGR565SEGreenPosition, 8-6 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ARGBGreenPosition))						\
										)

#define fskConvert16BGR565SE32BGRA(p)	(p =	(fskDefaultAlpha <<                                  fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 6, fsk16BGR565SEGreenPosition, 8-6 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											                |	fskFieldMask(8-5, fsk32BGRABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32BGRAGreenPosition))						\
										)

#define fskConvert16BGR565SE32ABGR(p)	(p =	(fskDefaultAlpha <<                                  fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 6, fsk16BGR565SEGreenPosition, 8-6 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											                |	fskFieldMask(8-5, fsk32ABGRBluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ABGRGreenPosition))						\
										)

#define fskConvert16BGR565SE32RGBA(p)	(p =	(fskDefaultAlpha <<                                  fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 6, fsk16BGR565SEGreenPosition, 8-6 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR565SEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											                |	fskFieldMask(8-5, fsk32RGBABluePosition)),						\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32RGBAGreenPosition))						\
										)


/* 16 BGR 565 DE --> */
#define fskConvert16BGR565DE32ARGB(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                                  fsk32ARGBAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
												|	FskMoveField(p, 6, fsk16BGR565DEGreenPosition, 8-6 + fsk32ARGBGreenPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)								\
											                |	fskFieldMask(8-5, fsk32ARGBBluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ARGBGreenPosition))							\
										)

#define fskConvert16BGR565DE32BGRA(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32BGRAAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DERedPosition,   8-5 + fsk32BGRARedPosition  )	\
												|	FskMoveField(p, 6, fsk16BGR565DEGreenPosition, 8-6 + fsk32BGRAGreenPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)								\
											                |	fskFieldMask(8-5, fsk32BGRABluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32BGRAGreenPosition))							\
										)

#define fskConvert16BGR565DE32ABGR(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32ABGRAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
												|	FskMoveField(p, 6, fsk16BGR565DEGreenPosition, 8-6 + fsk32ABGRGreenPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)								\
											                |	fskFieldMask(8-5, fsk32ABGRBluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32ABGRGreenPosition))							\
										)

#define fskConvert16BGR565DE32RGBA(p)	(	p &= 0xFFFF, p |= p << 16,																\
											p =		(fskDefaultAlpha <<                             	 fsk32RGBAAlphaPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DERedPosition,   8-5 + fsk32RGBARedPosition  )	\
												|	FskMoveField(p, 6, fsk16BGR565DEGreenPosition, 8-6 + fsk32RGBAGreenPosition)	\
												|	FskMoveField(p, 5, fsk16BGR565DEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)								\
											                |	fskFieldMask(8-5, fsk32RGBABluePosition)),							\
											p |= (p >> 6) & (	fskFieldMask(8-6, fsk32RGBAGreenPosition))							\
										)


/* 16 RGB 5515 SE --> */
#define fskConvert16RGB5515SE32ARGB(p)	(p =	((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255)  << fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition, 8-5 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition))						\
										)

#define fskConvert16RGB5515SE32BGRA(p)	(p =	((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255)  << fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition, 8-5 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32BGRAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32BGRABluePosition))						\
										)

#define fskConvert16RGB5515SE32ABGR(p)	(p =	((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255)  << fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition, 8-5 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ABGRGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ABGRBluePosition))						\
										)

#define fskConvert16RGB5515SE32RGBA(p)	(p =	((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255)  << fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition, 8-5 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32RGBAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32RGBABluePosition))						\
										)


/* 16 RGB 5515 DE --> */
#define fskConvert16RGB5515DE32ARGB(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255)  << fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition, 8-5 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition))						\
										)

#define fskConvert16RGB5515DE32BGRA(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255)  << fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition, 8-5 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32BGRAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32BGRABluePosition))						\
										)

#define fskConvert16RGB5515DE32ABGR(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255)  << fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition, 8-5 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ABGRGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ABGRBluePosition))						\
										)

#define fskConvert16RGB5515DE32RGBA(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255)  << fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition, 8-5 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32RGBAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32RGBABluePosition))						\
										)


/* 16 BGR 5515 SE --> */
#define fskConvert16BGR5515SE32ARGB(p)	(p =	((((p >> fsk16BGR5515SEAlphaPosition) & 1) * 255)  << fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition, 8-5 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition))						\
										)

#define fskConvert16BGR5515SE32BGRA(p)	(p =	((((p >> fsk16BGR5515SEAlphaPosition) & 1) * 255)  << fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition, 8-5 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32BGRAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32BGRABluePosition))						\
										)

#define fskConvert16BGR5515SE32ABGR(p)	(p =	((((p >> fsk16BGR5515SEAlphaPosition) & 1) * 255)  << fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition, 8-5 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ABGRGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ABGRBluePosition))						\
										)

#define fskConvert16BGR5515SE32RGBA(p)	(p =	((((p >> fsk16BGR5515SEAlphaPosition) & 1) * 255)  << fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition, 8-5 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32RGBAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32RGBABluePosition))						\
										)


/* 16 BGR 5515 DE --> */
#define fskConvert16BGR5515DE32ARGB(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16BGR5515DEAlphaPosition) & 1) * 255)  << fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DERedPosition,   8-5 + fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition, 8-5 + fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,  8-5 + fsk32ARGBBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ARGBRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ARGBGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ARGBBluePosition))						\
										)

#define fskConvert16BGR5515DE32BGRA(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16BGR5515DEAlphaPosition) & 1) * 255)  << fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DERedPosition,   8-5 + fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition, 8-5 + fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,  8-5 + fsk32BGRABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32BGRARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32BGRAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32BGRABluePosition))						\
										)

#define fskConvert16BGR5515DE32ABGR(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16BGR5515DEAlphaPosition) & 1) * 255)  << fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DERedPosition,   8-5 + fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition, 8-5 + fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,  8-5 + fsk32ABGRBluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32ABGRRedPosition)							\
											               	|	fskFieldMask(8-5, fsk32ABGRGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32ABGRBluePosition))						\
										)

#define fskConvert16BGR5515DE32RGBA(p)	(	p &= 0xFFFF, p |= p << 16,															\
											p =	((((p >> fsk16BGR5515DEAlphaPosition) & 1) * 255)  << fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DERedPosition,   8-5 + fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition, 8-5 + fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,  8-5 + fsk32RGBABluePosition ),	\
											p |= (p >> 5) & (	fskFieldMask(8-5, fsk32RGBARedPosition)							\
											               	|	fskFieldMask(8-5, fsk32RGBAGreenPosition)						\
											               	|	fskFieldMask(8-5, fsk32RGBABluePosition))						\
										)


/* 16 RGBA 4444 SE --> */
#define fskConvert16RGBA4444SE32ARGB(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk32ARGBBluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444SE32BGRA(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk32BGRABluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444SE32RGBA(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk32RGBABluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444SE32ABGR(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk32ABGRBluePosition ),	\
											p |= p << 4																		\
										)


/* 16 RGBA 4444 DE --> */
#define fskConvert16RGBA4444DE32ARGB(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  fsk32ARGBBluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444DE32BGRA(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  fsk32BGRABluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444DE32RGBA(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  fsk32RGBABluePosition ),	\
											p |= p << 4																		\
										)

#define fskConvert16RGBA4444DE32ABGR(p)	(p	=	FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  fsk32ABGRBluePosition ),	\
											p |= p << 4																		\
										)


/* 16 AG -> 32 */
#define fskConvert16AG32ARGB(p)			(p =	FskMoveField(p, 8, fsk16AGAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ARGBBluePosition )	\
										)
#define fskConvert16AG32BGRA(p)			(p =	FskMoveField(p, 8, fsk16AGAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32BGRABluePosition )	\
										)
#define fskConvert16AG32ABGR(p)			(p =	FskMoveField(p, 8, fsk16AGAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32ABGRBluePosition )	\
										)
#define fskConvert16AG32RGBA(p)			(p =	FskMoveField(p, 8, fsk16AGAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 8, fsk16AGGrayPosition,  fsk32RGBABluePosition )	\
										)


/* 16 GA -> 32 */
#define fskConvert16GA32ARGB(p)			(p =	FskMoveField(p, 8, fsk16GAAlphaPosition, fsk32ARGBAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ARGBRedPosition  )	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ARGBGreenPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ARGBBluePosition )	\
										)
#define fskConvert16GA32BGRA(p)			(p =	FskMoveField(p, 8, fsk16GAAlphaPosition, fsk32BGRAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32BGRARedPosition  )	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32BGRAGreenPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32BGRABluePosition )	\
										)
#define fskConvert16GA32ABGR(p)			(p =	FskMoveField(p, 8, fsk16GAAlphaPosition, fsk32ABGRAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ABGRRedPosition  )	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ABGRGreenPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32ABGRBluePosition )	\
										)
#define fskConvert16GA32RGBA(p)			(p =	FskMoveField(p, 8, fsk16GAAlphaPosition, fsk32RGBAAlphaPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32RGBARedPosition  )	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32RGBAGreenPosition)	\
											|	FskMoveField(p, 8, fsk16GAGrayPosition,  fsk32RGBABluePosition )	\
										)
#define fskConvert16GA32A16RGB565SE(p)\
										(p =	FskMoveField(p, 8, fsk16GAAlphaPosition,     fsk32A16RGB565SEAlphaPosition)	\
											|	FskMoveField(p, 5, fsk16GAGrayPosition + 3,  fsk32A16RGB565SERedPosition  )	\
											|	FskMoveField(p, 6, fsk16GAGrayPosition + 2,  fsk32A16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, fsk16GAGrayPosition + 3,  fsk32A16RGB565SEBluePosition )	\
										)


/********************************************************************************
 * 16 -> 24
 ********************************************************************************/


/* 16 RGB 565 SE --> */
#define fskConvert16RGB565SE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565SERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565SERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16RGB565SEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16RGB565SEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565SEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565SEBluePosition  + 2*5-8,   0))	\
										)

#define fskConvert16RGB565SE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565SERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565SERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16RGB565SEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16RGB565SEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565SEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565SEBluePosition  + 2*5-8,   0))	\
										)

/* 16 RGB 565 DE --> */
#define fskConvert16RGB565DE24RGB(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
											((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565DERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565DERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16RGB565DEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16RGB565DEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565DEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565DEBluePosition  + 2*5-8,   0))	\
										)

#define fskConvert16RGB565DE24BGR(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
											((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565DERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565DERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16RGB565DEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16RGB565DEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB565DEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16RGB565DEBluePosition  + 2*5-8,   0))	\
										)

/* 16 BGR 565 SE --> */
#define fskConvert16BGR565SE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565SERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565SERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16BGR565SEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16BGR565SEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565SEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565SEBluePosition  + 2*5-8,   0))	\
										)

#define fskConvert16BGR565SE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565SERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565SERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16BGR565SEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16BGR565SEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565SEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565SEBluePosition  + 2*5-8,   0))	\
										)

/* 16 BGR 565 DE --> */
#define fskConvert16BGR565DE24RGB(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
											((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565DERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565DERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16BGR565DEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16BGR565DEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565DEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565DEBluePosition  + 2*5-8,   0))	\
										)

#define fskConvert16BGR565DE24BGR(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
											((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565DERedPosition,           8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565DERedPosition   + 2*5-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   6, fsk16BGR565DEGreenPosition,         8-6)		\
																							| FskMoveField(p, 8-6, fsk16BGR565DEGreenPosition + 2*6-8,   0)),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR565DEBluePosition,          8-5)		\
																							| FskMoveField(p, 8-5, fsk16BGR565DEBluePosition  + 2*5-8,   0))	\
										)

/* 16 RGB 5515 SE --> */
#define fskConvert16RGB5515SE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SEBluePosition  + 2*5-8,   0))	\
											)

#define fskConvert16RGB5515SE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515SEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515SEBluePosition  + 2*5-8,   0))	\
											)

/* 16 RGB 5515 DE --> */
#define fskConvert16RGB5515DE24RGB(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
												((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DEBluePosition  + 2*5-8,   0))	\
											)

#define fskConvert16RGB5515DE24BGR(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
												((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16RGB5515DEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16RGB5515DEBluePosition  + 2*5-8,   0))	\
											)

#define fskConvert16BGR5515SE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SEBluePosition  + 2*5-8,   0))	\
											)

/* 16 BGR 5515 SE --> */
#define fskConvert16BGR5515SE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515SEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515SEBluePosition  + 2*5-8,   0))	\
											)

#define fskConvert16BGR5515DE24RGB(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
												((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 2*5-8,   0))	\
											)

/* 16 BGR 5515 DE --> */
#define fskConvert16BGR5515DE24BGR(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
												((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 2*5-8,   0))	\
											)

#define fskConvert16BGR5515DE24RGB(p, q)	(	p &= 0xFFFF, p |= (p << 16),																						\
												((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DERedPosition,           8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEGreenPosition,         8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 2*5-8,   0)),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p,   5, fsk16BGR5515DEBluePosition,          8-5)	\
																								| FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 2*5-8,   0))	\
											)

/* 16 RGBA 4444 SE --> */
#define fskConvert16RGBA4444SE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, 0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  0) * 0x11)	\
											)

#define fskConvert16RGBA4444SE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, 0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  0) * 0x11)	\
											)

/* 16 RGBA 4444 DE --> */
#define fskConvert16RGBA4444DE24BGR(p, q)	(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24BGRGreenPosition]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, 0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  0) * 0x11)	\
											)

#define fskConvert16RGBA4444DE24RGB(p, q)	(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24RGBGreenPosition]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, 0) * 0x11),	\
												((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)(FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  0) * 0x11)	\
											)


/* 16 AG -> 24 */
#define fskConvert16AG24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	=													\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	=													\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)FskMoveField(p, 8, fsk16AGGrayPosition, 0)	\
										)

#define fskConvert16AG24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	=													\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	=													\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)FskMoveField(p, 8, fsk16AGGrayPosition, 0)	\
										)

/* 16 GA -> 24 */
#define fskConvert16GA24RGB(p,q)		(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ]	=													\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition]	=													\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ]	= (UInt8)FskMoveField(p, 8, fsk16GAGrayPosition, 0)	\
										)

#define fskConvert16GA24BGR(p,q)		(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ]	=													\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition]	=													\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ]	= (UInt8)FskMoveField(p, 8, fsk16GAGrayPosition, 0)	\
										)



/********************************************************************************
 * 16 -> 16
 ********************************************************************************/


#define fskConvert16RGB565SE16RGB565DE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16RGB565DE16RGB565SE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16BGR565SE16BGR565DE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16BGR565DE16BGR565SE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16RGB5515SE16RGB5515DE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16RGB5515DE16RGB5515SE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16BGR5515SE16BGR5515DE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16BGR5515DE16BGR5515SE(p)		(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16RGBA4444SE16RGBA4444DE(p)	(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */
#define fskConvert16RGBA4444DE16RGBA4444SE(p)	(	p = ((p >> 8) & 0xFF) | ((p & 0xFF) << 8)	)	/* Byte swap */


/* 565 --> 565 */
#define fskConvert16RGB565SE16BGR565SE(p)	(	p	= FskMoveField(p, 5, fsk16RGB565SERedPosition,   fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 6, fsk16RGB565SEGreenPosition, fsk16BGR565SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,  fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16RGB565SE16BGR565DE(p)	(	p	= FskMoveField(p, 5, fsk16RGB565SERedPosition,   fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 6, fsk16RGB565SEGreenPosition, fsk16BGR565DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,  fsk16BGR565DEBluePosition)	,	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565SE16RGB565SE(p)	(	p	= FskMoveField(p, 5, fsk16BGR565SERedPosition,   fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 6, fsk16BGR565SEGreenPosition, fsk16RGB565SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,  fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16BGR565SE16RGB565DE(p)	(	p	= FskMoveField(p, 5, fsk16BGR565SERedPosition,   fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 6, fsk16BGR565SEGreenPosition, fsk16RGB565DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,  fsk16RGB565DEBluePosition)	,	\
												p	|= p >> 16																		\
											)
#define fskConvert16RGB565DE16BGR565SE(p)	(	p	|= p << 16,																		\
												p	= FskMoveField(p, 5, fsk16RGB565SERedPosition,   fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 6, fsk16RGB565SEGreenPosition, fsk16BGR565SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,  fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16RGB565DE16BGR565DE(p)	(	p	|= p << 16,																		\
												p	= FskMoveField(p, 5, fsk16RGB565SERedPosition,   fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 6, fsk16RGB565SEGreenPosition, fsk16BGR565DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,  fsk16BGR565DEBluePosition)	,	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565DE16RGB565SE(p)	(	p	|= p << 16,																		\
												p	= FskMoveField(p, 5, fsk16BGR565SERedPosition,   fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 6, fsk16BGR565SEGreenPosition, fsk16RGB565SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,  fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16BGR565DE16RGB565DE(p)	(	p	|= p << 16,																		\
												p	= FskMoveField(p, 5, fsk16BGR565SERedPosition,   fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 6, fsk16BGR565SEGreenPosition, fsk16RGB565DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,  fsk16RGB565DEBluePosition)	,	\
												p	|= p >> 16																		\
											)

/* 16 5515 --> 565 */
#define fskConvert16RGB5515SE16RGB565SE(p)	(	p	= FskMoveField(p, 5, fsk16RGB5515SERedPosition,     fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,   fsk16RGB565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515SEGreenPosition+4, fsk16RGB565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEBluePosition,    fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16RGB5515SE16RGB565DE(p)	(	p	= FskMoveField(p, 5, fsk16RGB5515SERedPosition,     fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,   fsk16RGB565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515SEGreenPosition+4, fsk16RGB565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEBluePosition,    fsk16RGB565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16RGB5515SE16BGR565SE(p)	(	p	= FskMoveField(p, 5, fsk16RGB5515SERedPosition,     fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,   fsk16BGR565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515SEGreenPosition+4, fsk16BGR565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEBluePosition,    fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16RGB5515SE16BGR565DE(p)	(	p	= FskMoveField(p, 5, fsk16RGB5515SERedPosition,     fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,   fsk16BGR565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515SEGreenPosition+4, fsk16BGR565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515SEBluePosition,    fsk16BGR565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16RGB5515DE16RGB565SE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16RGB5515DERedPosition,     fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,   fsk16RGB565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515DEGreenPosition+4, fsk16RGB565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEBluePosition,    fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16RGB5515DE16RGB565DE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16RGB5515DERedPosition,     fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,   fsk16RGB565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515DEGreenPosition+4, fsk16RGB565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEBluePosition,    fsk16RGB565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16RGB5515DE16BGR565SE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16RGB5515DERedPosition,     fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,   fsk16BGR565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515DEGreenPosition+4, fsk16BGR565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEBluePosition,    fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16RGB5515DE16BGR565DE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16RGB5515DERedPosition,     fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,   fsk16BGR565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16RGB5515DEGreenPosition+4, fsk16BGR565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16RGB5515DEBluePosition,    fsk16BGR565DEBluePosition),		\
												p	|= p >> 16																			\
											)


#define fskConvert16BGR5515SE16RGB565SE(p)	(	p	= FskMoveField(p, 5, fsk16BGR5515SERedPosition,     fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,   fsk16RGB565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515SEGreenPosition+4, fsk16RGB565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEBluePosition,    fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16BGR5515SE16RGB565DE(p)	(	p	= FskMoveField(p, 5, fsk16BGR5515SERedPosition,     fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,   fsk16RGB565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515SEGreenPosition+4, fsk16RGB565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEBluePosition,    fsk16RGB565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16BGR5515SE16BGR565SE(p)	(	p	= FskMoveField(p, 5, fsk16BGR5515SERedPosition,     fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,   fsk16BGR565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515SEGreenPosition+4, fsk16BGR565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEBluePosition,    fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16BGR5515SE16BGR565DE(p)	(	p	= FskMoveField(p, 5, fsk16BGR5515SERedPosition,     fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,   fsk16BGR565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515SEGreenPosition+4, fsk16BGR565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515SEBluePosition,    fsk16BGR565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16BGR5515DE16RGB565SE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16BGR5515DERedPosition,     fsk16RGB565SERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,   fsk16RGB565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515DEGreenPosition+4, fsk16RGB565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEBluePosition,    fsk16RGB565SEBluePosition)		\
											)
#define fskConvert16BGR5515DE16RGB565DE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16BGR5515DERedPosition,     fsk16RGB565DERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,   fsk16RGB565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515DEGreenPosition+4, fsk16RGB565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEBluePosition,    fsk16RGB565DEBluePosition),		\
												p	|= p >> 16																			\
											)
#define fskConvert16BGR5515DE16BGR565SE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16BGR5515DERedPosition,     fsk16BGR565SERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,   fsk16BGR565SEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515DEGreenPosition+4, fsk16BGR565SEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEBluePosition,    fsk16BGR565SEBluePosition)		\
											)
#define fskConvert16BGR5515DE16BGR565DE(p)	(	p	|= p << 16,																			\
												p	= FskMoveField(p, 5, fsk16BGR5515DERedPosition,     fsk16BGR565DERedPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,   fsk16BGR565DEGreenPosition+1)	\
													| FskMoveField(p, 1, fsk16BGR5515DEGreenPosition+4, fsk16BGR565DEGreenPosition)		\
													| FskMoveField(p, 5, fsk16BGR5515DEBluePosition,    fsk16BGR565DEBluePosition),		\
												p	|= p >> 16																			\
											)

/* 565 --> 5515 */
#define fskConvert16RGB565SE16RGB5515SE(p)	(	p	= (fskDefaultAlpha1                             << fsk16RGB5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SERedPosition,     fsk16RGB5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEGreenPosition+1, fsk16RGB5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,    fsk16RGB5515SEBluePosition)	\
											)
#define fskConvert16RGB565SE16BGR5515SE(p)	(	p	= (fskDefaultAlpha1                             << fsk16BGR5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SERedPosition,     fsk16BGR5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEGreenPosition+1, fsk16BGR5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,    fsk16BGR5515SEBluePosition)	\
											)
#define fskConvert16RGB565SE16RGB5515DE(p)	(	p	= (fskDefaultAlpha1                             << fsk16RGB5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SERedPosition,     fsk16RGB5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEGreenPosition+1, fsk16RGB5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,    fsk16RGB5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16RGB565SE16BGR5515DE(p)	(	p	= (fskDefaultAlpha1                             << fsk16BGR5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SERedPosition,     fsk16BGR5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEGreenPosition+1, fsk16BGR5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565SEBluePosition,    fsk16BGR5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565SE16RGB5515SE(p)	(	p	= (fskDefaultAlpha1                             << fsk16RGB5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SERedPosition,     fsk16RGB5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEGreenPosition+1, fsk16RGB5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,    fsk16RGB5515SEBluePosition)	\
											)
#define fskConvert16BGR565SE16BGR5515SE(p)	(	p	= (fskDefaultAlpha1                             << fsk16BGR5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SERedPosition,     fsk16BGR5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEGreenPosition+1, fsk16BGR5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,    fsk16BGR5515SEBluePosition)	\
											)
#define fskConvert16BGR565SE16RGB5515DE(p)	(	p	= (fskDefaultAlpha1                             << fsk16RGB5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SERedPosition,     fsk16RGB5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEGreenPosition+1, fsk16RGB5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,    fsk16RGB5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565SE16BGR5515DE(p)	(	p	= (fskDefaultAlpha1                             << fsk16BGR5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SERedPosition,     fsk16BGR5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEGreenPosition+1, fsk16BGR5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565SEBluePosition,    fsk16BGR5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16RGB565DE16RGB5515SE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16RGB5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DERedPosition,     fsk16RGB5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEGreenPosition+1, fsk16RGB5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEBluePosition,    fsk16RGB5515SEBluePosition)	\
											)
#define fskConvert16RGB565DE16BGR5515SE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16BGR5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DERedPosition,     fsk16BGR5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEGreenPosition+1, fsk16BGR5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEBluePosition,    fsk16BGR5515SEBluePosition)	\
											)
#define fskConvert16RGB565DE16RGB5515DE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16RGB5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DERedPosition,     fsk16RGB5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEGreenPosition+1, fsk16RGB5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEBluePosition,    fsk16RGB5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16RGB565DE16BGR5515DE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16BGR5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DERedPosition,     fsk16BGR5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEGreenPosition+1, fsk16BGR5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16RGB565DEBluePosition,    fsk16BGR5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565DE16RGB5515SE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16RGB5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DERedPosition,     fsk16RGB5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEGreenPosition+1, fsk16RGB5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEBluePosition,    fsk16RGB5515SEBluePosition)	\
											)
#define fskConvert16BGR565DE16BGR5515SE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16BGR5515SEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DERedPosition,     fsk16BGR5515SERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEGreenPosition+1, fsk16BGR5515SEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEBluePosition,    fsk16BGR5515SEBluePosition)	\
											)
#define fskConvert16BGR565DE16RGB5515DE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16RGB5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DERedPosition,     fsk16RGB5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEGreenPosition+1, fsk16RGB5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEBluePosition,    fsk16RGB5515DEBluePosition),	\
												p	|= p >> 16																		\
											)
#define fskConvert16BGR565DE16BGR5515DE(p)	(	p	|= p << 16,																		\
												p	= (fskDefaultAlpha1                             << fsk16BGR5515DEAlphaPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DERedPosition,     fsk16BGR5515DERedPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEGreenPosition+1, fsk16BGR5515DEGreenPosition)	\
													| FskMoveField(p, 5, fsk16BGR565DEBluePosition,    fsk16BGR5515DEBluePosition),	\
												p	|= p >> 16																		\
											)

/* 565 -> 4444 */
#define fskConvert16RGB565SE16RGBA4444SE(p)	(	p	= (fskDefaultAlpha4                             << fsk16RGBA4444SEAlphaPosition)	\
													| FskMoveField(p, 4, fsk16RGB565SERedPosition  +1, fsk16RGBA4444SERedPosition  )	\
													| FskMoveField(p, 4, fsk16RGB565SEGreenPosition+2, fsk16RGBA4444SEGreenPosition)	\
													| FskMoveField(p, 4, fsk16RGB565SEBluePosition +1, fsk16RGBA4444SEBluePosition )	\
											)
#define fskConvert16RGB565DE16RGBA4444SE(p)	(	p	|= p << 16,																			\
												p	= (fskDefaultAlpha4                             << fsk16RGBA4444SEAlphaPosition)	\
													| FskMoveField(p, 4, fsk16RGB565DERedPosition  +1, fsk16RGBA4444SERedPosition  )	\
													| FskMoveField(p, 4, fsk16RGB565DEGreenPosition+2, fsk16RGBA4444SEGreenPosition)	\
													| FskMoveField(p, 4, fsk16RGB565DEBluePosition +1, fsk16RGBA4444SEBluePosition )	\
											)
#define fskConvert16RGB565SE16RGBA4444DE(p)	(	p	= (fskDefaultAlpha4                             << fsk16RGBA4444DEAlphaPosition)	\
													| FskMoveField(p, 4, fsk16RGB565SERedPosition  +1, fsk16RGBA4444DERedPosition  )	\
													| FskMoveField(p, 4, fsk16RGB565SEGreenPosition+2, fsk16RGBA4444DEGreenPosition)	\
													| FskMoveField(p, 4, fsk16RGB565SEBluePosition +1, fsk16RGBA4444DEBluePosition )	\
											)
#define fskConvert16RGB565DE16RGBA4444DE(p)	(	p	|= p << 16,																			\
												p	= (fskDefaultAlpha4                             << fsk16RGBA4444DEAlphaPosition)	\
													| FskMoveField(p, 4, fsk16RGB565DERedPosition  +1, fsk16RGBA4444DERedPosition  )	\
													| FskMoveField(p, 4, fsk16RGB565DEGreenPosition+2, fsk16RGBA4444DEGreenPosition)	\
													| FskMoveField(p, 4, fsk16RGB565DEBluePosition +1, fsk16RGBA4444DEBluePosition )	\
											)

/* 5515 -> 4444 */
#define fskConvert16RGB5515SE16RGBA4444SE(p)	(	p	= ((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255) << fsk16RGBA4444SEAlphaPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515SERedPosition  +1,    fsk16RGBA4444SERedPosition  )	\
														| FskMoveField(p, 4, fsk16RGB5515SEGreenPosition+1,    fsk16RGBA4444SEGreenPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515SEBluePosition +1,    fsk16RGBA4444SEBluePosition )	\
												)
#define fskConvert16RGB5515DE16RGBA4444SE(p)	(	p	|= p << 16,																				\
													p	= ((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255) << fsk16RGBA4444SEAlphaPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515DERedPosition  +1,    fsk16RGBA4444SERedPosition  )	\
														| FskMoveField(p, 4, fsk16RGB5515DEGreenPosition+1,    fsk16RGBA4444SEGreenPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515DEBluePosition +1,    fsk16RGBA4444SEBluePosition )	\
												)
#define fskConvert16RGB5515SE16RGBA4444DE(p)	(	p	= ((((p >> fsk16RGB5515SEAlphaPosition) & 1) * 255) << fsk16RGBA4444SEAlphaPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515SERedPosition  +1,    fsk16RGBA4444DERedPosition  )	\
														| FskMoveField(p, 4, fsk16RGB5515SEGreenPosition+1,    fsk16RGBA4444DEGreenPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515SEBluePosition +1,    fsk16RGBA4444DEBluePosition )	\
												)
#define fskConvert16RGB5515DE16RGBA4444DE(p)	(	p	|= p << 16,																				\
													p	= ((((p >> fsk16RGB5515DEAlphaPosition) & 1) * 255) << fsk16RGBA4444SEAlphaPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515DERedPosition  +1,    fsk16RGBA4444DERedPosition  )	\
														| FskMoveField(p, 4, fsk16RGB5515DEGreenPosition+1,    fsk16RGBA4444DEGreenPosition)	\
														| FskMoveField(p, 4, fsk16RGB5515DEBluePosition +1,    fsk16RGBA4444DEBluePosition )	\
												)

/********************************************************************************
 * 16 -> 16AG
 ********************************************************************************/

#define fskConvert16RGB565SE16AG(p)			(p = ((((	FskMoveField(p, 5,   fsk16RGB565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16RGB565DE16AG(p)			(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5,   fsk16RGB565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16BGR565SE16AG(p)			(p = ((((	FskMoveField(p, 5,   fsk16BGR565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16BGR565DE16AG(p)			(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5,   fsk16BGR565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16RGB5515SE16AG(p)		(p = ((((	FskMoveField(p, 5, fsk16RGB5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16RGB5515DE16AG(p)		(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5, fsk16RGB5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16BGR5515SE16AG(p)		(p = ((((	FskMoveField(p, 5, fsk16BGR5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16BGR5515DE16AG(p)		(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5, fsk16BGR5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)							\
												| (fskDefaultAlpha << fsk16AGAlphaPosition)								\
											)
#define fskConvert16RGBA4444SE16AG(p)		(p = ((((	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   0)									\
													) * (fskRtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, 0)									\
													) * (fskGtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 5, fsk16RGBA4444SEBluePosition,  0)									\
													) * (fskBtoYCoeff * 0x11)																\
												+	(1 << (fskToYCoeffShift - 1))															\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)												\
												| ((FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, 0) * 0x11) << fsk16AGAlphaPosition)	\
											)
#define fskConvert16RGBA4444DE16AG(p)		(p = ((((	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   0)									\
													) * (fskRtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, 0)									\
													) * (fskGtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 5, fsk16RGBA4444DEBluePosition,  0)									\
													) * (fskBtoYCoeff * 0x11)																\
												+	(1 << (fskToYCoeffShift - 1))															\
												) >> fskToYCoeffShift) << fsk16AGGrayPosition)												\
												| ((FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, 0) * 0x11) << fsk16AGAlphaPosition)	\
											)


/********************************************************************************
 * 16 -> 16GA
 ********************************************************************************/

#define fskConvert16RGB565SE16GA(p)			(p = ((((	FskMoveField(p, 5,   fsk16RGB565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16RGB565DE16GA(p)			(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5,   fsk16RGB565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16BGR565SE16GA(p)			(p = ((((	FskMoveField(p, 5,   fsk16BGR565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16BGR565DE16GA(p)			(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5,   fsk16BGR565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16RGB5515SE16GA(p)		(p = ((((	FskMoveField(p, 5, fsk16RGB5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16RGB5515DE16GA(p)		(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5, fsk16RGB5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16BGR5515SE16GA(p)		(p = ((((	FskMoveField(p, 5, fsk16BGR5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16BGR5515DE16GA(p)		(p |= p << 16,																\
											p = ((((	FskMoveField(p, 5, fsk16BGR5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)							\
												| (fskDefaultAlpha << fsk16GAAlphaPosition)								\
											)
#define fskConvert16RGBA4444SE16GA(p)		(p = ((((	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   0)									\
													) * (fskRtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, 0)									\
													) * (fskGtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 5, fsk16RGBA4444SEBluePosition,  0)									\
													) * (fskBtoYCoeff * 0x11)																\
												+	(1 << (fskToYCoeffShift - 1))															\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)												\
												| ((FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, 0) * 0x11) << fsk16GAAlphaPosition)	\
											)

#define fskConvert16RGBA4444DE16GA(p)		(p = ((((	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   0)									\
													) * (fskRtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, 0)									\
													) * (fskGtoYCoeff * 0x11)																\
												+	(	FskMoveField(p, 5, fsk16RGBA4444DEBluePosition,  0)									\
													) * (fskBtoYCoeff * 0x11)																\
												+	(1 << (fskToYCoeffShift - 1))															\
												) >> fskToYCoeffShift) << fsk16GAGrayPosition)												\
												| ((FskMoveField(p, 4, fsk16RGBA4444DEAlphaPosition, 0) * 0x11) << fsk16GAAlphaPosition)	\
											)


/********************************************************************************
 * 16AG -> 16
 ********************************************************************************/

#define fskConvert16AG16RGB565SE(p)		(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, (fsk16AGGrayPosition+8-6), fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16RGB565SEBluePosition)	\
										)
#define fskConvert16AG16RGB565DE(p)		(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, (fsk16AGGrayPosition+8-6), fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16AG16BGR565SE(p)		(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, (fsk16AGGrayPosition+8-6), fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16BGR565SEBluePosition)	\
										)
#define fskConvert16AG16BGR565DE(p)		(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, (fsk16AGGrayPosition+8-6), fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16AG16RGB5515SE(p)	(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5), fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16RGB5515SEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16AGAlphaPosition+8-1),fsk16RGB5515SEAlphaPosition)	\
										)
#define fskConvert16AG16RGB5515DE(p)	(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5), fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16RGB5515DEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16AGAlphaPosition+8-1),fsk16RGB5515DEAlphaPosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16AG16BGR5515SE(p)	(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5), fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16BGR5515SEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16AGAlphaPosition+8-1),fsk16BGR5515SEAlphaPosition)	\
										)
#define fskConvert16AG16BGR5515DE(p)	(p =	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5), fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16AGGrayPosition+8-5),  fsk16BGR5515DEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16AGAlphaPosition+8-1),fsk16BGR5515DEAlphaPosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)

#define fskConvert16AG16RGBA4444SE(p)	(p	= FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444SERedPosition)	\
											| FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444SEGreenPosition)	\
											| FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444SEBluePosition)	\
											| FskMoveField(p, 4, fsk16AGAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
										)

#define fskConvert16AG16RGBA4444DE(p)	(p	= FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444DERedPosition)	\
											| FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444DEGreenPosition)	\
											| FskMoveField(p, 4, fsk16AGGrayPosition+4,  fsk16RGBA4444DEBluePosition)	\
											| FskMoveField(p, 4, fsk16AGAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
										)


/********************************************************************************
 * 16GA -> 16
 ********************************************************************************/

#define fskConvert16GA16RGB565SE(p)		(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, (fsk16GAGrayPosition+8-6), fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16RGB565SEBluePosition)	\
										)
#define fskConvert16GA16RGB565DE(p)		(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, (fsk16GAGrayPosition+8-6), fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16GA16BGR565SE(p)		(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, (fsk16GAGrayPosition+8-6), fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16BGR565SEBluePosition)	\
										)
#define fskConvert16GA16BGR565DE(p)		(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, (fsk16GAGrayPosition+8-6), fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16GA16RGB5515SE(p)	(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5), fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16RGB5515SEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16GAAlphaPosition+8-1),fsk16RGB5515SEAlphaPosition)	\
										)
#define fskConvert16GA16RGB5515DE(p)	(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5), fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16RGB5515DEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16GAAlphaPosition+8-1),fsk16RGB5515DEAlphaPosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)
#define fskConvert16GA16BGR5515SE(p)	(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5), fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16BGR5515SEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16GAAlphaPosition+8-1),fsk16BGR5515SEAlphaPosition)	\
										)
#define fskConvert16GA16BGR5515DE(p)	(p =	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5), fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (fsk16GAGrayPosition+8-5),  fsk16BGR5515DEBluePosition)	\
											|	FskMoveField(p, 1, (fsk16GAAlphaPosition+8-1),fsk16BGR5515DEAlphaPosition),	\
											p |= (p >> 16), p &= 0xFFFF														\
										)

#define fskConvert16AG16GA(p)			(p	= FskMoveField(p, 8, fsk16AGAlphaPosition, fsk16GAAlphaPosition)	\
											| FskMoveField(p, 8,  fsk16AGGrayPosition,  fsk16GAGrayPosition)	\
										)

#define fskConvert16GA16AG(p)			(p	= FskMoveField(p, 8, fsk16GAAlphaPosition, fsk16AGAlphaPosition)	\
											| FskMoveField(p, 8,  fsk16GAGrayPosition,  fsk16AGGrayPosition)	\
										)

#define fskConvert16GA16RGBA4444SE(p)	(p	= FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444SERedPosition)	\
											| FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444SEGreenPosition)	\
											| FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444SEBluePosition)	\
											| FskMoveField(p, 4, fsk16GAAlphaPosition+4, fsk16RGBA4444SEAlphaPosition)	\
										)

#define fskConvert16GA16RGBA4444DE(p)	(p	= FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444DERedPosition)	\
											| FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444DEGreenPosition)	\
											| FskMoveField(p, 4, fsk16GAGrayPosition+4,  fsk16RGBA4444DEBluePosition)	\
											| FskMoveField(p, 4, fsk16GAAlphaPosition+4, fsk16RGBA4444DEAlphaPosition)	\
										)


/********************************************************************************
 * 16 RGB 4444 -> 16
 ********************************************************************************/

#define fskConvert16RGBA4444SE16RGB565SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk16RGB565SERedPosition   + (fsk16RGB565SERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk16RGB565SEGreenPosition + (fsk16RGB565SEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk16RGB565SEBluePosition  + (fsk16RGB565SEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16RGB565SERedBits  -4, fsk16RGB565SERedPosition  )							\
																|	fskFieldMask(fsk16RGB565SEGreenBits-4, fsk16RGB565SEGreenPosition)							\
																|	fskFieldMask(fsk16RGB565SEBlueBits -4, fsk16RGB565SEBluePosition )							\
																)																								\
											)

#define fskConvert16RGBA4444SE16RGB565DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk16RGB565DERedPosition   + (fsk16RGB565DERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk16RGB565DEGreenPosition + (fsk16RGB565DEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk16RGB565DEBluePosition  + (fsk16RGB565DEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16RGB565DERedBits  -4, fsk16RGB565DERedPosition  )							\
																|	fskFieldMask(fsk16RGB565DEGreenBits-4, fsk16RGB565DEGreenPosition)							\
																|	fskFieldMask(fsk16RGB565DEBlueBits -4, fsk16RGB565DEBluePosition )							\
																),																								\
												p |= (p >> 16)																									\
											)

#define fskConvert16RGBA4444SE16BGR565SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk16BGR565SERedPosition   + (fsk16BGR565SERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk16BGR565SEGreenPosition + (fsk16BGR565SEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk16BGR565SEBluePosition  + (fsk16BGR565SEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16BGR565SERedBits  -4, fsk16BGR565SERedPosition  )							\
																|	fskFieldMask(fsk16BGR565SEGreenBits-4, fsk16BGR565SEGreenPosition)							\
																|	fskFieldMask(fsk16BGR565SEBlueBits -4, fsk16BGR565SEBluePosition )							\
																)																								\
											)

#define fskConvert16RGBA4444SE16BGR565DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   fsk16BGR565DERedPosition   + (fsk16BGR565DERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, fsk16BGR565DEGreenPosition + (fsk16BGR565DEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  fsk16BGR565DEBluePosition  + (fsk16BGR565DEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16BGR565DERedBits  -4, fsk16BGR565DERedPosition  )							\
																|	fskFieldMask(fsk16BGR565DEGreenBits-4, fsk16BGR565DEGreenPosition)							\
																|	fskFieldMask(fsk16BGR565DEBlueBits -4, fsk16BGR565DEBluePosition )							\
																),																								\
												p |= (p >> 16)																									\
											)

#define fskConvert16RGBA4444SE16RGB5515SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,     fsk16RGB5515SERedPosition   + (fsk16RGB5515SERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition,   fsk16RGB5515SEGreenPosition + (fsk16RGB5515SEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,    fsk16RGB5515SEBluePosition  + (fsk16RGB5515SEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444SEAlphaPosition+3, fsk16RGB5515SEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16RGB5515SERedBits  -4, fsk16RGB5515SERedPosition  )						\
																	|	fskFieldMask(fsk16RGB5515SEGreenBits-4, fsk16RGB5515SEGreenPosition)						\
																	|	fskFieldMask(fsk16RGB5515SEBlueBits -4, fsk16RGB5515SEBluePosition )						\
																	)																								\
												)

#define fskConvert16RGBA4444SE16RGB5515DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,     fsk16RGB5515DERedPosition   + (fsk16RGB5515DERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition,   fsk16RGB5515DEGreenPosition + (fsk16RGB5515DEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,    fsk16RGB5515DEBluePosition  + (fsk16RGB5515DEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444SEAlphaPosition+3, fsk16RGB5515DEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16RGB5515DERedBits  -4, fsk16RGB5515DERedPosition  )						\
																	|	fskFieldMask(fsk16RGB5515DEGreenBits-4, fsk16RGB5515DEGreenPosition)						\
																	|	fskFieldMask(fsk16RGB5515DEBlueBits -4, fsk16RGB5515DEBluePosition )						\
																	),																								\
													p |= (p >> 16)																									\
												)

#define fskConvert16RGBA4444SE16BGR5515SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,     fsk16BGR5515SERedPosition   + (fsk16BGR5515SERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition,   fsk16BGR5515SEGreenPosition + (fsk16BGR5515SEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,    fsk16BGR5515SEBluePosition  + (fsk16BGR5515SEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444SEAlphaPosition+3, fsk16BGR5515SEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16BGR5515SERedBits  -4, fsk16BGR5515SERedPosition  )						\
																	|	fskFieldMask(fsk16BGR5515SEGreenBits-4, fsk16BGR5515SEGreenPosition)						\
																	|	fskFieldMask(fsk16BGR5515SEBlueBits -4, fsk16BGR5515SEBluePosition )						\
																	)																								\
												)

#define fskConvert16RGBA4444SE16BGR5515DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444SERedPosition,     fsk16BGR5515DERedPosition   + (fsk16BGR5515DERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition,   fsk16BGR5515DEGreenPosition + (fsk16BGR5515DEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,    fsk16BGR5515DEBluePosition  + (fsk16BGR5515DEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444SEAlphaPosition+3, fsk16BGR5515DEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16BGR5515DERedBits  -4, fsk16BGR5515DERedPosition  )						\
																	|	fskFieldMask(fsk16BGR5515DEGreenBits-4, fsk16BGR5515DEGreenPosition)						\
																	|	fskFieldMask(fsk16BGR5515DEBlueBits -4, fsk16BGR5515DEBluePosition )						\
																	),																								\
													p |= (p >> 16)																									\
												)


#define fskConvert16RGBA4444DE16RGB565SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk16RGB565SERedPosition   + (fsk16RGB565SERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16RGB565SEGreenPosition + (fsk16RGB565SEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16RGB565SEGreenPosition + (fsk16RGB565SEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16RGB565SERedBits  -4, fsk16RGB565SERedPosition  )							\
																|	fskFieldMask(fsk16RGB565SEGreenBits-4, fsk16RGB565SEGreenPosition)							\
																|	fskFieldMask(fsk16RGB565SEBlueBits -4, fsk16RGB565SEBluePosition )							\
																)																								\
											)

#define fskConvert16RGBA4444DE16RGB565DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk16RGB565DERedPosition   + (fsk16RGB565DERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16RGB565DEGreenPosition + (fsk16RGB565DEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16RGB565DEGreenPosition + (fsk16RGB565DEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16RGB565DERedBits  -4, fsk16RGB565DERedPosition  )							\
																|	fskFieldMask(fsk16RGB565DEGreenBits-4, fsk16RGB565DEGreenPosition)							\
																|	fskFieldMask(fsk16RGB565DEBlueBits -4, fsk16RGB565DEBluePosition )							\
																),																								\
												p |= (p >> 16)																									\
											)

#define fskConvert16RGBA4444DE16BGR565SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk16BGR565SERedPosition   + (fsk16BGR565SERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16BGR565SEGreenPosition + (fsk16BGR565SEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16BGR565SEGreenPosition + (fsk16BGR565SEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16BGR565SERedBits  -4, fsk16BGR565SERedPosition  )							\
																|	fskFieldMask(fsk16BGR565SEGreenBits-4, fsk16BGR565SEGreenPosition)							\
																|	fskFieldMask(fsk16BGR565SEBlueBits -4, fsk16BGR565SEBluePosition )							\
																)																								\
											)

#define fskConvert16RGBA4444DE16BGR565DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   fsk16BGR565DERedPosition   + (fsk16BGR565DERedBits  -4)) 	\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16BGR565DEGreenPosition + (fsk16BGR565DEGreenBits-4))		\
												| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, fsk16BGR565DEGreenPosition + (fsk16BGR565DEBlueBits -4)),	\
												p |= (p >> 4) & (	fskFieldMask(fsk16BGR565DERedBits  -4, fsk16BGR565DERedPosition  )							\
																|	fskFieldMask(fsk16BGR565DEGreenBits-4, fsk16BGR565DEGreenPosition)							\
																|	fskFieldMask(fsk16BGR565DEBlueBits -4, fsk16BGR565DEBluePosition )							\
																),																								\
												p |= (p >> 16)																									\
											)

#define fskConvert16RGBA4444DE16RGB5515SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,     fsk16RGB5515SERedPosition   + (fsk16RGB5515SERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition,   fsk16RGB5515SEGreenPosition + (fsk16RGB5515SEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,    fsk16RGB5515SEBluePosition  + (fsk16RGB5515SEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444DEAlphaPosition+3, fsk16RGB5515SEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16RGB5515SERedBits  -4, fsk16RGB5515SERedPosition  )						\
																	|	fskFieldMask(fsk16RGB5515SEGreenBits-4, fsk16RGB5515SEGreenPosition)						\
																	|	fskFieldMask(fsk16RGB5515SEBlueBits -4, fsk16RGB5515SEBluePosition )						\
																	)																								\
												)

#define fskConvert16RGBA4444DE16RGB5515DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,     fsk16RGB5515DERedPosition   + (fsk16RGB5515DERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition,   fsk16RGB5515DEGreenPosition + (fsk16RGB5515DEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,    fsk16RGB5515DEBluePosition  + (fsk16RGB5515DEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444DEAlphaPosition+3, fsk16RGB5515DEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16RGB5515DERedBits  -4, fsk16RGB5515DERedPosition  )						\
																	|	fskFieldMask(fsk16RGB5515DEGreenBits-4, fsk16RGB5515DEGreenPosition)						\
																	|	fskFieldMask(fsk16RGB5515DEBlueBits -4, fsk16RGB5515DEBluePosition )						\
																	),																								\
													p |= (p >> 16)																									\
												)

#define fskConvert16RGBA4444DE16BGR5515SE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,     fsk16BGR5515SERedPosition   + (fsk16BGR5515SERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition,   fsk16BGR5515SEGreenPosition + (fsk16BGR5515SEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,    fsk16BGR5515SEBluePosition  + (fsk16BGR5515SEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444DEAlphaPosition+3, fsk16BGR5515SEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16BGR5515SERedBits  -4, fsk16BGR5515SERedPosition  )						\
																	|	fskFieldMask(fsk16BGR5515SEGreenBits-4, fsk16BGR5515SEGreenPosition)						\
																	|	fskFieldMask(fsk16BGR5515SEBlueBits -4, fsk16BGR5515SEBluePosition )						\
																	)																								\
												)

#define fskConvert16RGBA4444DE16BGR5515DE(p)	(p	= FskMoveField(p, 4, fsk16RGBA4444DERedPosition,     fsk16BGR5515DERedPosition   + (fsk16BGR5515DERedBits  -4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition,   fsk16BGR5515DEGreenPosition + (fsk16BGR5515DEGreenBits-4))	\
													| FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,    fsk16BGR5515DEBluePosition  + (fsk16BGR5515DEBlueBits -4))	\
													| FskMoveField(p, 1, fsk16RGBA4444DEAlphaPosition+3, fsk16BGR5515DEAlphaPosition							),	\
													p |= (p >> 4) & (	fskFieldMask(fsk16BGR5515DERedBits  -4, fsk16BGR5515DERedPosition  )						\
																	|	fskFieldMask(fsk16BGR5515DEGreenBits-4, fsk16BGR5515DEGreenPosition)						\
																	|	fskFieldMask(fsk16BGR5515DEBlueBits -4, fsk16BGR5515DEBluePosition )						\
																	),																								\
													p |= (p >> 16)																									\
												)



/********************************************************************************
 * 16 -> 8G
 ********************************************************************************/

#define fskConvert16RGB565SE8G(p)			(p = (UInt8)(																\
												(	(	FskMoveField(p, 5,   fsk16RGB565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16RGB565DE8G(p)			(p |= p << 16,																\
											p = (UInt8)(																\
												(	(	FskMoveField(p, 5,   fsk16RGB565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16RGB565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16RGB565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16RGB565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16BGR565SE8G(p)			(p = (UInt8)(																\
												(	(	FskMoveField(p, 5,   fsk16BGR565SERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565SEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565SEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565SEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16BGR565DE8G(p)			(p |= p << 16,																\
											p = (UInt8)(																\
												(	(	FskMoveField(p, 5,   fsk16BGR565DERedPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 6,   fsk16BGR565DEGreenPosition,         8-6)	\
													|	FskMoveField(p, 8-6, fsk16BGR565DEGreenPosition + 6-(8-6), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5,   fsk16BGR565DEBluePosition,          8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR565DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16RGB5515SE8G(p)			(p = (UInt8)(																\
												(	(	FskMoveField(p, 5, fsk16RGB5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16RGB5515DE8G(p)			(p |= p << 16,																\
											p = (UInt8)(																\
												(	(	FskMoveField(p, 5, fsk16RGB5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16RGB5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16RGB5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16BGR5515SE8G(p)			(p = (UInt8)(																\
												(	(	FskMoveField(p, 5, fsk16BGR5515SERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515SEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515SEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)
#define fskConvert16BGR5515DE8G(p)			(p |= p << 16,																\
											p = (UInt8)(																\
												(	(	FskMoveField(p, 5, fsk16BGR5515DERedPosition,             8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DERedPosition   + 5-(8-5), 0)	\
													) * fskRtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEGreenPosition,           8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEGreenPosition + 5-(8-5), 0)	\
													) * fskGtoYCoeff													\
												+	(	FskMoveField(p, 5, fsk16BGR5515DEBluePosition,            8-5)	\
													|	FskMoveField(p, 8-5, fsk16BGR5515DEBluePosition  + 5-(8-5), 0)	\
													) * fskBtoYCoeff													\
												+	(1 << (fskToYCoeffShift - 1))										\
												) >> fskToYCoeffShift)													\
											)

#define fskConvert16RGBA4444SE8G(p)			(p = ((	FskMoveField(p, 4, fsk16RGBA4444SERedPosition,   0) * (0x11 * fskRtoYCoeff)	\
												+	FskMoveField(p, 4, fsk16RGBA4444SEGreenPosition, 0) * (0x11 * fskGtoYCoeff)	\
												+	FskMoveField(p, 4, fsk16RGBA4444SEBluePosition,  0) * (0x11 * fskBtoYCoeff)	\
												+	(1 << (fskToYCoeffShift - 1))												\
												) >> fskToYCoeffShift)															\
											)

#define fskConvert16RGBA4444DE8G(p)			(p = ((	FskMoveField(p, 4, fsk16RGBA4444DERedPosition,   0) * (0x11 * fskRtoYCoeff)	\
												+	FskMoveField(p, 4, fsk16RGBA4444DEGreenPosition, 0) * (0x11 * fskGtoYCoeff)	\
												+	FskMoveField(p, 4, fsk16RGBA4444DEBluePosition,  0) * (0x11 * fskBtoYCoeff)	\
												+	(1 << (fskToYCoeffShift - 1))												\
												) >> fskToYCoeffShift)															\
											)

#define fskConvert16AG8G(p)					(p = (UInt8)FskMoveField(p, 8, fsk16AGGrayPosition, 0))

#define fskConvert16GA8G(p)					(p = (UInt8)FskMoveField(p, 8, fsk16GAGrayPosition, 0))



/********************************************************************************
 * 16 -> 8A
 ********************************************************************************/

#define fskConvert16RGB565SE8A(p)			(p = fskDefaultAlpha)
#define fskConvert16RGB565DE8A(p)			(p = fskDefaultAlpha)
#define fskConvert16BGR565SE8A(p)			(p = fskDefaultAlpha)
#define fskConvert16BGR565DE8A(p)			(p = fskDefaultAlpha)
#define fskConvert16RGB5515SE8A(p)			(p = FskMoveField(p, 1, fsk16RGB5515SEAlphaPosition,  0) * 255)
#define fskConvert16RGB5515DE8A(p)			(p = FskMoveField(p, 1, fsk16RGB5515DEAlphaPosition,  0) * 255)
#define fskConvert16BGR5515SE8A(p)			(p = FskMoveField(p, 1, fsk16BGR5515SEAlphaPosition,  0) * 255)
#define fskConvert16BGR5515DE8A(p)			(p = FskMoveField(p, 1, fsk16BGR5515DEAlphaPosition,  0) * 255)
#define fskConvert16RGBA4444SE8A(p)			(p = FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, 0) * 17 )
#define fskConvert16RGBA4444DE8A(p)			(p = FskMoveField(p, 4, fsk16RGBA4444SEAlphaPosition, 0) * 17 )
#define fskConvert16AG8A(p)					(p = FskMoveField(p, 8, fsk16AGAlphaPosition,         0)      )
#define fskConvert16GA8A(p)					(p = FskMoveField(p, 8, fsk16GAAlphaPosition,         0)      )



/********************************************************************************
 * 8G -> 32
 ********************************************************************************/

#define fskConvert8G32ARGB(p)			(p =	(fskDefaultAlpha << fsk32ARGBAlphaPosition)	\
											|	(p               << fsk32ARGBRedPosition  )	\
											|	(p               << fsk32ARGBGreenPosition)	\
											|	(p               << fsk32ARGBBluePosition )	\
										)
#define fskConvert8G32BGRA(p)			(p =	(fskDefaultAlpha << fsk32BGRAAlphaPosition)	\
											|	(p               << fsk32BGRARedPosition  )	\
											|	(p               << fsk32BGRAGreenPosition)	\
											|	(p               << fsk32BGRABluePosition )	\
										)
#define fskConvert8G32ABGR(p)			(p =	(fskDefaultAlpha << fsk32ABGRAlphaPosition)	\
											|	(p               << fsk32ABGRRedPosition  )	\
											|	(p               << fsk32ABGRGreenPosition)	\
											|	(p               << fsk32ABGRBluePosition )	\
										)
#define fskConvert8G32RGBA(p)			(p =	(fskDefaultAlpha << fsk32RGBAAlphaPosition)	\
											|	(p               << fsk32RGBARedPosition  )	\
											|	(p               << fsk32RGBAGreenPosition)	\
											|	(p               << fsk32RGBABluePosition )	\
										)
#define fskConvert8G32A16RGB565SE(p)	(p =	(fskDefaultAlpha << fsk32A16RGB565SEAlphaPosition)			\
											|	((p >> 3)               << fsk32A16RGB565SERedPosition  )	\
											|	((p >> 2)               << fsk32A16RGB565SEGreenPosition)	\
											|	((p >> 3)               << fsk32A16RGB565SEBluePosition )	\
										)



/********************************************************************************
 * 8G -> 24
 ********************************************************************************/

#define fskConvert8G24RGB(p,q)			(	((UInt8*)(&(q)))[fsk24RGBRedPosition  ] = (UInt8)(p),	\
											((UInt8*)(&(q)))[fsk24RGBGreenPosition] = (UInt8)(p),	\
											((UInt8*)(&(q)))[fsk24RGBBluePosition ] = (UInt8)(p)	\
										)
#define fskConvert8G24BGR(p,q)			(	((UInt8*)(&(q)))[fsk24BGRRedPosition  ] = (UInt8)(p),	\
											((UInt8*)(&(q)))[fsk24BGRGreenPosition] = (UInt8)(p),	\
											((UInt8*)(&(q)))[fsk24BGRBluePosition ] = (UInt8)(p)	\
										)


/********************************************************************************
 * 8G -> 16
 ********************************************************************************/

#define fskConvert8G16RGB565SE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16RGB565SERedPosition)	\
											|	FskMoveField(p, 6, (8-6), fsk16RGB565SEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16RGB565SEBluePosition)	\
										)
#define fskConvert8G16RGB565DE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16RGB565DERedPosition)	\
											|	FskMoveField(p, 6, (8-6), fsk16RGB565DEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16RGB565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */							\
										)
#define fskConvert8G16BGR565SE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16BGR565SERedPosition)	\
											|	FskMoveField(p, 6, (8-6), fsk16BGR565SEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16BGR565SEBluePosition)	\
										)
#define fskConvert8G16BGR565DE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16BGR565DERedPosition)	\
											|	FskMoveField(p, 6, (8-6), fsk16BGR565DEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16BGR565DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */							\
										)
#define fskConvert8G16RGB5515SE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16RGB5515SERedPosition)	\
											|	FskMoveField(p, 5, (8-5), fsk16RGB5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16RGB5515SEBluePosition)	\
										)
#define fskConvert8G16RGB5515DE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16RGB5515DERedPosition)	\
											|	FskMoveField(p, 5, (8-5), fsk16RGB5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16RGB5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */							\
										)
#define fskConvert8G16BGR5515SE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16BGR5515SERedPosition)	\
											|	FskMoveField(p, 5, (8-5), fsk16BGR5515SEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16BGR5515SEBluePosition)	\
										)
#define fskConvert8G16BGR5515DE(p)		(p =	FskMoveField(p, 5, (8-5),   fsk16BGR5515DERedPosition)	\
											|	FskMoveField(p, 5, (8-5), fsk16BGR5515DEGreenPosition)	\
											|	FskMoveField(p, 5, (8-5),  fsk16BGR5515DEBluePosition),	\
											p |= (p >> 16)	/* , p &= 0xFFFF */							\
										)

#define fskConvert8G16AG(p)				(p =	(fskDefaultAlpha << fsk16AGAlphaPosition)				\
											|	(p << fsk16AGGrayPosition)								\
										)


#define fskConvert8G16GA(p)				(p =	(fskDefaultAlpha << fsk16GAAlphaPosition)				\
											|	(p << fsk16GAGrayPosition)								\
										)

#define fskConvert8G16RGBA4444SE(p)		(p	= (fskDefaultAlpha4 <<  fsk16RGBA4444SEAlphaPosition)	\
											| FskMoveField(p, 4, 4, fsk16RGBA4444SERedPosition)		\
											| FskMoveField(p, 4, 4, fsk16RGBA4444SEGreenPosition)	\
											| FskMoveField(p, 4, 4, fsk16RGBA4444SEBluePosition)	\
										)

#define fskConvert8G16RGBA4444DE(p)		(p	= (fskDefaultAlpha4 <<  fsk16RGBA4444DEAlphaPosition)	\
											| FskMoveField(p, 4, 4, fsk16RGBA4444DERedPosition)		\
											| FskMoveField(p, 4, 4, fsk16RGBA4444DEGreenPosition)	\
											| FskMoveField(p, 4, 4, fsk16RGBA4444DEBluePosition)	\
										)

/********************************************************************************
 * 8 --> 8
 ********************************************************************************/

#define fskConvert8G8A(p)				(p = fskDefaultAlpha)
#define fskConvert8A8A(p)				/* Identity */


/********************************************************************************
 * Fixed -> 32
 ********************************************************************************/

#define fskConvertFixed32ARGB(r, g, b, n, p)		(p =	(fskDefaultAlpha                                           << fsk32ARGBAlphaPosition)	\
														|	FskMoveField(r, fsk32ARGBRedBits,   (n) - fsk32ARGBRedBits  , fsk32ARGBRedPosition)		\
														|	FskMoveField(g, fsk32ARGBGreenBits, (n) - fsk32ARGBGreenBits, fsk32ARGBGreenPosition)	\
														|	FskMoveField(b, fsk32ARGBBlueBits,  (n) - fsk32ARGBBlueBits , fsk32ARGBBluePosition)	\
													)
#define fskConvertFixed32BGRA(r, g, b, n, p)		(p =	(fskDefaultAlpha                                           << fsk32BGRAAlphaPosition)	\
														|	FskMoveField(r, fsk32BGRARedBits,   (n) - fsk32BGRARedBits  , fsk32BGRARedPosition)		\
														|	FskMoveField(g, fsk32BGRAGreenBits, (n) - fsk32BGRAGreenBits, fsk32BGRAGreenPosition)	\
														|	FskMoveField(b, fsk32BGRABlueBits,  (n) - fsk32BGRABlueBits , fsk32BGRABluePosition)	\
													)
#define fskConvertFixed32ABGR(r, g, b, n, p)		(p =	(fskDefaultAlpha                                           << fsk32ABGRAlphaPosition)	\
														|	FskMoveField(r, fsk32ABGRRedBits,   (n) - fsk32ABGRRedBits  , fsk32ABGRRedPosition)		\
														|	FskMoveField(g, fsk32ABGRGreenBits, (n) - fsk32ABGRGreenBits, fsk32ABGRGreenPosition)	\
														|	FskMoveField(b, fsk32ABGRBlueBits,  (n) - fsk32ABGRBlueBits , fsk32ABGRBluePosition)	\
													)
#define fskConvertFixed32RGBA(r, g, b, n, p)		(p =	(fskDefaultAlpha                                           << fsk32RGBAAlphaPosition)	\
														|	FskMoveField(r, fsk32RGBARedBits,   (n) - fsk32RGBARedBits  , fsk32RGBARedPosition)		\
														|	FskMoveField(g, fsk32RGBAGreenBits, (n) - fsk32RGBAGreenBits, fsk32RGBAGreenPosition)	\
														|	FskMoveField(b, fsk32RGBABlueBits,  (n) - fsk32RGBABlueBits , fsk32RGBABluePosition)	\
													)
#define fskConvertFixed32A16RGB565SE(r, g, b, n, p)		(p =	(fskDefaultAlpha                                                     << fsk32A16RGB565SEAlphaPosition)	\
														|	FskMoveField(r, fsk32A16RGB565SERedBits,   (n) - fsk32A16RGB565SERedBits  , fsk32A16RGB565SERedPosition)	\
														|	FskMoveField(g, fsk32A16RGB565SEGreenBits, (n) - fsk32A16RGB565SEGreenBits, fsk32A16RGB565SEGreenPosition)	\
														|	FskMoveField(b, fsk32A16RGB565SEBlueBits,  (n) - fsk32A16RGB565SEBlueBits , fsk32A16RGB565SEBluePosition)	\
													)


/********************************************************************************
 * Fixed -> 16
 ********************************************************************************/

#define fskConvertFixed16RGB565SE(r, g, b, n, p)	(p =	FskMoveField(r, fsk16RGB565SERedBits,   (n) - fsk16RGB565SERedBits  , fsk16RGB565SERedPosition)		\
														|	FskMoveField(g, fsk16RGB565SEGreenBits, (n) - fsk16RGB565SEGreenBits, fsk16RGB565SEGreenPosition)	\
														|	FskMoveField(b, fsk16RGB565SEBlueBits,  (n) - fsk16RGB565SEBlueBits , fsk16RGB565SEBluePosition)	\
													)
#define fskConvertFixed16RGB565DE(r, g, b, n, p)	(p =	FskMoveField(r, fsk16RGB565DERedBits,   (n) - fsk16RGB565DERedBits  , fsk16RGB565DERedPosition)		\
														|	FskMoveField(g, fsk16RGB565DEGreenBits, (n) - fsk16RGB565DEGreenBits, fsk16RGB565DEGreenPosition)	\
														|	FskMoveField(b, fsk16RGB565DEBlueBits,  (n) - fsk16RGB565DEBlueBits , fsk16RGB565DEBluePosition),	\
														p |= (p >> 16)	/* , p &= 0xFFFF */																		\
													)
#define fskConvertFixed16BGR565SE(r, g, b, n, p)	(p =	FskMoveField(r, fsk16BGR565SERedBits,   (n) - fsk16BGR565SERedBits  , fsk16BGR565SERedPosition)		\
														|	FskMoveField(g, fsk16BGR565SEGreenBits, (n) - fsk16BGR565SEGreenBits, fsk16BGR565SEGreenPosition)	\
														|	FskMoveField(b, fsk16BGR565SEBlueBits,  (n) - fsk16BGR565SEBlueBits , fsk16BGR565SEBluePosition)	\
													)
#define fskConvertFixed16BGR565DE(r, g, b, n, p)	(p =	FskMoveField(r, fsk16BGR565DERedBits,   (n) - fsk16BGR565DERedBits  , fsk16BGR565DERedPosition)		\
														|	FskMoveField(g, fsk16BGR565DEGreenBits, (n) - fsk16BGR565DEGreenBits, fsk16BGR565DEGreenPosition)	\
														|	FskMoveField(b, fsk16BGR565DEBlueBits,  (n) - fsk16BGR565DEBlueBits , fsk16BGR565DEBluePosition),	\
														p |= (p >> 16)	/* , p &= 0xFFFF */																		\
													)
#define fskConvertFixed16RGB5515SE(r, g, b, n, p)	(p =	(fskDefaultAlpha1                                                    << fsk16RGB5515SEAlphaPosition)	\
														|	FskMoveField(r, fsk16RGB5515SERedBits,   (n) - fsk16RGB5515SERedBits  , fsk16RGB5515SERedPosition)		\
														|	FskMoveField(g, fsk16RGB5515SEGreenBits, (n) - fsk16RGB5515SEGreenBits, fsk16RGB5515SEGreenPosition)	\
														|	FskMoveField(b, fsk16RGB5515SEBlueBits,  (n) - fsk16RGB5515SEBlueBits , fsk16RGB5515SEBluePosition)		\
													)
#define fskConvertFixed16RGB5515DE(r, g, b, n, p)	(p =	(fskDefaultAlpha1                                                    << fsk16RGB5515DEAlphaPosition)	\
														|	FskMoveField(r, fsk16RGB5515DERedBits,   (n) - fsk16RGB5515DERedBits  , fsk16RGB5515DERedPosition)		\
														|	FskMoveField(g, fsk16RGB5515DEGreenBits, (n) - fsk16RGB5515DEGreenBits, fsk16RGB5515DEGreenPosition)	\
														|	FskMoveField(b, fsk16RGB5515DEBlueBits,  (n) - fsk16RGB5515DEBlueBits , fsk16RGB5515DEBluePosition),	\
														p |= (p >> 16)	/* , p &= 0xFFFF */																			\
													)
#define fskConvertFixed16BGR5515SE(r, g, b, n, p)	(p =	(fskDefaultAlpha1                                                    << fsk16BGR5515SEAlphaPosition)	\
														|	FskMoveField(r, fsk16BGR5515SERedBits,   (n) - fsk16BGR5515SERedBits  , fsk16BGR5515SERedPosition)		\
														|	FskMoveField(g, fsk16BGR5515SEGreenBits, (n) - fsk16BGR5515SEGreenBits, fsk16BGR5515SEGreenPosition)	\
														|	FskMoveField(b, fsk16BGR5515SEBlueBits,  (n) - fsk16BGR5515SEBlueBits , fsk16BGR5515SEBluePosition)		\
													)
#define fskConvertFixed16BGR5515DE(r, g, b, n, p)	(p =	(fskDefaultAlpha1                                                    << fsk16BGR5515DEAlphaPosition)	\
														|	FskMoveField(r, fsk16BGR5515DERedBits,   (n) - fsk16BGR5515DERedBits  , fsk16BGR5515DERedPosition)		\
														|	FskMoveField(g, fsk16BGR5515DEGreenBits, (n) - fsk16BGR5515DEGreenBits, fsk16BGR5515DEGreenPosition)	\
														|	FskMoveField(b, fsk16BGR5515DEBlueBits,  (n) - fsk16BGR5515DEBlueBits , fsk16BGR5515DEBluePosition),	\
														p |= (p >> 16)	/* , p &= 0xFFFF */																			\
													)

#define fskConvertFixed16RGBA4444SE(r, g, b, n, p)	(p	= (fskDefaultAlpha4                                                      << fsk16RGBA4444SEAlphaPosition)	\
														| FskMoveField(r, fsk16RGBA4444SERedBits,   (n) - fsk16RGBA4444SERedBits,   fsk16RGBA4444SERedPosition  )	\
														| FskMoveField(r, fsk16RGBA4444SEGreenBits, (n) - fsk16RGBA4444SEGreenBits, fsk16RGBA4444SEGreenPosition)	\
														| FskMoveField(r, fsk16RGBA4444SEBlueBits,  (n) - fsk16RGBA4444SEBlueBits,  fsk16RGBA4444SEBluePosition )	\
													)

#define fskConvertFixed16RGBA4444DE(r, g, b, n, p)	(p	= (fskDefaultAlpha4                                                      << fsk16RGBA4444DEAlphaPosition)	\
														| FskMoveField(r, fsk16RGBA4444DERedBits,   (n) - fsk16RGBA4444DERedBits,   fsk16RGBA4444DERedPosition  )	\
														| FskMoveField(r, fsk16RGBA4444DEGreenBits, (n) - fsk16RGBA4444DEGreenBits, fsk16RGBA4444DEGreenPosition)	\
														| FskMoveField(r, fsk16RGBA4444DEBlueBits,  (n) - fsk16RGBA4444DEBlueBits,  fsk16RGBA4444DEBluePosition )	\
													)


/********************************************************************************
 * Fixed -> 24
 ********************************************************************************/

#define fskConvertFixed24RGB(r, g, b, n, p)			(	((UInt8*)(&(p)))[fsk24RGBRedPosition  ] = (UInt8)((r) >> ((n) - fsk24RGBRedBits  )),	\
														((UInt8*)(&(p)))[fsk24RGBGreenPosition] = (UInt8)((g) >> ((n) - fsk24RGBGreenBits)),	\
														((UInt8*)(&(p)))[fsk24RGBBluePosition ] = (UInt8)((b) >> ((n) - fsk24RGBBlueBits ))		\
													)


#define fskConvertFixed24BGR(r, g, b, n, p)			(	((UInt8*)(&(p)))[fsk24BGRRedPosition  ] = (UInt8)((r) >> ((n) - fsk24BGRRedBits  )),	\
														((UInt8*)(&(p)))[fsk24BGRGreenPosition] = (UInt8)((g) >> ((n) - fsk24BGRGreenBits)),	\
														((UInt8*)(&(p)))[fsk24BGRBluePosition ] = (UInt8)((b) >> ((n) - fsk24BGRBlueBits ))		\
													)


/********************************************************************************
 * Fixed -> 8
 ********************************************************************************/

#define fskConvertFixed8G(r, g, b, n, p)	(p) = (((((r) >> ((n) - fsk8GGrayBits)) * fskRtoYCoeff)		\
												+	(((g) >> ((n) - fsk8GGrayBits)) * fskGtoYCoeff)		\
												+	(((b) >> ((n) - fsk8GGrayBits)) * fskBtoYCoeff)		\
												+	(1 << (fskToYCoeffShift - 1))						\
												) >> fskToYCoeffShift)									\


/********************************************************************************
 * RGB <--> YUV Conversion, from Poynton + 2008-10-20 Errata
 ********************************************************************************/

/* Convert CCIR-601 {Y, Cb, Cr} to {R, G, B} */
#define fskConvertYUV601RGB(y, u, v, r, g, b) do {														\
	SInt32 yp = ((SInt32)(y) - 16) * 1192 + (1 << 9), up = (SInt32)(u) - 128, vp = (SInt32)(v) - 128;	\
	r = (yp + 1634 * vp) >> 10; g = (yp - 401 * up - 832 * vp) >> 10; b = (yp + 2066 * up) >> 10;		\
	if (r < 0) r = 0; else if (r > 255) r = 255;	\
	if (g < 0) g = 0; else if (g > 255) g = 255;	\
	if (b < 0) b = 0; else if (b > 255) b = 255;	\
} while(0)

/* Convert {R, G, B} to CCIR-601 {Y, Cb, Cr} */
#define fskConvertRGBYUV601(r, g, b, y, u, v) do {					\
	y = (( 263 * r + 516 * g + 100 * b + (1 << 9)) >> 10) +  16;	\
	u = ((-152 * r - 298 * g + 450 * b + (1 << 9)) >> 10) + 128;	\
	v = (( 450 * r - 377 * g -  73 * b + (1 << 9)) >> 10) + 128;	\
	if (y < 0) y = 0; else if (y > 255) y = 255;					\
	if (u < 0) u = 0; else if (u > 255) u = 255;					\
	if (v < 0) v = 0; else if (v > 255) v = 255;					\
} while(0)


/* Convert CCIR-709 {Y, Cb, Cr} to {R, G, B} */
#define fskConvertYUV709RGB(y, u, v, r, g, b) do {														\
	SInt32 yp = ((SInt32)(y) - 16) * 1192 + (1 << 9), up = (SInt32)(u) - 128, vp = (SInt32)(v) - 128;	\
	r = (yp + 1836 * vp) >> 10; g = (yp - 218 * up - 546 * vp) >> 10; b = (yp + 2163 * up) >> 10;		\
	if (r < 0) r = 0; else if (r > 255) r = 255;	\
	if (g < 0) g = 0; else if (g > 255) g = 255;	\
	if (b < 0) b = 0; else if (b > 255) b = 255;	\
} while(0)

/* Convert {R, G, B} to CCIR-709 {Y, Cb, Cr} */
#define fskConvertRGBYUV709(r, g, b, y, u, v) do {					\
	y = (( 187 * r + 629 * g +  63 * b + (1 << 9)) >> 10) +  16;	\
	u = ((-103 * r - 347 * g + 450 * b + (1 << 9)) >> 10) + 128;	\
	v = (( 450 * r - 409 * g -  41 * b + (1 << 9)) >> 10) + 128;	\
	if (y < 0) y = 0; else if (y > 255) y = 255;					\
	if (u < 0) u = 0; else if (u > 255) u = 255;					\
	if (v < 0) v = 0; else if (v > 255) v = 255;					\
} while(0)


#define fskConvertYUVRGB(y, u, v, r, g, b)	fskConvertYUV601RGB(y, u, v, r, g, b)
#define fskConvertRGBYUV(r, g, b, y, u, v)	fskConvertRGBYUV601(r, g, b, y, u, v)

/********************************************************************************
 * Conversion procs.
 ********************************************************************************/

UInt32			FskConvertYUV42032ARGB(      UInt8 y, UInt8 u, UInt8 v);
UInt32			FskConvertYUV42032BGRA(      UInt8 y, UInt8 u, UInt8 v);
UInt32			FskConvertYUV42032ABGR(      UInt8 y, UInt8 u, UInt8 v);
UInt32			FskConvertYUV42032RGBA(      UInt8 y, UInt8 u, UInt8 v);
Fsk24BitType	FskConvertYUV42024RGB(       UInt8 y, UInt8 u, UInt8 v);
Fsk24BitType	FskConvertYUV42024BGR(       UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGB565SE(  UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016BGR565SE(  UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGB565DE(  UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016BGR565DE(  UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGB5515SE( UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016BGR5515SE( UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGB5515DE( UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016BGR5515DE( UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGBA4444SE(UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016RGBA4444DE(UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016AG(        UInt8 y, UInt8 u, UInt8 v);
UInt16			FskConvertYUV42016GA(        UInt8 y, UInt8 u, UInt8 v);
UInt8			FskConvertYUV4208G(          UInt8 y, UInt8 u, UInt8 v);
UInt32			FskConvertYUV42032A16RGB565SE(UInt8 y, UInt8 u, UInt8 v);
FskAPI(void)	FskConvertRGBHSL(const UInt8 *rgb, UInt8 *hsl);
FskAPI(void)	FskConvertHSLRGB(const UInt8 *hsl, UInt8 *rgb);



/********************************************************************************
 * Bilinear interpolation, with 4 bit fractions.
 * Make sure to round to the nearest 1/16, in the same way that you
 * round to the nearest 1/2 for point sampling.
 ********************************************************************************/

FskAPI(UInt32)			FskBilerp32(               UInt32 di, UInt32 dj, const UInt32       *s, SInt32 rb);
#if 0
FskAPI(UInt32)			FskBilerp32_neon_di_dj(    UInt32 di, UInt32 dj, const UInt32       *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp32_neon_di(       UInt32 di, const UInt32       *s);
FskAPI(UInt32)			FskBilerp32_neon_dj(       UInt32 dj, const UInt32       *s, SInt32 rb);
#endif
FskAPI(UInt32)			FskBilerp32A16RGB565SE(    UInt32 di, UInt32 dj, const UInt32       *s, SInt32 rb);
FskAPI(Fsk24BitType)	FskBilerp24(               UInt32 di, UInt32 dj, const Fsk24BitType *s, SInt32 rb);
FskAPI(UInt16)			FskBilerp565SE(            UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt16)			FskBilerp565DE(            UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt16)			FskBilerp5515SE(           UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt16)			FskBilerp5515DE(           UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt8)			FskBilerp8(                UInt32 di, UInt32 dj, const UInt8        *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp16RGB565SE32ARGB( UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp16RGB565DE32ARGB( UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp16RGB565SE32A16RGB565SE(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp16RGBA444SE32ARGB(UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt32)			FskBilerp16RGBA444DE32ARGB(UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);
FskAPI(UInt16)			FskBilerp16XX(             UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);	/* for 2 8-bit components, usually alpha and gray */
FskAPI(UInt16)			FskBilerp16XXXX(           UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);	/* for 4 4-bit components */
FskAPI(UInt32)			FskBilerp16XXXX32XXXX(     UInt32 di, UInt32 dj, const UInt16       *s, SInt32 rb);	/* Interpolate 4 4-bit components to 4 8-bit components */
FskAPI(UInt16)			FskBilerpUV(               UInt32 di, UInt32 dj, const UInt8 *u, const UInt8 *v, SInt32 rb);

/********************************************************************************
 * Blending between foreground and background
 ********************************************************************************/

FskAPI(void)	FskBlend32(				UInt32 *d,       UInt32 p, UInt8 opacity);
FskAPI(void)	FskBlend32A16RGB565SE(	UInt32 *d,       UInt32 p, UInt8 opacity);
FskAPI(void)	FskBlend24(       Fsk24BitType *d, Fsk24BitType p, UInt8 opacity);
FskAPI(void)	FskBlend565SE(			UInt16 *d,       UInt16 p, UInt8 opacity);
FskAPI(void)	FskBlend565DE(			UInt16 *d,       UInt16 p, UInt8 opacity);
void			FskBlend5515SE(			UInt16 *d,       UInt16 p, UInt8 opacity);
void			FskBlend5515DE(			UInt16 *d,       UInt16 p, UInt8 opacity);
void			FskBlend4444(			UInt16 *d,       UInt16 p, UInt8 opacity);
void			FskBlend88(				UInt16 *d,       UInt16 p, UInt8 opacity);
void			FskBlend8(		        UInt8 *d,        UInt8 p, UInt8 opacity);

FskAPI(void)	FskFastBlend565SE(UInt16 *d, UInt16 p, UInt8 opacity);						/* This uses a 5 rather than a 6 bit opacity */


/********************************************************************************
 * Alpha blending between foreground and background, with opacity control
 ********************************************************************************/

void			FskAlphaBlendA32(   UInt32 *d, UInt32 p, UInt8 opacity);
FskAPI(void)	FskAlphaBlend32A(   UInt32 *d, UInt32 p, UInt8 opacity);
void			FskAlphaBlend32A16RGB565SE
								(   UInt32 *d, UInt32 p, UInt8 opacity);
void			FskAlphaBlend16AG(  UInt16 *d, UInt16 p, UInt8 opacity);
void			FskAlphaBlend16GA(  UInt16 *d, UInt16 p, UInt8 opacity);
void			FskAlphaBlend16XXXA(UInt16 *d, UInt16 p, UInt8 opacity);
void			FskAlphaBlend16XAXX(UInt16 *d, UInt16 p, UInt8 opacity);


/********************************************************************************
 * Alpha "over" operator
 ********************************************************************************/

FskAPI(void)	FskAlphaA32(   UInt32 *d, UInt32 p);
FskAPI(void)	FskAlpha32A(   UInt32 *d, UInt32 p);
FskAPI(void)	FskAlpha32A16RGB565SE
							(   UInt32 *d, UInt32 p);
void			FskAlpha16AG(  UInt16 *d, UInt16 p);
void			FskAlpha16GA(  UInt16 *d, UInt16 p);
void			FskAlpha16XXXA(UInt16 *d, UInt16 p);
void			FskAlpha16XAXX(UInt16 *d, UInt16 p);


/********************************************************************************
 * Alpha "over" operator, as described by Porter and Duff.
 ********************************************************************************/

FskAPI(UInt32)	FskAlphaStraightOver32AXXX(UInt32 dst, UInt32 src);
FskAPI(UInt32)	FskAlphaStraightOver32XXXA(UInt32 dst, UInt32 src);
FskAPI(UInt32)	FskAlphaStraightOver32A16RGB565SE(UInt32 dst, UInt32 src);



/********************************************************************************
 * Alpha scaled add operator A * X + Y
 ********************************************************************************/

/** Add a scaled portion of x to y. Helper for premultiplied alpha to 16RGB565SE.
 *	\param[in]	alpha6	6 bit fraction
 *	\param[in]	fr		the pixel pixel to be scaled and added to "to".
 *	\param[in]	to		the source pixel.
 **/
FskAPI(UInt16)	FskAXPY16RGB565SE(UInt8 alpha6, UInt16 x, UInt16 y);

/** Add a scaled portion of x to y. Helper for premultiplied alpha to 16RGB565DE.
 *	\param[in]	alpha6	6 bit fraction
 *	\param[in]	fr		the pixel pixel to be scaled and added to "to".
 *	\param[in]	to		the source pixel.
 **/
FskAPI(UInt16)	FskAXPY16RGB565DE(UInt8 alpha6, UInt16 x, UInt16 y);

/** Add a scaled portion of x to y. Helper for premultiplied alpha to 32XYZW.
 *	\param[in]	alpha	8 bit fraction
 *	\param[in]	fr		the pixel pixel to be scaled and added to "to".
 *	\param[in]	to		the source pixel.
 **/
FskAPI(UInt32)	FskAXPY8888(UInt8 alpha, UInt32 x, UInt32 y);

/** Add a scaled portion of x to y. Helper for premultiplied alpha to 24XYZ.
 *	\param[in]	alpha	8 bit fraction
 *	\param[in]	fr		the pixel pixel to be scaled and added to "to".
 *	\param[in]	to		the source pixel.
 **/
FskAPI(Fsk24BitType)	FskAXPY24(UInt8 alpha, Fsk24BitType x, Fsk24BitType y);


/********************************************************************************
 * Alpha "over" operator, premultiplied to black
 ********************************************************************************/

FskAPI(void)	FskAlphaBlackA32( UInt32 *d, UInt32 src);							/* 32ARGB --> 32ARGB, or 32ABGR --> 32ABGR */
FskAPI(void)	FskAlphaBlack32A( UInt32 *d, UInt32 src);							/* 32RGBA --> 32ARGB, or 32BGRA --> 32BGRA */
void			FskAlphaBlack16AG(UInt16 *d, UInt16 src);							/* 16AG --> 16AG */
void			FskAlphaBlack16GA(UInt16 *d, UInt16 src);							/* 16GA --> 16GA */
void			FskAlphaBlack32A16RGB565SE(UInt32 *d, UInt32 src);					/* 32A16RGB565SE --> 32A16RGB565SE */


/** Composite a premultiplied src into a 16RGB565SE dst.
 *	This performs the common cases of transparent and opaque quickly, inline, but calls a subroutine if blending is needed.
 *	\param[in,out]	d				pointer to the dst pixel.
 *	\param[in,out]	src				the source pixel. Beware: this is modified!
 *	\param[in]		SrcPixelKind	one of 32RGBA, 32BGRA, 32ABGR, 32ARGB, 32A16RGB565SE.
 **/
#define FskAlphaBlack16RGB565SE(d, src, SrcPixelKind)	do { UInt8 alfa; if (0 != (alfa = FskMoveField(src, 6, FskName3(fsk,SrcPixelKind,AlphaPosition) + FskName3(fsk,SrcPixelKind,AlphaBits) - 6, 0)))	\
							{ FskName3(fskConvert,SrcPixelKind,16RGB565SE)(src); if ((alfa = 63 - alfa) != 0) src = FskAXPY16RGB565SE(alfa, *d, (UInt16)src); *d = (UInt16)src; }} while(0)
#define FskAlphaBlack8888(d, src, SrcPixelKind, DstPixelKind)	do { UInt8 alfa; if (0 != (alfa = FskMoveField(src, FskName3(fsk,SrcPixelKind,AlphaBits), FskName3(fsk,SrcPixelKind,AlphaPosition), 0)))	\
							{ FskName3(fskConvert,SrcPixelKind,DstPixelKind)(src); if ((alfa = 255 - alfa) != 0) src = FskAXPY8888(alfa, *d, src); *d = src; }} while(0)


/********************************************************************************
 * Other Porter-Duff Composition Operators, premultiplied to black
 ********************************************************************************/

				/* Over */
#define			FskAlphaBlackSourceOverA32				FskAlphaBlackA32
#define			FskAlphaBlackSourceOver32A				FskAlphaBlack32A
#define			FskAlphaBlackSourceOver32A16RGB565SE	FskAlphaBlack32A16RGB565SE
#define			FskAlphaBlackSourceOver16AG				FskAlphaBlack16AG
#define			FskAlphaBlackSourceOver16GA				FskAlphaBlack16GA
FskAPI(void)	FskAlphaBlackDestinationOverA32(UInt32 *d, UInt32 p);
FskAPI(void)	FskAlphaBlackDestinationOver32A(UInt32 *d, UInt32 p);

		/* In */
void	FskAlphaBlackSourceInA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackSourceIn32A(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationInA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationIn32A(UInt32 *d, UInt32 p);

		/* Out */
void	FskAlphaBlackSourceOutA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackSourceOut32A(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationOutA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationOut32A(UInt32 *d, UInt32 p);

		/* Atop */
void	FskAlphaBlackSourceAtopA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackSourceAtop32A(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationAtopA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackDestinationAtop32A(UInt32 *d, UInt32 p);

		/* Lighter */
void	FskAlphaBlackLighter(UInt32 *d, UInt32 p);
#define	FskAlphaBlackLighterA32	FskAlphaBlackLighter
#define	FskAlphaBlackLighter32A	FskAlphaBlackLighter

		/* Xor */
void	FskAlphaBlackXorA32(UInt32 *d, UInt32 p);
void	FskAlphaBlackXor32A(UInt32 *d, UInt32 p);


/********************************************************************************
 * Multiply two pixels together
 ********************************************************************************/

UInt32			FskPixelMul32(      UInt32 p0, UInt32       p1);
UInt32			FskPixelMul32A16RGB565SE
							(      UInt32 p0, UInt32       p1);
Fsk24BitType	FskPixelMul24(Fsk24BitType p0, Fsk24BitType p1);
UInt16			FskPixelMul16SE(    UInt16 p0, UInt16       p1);
UInt16			FskPixelMul16DE(    UInt16 p0, UInt16       p1);
UInt16			FskPixelMul5515SE(  UInt16 p0, UInt16       p1);
UInt16			FskPixelMul5515DE(  UInt16 p0, UInt16       p1);
UInt16			FskPixelMul88(      UInt16 p0, UInt16       p1);	/* for 2 8-bit components, usually alpha and gray */
UInt16			FskPixelMul4444(    UInt16 p0, UInt16       p1);	/* for 4 4-bit components */
UInt8			FskPixelMul8(       UInt8  p0, UInt8        p1);


/********************************************************************************
 * Multiply two pixels together, but reverse the order of the components
 ********************************************************************************/

UInt32			FskPixelMul32Swap(      UInt32 p0,       UInt32 p1);
Fsk24BitType	FskPixelMul24Swap(Fsk24BitType p0, Fsk24BitType p1);
UInt16			FskPixelMul16SESwap(    UInt16 p0,       UInt16 p1);
UInt16			FskPixelMul16DESwap(    UInt16 p0,       UInt16 p1);
UInt16			FskPixelMul5515SESwap(  UInt16 p0,       UInt16 p1);
UInt16			FskPixelMul5515DESwap(  UInt16 p0,       UInt16 p1);


/********************************************************************************
 * Other Adobe composition operators, as found in the PDF specification,
 * PDF 32000-1:2008, section 11.3.5 Blend Mode.
 * Typically, these are applied componentwise, except for alpha, which always uses FskPixelScreen8.
 ********************************************************************************/

UInt8 FskPixelScreen8(    UInt8 d, UInt8 s);	/* symmetric */
UInt8 FskPixelHardLight8( UInt8 d, UInt8 s);	/* asymmetric */
UInt8 FskPixelOverlay8(   UInt8 d, UInt8 s);	/* asymmetric */
UInt8 FskPixelDarken8(    UInt8 d, UInt8 s);	/* symmetric */
UInt8 FskPixelLighten8(   UInt8 d, UInt8 s);	/* symmetric */
UInt8 FskPixelColorDodge8(UInt8 d, UInt8 s);	/* asymmetric */
UInt8 FskPixelColorBurn8( UInt8 d, UInt8 s);	/* asymmetric */
UInt8 FskPixelSoftLight8( UInt8 d, UInt8 s);	/* asymmetric */
UInt8 FskPixelDifference8(UInt8 d, UInt8 s);	/* symmetric */
UInt8 FskPixelExclusion8( UInt8 d, UInt8 s);	/* symmetric */


/********************************************************************************
 * Multiply two alphas together.
 ********************************************************************************/

FskAPI(UInt8)	FskAlphaMul(UInt8 p0, UInt8 p1);


/********************************************************************************
 * Scale a 24 bit pixel (3 components of 8 bits each) by alpha.
 ********************************************************************************/

Fsk24BitType FskAlphaScale24(UInt8 a, Fsk24BitType p);


/********************************************************************************
 * Scale a 32 bit pixel (4 components of 8 bits each) by alpha.
 ********************************************************************************/

UInt32 FskAlphaScale32(UInt8 a, UInt32 p);


/********************************************************************************
 * Scale a 16 bit pixel, with 2 components of 8 bits, by alpha.
 ********************************************************************************/

UInt16 FskAlphaScale16GA(UInt8 a, UInt16 p);


/********************************************************************************
 * Convert a straight alpha color to a premultiplied alpha color.
 ********************************************************************************/

void FskPremultiplyColorRGBA(FskConstColorRGBA fr, FskColorRGBA to);


/********************************************************************************
 * Convert FskColorRGB to the specified pixel format
 * We usually make bitmapPixel a pointer to a UInt32, but this isn't necessary
 * if you know what you're doing.
 * If the pixel format is planar, it is flattened into a chunky 1x1 pixel.
 ********************************************************************************/

FskAPI(FskErr)	FskConvertColorRGBToBitmapPixel (FskConstColorRGB  rgb,  FskBitmapFormatEnum pixelFormat, void *bitmapPixel);
FskAPI(FskErr)	FskConvertColorRGBAToBitmapPixel(FskConstColorRGBA rgba, FskBitmapFormatEnum pixelFormat, void *bitmapPixel);


/********************************************************************************
 * Convert the specified pixel format to FskColorRGB.
 * This doesn't work for planar pixels.
 ********************************************************************************/

FskAPI(FskErr)	FskConvertBitmapPixelToColorRGB( const void *bitmapPixel, FskBitmapFormatEnum pixelFormat, FskColorRGB  rgb);
FskAPI(FskErr)	FskConvertBitmapPixelToColorRGBA(const void *bitmapPixel, FskBitmapFormatEnum pixelFormat, FskColorRGBA rgba);


/********************************************************************************
 * Get information about bitmap pixel formats.
 ********************************************************************************/

FskAPI(UInt8) FskBitmapFormatDepth        (FskBitmapFormatEnum pixelFormat);	/**< Depth of a pixel.			\param[in] pixelFormat pixel format. \return number of bits per pixel.	*/
FskAPI(UInt8) FskBitmapFormatPixelBytes   (FskBitmapFormatEnum pixelFormat);	/**< Bytes per pixel.			\param[in] pixelFormat pixel format. \return number of bytes per pixel.	*/
FskAPI(UInt8) FskBitmapFormatAlphaPosition(FskBitmapFormatEnum pixelFormat);	/**< Bit position of alpha.		\param[in] pixelFormat pixel format. \return position of alpha.			*/
FskAPI(UInt8) FskBitmapFormatRedPosition  (FskBitmapFormatEnum pixelFormat);	/**< Bit position of red.		\param[in] pixelFormat pixel format. \return position of red.			*/
FskAPI(UInt8) FskBitmapFormatGreenPosition(FskBitmapFormatEnum pixelFormat);	/**< Bit position of green.		\param[in] pixelFormat pixel format. \return position of green.			*/
FskAPI(UInt8) FskBitmapFormatBluePosition (FskBitmapFormatEnum pixelFormat);	/**< Bit position of blue.		\param[in] pixelFormat pixel format. \return position of blue.			*/
FskAPI(UInt8) FskBitmapFormatGrayPosition (FskBitmapFormatEnum pixelFormat);	/**< Bit position of gray.		\param[in] pixelFormat pixel format. \return position of gray.			*/
FskAPI(UInt8) FskBitmapFormatAlphaBits    (FskBitmapFormatEnum pixelFormat);	/**< Number of bits for alpha.	\param[in] pixelFormat pixel format. \return bits for alpha.			*/
FskAPI(UInt8) FskBitmapFormatRedBits      (FskBitmapFormatEnum pixelFormat);	/**< Number of bits for red.	\param[in] pixelFormat pixel format. \return bits for red.				*/
FskAPI(UInt8) FskBitmapFormatGreenBits    (FskBitmapFormatEnum pixelFormat);	/**< Number of bits for green.	\param[in] pixelFormat pixel format. \return bits for green.			*/
FskAPI(UInt8) FskBitmapFormatBlueBits     (FskBitmapFormatEnum pixelFormat);	/**< Number of bits for blue.	\param[in] pixelFormat pixel format. \return bits for blue.				*/
FskAPI(UInt8) FskBitmapFormatGrayBits     (FskBitmapFormatEnum pixelFormat);	/**< Number of bits for gray.	\param[in] pixelFormat pixel format. \return bits for gray.				*/
FskAPI(UInt8) FskBitmapFormatAlphaOffset  (FskBitmapFormatEnum pixelFormat);	/**< Byte offset of alpha.		\param[in] pixelFormat pixel format. \return offset to alpha.			*/
FskAPI(UInt8) FskBitmapFormatRedOffset    (FskBitmapFormatEnum pixelFormat);	/**< Byte offset of red.		\param[in] pixelFormat pixel format. \return offset to red.				*/
FskAPI(UInt8) FskBitmapFormatGreenOffset  (FskBitmapFormatEnum pixelFormat);	/**< Byte offset of gren.		\param[in] pixelFormat pixel format. \return offset to green.			*/
FskAPI(UInt8) FskBitmapFormatBlueOffset   (FskBitmapFormatEnum pixelFormat);	/**< Byte offset of blue.		\param[in] pixelFormat pixel format. \return offset to blue.			*/
FskAPI(UInt8) FskBitmapFormatGrayOffset   (FskBitmapFormatEnum pixelFormat);	/**< Byte offset of gray.		\param[in] pixelFormat pixel format. \return offset to gray.			*/
FskAPI(UInt8) FskBitmapFormatCanSrc       (FskBitmapFormatEnum pixelFormat);	/**< Can be used as source.		\param[in] pixelFormat pixel format. \return nonzero if true.			*/
FskAPI(UInt8) FskBitmapFormatCanDst       (FskBitmapFormatEnum pixelFormat);	/**< Can be used as destination. \param[in] pixelFormat pixel format. \return nonzero if true.			*/
FskAPI(UInt8) FskBitmapFormatPixelPacking (FskBitmapFormatEnum pixelFormat);	/**< Type of pixel packing.		\param[in] pixelFormat pixel format. \return the type of pixel packing.	*/
FskAPI(const char*) FskBitmapFormatName   (FskBitmapFormatEnum pixelFormat);	/**< Name of pixel format.		\param[in] pixelFormat pixel format. \return pixel format name string.	*/


/********************************************************************************
 * Define particular operations from more generic ones.
 ********************************************************************************/

/* Bilinear interpolation (UInt32 di, UInt32 dj, const Pixel *s, SInt32 rb) */
#define FskBilerp32ABGR						FskBilerp32
#define FskBilerp32ARGB						FskBilerp32
#define FskBilerp32BGRA						FskBilerp32
#define FskBilerp32RGBA						FskBilerp32
#define FskBilerp24BGR						FskBilerp24
#define FskBilerp24RGB						FskBilerp24
#define FskBilerp16BGR5515DE				FskBilerp5515DE
#define FskBilerp16BGR5515SE				FskBilerp5515SE
#define FskBilerp16BGR565DE					FskBilerp565DE
#define FskBilerp16BGR565SE					FskBilerp565SE
#define FskBilerp16RGB5515DE				FskBilerp5515DE
#define FskBilerp16RGB5515SE				FskBilerp5515SE
#define FskBilerp16RGB565DE					FskBilerp565DE
#define FskBilerp16RGB565SE					FskBilerp565SE
#define FskBilerp16RGBA4444DE				FskBilerp16XXXX
#define FskBilerp16RGBA4444SE				FskBilerp16XXXX
#define FskBilerp16RGBA4444SE32RGBA			FskBilerp16XXXX32XXXX
#define FskBilerp16AG						FskBilerp16XX
#define FskBilerp16GA						FskBilerp16XX
#define FskBilerp8A							FskBilerp8
#define FskBilerp8G							FskBilerp8

/* Blend (Pixel *d, Pixel p, UInt8 opacity) */
#define FskBlend32ABGR						FskBlend32
#define FskBlend32ARGB						FskBlend32
#define FskBlend32BGRA						FskBlend32
#define FskBlend32RGBA						FskBlend32
#define FskBlend24BGR						FskBlend24
#define FskBlend24RGB						FskBlend24
#define FskBlend16BGR5515DE					FskBlend5515DE
#define FskBlend16BGR5515SE					FskBlend5515SE
#define FskBlend16BGR565DE					FskBlend565DE
#define FskBlend16BGR565SE					FskBlend565SE
#define FskBlend16RGB5515DE					FskBlend5515DE
#define FskBlend16RGB5515SE					FskBlend5515SE
#define FskBlend16RGB565DE					FskBlend565DE
#define FskBlend16RGB565SE					FskBlend565SE
#define FskBlend16RGBA4444DE				FskBlend4444
#define FskBlend16RGBA4444SE				FskBlend4444
#define FskBlend16AG						FskBlend88
#define FskBlend16GA						FskBlend88
#define FskBlend8A							FskBlend8
#define FskBlend8G							FskBlend8

/* Alpha over operator (UInt32 *d, UInt32 p) */
#define FskAlpha32ABGR						FskAlphaA32
#define FskAlpha32ARGB						FskAlphaA32
#define FskAlpha32BGRA						FskAlpha32A
#define FskAlpha32RGBA						FskAlpha32A
#define FskAlpha16RGBA4444DE				FskAlpha16XAXX
#define FskAlpha16RGBA4444SE				FskAlpha16XXXA

/* Alpha blend (Pixel *d, Pixel p, UInt8 opacity) */
#define FskAlphaBlend32ABGR					FskAlphaBlendA32
#define FskAlphaBlend32ARGB					FskAlphaBlendA32
#define FskAlphaBlend32BGRA					FskAlphaBlend32A
#define FskAlphaBlend32RGBA					FskAlphaBlend32A
#define FskAlphaBlend16RGBA4444DE			FskAlphaBlend16XAXX
#define FskAlphaBlend16RGBA4444SE			FskAlphaBlend16XXXA

/* Pixel multiplication (Pixel p0, Pixel p1) */
#define FskPixelMul32ABGR					FskPixelMul32
#define FskPixelMul32ARGB					FskPixelMul32
#define FskPixelMul32BGRA					FskPixelMul32
#define FskPixelMul32RGBA					FskPixelMul32
#define FskPixelMul24BGR					FskPixelMul24
#define FskPixelMul24RGB					FskPixelMul24
#define FskPixelMul16AG						FskPixelMul88
#define FskPixelMul16GA						FskPixelMul88
#define FskPixelMul16BGR5515DE				FskPixelMul5515DE
#define FskPixelMul16BGR5515DE16RGB5515DE	FskPixelMul5515DESwap
#define FskPixelMul16BGR5515SE				FskPixelMul5515SE
#define FskPixelMul16BGR5515SE16RGB5515SE	FskPixelMul5515SESwap
#define FskPixelMul16BGR565DE				FskPixelMul16DE
#define FskPixelMul16BGR565DE16RGB565DE		FskPixelMul16DESwap
#define FskPixelMul16BGR565SE				FskPixelMul16SE
#define FskPixelMul16BGR565SE16RGB565SE		FskPixelMul16SESwap
#define FskPixelMul16RGB5515DE				FskPixelMul5515DE
#define FskPixelMul16RGB5515DE16BGR5515DE	FskPixelMul5515DESwap
#define FskPixelMul16RGB5515SE				FskPixelMul5515SE
#define FskPixelMul16RGB5515SE16BGR5515SE	FskPixelMul5515SESwap
#define FskPixelMul16RGB565DE				FskPixelMul16DE
#define FskPixelMul16RGB565DE16BGR565DE		FskPixelMul16DESwap
#define FskPixelMul16RGB565SE				FskPixelMul16SE
#define FskPixelMul16RGB565SE16BGR565SE		FskPixelMul16SESwap
#define FskPixelMul16RGBA4444DE				FskPixelMul4444
#define FskPixelMul16RGBA4444SE				FskPixelMul4444
#define FskPixelMul8A						FskPixelMul8
#define FskPixelMul8G						FskPixelMul8

/* Porter-Duff Straight Alpha Over Operator (UInt32 dst, UInt32 src) */
#define FskAlphaStraightOver32ABGR			FskAlphaStraightOver32AXXX
#define FskAlphaStraightOver32ARGB			FskAlphaStraightOver32AXXX
#define FskAlphaStraightOver32BGRA			FskAlphaStraightOver32XXXA
#define FskAlphaStraightOver32RGBA			FskAlphaStraightOver32XXXA

/* Porter-Duff Premultiplied Alpha Operators (Pixel *dst, Pixel src) */
#define	FskAlphaBlackLighter32ABGR			FskAlphaBlackLighterA32
#define	FskAlphaBlackLighter32ARGB			FskAlphaBlackLighterA32
#define	FskAlphaBlackLighter32BGRA			FskAlphaBlackLighter32A
#define	FskAlphaBlackLighter32RGBA			FskAlphaBlackLighter32A
#define FskAlphaBlackDestinationAtop32ABGR	FskAlphaBlackDestinationAtopA32
#define FskAlphaBlackDestinationAtop32ARGB	FskAlphaBlackDestinationAtopA32
#define FskAlphaBlackDestinationAtop32BGRA	FskAlphaBlackDestinationAtop32A
#define FskAlphaBlackDestinationAtop32RGBA	FskAlphaBlackDestinationAtop32A
#define FskAlphaBlackDestinationIn32ABGR	FskAlphaBlackDestinationInA32
#define FskAlphaBlackDestinationIn32ARGB	FskAlphaBlackDestinationInA32
#define FskAlphaBlackDestinationIn32BGRA	FskAlphaBlackDestinationIn32A
#define FskAlphaBlackDestinationIn32RGBA	FskAlphaBlackDestinationIn32A
#define FskAlphaBlackDestinationOut32ABGR	FskAlphaBlackDestinationOutA32
#define FskAlphaBlackDestinationOut32ARGB	FskAlphaBlackDestinationOutA32
#define FskAlphaBlackDestinationOut32BGRA	FskAlphaBlackDestinationOut32A
#define FskAlphaBlackDestinationOut32RGBA	FskAlphaBlackDestinationOut32A
#define FskAlphaBlackDestinationOver32ABGR	FskAlphaBlackDestinationOverA32
#define FskAlphaBlackDestinationOver32ARGB	FskAlphaBlackDestinationOverA32
#define FskAlphaBlackDestinationOver32BGRA	FskAlphaBlackDestinationOver32A
#define FskAlphaBlackDestinationOver32RGBA	FskAlphaBlackDestinationOver32A
#define FskAlphaBlackSourceAtop32ABGR		FskAlphaBlackSourceAtopA32
#define FskAlphaBlackSourceAtop32ARGB		FskAlphaBlackSourceAtopA32
#define FskAlphaBlackSourceAtop32BGRA		FskAlphaBlackSourceAtop32A
#define FskAlphaBlackSourceAtop32RGBA		FskAlphaBlackSourceAtop32A
#define FskAlphaBlackSourceIn32ABGR			FskAlphaBlackSourceInA32
#define FskAlphaBlackSourceIn32ARGB			FskAlphaBlackSourceInA32
#define FskAlphaBlackSourceIn32BGRA			FskAlphaBlackSourceIn32A
#define FskAlphaBlackSourceIn32RGBA			FskAlphaBlackSourceIn32A
#define FskAlphaBlackSourceOut32ABGR		FskAlphaBlackSourceOutA32
#define FskAlphaBlackSourceOut32ARGB		FskAlphaBlackSourceOutA32
#define FskAlphaBlackSourceOut32BGRA		FskAlphaBlackSourceOut32A
#define FskAlphaBlackSourceOut32RGBA		FskAlphaBlackSourceOut32A
#define FskAlphaBlackSourceOver32ABGR		FskAlphaBlackSourceOverA32
#define FskAlphaBlackSourceOver32ARGB		FskAlphaBlackSourceOverA32
#define FskAlphaBlackSourceOver32BGRA		FskAlphaBlackSourceOver32A
#define FskAlphaBlackSourceOver32RGBA		FskAlphaBlackSourceOver32A
#define FskAlphaBlackXor32ABGR				FskAlphaBlackXorA32
#define FskAlphaBlackXor32ARGB				FskAlphaBlackXorA32
#define FskAlphaBlackXor32BGRA				FskAlphaBlackXor32A
#define FskAlphaBlackXor32RGBA				FskAlphaBlackXor32A

#define FskAlphaScale32RGBA					FskAlphaScale32
#define FskAlphaScale32BGRA					FskAlphaScale32
#define FskAlphaScale32ABGR					FskAlphaScale32
#define FskAlphaScale32ARGB					FskAlphaScale32
#define FskAlphaScale24RGB					FskAlphaScale24
#define FskAlphaScale24BGR					FskAlphaScale24
#define FskAlphaScale8G						FskAlphaMul
#define FskAlphaScale8A						FskAlphaMul

#define FskAXPY32ARGB						FskAXPY8888
#define FskAXPY32ABGR						FskAXPY8888
#define FskAXPY32BGRA						FskAXPY8888
#define FskAXPY32RGBA						FskAXPY8888
#define FskAXPY24RGB						FskAXPY24
#define FskAXPY24BGR						FskAXPY24
#define FskAXPY8G(a, x, y)					(UInt8)(FskAlphaMul(a, x) + y)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKPIXELOPS__ */

