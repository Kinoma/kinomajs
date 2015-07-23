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
	\file	FskRectBlit.c
	\brief	Implementation of pixel transfers between rectangular regions.
*/
#define __FSKBITMAP_PRIV__	/* To get access to the FskBitmap data structure */

#include "FskRectBlit.h"
#include "FskRectBlitPatch.h"
#include "FskPixelOps.h"
#include "FskPlatformImplementation.h"
#include "FskPremultipliedAlpha.h"
#include "FskMemory.h"
#include "FskArch.h"
#include "FskTime.h"
#include <math.h>
#if FSKBITMAP_OPENGL
	#include "FskGLBlit.h"
#endif /* FSKBITMAP_OPENGL */

#if SUPPORT_INSTRUMENTATION
	#define LOG_PARAMETERS		/**< Log the parameters of API calls. */
#endif /* SUPPORT_INSTRUMENTATION */

#if	defined(LOG_PARAMETERS)
	#define BLIT_DEBUG	1
#else /* !LOG_PARAMETERS */
	#define BLIT_DEBUG	0
#endif /* LOG_PARAMETERS */

#if SUPPORT_INSTRUMENTATION
	FskInstrumentedSimpleType(Blit, blit);
	#define LOGD(...)  do { FskInstrumentedTypePrintfDebug  (&gBlitTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGI(...)  do { FskInstrumentedTypePrintfVerbose(&gBlitTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGE(...)  do { FskInstrumentedTypePrintfMinimal(&gBlitTypeInstrumentation, __VA_ARGS__); } while(0)
#endif /* SUPPORT_INSTRUMENTATION */
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */

#if defined(__MWERKS__)
	#define COMPILER_CAN_GENERATE_CONST_PROC_POINTERS	1
#else
	#define COMPILER_CAN_GENERATE_CONST_PROC_POINTERS	0
#endif

#undef TARGET_RT_FPU
#define TARGET_RT_FPU 0

#ifndef __FSKPLATFORM__  /* these will go away sometime soon */
	#error FskPlatform.h not included.
#endif /* __FSKPLATFORM__ */


#include "FskBlitDispatchDef.h"


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***				Constant and token definitions							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#ifndef min
	#define min(a,b)	((a < b) ? a : b)
#endif

#define FIXED_SUBBITS	16							/**< The number of fractional bits in the standard 16.16 fixed point data type. */
#define FIXED_ONE		(1 << FIXED_SUBBITS)		/**< One,      as represented in standard 16.16 fixed point notation. */
#define FIXED_HALF		(1 << (FIXED_SUBBITS-1))	/**< One half, as represented in standard 16.16 fixed point notation. */
#define FWD_SUBBITS		kFskRectBlitBits			/**< The number of fractional bits used in forward differencing. */
#define FWD_ONE			(1 << FWD_SUBBITS)			/**< One,      as used in the forward differencing computations. */
#define FWD_HALF		(1 << (FWD_SUBBITS - 1))	/**< One half, as used in the forward differencing computations. */
#define FWD_FRAC_MASK	(FWD_ONE - 1)				/**< A mask for the fraction part of numbers used in forward differencing. */
#define ASM_SUBBITS		16							/**< The number of fractional bits used in the assembler patch API. */
#define ASM_ONE			(1 << ASM_SUBBITS)			/**< One,      as represented in the assembler patch API. */
#define ASM_HALF		(1 << (ASM_SUBBITS - 1))	/**< One half, as represented in the assembler patch API. */

#define QUIET_BAIL_IF_TRUE(q, err, code)	do { if (  q ) { (err) = (code); goto bail; } } while(0)	/**< goto bail conditionally without raising an alarm. */
#define QUIET_BAIL_IF_FALSE(q, err, code)	do { if (!(q)) { (err) = (code); goto bail; } } while(0)	/**< goto bail conditionally without raising an alarm. */

#if kFskScaleBits < FWD_SUBBITS
	#error kFskScaleBits < FWD_SUBBITS
#endif /* kFskScaleBits < FWD_SUBBITS */
#if FWD_SUBBITS < kFskOffsetBits
	#error FWD_SUBBITS < kFskOffsetBits
#endif /* FWD_SUBBITS < kFskOffsetBits */
#if ASM_SUBBITS > FWD_SUBBITS
	#error ASM_SUBBITS > FWD_SUBBITS
#endif /* ASM_SUBBITS > FWD_SUBBITS */

#if TARGET_OS_IPHONE
	#include "FskPixelOpsTo32Inlines.c"
#endif

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***					Common blit loop macros
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#define BLIT_DECL		UInt32		widthBytes	= params->dstWidth * FskName3(fsk,DstPixelKind,Bytes),	height = params->dstHeight;	\
						SInt32		srb		= params->srcRowBytes,	dBump	= params->dstRowBytes - widthBytes;						\
						FskFixed	x0		= params->srcX0,		y		= params->srcY0;										\
						FskFixed	xd		= params->srcXInc,		yd		= params->srcYInc;										\
						const UInt8	*s0		= (const UInt8*)(params->srcBaseAddr),	*sr;											\
						const FskName3(Fsk,SrcPixelKind,Type)	*s;																	\
						register FskName3(Fsk,DstPixelKind,Type)*d = (FskName3(Fsk,DstPixelKind,Type)*)params->dstBaseAddr;			\
						register FskFixed						x;																	\
						register UInt8							*dEnd												/* BLIT_DECL */
#define BLIT_STARTLOOP	for ( ; height--; y += yd, d = (FskName3(Fsk,DstPixelKind,Type)*)(dBump + (char *)d)) { sr = s0 + (y >> FWD_SUBBITS) * srb;	\
							for (x = x0, dEnd = widthBytes + (UInt8 *)d; (UInt8 *)d != dEnd; x += xd,												\
								d = (FskName3(Fsk,DstPixelKind,Type)*)(((char*)d) + FskName3(fsk,DstPixelKind,Bytes))) {							\
								s = (const FskName3(Fsk,SrcPixelKind,Type)*)(sr + (x >> FWD_SUBBITS) * FskName3(fsk,SrcPixelKind,Bytes));	/* BLIT_STARTLOOP */
#define BLIT_ENDLOOP	}}																							/* BLIT_ENDLOOP */
#define xf				((x >> (FWD_SUBBITS - 4)) & 0xF)
#define yf				((y >> (FWD_SUBBITS - 4)) & 0xF)

#define YUV_BLIT_DECL		UInt32		width	= params->dstWidth,			height		= params->dstHeight,	i;			\
							SInt32		Yrb		= params->srcRowBytes,		Crb			= Yrb >> 1;							\
							SInt32		drb		= params->dstRowBytes,		j;												\
							FskFixed	x0		= params->srcX0,			y			= params->srcY0,		x, xC, yC;	\
							FskFixed	xd		= params->srcXInc,			yd			= params->srcYInc;					\
							FskFixed	xCmax	= ((params->srcWidth  >> 1) - 1) << FWD_SUBBITS;							\
							FskFixed	yCmax	= ((params->srcHeight >> 1) - 1) << FWD_SUBBITS;							\
							const UInt8	*Y0		= (const UInt8*)(params->srcBaseAddr ),		*Yr, *Y;						\
							const UInt8	*U0		= (const UInt8*)(params->srcUBaseAddr),		*Ur, *U;						\
							const UInt8	*V0		= (const UInt8*)(params->srcVBaseAddr),		*Vr, *V;						\
							UInt8		*d0		= (UInt8*)(params->dstBaseAddr);											\
							FskName3(Fsk,DstPixelKind,Type)	*d												/* YUV_BLIT_DECL */
#define YUV_BLIT_STARTLOOP	for ( ; height--; y += yd, d0 += drb) {																		\
								yC = (y - FWD_HALF) >> 1;	if (yC < 0) yC = 0;	else if (yC > yCmax) yC = yCmax;						\
								Yr = Y0 + (y >> FWD_SUBBITS) * Yrb;		Ur = U0 + (j = (yC >> FWD_SUBBITS) * Crb);		Vr = V0 + j;	\
								for (i = width, x = x0, d = (FskName3(Fsk,DstPixelKind,Type)*)d0; i--; x += xd,							\
									d = (FskName3(Fsk,DstPixelKind,Type)*)(((char*)d) + FskName3(fsk,DstPixelKind,Bytes)) ) {			\
									xC = (x - FWD_HALF) >> 1;	if (xC < 0) xC = 0;	else if (xC > xCmax) xC = xCmax;					\
									Y = Yr + (x >> FWD_SUBBITS);	U = Ur + (j = xC >> FWD_SUBBITS);	V = Vr + j;	/* YUV_BLIT_STARTLOOP */
#define YUV_BLIT_ENDLOOP	}}																						/* YUV_BLIT_ENDLOOP */
#define xcf					((xC >> (FWD_SUBBITS - 4)) & 0xF)
#define ycf					((yC >> (FWD_SUBBITS - 4)) & 0xF)


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***				Specialized Bilinear 16 with higher quality
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * Bilinear conversions 16 -> 32
 *	Higher quality than the standard.
 ********************************************************************************/

/* 565 RGB SE -> 32 BGRA  ||  565 BGR SE -> 32 RGBA */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32RGBA,KindFormat)))
static UInt32
FskBilerp16RGB565SE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);			/*  RGB -> ARGB */
	fskConvert32ARGB32BGRA(pix);											/* ARGB -> BGRA */
	return(pix);
}
#endif

/* 565 RGB DE -> 32 BGRA  ||  565 BGR DE -> 32 RGBA */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32RGBA,KindFormat)))
static UInt32
FskBilerp16RGB565DE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);			/*  RGB -> ARGB */
	fskConvert32ARGB32BGRA(pix);											/* ARGB -> BGRA */
	return(pix);
}
#endif

/* 565 BGR SE -> 32 BGRA  ||  565 RGB SE -> 32 RGBA */
#if		(FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32RGBA,KindFormat)))
static UInt32
FskBilerp16BGR565SE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);			/*  BGR -> ABGR */
	pix = (pix << 8) | (pix >> 24);											/* ABGR -> BGRA */
	return(pix);
}
#endif

/* 565 BGR DE -> 32 BGRA  ||  565 RGB DE -> 32 RGBA */
#if		(FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32RGBA,KindFormat)))
static UInt32
FskBilerp16BGR565DE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);			/*  BGR -> ABGR */
	pix = (pix << 8) | (pix >> 24);											/* ABGR -> BGRA */
	return(pix);
}
#endif

/* 565 BGR SE -> 32 ARGB  || 565 RGB SE -> 32 ABGR */
#if		(FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ARGB,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ABGR,KindFormat)))
static UInt32
FskBilerp16BGR565SE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);			/*  BGR -> ABGR */
	pix = (pix & 0xFF00FF00) | ((pix >> 16) & 0xFF) | ((pix & 0xFF) << 16);	/* ABGR -> ARGB */
	return(pix);
}
#endif

/* 565 BGR DE -> 32 ARGB  || 565 RGB DE -> 32 ABGR */
#if		(FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ARGB,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ABGR,KindFormat)))
static UInt32
FskBilerp16BGR565DE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);			/*  BGR -> ABGR */
	pix = (pix & 0xFF00FF00) | ((pix >> 16) & 0xFF) | ((pix & 0xFF) << 16);	/* ABGR -> ARGB */
	return(pix);
}
#endif

/* 16 RGBA 4444 SE -> 32 ARGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ARGB,KindFormat)))
static UInt32
FskBilerp16RGBA4444SE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	fskConvert32RGBA32ARGB(pix);
	return(pix);
}
#endif

/* 16 RGBA 4444 SE -> 32 BGRA */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))
static UInt32
FskBilerp16RGBA4444SE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	fskConvert32RGBA32BGRA(pix);
	return(pix);
}
#endif

/* 16 RGBA 4444 SE -> 32 ABGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ABGR,KindFormat)))
static UInt32
FskBilerp16RGBA4444SE32ABGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	fskConvert32RGBA32ABGR(pix);
	return(pix);
}
#endif

/* 16 RGBA 4444 DE -> 32 RGBA */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32RGBA,KindFormat)))
static UInt32
FskBilerp16RGBA4444DE32RGBA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	pix =	FskMoveField(pix, 8, 24, fsk32RGBABluePosition  )
		|	FskMoveField(pix, 8, 16, fsk32RGBAAlphaPosition )
		|	FskMoveField(pix, 8,  8, fsk32RGBARedPosition   )
		|	FskMoveField(pix, 8,  0, fsk32RGBAGreenPosition );
	return(pix);
}
#endif

/* 16 RGBA 4444 DE -> 32 ARGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ARGB,KindFormat)))
static UInt32
FskBilerp16RGBA4444DE32ARGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	pix =	FskMoveField(pix, 8, 24, fsk32ARGBBluePosition  )
		|	FskMoveField(pix, 8, 16, fsk32ARGBAlphaPosition )
		|	FskMoveField(pix, 8,  8, fsk32ARGBRedPosition   )
		|	FskMoveField(pix, 8,  0, fsk32ARGBGreenPosition );
	return(pix);
}
#endif

/* 16 RGBA 4444 DE -> 32 BGRA */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32BGRA,KindFormat)))
static UInt32
FskBilerp16RGBA4444DE32BGRA(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	pix =	FskMoveField(pix, 8, 24, fsk32BGRABluePosition  )
		|	FskMoveField(pix, 8, 16, fsk32BGRAAlphaPosition )
		|	FskMoveField(pix, 8,  8, fsk32BGRARedPosition   )
		|	FskMoveField(pix, 8,  0, fsk32BGRAGreenPosition );
	return(pix);
}
#endif

/* 16 RGBA 4444 DE -> 32 ABGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,32ABGR,KindFormat)))
static UInt32
FskBilerp16RGBA4444DE32ABGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	pix =	FskMoveField(pix, 8, 24, fsk32ABGRBluePosition  )
		|	FskMoveField(pix, 8, 16, fsk32ABGRAlphaPosition )
		|	FskMoveField(pix, 8,  8, fsk32ABGRRedPosition   )
		|	FskMoveField(pix, 8,  0, fsk32ABGRGreenPosition );
	return(pix);
}
#endif


#define FskBilerp16BGR565SE32RGBA	FskBilerp16RGB565SE32BGRA
#define FskBilerp16BGR565DE32RGBA	FskBilerp16RGB565DE32BGRA
#define FskBilerp16RGB565SE32RGBA	FskBilerp16BGR565SE32BGRA
#define FskBilerp16RGB565DE32RGBA	FskBilerp16BGR565DE32BGRA
#define FskBilerp16RGB565SE32ABGR	FskBilerp16BGR565SE32ARGB
#define FskBilerp16RGB565DE32ABGR	FskBilerp16BGR565DE32ARGB


/********************************************************************************
 * Bilinear conversions 16 -> 24
 *		higher quality than the standard
 ********************************************************************************/

/* 565 RGB SE -> 24 RGB */
/* 565 BGR SE -> 24 BGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,24RGB,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))
static void
FskBilerp16RGB565SE24RGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);
	((UInt8*)d)[fsk24RGBRedPosition  ] = (UInt8)(pix >> fsk32ARGBRedPosition);
	((UInt8*)d)[fsk24RGBGreenPosition] = (UInt8)(pix >> fsk32ARGBGreenPosition);
	((UInt8*)d)[fsk24RGBBluePosition ] = (UInt8)(pix >> fsk32ARGBBluePosition);
}
#endif

/* 565 RGB DE -> 24 RGB */
/* 565 BGR DE -> 24 BGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24RGB,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))
static void
FskBilerp16RGB565DE24RGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);
	((UInt8*)d)[fsk24RGBRedPosition  ] = (UInt8)(pix >> fsk32ARGBRedPosition);
	((UInt8*)d)[fsk24RGBGreenPosition] = (UInt8)(pix >> fsk32ARGBGreenPosition);
	((UInt8*)d)[fsk24RGBBluePosition ] = (UInt8)(pix >> fsk32ARGBBluePosition);
}
#endif

/* 565 RGB SE -> 24 BGR */
/* 565 BGR SE -> 24 RGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,24RGB,KindFormat)))
static void
FskBilerp16RGB565SE24BGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);
	((UInt8*)d)[fsk24BGRRedPosition  ] = (UInt8)(pix >> fsk32ARGBRedPosition);
	((UInt8*)d)[fsk24BGRGreenPosition] = (UInt8)(pix >> fsk32ARGBGreenPosition);
	((UInt8*)d)[fsk24BGRBluePosition ] = (UInt8)(pix >> fsk32ARGBBluePosition);
}
#endif

/* 565 RGB DE -> 24 BGR */
/* 565 BGR DE -> 24 RGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))	\
	||	(FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24RGB,KindFormat)))
static void
FskBilerp16RGB565DE24BGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);
	((UInt8*)d)[fsk24BGRRedPosition  ] = (UInt8)(pix >> fsk32ARGBRedPosition);
	((UInt8*)d)[fsk24BGRGreenPosition] = (UInt8)(pix >> fsk32ARGBGreenPosition);
	((UInt8*)d)[fsk24BGRBluePosition ] = (UInt8)(pix >> fsk32ARGBBluePosition);
}
#endif

/* 16 RGBA 4444 SE -> 24 BGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))
static void
FskBilerp16RGBA4444SE24BGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32			pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	((UInt8*)d)[fsk24BGRRedPosition  ] = (UInt8)(pix >> fsk32RGBARedPosition);
	((UInt8*)d)[fsk24BGRGreenPosition] = (UInt8)(pix >> fsk32RGBAGreenPosition);
	((UInt8*)d)[fsk24BGRBluePosition ] = (UInt8)(pix >> fsk32RGBABluePosition);
}
#endif

/* 16 RGBA 4444 SE -> 24 RGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk, 24RGB,KindFormat)))
static void
FskBilerp16RGBA4444SE24RGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32			pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	((UInt8*)d)[fsk24RGBRedPosition  ] = (UInt8)(pix >> fsk32RGBARedPosition);
	((UInt8*)d)[fsk24RGBGreenPosition] = (UInt8)(pix >> fsk32RGBAGreenPosition);
	((UInt8*)d)[fsk24RGBBluePosition ] = (UInt8)(pix >> fsk32RGBABluePosition);
}
#endif

/* 16 RGBA 4444 DE -> 24 BGR */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24BGR,KindFormat)))
static void
FskBilerp16RGBA4444DE24BGR(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32			pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	((UInt8*)d)[fsk24BGRRedPosition  ] = (UInt8)(pix >>  8);	/* R */
	((UInt8*)d)[fsk24BGRGreenPosition] = (UInt8)(pix >>  0);	/* G */
	((UInt8*)d)[fsk24BGRBluePosition ] = (UInt8)(pix >>  24);	/* B */
}
#endif

/* 16 RGBA 4444 DE -> 24 RGB */
#if		(FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,24RGB,KindFormat)))
static void
FskBilerp16RGBA4444DE24RGB(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb, Fsk24BitType *d)
{
	UInt32			pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);
	((UInt8*)d)[fsk24RGBRedPosition  ] = (UInt8)(pix >>  8);	/* R */
	((UInt8*)d)[fsk24RGBGreenPosition] = (UInt8)(pix >>  0);	/* G */
	((UInt8*)d)[fsk24RGBBluePosition ] = (UInt8)(pix >>  24);	/* B */
}
#endif

#define FskBilerp16BGR565SE24BGR FskBilerp16RGB565SE24RGB
#define FskBilerp16BGR565DE24BGR FskBilerp16RGB565DE24RGB
#define FskBilerp16BGR565SE24RGB FskBilerp16RGB565SE24BGR
#define FskBilerp16BGR565DE24RGB FskBilerp16RGB565DE24BGR


/********************************************************************************
 * Bilinear conversions 16 -> 8
 *	Higher quality than the standard.
 ********************************************************************************/

/* 565 RGB SE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16RGB565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16RGB565SE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);			/*  16RGB -> 32ARGB */
	fskConvert32ARGB8G(pix);												/* 32ARGB -> 8G     */
	return (UInt8)pix;
}
#endif

/* 565 RGB DE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16RGB565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16RGB565DE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);			/*  16RGB -> 32ARGB */
	fskConvert32ARGB8G(pix);												/* 32ARGB -> 8G     */
	return (UInt8)pix;
}
#endif

/* 565 BGR SE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16BGR565SE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16BGR565SE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565SE32ARGB(di, dj, s, rb);			/* 16BGR  -> 32ABGR */
	fskConvert32ARGB8G(pix);										/* 32ARGB -> 8G     */
	return (UInt8)pix;
}
#endif

/* 565 BGR DE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16BGR565DE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16BGR565DE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGB565DE32ARGB(di, dj, s, rb);			/* 16BGR  -> 32ABGR */
	fskConvert32ARGB8G(pix);										/* 32ARGB -> 8G     */
	return (UInt8)pix;
}
#endif

/* 4444 RGBA SE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16RGBA4444SE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16RGBA4444SE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);		/* 16RGBA4444SE -> 32RGBA */
	fskConvert32RGBA8G(pix);										/*       32ARGB -> 8G     */
	return (UInt8)pix;
}
#endif

/* 4444 RGBA DE -> 8 G */
#if FskName2(SRC_,FskName3(fsk,16RGBA4444DE,KindFormat)) && FskName2(DST_,FskName3(fsk,8G,KindFormat))
static UInt8
FskBilerp16RGBA4444DE8G(UInt32 di, UInt32 dj, const UInt16 *s, SInt32 rb)
{
	UInt32 pix = FskBilerp16RGBA4444SE32RGBA(di, dj, s, rb);		/* 16RGBA4444DE -> 32BARG  */
	pix = (pix << 16) | (pix >> 16);								/*       32BARG -> 32RGBA  */
	fskConvert32RGBA8G(pix);										/*       32ARGB -> 8G      */
	return (UInt8)pix;
}
#endif




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***		Optimized 1:1 rectBlits
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskUnityCopyHomogeneousRect
 ********************************************************************************/

static void
FskUnityCopyHomogeneousRect(const FskRectBlitParams *params, UInt32 pixelBytes)
{
	UInt32			width	= params->dstWidth * pixelBytes;
	UInt32			height	= params->dstHeight;
	SInt32			srb		= params->srcRowBytes;
	SInt32			drb		= params->dstRowBytes;
	const UInt8		*s0		= (const UInt8*)(params->srcBaseAddr)
							+ (params->srcY0 >> FWD_SUBBITS) * srb
							+ (params->srcX0 >> FWD_SUBBITS) * pixelBytes;
	UInt8			*d0		= (UInt8*)(params->dstBaseAddr);
	SInt32			w;
	register SInt32	i;
	const UInt8		*sPix;
	UInt8			*dPix;

#if (!(TARGET_CPU_ARM && !TARGET_OS_IPHONE))
#else
	extern void doCopyPlane(const UInt8 *s, SInt32 sBump, UInt8 *d, SInt32 dBump, UInt32 dh, UInt32 dw);

	if ((width >= 16) && height) {
		doCopyPlane(s0, srb - width, d0, drb - width, height, width);
		return;
	}
#endif

	for ( ; height--; s0 += srb, d0 += drb) {
		w    = width;
		sPix = s0;
		dPix = d0;

		/* Source and destination are appropriately aligned */
		if ((i = ((int)sPix & 3)) == ((int)dPix & 3)) {			/* Src & Dst are at the same phase relative to a quadByte */

			/* Leading stragglers */
			if (i != 0) {
				if ((w -= (i = 4 - i)) < 0) {					/* Width ends before the end of the quadByte */
					i += w;										/* Make the initial straggle subspan accordingly shorter */
					w = 0;										/* Zero w to indicate that we're done */
				}
				while (i--)
					*dPix++ = *sPix++;							/* Copy initial straggler singletons */
			}

			/* Middle swath */
			{	register const UInt32	*s32	= (const UInt32*)sPix;
				register UInt32			*d32	= (      UInt32*)dPix;

				i = (UInt32)w / sizeof(UInt32);					/* This should shift rather than divide */
				w &= 3;											/* Compute the number of trailing stragglers for later */
				while (i--)
					*d32++ = *s32++;							/* Copy middle swath 4 bytes at a time */
				sPix = (const UInt8*)s32;						/* Update the byte pointers for the src... */
				dPix =       (UInt8*)d32;						/* ... and the dst */
			}
		}

		else if ((i = ((int)sPix & 1)) == ((int)dPix & 1)) {	/* Src & Dst are at the same phase relative to a duoByte */

			/* Leading stragglers */
			if (i != 0) {
				if ((w -= (i = 2 - i)) < 0) {					/* Width ends before the end of the duoByte */
					i += w;										/* Make the initial straggle subspan accordingly shorter */
					w = 0;										/* Zero w to indicate that we're done */
				}
				while (i--)
					*dPix++ = *sPix++;							/* Copy initial straggler singletons */
			}

			/* Middle swath */
			{	register const UInt16	*s16	= (const UInt16*)sPix;
				register UInt16			*d16	= (      UInt16*)dPix;

				i = (UInt32)w / sizeof(UInt16);					/* This should shift rather than divide */
				w &= 1;											/* Compute the number of trailing stragglers for later */
				while (i--)
					*d16++ = *s16++;							/* Copy middle swath 2 bytes at a time */
				sPix = (const UInt8*)s16;						/* Update the byte pointers for the src... */
				dPix =       (UInt8*)d16;						/* ... and the dst */
			}
		}

		/* Copy trailing stragglers and scanlines not otherwise aligned */
		for (i = w; i--; )
			*dPix++ = *sPix++;

	}
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***					Rect Blit Proc Definitions
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


#define GENERAL_BLIT_PROTO_FILE	"FskRectBlitTo32.c"
#include "FskBlitSrcDst.h"

#define GENERAL_BLIT_PROTO_FILE	"FskRectBlitTo24.c"
#include "FskBlitSrcDst.h"

#define GENERAL_BLIT_PROTO_FILE	"FskRectBlitTo16.c"
#include "FskBlitSrcDst.h"

#define GENERAL_BLIT_PROTO_FILE	"FskRectBlitTo8.c"
#include "FskBlitSrcDst.h"

#define UNITY_BLIT_PROTO_FILE	"FskRectBlitUnity.c"
#include "FskBlitSrcDst.h"

#if SRC_YUV420
	#define SRC_KIND YUV420
	#define BLIT_PROTO_FILE		"FskRectBlitFromYUV.c"
	#include "FskBlitDst.h"
	#undef BLIT_PROTO_FILE
#endif /* SRC_YUV420 */

#if DST_YUV420
	#define DST_KIND YUV420
	#define BLIT_PROTO_FILE		"FskRectBlitToYUV.c"
	#include "FskBlitSrc.h"
	#undef BLIT_PROTO_FILE
#endif /* DST_YUV420 */

#if SRC_YUV420 && DST_YUV420
	#include "FskRectBlitYUV.c"
#endif /* SRC_YUV420 && DST_YUV420 */


#if !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS
/********************************************************************************
 ********************************************************************************
 **	NULLify procs
 **
 ** It would be nice to generate these all automatically in the above files.
 ** The following DOES work in MetroWerks and Visual Studio 8:
 **		static const FskRectTransferProc FskName3(FskCopy,SrcPixelKind,YUV420)	= NULL;
 **		const FskRectTransferProc FskName3(FskCopy,SrcPixelKind,YUV420)			= NULL;
 ** whereas XCode and Visual Studio7 complain that the resultant variable is not constant,
 ** in spite of the fact that the keyword "const" is used explicitly.
 ** These are invoked in the files "FskRectBlitUnity.c" and "FskRectBlitToYUV.c" for these compilers.
 **
 ** Every compiler complains about these two:
 **		static void FskName3(FskCopy,SrcPixelKind,YUV420)(const FskRectBlitParams *params)	= NULL;
 **		#define FskCopy##SrcPixelKind##YUV420									NULL
 ** Until all compilers can generate thee code automatically, we have the following:
 ********************************************************************************
 ********************************************************************************/

	#include "FskRectBlitNull.h"

#endif /* !COMPILER_CAN_GENERATE_CONST_PROC_POINTERS */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***		Dispatcher
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * These make the dispatch table manageable
 ********************************************************************************/

#define NUM_OPS 					5
#define OP_0						Copy
#define OP_1						Blend
#define OP_2						Alpha
#define OP_3						AlphaBlend
#define OP_4						TintCopy
#define OP_INDEX_COPY				0
#define OP_INDEX_BLEND				1
#define OP_INDEX_ALPHA				2
#define OP_INDEX_ALPHABLEND			3
#define OP_INDEX_TINTCOPY			4

#define NUM_QUALS					2
#define QUAL_0						Fsk
#define QUAL_1						FskBilinear


/********************************************************************************
 * PixelFormatToProcTableSrcIndex
 ********************************************************************************/

static SInt32
PixelFormatToProcTableSrcIndex(UInt32 pixelFormat)
{
	return (pixelFormat < sizeof(srcPixelFormatToPixelKindIndex) / sizeof(srcPixelFormatToPixelKindIndex[0])) ?
			srcPixelFormatToPixelKindIndex[pixelFormat] :
			-1;
}


/********************************************************************************
 * PixelFormatToProcTableDstIndex
 ********************************************************************************/

static SInt32
PixelFormatToProcTableDstIndex(UInt32 pixelFormat)
{
	if (DISPATCH_FORMATS_OUT_OF_SYNC) {
		LOGE("DISPATCH_FORMATS_ARE_OUT_OF_SYNC\n");
		return -1;
	}
	return (pixelFormat < sizeof(dstPixelFormatToPixelKindIndex) / sizeof(dstPixelFormatToPixelKindIndex[0])) ?
			dstPixelFormatToPixelKindIndex[pixelFormat] :
			-1;
}


/********************************************************************************
 * ModeToProcTableOpIndex
 ********************************************************************************/

static SInt32
ModeToProcTableOpIndex(UInt32 mode, UInt8 blendLevel)
{
	SInt32 index = -1;

	switch (mode) {
		case kFskGraphicsModeCopy:			index = (blendLevel == 255) ?
													OP_INDEX_COPY :
													OP_INDEX_BLEND;			break;
		case kFskGraphicsModeAlpha:			index = (blendLevel == 255) ?
													OP_INDEX_ALPHA :
													OP_INDEX_ALPHABLEND;	break;
		case kFskGraphicsModeColorize:		index = OP_INDEX_TINTCOPY;		break;
	}

	return(index);
}



#ifdef RECT_BLIT_OVERRIDE
	#include RECT_BLIT_OVERRIDE
#endif /* RECT_BLIT_OVERRIDE */


/********************************************************************************
 * Dispatch table: dst, src, op, qual
 ********************************************************************************/

#define P_DSOQ(d,s,o,q) 	FskName4(q,o,s,d)
#define P_DSO(d,s,o) 		{ FskName4(QUAL_0,o,s,d),FskName4(QUAL_1,o,s,d) }
#define P_DS(d,s)			{ P_DSO(d,s,OP_0),P_DSO(d,s,OP_1),P_DSO(d,s,OP_2),P_DSO(d,s,OP_3),P_DSO(d,s,OP_4) }
#define PC_DS(d,s)			FskName3( FskUnityCopy,s,d)
#define PA_DS(d,s)			FskName3(FskUnityAlpha,s,d)

#if   NUM_SRC_FORMATS == 1
	#define P_D(d)		{ P_DS(d,SRC_KIND_0) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0) }
#elif NUM_SRC_FORMATS == 2
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1) }
#elif NUM_SRC_FORMATS == 3
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2) }
#elif NUM_SRC_FORMATS == 4
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3) }
#elif NUM_SRC_FORMATS == 5
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4) }
#elif NUM_SRC_FORMATS == 6
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5) }
#elif NUM_SRC_FORMATS == 7
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6) }
#elif NUM_SRC_FORMATS == 8
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7) }
#elif NUM_SRC_FORMATS == 9
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8) }
#elif NUM_SRC_FORMATS == 10
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9) }
#elif NUM_SRC_FORMATS == 11
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10) }
#elif NUM_SRC_FORMATS == 12
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11) }
#elif NUM_SRC_FORMATS == 13
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12) }
#elif NUM_SRC_FORMATS == 14
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13) }
#elif NUM_SRC_FORMATS == 15
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14) }
#elif NUM_SRC_FORMATS == 16
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14),PC_DS(d,SRC_KIND_15) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14),PA_DS(d,SRC_KIND_15) }
#elif NUM_SRC_FORMATS == 17
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14),PC_DS(d,SRC_KIND_15),PC_DS(d,SRC_KIND_16) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14),PA_DS(d,SRC_KIND_15),PA_DS(d,SRC_KIND_16) }
#elif NUM_SRC_FORMATS == 18
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14),PC_DS(d,SRC_KIND_15),PC_DS(d,SRC_KIND_16),PC_DS(d,SRC_KIND_17) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14),PA_DS(d,SRC_KIND_15),PA_DS(d,SRC_KIND_16),PA_DS(d,SRC_KIND_17) }
#elif NUM_SRC_FORMATS == 19
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17),P_DS(d,SRC_KIND_18) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14),PC_DS(d,SRC_KIND_15),PC_DS(d,SRC_KIND_16),PC_DS(d,SRC_KIND_17),PC_DS(d,SRC_KIND_18) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14),PA_DS(d,SRC_KIND_15),PA_DS(d,SRC_KIND_16),PA_DS(d,SRC_KIND_17),PA_DS(d,SRC_KIND_18) }
#elif NUM_SRC_FORMATS == 20
	#define P_D(d)		{ P_DS(d,SRC_KIND_0),P_DS(d,SRC_KIND_1),P_DS(d,SRC_KIND_2),P_DS(d,SRC_KIND_3),P_DS(d,SRC_KIND_4),P_DS(d,SRC_KIND_5),P_DS(d,SRC_KIND_6),P_DS(d,SRC_KIND_7),P_DS(d,SRC_KIND_8),P_DS(d,SRC_KIND_9),P_DS(d,SRC_KIND_10),P_DS(d,SRC_KIND_11),P_DS(d,SRC_KIND_12),P_DS(d,SRC_KIND_13),P_DS(d,SRC_KIND_14),P_DS(d,SRC_KIND_15),P_DS(d,SRC_KIND_16),P_DS(d,SRC_KIND_17),P_DS(d,SRC_KIND_18),P_DS(d,SRC_KIND_19) }
	#define PC_D(d)		{ PC_DS(d,SRC_KIND_0),PC_DS(d,SRC_KIND_1),PC_DS(d,SRC_KIND_2),PC_DS(d,SRC_KIND_3),PC_DS(d,SRC_KIND_4),PC_DS(d,SRC_KIND_5),PC_DS(d,SRC_KIND_6),PC_DS(d,SRC_KIND_7),PC_DS(d,SRC_KIND_8),PC_DS(d,SRC_KIND_9),PC_DS(d,SRC_KIND_10),PC_DS(d,SRC_KIND_11),PC_DS(d,SRC_KIND_12),PC_DS(d,SRC_KIND_13),PC_DS(d,SRC_KIND_14),PC_DS(d,SRC_KIND_15),PC_DS(d,SRC_KIND_16),PC_DS(d,SRC_KIND_17),PC_DS(d,SRC_KIND_18),PC_DS(d,SRC_KIND_19) }
	#define PA_D(d)		{ PA_DS(d,SRC_KIND_0),PA_DS(d,SRC_KIND_1),PA_DS(d,SRC_KIND_2),PA_DS(d,SRC_KIND_3),PA_DS(d,SRC_KIND_4),PA_DS(d,SRC_KIND_5),PA_DS(d,SRC_KIND_6),PA_DS(d,SRC_KIND_7),PA_DS(d,SRC_KIND_8),PA_DS(d,SRC_KIND_9),PA_DS(d,SRC_KIND_10),PA_DS(d,SRC_KIND_11),PA_DS(d,SRC_KIND_12),PA_DS(d,SRC_KIND_13),PA_DS(d,SRC_KIND_14),PA_DS(d,SRC_KIND_15),PA_DS(d,SRC_KIND_16),PA_DS(d,SRC_KIND_17),PA_DS(d,SRC_KIND_18),PA_DS(d,SRC_KIND_19) }
#else
	#error Unexpected large number of source pixel formats
#endif /* NUM_SRC_FORMATS */

static FskRectTransferProc	procDispatch[NUM_DST_FORMATS][NUM_SRC_FORMATS][NUM_OPS][NUM_QUALS] = {
		P_D(DST_KIND_0)
	#if NUM_DST_FORMATS > 1
		,P_D(DST_KIND_1)
	#endif
	#if NUM_DST_FORMATS > 2
		,P_D(DST_KIND_2)
	#endif
	#if NUM_DST_FORMATS > 3
		,P_D(DST_KIND_3)
	#endif
	#if NUM_DST_FORMATS > 4
		,P_D(DST_KIND_4)
	#endif
	#if NUM_DST_FORMATS > 5
		,P_D(DST_KIND_5)
	#endif
	#if NUM_DST_FORMATS > 6
		,P_D(DST_KIND_6)
	#endif
	#if NUM_DST_FORMATS > 7
		,P_D(DST_KIND_7)
	#endif
	#if NUM_DST_FORMATS > 8
		,P_D(DST_KIND_8)
	#endif
	#if NUM_DST_FORMATS > 9
		,P_D(DST_KIND_9)
	#endif
	#if NUM_DST_FORMATS > 10
		,P_D(DST_KIND_10)
	#endif
	#if NUM_DST_FORMATS > 11
		,P_D(DST_KIND_11)
	#endif
	#if NUM_DST_FORMATS > 12
		,P_D(DST_KIND_12)
	#endif
	#if NUM_DST_FORMATS > 13
		,P_D(DST_KIND_13)
	#endif
	#if NUM_DST_FORMATS > 14
		,P_D(DST_KIND_14)
	#endif
	#if NUM_DST_FORMATS > 15
		,P_D(DST_KIND_15)
	#endif
	#if NUM_DST_FORMATS > 16
		,P_D(DST_KIND_16)
	#endif
	#if NUM_DST_FORMATS > 17
		#error Unexpected large number of destination pixel formats
	#endif
 };

static FskRectTransferProc	unityCopyProc [NUM_DST_FORMATS][NUM_SRC_FORMATS] = {
		PC_D(DST_UNITY_KIND_0)
	#if NUM_DST_UNITY_FORMATS > 1
		,PC_D(DST_UNITY_KIND_1)
	#endif
	#if NUM_DST_UNITY_FORMATS > 2
		,PC_D(DST_UNITY_KIND_2)
	#endif
	#if NUM_DST_UNITY_FORMATS > 3
		,PC_D(DST_UNITY_KIND_3)
	#endif
	#if NUM_DST_UNITY_FORMATS > 4
		,PC_D(DST_UNITY_KIND_4)
	#endif
	#if NUM_DST_UNITY_FORMATS > 5
		,PC_D(DST_UNITY_KIND_5)
	#endif
	#if NUM_DST_UNITY_FORMATS > 6
		#error Unexpected large number of unityCopyProc destination  pixel formats
	#endif
};
static FskRectTransferProc	unityAlphaProc[NUM_DST_FORMATS][NUM_SRC_FORMATS] = {
		PA_D(DST_UNITY_KIND_0)
	#if NUM_DST_UNITY_FORMATS > 1
		,PA_D(DST_UNITY_KIND_1)
	#endif
	#if NUM_DST_UNITY_FORMATS > 2
		,PA_D(DST_UNITY_KIND_2)
	#endif
	#if NUM_DST_UNITY_FORMATS > 3
		,PA_D(DST_UNITY_KIND_3)
	#endif
	#if NUM_DST_UNITY_FORMATS > 4
		,PA_D(DST_UNITY_KIND_4)
	#endif
	#if NUM_DST_UNITY_FORMATS > 5
		,PA_D(DST_UNITY_KIND_5)
	#endif
	#if NUM_DST_UNITY_FORMATS > 6
		#error Unexpected large number of unityAlphaProc destination pixel formats
	#endif
};



#if TARGET_RT_FPU
/********************************************************************************
 * RoundDoubleToSInt32
 ********************************************************************************/

static SInt32
RoundDoubleToSInt32(double x)
{
	x += (x < 0.) ? -.5 : +.5;
	#if SUPPORT_INSTRUMENTATION
		if (x < -2147483648. || x > +2147483647)
			LOGE("RoundDoubleToSInt32(%.17g) overflow saturating to %d", x - ((x < 0.) ? -.5 : +.5), (int)x);
	#endif /* SUPPORT_INSTRUMENTATION */
	return (SInt32)x;
}

/********************************************************************************
 * RoundDoubleToFWD
 ********************************************************************************/

static FskFixed
RoundDoubleToFWD(double x)
{
	x = x * FWD_ONE + ((x < 0.) ? -.5 : +.5);
	#if SUPPORT_INSTRUMENTATION
		if (x < -2147483648. || x > +2147483647)
			LOGE("RoundDoubleToFWD(%.17g) overflow saturating to %d", (x - ((x < 0.) ? -.5 : +.5)) / FWD_ONE, (int)x);
	#endif /* SUPPORT_INSTRUMENTATION */
	return (FskFixed)x;
}

/********************************************************************************
 * lceil
 ********************************************************************************/

static SInt32
lceil(double x)
{
	x += (x < 0.) ? -.005 : +.995;
	#if SUPPORT_INSTRUMENTATION
		if (x < -2147483648. || x > +2147483647)
			LOGE("lceil(%.17g) overflow saturating to %d", x - ((x < 0.) ? -.005 : +.995), (int)x);
	#endif /* SUPPORT_INSTRUMENTATION */
	return (SInt32)x;
}


/********************************************************************************
 * lfloor
 ********************************************************************************/

static SInt32
lfloor(double x)
{
	x += (x < 0) ? -0.995 : +0.005;
	#if SUPPORT_INSTRUMENTATION
		if (x < -2147483648. || x > +2147483647)
			LOGE("lfloor(%.17g) overflow saturating to %d", x - ((x < 0) ? -0.995 : +0.005), (int)x);
	#endif /* SUPPORT_INSTRUMENTATION */
	return (SInt32)x;
}

#endif /* TARGET_RT_FPU */

/********************************************************************************
 * GetUnityProc
 ********************************************************************************/

static FskRectTransferProc
GetUnityProc(SInt32 srcIndex, SInt32 dstIndex, SInt32 opIndex)
{
	FskRectTransferProc blitProc	= NULL;

	#if NUM_DST_UNITY_FORMATS > 0
		if ((dstIndex = dstPixelKindUnityIndex[dstIndex]) >= 0) {
			if      (opIndex == OP_INDEX_COPY)			blitProc = unityCopyProc [dstIndex][srcIndex];
			else if (opIndex == OP_INDEX_ALPHA)			blitProc = unityAlphaProc[dstIndex][srcIndex];
		}
	#endif /* NUM_DST_UNITY_FORMATS */

	return(blitProc);
}

FskRectTransferProc GetUnityProcByPixFormat(UInt32 srcPixFormat, UInt32 dstPixFormat, UInt32 mode, UInt32 alpha);	/* secret export */
FskRectTransferProc
GetUnityProcByPixFormat(UInt32 srcPixFormat, UInt32 dstPixFormat, UInt32 mode, UInt32 alpha)
{
	FskRectTransferProc blitProc	= NULL;
	SInt32 srcIndex	= PixelFormatToProcTableSrcIndex(srcPixFormat);
	SInt32 dstIndex	= PixelFormatToProcTableDstIndex(dstPixFormat);
	SInt32 opIndex	= ModeToProcTableOpIndex(mode & kFskGraphicsModeMask, (UInt8)alpha);
	SInt32 quality  = (mode & kFskGraphicsModeBilinear) ? 1 : 0;

	if (srcIndex < 0 || dstIndex < 0 || opIndex < 0)
		return NULL;

	#if NUM_DST_UNITY_FORMATS > 0
		if (dstPixelKindUnityIndex[dstIndex] >= 0) {
			dstIndex = dstPixelKindUnityIndex[dstIndex];
			if      (opIndex == OP_INDEX_COPY)			blitProc = unityCopyProc [dstIndex][srcIndex];
			else if (opIndex == OP_INDEX_ALPHA)			blitProc = unityAlphaProc[dstIndex][srcIndex];
		}
	#endif /* NUM_DST_UNITY_FORMATS */

	/* Get blit proc and invoke it, looking for an optimized unity proc first */
	if( blitProc == NULL)				/* No unity proc exists */
		blitProc = procDispatch[dstIndex][srcIndex][opIndex][quality];

	return(blitProc);
}


/********************************************************************************
 * SwapProc
 ********************************************************************************/

static FskRectTransferProc
SwapProc(FskRectTransferProc *procSlot, FskRectTransferProc blitProc)
{
	FskRectTransferProc oldProc;

	if (procSlot != NULL) {
		oldProc   = *procSlot;
		*procSlot = blitProc;

	}
	else {
		oldProc = (FskRectTransferProc)kFskErrInvalidParameter;
	}

	return oldProc;
}


/********************************************************************************
 * EnforceYUVRectConstraints
 * Coerce rect to have even pixel alignment.
 * YUV 4:2:0 is guaranteed to have an even number of pixels by definition.
 ********************************************************************************/

static void
EnforceYUVRectConstraints(register FskRectangle r)
{
	register int i;

	i = r->x      & 1;		r->width  += i;		r->x -= i;
	i = r->width  & 1;		r->width  += i;
	i = r->y      & 1;		r->height += i;		r->y -= i;
	i = r->height & 1;		r->height += i;
}


/* For initialization */
static char patchInstalled = 0;

/* Forward declaration */
static void InstallPatches(void);


/****************************************************************************//**
 * SetParamOpColor will set the Rect Blit parameter color {p->red, p->green, p->blue, p->alpha}.
 *	\param[in,out]	p			pointer to the Rect Blit parameter block.
 *								p->isPremul = srcBM->hasAlpha && srcBM->alphaIsPremultiplied; should already be set.
 *	\param[in]		opColor		the opColor; NULL implies opaque white.
 *	\param[in]		modeParams	the mode parameters, notably specifying blendLevel. NULL implies blendLevel = 255.
 *								If blendLevel is not in [0, 255], 255 is used instead.
 ********************************************************************************/

static void SetParamOpColor(FskRectBlitParams *p, FskConstColorRGBA opColor, FskConstGraphicsModeParameters	modeParams)
{
	p->alpha = (UInt8)((modeParams && ((UInt32)modeParams->blendLevel < 255U)) ? modeParams->blendLevel : 255U);
	if (opColor) {																/* If given an opColor, ... */
		p->alpha = FskAlphaMul(p->alpha, opColor->a);							/* Modulate the blend level by the opcolor alpha */
		if (!p->isPremul) {														/* ... and the source is straight alpha, ... */
			p->red   = opColor->r;												/* ... use the opColor directly; */
			p->green = opColor->g;
			p->blue  = opColor->b;
		}
		else {																	/* otherwise, if the source is premultiplied, ... */
			p->red   = FskAlphaMul(p->alpha, opColor->r);						/* ... convert the opColor to premultiplied. */
			p->green = FskAlphaMul(p->alpha, opColor->g);
			p->blue  = FskAlphaMul(p->alpha, opColor->b);
		}
	}
	else {																		/* If no op color given, ... */
		if (!p->isPremul) {														/* ... and the source is straight alpha, ... */
			p->red   = 255;														/* ... use white */
			p->green = 255;
			p->blue  = 255;
		}
		else {																	/* otherwise, if the source is premultiplied, ... */
			p->red   = p->alpha;												/* ... use premultiplied white */
			p->green = p->alpha;
			p->blue  = p->alpha;
		}
	}
}


#ifdef LOG_PARAMETERS

typedef struct LookupEntry { int code;	const char *str; } LookupEntry;
static const char* StringFromCode(int code, const LookupEntry *tab) {
	const LookupEntry *p;
	for (p = tab; p->code != 0; ++p)
		if (code == p->code)
			break;
	return p->str;
}
static const char* ModeString(UInt32 mode) {
	static const LookupEntry modeTab[] = {
		{	kFskGraphicsModeBilinear|kFskGraphicsModeCopy,		"BilinearCopy"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeAlpha,		"BilinearAlpha"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeColorize,	"BilinearColorize"	},
		{	kFskGraphicsModeCopy,								"Copy"				},
		{	kFskGraphicsModeAlpha,								"Alpha"				},
		{	kFskGraphicsModeColorize,							"Colorize"			},

	};
	return StringFromCode(mode, modeTab);
}

static void LogScaleOffset(const FskScaleOffset *scaleOffset) {
	if (scaleOffset)
		LOGD("\tscale(%.8f, %.8f), offset(%.5f, %.5f)",
			scaleOffset->scaleX  * (1./((double)(1 << kFskScaleBits))),
			scaleOffset->scaleY  * (1./((double)(1 << kFskScaleBits))),
			scaleOffset->offsetX * (1./((double)(1 << kFskOffsetBits))),
			scaleOffset->offsetY * (1./((double)(1 << kFskOffsetBits))));
}

static void LogBitmap(FskConstBitmap bm, const char *name) {
	#if FSKBITMAP_OPENGL
		if (!name) name = "BM";
		if (bm->glPort)
			LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%d, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, texture=#%d",
				name, bm->bounds.x, bm->bounds.y, bm->bounds.width, bm->bounds.height, bm->depth,
				FskBitmapFormatName(bm->pixelFormat), bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied,
				FskGLPortSourceTexture(bm->glPort));
		else
	#endif /* FSKBITMAP_OPENGL */
	LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%d, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d",
		name, bm->bounds.x, bm->bounds.y, bm->bounds.width, bm->bounds.height, bm->depth,
		FskBitmapFormatName(bm->pixelFormat), bm->rowBytes, bm->bits, bm->hasAlpha, bm->alphaIsPremultiplied);
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!name) name = "RECT";
	if (r) LOGD("\t%s(%d, %d, %d, %d)", name, r->x, r->y, r->width, r->height);
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	if (!name) name = "COLOR";
	if (color) LOGD("\t%s(%d, %d, %d, %d)", name, color->r, color->g, color->b, color->a);
}

static void LogModeParams(FskConstGraphicsModeParameters modeParams) {
	if (modeParams) {
		if (modeParams->dataSize <= sizeof(FskGraphicsModeParametersRecord)) {
			LOGD("\tmodeParams: dataSize=%d, blendLevel=%d", modeParams->dataSize, modeParams->blendLevel);
		} else {
			FskGraphicsModeParametersVideo videoParams = (FskGraphicsModeParametersVideo)modeParams;
			LOGD("\tmodeParams: dataSize=%d, blendLevel=%d, kind='%4s, contrast=%f, brightness=%f, sprites=%p",
				videoParams->header.dataSize, videoParams->header.blendLevel, &videoParams->kind, videoParams->contrast, videoParams->brightness, videoParams->sprites);
		}
	}
}

static void LogBitmapDraw(
	FskConstBitmap					srcBM,		/* Source bitmap */
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,	/* ...to this rect */
	FskConstRectangle				dstClip,	/* But clip thuswise */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("BitmapDraw(srcBM=%p, srcRect=%p, dstBM=%p, dstRect=%p, dstClip=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstRect, dstClip, opColor, ModeString(mode), modeParams);
	LogBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

static void LogScaleOffsetBitmap(
	FskConstBitmap					srcBM,		/* Source bitmap */
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,	/* But clip thuswise */
	const FskScaleOffset 			*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("FskScaleOffsetBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstClip=%p, scaleOffset=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstClip, scaleOffset, opColor, ModeString(mode), modeParams);
	LogBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogBitmap(dstBM, "dstBM");
	LogRect(dstClip, "dstClip");
	LogScaleOffset(scaleOffset);
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

static void LogTileBitmap(
	FskConstBitmap					srcBM,				/* Source bitmap */
	FskConstRectangle				srcRect,			/* Bounds of source bitmaps to be used */
	FskBitmap						dstBM,				/* Destination bitmap */
	FskConstRectangle				dstRect,			/* Bounds to tile in destination*/
	FskConstRectangle				dstClip,			/* Clip of destination, incorporating source clip */
	FskFixed						scale,				/* source tile scale */
	FskConstColorRGBA				opColor,			/* Operation color used if needed for the given transfer mode */
	UInt32							mode,				/* Transfer mode, incorporating quality */
	FskConstGraphicsModeParameters	modeParams			/* We get blend level and tint color from here */
) {
	LOGD("TileBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstRect=%p, dstClip=%p, scale=%.6g opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstRect, dstClip, scale * (1./65536.), opColor, ModeString(mode), modeParams);
	LogBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}
#endif /* LOG_PARAMETERS */


/****************************************************************************//**
 * ScaleOffsetBitmap scales and offsets a bitmap.
 *	\param[in]	srcBM,				Source bitmap.
 *	\param[in]	srcClipRect			Bounds of source bitmaps to be used (can be NULL).
 *	\param[out]	dstBM				Destination bitmap.
 *	\param[in]	dstClipRect			Clip of destination, incorporating source clip (can be NULL).
 *	\param[in]	invScaleOffset		Mapping from destination to source, using FWD_SUBBITS fractional bits.
 *	\param[in]	opColor				Operation color used if needed for the given transfer mode.
 *	\param[in]	mode				Transfer mode, incorporating quality.
 *	\param[in]	modeParams			We get blend level and tint color from here.
 *
 * The inverse transform is specified, which maps the destination to the source.
 * The dstClipRect is assumed to have been intersected with the transformed srcClipRect.
 * The srcClipRect is used only for bounds checking.
 ********************************************************************************/

static FskErr
ScaleOffsetBitmap(
	FskConstBitmap					srcBM,				/* Source bitmap */
	FskConstRectangle				srcClipRect,		/* Bounds of source bitmaps to be used */
	FskBitmap						dstBM,				/* Destination bitmap */
	FskConstRectangle				dstClipRect,		/* Clip of destination, incorporating source clip */
	const FskScaleOffset			*invScaleOffset,	/* Mapping from destination to source, using FWD_SUBBITS fractional bits */
	FskConstColorRGBA				opColor,			/* Operation color used if needed for the given transfer mode */
	UInt32							mode,				/* Transfer mode, incorporating quality */
	FskConstGraphicsModeParameters	modeParams			/* We get blend level and tint color from here */
)
{
	FskRectBlitParams	p;
	FskErr				err = kFskErrNone;
	int					i, j;
	int					srcIndex, dstIndex, opIndex, quality;
	FskRectTransferProc	blitProc;
	FskRectangleRecord	mySrcClipRect;
	FskRectangleRecord	myDstClipRect;
#if USE_FRAMEBUFFER_VECTORS
	UInt32				srcDstBitmap[4];
#endif


	#ifdef LOG_PARAMETERS
		LOGD("ScaleOffsetBitmap(srcBM=%p srcClipRect=%p, dstBM=%p dstClipRect=%p invScaleOffset=%p opColor=%p mode=%s modeParams=%p)",
			srcBM, srcClipRect, dstBM, dstClipRect, invScaleOffset, opColor, ModeString(mode), modeParams
		);
		LogBitmap(srcBM, "srcBM");
		LogRect(srcClipRect, "srcClipRect");
		LogBitmap(dstBM, "dstBM");
		LogRect(dstClipRect, "dstClipRect");
		LogScaleOffset(invScaleOffset);
		LogColor(opColor, "opColor");
		LogModeParams(modeParams);
	#endif /* LOG_PARAMETERS */

	QUIET_BAIL_IF_TRUE(	-2147483647 >= invScaleOffset->offsetX || 2147483647 == invScaleOffset->offsetX ||		/* Saturation indicates out of coordinate range */
						-2147483647 >= invScaleOffset->offsetY || 2147483647 == invScaleOffset->offsetY,
						err, kFskErrNone);


	if (srcClipRect)	QUIET_BAIL_IF_FALSE(FskRectangleIntersect(srcClipRect, &srcBM->bounds, &mySrcClipRect), err, kFskErrNone);
	else				{ mySrcClipRect = srcBM->bounds; srcClipRect = &srcBM->bounds; }
	if (dstClipRect)	QUIET_BAIL_IF_FALSE(FskRectangleIntersect(dstClipRect, &dstBM->bounds, &myDstClipRect), err, kFskErrNone);
	else				myDstClipRect = dstBM->bounds;

	/* We provide an extension mechanism for platform-specific communication to the blit procs,
	 * instigated by a need to interface to hardware acceleration. An untyped pointer is provided
	 * to point to additional data.
	 */
	#if USE_FRAMEBUFFER_VECTORS			/* Escher needs to get access to additional custom elements of the src and dst BitMap data structures */
		srcDstBitmap[0]	= (UInt32)srcBM;
		srcDstBitmap[1]	= (UInt32)(&mySrcClipRect);
		srcDstBitmap[2]	= (UInt32)dstBM;
		srcDstBitmap[3]	= (UInt32)(&myDstClipRect);
		p.extension		= srcDstBitmap;
	#else				/* No one else needs this extra data */
		p.extension		= NULL;
	#endif /* USE_FRAMEBUFFER_VECTORS */

	/* Clip YUV destinations to have even x, y, width and height */
	if (dstBM->pixelFormat == kFskBitmapFormatYUV420) {				/* Accommodate YUV alignment */
		if (1 & myDstClipRect.x)		{	myDstClipRect.x++;		myDstClipRect.width--;	err = kFskErrUnalignedYUV;	}	/* Assure that      X is even */
		if (1 & myDstClipRect.y)		{	myDstClipRect.y++;		myDstClipRect.height--;	err = kFskErrUnalignedYUV;	}	/* Assure that      Y is even */
		if (1 & myDstClipRect.width)	{	myDstClipRect.width--;							err = kFskErrUnalignedYUV;	}	/* Assure that  width is even */
		if (1 & myDstClipRect.height)	{	myDstClipRect.height--;							err = kFskErrUnalignedYUV;	}	/* Assure that height is even */
	}

	/* Change offsets to have the origin at the upper left corner */
	p.srcXInc = invScaleOffset->scaleX;
	p.srcYInc = invScaleOffset->scaleY;
	p.srcX0   = invScaleOffset->offsetX + p.srcXInc * myDstClipRect.x - ((mySrcClipRect.x - srcClipRect->x) << FWD_SUBBITS);
	p.srcY0   = invScaleOffset->offsetY + p.srcYInc * myDstClipRect.y - ((mySrcClipRect.y - srcClipRect->y) << FWD_SUBBITS);

	/* Set blending, tint, and quality parameters */
	if ((false == srcBM->hasAlpha) && (kFskGraphicsModeAlpha == (mode & kFskGraphicsModeMask)))
		mode = kFskGraphicsModeCopy | (~kFskGraphicsModeMask & mode);
	p.isPremul = srcBM->hasAlpha && srcBM->alphaIsPremultiplied;
	SetParamOpColor(&p, opColor, modeParams);

	/* contrast, brightness */
	if ((NULL != modeParams) && (modeParams->dataSize >= sizeof(FskGraphicsModeParametersVideoRecord)) && ('cbcb' == ((FskGraphicsModeParametersVideo)modeParams)->kind)) {
		p.contrast = ((FskGraphicsModeParametersVideo)modeParams)->contrast;
		p.brightness = ((FskGraphicsModeParametersVideo)modeParams)->brightness;
		p.sprites = ((FskGraphicsModeParametersVideo)modeParams)->sprites;
	}
	else {
		p.contrast = 0;
		p.brightness = 0;
		p.sprites = 0;
	}
	p.srcPixelFormat = srcBM->pixelFormat;
	p.dstPixelFormat = dstBM->pixelFormat;

	/* Src image description */
	p.srcBaseAddr  = srcBM->bits;												/* BaseAddr of srcBM->bounds */
	p.srcRowBytes  = srcBM->rowBytes;
	j              = srcBM->bounds.height * p.srcRowBytes;						/* The size of the Y plane */
	p.srcUBaseAddr = (void*)(((char*)p.srcBaseAddr)  +  j);						/* There are very specific relationships between the Y and the U ... */
	j              = (srcBM->bounds.height >> 1) * (p.srcRowBytes >> 1);		/* (the size of the U plane) */
	p.srcVBaseAddr = (void*)(((char*)p.srcUBaseAddr) + j);						/* ... and V planes */

	i              = (mySrcClipRect.x - srcBM->bounds.x);						/* Offset to mySrcClipRect.x */
	if (srcBM->pixelFormat != kFskBitmapFormatYUV420)							/* depth isn'y useful for YUV420 */
		i         *= (srcBM->depth >> 3);										/* Offset to mySrcClipRect.x */
	if( srcBM->pixelFormat == kFskBitmapFormatYUV420i )
	{
		int dy = mySrcClipRect.y - srcBM->bounds.y;

		if( dy%2 != 0 )
			dy++;
		j          = dy * srcBM->rowBytes;		/* Offset to mySrcClipRect.y */
		j /= 2;
	}
	else
		j          = (mySrcClipRect.y - srcBM->bounds.y) * srcBM->rowBytes;		/* Offset to mySrcClipRect.y */

	p.srcBaseAddr  = (void*)((char*)(p.srcBaseAddr ) +  i       +  j);			/* srcbaseAddr  of srcClipRect */
	i >>= 1;																	/* Half the pixels for U and V */
	j              = ((mySrcClipRect.y - srcBM->bounds.y) >> 1) * (srcBM->rowBytes >> 1);	/* Offset to uClipRect->y, using half the height and half the rowbytes of Y */
	p.srcUBaseAddr = (void*)((char*)(p.srcUBaseAddr) + i + j);					/* srcUBaseAddr of srcClipRect */
	p.srcVBaseAddr = (void*)((char*)(p.srcVBaseAddr) + i + j);					/* srcVBaseAddr of srcClipRect */
	p.srcWidth     = mySrcClipRect.width;										/* srcWidth is only used for clipping */
	p.srcHeight    = mySrcClipRect.height;										/* srcHeight is only used for clipping */

	/* Dst image description: coordinates relative to dClip */
	p.dstBaseAddr  = dstBM->bits;												/* BaseAddr of dstBM->bounds */
	p.dstRowBytes  = dstBM->rowBytes;
	j              = dstBM->bounds.height * p.dstRowBytes;						/* The size of the Y plane */
	p.dstUBaseAddr = (void*)(((char*)p.dstBaseAddr)  +  j);						/* There are very specific relationships between the Y and the U ... */
	j              = (dstBM->bounds.height >> 1) * (p.dstRowBytes >> 1);		/* (the size of the U plane) */
	p.dstVBaseAddr = (void*)(((char*)p.dstUBaseAddr) + j);						/* ... and V planes */
	i              = (myDstClipRect.x - dstBM->bounds.x);						/* Offset to myDstClipRect.x */
	if (dstBM->pixelFormat != kFskBitmapFormatYUV420)							/* depth isn'y useful for YUV420 */
		i         *= (dstBM->depth >> 3);										/* Offset to myDstClipRect.x */
	j              = (myDstClipRect.y - dstBM->bounds.y) * dstBM->rowBytes;		/* Offset to myDstClipRect.y */
	p.dstBaseAddr  = (void*)((char*)(p.dstBaseAddr ) +  i       +  j);			/* dstbaseAddr  of myDstClipRect */
	i >>= 1;																	/* Half the pixels for U and V */
	j              = ((myDstClipRect.y - dstBM->bounds.y) >> 1) * (dstBM->rowBytes >> 1);	/* Offset to uClipRect->y */
	p.dstUBaseAddr = (void*)((char*)(p.dstUBaseAddr) + i + j);					/* dstUBaseAddr of myDstClipRect */
	p.dstVBaseAddr = (void*)((char*)(p.dstVBaseAddr) + i + j);					/* dstVBaseAddr of myDstClipRect */
	p.dstWidth     = myDstClipRect.width;										/* dst bounds now given by ((0,0,), (p.dstWidth, p.dstHeight)) */
	p.dstHeight    = myDstClipRect.height;

	/* Get dispatch selectors */
	srcIndex	= PixelFormatToProcTableSrcIndex(srcBM->pixelFormat);
	dstIndex	= PixelFormatToProcTableDstIndex(dstBM->pixelFormat);
	opIndex		= ModeToProcTableOpIndex(mode & kFskGraphicsModeMask, p.alpha);
	BAIL_IF_TRUE(((srcIndex < 0) || (dstIndex < 0)), err, kFskErrUnsupportedPixelType);
	BAIL_IF_TRUE(( opIndex  < 0),                    err, kFskErrInvalidParameter);

	/* Initialize the dispatch table by patching in machine-specific blitters */
	/* Tests Force CPU to enable different arch implementation */
	if (!patchInstalled) {
		InstallPatches();
		patchInstalled = 1;
	}

	quality = 0;																						/* Point-sampled or bilinear */
	if ((mode & kFskGraphicsModeBilinear) && ((p.srcXInc != FWD_ONE) || (p.srcYInc != FWD_ONE) || ((p.srcX0 & FWD_FRAC_MASK) != 0) || ((p.srcY0 & FWD_FRAC_MASK) != 0)))
		quality = 1;

	/* Get blit proc and invoke it, looking for an optimized unity proc first */
	if (	(NUM_DST_UNITY_FORMATS == 0)
		||	(p.srcXInc != FWD_ONE)																		/* X scale isn't 1 */
		||	(p.srcYInc != FWD_ONE)																		/* Y scale isn't 1 */
		||	((quality  != 0) && (((p.srcX0 & FWD_FRAC_MASK) != 0) || ((p.srcY0 & FWD_FRAC_MASK) != 0)))	/* High quality with subpixel positioning */
		||	((blitProc = GetUnityProc(srcIndex, dstIndex, opIndex)) == NULL)							/* No unity proc exists */
	) {
		BAIL_IF_NULL((blitProc = procDispatch[dstIndex][srcIndex][opIndex][quality]), err, kFskErrUnimplemented);	/* Get proc from dispatch table */
	}

	i       = FWD_HALF >> (quality << 2);																/* Rounding: 1/2 for point sampling, 1/32 for bilinear interpolation */
	p.srcX0 += i;																						/* Pre-round for the blit procs */
	p.srcY0 += i;

	(*blitProc)(&p);

bail:
	return(err);
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***		API
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskBitmapDraw
 *
 * The transformation is determined by
 *		srcRect -> dstRect
 * Mode is one of
 *		kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize
 *
 ********************************************************************************/

FskErr
FskBitmapDraw(
	FskConstBitmap					srcBM,		/* This should be declared FskConstBitmap */
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,	/* ...to this rect */
	FskConstRectangle				dstClip,	/* But clip thuswise */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
)
{
	FskRectangleRecord	srcTranRect, dstTranRect, srcClipRect, dstClipRect;
	FskErr				err = kFskErrNone;
	int					i;
	FskScaleOffset		invXfm;

	#ifdef LOG_PARAMETERS
		LogBitmapDraw(srcBM, srcRect, dstBM, dstRect, dstClip, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dstBM)) {
		return FskGLBitmapDraw(srcBM, srcRect, dstBM, dstRect, dstClip, opColor, mode, modeParams);
	}
	#endif /* FSKBITMAP_OPENGL */

	FskBitmapReadBegin((FskBitmap)srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(dstBM, NULL, NULL, NULL);

	/* Rects of transformation and clipping: srcTranRect, dstTranRect, srcClipRect, dstClipRect */
	if (srcRect) {	srcTranRect = *srcRect;	QUIET_BAIL_IF_FALSE(FskRectangleIntersect(srcRect, &srcBM->bounds, &srcClipRect), err, kFskErrNone);	}
	else {			srcTranRect =			srcClipRect = srcBM->bounds; srcRect = &srcBM->bounds;													}
	if (dstRect) {	dstTranRect = *dstRect;	QUIET_BAIL_IF_FALSE(FskRectangleIntersect(dstRect, &dstBM->bounds, &dstClipRect), err, kFskErrNone);	}
	else {			dstTranRect =			dstClipRect = dstBM->bounds;																			}
	if (dstClip) {							QUIET_BAIL_IF_FALSE(FskRectangleIntersect(dstClip, &dstClipRect,   &dstClipRect), err, kFskErrNone);	}

	/* If YUV, coerce srcClipRect to have even pixel alignment. YUV 4:2:0 is guaranteed to have an even number of pixels by definition */
	if (srcBM->pixelFormat == kFskBitmapFormatYUV420)
		EnforceYUVRectConstraints(&srcClipRect);

	/* Check for widths too small. If the destination width/height is 1, then we have an ambiguous
	 * many-to-one mapping. If the source width/height is 1, we just smear one pixel out over the destination.
	 */
	if (	(--srcTranRect.width  < 0)
		||	(--srcTranRect.height < 0)
		||	(--dstTranRect.width  < 0)
		||	(--dstTranRect.height < 0)
	)
        BAIL(kFskErrBadData);
	if (dstTranRect.width == 0) {	/* If the destination is 1 pixel in width, double the source width and destination width to disambiguate the mapping */
		dstTranRect.width = 1;
		srcTranRect.width = (srcTranRect.width << 1) + 1;	/* (srcTranRect.width + 1) * 2 - 1 */
	}
	if (dstTranRect.height == 0) {	/* If the destination is 1 pixel in height, double the source width and destination width to disambiguate the mapping */
		dstTranRect.height = 1;
		srcTranRect.height = (srcTranRect.height << 1) + 1;	/* (srcTranRect.height + 1) * 2 - 1 */
	}

	/* Compute the inverse transformation, mapping destination to source.
	 * Also, intersect the dstClipRect by the transformed srcClipRect.
	 */
	if ((srcTranRect.height == dstTranRect.height) && (srcTranRect.width == dstTranRect.width)) {
		UInt32 offsetDtoS;

		/* Compute transformation for X */
		offsetDtoS = srcTranRect.x - dstTranRect.x;
		invXfm.scaleX  = FWD_ONE;
		invXfm.offsetX = (offsetDtoS - srcRect->x) << FWD_SUBBITS;

		/* Compute clipping for X: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.width != 0) {
			if (dstClipRect.x      < (i = (srcClipRect.x                         - offsetDtoS)))
				dstClipRect.x      =  i;
			if (dstClipRect.width  > (i = (srcClipRect.x + srcClipRect.width - 1 - offsetDtoS) - dstClipRect.x + 1))
				dstClipRect.width  =  i;
		}

		/* Compute transformation for Y */
		offsetDtoS = srcTranRect.y - dstTranRect.y;
		invXfm.scaleY  = FWD_ONE;
		invXfm.offsetY = (offsetDtoS - srcRect->y) << FWD_SUBBITS;

		/* Compute clipping for Y: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.height != 0) {
			if (dstClipRect.y      < (i = (srcClipRect.y                          - offsetDtoS)))
				dstClipRect.y      =  i;
			if (dstClipRect.height > (i = (srcClipRect.y + srcClipRect.height - 1 - offsetDtoS) - dstClipRect.y + 1))
				dstClipRect.height =  i;
		}
	}
	else {
	#if TARGET_RT_FPU
		double	scaleDtoS, offsetDtoS, scaleStoD;

		/* Compute transformation for X */
		scaleDtoS  = (double)(srcTranRect.width)  / (double)(dstTranRect.width);
		offsetDtoS = srcTranRect.x - scaleDtoS * dstTranRect.x;
		invXfm.scaleX  = RoundDoubleToFWD(scaleDtoS);
		invXfm.offsetX = RoundDoubleToFWD(offsetDtoS - srcRect->y);

		/* Compute clipping for X: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.width != 0) {
			scaleStoD = 1.0 / scaleDtoS;
			if (dstClipRect.x      < (i = lceil( (srcClipRect.x                         - offsetDtoS) * scaleStoD)))
				dstClipRect.x      =  i;
			if (dstClipRect.width  > (i = lfloor((srcClipRect.x + srcClipRect.width - 1 - offsetDtoS) * scaleStoD) - dstClipRect.x + 1))
				dstClipRect.width  =  i;
		}

		/* Compute transformation for Y */
		scaleDtoS  = (double)(srcTranRect.height) / (double)(dstTranRect.height);
		offsetDtoS = srcTranRect.y - scaleDtoS * dstTranRect.y;
		invXfm.scaleY  = RoundDoubleToFWD(scaleDtoS);
		invXfm.offsetY = RoundDoubleToFWD(offsetDtoS - srcRect->y);

		/* Compute clipping for Y: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.height != 0) {
			scaleStoD = 1.0 / scaleDtoS;
			if (dstClipRect.y      < (i = lceil( (srcClipRect.y                          - offsetDtoS) * scaleStoD)))
				dstClipRect.y      =  i;
			if (dstClipRect.height > (i = lfloor((srcClipRect.y + srcClipRect.height - 1 - offsetDtoS) * scaleStoD) - dstClipRect.y + 1))
				dstClipRect.height =  i;
		}

	#else /* TARGET_RT_FPU */

		/* Compute transformation for X */
		invXfm.scaleX  = FskFixedNDiv(srcTranRect.width, dstTranRect.width, FWD_SUBBITS);
		#ifdef LESS_ACCURATE_COMPUTATIONS
			invXfm.offsetX = ((srcTranRect.x - srcRect->x) << FWD_SUBBITS) - invXfm.scaleX * dstTranRect.x;
		#else /* MORE_ACCURATE_COMPUTATIONS */
			invXfm.offsetX = FskFixedNRatio(dstTranRect.x, srcTranRect.width, dstTranRect.width, FWD_SUBBITS);
			if (0x7fffffff == invXfm.offsetX)
				invXfm.offsetX = invXfm.scaleX * dstTranRect.x;
			invXfm.offsetX = ((srcTranRect.x - srcRect->x) << FWD_SUBBITS) - invXfm.offsetX;
		#endif /* MORE_ACCURATE_COMPUTATIONS */

		/* Compute clipping for X: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.width != 0) {
			#ifdef LESS_ACCURATE_COMPUTATIONS
				i = (FskFixed)(((FskInt64)(((srcClipRect.x << FWD_SUBBITS) - invXfm.offsetX)) * dstTranRect.width + (srcTranRect.width << FWD_SUBBITS) - 1) / (srcTranRect.width << FWD_SUBBITS));
			#else /* MORE_ACCURATE_COMPUTATIONS */
				i = FskCeilFixedToInt(FskFixedNRatio(srcClipRect.x - srcTranRect.x, dstTranRect.width, srcTranRect.width, 16)) + dstTranRect.x;
			#endif /* MORE_ACCURATE_COMPUTATIONS */
			if (dstClipRect.x < i)
				dstClipRect.x = i;
			#ifdef LESS_ACCURATE_COMPUTATIONS
				i = (FskFixed)(((FskInt64)(((srcClipRect.x + srcClipRect.width - 1) << FWD_SUBBITS) - invXfm.offsetX) * dstTranRect.width) / (srcTranRect.width << FWD_SUBBITS)) - dstClipRect.x + 1;
			#else /* MORE_ACCURATE_COMPUTATIONS */
				i = (srcClipRect.x - srcTranRect.x + srcClipRect.width - 1) * dstTranRect.width / srcTranRect.width + dstTranRect.x - dstClipRect.x + 1;
			#endif /* MORE_ACCURATE_COMPUTATIONS */
			if (dstClipRect.width > i)
				dstClipRect.width =  i;
		}

		/* Compute transformation for Y */
		invXfm.scaleY  = FskFixedNDiv(srcTranRect.height, dstTranRect.height, FWD_SUBBITS);
		#ifdef LESS_ACCURATE_COMPUTATIONS
			invXfm.offsetY = ((srcTranRect.y - srcRect->y) << FWD_SUBBITS) - invXfm.scaleY * dstTranRect.y;
		#else /* MORE_ACCURATE_COMPUTATIONS */
			invXfm.offsetY = FskFixedNRatio(dstTranRect.y, srcTranRect.height, dstTranRect.height, FWD_SUBBITS);
			if (0x7fffffff == invXfm.offsetY)
				invXfm.offsetY = invXfm.scaleY * dstTranRect.y;
			invXfm.offsetY = ((srcTranRect.y - srcRect->y) << FWD_SUBBITS) - invXfm.offsetY;
		#endif /* MORE_ACCURATE_COMPUTATIONS */

		/* Compute clipping for Y: transform srcClipRect to dst and clip dstClipRect */
		if (srcTranRect.height != 0) {
			#ifdef LESS_ACCURATE_COMPUTATIONS
				i = (FskFixed)(((FskInt64)((srcClipRect.y << FWD_SUBBITS) - invXfm.offsetY) * dstTranRect.height + (srcTranRect.height << FWD_SUBBITS) - 1) / (srcTranRect.height << FWD_SUBBITS));
			#else /* MORE_ACCURATE_COMPUTATIONS */
				i = FskCeilFixedToInt(FskFixedNRatio(srcClipRect.y - srcTranRect.y, dstTranRect.height, srcTranRect.height, 16)) + dstTranRect.y;
			#endif /* MORE_ACCURATE_COMPUTATIONS */
			if (dstClipRect.y < i)
				dstClipRect.y = i;
			#ifdef LESS_ACCURATE_COMPUTATIONS
				i = (FskFixed)(((FskInt64)(((srcClipRect.y + srcClipRect.height - 1) << FWD_SUBBITS) - invXfm.offsetY) * dstTranRect.height) / (srcTranRect.height << FWD_SUBBITS)) - dstClipRect.y + 1;
			#else /* MORE_ACCURATE_COMPUTATIONS */
				i = (srcClipRect.y - srcTranRect.y + srcClipRect.height - 1) * dstTranRect.height / srcTranRect.height + dstTranRect.y - dstClipRect.y + 1;
			#endif /* MORE_ACCURATE_COMPUTATIONS */
			if (dstClipRect.height > i)
				dstClipRect.height =  i;
		}
	#endif /* TARGET_RT_FPU */
	}

	/* If everything is clipped out, we're done */
	QUIET_BAIL_IF_TRUE(((dstClipRect.width <= 0) || (dstClipRect.height <= 0)), err, kFskErrNone);
	#if BLIT_DEBUG
		if (((invXfm.offsetX & FWD_FRAC_MASK) && (invXfm.scaleX == FWD_ONE)) || ((invXfm.offsetY & FWD_FRAC_MASK) && (invXfm.scaleY == FWD_ONE)))
			LOGE("FskBitmapDraw calls ScaleOffsetBitmap with scale=(%.6f,%.6f) offset=(%.6f,%.6f)%s",
				invXfm.scaleX/(float)FWD_ONE, invXfm.scaleY/(float)FWD_ONE, invXfm.offsetX/(float)FWD_ONE, invXfm.offsetY/(float)FWD_ONE,
				(((invXfm.scaleX == FWD_ONE) || (invXfm.scaleY == FWD_ONE)) ? " UNITY" : ""));
		//if ((invXfm.offsetX & FWD_FRAC_MASK) | (invXfm.offsetY & FWD_FRAC_MASK))
		//	LOGD("FskBitmapDraw calls ScaleOffsetBitmap with scale=(%.6f,%.6f) offset=(%.6f,%.6f) %s",
		//		invXfm.scaleX/(float)FWD_ONE, invXfm.scaleY/(float)FWD_ONE, invXfm.offsetX/(float)FWD_ONE, invXfm.offsetY/(float)FWD_ONE,
		//		(((invXfm.scaleX == FWD_ONE) || (invXfm.scaleY == FWD_ONE)) ? "UNITY" : ""));
	#endif /* BLIT_DEBUG */
	err = ScaleOffsetBitmap(srcBM, &srcClipRect, dstBM, &dstClipRect, &invXfm, opColor, mode, modeParams);

	#if TARGET_OS_LINUX
		if (err) {
			LOGD("FskBitmapDraw - calling ScaleOffsetBitamp returned %d srcFmt:%u, dstFmt:%u\n",
					(int)err, (unsigned)(srcBM->pixelFormat), (unsigned)(dstBM->pixelFormat));
			LOGD("              - src: [%d %d %d %d] dst: [ %d %d %d %d]\n",
					(int)srcClipRect.x, (int)srcClipRect.y, (int)srcClipRect.width, (int)srcClipRect.height,
					(int)dstClipRect.x, (int)dstClipRect.y, (int)dstClipRect.width, (int)dstClipRect.height);
		}
	#endif /* TARGET_OS_LINUX */

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapWriteEnd(dstBM);

	/* If we copy from straight to premul, make sure to convert it */
// 	if (!err && !srcBM->alphaIsPremultiplied && dstBM->alphaIsPremultiplied && (mode & kFskGraphicsModeMask) != kFskGraphicsModeAlpha && dstBM->hasAlpha && srcBM->hasAlpha)
// 		FskStraightAlphaToPremultipliedAlpha(dstBM, &dstRect);

	return(err);
}


/********************************************************************************
 * FskScaleOffsetBitmap
 ********************************************************************************/

FskErr
FskScaleOffsetBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const FskScaleOffset 			*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* blend level tint color */
)
{
	FskRectangleRecord	srcClipRect, dstClipRect;
	FskErr				err = kFskErrNone;
	FskScaleOffset		invXfm;

	#ifdef LOG_PARAMETERS
		LogScaleOffsetBitmap(srcBM, srcRect, dstBM, dstClip, scaleOffset, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	#if FSKBITMAP_OPENGL
		if (FskBitmapIsOpenGLDestinationAccelerated(dstBM)) {
			return FskGLScaleOffsetBitmap(srcBM, srcRect, dstBM, dstClip, scaleOffset, opColor, mode, modeParams);
		}
	#endif /* FSKBITMAP_OPENGL */

	FskBitmapReadBegin((FskBitmap)srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(dstBM, NULL, NULL, NULL);

	/* Intersect srcRect and srcBounds */
	if (srcRect)	{ QUIET_BAIL_IF_FALSE(FskRectangleIntersect(srcRect, &srcBM->bounds, &srcClipRect), err, kFskErrNone);	}
	else			{ srcClipRect = srcBM->bounds;	srcRect = &srcBM->bounds;												}

	/* Transform srcClipRect -> dstClipRect */
	#if !TARGET_RT_FPU
	{	FskInt64 x0, x1, y0, y1, t;
			x0	= (srcClipRect.x - srcRect->x) * (FskInt64)(scaleOffset->scaleX) + ((FskInt64)(scaleOffset->offsetX) << (kFskScaleBits - kFskOffsetBits));
			y0	= (srcClipRect.y - srcRect->y) * (FskInt64)(scaleOffset->scaleY) + ((FskInt64)(scaleOffset->offsetY) << (kFskScaleBits - kFskOffsetBits));
		x1		= (srcClipRect.width  - 1)     * (FskInt64)(scaleOffset->scaleX) + x0;
		y1		= (srcClipRect.height - 1)     * (FskInt64)(scaleOffset->scaleY) + y0;
		if (x0 > x1) {	t = x0; x0 = x1; x1 = t; }													/* Accommodate reflection in X */
		if (y0 > y1) {	t = y0; y0 = y1; y1 = t; }													/* Accommodate reflection in Y */
		x0 = (x0 + ((1 << kFskScaleBits) - 1)) >> kFskScaleBits;									/* ceil */
		y0 = (y0 + ((1 << kFskScaleBits) - 1)) >> kFskScaleBits;									/* ceil */
		x1 = (x1 >> kFskScaleBits) + 1;																/* floor + 1 */
		y1 = (y1 >> kFskScaleBits) + 1;																/* floor + 1 */
		if (x0 < (t = dstBM->bounds.x))							x0 = t;								/* Clip against dstBM->bounds at higher precision than FskRectangleIntersect() */
		if (y0 < (t = dstBM->bounds.y)) 						y0 = t;
		if (x1 > (t = dstBM->bounds.x + dstBM->bounds.width ))	x1 = t;
		if (y1 > (t = dstBM->bounds.y + dstBM->bounds.height))	y1 = t;
		dstClipRect.x		= (SInt32)x0;															/* Convert to FskRectangle */
		dstClipRect.y		= (SInt32)y0;
		dstClipRect.width	= (SInt32)x1 - dstClipRect.x;
		dstClipRect.height	= (SInt32)y1 - dstClipRect.y;
	}
	#else /* TARGET_RT_FPU */
	{	double x0, x1, y0, y1, t;
		x1 = ldexp(scaleOffset->scaleX, -kFskScaleBits);											/* Convert scaleX from Fixed24 to double */
		y1 = ldexp(scaleOffset->scaleY, -kFskScaleBits);											/* Convert scaleY from Fixed24 to double */
			x0	= (srcClipRect.x - srcRect->x) * x1 + ldexp(scaleOffset->offsetX, -kFskOffsetBits);
			y0	= (srcClipRect.y - srcRect->y) * y1 + ldexp(scaleOffset->offsetY, -kFskOffsetBits);
		x1		= (srcClipRect.width  - 1) * x1 + x0;
		y1		= (srcClipRect.height - 1) * y1 + y0;
		if (x0 > x1) {	t = x0; x0 = x1; x1 = t; }													/* Accommodate reflection in X */
		if (y0 > y1) {	t = y0; y0 = y1; y1 = t; }													/* Accommodate reflection in Y */
		x0 = ceil(x0);
		y0 = ceil(y0);
		x1 = floor(x1) + 1.;
		y1 = floor(y1) + 1.;
		if (x0 < (t = dstBM->bounds.x))							x0 = t;								/* Clip against dstBM->bounds at higher precision than FskRectangleIntersect() */
		if (y0 < (t = dstBM->bounds.y)) 						y0 = t;
		if (x1 > (t = dstBM->bounds.x + dstBM->bounds.width ))	x1 = t;
		if (y1 > (t = dstBM->bounds.y + dstBM->bounds.height))	y1 = t;
		dstClipRect.x		= (SInt32)x0;															/* Convert to FskRectangle */
		dstClipRect.y		= (SInt32)y0;
		dstClipRect.width	= (SInt32)x1 - dstClipRect.x;
		dstClipRect.height	= (SInt32)y1 - dstClipRect.y;
	}
	#endif /* TARGET_RT_FPU */

	QUIET_BAIL_IF_FALSE(dstClipRect.width > 0 && dstClipRect.height > 0, err, kFskErrNone);

	/* Then intersect with dstClip */
	if (dstClip) {	QUIET_BAIL_IF_FALSE(FskRectangleIntersect(dstClip, &dstClipRect, &dstClipRect), err, kFskErrNone);	}

	/* If everything is clipped out, we're done */
	QUIET_BAIL_IF_TRUE(((dstClipRect.width <= 0) || (dstClipRect.height <= 0)), err, kFskErrNone);

	/* If YUV, coerce srcClipRect to have even pixel alignment. YUV 4:2:0 is guaranteed to have an even number of pixels by definition */
	if (srcBM->pixelFormat == kFskBitmapFormatYUV420)
		EnforceYUVRectConstraints(&srcClipRect);

	/* Invert the scale and offset, to map dst into src */
	invXfm.scaleX  =   FskFixedNDiv(FWD_ONE,                    scaleOffset->scaleX, kFskScaleBits);
	invXfm.scaleY  =   FskFixedNDiv(FWD_ONE,                    scaleOffset->scaleY, kFskScaleBits);
	invXfm.offsetX = -(FskFixedNDiv(scaleOffset->offsetX, scaleOffset->scaleX, FWD_SUBBITS + kFskScaleBits - kFskOffsetBits));
	invXfm.offsetY = -(FskFixedNDiv(scaleOffset->offsetY, scaleOffset->scaleY, FWD_SUBBITS + kFskScaleBits - kFskOffsetBits));

	err = ScaleOffsetBitmap(srcBM, &srcClipRect, dstBM, &dstClipRect, &invXfm, opColor, mode, modeParams);

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapWriteEnd(dstBM);

	/* If we copy from straight to premul, make sure to convert it */
// 	if (!err && !srcBM->alphaIsPremultiplied && dstBM->alphaIsPremultiplied && (mode & kFskGraphicsModeMask) != kFskGraphicsModeAlpha && dstBM->hasAlpha && srcBM->hasAlpha)
// 		FskStraightAlphaToPremultipliedAlpha(dstBM, &dstRect);

	return err;
}

/********************************************************************************
 * FskTileBitmap
 ********************************************************************************/

typedef SInt32 (*unscaleSIn32Proc)(FskFixed n, FskFixed d);
static SInt32 unscaleSInt3215x(FskFixed n, FskFixed d);
static SInt32 unscaleSInt32(FskFixed n, FskFixed d);
static SInt32 unscaleFixed(FskFixed n, FskFixed d);

FskErr
FskTileBitmap(
	FskConstBitmap					srcBM,				/* Source bitmap */
	FskConstRectangle				srcClipRect,		/* Bounds of source bitmaps to be used */
	FskBitmap						dstBM,				/* Destination bitmap */
	FskConstRectangle				dstRect,			/* Bounds to tile in destination*/
	FskConstRectangle				dstClipRect,		/* Clip of destination, incorporating source clip */
	FskFixed						scale,				/* source tile scale */
	FskConstColorRGBA				opColor,			/* Operation color used if needed for the given transfer mode */
	UInt32							mode,				/* Transfer mode, incorporating quality */
	FskConstGraphicsModeParameters	modeParams			/* We get blend level and tint color from here */
)
{
	FskRectBlitParams	p;
	FskErr				err = kFskErrNone;
	int					i, j, tileWidth, tileHeight;
	int					srcIndex, dstIndex, opIndex, quality;
	FskRectTransferProc	blitProc;
	FskRectangleRecord	myDstClipRect, leftTile;
	SInt32				right, bottom;
    FskBitmap           tmpBM = NULL;
    FskRectangleRecord  tmpClipRect;
	unscaleSIn32Proc	doUnscale;

	#ifdef LOG_PARAMETERS
		LogTileBitmap(srcBM, srcClipRect, dstBM, dstRect, dstClipRect, scale, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	#if FSKBITMAP_OPENGL
	if (FskBitmapIsOpenGLDestinationAccelerated(dstBM)) {
		return FskGLTileBitmap(srcBM, srcClipRect, dstBM, dstRect, dstClipRect, scale, opColor, mode, modeParams);
	}
	#endif /* FSKBITMAP_OPENGL */

	if (0x010000 == scale)
		doUnscale = unscaleSInt32;
	else if (0x018000 == scale)
		doUnscale = unscaleSInt3215x;
	else
		doUnscale = unscaleFixed;

	/* Clip the dstClipRect to the bounds of the dst and the dstRect */
	if (!dstClipRect)
		myDstClipRect = dstBM->bounds;
	else if (!FskRectangleIntersect(&dstBM->bounds, dstClipRect, &myDstClipRect))
		return kFskErrNothingRendered;
	if (!dstRect)
		dstRect = &dstBM->bounds;
	else if (!FskRectangleIntersect(dstRect, &myDstClipRect, &myDstClipRect))
		return kFskErrNothingRendered;

	/* Assure that the srcClipRect is within the srcBounds */
	if (!srcClipRect)
		srcClipRect = &srcBM->bounds;
	else if ((srcClipRect->x < srcBM->bounds.x) || (srcClipRect->y < srcBM->bounds.y) ||
		((srcClipRect->x + srcClipRect->width)  > srcBM->bounds.width) ||
		((srcClipRect->y + srcClipRect->height) > srcBM->bounds.height)
	)
		return kFskErrNothingRendered;

//	tileWidth  = FskFixMul(srcClipRect->width,  scale);								/* This is more accurate */
//	tileHeight = FskFixMul(srcClipRect->height, scale);
	tileWidth  = (int)(((FskInt64)(srcClipRect->width)  * scale) >> 16);			/* This is safer: truncate tile dimensions in case they are not integers */
	tileHeight = (int)(((FskInt64)(srcClipRect->height) * scale) >> 16);

	quality = (((mode & kFskGraphicsModeBilinear) != 0) && (FskIntToFixed(1) != scale)) ? 1 : 0;	/* No need to do bilinear interpolation with 1X scale */

	if (quality ||																	/* If doing bilinear interpolation, or */
		((scale != FskIntToFixed(1)) && (											/* Zooming in and ... */
			((srcClipRect->y + srcClipRect->height) == srcBM->bounds.height) ||		/* ... either going to the max height ... */
			((srcClipRect->x + srcClipRect->width)  == srcBM->bounds.width)			/* ...   or   going to the max width      */
		))
	) {
		/* Need to augment the bitmap so that it will wrap */
		Boolean	wrapX			= dstRect->width  > tileWidth;						/* It wraps horizontally if the dst is  wider than the tile */
		Boolean	wrapY			= dstRect->height > tileHeight;						/* It wraps   vertically if the dst is taller than the tile */
		UInt32	pixelBytes		= srcBM->depth >> 3;								/* The number of bytes per pixel */
		UInt32	srcWidthBytes	= srcClipRect->width * pixelBytes;					/* The number of active pixel bytes per line */
		UInt32	rightPixelBytes	= srcWidthBytes - pixelBytes;						/* The byte offset to the rightmost pixel */
		UInt8	*sp, *dp, *dpInitial;
		SInt32	srb, drb;

		BAIL_IF_ERR(err = FskBitmapNew(srcClipRect->width + 1, srcClipRect->height + 1, srcBM->pixelFormat, &tmpBM));	/* bitmap 1 wider and 1 taller */
		FskRectangleSet(&tmpClipRect, 0, 0, srcClipRect->width, srcClipRect->height);	/* tmpClipRect is the same as the srcClipRect, except its origin is at (0,0) */

		FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&sp, &srb, NULL);
		FskBitmapWriteBegin(          tmpBM,             (void**)(void*)&dp, &drb, NULL);

		sp += (srcClipRect->y * srb) + (srcClipRect->x * pixelBytes);				/* Copy the src in the srcClipRect, augmenting with an extra pixel horizontally and vertically */
		dpInitial = dp;																/* Pointer to the top of the destination */
		for (i = srcClipRect->height; i-- > 0; sp += srb, dp += drb) {				/* For each scanline */
			UInt8 *rep = sp;														/* Set up to replicate left pixel if wrapping */
			FskMemCopy(dp, sp, srcWidthBytes);										/* Copy scanline */
			if (!wrapX)
				rep += rightPixelBytes;												/* Set up to replicate right pixel if not wrapping */
			FskMemCopy(dp + srcWidthBytes, rep, pixelBytes);						/* Replicate the pixel */
		}
		FskMemCopy(dp, (wrapY ? dpInitial : dp - drb), srcWidthBytes + pixelBytes);	/* Replicate first or last line */

		FskBitmapReadEnd((FskBitmap)srcBM);
		FskBitmapWriteEnd(tmpBM);

		/* Replace src bitmap */
		tmpBM->hasAlpha = srcBM->hasAlpha;
		srcBM = tmpBM;
		srcClipRect = &tmpClipRect;
	}

	FskBitmapReadBegin((FskBitmap)srcBM, NULL, NULL, NULL);
	FskBitmapWriteBegin(dstBM, NULL, NULL, NULL);

	/* Change offsets to have the origin at the upper left corner */
	p.srcXInc = FskFixedNDivInline(FWD_ONE, scale, 16);	/* Source increment is the reciprocal of the scale */
	p.srcYInc = p.srcXInc;
	#ifdef ROUND_TO_NEAREST_TILE_COORDINATE
		i = FWD_HALF >> (quality << 2);	/* Rounding: 1/2 for point sampling, 1/32 for bilinear interpolation */
	#else/* TRUNCATE_TO_TILE_COORDINATE */
		i = 0;
	#endif/* TRUNCATE_TO_TILE_COORDINATE */
	p.srcX0   = i;
	p.srcY0   = i;

	/* Set blending, tint, and quality parameters */
	if ((false == srcBM->hasAlpha) && (kFskGraphicsModeAlpha == (mode & kFskGraphicsModeMask)))
		mode = kFskGraphicsModeCopy | (~kFskGraphicsModeMask & mode);
	p.isPremul = srcBM->hasAlpha && srcBM->alphaIsPremultiplied;
	SetParamOpColor(&p, opColor, modeParams);

	/* Get dispatch selectors */
	srcIndex	= PixelFormatToProcTableSrcIndex(srcBM->pixelFormat);
	dstIndex	= PixelFormatToProcTableDstIndex(dstBM->pixelFormat);
	opIndex		= ModeToProcTableOpIndex(mode & kFskGraphicsModeMask, p.alpha);
	BAIL_IF_TRUE(((srcIndex < 0) || (dstIndex < 0)), err, kFskErrUnsupportedPixelType);
	BAIL_IF_TRUE(( opIndex  < 0),                    err, kFskErrInvalidParameter);

	/* Get blit proc and invoke it, looking for an optimized unity proc first */
	if (	(NUM_DST_UNITY_FORMATS == 0)
		||	(0x10000 != scale)
		||	((blitProc = GetUnityProc(srcIndex, dstIndex, opIndex)) == NULL)				/* No unity proc exists */
	) {
		BAIL_IF_NULL((blitProc = procDispatch[dstIndex][srcIndex][opIndex][quality]), err, kFskErrInvalidParameter);	/* Get proc from dispatch table */
	}

	right  = myDstClipRect.x + myDstClipRect.width;
	bottom = myDstClipRect.y + myDstClipRect.height;

	leftTile.width	= FskFixMul(srcClipRect->width,	 scale);
	leftTile.height = FskFixMul(srcClipRect->height, scale);
	leftTile.x = (((myDstClipRect.x - dstRect->x) / leftTile.width)  * leftTile.width)  + dstRect->x;
	leftTile.y = (((myDstClipRect.y - dstRect->y) / leftTile.height) * leftTile.height) + dstRect->y;

	p.srcRowBytes  = srcBM->rowBytes;
	p.dstRowBytes  = dstBM->rowBytes;

	p.srcWidth     = srcClipRect->width;
	p.srcHeight    = srcClipRect->height;
	if (tmpBM) {
		p.srcWidth++;	/* This allows bilinear interpolation around the edges */
		p.srcHeight++;
	}

	for (; leftTile.y < bottom; leftTile.y += tileHeight) {
		FskRectangleRecord tile = leftTile;

		for (; tile.x < right; tile.x += tileWidth) {
			FskRectangleRecord tileClipped;

			FskRectangleIntersect(&myDstClipRect, &tile, &tileClipped);

			i              = (srcClipRect->x + doUnscale(tileClipped.x - tile.x, scale) - srcBM->bounds.x);							/* Offset to srcClipped.x */
			i			  *= (srcBM->depth >> 3);										/* Offset to srcClipped.x */
			j              = (srcClipRect->y + doUnscale(tileClipped.y - tile.y, scale) - srcBM->bounds.y) *	srcBM->rowBytes;		/* Offset to srcClipped.y */
			p.srcBaseAddr  = (void*)((char*)(srcBM->bits ) +  i       +  j);			/* srcbaseAddr  of srcClipRect */

			/* Dst image description: coordinates relative to dClip */
			i              = (tileClipped.x - dstBM->bounds.x);							/* Offset to myDstClipRect.x */
			i			  *= (dstBM->depth >> 3);										/* Offset to myDstClipRect.x */
			j              = (tileClipped.y - dstBM->bounds.y) *  dstBM->rowBytes;		/* Offset to myDstClipRect.y */
			p.dstBaseAddr  = (void*)((char*)(dstBM->bits ) +  i       +  j);			/* dstbaseAddr  of myDstClipRect */

			p.dstWidth     = tileClipped.width;											/* dst bounds now given by ((0,0,), (p.dstWidth, p.dstHeight)) */
			p.dstHeight    = tileClipped.height;

			(*blitProc)(&p);
		}
	}

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapWriteEnd(dstBM);
	if (tmpBM)
        FskBitmapDispose(tmpBM);

	/* If we copy from straight to premul, make sure to convert it */
// 	if (!err && !srcBM->alphaIsPremultiplied && dstBM->alphaIsPremultiplied && (mode & kFskGraphicsModeMask) != kFskGraphicsModeAlpha && dstBM->hasAlpha && srcBM->hasAlpha)
// 		FskStraightAlphaToPremultipliedAlpha(dstBM, dstRect);

	return(err);
}

#define UNSCALE15x(x) (((x << 1) + 1) / 3)

FskFixed unscaleSInt3215x(FskFixed v, FskFixed d)
{
	return UNSCALE15x(v);
}

FskFixed unscaleSInt32(FskFixed v, FskFixed d)
{
	return v;
}

FskFixed unscaleFixed(FskFixed n, FskFixed d)
{
	return FskFixDiv(n, d);
}


/********************************************************************************
 ********************************************************************************
 *****						Patching Rect Blit Procs						*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskPatchRectBlitProc
 *
 * The old proc is returned, unless there is an error,
 * in which case the result is negative, with value consistent with FskErrors.h.
 * Note that NULL is not an error -- it only indicates that there was no blit proc
 * previously installed.
 ********************************************************************************/

FskRectTransferProc
FskPatchRectBlitProc(
	FskRectTransferProc		blitProc,
	UInt32					srcPixelFormat,
	UInt32					dstPixelFormat,
	UInt32					mode,
	UInt32					canBlend
)
{
	FskErr				err	= kFskErrNone;
	int					srcIndex, dstIndex, opIndex, quality;
	FskRectTransferProc	*procSlot;

	srcIndex	= PixelFormatToProcTableSrcIndex(srcPixelFormat);
	dstIndex	= PixelFormatToProcTableDstIndex(dstPixelFormat);
	opIndex		= ModeToProcTableOpIndex(mode & kFskGraphicsModeMask, (canBlend ? 1 : 255));
	quality		= (mode & kFskGraphicsModeBilinear) ? 1 : 0;
	BAIL_IF_TRUE(((srcIndex < 0) || (dstIndex < 0)), err, kFskErrUnsupportedPixelType);
	BAIL_IF_TRUE(( opIndex  < 0),                    err, kFskErrInvalidParameter);
	procSlot	= &procDispatch[dstIndex][srcIndex][opIndex][quality];
	blitProc	= SwapProc(procSlot, blitProc);

bail:
	if (err)
		blitProc = (FskRectTransferProc)err;
	return blitProc;
}


/********************************************************************************
 * FskPatchUnityRectBlitProc
 ********************************************************************************/

FskRectTransferProc
FskPatchUnityRectBlitProc(
	FskRectTransferProc		blitProc,
	UInt32					srcPixelFormat,
	UInt32					dstPixelFormat,
	UInt32					mode,
	UInt32					canBlend
)
{
	int						srcIndex, dstIndex, opIndex;
	FskRectTransferProc		*procSlot		= NULL;

	if (	((srcIndex = PixelFormatToProcTableSrcIndex(srcPixelFormat)) >= 0)
		&&	((dstIndex = PixelFormatToProcTableDstIndex(dstPixelFormat)) >= 0)
		&&	((dstIndex = dstPixelKindUnityIndex[dstIndex]) >= 0)
	) {
		opIndex		= ModeToProcTableOpIndex(mode & kFskGraphicsModeMask, (canBlend ? 1 : 255));
		if      (opIndex == OP_INDEX_COPY)			procSlot = &unityCopyProc [dstIndex][srcIndex];
		else if (opIndex == OP_INDEX_ALPHA)			procSlot = &unityAlphaProc[dstIndex][srcIndex];
	}
	blitProc = (procSlot != NULL) ? SwapProc(procSlot, blitProc) : (FskRectTransferProc)kFskErrUnsupportedPixelType;

	return blitProc;
}


/********************************************************************************
 ********************************************************************************
 *****								Patches									*****
 ********************************************************************************
 ********************************************************************************/
#if TARGET_OS_KPL
	#include "FskPlatformImplementation.Kpl.h"
	#if USE_ARM_YUV_TO_RGB
		#define MY_YUV
	#endif
#elif (TARGET_CPU_ARM && !TARGET_OS_IPHONE)
	#define MY_YUV
#endif /* (TARGET_CPU_ARM && !TARGET_OS_IPHONE) */

#ifdef MY_YUV

extern void my_FskCopyYUV42016RGB565SE_unity_prototype_arm_v4
(
	UInt32 width,
	UInt32 height,
	UInt8 *yBase,
	SInt32 yRowBytes,
	UInt8 *uBase,
	UInt8 *vBase,
	SInt32 uvRowBytes,
	UInt32 *rgbBase,
	SInt32 rgbRowBytes
);

void my_FskCopyYUV42016RGB565SE_unity_bc_prototype_arm_v4
(
	unsigned char	*y0,
	unsigned char	*u0,
	unsigned char	*v0,
	unsigned char	*dst,
	int				B1,
	int				C,
	int				height,
	int				width,
	int				yrb,
	int				uvrb,
	int				drb
);

void my_FskCopyYUV42016RGB565SE_unity_bc_prototype_arm_v5
(
	unsigned char	*y0,
	unsigned char	*u0,
	unsigned char	*v0,
	unsigned char	*dst,
	int				B1,
	int				C,
	int				height,
	int				width,
	int				yrb,
	int				uvrb,
	int				drb
);

extern void my_FskCopyYUV42016RGB565SE_scale_bc_prototype_arm_v5
(
	int				width,
	int				height,
	int				Yrb,
	int				Crb,
	int				drb,
	unsigned char	*Y0,
	unsigned char	*U0,
	unsigned char	*V0,
	unsigned short	*d0,
	FskFixed		x0,
	FskFixed		y0,
	FskFixed		xd,
	FskFixed		yd,
	FskFixed		xCmax,
	FskFixed		yCmax,
	int				Bx,
	int				Cx
);

extern void my_FskCopyYUV42016RGB565SE_scale_bc_prototype_arm_v4
(
	int				width,
	int				height,
	int				Yrb,
	int				Crb,
	int				drb,
	unsigned char	*Y0,
	unsigned char	*U0,
	unsigned char	*V0,
	unsigned short	*d0,
	FskFixed		x0,
	FskFixed		y0,
	FskFixed		xd,
	FskFixed		yd,
	FskFixed		xCmax,
	FskFixed		yCmax,
	int				Bx,
	int				Cx
);

void my_FskCopyYUV42016RGB565SE_scale_prototype_arm_v4
(
	const FskRectBlitParams *params
);

static void my_FskCopyYUV42016RGB565SE_scale_bc_arm_v4(const FskRectBlitParams *params)
{
	//my_FskCopyYUV42016RGB565SE_82_bc_c_prototype
	if( params->contrast == 0 && params->brightness == 0 )
	{
		my_FskCopyYUV42016RGB565SE_scale_prototype_arm_v4(params);
	}
	else
	{
		int		width	= params->dstWidth;
		int		height	= params->dstHeight;
		int		Yrb		= params->srcRowBytes;
		int		Crb		= Yrb >> 1;
		int		drb		= params->dstRowBytes;

		unsigned char	*Y0	= (UInt8*)(params->srcBaseAddr );
		unsigned char	*U0	= (UInt8*)(params->srcUBaseAddr);
		unsigned char	*V0	= (UInt8*)(params->srcVBaseAddr);
		unsigned short	*d0	= (unsigned short*)(params->dstBaseAddr);

		FskFixed		x0	= params->srcX0   >> (FWD_SUBBITS - ASM_SUBBITS);		/* The YUV patch API assumes 16.16 */
		FskFixed		y0	= params->srcY0   >> (FWD_SUBBITS - ASM_SUBBITS);
		FskFixed		xd	= params->srcXInc >> (FWD_SUBBITS - ASM_SUBBITS);
		FskFixed		yd	= params->srcYInc >> (FWD_SUBBITS - ASM_SUBBITS);

		FskFixed		xCmax	= ((params->srcWidth  >> 1) - 1) << ASM_SUBBITS;
		FskFixed		yCmax	= ((params->srcHeight >> 1) - 1) << ASM_SUBBITS;

		int	constrast  = params->contrast;
		int	brightness = params->brightness;
		int C88, B88;
		int	B1,  C3;

		if( constrast > 0 )
			C88   = 32 + ((constrast*3)>>12);
		else
			C88   = 32 + (constrast>>11);

		B88 = ( brightness / 16 + 88 * ( 32 - C88 ));
		B1  = ((B88 - (16 * C88)) * 149);
		C3  = ((C88*102)<<16)|(C88 * 149);

		my_FskCopyYUV42016RGB565SE_scale_bc_prototype_arm_v4
		(
			width,
			height,
			Yrb,
			Crb,
			drb,
			Y0,
			U0,
			V0,
			d0,
			x0,
			y0,
			xd,
			yd,
			xCmax,
			yCmax,
			B1,
			C3
		);
	}
}

static void my_FskCopyYUV42016RGB565SE_scale_bc_arm_v5(const FskRectBlitParams *params)
{
	int		width	= params->dstWidth;
	int		height	= params->dstHeight;
	int		Yrb		= params->srcRowBytes;
	int		Crb		= Yrb >> 1;
	int		drb		= params->dstRowBytes;

	unsigned char	*Y0	= (UInt8*)(params->srcBaseAddr );
	unsigned char	*U0	= (UInt8*)(params->srcUBaseAddr);
	unsigned char	*V0	= (UInt8*)(params->srcVBaseAddr);
	unsigned short	*d0	= (unsigned short*)(params->dstBaseAddr);

	FskFixed		x0	= params->srcX0   >> (FWD_SUBBITS - ASM_SUBBITS);	/* The YUV patch API assumes 16.16 */
	FskFixed		y0	= params->srcY0   >> (FWD_SUBBITS - ASM_SUBBITS);
	FskFixed		xd	= params->srcXInc >> (FWD_SUBBITS - ASM_SUBBITS);
	FskFixed		yd	= params->srcYInc >> (FWD_SUBBITS - ASM_SUBBITS);

	FskFixed		xCmax	= ((params->srcWidth  >> 1) - 1) << ASM_SUBBITS;
	FskFixed		yCmax	= ((params->srcHeight >> 1) - 1) << ASM_SUBBITS;

	int	constrast  = params->contrast;
	int	brightness = params->brightness;
	int C88, B88;
	int	B1,  C3;

	if( constrast > 0 )
		C88   = 32 + ((constrast*3)>>12);
	else
		C88   = 32 + (constrast>>11);

	B88 = ( brightness / 16 + 88 * ( 32 - C88 ));
	B1  = ((B88 - (16 * C88)) * 149);
	C3  = ((C88*102)<<16)|(C88 * 149);

	my_FskCopyYUV42016RGB565SE_scale_bc_prototype_arm_v5
	(
		width,
		height,
		Yrb,
		Crb,
		drb,
		Y0,
		U0,
		V0,
		d0,
		x0,
		y0,
		xd,
		yd,
		xCmax,
		yCmax,
		B1,
		C3
	);
}


static void
find_src_clipped_addr(const FskRectBlitParams *params, unsigned char **Y, unsigned char **U, unsigned char **V )
{
	int				Yrb		= params->srcRowBytes;
	int				Crb		= Yrb >> 1;
	int				t;

	unsigned char	*Y0	= (UInt8*)(params->srcBaseAddr );
	unsigned char	*U0	= (UInt8*)(params->srcUBaseAddr);
	unsigned char	*V0	= (UInt8*)(params->srcVBaseAddr);
	unsigned char	*Yr, *Ur, *Vr;

	FskFixed		x0	= params->srcX0;									/* We can work with full precision in this leaf function */
	FskFixed		y0	= params->srcY0;
	FskFixed		x, y;

	FskFixed		xCmax	= ((params->srcWidth  >> 1) - 1) << FWD_SUBBITS;
	FskFixed		yCmax	= ((params->srcHeight >> 1) - 1) << FWD_SUBBITS;
	FskFixed		xC, yC;

	if( x0 == FWD_HALF && y0 == FWD_HALF )
	{
		*Y = Y0;
		*U = U0;
		*V = V0;
	}
	else
	{
		y = y0;
		t = (y >> FWD_SUBBITS);
		Yr = Y0 + t*Yrb;

		yC = (y - FWD_HALF) >> 1;
		if (yC < 0)
			yC = 0;
		else if (yC > yCmax)
			yC = yCmax;

		t  = (yC >> FWD_SUBBITS);
		Ur = U0 + t *Crb;
		Vr = V0 + t *Crb;

		x = x0;
		t = (x >> FWD_SUBBITS);
		*Y = Yr + t;

		xC = (x - FWD_HALF) >> 1;
		if (xC < 0)
			xC = 0;
		else if (xC > xCmax)
			xC = xCmax;

		t = (xC >> FWD_SUBBITS);
		*U = Ur + t;
		*V = Vr + t;
	}
}


void my_FskCopyYUV42016RGB565SE_unity_bc_arm_v4(const FskRectBlitParams *p)
{
	//if( (int)p->srcBaseAddr%2 == 0 && (int)p->dstBaseAddr%4 == 0 )
	if( (int)p->dstBaseAddr%4 == 0 )
	{
		int width  = min( p->srcWidth, p->dstWidth );
		int height = min( p->srcHeight, p->dstHeight );
		unsigned char *Y, *U, *V;

		// height must be a multiple of 2 or crash.
		height &= ~1;
		if (!height) return;

		find_src_clipped_addr(p, &Y, &U, &V );

		if( p->contrast == 0 &&  p->brightness == 0 )
			my_FskCopyYUV42016RGB565SE_unity_prototype_arm_v4
			(
				width,
				height,
				Y,
				p->srcRowBytes,
				U,
				V,
				p->srcRowBytes >> 1,
				p->dstBaseAddr,
				p->dstRowBytes
			);
		else
		{
			int	constrast  = p->contrast;
			int	brightness = p->brightness;
			int C88, B88;
			int	Bx,  Cx;

			if( constrast > 0 )
				C88   = 32 + ((constrast*3)>>12);
			else
				C88   = 32 + (constrast>>11);

			B88 = ( brightness / 16 + 88 * ( 32 - C88 ));
			Bx  = ((B88 - (16 * C88)) * 149);
			Cx  = ((C88*102)<<16)|(C88 * 149);

			my_FskCopyYUV42016RGB565SE_unity_bc_prototype_arm_v4
			(
				Y,
				U,
				V,
				p->dstBaseAddr,
				Bx,
				Cx,
				height,
				width,
				p->srcRowBytes,
				p->srcRowBytes >> 1,
				p->dstRowBytes
			);
		}
	}
	else
		my_FskCopyYUV42016RGB565SE_scale_bc_arm_v4(p);
}

void my_FskCopyYUV42016RGB565SE_unity_bc_arm_v5(const FskRectBlitParams *p)
{
	if( (int)p->dstBaseAddr%4 == 0 )
	{
		int width  = min( p->srcWidth, p->dstWidth );
		int height = min( p->srcHeight, p->dstHeight );
		unsigned char *Y, *U, *V;

		// height must be a multiple of 2 or crash.
		height &= ~1;
		if (!height) return;

		find_src_clipped_addr(p, &Y, &U, &V );

		{
			int	constrast  = p->contrast;
			int	brightness = p->brightness;
			int C88, B88;
			int	Bx,  Cx;

			if( constrast > 0 )
				C88   = 32 + ((constrast*3)>>12);
			else
				C88   = 32 + (constrast>>11);

			B88 = ( brightness / 16 + 88 * ( 32 - C88 ));
			Bx  = ((B88 - (16 * C88)) * 149);
			Cx  = ((C88*102)<<16)|(C88 * 149);

			//my_FskCopyYUV42016RGB565SE_unity_bc_dub_ver_prototype_arm_v5
			my_FskCopyYUV42016RGB565SE_unity_bc_prototype_arm_v5
			(
				Y,
				U,
				V,
				p->dstBaseAddr,
				Bx,
				Cx,
				height,
				width,
				p->srcRowBytes,
				p->srcRowBytes >> 1,
				p->dstRowBytes
			);
		}
	}
	else
		my_FskCopyYUV42016RGB565SE_scale_bc_arm_v5(p);
}


/********************************************************************************
 ********************************************************************************
 * InstallPatches
 ********************************************************************************
 ********************************************************************************/
//static FskRectTransferProc	procDispatchPatched[1] = { NULL };

#endif /* MY_YUV */


/********************************************************************************
 ********************************************************************************
 * InstallPatches
 ********************************************************************************
 ********************************************************************************/

#include "FskYUV420Copy.h"
#include "FskArch.h"

FskInstrumentedSimpleType(rectblit, rectblit);
#define mlog  FskrectblitPrintfMinimal
#define nlog  FskrectblitPrintfNormal
#define vlog  FskrectblitPrintfVerbose
#define dlog  FskrectblitPrintfDebug

#if (TARGET_OS_ANDROID || TARGET_OS_LINUX || defined( MARVELL_SOC_PXA168 ) || defined( SUPPORT_YUV420_WMMX_OPT ) || defined(ANDROID_PLATFORM) ) && TARGET_CPU_ARM
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static  pthread_once_t  g_once;
static  int  g_has_wmmx = 0;
void check_wmmx()
{
	char cpuinfo[4096];
	char *p = cpuinfo;
	int  cpuinfo_len = 0;
	int  fd;
	int	 i;

	fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) return;

	do
		cpuinfo_len = read(fd, cpuinfo, sizeof cpuinfo);
	while (cpuinfo_len < 0 && errno == EINTR);
	close(fd);

	for( i = 0; i < cpuinfo_len - 3; i++ )
	{
		if( p[i]=='w'&&p[i+1]=='m'&&p[i+2]=='m'&&p[i+3]=='x' )
		{
			dlog("found string wmmx !\n");
			g_has_wmmx = 1;
			break;
		}
	}
}

int FskHardwareGetARMCPU_Linux()
{
	int implementation = FSK_ARCH_ARM_V5;

	pthread_once(&g_once, check_wmmx);
	if( g_has_wmmx )
	{
		dlog("CPU support wmmx !\n");
		implementation = FSK_ARCH_XSCALE;
		//implementation = FSK_ARCH_ARM_V5;
	}

	return implementation;
}

#endif

#if TARGET_OS_ANDROID && TARGET_CPU_ARM

#include "cpu-features.h"

extern uint64_t    android_getCpuFeatures(void);

static int FskHardwareGetARMCPU_Android()
{
	uint64_t	features;
	int			implementation = FSK_ARCH_C;

	//dlog("getting arm cpu feature\n");

	if (android_getCpuFamily() != ANDROID_CPU_FAMILY_ARM)
	{
        //dlog("Not an ARM CPU !\n");
		implementation = FSK_ARCH_C;
        goto bail;
	}

    features = android_getCpuFeatures();
	//dlog("features: %x\n", (int)features);
    if ((features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON) )
	{
		//dlog("CPU support arm v7 and NEON !\n");
		implementation = FSK_ARCH_ARM_V7;
        goto bail;
    }

    if (features & ANDROID_CPU_ARM_FEATURE_ARMv7)//no neon
	{
       // dlog("CPU support arm v7 but no NEON, we treat it as v6!\n");
 		implementation = FSK_ARCH_ARM_V6;
		goto bail;
    }

    if ((features & ANDROID_CPU_ARM_FEATURE_ARMv6) )
	{
        //dlog("CPU support arm v6 !\n");
 		implementation = FSK_ARCH_ARM_V6;
		goto bail;
    }

    if ((features & ANDROID_CPU_ARM_FEATURE_ARMv5) )
	{
		//dlog("CPU support arm v5 !\n");
 		implementation = FSK_ARCH_ARM_V5;

		pthread_once(&g_once, check_wmmx);
		if( g_has_wmmx )
		{
			//dlog("CPU support wmmx !\n");
			implementation = FSK_ARCH_XSCALE;
			//implementation = FSK_ARCH_ARM_V5;
		}
		goto bail;
    }

	//bottom line
	//dlog("CPU only support arm v4 !\n");
	implementation = FSK_ARCH_ARM_V4;

bail:

	return implementation;
}
//#endif
#endif /* TARGET_OS_ANDROID && TARGET_CPU_ARM */

static int gForceThisArch = FSK_ARCH_AUTO;

int FskHardwareGetARMCPU_All(void)
{
	int implementation = FSK_ARCH_C;

	if( gForceThisArch > FSK_ARCH_AUTO )
		return gForceThisArch;

#if   TARGET_OS_ANDROID && TARGET_CPU_ARM
	implementation = FskHardwareGetARMCPU_Android();
#elif (TARGET_OS_LINUX || defined( MARVELL_SOC_PXA168 ) || defined( SUPPORT_YUV420_WMMX_OPT ) )  && (__XSCALE__)
	implementation = FskHardwareGetARMCPU_Linux();
#elif TARGET_OS_LINUX && TARGET_CPU_ARM
	implementation = FSK_ARCH_ARM_V5;
#elif TARGET_OS_IPHONE && TARGET_CPU_ARM
	implementation = FSK_ARCH_ARM_V7;
#else
	//implementation = FSK_ARCH_C;
#endif

	return implementation;
}


FskErr FskHardwareForceARMCPU(int arch)
{
	int cache_forced_arc;
	int real_arch;

	cache_forced_arc	= gForceThisArch;				//cache
	gForceThisArch		= FSK_ARCH_AUTO;				//get real system arch
	real_arch			= FskHardwareGetARMCPU_All();
	gForceThisArch		= cache_forced_arc;

	if( arch > real_arch )
		return kFskErrInvalidParameter;

	if( (real_arch != FSK_ARCH_XSCALE) && (arch==FSK_ARCH_XSCALE) ) //assuming no arm v6,v7 + wmmx devices
		return kFskErrInvalidParameter;

	gForceThisArch = arch;
	return kFskErrNone;
}

int  FskHardwareGetForcedARMCPU(void)
{
	return gForceThisArch;
}

#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
void
my_FskBilinearCopy32sameSrcDst_neon(const FskRectBlitParams *params)
{
	UInt32 widthBytes = params->dstWidth * 4, height = params->dstHeight;
	SInt32 srb = params->srcRowBytes, dBump = params->dstRowBytes - widthBytes;
	FskFixed x0 = params->srcX0, y = params->srcY0;
	FskFixed xd = params->srcXInc, yd = params->srcYInc;
	FskFixed di,dj;
	const UInt8 *s0 = (const UInt8*)(params->srcBaseAddr), *sr;
	const UInt32 *s;
	register UInt32 *d = (UInt32*)params->dstBaseAddr;
	register FskFixed x;
	register UInt32 p = 0, di0 = 0, dj0 = 0;
	register UInt32 width;

    dlog("into my_FskBilinearCopy32sameSrcDst_neon()\n");

#if !__clang__
	asm volatile(".fpu neon\n");
#endif
	for ( ; height--; y += yd, d = (UInt32*)(dBump + (char *)d)) {
		sr = s0 + (y >> 18) * srb;
		dj = (y >> (18 - 4)) & 0xF;
		width = widthBytes>>2;
		if(dj!=0) {
			dj0 = (1<<4) - dj;
			asm volatile (
				"vdup.16	d5,		%[dj0] 			\n"
				"vdup.16	d4,		%[dj] 			\n"
				"vdup.8		d13,	%[dj0] 			\n"
				"vdup.8		d12,	%[dj] 			\n"
				::[dj] "r" (dj),[dj0] "r" (dj0)
			);
			for (x = x0; width--; x += xd, d++) {
				s = (const UInt32*)(sr + (x >> 18) * 4);
				di = (x >> (18 - 4)) & 0xF;
				if(di!=0) {
					asm volatile(
					"vld1.32	d0,		[%[s]],	%[rb]	\n"
					"vld1.32	d1,		[%[s]]			\n"
					"vzip.32	d0,		d1				\n"
					"rsb		%[di0],	%[di],	#16		\n"
					"vdup.8		d3,		%[di0]			\n"
					"vdup.8		d2,		%[di]			\n"
					"vmull.u8	q4,		d0,		d3		\n"
					"vmlal.u8	q4,		d1,		d2		\n"
					"vmul.u16	d0,		d8,		d5		\n"
					"vmla.u16	d0,		d9,		d4		\n"
					"vrshr.u16	d0,		d0,		#8		\n"
					"vmovn.i16	d10,	q0				\n"
					"vmov.32	%[p],	d10[0]			\n"
					:[p] "=r" (p),[s] "+&r" (s)
					:[di] "r" (di),[rb] "r" (srb),[di0] "r" (di0)
					);
				}
				else {
					asm volatile(
					"ldr		%[p],	[%[s]]			\n"
					"ldr		%[di0],	[%[s],%[rb]]	\n"
					"vmov		d0,		%[p],	%[di0]	\n"
					"vext.8		d6,		d13,	d12, #4	\n"
					"vmull.u8	q4,		d0,		d6		\n"
					"vadd.u16	d8,		d8,		d9		\n"
					"vrshr.u16	d8,		d8,		#4		\n"
					"vmovn.i16	d10,	q4				\n"
					"vmov.32	%[p],	d10[0]			\n"
					:[p] "+&r" (p),[s] "+&r" (s)
					:[rb] "r" (srb),[di0] "r" (di0)
					);
				}
				*d = p;
			}
		}
		else {
			for (x = x0 ; width--; x += xd, d++) {
				s = (const UInt32*)(sr + (x >> 18) * 4);
				di = (x >> (18 - 4)) & 0xF;
				if(di!=0) {
					asm volatile(
					"vld1.32	d0,		[%[s]]			\n"
					"rsb		%[di0],	%[di],	#16		\n"
					"vdup.8		d3,		%[di0] 			\n"
					"vdup.8		d2,		%[di] 			\n"
					"vext.8		d6,		d3,		d2, #4	\n"
					"vmull.u8	q4,		d0,		d6		\n"
					"vadd.u16	d8,		d8,		d9		\n"
					"vrshr.u16	d8,		d8,		#4		\n"
					"vmovn.i16	d10,	q4				\n"
					"vmov.32	%[p],	d10[0]			\n"
					:[p] "=r" (p)
					:[di] "r" (di),[di0] "r" (di0),[s] "r" (s)
					);
				}
				else {
					p = *s;
				}
				*d = p;
			}
		}
	}
}
#endif //defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)


static void
InstallPatches(void)
{
#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS) || (defined(MY_YUV) && (SRC_YUV420i || SRC_YUV420))
	int implementation = FskHardwareGetARMCPU_All();
#endif

#if defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)
	if (implementation == FSK_ARCH_ARM_V7)
    {
		static const FskBitmapFormatEnum formats[] = { kFskBitmapFormat32ARGB, kFskBitmapFormat32ABGR, kFskBitmapFormat32BGRA, kFskBitmapFormat32RGBA };
		unsigned i;
		for (i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
        {
            dlog("patching my_FskBilinearCopy32sameSrcDst_neon, src: %d, dst: %d, mode: %d\n", formats[i], formats[i], kFskGraphicsModeCopy | kFskGraphicsModeBilinear);
			(void)FskPatchRectBlitProc(my_FskBilinearCopy32sameSrcDst_neon, formats[i], formats[i], kFskGraphicsModeCopy | kFskGraphicsModeBilinear, false);
        }
    }
#endif //defined(SUPPORT_NEON) || defined(SUPPORT_NEON_IOS)

#if defined(SUPPORT_WMMX)	&& defined(MARVELL_SOC_PXA168)//
	if( implementation == FSK_ARCH_XSCALE )
    {
        (void)FskPatchRectBlitProc(FskBilinearBlend16RGB565SE16RGB565SE_wmmx, kFskBitmapFormat16RGB565LE, kFskBitmapFormat16RGB565LE, kFskGraphicsModeCopy | kFskGraphicsModeBilinear, 1);
    }
	(void)FskPatchRectBlitProc(FskBilinearAlpha32A16RGB565SE16RGB565SE_wmmx, kFskBitmapFormat32A16RGB565LE,kFskBitmapFormat16RGB565LE,kFskGraphicsModeAlpha | kFskGraphicsModeBilinear, false);
    (void)FskPatchRectBlitProc(FskBilinearAlpha16RGB565SE16RGB565SE_wmmx, kFskBitmapFormat16RGB565LE, kFskBitmapFormat16RGB565LE, kFskGraphicsModeCopy, 1);
#endif

#ifdef MY_YUV	//
#if SRC_YUV420i	//
	ConfigCopyYUV420iProcs( implementation );

#ifdef SUPPORT_NEON	//
	if( implementation == FSK_ARCH_ARM_V7 )
	{
        dlog("FSK_ARCH_ARM_V7 NEON yuv420i to rgb is being implemented!\n");

		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v7,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v7,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v7,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v7,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v7 !\n");
		return;
	}
#endif	//defined(SUPPORT_NEON)

#ifndef SUPPORT_YUV420_WMMX_OPT			//no v6 for xscale device
	if( implementation >= FSK_ARCH_ARM_V6 )
	{
		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v6,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v6,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v6,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v6,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v6 !\n");
		return;
	}
#endif

#ifdef SUPPORT_WMMX	//
	if( implementation == FSK_ARCH_XSCALE )
	{
		FskPatchRectBlitProc(
								 my_FskCopyYUV420i16RGB565SE_scale_bc_arm_wmmx,
								 kFskBitmapFormatYUV420i,
								 kFskBitmapFormat16RGB565LE,
								 kFskGraphicsModeCopy,
								 false
								 );

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
		return;
	}
#endif	//defined(SUPPORT_WMMX)

	if( implementation >= FSK_ARCH_ARM_V5 )
	{
		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v5,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v5,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v5,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v5,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v5!\n");
		return;
	}

	//default is arm v4
	{
		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v4,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v4,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v4,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_arm_v4,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v4 !\n");
		return;
	}

#endif	//SRC_YUV420i


#if SRC_YUV420

#if (defined SUPPORT_WMMX ) && (defined SUPPORT_YUV420_WMMX_OPT)
        if( implementation == FSK_ARCH_XSCALE )
        {
			FskPatchRectBlitProc(
								my_FskCopyYUV42016RGB565SE_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

			FskPatchUnityRectBlitProc(
								my_FskCopyYUV42016RGB565SE_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

			FskPatchRectBlitProc(
								my_FskCopyYUV42032BGRA_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

			FskPatchUnityRectBlitProc(
								my_FskCopyYUV42032BGRA_scale_bc_arm_wmmx,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
			return;
        }
#endif  //defined(SUPPORT_WMMX)

	if( implementation >= FSK_ARCH_ARM_V5 )
	{
		FskPatchRectBlitProc(
								my_FskCopyYUV42016RGB565SE_scale_bc_arm_v5,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

								FskPatchUnityRectBlitProc(
								my_FskCopyYUV42016RGB565SE_unity_bc_arm_v5,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v5!\n");
		return;
	}

	//default is arm v4
	{
		FskPatchRectBlitProc(
								my_FskCopyYUV42016RGB565SE_scale_bc_arm_v4,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
							);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV42016RGB565SE_unity_bc_arm_v4,
								kFskBitmapFormatYUV420,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);
		dlog("patch with arm v4 !\n");
		return;
	}

#endif //SRC_YUV420

#else	//MY_YUV

//for windows and mac build, mostly for yuv420i verification purpose
#if SRC_YUV420i	//
		ConfigCopyYUV420iProcs_c();
#ifdef SUPPORT_NEON	//
		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_neon1_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_neon1_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_neon1_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_neon1_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
#else	//
		FskPatchRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i16RGB565SE_scale_bc_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat16RGB565LE,
								kFskGraphicsModeCopy,
								0
								);

		FskPatchRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								false
								);

		FskPatchUnityRectBlitProc(
								my_FskCopyYUV420i32BGRA_scale_bc_c,
								kFskBitmapFormatYUV420i,
								kFskBitmapFormat32BGRA,
								kFskGraphicsModeCopy,
								0
								);
#endif	//SUPPORT_NEON
#endif  //SRC_YUV420i

#endif	//MY_YUV
}
