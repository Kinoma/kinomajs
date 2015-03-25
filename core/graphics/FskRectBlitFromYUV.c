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
/* Only generate the procs if we're going to use them */
#if		FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat))						\
	&&	(FskName3(fsk,DstPixelKind,PixelPacking) == fskUniformChunkyPixelPacking)


/********************************************************************************
 ********************************************************************************
 **		SetTint
 ********************************************************************************
 ********************************************************************************/

#if   FskName3(fsk,DstPixelKind,Bytes) == 1
	#define SetTint(tint)	tint = ((params->red * fskRtoYCoeff + params->green * fskGtoYCoeff + params->blue * fskBtoYCoeff + (1 << (fskToYCoeffShift - 1))) >> fskToYCoeffShift)
#elif FskName3(fsk,DstPixelKind,Bytes) == 2
	#define SetTint(tint)	tint	= (		 params->red   >> (8 - FskName4(fsk,DstPixelKind,Red,Bits  )) << FskName4(fsk,DstPixelKind,Red,Position  ))	\
									| (		 params->green >> (8 - FskName4(fsk,DstPixelKind,Green,Bits)) << FskName4(fsk,DstPixelKind,Green,Position))	\
									| (		 params->blue  >> (8 - FskName4(fsk,DstPixelKind,Blue,Bits )) << FskName4(fsk,DstPixelKind,Blue,Position ))	\
									| (		(params->red   >> (8 - FskName4(fsk,DstPixelKind,Red,Bits  )) << FskName4(fsk,DstPixelKind,Red,Position  ))	\
										|	(params->green >> (8 - FskName4(fsk,DstPixelKind,Green,Bits)) << FskName4(fsk,DstPixelKind,Green,Position))	\
										|	(params->blue  >> (8 - FskName4(fsk,DstPixelKind,Blue,Bits )) << FskName4(fsk,DstPixelKind,Blue,Position ))	\
									  ) >> 16L
#elif FskName3(fsk,DstPixelKind,Bytes) == 3
	#define SetTint(tint)	tint.c[FskName4(fsk,DstPixelKind,Red,Position    )] = params->red,		\
							tint.c[FskName4(fsk,DstPixelKind,Green,Position  )] = params->green,	\
							tint.c[FskName4(fsk,DstPixelKind,Blue,Position   )] = params->blue
#elif FskName3(fsk,DstPixelKind,Bytes) == 4
	#define SetTint(tint)	tint	= (params->red   << FskName4(fsk,DstPixelKind,Red,Position  ))	\
									| (params->green << FskName4(fsk,DstPixelKind,Green,Position))	\
									| (params->blue  << FskName4(fsk,DstPixelKind,Blue,Position ))
#else
	#error Unsupported pixel kind DstPixelKind
#endif


/********************************************************************************
 ********************************************************************************
 **		Rect Procs
 ********************************************************************************
 ********************************************************************************/





/********************************************************************************
 * Copy
 ********************************************************************************/

static void
FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	YUV_BLIT_STARTLOOP {
		*d = FskName3(FskConvert,SrcPixelKind,DstPixelKind)(*Y, *U, *V);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Copy
 ********************************************************************************/

static void
FskName4(FskBilinear,Copy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	YUV_BLIT_STARTLOOP {
		UInt8	cy		= FskBilerp8(  xf,  yf, Y,    Yrb);
		UInt16	cuv		= FskBilerpUV(xcf, ycf, U, V, Crb);
		*d = FskName3(FskConvert,SrcPixelKind,DstPixelKind)(cy, (UInt8)(cuv>>8), (UInt8)cuv);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************
 * Blend
 ********************************************************************************/

static void
FskName4(Fsk,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	UInt8 alpha = params->alpha;
	YUV_BLIT_STARTLOOP {
		FskName2(FskBlend,DstPixelKind)(d, FskName3(FskConvert,SrcPixelKind,DstPixelKind)(*Y, *U, *V), alpha);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear Blend
 ********************************************************************************/

static void
FskName4(FskBilinear,Blend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	UInt8 alpha = params->alpha;
	YUV_BLIT_STARTLOOP {
		UInt8	cy		= FskBilerp8(  xf,  yf, Y,    Yrb);
		UInt16	cuv		= FskBilerpUV(xcf, ycf, U, V, Crb);
		FskName2(FskBlend,DstPixelKind)(d, FskName3(FskConvert,SrcPixelKind,DstPixelKind)(cy, (UInt8)(cuv>>8), (UInt8)cuv), alpha);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************
 * TintCopy
 ********************************************************************************/

static void
FskName4(Fsk,TintCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	FskName3(Fsk,DstPixelKind,Type) tint;
	SetTint(tint);
	YUV_BLIT_STARTLOOP {
		*d = FskName2(FskPixelMul,DstPixelKind)(FskName3(FskConvert,SrcPixelKind,DstPixelKind)(*Y, *U, *V), tint);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************
 * Bilinear TintCopy
 ********************************************************************************/

static void
FskName4(FskBilinear,TintCopy,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)
{
	YUV_BLIT_DECL;
	FskName3(Fsk,DstPixelKind,Type) tint;
	SetTint(tint);
	YUV_BLIT_STARTLOOP {
		UInt8	cy		= FskBilerp8(  xf,  yf, Y,    Yrb);
		UInt16	cuv		= FskBilerpUV(xcf, ycf, U, V, Crb);
		*d = FskName2(FskPixelMul,DstPixelKind)(FskName3(FskConvert,SrcPixelKind,DstPixelKind)(cy, (UInt8)(cuv>>8), (UInt8)cuv), tint);
	} YUV_BLIT_ENDLOOP
}


/********************************************************************************/


/* This takes up a few more bytes than #defines, but is easier to write code for */
static void FskName4(Fsk,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)				{ FskName4(Fsk,Copy,SrcPixelKind,DstPixelKind)(params);				}
static void FskName4(FskBilinear,Alpha,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)		{ FskName4(FskBilinear,Copy,SrcPixelKind,DstPixelKind)(params);		}
static void FskName4(Fsk,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)			{ FskName4(Fsk,Blend,SrcPixelKind,DstPixelKind)(params);			}
static void FskName4(FskBilinear,AlphaBlend,SrcPixelKind,DstPixelKind)(const FskRectBlitParams *params)	{ FskName4(FskBilinear,Blend,SrcPixelKind,DstPixelKind)(params);	}


#undef SetTint


#endif /* FskName2(DST_,FskName3(fsk,DstPixelKind,KindFormat)) */


#undef SrcPixelKind
#undef DstPixelKind

